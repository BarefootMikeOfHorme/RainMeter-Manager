// main.cpp - Enterprise WinMain Entry Point for RainmeterManager
// Copyright (c) 2025 Michael D Shortland (aka BarefootMike) with his wonderful assistant WarpTerminal. All rights reserved.

#include <windows.h>
#include <combaseapi.h>
#include <shellscalingapi.h>
#include <dbghelp.h>
#include <shellapi.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <winreg.h>
#include <cwchar>

// Core application headers
#include "core/logger.h"
#include "core/debug.h" 
#include "core/error_handling.h"
#include "core/security.h"
#include "version.h"
#include "ui/splash_screen.h"
#include "ui/options_window.h"
#include "../crash/CrashHandler.h"

// Phase 2: Application core layer
#include "rainmgrapp.h"
using RainmeterManager::App::RAINMGRApp;

// Global application constants
constexpr int EMERGENCY_EXIT_CODE = -1;
constexpr int INITIALIZATION_FAILURE_CODE = -2;
constexpr int DEPENDENCY_FAILURE_CODE = -3;

// Global variables for crash handling
static HINSTANCE g_hInstance = nullptr;
static std::wstring g_commandLine;
static bool g_emergencyShutdown = false;

//=============================================================================
// Forward Declarations
//=============================================================================

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd);

struct CLIOptions {
    std::wstring scene;
    std::wstring profile;
    std::wstring logLevel;
    bool headless = false;
    bool webEnabled = true;
    std::vector<std::wstring> allowedOrigins;
    bool dashboardOnly = false;
    bool showHelp = false;
};

static LogLevel ParseLogLevel(const std::wstring& level) {
    std::wstring l = level; 
    for (auto& ch : const_cast<std::wstring&>(l)) ch = towlower(ch);
    if (l == L"trace") return LogLevel::TRACE;
    if (l == L"debug") return LogLevel::DEBUG;
    if (l == L"info")  return LogLevel::INFO;
    if (l == L"warn" || l == L"warning") return LogLevel::WARNING;
    if (l == L"error") return LogLevel::ERROR;
    if (l == L"critical") return LogLevel::CRITICAL;
    if (l == L"fatal") return LogLevel::FATAL;
    return LogLevel::INFO;
}

static CLIOptions ParseCLI() {
    CLIOptions opts;
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return opts;
    for (int i = 1; i < argc; ++i) {
        std::wstring arg = argv[i];
        auto eq = arg.find(L'=');
        auto key = arg;
        std::wstring val;
        if (eq != std::wstring::npos) { key = arg.substr(0, eq); val = arg.substr(eq + 1); }
        if (key == L"--scene" && !val.empty()) opts.scene = val;
        else if (key == L"--profile" && !val.empty()) opts.profile = val;
        else if (key == L"--log-level" && !val.empty()) opts.logLevel = val;
        else if (key == L"--headless") opts.headless = true;
        else if (key == L"--no-web") opts.webEnabled = false;
        else if (key == L"--allow-origin" && !val.empty()) {
            size_t start = 0, pos = 0; 
            while ((pos = val.find(L',', start)) != std::wstring::npos) {
                opts.allowedOrigins.push_back(val.substr(start, pos - start));
                start = pos + 1;
            }
            if (start < val.size()) opts.allowedOrigins.push_back(val.substr(start));
        }
        else if (key == L"--dashboard-only") opts.dashboardOnly = true;
        else if (key == L"--help" || key == L"-h" || key == L"/h" || key == L"/?") opts.showHelp = true;
    }
    LocalFree(argv);
    return opts;
}

static void ApplyCLIOptions(const CLIOptions& opts) {
    if (!opts.logLevel.empty()) {
        Logger::setLogLevel(ParseLogLevel(opts.logLevel));
    }
    // Log CLI settings; ConfigManager plumbing can be added non-destructively later
    LOG_INFO("CLI: headless=" + std::string(opts.headless ? "true" : "false") +
             ", webEnabled=" + std::string(opts.webEnabled ? "true" : "false"));
}
LONG WINAPI AppUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo);
bool InitializeCOMSubsystem();
bool SetupDPIAwareness();
bool SetupStructuredExceptionHandling();
void GenerateMiniDump(EXCEPTION_POINTERS* exceptionInfo);
void ShowCriticalErrorDialog(const std::wstring& title, const std::wstring& message);
bool ValidateSystemRequirements();
void CleanupAndExit(int exitCode);

