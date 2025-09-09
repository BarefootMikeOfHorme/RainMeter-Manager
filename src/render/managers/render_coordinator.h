#pragma once

#include "../interfaces/render_command.h"
#include "../interfaces/irender_backend_proxy.h"
#include "../ipc/render_ipc_bridge.h"
#include "../../core/logger.h"
#include "../../core/service_locator.h"
#include "../../config/config_manager.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <queue>
#include <functional>
#include <future>

namespace RainmeterManager::Render {

    // Forward declarations
    class MonitorManager;
    
    // Render event callbacks
    using RenderReadyCallback = std::function<void()>;
    using RenderErrorCallback = std::function<void(const std::string&, uint32_t widgetId)>;
    using PerformanceUpdateCallback = std::function<void(const PerformanceMetrics&)>;

    /**
     * @brief Central coordinator for all rendering operations
     * 
     * This class manages the render process lifecycle, coordinates
     * multiple render backends, handles widget lifecycle, and
     * provides a unified interface for the application.
     */
    class RenderCoordinator {
    public:
        RenderCoordinator();
        ~RenderCoordinator();

        // === Lifecycle Management ===
        
        /**
         * @brief Initialize the render coordinator
         * @param configPath Path to render configuration file
         * @return True if initialization succeeded
         */
        bool Initialize(const std::wstring& configPath = L"");
        
        /**
         * @brief Start the rendering system
         * @return True if startup succeeded
         */
        bool Start();
        
        /**
         * @brief Stop the rendering system gracefully
         * @param timeoutMs Shutdown timeout in milliseconds
         * @return True if shutdown completed gracefully
         */
        bool Stop(uint32_t timeoutMs = 10000);
        
        /**
         * @brief Check if render system is running
         * @return True if system is operational
         */
        bool IsRunning() const;

        // === Widget Management ===
        
        /**
         * @brief Create a new render widget
         * @param windowHandle Target window handle
         * @param bounds Initial widget bounds
         * @param backendType Preferred backend type
         * @return Widget ID (0 if creation failed)
         */
        uint32_t CreateWidget(HWND windowHandle, const RenderRect& bounds, 
                             RenderBackendType backendType = RenderBackendType::Auto);
        
        /**
         * @brief Destroy a render widget
         * @param widgetId Widget identifier
         * @return True if destruction succeeded
         */
        bool DestroyWidget(uint32_t widgetId);
        
        /**
         * @brief Get render backend proxy for widget
         * @param widgetId Widget identifier
         * @return Shared pointer to backend proxy (null if not found)
         */
        std::shared_ptr<IRenderBackendProxy> GetWidgetBackend(uint32_t widgetId);
        
        /**
         * @brief Get all active widget IDs
         * @return Vector of active widget identifiers
         */
        std::vector<uint32_t> GetActiveWidgets() const;
        
        /**
         * @brief Get widget count
         * @return Number of active widgets
         */
        size_t GetWidgetCount() const;

        // === Rendering Operations ===
        
        /**
         * @brief Render content for specific widget
         * @param widgetId Widget identifier
         * @param content Content parameters
         * @param properties Render properties
         * @return Future for render result
         */
        std::future<RenderResult> RenderWidgetAsync(
            uint32_t widgetId,
            const ContentParameters& content,
            const RenderProperties& properties = RenderProperties{}
        );
        
        /**
         * @brief Update widget content
         * @param widgetId Widget identifier
         * @param content New content parameters
         * @return Future for update result
         */
        std::future<RenderResult> UpdateWidgetContentAsync(
            uint32_t widgetId,
            const ContentParameters& content
        );
        
        /**
         * @brief Resize widget
         * @param widgetId Widget identifier
         * @param newBounds New widget bounds
         * @return Future for resize result
         */
        std::future<RenderResult> ResizeWidgetAsync(
            uint32_t widgetId,
            const RenderRect& newBounds
        );
        
        /**
         * @brief Render all widgets (bulk operation)
         * @return Vector of futures for all render results
         */
        std::vector<std::future<RenderResult>> RenderAllWidgetsAsync();

        // === Backend Management ===
        
        /**
         * @brief Switch widget to different backend
         * @param widgetId Widget identifier
         * @param newBackendType New backend type
         * @return Future for switch result
         */
        std::future<RenderResult> SwitchWidgetBackendAsync(
            uint32_t widgetId,
            RenderBackendType newBackendType
        );
        
        /**
         * @brief Get available render backends
         * @return Vector of supported backend types
         */
        std::vector<RenderBackendType> GetAvailableBackends() const;
        
