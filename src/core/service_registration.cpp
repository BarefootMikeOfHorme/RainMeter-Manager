// service_registration.cpp - Register core services with ServiceLocator (stub)
#include "service_locator.h"
#include "../services/telemetry_service.h"
#include "../ui/ui_framework.h"
#include "../core/logger.h"
// Optional: config manager
#include "../config/configuration_manager.h" // Assumed to exist

namespace RainmeterManager {
namespace Core {

void RegisterCoreServices() {
    auto& locator = ServiceLocator::Instance();

    // Telemetry
    locator.Register(std::make_shared<Services::TelemetryService>());
    LOG_INFO("Registered TelemetryService");

    // UI Framework
    locator.Register(std::make_shared<UI::UIFramework>());
    LOG_INFO("Registered UIFramework");

    // Configuration Manager
    locator.Register(std::make_shared<Config::ConfigurationManager>());
    LOG_INFO("Registered ConfigurationManager");
}

} // namespace Core
} // namespace RainmeterManager
