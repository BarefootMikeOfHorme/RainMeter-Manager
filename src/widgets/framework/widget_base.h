#pragma once
// widget_base.h - Base class for all RainmeterManager widgets
// Copyright (c) 2025 RainmeterManager. All rights reserved.

#include <string>
#include <memory>
#include <windows.h>
#include <d2d1.h>

namespace RainmeterManager {
namespace Widgets {

// Forward declarations
class WidgetRenderer;
class WidgetConfig;
class SystemMonitor;

/**
 * @brief Base interface for all widgets
 * 
 * All widgets (CPU hexagon, 3D orbs, network globe, etc.) inherit from this.
 * Provides common lifecycle methods and rendering interface.
 */
class IWidget {
public:
    virtual ~IWidget() = default;

    // Lifecycle
    virtual bool Initialize() = 0;
    virtual void Update() = 0;
    virtual void Shutdown() = 0;

    // Rendering
    virtual void Render(ID2D1RenderTarget* renderTarget) = 0;
    virtual void OnResize(UINT width, UINT height) = 0;

    // Input handling
    virtual void OnMouseMove(int x, int y) = 0;
    virtual void OnMouseClick(int x, int y, bool leftButton) = 0;
    virtual void OnMouseLeave() = 0;

    // Properties
    virtual std::wstring GetName() const = 0;
    virtual std::wstring GetDescription() const = 0;
    virtual bool IsVisible() const = 0;
    virtual void SetVisible(bool visible) = 0;
};

/**
 * @brief Base implementation of IWidget with common functionality
 */
class WidgetBase : public IWidget {
protected:
    std::wstring name_;
    std::wstring description_;
    bool visible_;
    bool initialized_;
    
    // Position and size
    int x_;
    int y_;
    UINT width_;
    UINT height_;

    // System monitoring data source
    std::shared_ptr<SystemMonitor> monitor_;

public:
    WidgetBase(const std::wstring& name, const std::wstring& description);
    virtual ~WidgetBase();

    // IWidget implementation
    bool Initialize() override;
    void Update() override;
    void Shutdown() override;
    void Render(ID2D1RenderTarget* renderTarget) override;
    void OnResize(UINT width, UINT height) override;
    void OnMouseMove(int x, int y) override;
    void OnMouseClick(int x, int y, bool leftButton) override;
    void OnMouseLeave() override;

    // Properties
    std::wstring GetName() const override { return name_; }
    std::wstring GetDescription() const override { return description_; }
    bool IsVisible() const override { return visible_; }
    void SetVisible(bool visible) override { visible_ = visible; }

    // Position
    void SetPosition(int x, int y);
    void GetPosition(int& x, int& y) const;
    void SetSize(UINT width, UINT height);
    void GetSize(UINT& width, UINT& height) const;

    // System monitor access
    void SetSystemMonitor(std::shared_ptr<SystemMonitor> monitor);
    std::shared_ptr<SystemMonitor> GetSystemMonitor() const { return monitor_; }

protected:
    // Virtual methods for derived widgets to override
    virtual bool OnInitialize() { return true; }
    virtual void OnUpdate() {}
    virtual void OnShutdown() {}
    virtual void OnRender(ID2D1RenderTarget* renderTarget) = 0;
};

/**
 * @brief Widget configuration loaded from JSON or similar
 */
struct WidgetConfig {
    std::wstring name;
    std::wstring type;  // "CPUHexagon", "3DOrbs", "NetworkGlobe", etc.
    int x;
    int y;
    UINT width;
    UINT height;
    bool visible;
    std::wstring configFile;  // Path to widget-specific config
    
    // Update rate
    UINT updateIntervalMs;  // e.g., 16 for 60fps, 100 for 10fps, etc.

    // TODO: Add more configuration options as needed
};

/**
 * @brief Factory for creating widgets based on type
 */
class WidgetFactory {
public:
    static std::unique_ptr<IWidget> CreateWidget(const WidgetConfig& config);
    static std::vector<std::wstring> GetAvailableWidgetTypes();
};

} // namespace Widgets
} // namespace RainmeterManager
