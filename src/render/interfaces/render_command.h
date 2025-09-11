#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <variant>

namespace RainmeterManager::Render {

    // Render backend types
    enum class RenderBackendType : uint32_t {
        SkiaSharp = 0,
        Direct3D = 1,
        WebView = 2,
        Auto = 99  // Auto-select best backend
    };

    // Content source types
    enum class ContentSourceType : uint32_t {
        Static = 0,      // Static content (text, images)
        Web = 1,         // Web URLs, HTML content
        API = 2,         // REST/GraphQL APIs
        Media = 3,       // Video/audio streams
        File = 4,        // Local files
        Office = 5,      // Excel, Word, PowerPoint
        Custom = 99      // Custom content source
    };

    // Render command types
    enum class RenderCommandType : uint32_t {
        Initialize = 0,
        Render = 1,
        Resize = 2,
        Destroy = 3,
        SwitchBackend = 4,
        UpdateContent = 5,
        SetProperty = 6,
        // Diagnostics / Data queries (must align with C#)
        GetSystemSnapshot = 100,
        GetProcessSnapshot = 101
    };

    // Render result status
    enum class RenderResultStatus : uint32_t {
        Success = 0,
        Failure = 1,
        Pending = 2,
        BackendNotSupported = 3,
        ContentLoadError = 4,
        InvalidParameters = 5
    };

    // Basic geometric structures
    struct RenderRect {
        int x, y, width, height;
        
        RenderRect() : x(0), y(0), width(0), height(0) {}
        RenderRect(int x, int y, int w, int h) : x(x), y(y), width(w), height(h) {}
    };

    struct RenderColor {
        uint8_t r, g, b, a;
        
        RenderColor() : r(0), g(0), b(0), a(255) {}
        RenderColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) 
            : r(red), g(green), b(blue), a(alpha) {}
    };

    // Content source parameters
    struct ContentParameters {
        ContentSourceType sourceType;
        std::string sourceUrl;           // URL, file path, API endpoint
        std::string templatePath;        // Optional template file
        std::string authToken;           // Authentication token for APIs
        int refreshIntervalMs;           // Content refresh interval
        bool cacheEnabled;               // Enable content caching
        std::vector<std::pair<std::string, std::string>> customHeaders;  // HTTP headers
        std::vector<std::pair<std::string, std::string>> parameters;     // Custom parameters
        
        ContentParameters() : sourceType(ContentSourceType::Static), 
                            refreshIntervalMs(0), cacheEnabled(true) {}
    };

    // Render properties
    struct RenderProperties {
        float opacity = 1.0f;
        bool visible = true;
        bool clickThrough = false;
        bool topMost = false;
        RenderColor backgroundColor = RenderColor(0, 0, 0, 0);
        int zOrder = 0;
        bool enableAnimations = true;
        int targetFps = 60;
        bool enableVSync = true;
        
        // Advanced properties
        float scaleX = 1.0f;
        float scaleY = 1.0f;
        float rotation = 0.0f;
        bool enableBlur = false;
        float blurRadius = 0.0f;
        bool enableShadow = false;
        RenderColor shadowColor = RenderColor(0, 0, 0, 128);
        int shadowOffsetX = 2;
        int shadowOffsetY = 2;
    };

    // Main render command structure
    struct RenderCommand {
        uint64_t commandId;              // Unique command identifier
        RenderCommandType commandType;
        uint32_t widgetId;               // Widget identifier
        HWND windowHandle;               // Target window handle
        RenderBackendType backendType;
        RenderRect bounds;               // Rendering bounds
        ContentParameters content;       // Content parameters
        RenderProperties properties;     // Render properties
        uint64_t timestamp;              // Command timestamp
        
        RenderCommand() : commandId(0), commandType(RenderCommandType::Initialize),
                         widgetId(0), windowHandle(nullptr), 
                         backendType(RenderBackendType::Auto),
                         timestamp(0) {}
    };

    // Render result structure
    struct RenderResult {
        uint64_t commandId;              // Matching command ID
        uint32_t widgetId;
        RenderResultStatus status;
        std::string errorMessage;
        uint64_t renderTimeMs;           // Render time in milliseconds
        uint32_t frameCount;             // Frames rendered
        float averageFps;                // Average FPS
        size_t memoryUsageMB;            // Memory usage in MB
        uint64_t timestamp;              // Result timestamp
        
        RenderResult() : commandId(0), widgetId(0), 
                        status(RenderResultStatus::Pending),
                        renderTimeMs(0), frameCount(0), averageFps(0.0f),
                        memoryUsageMB(0), timestamp(0) {}
    };

    // Performance metrics
    struct PerformanceMetrics {
        float currentFps;
        float averageFps;
        uint64_t totalFrames;
        uint64_t droppedFrames;
        size_t memoryUsageMB;
        size_t vramUsageMB;
        float cpuUsagePercent;
        float gpuUsagePercent;
        uint64_t renderTimeMs;
        uint64_t contentLoadTimeMs;
        
        PerformanceMetrics() : currentFps(0.0f), averageFps(0.0f), totalFrames(0),
                              droppedFrames(0), memoryUsageMB(0), vramUsageMB(0),
                              cpuUsagePercent(0.0f), gpuUsagePercent(0.0f),
                              renderTimeMs(0), contentLoadTimeMs(0) {}
    };

    // Monitor information
    struct MonitorInfo {
        int monitorId;
        HMONITOR hMonitor;
        RenderRect bounds;               // Monitor bounds
        RenderRect workArea;             // Work area (excluding taskbar)
        float dpiX, dpiY;               // DPI scaling factors
        bool isPrimary;
        std::string deviceName;
        
        MonitorInfo() : monitorId(0), hMonitor(nullptr), dpiX(96.0f), dpiY(96.0f), 
                       isPrimary(false) {}
    };

    // System capabilities
    struct SystemCapabilities {
        bool supportsSkiaSharp;
        bool supportsDirect3D;
        bool supportsWebView2;
        bool supportsHardwareAcceleration;
        bool supportsMultiMonitor;
        bool supportsHighDPI;
        std::string gpuName;
        std::string driverVersion;
        size_t totalVRAM;
        size_t availableVRAM;
        
        SystemCapabilities() : supportsSkiaSharp(false), supportsDirect3D(false),
                              supportsWebView2(false), supportsHardwareAcceleration(false),
                              supportsMultiMonitor(false), supportsHighDPI(false),
                              totalVRAM(0), availableVRAM(0) {}
    };

} // namespace RainmeterManager::Render