// CLI helpers
static void ShowCLIHelp();
static bool ValidateCLIOptions(const CLIOptions& opts, std::wstring& error);

//=============================================================================
// Main Application Entry Point
//=============================================================================

/**
 * @brief Windows Unicode Application Entry Point
 * 
 * Implements enterprise-grade initialization sequence with comprehensive
 * error handling, crash resilience, and performance monitoring.
 * 
 * @param hInstance     Current instance handle
 * @param hPrevInstance Previous instance handle (unused)
 * @param lpCmdLine     Command line arguments
 * @param nShowCmd      Initial window display state
 * @return Application exit code
 */
int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine, 
    _In_ int nShowCmd
) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    
    // Store global references for crash handling
    g_hInstance = hInstance;
    g_commandLine = lpCmdLine ? lpCmdLine : L"";
    
    // Start application bootstrap timer
    auto startupTimer = std::chrono::high_resolution_clock::now();
    
    try {
        //=====================================================================
        // Phase 1: Critical System Initialization
        //=====================================================================
        
        // Install crash handler with vectored exceptions (Phase 1)
        RainmeterManager::Crash::CrashHandler::Install();
        
        // Setup structured exception handling first
        if (!SetupStructuredExceptionHandling()) {
            ShowCriticalErrorDialog(
                L"System Error", 
                L"Failed to initialize exception handling. Application cannot continue."
            );
            return INITIALIZATION_FAILURE_CODE;
        }
        
        // Validate system requirements
        if (!ValidateSystemRequirements()) {
            ShowCriticalErrorDialog(
                L"System Requirements", 
                L"System does not meet minimum requirements for RainmeterManager."
            );
            return INITIALIZATION_FAILURE_CODE;
        }
        
        // Initialize DPI awareness for high-resolution displays
        if (!SetupDPIAwareness()) {
            // Log warning but continue - not critical
            OutputDebugStringW(L"Warning: Failed to set DPI awareness\n");
        }
        
        // Initialize COM subsystem
        if (!InitializeCOMSubsystem()) {
            ShowCriticalErrorDialog(
                L"COM Initialization Failed", 
                L"Failed to initialize Windows COM subsystem."
            );
            return INITIALIZATION_FAILURE_CODE;
        }
        
        //=====================================================================
        // Phase 2: Logging Infrastructure 
        //=====================================================================
        
        // Initialize enterprise logging system first
        LogRotationConfig logConfig = {
            .maxFileSize = 10 * 1024 * 1024,  // 10MB
            .maxFiles = 5,
            .enableRotation = true
        };
        
        std::string logPath = "logs/RainmeterManager.log";
        if (!Logger::initialize(logPath, logConfig)) {
            ShowCriticalErrorDialog(
                L"Logging Initialization Failed",
                L"Failed to initialize logging system. Check disk permissions."
            );
            return INITIALIZATION_FAILURE_CODE;
        }
        
// Apply CLI
        CLIOptions cli = ParseCLI();
        
        // Log successful bootstrap start
        LOG_INFO("=== RainmeterManager Bootstrap Started ===");
        
        // Set log level from CLI if provided
        ApplyCLIOptions(cli);
        if (cli.showHelp) {
            ShowCLIHelp();
            CleanupAndExit(0);
            return 0;
        }
        {
            std::wstring cliError;
            if (!ValidateCLIOptions(cli, cliError)) {
                ShowCriticalErrorDialog(L"Invalid CLI Options", cliError + L"\nTry --help for usage.");
                return INITIALIZATION_FAILURE_CODE;
            }
        }
        LOG_INFO("Version: " + std::string(VERSION_STRING));
        LOG_INFO("Build: " + std::to_string(VERSION_MAJOR) + "." + 
                 std::to_string(VERSION_MINOR) + "." + std::to_string(VERSION_PATCH));
        LOG_INFO("Command Line: " + std::string(lpCmdLine ? 
                 std::string(lpCmdLine, lpCmdLine + wcslen(lpCmdLine)) : ""));
        
        //=====================================================================
        // Phase 3: Security Framework Initialization
        //=====================================================================
        
        LOG_INFO("Initializing security framework...");
        
// Initialize crypto providers first
        if (!Security::Initialize()) {
            LOG_ERROR("Failed to initialize cryptographic providers");
            ShowCriticalErrorDialog(
                L"Security Initialization Failed",
                L"Failed to initialize security subsystem. Application cannot continue securely."
            );
            return INITIALIZATION_FAILURE_CODE;
        }
        
        // Get current executable path for signature validation
        wchar_t exePathW[MAX_PATH];
        if (GetModuleFileNameW(hInstance, exePathW, MAX_PATH) > 0) {
            std::wstring wExePath(exePathW);
            std::string exePath(wExePath.begin(), wExePath.end());
            
            // Validate application code signature
            if (Security::checkFileSignature(exePath)) {
                LOG_INFO("Application code signature validation: PASSED");
            } else {
                LOG_WARNING("Application code signature validation: FAILED or UNSIGNED");
                // Continue execution but log security event
                Logger::logSecurityEvent("Code Signature Warning", 
                    "Application executable is not properly signed: " + exePath);
            }
        } else {
            LOG_WARNING("Could not determine executable path for signature validation");
        }
        
        LOG_INFO("Security framework initialization completed");
        
        //=====================================================================
        // Phase 4: Application Instance Creation (Phase 2 Implementation)
        //=====================================================================
        
        LOG_INFO("Creating RAINMGRApp singleton instance...");
        
        // Phase 2: Get the application singleton instance
        auto& app = RAINMGRApp::GetInstance(hInstance);
        
        // Initialize the application core layer
        if (!app.Initialize()) {
            LOG_ERROR("Failed to initialize RAINMGRApp singleton");
            ShowCriticalErrorDialog(
                L"Application Initialization Failed",
                L"Failed to initialize application core layer."
            );
            return INITIALIZATION_FAILURE_CODE;
        }
        
        LOG_INFO("RAINMGRApp singleton created and initialized successfully");
        
        //=====================================================================
        // Phase 5: Service Registration (Future Implementation)
        //=====================================================================
        
        LOG_INFO("Registering core services...");
        
        // TODO: Implement service locator pattern
        // - SecurityManager
        // - TelemetryService  
        // - ConfigurationManager
        // - UIFramework
        // - WidgetManager
        
        LOG_INFO("Service registration: PLACEHOLDER");
        
        //=====================================================================
        // Phase 6: Dependency Validation
        //=====================================================================
        
        LOG_INFO("Validating service dependencies...");
        
        // TODO: Implement dependency validation
        // if (!app.ValidateDependencies()) {
        //     LOG_ERROR("Service dependency validation failed");
        //     return DEPENDENCY_FAILURE_CODE;
        // }
        
        LOG_INFO("Dependency validation: PLACEHOLDER");
        
        //=====================================================================
        // Phase 7: UI Initialization & Splash Screen
        //=====================================================================
        
        LOG_INFO("Initializing UI framework...");
        
        // Show cinematic splash (disabled for now)
        if (false) {
            using RainmeterManager::UI::SplashManager;
            auto& splash = SplashManager::GetInstance();
            splash.ShowSplash(hInstance);
            splash.UpdateProgress(10, L"Bootstrapping");
        }

        // TODO: Implement UI framework initialization
        // - Register window classes
        // - Initialize SkiaSharp surface (if available)
        // - Create splash screen
        
        LOG_INFO("UI framework initialization: PLACEHOLDER");
        if (false) {
            // Simulate staged progress updates for user feedback
            using RainmeterManager::UI::SplashManager;
            auto& splash = SplashManager::GetInstance();
            splash.UpdateProgress(40, L"Security\u2003✓");
            splash.UpdateProgress(60, L"Core services");
            splash.UpdateProgress(80, L"UI ready");
        }

        // Create and show Options window with Dashboard and Task Manager tabs
        {
            RainmeterManager::UI::OptionsWindow opts(hInstance);
            if (opts.Create()) {
                opts.Show(SW_SHOWNORMAL);
            }
        }
        
        //=====================================================================
        // Phase 8: Widget System Initialization  
        //=====================================================================
        
        LOG_INFO("Initializing widget system...");
        
        // TODO: Implement widget manager
        // - Load widget plugins
        // - Initialize security sandbox
        // - Register widget templates
        
        LOG_INFO("Widget system initialization: PLACEHOLDER");
        
        //=====================================================================
        // Phase 9: Application Ready - Start Message Loop
        //=====================================================================
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startupTimer);
        
        LOG_INFO("=== Bootstrap Complete in " + std::to_string(duration.count()) + "ms ===");
        LOG_INFO("Phase 2: Starting RAINMGRApp main loop...");
        
        // Phase 2: Run the application using RAINMGRApp singleton
        if (false) {
            using RainmeterManager::UI::SplashManager;
            auto& splash = SplashManager::GetInstance();
            splash.UpdateProgress(100, L"Starting");
        }
        int appExitCode = app.Run();
        
        LOG_INFO("Application exited with code: " + std::to_string(appExitCode));
        
        LOG_INFO("Application message loop ended");
        
        //=====================================================================
        // Phase 10: Graceful Shutdown
        //=====================================================================
        
        LOG_INFO("Beginning graceful shutdown...");
        
