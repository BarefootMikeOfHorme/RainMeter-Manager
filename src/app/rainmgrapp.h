#pragma once

#include <Windows.h>
#include <memory>
#include <string>
#include <functional>
#include "../core/logger.h"
#include "../core/service_locator.h"

namespace RainmeterManager {
namespace App {

/**
 * @brief RAINMGRApp - Main Application Singleton Class
 * 
 * Enterprise-grade application lifecycle management singleton that provides:
 * - Application initialization and shutdown coordination
 * - Service locator integration for dependency injection
 * - Windows message loop management
 * - Resource cleanup orchestration
 * - Exception handling and recovery
 * 
 * Phase 2 Implementation - Core Application Layer
 */
class RAINMGRApp {
private:
    static std::unique_ptr<RAINMGRApp> instance_;
    static std::mutex instance_mutex_;
    
    HINSTANCE hInstance_;
    HWND mainWindow_;
    std::wstring applicationPath_;
    std::wstring configPath_;
    bool initialized_;
    bool shutdownRequested_;
    
    // Service locator for dependency injection
    std::unique_ptr<Core::ServiceLocator> serviceLocator_;
    
    // Message loop control
    std::atomic<bool> messageLoopRunning_;
    DWORD mainThreadId_;
    
    // Shutdown coordination
    std::vector<std::function<void()>> shutdownHandlers_;
    std::mutex shutdownMutex_;

    // Private constructor for singleton
    explicit RAINMGRApp(HINSTANCE hInstance);

public:
    // Singleton access
    static RAINMGRApp& GetInstance(HINSTANCE hInstance = nullptr);
    static void DestroyInstance();
    
    // Prevent copy/move
    RAINMGRApp(const RAINMGRApp&) = delete;
    RAINMGRApp& operator=(const RAINMGRApp&) = delete;
    RAINMGRApp(RAINMGRApp&&) = delete;
    RAINMGRApp& operator=(RAINMGRApp&&) = delete;
    
    ~RAINMGRApp();

    // Application lifecycle management
    bool Initialize();
    int Run();
    void RequestShutdown();
    void Shutdown();
    
    // Message loop management
    bool ProcessMessages();
    void PostQuitMessage(int exitCode = 0);
    
    // Service locator access
    Core::ServiceLocator& GetServiceLocator() { return *serviceLocator_; }
    
    // Window management
    HWND GetMainWindow() const { return mainWindow_; }
    void SetMainWindow(HWND hwnd) { mainWindow_ = hwnd; }
    
    // Application information
    HINSTANCE GetHInstance() const { return hInstance_; }
    const std::wstring& GetApplicationPath() const { return applicationPath_; }
    const std::wstring& GetConfigPath() const { return configPath_; }
    bool IsInitialized() const { return initialized_; }
    bool IsShutdownRequested() const { return shutdownRequested_; }
    
    // Shutdown handler registration
    void RegisterShutdownHandler(std::function<void()> handler);
    void UnregisterAllShutdownHandlers();
    
private:
    // Internal initialization methods
    bool InitializePaths();
    bool InitializeServices();
    bool InitializeLogging();
    bool InitializeSecurity();
    bool CreateMainWindow();
    
    // Internal shutdown methods
    void ExecuteShutdownHandlers();
    void CleanupResources();
    void CleanupServices();
    
    // Message handling
    static LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Error handling
    void HandleFatalError(const std::wstring& error);
    void LogApplicationEvent(const std::wstring& event, Core::LogLevel level = Core::LogLevel::Info);
};

} // namespace App
} // namespace RainmeterManager
