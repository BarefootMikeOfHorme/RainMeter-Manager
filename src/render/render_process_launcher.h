// render_process_launcher.h - Render Process Launcher with Sandboxing
#pragma once

#include <functional>
#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include <string>
#include <chrono>
#include <windows.h>

namespace RainmeterManager {
namespace Render {

// Process restart policy
enum class RestartPolicy {
    Never,              // Don't restart on crash
    OnCrash,            // Restart only on crash
    Always,             // Always restart
    ExponentialBackoff  // Restart with increasing delay
};

// Network policy
enum class NetworkPolicy {
    FullAccess,         // No restrictions
    LocalhostOnly,      // Only IPC/localhost (default for render process)
    Blocked,            // No network access
    UserPrompt          // Ask user on first network attempt
};

// DLL loading policy
enum class DLLLoadingPolicy {
    AllowAll,           // No restrictions (not recommended)
    SignedOnly,         // Only signed DLLs
    ValidatedOnly,      // Smart validation with user decisions
    SystemOnly          // Only Windows system DLLs
};

// Sandboxing options
struct SandboxConfig {
    bool enableJobObject = true;           // Use Windows job object for resource limits
    bool enableLowIntegrity = false;       // Run at low integrity level
    bool enableMitigations = true;         // Apply process mitigation policies
    bool restrictUIAccess = false;         // Prevent UI interactions
    
    // Job object limits
    SIZE_T maxMemoryMB = 512;              // Max memory usage
    DWORD maxCpuPercent = 80;              // Max CPU usage percentage
    DWORD maxActiveProcesses = 1;          // Prevent fork bombs
    bool killOnJobClose = true;            // Terminate process when job closed
    
    // Enhanced mitigations
    bool enableCFG = true;                 // Control Flow Guard
    bool enableDynamicCodeProhibit = false;// Block runtime code generation (may break JIT)
    bool enableWin32kLockdown = false;     // Disable Win32k for headless rendering
    bool enforceCodeSigning = false;       // Require signed code (strict mode)
    
    // Token restrictions
    bool reducePrivileges = true;          // Drop unnecessary privileges
    bool addRestrictedSIDs = false;        // Add write-restricted SIDs (breaks some file ops)
    
    // DLL security
    DLLLoadingPolicy dllPolicy = DLLLoadingPolicy::ValidatedOnly;
    std::string trustedDLLDirectory;       // Path for user's trusted DLLs
    bool allowUserSignedDLLs = true;       // Trust DLLs signed by user
    
    // Network security
    NetworkPolicy networkPolicy = NetworkPolicy::LocalhostOnly;
    std::vector<std::string> allowedHosts; // Whitelist for FullAccess mode
};

// Launch configuration
struct LaunchConfig {
    std::string executablePath;            // Path to render process executable
    std::string commandLineArgs;           // Additional command-line arguments
    std::string workingDirectory;          // Working directory (default: exe dir)
    std::string ipcPipeName;               // IPC pipe name to pass to child
    
    RestartPolicy restartPolicy = RestartPolicy::OnCrash;
    int maxRestartAttempts = 5;
    int restartDelayMs = 1000;
    int maxRestartDelayMs = 30000;
    
    SandboxConfig sandbox;
    
    bool hideConsoleWindow = true;
    bool inheritHandles = false;
};

// Process information
struct ProcessInfo {
    DWORD processId;
    HANDLE processHandle;
    HANDLE threadHandle;
    std::chrono::steady_clock::time_point startTime;
    int restartCount;
    bool crashed;
    int exitCode;
};

/**
 * @brief Render Process Launcher with Sandboxing
 * 
 * Features:
 * - CreateProcess-based process launching
 * - Lightweight sandboxing (job objects, integrity levels, mitigations)
 * - Process health monitoring and crash detection
 * - Auto-restart with configurable policies
 * - IPC integration for inter-process communication
 * - Resource limits and security restrictions
 * - Clean termination and cleanup
 */
class RenderProcessLauncher {
public:
    using CrashHandler = std::function<void(int exitCode, bool crashed)>;
    using StartHandler = std::function<void(DWORD processId)>;
    using StopHandler = std::function<void(DWORD processId, int exitCode)>;

    RenderProcessLauncher();
    ~RenderProcessLauncher();

    // Lifecycle
    bool Launch(const LaunchConfig& config);
    void Terminate();
    bool IsRunning() const;
    
    // Process control
    bool Restart();
    void SuspendProcess();
    void ResumeProcess();
    
    // Information
    DWORD GetProcessId() const;
    ProcessInfo GetProcessInfo() const;
    std::chrono::milliseconds GetUptime() const;
    
    // Event handlers
    void OnCrashed(CrashHandler handler);
    void OnStarted(StartHandler handler);
    void OnStopped(StopHandler handler);

private:
    // Configuration
    LaunchConfig config_;
    
    // Process state
    ProcessInfo processInfo_;
    HANDLE jobObject_;
    std::atomic<bool> running_;
    std::atomic<bool> stopRequested_;
    mutable std::mutex processMutex_;
    
    // Monitoring
    std::unique_ptr<std::thread> monitorThread_;
    
    // Restart tracking
    int consecutiveRestarts_;
    std::chrono::steady_clock::time_point lastRestartTime_;
    
    // Event handlers
    CrashHandler crashHandler_;
    StartHandler startHandler_;
    StopHandler stopHandler_;
    
    // Internal methods
    bool CreateRenderProcess();
    bool ApplySandbox();
    bool CreateJobObject();
    bool SetLowIntegrityLevel(HANDLE processHandle);
    bool ApplyProcessMitigations(HANDLE processHandle);
    bool ApplyEnhancedMitigations(HANDLE processHandle);
    bool ReduceTokenPrivileges(HANDLE processHandle);
    bool ApplyNetworkRestrictions(HANDLE processHandle);
    bool ConfigureDLLLoadingPolicy(HANDLE processHandle);
    void MonitorProcess();
    bool ShouldRestart(int exitCode) const;
    int CalculateRestartDelay() const;
    void CleanupProcess();
    std::string BuildCommandLine() const;
    void NotifyCrash(int exitCode, bool crashed);
    void NotifyStarted(DWORD processId);
    void NotifyStopped(DWORD processId, int exitCode);
};

} // namespace Render
} // namespace RainmeterManager