        /**
         * @brief Get system rendering capabilities
         * @return System capabilities structure
         */
        SystemCapabilities GetSystemCapabilities() const;
        
        /**
         * @brief Get recommended backend for content type
         * @param contentType Type of content to render
         * @return Recommended backend type
         */
        RenderBackendType GetRecommendedBackend(ContentSourceType contentType) const;

        // === Performance & Monitoring ===
        
        /**
         * @brief Process pending render commands
         * Call this from main message loop for optimal performance
         */
        void ProcessPendingCommands();
        
        /**
         * @brief Get overall performance metrics
         * @return Aggregated performance metrics
         */
        PerformanceMetrics GetOverallPerformanceMetrics() const;
        
        /**
         * @brief Get performance metrics for specific widget
         * @param widgetId Widget identifier
         * @return Widget-specific performance metrics
         */
        PerformanceMetrics GetWidgetPerformanceMetrics(uint32_t widgetId) const;
        
        /**
         * @brief Enable performance profiling
         * @param enabled True to enable profiling
         * @param intervalMs Profiling update interval
         */
        void EnablePerformanceProfiling(bool enabled, uint32_t intervalMs = 1000);

        // === Configuration Management ===
        
        /**
         * @brief Reload render configuration
         * @return True if reload succeeded
         */
        bool ReloadConfiguration();
        
        /**
         * @brief Update render settings
         * @param settings New render settings (JSON format)
         * @return True if update succeeded
         */
        bool UpdateRenderSettings(const std::string& settings);
        
        /**
         * @brief Get current render configuration
         * @return Configuration JSON string
         */
        std::string GetCurrentConfiguration() const;

        // === Event Handling ===
        
        /**
         * @brief Set render ready callback
         * @param callback Callback function
         */
        void SetRenderReadyCallback(RenderReadyCallback callback);
        
        /**
         * @brief Set render error callback
         * @param callback Error callback function
         */
        void SetRenderErrorCallback(RenderErrorCallback callback);
        
        /**
         * @brief Set performance update callback
         * @param callback Performance callback function
         */
        void SetPerformanceUpdateCallback(PerformanceUpdateCallback callback);

        // === Monitor Management ===
        
        /**
         * @brief Handle monitor configuration change
         * Called by system when monitor setup changes
         */
        void HandleMonitorChange();
        
        /**
         * @brief Get monitor information
         * @return Vector of monitor information
         */
        std::vector<MonitorInfo> GetMonitorInformation() const;
        
        /**
         * @brief Move widget to specific monitor
         * @param widgetId Widget identifier
         * @param monitorId Target monitor ID
         * @return True if move succeeded
         */
        bool MoveWidgetToMonitor(uint32_t widgetId, int monitorId);

        // === Advanced Features ===
        
        /**
         * @brief Capture widget frame to file
         * @param widgetId Widget identifier
         * @param filePath Output file path
         * @param format Image format (PNG, JPEG, BMP)
         * @return Future for capture result
         */
        std::future<RenderResult> CaptureWidgetFrameAsync(
            uint32_t widgetId,
            const std::string& filePath,
            const std::string& format = "PNG"
        );
        
        /**
         * @brief Start widget animation recording
         * @param widgetId Widget identifier
         * @param outputPath Output video file path
         * @param durationMs Recording duration
         * @param fps Recording frame rate
         * @return True if recording started
         */
        bool StartWidgetRecording(uint32_t widgetId, const std::string& outputPath,
                                 uint32_t durationMs, int fps = 30);
        
        /**
         * @brief Stop widget recording
         * @param widgetId Widget identifier
         * @return True if recording stopped
         */
        bool StopWidgetRecording(uint32_t widgetId);

        // === Debugging & Diagnostics ===
        
        /**
         * @brief Enable debug rendering
         * @param enabled True to enable debug mode
         */
        void EnableDebugRendering(bool enabled);
        
        /**
         * @brief Get render system diagnostics
         * @return Diagnostic information JSON
         */
        std::string GetDiagnosticInfo() const;
        
        /**
         * @brief Validate render system health
         * @return Health check results
         */
        bool ValidateSystemHealth() const;

    private:
        // === Internal Structures ===
        
        struct WidgetInfo {
            uint32_t widgetId;
            HWND windowHandle;
            RenderRect bounds;
            RenderBackendType backendType;
            std::shared_ptr<IRenderBackendProxy> backend;
            std::chrono::steady_clock::time_point lastActivity;
            std::atomic<bool> isActive{true};
            RenderProperties properties;
            ContentParameters lastContent;
            PerformanceMetrics metrics;
        };
        