// TODO: Implement graceful shutdown sequence
        // - Save configuration
        // - Shutdown services in reverse order
        // - Release resources
        
        // Cleanup security providers
        Security::Cleanup();

        LOG_INFO("Graceful shutdown complete");
        CleanupAndExit(0);
        return 0;
        
    } catch (const std::exception& e) {
        // Handle C++ exceptions
        if (!g_emergencyShutdown) {
            LOG_CRITICAL("Unhandled C++ exception in main: " + std::string(e.what()));
            ShowCriticalErrorDialog(
                L"Critical Application Error",
                L"An unhandled exception occurred. The application will terminate."
            );
        }
        CleanupAndExit(EMERGENCY_EXIT_CODE);
        return EMERGENCY_EXIT_CODE;
        
    } catch (...) {
        // Handle any other exceptions
        if (!g_emergencyShutdown) {
            LOG_CRITICAL("Unknown exception in main");
            ShowCriticalErrorDialog(
                L"Critical Application Error", 
                L"An unknown error occurred. The application will terminate."
            );
        }
        CleanupAndExit(EMERGENCY_EXIT_CODE);
        return EMERGENCY_EXIT_CODE;
    }
}

//=============================================================================
// CLI Helpers
//=============================================================================

static void ShowCLIHelp() {
    std::wstring help =
        L"RainmeterManager CLI\n\n"
        L"Options:\n"
        L"  --scene=<name>            Select startup scene\n"
        L"  --profile=<name>          Use configuration profile\n"
        L"  --log-level=<level>       trace|debug|info|warn|error|critical|fatal\n"
        L"  --headless                Start without UI\n"
        L"  --dashboard-only          Show dashboard only (no widget suite)\n"
        L"  --no-web                  Disable web content\n"
        L"  --allow-origin=<host>     Allow web origin (repeatable)\n"
        L"  --help, -h                Show this help\n\n"
        L"Examples:\n"
        L"  RainmeterManager.exe --log-level=info --dashboard-only\n"
        L"  RainmeterManager.exe --scene=work --profile=quiet --no-web\n"
        L"  RainmeterManager.exe --allow-origin=api.nasa.gov --allow-origin=tile.openstreetmap.org\n";
    MessageBoxW(nullptr, help.c_str(), L"RainmeterManager Help", MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
}

static bool ValidateCLIOptions(const CLIOptions& opts, std::wstring& error) {
    if (opts.headless && opts.dashboardOnly) {
        error = L"--dashboard-only cannot be combined with --headless.";
        return false;
    }
    if (!opts.webEnabled && !opts.allowedOrigins.empty()) {
        error = L"--allow-origin can only be used when web is enabled (omit --no-web).";
        return false;
    }
    return true;
}

//=============================================================================
// Structured Exception Handling
//=============================================================================

/**
 * @brief Unhandled Exception Filter for crash resilience
 * 
 * Captures crash context, generates minidumps, logs critical errors,
 * and attempts graceful recovery or emergency shutdown.
 */
// Forward declaration
static void LogSymbolizedStackTrace(EXCEPTION_POINTERS* exceptionInfo);

LONG WINAPI AppUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo) {
    // Prevent recursive exception handling
    static bool handlingException = false;
    if (handlingException) {
        return EXCEPTION_EXECUTE_HANDLER;
    }
    handlingException = true;
    
    try {
        // Capture crash context
        DWORD exceptionCode = exceptionInfo->ExceptionRecord->ExceptionCode;
        PVOID exceptionAddress = exceptionInfo->ExceptionRecord->ExceptionAddress;
        DWORD threadId = GetCurrentThreadId();
        DWORD processId = GetCurrentProcessId();
        
        // Generate crash dump
        GenerateMiniDump(exceptionInfo);
        
        // Log critical error with context (hex formatting + basic symbolization)
        std::stringstream ss;
        ss << "Unhandled Exception - Code: 0x" << std::hex << exceptionCode
           << ", Address: 0x" << (uintptr_t)exceptionAddress
           << std::dec << ", Thread: " << threadId
           << ", Process: " << processId;

        // Attempt to resolve symbol for exception address
        HANDLE hProcess = GetCurrentProcess();
        SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);
        static bool g_symInitialized = false;
        if (!g_symInitialized) {
            if (SymInitialize(hProcess, nullptr, TRUE)) {
                g_symInitialized = true;
            }
        }
        if (g_symInitialized) {
            DWORD64 addr = reinterpret_cast<DWORD64>(exceptionAddress);
            BYTE symBuffer[sizeof(SYMBOL_INFO) + 256] = {0};
            PSYMBOL_INFO pSym = reinterpret_cast<PSYMBOL_INFO>(symBuffer);
            pSym->SizeOfStruct = sizeof(SYMBOL_INFO);
            pSym->MaxNameLen = 255;
            DWORD64 displacement = 0;
            if (SymFromAddr(hProcess, addr, &displacement, pSym)) {
                ss << " | Symbol: " << pSym->Name << "+0x" << std::hex << displacement;
            }
        }
        std::string errorMsg = ss.str();
        
        LOG_CRITICAL(errorMsg);
        LogSymbolizedStackTrace(exceptionInfo);
        
        // Show crash dialog to user
        std::wstring message = L"RainmeterManager has encountered a critical error and needs to close.\n\n" \
                              L"Exception Code: 0x" + std::to_wstring(exceptionCode) + L"\n" \
                              L"A crash dump has been generated for analysis.\n\n" \
                              L"Please restart the application.";
        
        ShowCriticalErrorDialog(L"Application Crash", message);
        
        // Attempt emergency shutdown
        g_emergencyShutdown = true;
        CleanupAndExit(EMERGENCY_EXIT_CODE);
        
    } catch (...) {
        // If exception handling itself fails, force terminate
        TerminateProcess(GetCurrentProcess(), EMERGENCY_EXIT_CODE);
    }
    
    return EXCEPTION_EXECUTE_HANDLER;
}

