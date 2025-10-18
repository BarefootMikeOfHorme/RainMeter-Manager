#pragma once
// widget_manager.h - Central manager for all widgets
// Copyright (c) 2025 RainmeterManager. All rights reserved.

#include "framework/widget_base.h"
#include "../core/system_monitor.h"
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace RainmeterManager {
namespace Widgets {

/**
 * @brief Central manager for all application widgets
 * 
 * Handles widget lifecycle, rendering coordination, and system monitor integration.
 * Thread-safe for multi-threaded rendering scenarios.
 */
class WidgetManager {
private:
    // Widget storage
    std::vector<std::unique_ptr<IWidget>> widgets_;
    std::map<std::wstring, IWidget*> widgetsByName_;
    
    // System monitoring
    std::shared_ptr<Core::ISystemMonitor> systemMonitor_;
    
    // Rendering
    ID2D1RenderTarget* renderTarget_;
    
    // Thread safety
    mutable std::mutex widgetMutex_;
    
    // State
    bool initialized_;
    DWORD lastUpdateTick_;
    DWORD updateIntervalMs_;

public:
    WidgetManager();
    ~WidgetManager();

    // Lifecycle
    bool Initialize(std::shared_ptr<Core::ISystemMonitor> monitor = nullptr);
    void Shutdown();
    void Update();

    // Rendering
    void SetRenderTarget(ID2D1RenderTarget* renderTarget);
    void RenderAll();
    void RenderWidget(const std::wstring& name);

    // Widget management
    bool AddWidget(std::unique_ptr<IWidget> widget);
    bool RemoveWidget(const std::wstring& name);
    IWidget* GetWidget(const std::wstring& name) const;
    std::vector<IWidget*> GetAllWidgets() const;
    size_t GetWidgetCount() const;
    void ClearAllWidgets();

    // Widget loading from config
    bool LoadWidgetsFromConfig(const std::wstring& configFile);
    bool SaveWidgetsToConfig(const std::wstring& configFile);

    // System monitor
    void SetSystemMonitor(std::shared_ptr<Core::ISystemMonitor> monitor);
    std::shared_ptr<Core::ISystemMonitor> GetSystemMonitor() const { return systemMonitor_; }

    // Input handling (dispatch to appropriate widget)
    void OnMouseMove(int x, int y);
    void OnMouseClick(int x, int y, bool leftButton);
    void OnResize(UINT width, UINT height);

    // Configuration
    void SetUpdateInterval(DWORD intervalMs) { updateIntervalMs_ = intervalMs; }
    DWORD GetUpdateInterval() const { return updateIntervalMs_; }

    // Widget visibility control
    void ShowWidget(const std::wstring& name);
    void HideWidget(const std::wstring& name);
    void ShowAllWidgets();
    void HideAllWidgets();

private:
    // Helper methods
    bool ShouldUpdate() const;
    IWidget* HitTest(int x, int y) const;
    void UpdateSystemMonitor();
};

} // namespace Widgets
} // namespace RainmeterManager
