#pragma once

#include "render_command.h"
#include <functional>
#include <future>
#include <memory>

namespace RainmeterManager::Render {

    // Forward declarations
    class RenderIPCBridge;

    // Callback types for asynchronous operations
    using RenderResultCallback = std::function<void(const RenderResult&)>;
    using PerformanceCallback = std::function<void(const PerformanceMetrics&)>;
    using ErrorCallback = std::function<void(const std::string&)>;

    /**
     * @brief Interface for render backend proxy
     * 
     * This interface provides C++ applications with access to the 
     * C# rendering process through IPC communication. It abstracts
     * the complexity of inter-process communication and provides
     * a clean, modern C++ API for rendering operations.
     */
    class IRenderBackendProxy {
    public:
        virtual ~IRenderBackendProxy() = default;

        // === Core Lifecycle Operations ===
        
        /**
         * @brief Initialize the render backend
         * @param backendType Preferred backend type (Auto for best available)
         * @param windowHandle Target window handle for rendering
         * @param bounds Initial rendering bounds
         * @return Future that completes when initialization is done
         */
        virtual std::future<RenderResult> InitializeAsync(
            RenderBackendType backendType,
            HWND windowHandle,
            const RenderRect& bounds
        ) = 0;

        /**
         * @brief Destroy the render backend and cleanup resources
         * @return Future that completes when cleanup is done
         */
        virtual std::future<RenderResult> DestroyAsync() = 0;

        // === Rendering Operations ===

        /**
         * @brief Render a frame with specified content
         * @param content Content parameters to render
         * @param properties Rendering properties and effects
         * @return Future that completes when frame is rendered
         */
        virtual std::future<RenderResult> RenderFrameAsync(
            const ContentParameters& content,
            const RenderProperties& properties = RenderProperties{}
        ) = 0;

        /**
         * @brief Update content without full re-render
         * @param content New content parameters
         * @return Future that completes when content is updated
         */
        virtual std::future<RenderResult> UpdateContentAsync(
            const ContentParameters& content
        ) = 0;

        /**
         * @brief Resize the render surface
         * @param newBounds New rendering bounds
         * @return Future that completes when resize is done
         */
        virtual std::future<RenderResult> ResizeAsync(
            const RenderRect& newBounds
        ) = 0;

        // === Backend Management ===

        /**
         * @brief Switch to a different render backend
         * @param newBackendType New backend type to switch to
         * @return Future that completes when backend switch is done
         */
        virtual std::future<RenderResult> SwitchBackendAsync(
            RenderBackendType newBackendType
        ) = 0;

        /**
         * @brief Get current backend type
         * @return Currently active backend type
         */
        virtual RenderBackendType GetCurrentBackend() const = 0;

        /**
         * @brief Get system rendering capabilities
         * @return System capabilities structure
         */
        virtual SystemCapabilities GetSystemCapabilities() const = 0;

        // === Property Management ===

        /**
         * @brief Set render properties
         * @param properties New render properties
         * @return Future that completes when properties are applied
         */
        virtual std::future<RenderResult> SetPropertiesAsync(
            const RenderProperties& properties
        ) = 0;

        /**
         * @brief Get current render properties
         * @return Current render properties
         */
        virtual RenderProperties GetCurrentProperties() const = 0;

        // === Performance & Monitoring ===

        /**
         * @brief Get current performance metrics
         * @return Current performance metrics
         */
        virtual PerformanceMetrics GetPerformanceMetrics() const = 0;

        /**
         * @brief Set performance monitoring callback
         * @param callback Callback function for performance updates
         * @param intervalMs Update interval in milliseconds
         */
        virtual void SetPerformanceCallback(
            PerformanceCallback callback,
            uint32_t intervalMs = 1000
        ) = 0;

        /**
         * @brief Set error callback for async error handling
         * @param callback Error callback function
         */
        virtual void SetErrorCallback(ErrorCallback callback) = 0;

        // === Synchronous Operations (for simple use cases) ===

        /**
         * @brief Synchronous render frame operation
         * @param content Content to render
         * @param properties Render properties
         * @param timeoutMs Timeout in milliseconds
         * @return Render result
         */
        virtual RenderResult RenderFrame(
            const ContentParameters& content,
            const RenderProperties& properties = RenderProperties{},
            uint32_t timeoutMs = 5000
        ) = 0;

        /**
         * @brief Check if render backend is ready
         * @return True if backend is initialized and ready
         */
        virtual bool IsReady() const = 0;

        /**
         * @brief Check if render process is alive
         * @return True if render process is running
         */
        virtual bool IsProcessAlive() const = 0;

        // === Utility Methods ===

        /**
         * @brief Capture current frame to file
         * @param filePath Output file path
         * @param format Image format (PNG, JPEG, BMP)
         * @return Future that completes when capture is done
         */
        virtual std::future<RenderResult> CaptureFrameAsync(
            const std::string& filePath,
            const std::string& format = "PNG"
        ) = 0;

        /**
         * @brief Get widget ID associated with this backend
         * @return Widget ID
         */
        virtual uint32_t GetWidgetId() const = 0;

        /**
         * @brief Get render surface information
         * @return Current render surface bounds and properties
         */
        virtual RenderRect GetRenderBounds() const = 0;
    };

    /**
     * @brief Factory function to create render backend proxy
     * @param widgetId Unique widget identifier
     * @param ipcBridge IPC bridge for communication
     * @return Unique pointer to render backend proxy
     */
    std::unique_ptr<IRenderBackendProxy> CreateRenderBackendProxy(
        uint32_t widgetId,
        std::shared_ptr<RenderIPCBridge> ipcBridge
    );

} // namespace RainmeterManager::Render