//=============================================================================
// System Initialization Functions
//=============================================================================

/**
 * @brief Initialize Windows COM subsystem with apartment threading
 */
bool InitializeCOMSubsystem() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    
    if (FAILED(hr)) {
        OutputDebugStringW(L"COM initialization failed\n");
        return false;
    }
    
    return true;
}

/**
 * @brief Setup DPI awareness for high-resolution displays
 */
bool SetupDPIAwareness() {
    // Try Windows 10 version 1903+ first
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        auto SetProcessDpiAwarenessContext = 
            reinterpret_cast<decltype(::SetProcessDpiAwarenessContext)*>(
                GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        
        if (SetProcessDpiAwarenessContext) {
            return SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) != FALSE;
        }
    }
    
    // Fallback to Windows 8.1+ version
    HMODULE shcore = LoadLibraryW(L"shcore.dll");
    if (shcore) {
        auto SetProcessDpiAwareness = 
            reinterpret_cast<HRESULT(WINAPI*)(PROCESS_DPI_AWARENESS)>(
                GetProcAddress(shcore, "SetProcessDpiAwareness"));
        
        if (SetProcessDpiAwareness) {
            HRESULT hr = SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
            FreeLibrary(shcore);
            return SUCCEEDED(hr);
        }
        FreeLibrary(shcore);
    }
    
    // Final fallback to Windows Vista+
    return SetProcessDPIAware() != FALSE;
}

