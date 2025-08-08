#pragma once

#include "../interfaces/render_command.h"
#include "../../core/logger.h"
#include <windows.h>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <atomic>

namespace RainmeterManager::Render {

    // Monitor event callbacks
    using MonitorChangeCallback = std::function<void(const std::vector<MonitorInfo>&)>;
    using DPIChangeCallback = std::function<void(HMONITOR, float, float)>;

    /**
     * @brief Monitor management and DPI awareness system
     * 
     * This class provides comprehensive multi-monitor support with
     * DPI awareness, automatic scaling, and monitor change detection
     * for enterprise desktop environments.
     */
    class MonitorManager {
    public:
        MonitorManager();
        ~MonitorManager();

        // === Initialization & Lifecycle ===
        
        /**
         * @brief Initialize monitor management system
         * @return True if initialization succeeded
         */
        bool Initialize();
        
        /**
         * @brief Cleanup monitor management resources
         */
        void Cleanup();
        
        /**
         * @brief Check if monitor manager is initialized
         * @return True if system is operational
         */
        bool IsInitialized() const;

        // === Monitor Discovery ===
        
        /**
         * @brief Refresh monitor information
         * @return True if refresh succeeded
         */
        bool RefreshMonitors();
        
        /**
         * @brief Get all available monitors
         * @return Vector of monitor information
         */
        std::vector<MonitorInfo> GetAllMonitors() const;
        
        /**
         * @brief Get monitor count
         * @return Number of available monitors
         */
        size_t GetMonitorCount() const;
        
        /**
         * @brief Get primary monitor information
         * @return Primary monitor info (empty if not found)
         */
        MonitorInfo GetPrimaryMonitor() const;
        
        /**
         * @brief Get monitor by ID
         * @param monitorId Monitor identifier
         * @return Monitor information (empty if not found)
         */
        MonitorInfo GetMonitorById(int monitorId) const;
        
        /**
         * @brief Get monitor containing point
         * @param x Screen X coordinate
         * @param y Screen Y coordinate
         * @return Monitor information (empty if not found)
         */
        MonitorInfo GetMonitorFromPoint(int x, int y) const;
        
        /**
         * @brief Get monitor containing window
         * @param windowHandle Window handle
         * @return Monitor information (empty if not found)
         */
        MonitorInfo GetMonitorFromWindow(HWND windowHandle) const;

        // === DPI Management ===
        
        /**
         * @brief Get DPI for monitor
         * @param monitorId Monitor identifier
         * @param dpiX Output DPI X value
         * @param dpiY Output DPI Y value
         * @return True if DPI retrieved successfully
         */
        bool GetMonitorDPI(int monitorId, float& dpiX, float& dpiY) const;
        
        /**
         * @brief Get DPI scaling factor
         * @param monitorId Monitor identifier
         * @return DPI scaling factor (1.0 = 96 DPI)
         */
        float GetDPIScalingFactor(int monitorId) const;
        
        /**
         * @brief Convert logical pixels to physical pixels
         * @param monitorId Monitor identifier
         * @param logicalPixels Logical pixel value
         * @return Physical pixel value
         */
        int LogicalToPhysicalPixels(int monitorId, int logicalPixels) const;
        
        /**
         * @brief Convert physical pixels to logical pixels
         * @param monitorId Monitor identifier
         * @param physicalPixels Physical pixel value
         * @return Logical pixel value
         */
        int PhysicalToLogicalPixels(int monitorId, int physicalPixels) const;
        
        /**
         * @brief Scale rectangle for DPI
         * @param monitorId Monitor identifier
         * @param rect Rectangle to scale
         * @return Scaled rectangle
         */
        RenderRect ScaleRectForDPI(int monitorId, const RenderRect& rect) const;

        // === Monitor Layout ===
        
        /**
         * @brief Get virtual screen bounds
         * @return Combined bounds of all monitors
         */
        RenderRect GetVirtualScreenBounds() const;
        
        /**
         * @brief Check if monitors are arranged horizontally
         * @return True if horizontal arrangement detected
         */
        bool IsHorizontalLayout() const;
        
