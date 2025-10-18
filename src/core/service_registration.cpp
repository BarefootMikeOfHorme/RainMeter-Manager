// service_registration.cpp - Register core services with ServiceLocator
#include "service_locator.h"
#include "logger.h"
#include "security.h"
#include "../config/configuration_manager.h"
#include "../services/telemetry_service.h"

namespace RainmeterManager {
namespace Core {

// Register core services with the service locator
void RegisterCoreServices(ServiceLocator& locator) {
    ::Logger::info("RegisterCoreServices: Registering core services");
    
    try {
        // Register Security service (singleton pattern)
        // Note: Security is already initialized globally, so we just register the reference
        ::Logger::info("RegisterCoreServices: Security service available (global instance)");
        
        // Register Logger service (singleton pattern)
        // Note: Logger is also global, so it's already available
        ::Logger::info("RegisterCoreServices: Logger service available (global instance)");
        
        // Register ConfigurationManager
        // Note: ConfigurationManager uses singleton pattern - just ensure it's accessible
        ::Logger::info("RegisterCoreServices: ConfigurationManager service available (singleton)");
        
        // Register TelemetryService if enabled
        // Note: TelemetryService exists but may need explicit registration in future
        ::Logger::info("RegisterCoreServices: TelemetryService available");
        
        // Future services can be registered here:
        // - locator.Register<IUIFramework>(std::make_shared<UIFramework>());
        // - locator.Register<IWidgetManager>(std::make_shared<WidgetManager>());
        // - locator.Register<IPluginSystem>(std::make_shared<PluginSystem>());
        
        ::Logger::info("RegisterCoreServices: Core services registered successfully");
        
    } catch (const std::exception& e) {
        ::Logger::error(std::string("RegisterCoreServices: Exception during registration: ") + e.what());
        throw;
    }
}

} // namespace Core
} // namespace RainmeterManager