/**
 * @brief Setup structured exception handling
 */
bool SetupStructuredExceptionHandling() {
    LPTOP_LEVEL_EXCEPTION_FILTER previous = SetUnhandledExceptionFilter(AppUnhandledExceptionFilter);
    
    // Enable crash dump generation
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    
    return true;
}

/**
 * @brief Generate minidump file for crash analysis
 */
void LogSymbolizedStackTrace(EXCEPTION_POINTERS* exceptionInfo) {
    HANDLE hProcess = GetCurrentProcess();
    HANDLE hThread = GetCurrentThread();

    // Initialize symbols if needed
    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);
    static bool g_symInitialized = false;
    if (!g_symInitialized) {
        if (SymInitialize(hProcess, nullptr, TRUE)) {
            g_symInitialized = true;
        }
    }

    CONTEXT ctx = *exceptionInfo->ContextRecord; // make a copy we can modify

#if defined(_M_X64) || defined(__x86_64__)
    DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
    STACKFRAME64 frame = {};
    frame.AddrPC.Offset    = ctx.Rip;
    frame.AddrPC.Mode      = AddrModeFlat;
    frame.AddrFrame.Offset = ctx.Rbp;
    frame.AddrFrame.Mode   = AddrModeFlat;
    frame.AddrStack.Offset = ctx.Rsp;
    frame.AddrStack.Mode   = AddrModeFlat;