        /**
         * @brief Get monitor arrangement type
         */
        enum class MonitorArrangement {
            Single,
            HorizontalDual,
            VerticalDual,
            MultipleHorizontal,
            MultipleVertical,
            Mixed,
            Unknown
        };
        
        /**
         * @brief Get current monitor arrangement
         * @return Monitor arrangement type
         */
        MonitorArrangement GetMonitorArrangement() const;
        
        /**
         * @brief Get monitor relative position
         * @param monitorId Monitor identifier
         * @return Position relative to primary monitor
         */
        RenderRect GetRelativePosition(int monitorId) const;

        // === Window Management ===
        
        /**
         * @brief Move window to monitor
         * @param windowHandle Window to move
         * @param monitorId Target monitor ID
         * @param preserveRelativePosition Keep relative position on monitor
         * @return True if move succeeded
         */
        bool MoveWindowToMonitor(HWND windowHandle, int monitorId, 
                                bool preserveRelativePosition = true);
        
        /**
         * @brief Center window on monitor
         * @param windowHandle Window to center
         * @param monitorId Target monitor ID
         * @return True if centering succeeded
         */
        bool CenterWindowOnMonitor(HWND windowHandle, int monitorId);
        
        /**
         * @brief Maximize window to monitor work area
         * @param windowHandle Window to maximize
         * @param monitorId Target monitor ID
         * @return True if maximize succeeded
         */
        bool MaximizeWindowToMonitor(HWND windowHandle, int monitorId);
        
        /**
         * @brief Get optimal monitor for window
         * @param windowHandle Window handle
         * @param preferPrimary Prefer primary monitor
         * @return Recommended monitor ID
         */
        int GetOptimalMonitorForWindow(HWND windowHandle, bool preferPrimary = false) const;

        // === Event Handling ===
        
        /**
         * @brief Set monitor change callback
         * @param callback Function to call when monitors change
         */
        void SetMonitorChangeCallback(MonitorChangeCallback callback);
        
        /**
         * @brief Set DPI change callback
         * @param callback Function to call when DPI changes
         */
        void SetDPIChangeCallback(DPIChangeCallback callback);
        
        /**
         * @brief Enable automatic monitor change detection
         * @param enabled True to enable detection
         */
        void EnableChangeDetection(bool enabled);
        
        /**
         * @brief Handle Windows display change message
         * @param wParam Message wParam
         * @param lParam Message lParam
         */
        void HandleDisplayChange(WPARAM wParam, LPARAM lParam);
        
        /**
         * @brief Handle Windows DPI change message
         * @param windowHandle Affected window
         * @param wParam Message wParam
         * @param lParam Message lParam
         */
        void HandleDPIChange(HWND windowHandle, WPARAM wParam, LPARAM lParam);

        // === Advanced Features ===
        
        /**
         * @brief Get monitor color profile
         * @param monitorId Monitor identifier
         * @return Color profile path (empty if not available)
         */
        std::wstring GetMonitorColorProfile(int monitorId) const;
        
        /**
         * @brief Get monitor refresh rate
         * @param monitorId Monitor identifier
         * @return Refresh rate in Hz (0 if unknown)
         */
        int GetMonitorRefreshRate(int monitorId) const;
        
        /**
         * @brief Check if monitor supports HDR
         * @param monitorId Monitor identifier
         * @return True if HDR is supported
         */
        bool IsHDRSupported(int monitorId) const;
        
        /**
         * @brief Get monitor bit depth
         * @param monitorId Monitor identifier
         * @return Color bit depth (0 if unknown)
         */
        int GetMonitorBitDepth(int monitorId) const;
        
        /**
         * @brief Check if monitor is touch-enabled
         * @param monitorId Monitor identifier
         * @return True if touch is supported
         */
        bool IsTouchSupported(int monitorId) const;

        // === Diagnostics & Debugging ===
        
        /**
         * @brief Get monitor diagnostics
         * @return JSON string with monitor information
         */
        std::string GetMonitorDiagnostics() const;
        
