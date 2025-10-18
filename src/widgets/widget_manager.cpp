// widget_manager.cpp - Implementation of WidgetManager
// Copyright (c) 2025 RainmeterManager. All rights reserved.

#include "widget_manager.h"
#include "../core/logger.h"
#include <algorithm>
#include <fstream>

namespace RainmeterManager {
namespace Widgets {

WidgetManager::WidgetManager()
    : renderTarget_(nullptr)
    , initialized_(false)
    , lastUpdateTick_(0)
    , updateIntervalMs_(16)  // Default ~60fps
{
    LOG_INFO("WidgetManager created");
}

WidgetManager::~WidgetManager() {
    if (initialized_) {
        Shutdown();
    }
    LOG_INFO("WidgetManager destroyed");
}

bool WidgetManager::Initialize(std::shared_ptr<Core::ISystemMonitor> monitor) {
    std::lock_guard<std::mutex> lock(widgetMutex_);

    if (initialized_) {
        LOG_WARNING("WidgetManager already initialized");
        return true;
    }

    LOG_INFO("Initializing WidgetManager");

    systemMonitor_ = monitor;

    // Initialize all existing widgets
    for (auto& widget : widgets_) {
        if (!widget->Initialize()) {
            LOG_ERROR("Failed to initialize widget: " + 
                     std::string(widget->GetName().begin(), widget->GetName().end()));
            continue;
        }

        // Attach system monitor if available
        if (systemMonitor_) {
            widget->SetSystemMonitor(systemMonitor_);
        }
    }

    initialized_ = true;
    lastUpdateTick_ = GetTickCount();

    LOG_INFO("WidgetManager initialized successfully with " + 
             std::to_string(widgets_.size()) + " widgets");
    return true;
}

void WidgetManager::Shutdown() {
    std::lock_guard<std::mutex> lock(widgetMutex_);

    if (!initialized_) {
        return;
    }

    LOG_INFO("Shutting down WidgetManager");

    // Shutdown all widgets in reverse order
    for (auto it = widgets_.rbegin(); it != widgets_.rend(); ++it) {
        (*it)->Shutdown();
    }

    widgets_.clear();
    widgetsByName_.clear();
    systemMonitor_.reset();
    renderTarget_ = nullptr;
    initialized_ = false;

    LOG_INFO("WidgetManager shut down successfully");
}

void WidgetManager::Update() {
    if (!initialized_ || !ShouldUpdate()) {
        return;
    }

    std::lock_guard<std::mutex> lock(widgetMutex_);

    // Update system monitor first
    UpdateSystemMonitor();

    // Update all widgets
    for (auto& widget : widgets_) {
        if (widget->IsVisible()) {
            widget->Update();
        }
    }

    lastUpdateTick_ = GetTickCount();
}

void WidgetManager::SetRenderTarget(ID2D1RenderTarget* renderTarget) {
    std::lock_guard<std::mutex> lock(widgetMutex_);
    renderTarget_ = renderTarget;
    LOG_DEBUG("Render target set for WidgetManager");
}

void WidgetManager::RenderAll() {
    if (!initialized_ || !renderTarget_) {
        return;
    }

    std::lock_guard<std::mutex> lock(widgetMutex_);

    // Render all visible widgets
    for (auto& widget : widgets_) {
        if (widget->IsVisible()) {
            widget->Render(renderTarget_);
        }
    }
}

void WidgetManager::RenderWidget(const std::wstring& name) {
    if (!initialized_ || !renderTarget_) {
        return;
    }

    std::lock_guard<std::mutex> lock(widgetMutex_);

    auto it = widgetsByName_.find(name);
    if (it != widgetsByName_.end() && it->second->IsVisible()) {
        it->second->Render(renderTarget_);
    }
}

bool WidgetManager::AddWidget(std::unique_ptr<IWidget> widget) {
    if (!widget) {
        LOG_ERROR("Attempted to add null widget");
        return false;
    }

    std::lock_guard<std::mutex> lock(widgetMutex_);

    std::wstring name = widget->GetName();

    // Check for duplicate names
    if (widgetsByName_.find(name) != widgetsByName_.end()) {
        LOG_ERROR("Widget with name already exists: " + 
                 std::string(name.begin(), name.end()));
        return false;
    }

    LOG_INFO("Adding widget: " + std::string(name.begin(), name.end()));

    // Initialize if manager is already initialized
    if (initialized_) {
        if (!widget->Initialize()) {
            LOG_ERROR("Failed to initialize new widget: " + 
                     std::string(name.begin(), name.end()));
            return false;
        }

        // Attach system monitor if available
        if (systemMonitor_) {
            widget->SetSystemMonitor(systemMonitor_);
        }
    }

    // Store pointers before moving
    IWidget* widgetPtr = widget.get();
    widgetsByName_[name] = widgetPtr;
    widgets_.push_back(std::move(widget));

    LOG_INFO("Widget added successfully: " + std::string(name.begin(), name.end()));
    return true;
}

bool WidgetManager::RemoveWidget(const std::wstring& name) {
    std::lock_guard<std::mutex> lock(widgetMutex_);

    auto it = widgetsByName_.find(name);
    if (it == widgetsByName_.end()) {
        LOG_WARNING("Widget not found: " + std::string(name.begin(), name.end()));
        return false;
    }

    LOG_INFO("Removing widget: " + std::string(name.begin(), name.end()));

    // Find and remove from vector
    auto vecIt = std::find_if(widgets_.begin(), widgets_.end(),
        [&name](const std::unique_ptr<IWidget>& w) {
            return w->GetName() == name;
        });

    if (vecIt != widgets_.end()) {
        (*vecIt)->Shutdown();
        widgets_.erase(vecIt);
    }

    widgetsByName_.erase(it);

    LOG_INFO("Widget removed successfully: " + std::string(name.begin(), name.end()));
    return true;
}

IWidget* WidgetManager::GetWidget(const std::wstring& name) const {
    std::lock_guard<std::mutex> lock(widgetMutex_);

    auto it = widgetsByName_.find(name);
    return (it != widgetsByName_.end()) ? it->second : nullptr;
}

std::vector<IWidget*> WidgetManager::GetAllWidgets() const {
    std::lock_guard<std::mutex> lock(widgetMutex_);

    std::vector<IWidget*> result;
    result.reserve(widgets_.size());

    for (const auto& widget : widgets_) {
        result.push_back(widget.get());
    }

    return result;
}

size_t WidgetManager::GetWidgetCount() const {
    std::lock_guard<std::mutex> lock(widgetMutex_);
    return widgets_.size();
}

void WidgetManager::ClearAllWidgets() {
    std::lock_guard<std::mutex> lock(widgetMutex_);

    LOG_INFO("Clearing all widgets (" + std::to_string(widgets_.size()) + ")");

    for (auto& widget : widgets_) {
        widget->Shutdown();
    }

    widgets_.clear();
    widgetsByName_.clear();

    LOG_INFO("All widgets cleared");
}

bool WidgetManager::LoadWidgetsFromConfig(const std::wstring& configFile) {
    LOG_INFO("Loading widgets from config: " + std::string(configFile.begin(), configFile.end()));

    // TODO: Implement JSON/INI config loading
    // For now, return false to indicate not implemented
    LOG_WARNING("LoadWidgetsFromConfig not yet implemented");
    return false;
}

bool WidgetManager::SaveWidgetsToConfig(const std::wstring& configFile) {
    LOG_INFO("Saving widgets to config: " + std::string(configFile.begin(), configFile.end()));

    // TODO: Implement JSON/INI config saving
    // For now, return false to indicate not implemented
    LOG_WARNING("SaveWidgetsToConfig not yet implemented");
    return false;
}

void WidgetManager::SetSystemMonitor(std::shared_ptr<Core::ISystemMonitor> monitor) {
    std::lock_guard<std::mutex> lock(widgetMutex_);

    systemMonitor_ = monitor;

    // Update all widgets with new monitor
    for (auto& widget : widgets_) {
        widget->SetSystemMonitor(monitor);
    }

    LOG_INFO("System monitor updated for all widgets");
}

void WidgetManager::OnMouseMove(int x, int y) {
    if (!initialized_) {
        return;
    }

    std::lock_guard<std::mutex> lock(widgetMutex_);

    // Find widget under cursor
    IWidget* hitWidget = HitTest(x, y);
    
    // Notify relevant widget
    if (hitWidget) {
        hitWidget->OnMouseMove(x, y);
    }
}

void WidgetManager::OnMouseClick(int x, int y, bool leftButton) {
    if (!initialized_) {
        return;
    }

    std::lock_guard<std::mutex> lock(widgetMutex_);

    // Find widget under cursor
    IWidget* hitWidget = HitTest(x, y);
    
    // Notify relevant widget
    if (hitWidget) {
        hitWidget->OnMouseClick(x, y, leftButton);
    }
}

void WidgetManager::OnResize(UINT width, UINT height) {
    if (!initialized_) {
        return;
    }

    std::lock_guard<std::mutex> lock(widgetMutex_);

    LOG_DEBUG("WidgetManager OnResize: " + std::to_string(width) + "x" + std::to_string(height));

    // Notify all widgets
    for (auto& widget : widgets_) {
        widget->OnResize(width, height);
    }
}

void WidgetManager::ShowWidget(const std::wstring& name) {
    IWidget* widget = GetWidget(name);
    if (widget) {
        widget->SetVisible(true);
        LOG_INFO("Widget shown: " + std::string(name.begin(), name.end()));
    }
}

void WidgetManager::HideWidget(const std::wstring& name) {
    IWidget* widget = GetWidget(name);
    if (widget) {
        widget->SetVisible(false);
        LOG_INFO("Widget hidden: " + std::string(name.begin(), name.end()));
    }
}

void WidgetManager::ShowAllWidgets() {
    std::lock_guard<std::mutex> lock(widgetMutex_);

    for (auto& widget : widgets_) {
        widget->SetVisible(true);
    }

    LOG_INFO("All widgets shown");
}

void WidgetManager::HideAllWidgets() {
    std::lock_guard<std::mutex> lock(widgetMutex_);

    for (auto& widget : widgets_) {
        widget->SetVisible(false);
    }

    LOG_INFO("All widgets hidden");
}

bool WidgetManager::ShouldUpdate() const {
    DWORD currentTick = GetTickCount();
    return (currentTick - lastUpdateTick_) >= updateIntervalMs_;
}

IWidget* WidgetManager::HitTest(int x, int y) const {
    // Simple hit test - check if point is inside widget bounds
    // Iterate in reverse order to check top widgets first
    for (auto it = widgets_.rbegin(); it != widgets_.rend(); ++it) {
        const auto& widget = *it;
        
        if (!widget->IsVisible()) {
            continue;
        }

        int wx, wy;
        UINT wwidth, wheight;
        widget->GetPosition(wx, wy);
        widget->GetSize(wwidth, wheight);

        if (x >= wx && x < (wx + static_cast<int>(wwidth)) &&
            y >= wy && y < (wy + static_cast<int>(wheight))) {
            return widget.get();
        }
    }

    return nullptr;
}

void WidgetManager::UpdateSystemMonitor() {
    if (systemMonitor_) {
        systemMonitor_->Update();
    }
}

} // namespace Widgets
} // namespace RainmeterManager