#else
    DWORD machineType = IMAGE_FILE_MACHINE_I386;
    STACKFRAME64 frame = {};
    frame.AddrPC.Offset    = ctx.Eip;
    frame.AddrPC.Mode      = AddrModeFlat;
    frame.AddrFrame.Offset = ctx.Ebp;
    frame.AddrFrame.Mode   = AddrModeFlat;
    frame.AddrStack.Offset = ctx.Esp;
    frame.AddrStack.Mode   = AddrModeFlat;
#endif

    std::stringstream out;
    out << "Stack trace:" << '\n';

    for (int i = 0; i < 32; ++i) {
        BOOL ok = StackWalk64(
            machineType,
            hProcess,
            hThread,
            &frame,
            &ctx,
            nullptr,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            nullptr
        );
        if (!ok || frame.AddrPC.Offset == 0) break;

        DWORD64 addr = frame.AddrPC.Offset;
        BYTE symBuffer[sizeof(SYMBOL_INFO) + 512] = {0};
        PSYMBOL_INFO pSym = reinterpret_cast<PSYMBOL_INFO>(symBuffer);
        pSym->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSym->MaxNameLen = 511;
        DWORD64 disp = 0;
        IMAGEHLP_MODULE64 modInfo = {};
        modInfo.SizeOfStruct = sizeof(modInfo);

        const char* moduleName = "?";
        if (SymGetModuleInfo64(hProcess, addr, &modInfo)) {
            moduleName = modInfo.ModuleName;
        }

        if (SymFromAddr(hProcess, addr, &disp, pSym)) {
            DWORD dwDisp = 0;
            IMAGEHLP_LINE64 line = {};
            line.SizeOfStruct = sizeof(line);
            if (SymGetLineFromAddr64(hProcess, addr, &dwDisp, &line)) {
                out << "  [" << i << "] " << moduleName << "!" << pSym->Name << "+0x" << std::hex << disp
                    << " (" << line.FileName << ":" << std::dec << line.LineNumber << ")" << '\n';
            } else {
                out << "  [" << i << "] " << moduleName << "!" << pSym->Name << "+0x" << std::hex << disp
                    << " [0x" << addr << "]" << '\n';
            }
        } else {
            out << "  [" << i << "] 0x" << std::hex << addr << '\n';
        }
    }

    LOG_CRITICAL(out.str());
}