        struct RenderConfig {
            bool enabled = true;
            RenderBackendType defaultBackend = RenderBackendType::Auto;
            RenderBackendType fallbackBackend = RenderBackendType::SkiaSharp;
            std::wstring processPath;
            std::wstring processArguments;
            bool autoRestart = true;
            int maxRestartAttempts = 3;
            uint32_t restartDelayMs = 2000;
            uint32_t maxConcurrentWidgets = 50;
            bool enableProfiling = true;
            uint32_t profileIntervalMs = 1000;
            bool enableDebugLogging = false;
        };

        // === Member Variables ===
        
        // Core components
        std::shared_ptr<RenderIPCBridge> ipcBridge_;
        std::unique_ptr<MonitorManager> monitorManager_;
        Core::ServiceLocator& serviceLocator_;
        Config::ConfigManager& configManager_;
        Core::Logger& logger_;
        
        // Widget management
        std::unordered_map<uint32_t, std::unique_ptr<WidgetInfo>> widgets_;
        mutable std::shared_mutex widgetsMutex_;
        std::atomic<uint32_t> nextWidgetId_{1};
        
        // Threading and synchronization
        std::thread performanceThread_;
        std::thread maintenanceThread_;
        std::atomic<bool> isRunning_{false};
        std::atomic<bool> shouldStop_{false};
        
        // Configuration
        RenderConfig config_;
        std::wstring configPath_;
        mutable std::mutex configMutex_;
        
        // Performance tracking
        std::atomic<bool> profilingEnabled_{false};
        std::atomic<uint32_t> profilingInterval_{1000};
        mutable std::mutex performanceMutex_;
        PerformanceMetrics overallMetrics_;
        
        // Event callbacks
        RenderReadyCallback renderReadyCallback_;
        RenderErrorCallback renderErrorCallback_;
        PerformanceUpdateCallback performanceUpdateCallback_;
        mutable std::mutex callbackMutex_;
        
        // Statistics and monitoring
        struct RenderStatistics {
            std::atomic<uint64_t> totalRenderCommands{0};
            std::atomic<uint64_t> successfulRenders{0};
            std::atomic<uint64_t> failedRenders{0};
            std::atomic<uint64_t> totalWidgetsCreated{0};
            std::atomic<uint64_t> totalWidgetsDestroyed{0};
            std::atomic<uint64_t> backendSwitches{0};
            std::chrono::steady_clock::time_point startTime;
        } stats_;
        
        // Advanced features
        std::atomic<bool> debugRenderingEnabled_{false};
        std::unordered_map<uint32_t, bool> recordingWidgets_;
        mutable std::mutex recordingMutex_;

        // === Internal Methods ===
        
        // Initialization and configuration
        bool LoadConfiguration();
        bool InitializeIPC();
        bool InitializeMonitorManager();
        bool ValidateConfiguration() const;
        void ApplyConfiguration();
        
        // Widget lifecycle
        uint32_t GenerateWidgetId();
        bool CreateWidgetInternal(uint32_t widgetId, HWND windowHandle, 
                                 const RenderRect& bounds, RenderBackendType backendType);
        void CleanupWidget(uint32_t widgetId);
        void CleanupAllWidgets();
        
        // Background threads
        void PerformanceMonitorLoop();
        void MaintenanceLoop();
        void UpdatePerformanceMetrics();
        void PerformMaintenance();
        
        // Error handling
        void HandleRenderError(const std::string& error, uint32_t widgetId);
        void HandleIPCError(const std::string& error);
        void HandleBackendError(uint32_t widgetId, RenderBackendType backendType);
        
        // Performance and statistics
        void UpdateWidgetMetrics(uint32_t widgetId, const PerformanceMetrics& metrics);
        void AggregatePerformanceMetrics();
        void LogPerformanceStatistics() const;
        
        // Utility methods
        RenderBackendType DetermineOptimalBackend(ContentSourceType contentType, 
                                                  HWND windowHandle) const;
        bool IsWidgetValid(uint32_t widgetId) const;
        std::string FormatDiagnosticInfo() const;
        void NotifyCallbacks();

        // Non-copyable
        RenderCoordinator(const RenderCoordinator&) = delete;
        RenderCoordinator& operator=(const RenderCoordinator&) = delete;
    };

    /**
     * @brief Factory function to create render coordinator
     * @return Unique pointer to render coordinator
     */
    std::unique_ptr<RenderCoordinator> CreateRenderCoordinator();

} // namespace RainmeterManager::Render
