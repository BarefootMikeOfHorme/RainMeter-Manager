#include "rainmgrapp.h"
#include "../core/error_handling.h"
#include "../core/security.h"
#include <filesystem>
#include <shlwapi.h>
#include <atomic>
#include <thread>

#pragma comment(lib, "shlwapi.lib")

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
    LogApplicationEvent(L"RAINMGRApp instance created");
}

RAINMGRApp::~RAINMGRApp() {
    if (initialized_) {
        Shutdown();
    }
    LogApplicationEvent(L"RAINMGRApp instance destroyed");
}

RAINMGRApp& RAINMGRApp::GetInstance(HINSTANCE hInstance) {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    
    if (!instance_) {
        if (hInstance == nullptr) {
            throw std::runtime_error("First call to GetInstance requires valid HINSTANCE");
        }
        instance_ = std::unique_ptr<RAINMGRApp>(new RAINMGRApp(hInstance));
    }
    
    return *instance_;
}

void RAINMGRApp::DestroyInstance() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    instance_.reset();
}

bool RAINMGRApp::Initialize() {
    try {
        LogApplicationEvent(L"Starting application initialization...");
        
        // Initialize paths first
        if (!InitializePaths()) {
            HandleFatalError(L"Failed to initialize application paths");
            return false;
        }
        
        // Initialize logging system
        if (!InitializeLogging()) {
            HandleFatalError(L"Failed to initialize logging system");
            return false;
        }
        
        // Initialize security framework
        if (!InitializeSecurity()) {
            HandleFatalError(L"Failed to initialize security framework");
            return false;
        }
        
        // Create service locator
        serviceLocator_ = std::make_unique<Core::ServiceLocator>();
        
        // Initialize core services
        if (!InitializeServices()) {
            HandleFatalError(L"Failed to initialize core services");
            return false;
        }
        
        // Create main window
        if (!CreateMainWindow()) {
            HandleFatalError(L"Failed to create main application window");
            return false;
        }
        
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
        // Get application executable path
        wchar_t exePath[MAX_PATH];
        if (GetModuleFileName(hInstance_, exePath, MAX_PATH) == 0) {
            LogApplicationEvent(L"Failed to get module file name", Core::LogLevel::Error);
            return false;
        }
        
        // Extract directory path
        wchar_t* lastSlash = wcsrchr(exePath, L'\\');
        if (lastSlash) {
            *lastSlash = L'\0';
        }
        
        applicationPath_ = exePath;
        
        // Set configuration path (same as application path for now)
        configPath_ = applicationPath_ + L"\\config";
        
        // Ensure config directory exists
        std::error_code ec;
        std::filesystem::create_directories(configPath_, ec);
        if (ec) {
            LogApplicationEvent(L"Failed to create config directory: " + configPath_, Core::LogLevel::Warning);
        }
        
        LogApplicationEvent(L"Application path: " + applicationPath_);
        LogApplicationEvent(L"Configuration path: " + configPath_);
        
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
        LogApplicationEvent(L"Initializing core services...");
        
        // Register core services with the service locator
        // Note: In a real implementation, you would register concrete service implementations here
        
        LogApplicationEvent(L"Core services initialized successfully");
        return true;
    } catch (const std::exception& e) {
        std::string errorMsg = "Exception in InitializeServices: ";
        errorMsg += e.what();
        std::wstring wideError(errorMsg.begin(), errorMsg.end());
        LogApplicationEvent(wideError, Core::LogLevel::Error);
        return false;
    }
}

bool RAINMGRApp::InitializeLogging() {
    try {
        // Logger is already initialized as singleton, just ensure it's working
        LogApplicationEvent(L"Logging system verified");
        return true;
    } catch (const std::exception& e) {
        // This is tricky since logging might not be working
        return false;
    }
}

bool RAINMGRApp::InitializeSecurity() {
    try {
        // Initialize security framework
        if (!Core::Security::Initialize()) {
            LogApplicationEvent(L"Security framework initialization failed", Core::LogLevel::Error);
            return false;
        }
        
        LogApplicationEvent(L"Security framework initialized successfully");
        return true;
    } catch (const std::exception& e) {
        std::string errorMsg = "Exception in InitializeSecurity: ";
        errorMsg += e.what();
        std::wstring wideError(errorMsg.begin(), errorMsg.end());
        LogApplicationEvent(wideError, Core::LogLevel::Error);
        return false;
    }
}

bool RAINMGRApp::CreateMainWindow() {
    try {
        // Register window class
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = MainWindowProc;
        wc.hInstance = hInstance_;
        wc.hIcon = LoadIcon(hInstance_, MAKEINTRESOURCE(101));  // Assuming icon resource ID 101
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = MAIN_WINDOW_CLASS;
        wc.hIconSm = LoadIcon(hInstance_, MAKEINTRESOURCE(101));
        
        if (!RegisterClassEx(&wc)) {
            DWORD error = GetLastError();
            if (error != ERROR_CLASS_ALREADY_EXISTS) {
                LogApplicationEvent(L"Failed to register window class, error: " + std::to_wstring(error), Core::LogLevel::Error);
                return false;
            }
        }
        
        // Create main window (hidden initially)
        mainWindow_ = CreateWindowEx(
            0,                              // Extended styles
            MAIN_WINDOW_CLASS,              // Class name
            L"Rainmeter Manager",           // Window title
            WS_OVERLAPPEDWINDOW,            // Window style
            CW_USEDEFAULT, CW_USEDEFAULT,   // Position
            800, 600,                       // Size
            nullptr,                        // Parent
            nullptr,                        // Menu
            hInstance_,                     // Instance
            this                           // User data (this pointer)
        );
        
        if (!mainWindow_) {
            DWORD error = GetLastError();
            LogApplicationEvent(L"Failed to create main window, error: " + std::to_wstring(error), Core::LogLevel::Error);
            return false;
        }
        
        // Show and update window
        ShowWindow(mainWindow_, SW_SHOW);
        UpdateWindow(mainWindow_);
        
        LogApplicationEvent(L"Main window created successfully");
        return true;
    } catch (const std::exception& e) {
        std::string errorMsg = "Exception in CreateMainWindow: ";
        errorMsg += e.what();
        std::wstring wideError(errorMsg.begin(), errorMsg.end());
        LogApplicationEvent(wideError, Core::LogLevel::Error);
        return false;
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
    RAINMGRApp* app = nullptr;
    
    if (msg == WM_NCCREATE) {
        // Get the this pointer from CREATESTRUCT
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        app = reinterpret_cast<RAINMGRApp*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    } else {
        // Get the this pointer from window user data
        app = reinterpret_cast<RAINMGRApp*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (app) {
        return app->HandleWindowMessage(hwnd, msg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT RAINMGRApp::HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
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
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
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
    try {
        auto& logger = Core::Logger::GetInstance();
        switch (level) {
            case Core::LogLevel::Error:
                logger.LogError(L"RAINMGRApp: " + event);
                break;
            case Core::LogLevel::Warning:
                logger.LogWarning(L"RAINMGRApp: " + event);
                break;
            case Core::LogLevel::Info:
            default:
                logger.LogInfo(L"RAINMGRApp: " + event);
                break;
        }
    } catch (...) {
        // If logging fails, there's not much we can do
        // In a production system, you might write to Windows Event Log as fallback
    }
}

} // namespace App
} // namespace RainmeterManager
