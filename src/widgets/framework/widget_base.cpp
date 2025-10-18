// widget_base.cpp - Implementation of widget base classes
// Copyright (c) 2025 RainmeterManager. All rights reserved.

#include "widget_base.h"
#include "../../core/logger.h"
#include <algorithm>

namespace RainmeterManager {
namespace Widgets {

// WidgetBase Implementation
WidgetBase::WidgetBase(const std::wstring& name, const std::wstring& description)
    : name_(name)
    , description_(description)
    , visible_(true)
    , initialized_(false)
    , x_(0)
    , y_(0)
    , width_(200)
    , height_(200)
    , monitor_(nullptr)
{
    LOG_DEBUG("WidgetBase created: " + std::string(name.begin(), name.end()));
}

WidgetBase::~WidgetBase() {
    if (initialized_) {
        Shutdown();
    }
    LOG_DEBUG("WidgetBase destroyed: " + std::string(name_.begin(), name_.end()));
}

bool WidgetBase::Initialize() {
    if (initialized_) {
        LOG_WARNING("Widget already initialized: " + std::string(name_.begin(), name_.end()));
        return true;
    }

    LOG_INFO("Initializing widget: " + std::string(name_.begin(), name_.end()));

    try {
        if (!OnInitialize()) {
            LOG_ERROR("Widget OnInitialize failed: " + std::string(name_.begin(), name_.end()));
            return false;
        }

        initialized_ = true;
        LOG_INFO("Widget initialized successfully: " + std::string(name_.begin(), name_.end()));
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Exception initializing widget: " + std::string(e.what()));
        return false;
    }
}

void WidgetBase::Update() {
    if (!initialized_) {
        return;
    }

    try {
        OnUpdate();
    }
    catch (const std::exception& e) {
        LOG_ERROR("Exception updating widget: " + std::string(e.what()));
    }
}

void WidgetBase::Shutdown() {
    if (!initialized_) {
        return;
    }

    LOG_INFO("Shutting down widget: " + std::string(name_.begin(), name_.end()));

    try {
        OnShutdown();
        initialized_ = false;
        LOG_INFO("Widget shut down successfully: " + std::string(name_.begin(), name_.end()));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Exception shutting down widget: " + std::string(e.what()));
    }
}

void WidgetBase::Render(ID2D1RenderTarget* renderTarget) {
    if (!initialized_ || !visible_ || !renderTarget) {
        return;
    }

    try {
        OnRender(renderTarget);
    }
    catch (const std::exception& e) {
        LOG_ERROR("Exception rendering widget: " + std::string(e.what()));
    }
}

void WidgetBase::OnResize(UINT width, UINT height) {
    width_ = width;
    height_ = height;
    LOG_DEBUG("Widget resized to: " + std::to_string(width) + "x" + std::to_string(height));
}

void WidgetBase::OnMouseMove(int x, int y) {
    // Base implementation does nothing - derived classes can override
}

void WidgetBase::OnMouseClick(int x, int y, bool leftButton) {
    // Base implementation does nothing - derived classes can override
}

void WidgetBase::OnMouseLeave() {
    // Base implementation does nothing - derived classes can override
}

void WidgetBase::SetPosition(int x, int y) {
    x_ = x;
    y_ = y;
}

void WidgetBase::GetPosition(int& x, int& y) const {
    x = x_;
    y = y_;
}

void WidgetBase::SetSize(UINT width, UINT height) {
    width_ = width;
    height_ = height;
}

void WidgetBase::GetSize(UINT& width, UINT& height) const {
    width = width_;
    height = height_;
}

void WidgetBase::SetSystemMonitor(std::shared_ptr<SystemMonitor> monitor) {
    monitor_ = monitor;
    LOG_DEBUG("System monitor attached to widget: " + std::string(name_.begin(), name_.end()));
}

// WidgetFactory Implementation
std::unique_ptr<IWidget> WidgetFactory::CreateWidget(const WidgetConfig& config) {
    LOG_INFO("Creating widget of type: " + std::string(config.type.begin(), config.type.end()));

    // TODO: Implement widget creation based on type
    // For now, return nullptr - this will be implemented as widgets are added
    
    // Example:
    // if (config.type == L"CPUHexagon") {
    //     return std::make_unique<CPUHexagonWidget>(config);
    // }
    // else if (config.type == L"3DOrbs") {
    //     return std::make_unique<ThreeDOrbsWidget>(config);
    // }
    // else if (config.type == L"NetworkGlobe") {
    //     return std::make_unique<NetworkGlobeWidget>(config);
    // }

    LOG_WARNING("Unknown widget type: " + std::string(config.type.begin(), config.type.end()));
    return nullptr;
}

std::vector<std::wstring> WidgetFactory::GetAvailableWidgetTypes() {
    // Return list of available widget types
    std::vector<std::wstring> types;
    
    // TODO: Add widget types as they are implemented
    // types.push_back(L"CPUHexagon");
    // types.push_back(L"3DOrbs");
    // types.push_back(L"NetworkGlobe");
    // types.push_back(L"DataDensity");

    return types;
}

} // namespace Widgets
} // namespace RainmeterManager
