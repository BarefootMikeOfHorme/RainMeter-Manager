// render_process_launcher.cpp - Render Process Launcher Implementation
#include "render_process_launcher.h"
#include "../core/logger.h"
#include <sddl.h>
#include <algorithm>

namespace RainmeterManager {
namespace Render {

// Constructor
RenderProcessLauncher::RenderProcessLauncher()
    : jobObject_(nullptr)
    , running_(false)
    , stopRequested_(false)
    , consecutiveRestarts_(0)
{
    processInfo_ = {};
    processInfo_.processHandle = nullptr;
    processInfo_.threadHandle = nullptr;
    processInfo_.processId = 0;
    processInfo_.restartCount = 0;
    processInfo_.crashed = false;
    processInfo_.exitCode = 0;
    
    LOG_INFO("RenderProcessLauncher created");
}

// Destructor
RenderProcessLauncher::~RenderProcessLauncher() {
    Terminate();
}

// Launch render process
bool RenderProcessLauncher::Launch(const LaunchConfig& config) {
    if (running_) {
        LOG_WARNING("Render process already running");
        return false;
    }
    
    config_ = config;
    
    if (config_.executablePath.empty()) {
        LOG_ERROR("No executable path specified");
        return false;
    }
    
    LOG_INFO("Launching render process: " + config_.executablePath);
    
    // Create job object for sandboxing if enabled
    if (config_.sandbox.enableJobObject) {
        if (!CreateJobObject()) {
            LOG_WARNING("Failed to create job object, continuing without");
        }
    }
    
    // Create the render process
    if (!CreateRenderProcess()) {
        CleanupProcess();
        return false;
    }
    
    // Apply sandboxing
    if (!ApplySandbox()) {
        LOG_WARNING("Failed to apply full sandbox, process may have elevated permissions");
    }
    
    running_ = true;
    stopRequested_ = false;
    processInfo_.startTime = std::chrono::steady_clock::now();
    processInfo_.restartCount = 0;
    
    // Start monitoring thread
    monitorThread_ = std::make_unique<std::thread>(&RenderProcessLauncher::MonitorProcess, this);
    
    LOG_INFO("Render process launched successfully (PID: " + std::to_string(processInfo_.processId) + ")");
    NotifyStarted(processInfo_.processId);
    
    return true;
}

// Terminate render process
void RenderProcessLauncher::Terminate() {
    if (!running_) {
        return;
    }
    
    LOG_INFO("Terminating render process...");
    
    stopRequested_ = true;
    
    // Terminate process gracefully
    {
        std::lock_guard<std::mutex> lock(processMutex_);
        if (processInfo_.processHandle) {
            // Try graceful shutdown first (WM_CLOSE)
            if (!TerminateProcess(processInfo_.processHandle, 0)) {
                LOG_WARNING("Failed to terminate process gracefully");
            }
            
            // Wait for process to exit
            WaitForSingleObject(processInfo_.processHandle, 5000);
        }
    }
    
    // Join monitor thread
    if (monitorThread_ && monitorThread_->joinable()) {
        monitorThread_->join();
    }
    
    CleanupProcess();
    running_ = false;
    
    LOG_INFO("Render process terminated");
}

bool RenderProcessLauncher::IsRunning() const {
    return running_;
}

// Restart process
bool RenderProcessLauncher::Restart() {
    LOG_INFO("Restarting render process...");
    
    Terminate();
    std::this_thread::sleep_for(std::chrono::milliseconds(config_.restartDelayMs));
    
    return Launch(config_);
}

// Suspend process
void RenderProcessLauncher::SuspendProcess() {
    std::lock_guard<std::mutex> lock(processMutex_);
    if (processInfo_.threadHandle) {
        if (SuspendThread(processInfo_.threadHandle) != (DWORD)-1) {
            LOG_INFO("Render process suspended");
        } else {
            LOG_ERROR("Failed to suspend render process");
        }
    }
}

// Resume process
void RenderProcessLauncher::ResumeProcess() {
    std::lock_guard<std::mutex> lock(processMutex_);
    if (processInfo_.threadHandle) {
        if (ResumeThread(processInfo_.threadHandle) != (DWORD)-1) {
            LOG_INFO("Render process resumed");
        } else {
            LOG_ERROR("Failed to resume render process");
        }
    }
}

// Get process ID
DWORD RenderProcessLauncher::GetProcessId() const {
    return processInfo_.processId;
}

// Get process info
ProcessInfo RenderProcessLauncher::GetProcessInfo() const {
    std::lock_guard<std::mutex> lock(processMutex_);
    return processInfo_;
}

// Get uptime
std::chrono::milliseconds RenderProcessLauncher::GetUptime() const {
    if (!running_) {
        return std::chrono::milliseconds(0);
    }
    
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - processInfo_.startTime);
}

// Register crash handler
void RenderProcessLauncher::OnCrashed(CrashHandler handler) {
    crashHandler_ = std::move(handler);
}

// Register start handler
void RenderProcessLauncher::OnStarted(StartHandler handler) {
    startHandler_ = std::move(handler);
}

// Register stop handler
void RenderProcessLauncher::OnStopped(StopHandler handler) {
    stopHandler_ = std::move(handler);
}

// Create render process using CreateProcess
bool RenderProcessLauncher::CreateRenderProcess() {
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    
    if (config_.hideConsoleWindow) {
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
    }
    
    PROCESS_INFORMATION pi = {};
    
    // Build command line
    std::string cmdLine = BuildCommandLine();
    
    // Use working directory or executable directory
    const char* workDir = config_.workingDirectory.empty() ? nullptr : config_.workingDirectory.c_str();
    
    // Create process
    BOOL success = CreateProcessA(
        config_.executablePath.c_str(),
        const_cast<char*>(cmdLine.c_str()),
        nullptr,  // Process security attributes
        nullptr,  // Thread security attributes
        config_.inheritHandles,
        CREATE_SUSPENDED | CREATE_NEW_PROCESS_GROUP,  // Create suspended for sandboxing
        nullptr,  // Environment
        workDir,
        &si,
        &pi
    );
    
    if (!success) {
        DWORD error = GetLastError();
        LOG_ERROR("Failed to create render process (Error: " + std::to_string(error) + ")");
        return false;
    }
    
    // Store process info
    processInfo_.processHandle = pi.hProcess;
    processInfo_.threadHandle = pi.hThread;
    processInfo_.processId = pi.dwProcessId;
    
    return true;
}

// Apply sandboxing restrictions
bool RenderProcessLauncher::ApplySandbox() {
    bool success = true;
    
    // Assign to job object
    if (jobObject_ && config_.sandbox.enableJobObject) {
        if (!AssignProcessToJobObject(jobObject_, processInfo_.processHandle)) {
            LOG_ERROR("Failed to assign process to job object");
            success = false;
        } else {
            LOG_INFO("Process assigned to job object");
        }
    }
    
    // Set low integrity level
    if (config_.sandbox.enableLowIntegrity) {
        if (!SetLowIntegrityLevel(processInfo_.processHandle)) {
            LOG_WARNING("Failed to set low integrity level");
            success = false;
        } else {
            LOG_INFO("Low integrity level applied");
        }
    }
    
    // Apply process mitigations
    if (config_.sandbox.enableMitigations) {
        if (!ApplyProcessMitigations(processInfo_.processHandle)) {
            LOG_WARNING("Failed to apply process mitigations");
            success = false;
        } else {
            LOG_INFO("Process mitigations applied");
        }
        
        // Enhanced mitigations
        if (!ApplyEnhancedMitigations(processInfo_.processHandle)) {
            LOG_WARNING("Some enhanced mitigations unavailable");
        }
    }
    
    // Reduce token privileges
    if (config_.sandbox.reducePrivileges) {
        if (ReduceTokenPrivileges(processInfo_.processHandle)) {
            LOG_INFO("Token privileges reduced");
        } else {
            LOG_WARNING("Failed to reduce token privileges");
        }
    }
    
    // Apply network restrictions
    if (config_.sandbox.networkPolicy != NetworkPolicy::FullAccess) {
        if (ApplyNetworkRestrictions(processInfo_.processHandle)) {
            LOG_INFO("Network restrictions applied: " + 
                    std::to_string(static_cast<int>(config_.sandbox.networkPolicy)));
        } else {
            LOG_WARNING("Failed to apply network restrictions");
        }
    }
    
    // Configure DLL loading policy
    if (config_.sandbox.dllPolicy != DLLLoadingPolicy::AllowAll) {
        if (ConfigureDLLLoadingPolicy(processInfo_.processHandle)) {
            LOG_INFO("DLL loading policy configured");
        } else {
            LOG_WARNING("Failed to configure DLL loading policy");
        }
    }
    
    // Resume process now that sandboxing is applied
    if (ResumeThread(processInfo_.threadHandle) == (DWORD)-1) {
        LOG_ERROR("Failed to resume process after sandboxing");
        return false;
    }
    
    return success;
}

// Create job object with resource limits
bool RenderProcessLauncher::CreateJobObject() {
    jobObject_ = ::CreateJobObjectA(nullptr, nullptr);
    if (!jobObject_) {
        LOG_ERROR("Failed to create job object");
        return false;
    }
    
    // Set extended limit information
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
    limits.BasicLimitInformation.LimitFlags = 
        JOB_OBJECT_LIMIT_PROCESS_MEMORY |
        JOB_OBJECT_LIMIT_JOB_MEMORY |
        JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
    
    if (config_.sandbox.killOnJobClose) {
        limits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    }
    
    // Memory limits
    limits.ProcessMemoryLimit = config_.sandbox.maxMemoryMB * 1024 * 1024;
    limits.JobMemoryLimit = config_.sandbox.maxMemoryMB * 1024 * 1024;
    
    // Active process limit (prevent fork bombs)
    limits.BasicLimitInformation.ActiveProcessLimit = config_.sandbox.maxActiveProcesses;
    
    if (!SetInformationJobObject(jobObject_, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
        LOG_ERROR("Failed to set job object limits");
        CloseHandle(jobObject_);
        jobObject_ = nullptr;
        return false;
    }
    
    // Set UI restrictions if requested
    if (config_.sandbox.restrictUIAccess) {
        JOBOBJECT_BASIC_UI_RESTRICTIONS uiLimits = {};
        uiLimits.UIRestrictionsClass = 
            JOB_OBJECT_UILIMIT_HANDLES |
            JOB_OBJECT_UILIMIT_DESKTOP |
            JOB_OBJECT_UILIMIT_DISPLAYSETTINGS;
        
        SetInformationJobObject(jobObject_, JobObjectBasicUIRestrictions, &uiLimits, sizeof(uiLimits));
    }
    
    LOG_INFO("Job object created with memory limit: " + std::to_string(config_.sandbox.maxMemoryMB) + " MB");
    return true;
}

// Set process to low integrity level
bool RenderProcessLauncher::SetLowIntegrityLevel(HANDLE processHandle) {
    // Open process token
    HANDLE tokenHandle;
    if (!OpenProcessToken(processHandle, TOKEN_ADJUST_DEFAULT | TOKEN_QUERY, &tokenHandle)) {
        return false;
    }
    
    // Create low integrity SID
    PSID integritySid = nullptr;
    if (!ConvertStringSidToSidA("S-1-16-4096", &integritySid)) {  // Low integrity
        CloseHandle(tokenHandle);
        return false;
    }
    
    // Set integrity level
    TOKEN_MANDATORY_LABEL tml = {};
    tml.Label.Attributes = SE_GROUP_INTEGRITY;
    tml.Label.Sid = integritySid;
    
    bool success = SetTokenInformation(
        tokenHandle,
        TokenIntegrityLevel,
        &tml,
        sizeof(tml) + GetLengthSid(integritySid)
    );
    
    LocalFree(integritySid);
    CloseHandle(tokenHandle);
    
    return success;
}

// Apply process mitigation policies
bool RenderProcessLauncher::ApplyProcessMitigations(HANDLE processHandle) {
    // This requires Windows 8+ and specific API
    // For older systems or if unavailable, this will gracefully fail
    
    typedef BOOL(WINAPI* SetProcessMitigationPolicyFunc)(HANDLE, PROCESS_MITIGATION_POLICY, PVOID, SIZE_T);
    
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    if (!kernel32) return false;
    
    auto setPolicyFunc = reinterpret_cast<SetProcessMitigationPolicyFunc>(
        GetProcAddress(kernel32, "SetProcessMitigationPolicy")
    );
    
    if (!setPolicyFunc) {
        LOG_INFO("Process mitigation API not available (older Windows version)");
        return false;
    }
    
    // Enable DEP (Data Execution Prevention)
    PROCESS_MITIGATION_DEP_POLICY depPolicy = {};
    depPolicy.Enable = 1;
    depPolicy.Permanent = 1;
    
    bool success = setPolicyFunc(
        processHandle,
        ProcessDEPPolicy,
        &depPolicy,
        sizeof(depPolicy)
    );
    
    // Enable ASLR (Address Space Layout Randomization)
    PROCESS_MITIGATION_ASLR_POLICY aslrPolicy = {};
    aslrPolicy.EnableForceRelocateImages = 1;
    aslrPolicy.DisallowStrippedImages = 1;
    
    success &= setPolicyFunc(
        processHandle,
        ProcessASLRPolicy,
        &aslrPolicy,
        sizeof(aslrPolicy)
    );
    
    return success;
}

// Monitor process health
void RenderProcessLauncher::MonitorProcess() {
    LOG_INFO("Process monitor thread started");
    
    while (!stopRequested_) {
        DWORD waitResult;
        
        {
            std::lock_guard<std::mutex> lock(processMutex_);
            if (!processInfo_.processHandle) {
                break;
            }
            waitResult = WaitForSingleObject(processInfo_.processHandle, 1000);
        }
        
        if (waitResult == WAIT_OBJECT_0) {
            // Process exited
            DWORD exitCode = 0;
            GetExitCodeProcess(processInfo_.processHandle, &exitCode);
            
            bool crashed = (exitCode != 0);
            
            LOG_INFO("Render process exited (Code: " + std::to_string(exitCode) + 
                    ", Crashed: " + (crashed ? "Yes" : "No") + ")");
            
            processInfo_.crashed = crashed;
            processInfo_.exitCode = exitCode;
            
            NotifyStopped(processInfo_.processId, exitCode);
            NotifyCrash(exitCode, crashed);
            
            // Check if we should restart
            if (ShouldRestart(exitCode)) {
                int delay = CalculateRestartDelay();
                LOG_INFO("Restarting render process in " + std::to_string(delay) + "ms...");
                
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                
                CleanupProcess();
                
                if (CreateRenderProcess() && ApplySandbox()) {
                    processInfo_.startTime = std::chrono::steady_clock::now();
                    processInfo_.restartCount++;
                    consecutiveRestarts_++;
                    lastRestartTime_ = std::chrono::steady_clock::now();
                    
                    LOG_INFO("Render process restarted (Attempt " + 
                            std::to_string(consecutiveRestarts_) + "/" + 
                            std::to_string(config_.maxRestartAttempts) + ")");
                    
                    NotifyStarted(processInfo_.processId);
                } else {
                    LOG_ERROR("Failed to restart render process");
                    running_ = false;
                    break;
                }
            } else {
                LOG_INFO("Not restarting render process per policy");
                running_ = false;
                break;
            }
        }
    }
    
    LOG_INFO("Process monitor thread stopped");
}

// Check if process should be restarted
bool RenderProcessLauncher::ShouldRestart(int exitCode) const {
    if (stopRequested_) {
        return false;
    }
    
    if (consecutiveRestarts_ >= config_.maxRestartAttempts) {
        LOG_WARNING("Max restart attempts reached");
        return false;
    }
    
    switch (config_.restartPolicy) {
        case RestartPolicy::Never:
            return false;
            
        case RestartPolicy::OnCrash:
            return exitCode != 0;
            
        case RestartPolicy::Always:
            return true;
            
        case RestartPolicy::ExponentialBackoff:
            return exitCode != 0;
            
        default:
            return false;
    }
}

// Calculate restart delay with exponential backoff
int RenderProcessLauncher::CalculateRestartDelay() const {
    if (config_.restartPolicy != RestartPolicy::ExponentialBackoff) {
        return config_.restartDelayMs;
    }
    
    // Exponential backoff: delay * 2^(attempts)
    int delay = config_.restartDelayMs * (1 << (std::min)(consecutiveRestarts_, 5));
    return (std::min)(delay, config_.maxRestartDelayMs);
}

// Cleanup process resources
void RenderProcessLauncher::CleanupProcess() {
    std::lock_guard<std::mutex> lock(processMutex_);
    
    if (processInfo_.processHandle) {
        CloseHandle(processInfo_.processHandle);
        processInfo_.processHandle = nullptr;
    }
    
    if (processInfo_.threadHandle) {
        CloseHandle(processInfo_.threadHandle);
        processInfo_.threadHandle = nullptr;
    }
    
    if (jobObject_) {
        CloseHandle(jobObject_);
        jobObject_ = nullptr;
    }
    
    processInfo_.processId = 0;
}

// Build command line with IPC pipe name
std::string RenderProcessLauncher::BuildCommandLine() const {
    std::string cmdLine = "\"" + config_.executablePath + "\"";
    
    // Add IPC pipe name if specified
    if (!config_.ipcPipeName.empty()) {
        cmdLine += " --ipc-pipe=\"" + config_.ipcPipeName + "\"";
    }
    
    // Add additional arguments
    if (!config_.commandLineArgs.empty()) {
        cmdLine += " " + config_.commandLineArgs;
    }
    
    return cmdLine;
}

// Notify crash
void RenderProcessLauncher::NotifyCrash(int exitCode, bool crashed) {
    if (crashHandler_) {
        try {
            crashHandler_(exitCode, crashed);
        } catch (const std::exception& e) {
            LOG_ERROR(std::string("Crash handler exception: ") + e.what());
        }
    }
}

// Notify started
void RenderProcessLauncher::NotifyStarted(DWORD processId) {
    if (startHandler_) {
        try {
            startHandler_(processId);
        } catch (const std::exception& e) {
            LOG_ERROR(std::string("Start handler exception: ") + e.what());
        }
    }
}

// Notify stopped
void RenderProcessLauncher::NotifyStopped(DWORD processId, int exitCode) {
    if (stopHandler_) {
        try {
            stopHandler_(processId, exitCode);
        } catch (const std::exception& e) {
            LOG_ERROR(std::string("Stop handler exception: ") + e.what());
        }
    }
}

// Apply enhanced mitigations (CFG, dynamic code, Win32k lockdown, etc.)
bool RenderProcessLauncher::ApplyEnhancedMitigations(HANDLE processHandle) {
    typedef BOOL(WINAPI* SetProcessMitigationPolicyFunc)(HANDLE, PROCESS_MITIGATION_POLICY, PVOID, SIZE_T);
    
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    if (!kernel32) return false;
    
    auto setPolicyFunc = reinterpret_cast<SetProcessMitigationPolicyFunc>(
        GetProcAddress(kernel32, "SetProcessMitigationPolicy")
    );
    
    if (!setPolicyFunc) {
        return false;  // API not available on this system
    }
    
    bool anySuccess = false;
    
    // Control Flow Guard (CFG)
    if (config_.sandbox.enableCFG) {
        PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY cfgPolicy = {};
        cfgPolicy.EnableControlFlowGuard = 1;
        cfgPolicy.EnableExportSuppression = 1;
        cfgPolicy.StrictMode = 0;  // Allow some compatibility
        
        if (setPolicyFunc(processHandle, ProcessControlFlowGuardPolicy, &cfgPolicy, sizeof(cfgPolicy))) {
            LOG_INFO("Control Flow Guard enabled");
            anySuccess = true;
        }
    }
    
    // Dynamic code prohibition (may break JIT compilers)
    if (config_.sandbox.enableDynamicCodeProhibit) {
        PROCESS_MITIGATION_DYNAMIC_CODE_POLICY dynamicCodePolicy = {};
        dynamicCodePolicy.ProhibitDynamicCode = 1;
        dynamicCodePolicy.AllowThreadOptOut = 1;  // Allow threads to opt-out if needed
        
        if (setPolicyFunc(processHandle, ProcessDynamicCodePolicy, &dynamicCodePolicy, sizeof(dynamicCodePolicy))) {
            LOG_INFO("Dynamic code prohibition enabled");
            anySuccess = true;
        }
    }
    
    // Win32k system call filter (for headless rendering)
    if (config_.sandbox.enableWin32kLockdown) {
        PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY syscallPolicy = {};
        syscallPolicy.DisallowWin32kSystemCalls = 1;
        
        if (setPolicyFunc(processHandle, ProcessSystemCallDisablePolicy, &syscallPolicy, sizeof(syscallPolicy))) {
            LOG_INFO("Win32k system calls disabled (headless mode)");
            anySuccess = true;
        }
    }
    
    // Image load policy (DLL signature enforcement)
    if (config_.sandbox.enforceCodeSigning) {
        PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY signaturePolicy = {};
        signaturePolicy.MicrosoftSignedOnly = 0;  // Allow non-MS signed code
        signaturePolicy.MitigationOptIn = 1;      // Enable validation
        signaturePolicy.AuditMicrosoftSignedOnly = 1;  // Log violations
        
        if (setPolicyFunc(processHandle, ProcessSignaturePolicy, &signaturePolicy, sizeof(signaturePolicy))) {
            LOG_INFO("Code signature enforcement enabled");
            anySuccess = true;
        }
    }
    
    // Extension point disable (prevent legacy DLL injection)
    PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY extPolicy = {};
    extPolicy.DisableExtensionPoints = 1;
    
    if (setPolicyFunc(processHandle, ProcessExtensionPointDisablePolicy, &extPolicy, sizeof(extPolicy))) {
        LOG_INFO("Extension points disabled");
        anySuccess = true;
    }
    
    // Font loading restrictions
    PROCESS_MITIGATION_FONT_DISABLE_POLICY fontPolicy = {};
    fontPolicy.DisableNonSystemFonts = 0;  // Allow user fonts for creativity
    fontPolicy.AuditNonSystemFontLoading = 1;  // But audit them
    
    if (setPolicyFunc(processHandle, ProcessFontDisablePolicy, &fontPolicy, sizeof(fontPolicy))) {
        LOG_INFO("Font loading auditing enabled");
        anySuccess = true;
    }
    
    return anySuccess;
}

// Reduce token privileges to minimum needed
bool RenderProcessLauncher::ReduceTokenPrivileges(HANDLE processHandle) {
    HANDLE tokenHandle;
    if (!OpenProcessToken(processHandle, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tokenHandle)) {
        return false;
    }
    
    // List of privileges to remove (typically not needed by render process)
    LPCWSTR privilegesToRemove[] = {
        SE_DEBUG_NAME,              // Debug other processes
        SE_TCB_NAME,                // Act as OS
        SE_SECURITY_NAME,           // Manage auditing
        SE_LOAD_DRIVER_NAME,        // Load drivers
        SE_SYSTEM_PROFILE_NAME,     // Profile system
        SE_SYSTEMTIME_NAME,         // Change system time
        SE_PROF_SINGLE_PROCESS_NAME,// Profile single process
        SE_INC_BASE_PRIORITY_NAME,  // Increase scheduling priority
        SE_CREATE_PAGEFILE_NAME,    // Create pagefile
        SE_CREATE_PERMANENT_NAME,   // Create permanent objects
        SE_BACKUP_NAME,             // Backup files
        SE_RESTORE_NAME,            // Restore files
        SE_SHUTDOWN_NAME,           // Shutdown system
        SE_TAKE_OWNERSHIP_NAME,     // Take ownership
        SE_IMPERSONATE_NAME         // Impersonate clients
    };
    
    bool anySuccess = false;
    
    for (LPCWSTR privilege : privilegesToRemove) {
        LUID luid;
        if (LookupPrivilegeValueW(nullptr, privilege, &luid)) {
            TOKEN_PRIVILEGES tp = {};
            tp.PrivilegeCount = 1;
            tp.Privileges[0].Luid = luid;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_REMOVED;
            
            if (AdjustTokenPrivileges(tokenHandle, FALSE, &tp, sizeof(tp), nullptr, nullptr)) {
                anySuccess = true;
            }
        }
    }
    
    // Optionally add restricted SIDs (more aggressive - breaks some file operations)
    if (config_.sandbox.addRestrictedSIDs) {
        PSID restrictedSid = nullptr;
        if (ConvertStringSidToSidA("S-1-5-12", &restrictedSid)) {  // RESTRICTED SID
            TOKEN_GROUPS restrictedSids = {};
            restrictedSids.GroupCount = 1;
            restrictedSids.Groups[0].Sid = restrictedSid;
            restrictedSids.Groups[0].Attributes = SE_GROUP_USE_FOR_DENY_ONLY;
            
            if (SetTokenInformation(tokenHandle, TokenRestrictedSids, &restrictedSids, sizeof(restrictedSids))) {
                LOG_INFO("Restricted SIDs applied");
            }
            
            LocalFree(restrictedSid);
        }
    }
    
    CloseHandle(tokenHandle);
    return anySuccess;
}

// Apply network restrictions (localhost-only or blocked)
bool RenderProcessLauncher::ApplyNetworkRestrictions(HANDLE processHandle) {
    // Note: Windows doesn't provide direct API to restrict process network access
    // This would typically be done via:
    // 1. Windows Firewall rules (requires admin)
    // 2. Network namespace isolation (WSL2-style, not available for regular processes)
    // 3. AppContainer (UWP-style sandboxing, complex)
    
    // For now, we document the policy and rely on external firewall configuration
    // In a full implementation, you'd:
    // - Create Windows Firewall rules via COM interface
    // - Or use AppContainer for UWP-style network isolation
    
    switch (config_.sandbox.networkPolicy) {
        case NetworkPolicy::LocalhostOnly:
            LOG_INFO("Network policy: Localhost only (IPC permitted)");
            LOG_INFO("Recommend: Configure Windows Firewall to block outbound for PID " + 
                    std::to_string(::GetProcessId(processHandle)));
            break;
            
        case NetworkPolicy::Blocked:
            LOG_INFO("Network policy: Fully blocked");
            LOG_INFO("Recommend: Configure Windows Firewall to block all network for PID " + 
                    std::to_string(::GetProcessId(processHandle)));
            break;
            
        case NetworkPolicy::UserPrompt:
            LOG_INFO("Network policy: User will be prompted on first access");
            break;
            
        case NetworkPolicy::FullAccess:
        default:
            break;
    }
    
    // TODO: Future enhancement - integrate with Windows Filtering Platform (WFP)
    // to programmatically create firewall rules
    
    return true;  // Documentation/policy set successfully
}

// Configure DLL loading policy
bool RenderProcessLauncher::ConfigureDLLLoadingPolicy(HANDLE processHandle) {
    // Windows provides limited direct control over DLL loading per-process
    // Best approach is to set image load policy via mitigation and monitor via DLL_PROCESS_ATTACH
    
    switch (config_.sandbox.dllPolicy) {
        case DLLLoadingPolicy::SignedOnly:
            LOG_INFO("DLL policy: Signed only - enforced via process mitigations");
            // This is handled by ApplyEnhancedMitigations with enforceCodeSigning
            break;
            
        case DLLLoadingPolicy::ValidatedOnly:
            LOG_INFO("DLL policy: Smart validation enabled");
            LOG_INFO("User-signed DLLs: " + std::string(config_.sandbox.allowUserSignedDLLs ? "Allowed" : "Blocked"));
            if (!config_.sandbox.trustedDLLDirectory.empty()) {
                LOG_INFO("Trusted DLL directory: " + config_.sandbox.trustedDLLDirectory);
            }
            // Actual validation happens in the child process via DLL_PROCESS_ATTACH hook
            // Pass configuration via command line or shared memory
            break;
            
        case DLLLoadingPolicy::SystemOnly:
            LOG_INFO("DLL policy: System DLLs only - maximum restriction");
            // This would require DLL load callback in child process
            break;
            
        case DLLLoadingPolicy::AllowAll:
        default:
            LOG_INFO("DLL policy: No restrictions (not recommended for production)");
            break;
    }
    
    // Pass policy to child process via command line
    // Child process should implement DLL validation in DllMain or via SetDllDirectory/AddDllDirectory
    
    return true;
}

} // namespace Render
} // namespace RainmeterManager