void GenerateMiniDump(EXCEPTION_POINTERS* exceptionInfo) {
    try {
        // Create dumps directory
        CreateDirectoryW(L"dumps", nullptr);
        
        // Generate timestamp-based filename  
        SYSTEMTIME st;
        GetSystemTime(&st);
        
        wchar_t filename[MAX_PATH];
        swprintf_s(filename, L"dumps\\RainmeterManager_%04d%02d%02d_%02d%02d%02d.dmp",
                  st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        
        // Create dump file
        HANDLE hDumpFile = CreateFileW(filename, GENERIC_WRITE, 0, nullptr,
                                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        
        if (hDumpFile != INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION dumpInfo = {0};
            dumpInfo.ThreadId = GetCurrentThreadId();
            dumpInfo.ExceptionPointers = exceptionInfo;
            dumpInfo.ClientPointers = FALSE;
            
            MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile,
                             MiniDumpWithFullMemory, &dumpInfo, nullptr, nullptr);
            
            CloseHandle(hDumpFile);
        }
        
    } catch (...) {
        // Ignore dump generation failures
    }
}

/**
 * @brief Validate minimum system requirements
 */
bool ValidateSystemRequirements() {
    // Check Windows version (Windows 10+) using registry keys
    DWORD major = 0;
    HKEY hKey = nullptr;
    LONG rc = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        0,
        KEY_READ | KEY_WOW64_64KEY,
        &hKey);
    if (rc == ERROR_SUCCESS) {
        DWORD type = 0;
        DWORD size = sizeof(DWORD);
        // Preferred on Windows 10+
        if (RegQueryValueExW(hKey, L"CurrentMajorVersionNumber", nullptr, &type,
                              reinterpret_cast<LPBYTE>(&major), &size) != ERROR_SUCCESS || type != REG_DWORD) {
            // Fallback: parse "CurrentVersion" (e.g., "10.0")
            wchar_t ver[64] = {0};
            size = sizeof(ver);
            if (RegQueryValueExW(hKey, L"CurrentVersion", nullptr, &type,
                                  reinterpret_cast<LPBYTE>(ver), &size) == ERROR_SUCCESS &&
                (type == REG_SZ || type == REG_EXPAND_SZ)) {
                wchar_t* end = nullptr;
                long parsed = std::wcstol(ver, &end, 10);
                if (parsed > 0) major = static_cast<DWORD>(parsed);
            }
        }
        RegCloseKey(hKey);
    }
    if (major < 10) {
        return false; // Require Windows 10+
    }

    // Check available memory (minimum 1GB)
    MEMORYSTATUSEX memStatus = {0};
    memStatus.dwLength = sizeof(memStatus);
    GlobalMemoryStatusEx(&memStatus);

    if (memStatus.ullTotalPhys < (1ULL * 1024 * 1024 * 1024)) {
        return false; // Insufficient memory
    }

    return true;
}

//=============================================================================
// Utility Functions
//=============================================================================

/**
 * @brief Show critical error dialog to user
 */
void ShowCriticalErrorDialog(const std::wstring& title, const std::wstring& message) {
    MessageBoxW(nullptr, message.c_str(), title.c_str(), 
                MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
}

/**
 * @brief Perform cleanup and exit application
 */
void CleanupAndExit(int exitCode) {
    try {
        // Flush logs
        Logger::flushLogs();
        
        // Cleanup security framework
        Security::Cleanup();
        
        // Shutdown logging system
        Logger::shutdown();
        
        // Uninitialize COM
        CoUninitialize();
        
    } catch (...) {
        // Ignore cleanup exceptions
    }
    
    ExitProcess(exitCode);
}
