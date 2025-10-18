// service_registration.cpp - Register core services with ServiceLocator (stub)
// TODO: Implement when service interfaces are finalized
#include "service_locator.h"
// Use global logger (outside RainmeterManager namespace)
#include "logger.h"

namespace RainmeterManager {
namespace Core {

// Stub implementation - services will be registered when interfaces are ready
void RegisterCoreServices(ServiceLocator& /* locator */) {
    ::Logger::info("RegisterCoreServices: Stub implementation (services not yet available)");
    
    // TODO: Register services when they're implemented:
    // - TelemetryService
    // - UIFramework  
    // - ConfigurationManager
    // - WidgetManager
}

} // namespace Core
} // namespace RainmeterManager
