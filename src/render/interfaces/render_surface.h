#pragma once

#include "render_command.h"
#include <functional>

namespace RainmeterManager::Render {

    // Surface event callback types
    using SurfaceResizeCallback = std::function<void(const RenderRect& newBounds)>;
    using SurfaceDPIChangeCallback = std::function<void(float newDpiX, float newDpiY)>;
    using SurfaceVisibilityCallback = std::function<void(bool visible)>;

    /**
     * @brief Interface for managing render surfaces
     * 
     * This interface provides methods for managing rendering surfaces,
     * handling window events, DPI changes, and surface properties.
     */
    class IRenderSurface {
    public:
        virtual ~IRenderSurface() = default;

        // === Surface Creation & Management ===

        /**
         * @brief Create a render surface from window handle
         * @param windowHandle Target window handle
         * @param bounds Initial surface bounds
         * @param properties Surface properties
         * @return True if surface creation succeeded
         */
        virtual bool CreateSurface(
            HWND windowHandle,
            const RenderRect& bounds,
            const RenderProperties& properties = RenderProperties{}
        ) = 0;

        /**
         * @brief Destroy the render surface
         */
        virtual void DestroySurface() = 0;

        /**
         * @brief Check if surface is valid and ready
         * @return True if surface is ready for rendering
         */
        virtual bool IsValid() const = 0;

        // === Surface Properties ===

        /**
         * @brief Get surface bounds
         * @return Current surface bounds
         */
        virtual RenderRect GetBounds() const = 0;

        /**
         * @brief Set surface bounds
         * @param bounds New surface bounds
         * @return True if resize succeeded
         */
        virtual bool SetBounds(const RenderRect& bounds) = 0;

        /**
         * @brief Get window handle
         * @return Associated window handle
         */
        virtual HWND GetWindowHandle() const = 0;

        /**
         * @brief Get surface DPI scaling
         * @param dpiX Output DPI X scaling factor
         * @param dpiY Output DPI Y scaling factor
         */
        virtual void GetDPI(float& dpiX, float& dpiY) const = 0;

        /**
         * @brief Set surface DPI scaling
         * @param dpiX New DPI X scaling factor
         * @param dpiY New DPI Y scaling factor
         */
        virtual void SetDPI(float dpiX, float dpiY) = 0;

        // === Surface State ===

        /**
         * @brief Set surface visibility
         * @param visible True to show surface, false to hide
         */
        virtual void SetVisible(bool visible) = 0;

        /**
         * @brief Check if surface is visible
         * @return True if surface is visible
         */
        virtual bool IsVisible() const = 0;

        /**
         * @brief Set surface opacity
         * @param opacity Opacity value (0.0 - 1.0)
         */
        virtual void SetOpacity(float opacity) = 0;

        /**
         * @brief Get surface opacity
         * @return Current opacity value (0.0 - 1.0)
         */
        virtual float GetOpacity() const = 0;

        /**
         * @brief Set surface as topmost
         * @param topmost True to keep surface on top
         */
        virtual void SetTopmost(bool topmost) = 0;

        /**
         * @brief Check if surface is topmost
         * @return True if surface is topmost
         */
        virtual bool IsTopmost() const = 0;

        /**
         * @brief Set click-through behavior
         * @param clickThrough True to allow clicks to pass through
         */
        virtual void SetClickThrough(bool clickThrough) = 0;

        /**
         * @brief Check click-through state
         * @return True if click-through is enabled
         */
        virtual bool IsClickThrough() const = 0;

        // === Surface Events ===

        /**
         * @brief Set resize callback
         * @param callback Callback function for resize events
         */
        virtual void SetResizeCallback(SurfaceResizeCallback callback) = 0;

        /**
         * @brief Set DPI change callback
         * @param callback Callback function for DPI change events
         */
        virtual void SetDPIChangeCallback(SurfaceDPIChangeCallback callback) = 0;

        /**
         * @brief Set visibility change callback
         * @param callback Callback function for visibility events
         */
        virtual void SetVisibilityCallback(SurfaceVisibilityCallback callback) = 0;

        // === Surface Utilities ===

        /**
         * @brief Convert screen coordinates to surface coordinates
         * @param screenX Screen X coordinate
         * @param screenY Screen Y coordinate
         * @param surfaceX Output surface X coordinate
         * @param surfaceY Output surface Y coordinate
         */
        virtual void ScreenToSurface(int screenX, int screenY, int& surfaceX, int& surfaceY) const = 0;

        /**
         * @brief Convert surface coordinates to screen coordinates
         * @param surfaceX Surface X coordinate
         * @param surfaceY Surface Y coordinate
         * @param screenX Output screen X coordinate
         * @param screenY Output screen Y coordinate
         */
        virtual void SurfaceToScreen(int surfaceX, int surfaceY, int& screenX, int& screenY) const = 0;

        /**
         * @brief Check if point is inside surface
         * @param x X coordinate
         * @param y Y coordinate
         * @return True if point is inside surface bounds
         */
        virtual bool ContainsPoint(int x, int y) const = 0;

        /**
         * @brief Invalidate surface region for redraw
         * @param region Region to invalidate (nullptr for entire surface)
         */
        virtual void Invalidate(const RenderRect* region = nullptr) = 0;

        /**
         * @brief Force immediate surface update
         */
        virtual void UpdateSurface() = 0;

        // === Monitor Integration ===

        /**
         * @brief Get monitor containing this surface
         * @return Monitor information
         */
        virtual MonitorInfo GetMonitorInfo() const = 0;

        /**
         * @brief Move surface to specified monitor
         * @param monitorId Target monitor ID
         * @return True if move succeeded
         */
        virtual bool MoveToMonitor(int monitorId) = 0;

        // === Advanced Features ===

        /**
         * @brief Enable/disable hardware acceleration
         * @param enabled True to enable hardware acceleration
         */
        virtual void SetHardwareAcceleration(bool enabled) = 0;

        /**
         * @brief Check hardware acceleration status
         * @return True if hardware acceleration is enabled
         */
        virtual bool IsHardwareAccelerated() const = 0;

        /**
         * @brief Set VSync for this surface
         * @param enabled True to enable VSync
         */
        virtual void SetVSync(bool enabled) = 0;

        /**
         * @brief Check VSync status
         * @return True if VSync is enabled
         */
        virtual bool IsVSyncEnabled() const = 0;
    };

    /**
     * @brief Factory function to create render surface
     * @return Unique pointer to render surface implementation
     */
    std::unique_ptr<IRenderSurface> CreateRenderSurface();

} // namespace RainmeterManager::Render