        /**
         * @brief Validate monitor configuration
         * @return True if configuration is valid
         */
        bool ValidateConfiguration() const;
        
        /**
         * @brief Log monitor information
         */
        void LogMonitorInformation() const;

    private:
        // === Internal Structures ===
        
        struct MonitorData {
            MonitorInfo info;
            HMONITOR hMonitor;
            HDC hdc;
            std::wstring deviceName;
            std::wstring friendlyName;
            std::wstring colorProfile;
            int refreshRate;
            int bitDepth;
            bool hdrSupported;
            bool touchSupported;
            std::chrono::steady_clock::time_point lastUpdate;
        };
        
        struct MonitorChangeData {
            bool changeDetectionEnabled;
            std::chrono::steady_clock::time_point lastChangeTime;
            std::vector<MonitorInfo> previousMonitors;
            UINT_PTR changeTimer;
        };

        // === Member Variables ===
        
        // Core components
        Core::Logger& logger_;
        
        // Monitor tracking
        std::vector<std::unique_ptr<MonitorData>> monitors_;
        mutable std::shared_mutex monitorsMutex_;
        std::atomic<bool> initialized_{false};
        
        // Change detection
        MonitorChangeData changeData_;
        MonitorChangeCallback monitorChangeCallback_;
        DPIChangeCallback dpiChangeCallback_;
        mutable std::mutex callbackMutex_;
        
        // DPI awareness
        std::unordered_map<int, float> dpiScalingCache_;
        mutable std::mutex dpiCacheMutex_;
        bool dpiAwarenessSet_;
        
        // Configuration
        struct MonitorConfig {
            bool enableChangeDetection = true;
            uint32_t changeDetectionDelayMs = 500;
            bool enableDPIAwareness = true;
            bool enablePerMonitorDPI = true;
            bool logMonitorChanges = true;
        } config_;
        
        // Statistics
        struct MonitorStatistics {
            std::atomic<uint64_t> monitorChanges{0};
            std::atomic<uint64_t> dpiChanges{0};
            std::atomic<uint64_t> windowMoves{0};
            std::chrono::steady_clock::time_point startTime;
        } stats_;

        // === Internal Methods ===
        
        // Initialization
        bool SetupDPIAwareness();
        bool InitializeMonitorList();
        void SetupChangeDetection();
        
        // Monitor enumeration
        static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, 
                                            LPRECT lprcMonitor, LPARAM dwData);
        bool PopulateMonitorInfo(MonitorData& monitorData);
        bool GetMonitorProperties(MonitorData& monitorData);
        bool GetMonitorCapabilities(MonitorData& monitorData);
        
        // Change detection
        static void CALLBACK ChangeDetectionTimer(HWND hwnd, UINT uMsg, 
                                                 UINT_PTR idEvent, DWORD dwTime);
        void ProcessMonitorChanges();
        bool CompareMonitorSets(const std::vector<MonitorInfo>& set1, 
                               const std::vector<MonitorInfo>& set2) const;
        
        // DPI management
        bool UpdateDPICache();
        float CalculateDPIScaling(float dpi) const;
        bool GetMonitorDPIInternal(HMONITOR hMonitor, float& dpiX, float& dpiY) const;
        
        // Utility methods
        int FindMonitorById(int monitorId) const;
        int FindMonitorByHandle(HMONITOR hMonitor) const;
        MonitorArrangement AnalyzeMonitorArrangement() const;
        std::string SerializeMonitorInfo() const;
        void UpdateStatistics();
        
        // Cleanup
        void CleanupMonitorList();
        void CleanupChangeDetection();

        // Non-copyable
        MonitorManager(const MonitorManager&) = delete;
        MonitorManager& operator=(const MonitorManager&) = delete;
    };

    /**
     * @brief Factory function to create monitor manager
     * @return Unique pointer to monitor manager
     */
    std::unique_ptr<MonitorManager> CreateMonitorManager();

} // namespace RainmeterManager::Render
