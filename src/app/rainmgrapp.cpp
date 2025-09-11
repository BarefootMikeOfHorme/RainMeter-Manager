#include "rainmgrapp.h"
#include "../core/error_handling.h"
#include "../core/security_adapter.h"
#include "../core/logger_adapter.h"
#include "../ui/splash_screen.h"
#include <filesystem>
#include <shlwapi.h>
#include <atomic>
#include <thread>
#include <cstdio>

#pragma comment(lib, "shlwapi.lib")

// Minimal raw trace to bypass Logger for crash forensics
static void RawTrace(const char* msg) {
    HANDLE h = CreateFileA("raw_trace.txt", FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
        DWORD bw = 0;
        if (msg) {
            DWORD len = static_cast<DWORD>(strlen(msg));
            if (len > 0) WriteFile(h, msg, len, &bw, nullptr);
        }
        const char* crlf = "\r\n";
        WriteFile(h, crlf, 2, &bw, nullptr);
        CloseHandle(h);
    }
}

namespace RainmeterManager {
namespace App {

// Static member definitions
std::unique_ptr<RAINMGRApp> RAINMGRApp::instance_ = nullptr;
std::mutex RAINMGRApp::instance_mutex_;

// Window class name for main window
constexpr wchar_t MAIN_WINDOW_CLASS[] = L"RainmeterManagerMainWindow";

RAINMGRApp::RAINMGRApp(HINSTANCE hInstance)
    : hInstance_(hInstance)
    , mainWindow_(nullptr)
    , initialized_(false)
    , shutdownRequested_(false)
    , messageLoopRunning_(false)
    , mainThreadId_(GetCurrentThreadId())
{
    // Constructor: keep minimal to avoid early crashes; logging deferred to Initialize
}

RAINMGRApp::~RAINMGRApp() {
    if (initialized_) {
        Shutdown();
    }
    // Destructor: avoid logging during teardown
}

RAINMGRApp& RAINMGRApp::GetInstance(HINSTANCE hInstance) {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (!instance_) {
        if (hInstance == nullptr) {
            throw std::runtime_error("First call to GetInstance requires valid HINSTANCE");
        }
        LOG_INFO("RAINMGRApp::GetInstance - constructing instance");
        instance_ = std::unique_ptr<RAINMGRApp>(new RAINMGRApp(hInstance));
        LOG_INFO("RAINMGRApp::GetInstance - instance constructed");
    }
    LOG_INFO("RAINMGRApp::GetInstance - returning instance");
    return *instance_;
}

void RAINMGRApp::DestroyInstance() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    instance_.reset();
}

bool RAINMGRApp::Initialize() {
    try {
        LOG_INFO("RAINMGRApp::Initialize - entry");
        LOG_INFO("RAINMGRApp::Initialize - starting...");
        
        // Initialize paths first
        LOG_INFO("RAINMGRApp::Initialize - before InitializePaths");
        if (!InitializePaths()) {
            HandleFatalError(L"Failed to initialize application paths");
            return false;
        }
        LOG_INFO("RAINMGRApp::Initialize - after InitializePaths");
        
        // Initialize logging system
        LOG_INFO("RAINMGRApp::Initialize - before InitializeLogging");
        if (!InitializeLogging()) {
            HandleFatalError(L"Failed to initialize logging system");
            return false;
        }
        LOG_INFO("RAINMGRApp::Initialize - after InitializeLogging");
        
        // Initialize security framework
        RawTrace("Init: before InitializeSecurity");
        LOG_INFO("RAINMGRApp::Initialize - before InitializeSecurity");
        if (!InitializeSecurity()) {
            HandleFatalError(L"Failed to initialize security framework");
            return false;
        }
        LOG_INFO("RAINMGRApp::Initialize - after InitializeSecurity");
        RawTrace("Init: after InitializeSecurity");
        
        // Create service locator
        LOG_INFO("RAINMGRApp::Initialize - before ServiceLocator creation");
        serviceLocator_ = std::make_unique<Core::ServiceLocator>();
        LOG_INFO("RAINMGRApp::Initialize - after ServiceLocator creation");
        
        // Initialize core services
        LOG_INFO("RAINMGRApp::Initialize - before InitializeServices");
        if (!InitializeServices()) {
            HandleFatalError(L"Failed to initialize core services");
            return false;
        }
        LOG_INFO("RAINMGRApp::Initialize - after InitializeServices");
        
        // Create main window (hidden); we'll show it after splash completes
        LOG_INFO("RAINMGRApp::Initialize - before CreateMainWindow");
        if (!CreateMainWindow()) {
            HandleFatalError(L"Failed to create main application window");
            return false;
        }
        LOG_INFO("RAINMGRApp::Initialize - after CreateMainWindow");

        // Display cinematic splash while completing background initialization (temporarily disabled for stability)
        bool useCinematicSplash = false;
        if (useCinematicSplash) {
            UI::CinematicSplashScreen::Config splashCfg;
            splashCfg.enableSound = false;
            bool splashShown = false;
            std::unique_ptr<UI::CinematicSplashScreen> splash;
            try {
                splash = std::make_unique<UI::CinematicSplashScreen>(hInstance_, splashCfg);
                splashShown = splash->Show();
            } catch (...) {
                splashShown = false;
                splash.reset();
            }

            auto splashStart = std::chrono::steady_clock::now();
            auto stage = [&](int pct, const wchar_t* msg){ if (splashShown && splash) splash->UpdateProgress(pct, msg); };
            stage(10, L"Initializing services");
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            stage(40, L"Preparing dashboard");
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            stage(70, L"Loading widgets");
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            stage(100, L"Starting");

            constexpr long kMinMs = 6000;
            constexpr long kPrefMaxMs = 8000;
            constexpr long kHardMaxMs = 12000;
            long targetMs = splashCfg.displayTimeMs;
            if (targetMs < kMinMs) targetMs = kMinMs; else if (targetMs > kPrefMaxMs) targetMs = kPrefMaxMs;

            bool criticalInitDone = true;
            while (true) {
                auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                      std::chrono::steady_clock::now() - splashStart)
                                      .count();
                if (elapsedMs >= targetMs && (criticalInitDone || elapsedMs >= kHardMaxMs)) break;
                MSG m{};
                while (PeekMessage(&m, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage(&m); DispatchMessage(&m); }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            if (splashShown && splash) splash->Hide();
        } else {
            // Minimal staged initialization without splash
            LogApplicationEvent(L"Initializing services (no splash)");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            LogApplicationEvent(L"Preparing dashboard (no splash)");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            LogApplicationEvent(L"Loading widgets (no splash)");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // Show main window now that splash has finished
        ShowMainWindow();

        // Mark initialized
        initialized_ = true;
        LogApplicationEvent(L"Application initialization completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        std::string errorMsg = "Exception during initialization: ";
        errorMsg += e.what();
        std::wstring wideError(errorMsg.begin(), errorMsg.end());
        HandleFatalError(wideError);
        return false;
    } catch (...) {
        HandleFatalError(L"Unknown exception during initialization");
        return false;
    }
}

int RAINMGRApp::Run() {
    if (!initialized_) {
        HandleFatalError(L"Cannot run application - not initialized");
        return -1;
    }
    
    LogApplicationEvent(L"Starting message loop...");
    messageLoopRunning_ = true;
    
    MSG msg = {};
    int exitCode = 0;
    
    // Enhanced message loop with idle processing
    while (!shutdownRequested_) {
        BOOL result = PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
        
        if (result > 0) {
            // Got a message
            if (msg.message == WM_QUIT) {
                exitCode = static_cast<int>(msg.wParam);
                LogApplicationEvent(L"WM_QUIT received, exit code: " + std::to_wstring(exitCode));
                break;
            }
            
            // Translate and dispatch message
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            // No messages pending - do idle processing
            if (!ProcessMessages()) {
                break;  // Application requested shutdown
            }
            
            // Prevent 100% CPU usage during idle
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
    messageLoopRunning_ = false;
    LogApplicationEvent(L"Message loop ended, exit code: " + std::to_wstring(exitCode));
    
    return exitCode;
}

bool RAINMGRApp::ProcessMessages() {
    // Check for shutdown request
    if (shutdownRequested_) {
        return false;
    }
    
    // Perform idle processing here
    // - Update UI elements that need periodic refresh
    // - Process background tasks
    // - Handle service lifecycle events
    
    // Idle processing placeholder
    // - Update UI elements that need periodic refresh
    // - Process background tasks
    // - Handle service lifecycle events
    
    return true;
}

void RAINMGRApp::PostQuitMessage(int exitCode) {
    LogApplicationEvent(L"PostQuitMessage called with exit code: " + std::to_wstring(exitCode));
    ::PostQuitMessage(exitCode);
}

void RAINMGRApp::RequestShutdown() {
    LogApplicationEvent(L"Shutdown requested");
    shutdownRequested_ = true;
    
    // If we're in the message loop, post a quit message
    if (messageLoopRunning_) {
        PostQuitMessage(0);
    }
}

void RAINMGRApp::Shutdown() {
    if (!initialized_) {
        return;  // Already shut down
    }
    
    LogApplicationEvent(L"Starting application shutdown...");
    shutdownRequested_ = true;
    
    try {
        // Execute registered shutdown handlers
        ExecuteShutdownHandlers();
        
        // Cleanup resources
        CleanupResources();
        
        // Cleanup services
        CleanupServices();
        
        initialized_ = false;
        LogApplicationEvent(L"Application shutdown completed");
        
    } catch (const std::exception& e) {
        std::string errorMsg = "Exception during shutdown: ";
        errorMsg += e.what();
        std::wstring wideError(errorMsg.begin(), errorMsg.end());
        LogApplicationEvent(wideError, Core::LogLevel::Error);
    } catch (...) {
        LogApplicationEvent(L"Unknown exception during shutdown", Core::LogLevel::Error);
    }
}

void RAINMGRApp::RegisterShutdownHandler(std::function<void()> handler) {
    std::lock_guard<std::mutex> lock(shutdownMutex_);
    shutdownHandlers_.push_back(std::move(handler));
    LogApplicationEvent(L"Shutdown handler registered (total: " + std::to_wstring(shutdownHandlers_.size()) + L")");
}

void RAINMGRApp::UnregisterAllShutdownHandlers() {
    std::lock_guard<std::mutex> lock(shutdownMutex_);
    size_t count = shutdownHandlers_.size();
    shutdownHandlers_.clear();
    LogApplicationEvent(L"All shutdown handlers unregistered (removed: " + std::to_wstring(count) + L")");
}

bool RAINMGRApp::InitializePaths() {
    try {
        LOG_INFO("InitializePaths: start");
        // Get application executable path
        wchar_t exePath[MAX_PATH] = {0};
        if (GetModuleFileName(hInstance_, exePath, MAX_PATH) == 0) {
            LogApplicationEvent(L"Failed to get module file name", Core::LogLevel::Error);
            return false;
        }
        LOG_INFO("InitializePaths: got module file name");
        
        // Extract directory path
        wchar_t* lastSlash = wcsrchr(exePath, L'\\');
        if (lastSlash) {
            *lastSlash = L'\0';
        }
        LOG_INFO("InitializePaths: extracted directory");
        
        applicationPath_ = exePath;
        
        // Set configuration path (same as application path for now)
        configPath_ = applicationPath_ + L"\\config";
        
        // Ensure config directory exists
        std::error_code ec;
        std::filesystem::create_directories(configPath_, ec);
        if (ec) {
            LogApplicationEvent(L"Failed to create config directory: " + configPath_, Core::LogLevel::Warning);
        }
        LOG_INFO("InitializePaths: directories ensured");
        
        LOG_INFO("InitializePaths: path variables set");
        
        return true;
    } catch (const std::exception& e) {
        std::string errorMsg = "Exception in InitializePaths: ";
        errorMsg += e.what();
        std::wstring wideError(errorMsg.begin(), errorMsg.end());
        LogApplicationEvent(wideError, Core::LogLevel::Error);
        return false;
    }
}

bool RAINMGRApp::InitializeServices() {
    try {
        RawTrace("InitServices: entered");
        
        // Register core services with the service locator
        // Note: In a real implementation, you would register concrete service implementations here
        
        RawTrace("InitServices: returning true");
        return true;
    } catch (const std::exception& e) {
        RawTrace("InitServices: caught std::exception");
        return false;
    } catch (...) {
        RawTrace("InitServices: caught unknown exception");
        return false;
    }
}

bool RAINMGRApp::InitializeLogging() {
    // Logger already initialized in main; skip verification to avoid re-entrancy during bootstrap
    return true;
}

bool RAINMGRApp::InitializeSecurity() {
    try {
        RawTrace("InitSecurity: entered");
        // Avoid duplicate initialization: main.cpp performs initial Security::Initialize()
        // Skipping re-initialization here to isolate startup access violation
        // Note: intentionally bypassing Logger here to test for logging-related AV
        RawTrace("InitSecurity: skipping reinit, returning true");
        return true;
    } catch (const std::exception& e) {
        RawTrace("InitSecurity: caught std::exception");
        return false;
    } catch (...) {
        RawTrace("InitSecurity: caught unknown exception");
        return false;
    }
}

bool RAINMGRApp::CreateMainWindow() {
    try {
        // Register window class
        RawTrace("CreateMainWindow: registering class");
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = MainWindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = sizeof(LONG_PTR); // Store 'this' pointer
        wc.hInstance = hInstance_;
        // Load application icons with safe fallbacks
        HICON hIconLarge = (HICON)LoadImageW(hInstance_, L"IDI_ICON1", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
        if (!hIconLarge) hIconLarge = LoadIconW(hInstance_, MAKEINTRESOURCEW(101));
        if (!hIconLarge) hIconLarge = LoadIconW(nullptr, IDI_APPLICATION);
        wc.hIcon = hIconLarge;

        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
        wc.lpszClassName = MAIN_WINDOW_CLASS;

        HICON hIconSmall = (HICON)LoadImageW(hInstance_, L"IDI_ICON1", IMAGE_ICON, 16, 16, 0);
        if (!hIconSmall) hIconSmall = LoadIconW(hInstance_, MAKEINTRESOURCEW(101));
        if (!hIconSmall) hIconSmall = LoadIconW(nullptr, IDI_APPLICATION);
        wc.hIconSm = hIconSmall;
        
        ATOM cls = RegisterClassExW(&wc);
        if (!cls) {
            DWORD error = GetLastError();
            if (error != ERROR_CLASS_ALREADY_EXISTS) {
                LogApplicationEvent(L"Failed to register window class, error: " + std::to_wstring(error), Core::LogLevel::Error);
                RawTrace("CreateMainWindow: RegisterClassEx failed");
                return false;
            }
        }
        
        // Create main window (hidden initially)
        RawTrace("CreateMainWindow: calling CreateWindowExW");
        mainWindow_ = CreateWindowExW(
            0,                              // Extended styles
            MAIN_WINDOW_CLASS,              // Class name
            L"Rainmeter Manager",           // Window title
            WS_OVERLAPPEDWINDOW,            // Window style
            CW_USEDEFAULT, CW_USEDEFAULT,   // Position
            1280, 800,                      // Size
            nullptr,                        // Parent
            nullptr,                        // Menu
            hInstance_,                     // Instance
            this                            // User data (this pointer)
        );
        RawTrace("CreateMainWindow: CreateWindowExW returned");
    }
}

void RAINMGRApp::ExecuteShutdownHandlers() {
    std::lock_guard<std::mutex> lock(shutdownMutex_);
    
    LogApplicationEvent(L"Executing shutdown handlers (" + std::to_wstring(shutdownHandlers_.size()) + L")...");
    
    // Execute in reverse order (LIFO)
    for (auto it = shutdownHandlers_.rbegin(); it != shutdownHandlers_.rend(); ++it) {
        try {
            (*it)();
        } catch (const std::exception& e) {
            std::string errorMsg = "Exception in shutdown handler: ";
            errorMsg += e.what();
            std::wstring wideError(errorMsg.begin(), errorMsg.end());
            LogApplicationEvent(wideError, Core::LogLevel::Error);
        } catch (...) {
            LogApplicationEvent(L"Unknown exception in shutdown handler", Core::LogLevel::Error);
        }
    }
    
    shutdownHandlers_.clear();
}

void RAINMGRApp::ShowMainWindow() {
    if (mainWindow_) {
        ShowWindow(mainWindow_, SW_SHOW);
        UpdateWindow(mainWindow_);
        LogApplicationEvent(L"Main window shown");
    }
}

void RAINMGRApp::CleanupResources() {
    LogApplicationEvent(L"Cleaning up resources...");
    
    // Destroy main window
    if (mainWindow_) {
        DestroyWindow(mainWindow_);
        mainWindow_ = nullptr;
    }
    
    // Unregister window class
    UnregisterClass(MAIN_WINDOW_CLASS, hInstance_);
}

void RAINMGRApp::CleanupServices() {
    LogApplicationEvent(L"Cleaning up services...");
    
    // Clear service locator
    if (serviceLocator_) {
        serviceLocator_->Clear();
        serviceLocator_.reset();
    }
    
    // Cleanup security framework
    Core::Security::Cleanup();
}

LRESULT CALLBACK RAINMGRApp::MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    char buf[64];
    sprintf_s(buf, "WndProc: msg=0x%04X", (unsigned int)msg);
    RawTrace(buf);
    RAINMGRApp* app = nullptr;
    
    if (msg == WM_NCCREATE) {
        // Get the this pointer from CREATESTRUCTW at earliest opportunity
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        if (cs && cs->lpCreateParams) {
            app = reinterpret_cast<RAINMGRApp*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        }
        return TRUE; // Continue window creation
    } else {
        // Get the this pointer from window user data
        app = reinterpret_cast<RAINMGRApp*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
    
    if (app) {
        return app->HandleWindowMessage(hwnd, msg, wParam, lParam);
    }
    
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT RAINMGRApp::HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Post deferred init message to ensure HWND fully valid
            PostMessageW(hwnd, WM_APP + 1, 0, 0);
            return 0;
        }
        
        case WM_APP + 1: {
            // Deferred initialization after window creation is complete
            if (IsWindow(hwnd)) {
                StartDeferredInitialization(hwnd);
            }
            return 0;
        }
        
        case WM_CLOSE:
            LogApplicationEvent(L"WM_CLOSE received");
            RequestShutdown();
            return 0;
        
        case WM_DESTROY:
            LogApplicationEvent(L"WM_DESTROY received");
            PostQuitMessage(0);
            return 0;
        
        case WM_SIZE:
            // Handle window resize
            if (wParam != SIZE_MINIMIZED) {
                UINT width = LOWORD(lParam);
                UINT height = HIWORD(lParam);
                LogApplicationEvent(L"Window resized to: " + std::to_wstring(width) + L"x" + std::to_wstring(height));
                // TODO: Notify UI components of size change
            }
            break;
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // Simple placeholder drawing
            RECT rect;
            GetClientRect(hwnd, &rect);
            
            // Draw a simple text message
            const wchar_t* message = L"Rainmeter Manager - Phase 2 Application Core";
            DrawText(hdc, message, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        default:
            break;
    }
    
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void RAINMGRApp::StartDeferredInitialization(HWND hwnd) {
    if (!IsWindow(hwnd)) {
        LogApplicationEvent(L"StartDeferredInitialization: Invalid window handle", Core::LogLevel::Error);
        return;
    }
    
    LogApplicationEvent(L"Starting deferred initialization...");
    
    try {
        // Initialize IPC Manager AFTER window is fully created to avoid AVs
        // TODO: Uncomment when IPCManager is implemented
        // IPCManager::Instance().Initialize();
        
        // Launch RenderProcess with validated args
        // TODO: Uncomment when RenderProcess launcher is implemented
        // LaunchRenderProcess();
        
        // Trigger first paint
        InvalidateRect(hwnd, nullptr, TRUE);
        
        LogApplicationEvent(L"Deferred initialization completed");
        
    } catch (const std::exception& e) {
        std::string errorMsg = "Exception in deferred initialization: ";
        errorMsg += e.what();
        std::wstring wideError(errorMsg.begin(), errorMsg.end());
        LogApplicationEvent(wideError, Core::LogLevel::Error);
    } catch (...) {
        LogApplicationEvent(L"Unknown exception in deferred initialization", Core::LogLevel::Error);
    }
}

void RAINMGRApp::HandleFatalError(const std::wstring& error) {
    LogApplicationEvent(L"FATAL ERROR: " + error, Core::LogLevel::Error);
    
    // Show message box for critical errors
    MessageBox(nullptr, error.c_str(), L"Rainmeter Manager - Fatal Error", MB_ICONERROR | MB_OK);
    
    // Attempt cleanup
    if (initialized_) {
        Shutdown();
    }
}

void RAINMGRApp::LogApplicationEvent(const std::wstring& event, Core::LogLevel level) {
    auto toUtf8 = [](const std::wstring& w) -> std::string {
        if (w.empty()) return std::string();
        int sz = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
        if (sz <= 0) return std::string();
        std::string out((size_t)sz, '\0');
        ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), out.data(), sz, nullptr, nullptr);
        return out;
    };
    std::string msg = std::string("RAINMGRApp: ") + toUtf8(event);
    switch (level) {
        case Core::LogLevel::Error:
            ::Logger::error(msg);
            break;
        case Core::LogLevel::Warning:
            ::Logger::warning(msg);
            break;
        case Core::LogLevel::Info:
        default:
            ::Logger::info(msg);
            break;
    }
}

} // namespace App
} // namespace RainmeterManager
