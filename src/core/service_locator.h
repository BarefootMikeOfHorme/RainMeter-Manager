#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <typeindex>
#include <functional>
#include <mutex>
#include "logger.h"

namespace RainmeterManager {
namespace Core {

/**
 * @brief Service Locator Pattern Implementation
 * 
 * Enterprise-grade dependency injection container providing:
 * - Type-safe service registration and resolution
 * - Singleton and factory service creation patterns
 * - Thread-safe service management
 * - Circular dependency detection
 * - Service lifecycle management
 * 
 * Phase 2 Implementation - Core Application Layer
 */
class ServiceLocator {
public:
    // Service creation patterns
    enum class ServiceLifetime {
        Singleton,  // Created once and reused
        Transient   // Created new instance each time
    };

private:
    struct ServiceEntry {
        std::function<std::shared_ptr<void>()> factory;
        std::shared_ptr<void> instance;
        ServiceLifetime lifetime;
        std::string name;
        
        ServiceEntry(std::function<std::shared_ptr<void>()> f, ServiceLifetime lt, const std::string& n)
            : factory(std::move(f)), lifetime(lt), name(n) {}
    };

    mutable std::mutex servicesMutex_;
    std::unordered_map<std::type_index, ServiceEntry> services_;
    std::unordered_map<std::string, std::type_index> namedServices_;
    
    // Circular dependency detection
    mutable std::unordered_set<std::type_index> resolutionStack_;
    mutable std::mutex resolutionMutex_;

public:
    ServiceLocator() = default;
    ~ServiceLocator();
    
    // Prevent copy/move
    ServiceLocator(const ServiceLocator&) = delete;
    ServiceLocator& operator=(const ServiceLocator&) = delete;
    ServiceLocator(ServiceLocator&&) = delete;
    ServiceLocator& operator=(ServiceLocator&&) = delete;

    /**
     * @brief Register a service with automatic type deduction
     * @tparam TInterface Interface type
     * @tparam TImplementation Implementation type
     * @param lifetime Service lifetime pattern
     * @param name Optional service name for named resolution
     */
    template<typename TInterface, typename TImplementation>
    void RegisterService(ServiceLifetime lifetime = ServiceLifetime::Singleton, 
                        const std::string& name = "") {
        static_assert(std::is_base_of_v<TInterface, TImplementation>, 
                     "TImplementation must inherit from TInterface");
        
        auto factory = []() -> std::shared_ptr<void> {
            return std::static_pointer_cast<void>(std::make_shared<TImplementation>());
        };
        
        RegisterServiceInternal<TInterface>(factory, lifetime, name);
    }

    /**
     * @brief Register a service with custom factory function
     * @tparam TInterface Interface type
     * @param factory Factory function to create service instances
     * @param lifetime Service lifetime pattern
     * @param name Optional service name for named resolution
     */
    template<typename TInterface>
    void RegisterFactory(std::function<std::shared_ptr<TInterface>()> factory,
                        ServiceLifetime lifetime = ServiceLifetime::Singleton,
                        const std::string& name = "") {
        auto wrappedFactory = [factory]() -> std::shared_ptr<void> {
            return std::static_pointer_cast<void>(factory());
        };
        
        RegisterServiceInternal<TInterface>(wrappedFactory, lifetime, name);
    }

    /**
     * @brief Register a service instance directly (always singleton)
     * @tparam TInterface Interface type
     * @param instance Pre-created service instance
     * @param name Optional service name for named resolution
     */
    template<typename TInterface>
    void RegisterInstance(std::shared_ptr<TInterface> instance, const std::string& name = "") {
        std::lock_guard<std::mutex> lock(servicesMutex_);
        
        auto typeIndex = std::type_index(typeid(TInterface));
        auto voidInstance = std::static_pointer_cast<void>(instance);
        
        auto factory = [voidInstance]() -> std::shared_ptr<void> {
            return voidInstance;
        };
        
        services_.emplace(typeIndex, ServiceEntry(factory, ServiceLifetime::Singleton, name));
        
        if (!name.empty()) {
            namedServices_[name] = typeIndex;
        }
        
        LogServiceRegistration<TInterface>("Instance", name);
    }

    /**
     * @brief Resolve a service by type
     * @tparam TInterface Interface type to resolve
     * @return Shared pointer to service instance
     * @throws std::runtime_error if service not registered or circular dependency detected
     */
    template<typename TInterface>
    std::shared_ptr<TInterface> Resolve() {
        auto typeIndex = std::type_index(typeid(TInterface));
        return ResolveInternal<TInterface>(typeIndex);
    }

    /**
     * @brief Resolve a service by name
     * @tparam TInterface Interface type to resolve
     * @param name Service name
     * @return Shared pointer to service instance
     * @throws std::runtime_error if named service not found
     */
    template<typename TInterface>
    std::shared_ptr<TInterface> Resolve(const std::string& name) {
        std::lock_guard<std::mutex> lock(servicesMutex_);
        
        auto it = namedServices_.find(name);
        if (it == namedServices_.end()) {
            throw std::runtime_error("Named service not found: " + name);
        }
        
        return ResolveInternal<TInterface>(it->second);
    }

    /**
     * @brief Check if a service type is registered
     * @tparam TInterface Interface type to check
     * @return true if registered, false otherwise
     */
    template<typename TInterface>
    bool IsRegistered() const {
        std::lock_guard<std::mutex> lock(servicesMutex_);
        auto typeIndex = std::type_index(typeid(TInterface));
        return services_.find(typeIndex) != services_.end();
    }

    /**
     * @brief Check if a named service is registered
     * @param name Service name to check
     * @return true if registered, false otherwise
     */
    bool IsRegistered(const std::string& name) const {
        std::lock_guard<std::mutex> lock(servicesMutex_);
        return namedServices_.find(name) != namedServices_.end();
    }

    /**
     * @brief Unregister a service by type
     * @tparam TInterface Interface type to unregister
     */
    template<typename TInterface>
    void Unregister() {
        std::lock_guard<std::mutex> lock(servicesMutex_);
        auto typeIndex = std::type_index(typeid(TInterface));
        
        auto it = services_.find(typeIndex);
        if (it != services_.end()) {
            // Remove from named services if it has a name
            if (!it->second.name.empty()) {
                namedServices_.erase(it->second.name);
            }
            services_.erase(it);
            
            LogServiceUnregistration<TInterface>();
        }
    }

    /**
     * @brief Unregister a named service
     * @param name Service name to unregister
     */
    void Unregister(const std::string& name) {
        std::lock_guard<std::mutex> lock(servicesMutex_);
        
        auto namedIt = namedServices_.find(name);
        if (namedIt != namedServices_.end()) {
            auto typeIndex = namedIt->second;
            services_.erase(typeIndex);
            namedServices_.erase(namedIt);
        }
    }

    /**
     * @brief Clear all registered services
     */
    void Clear() {
        std::lock_guard<std::mutex> lock(servicesMutex_);
        services_.clear();
        namedServices_.clear();
        Logger::GetInstance().LogInfo(L"ServiceLocator: All services cleared");
    }

    /**
     * @brief Get count of registered services
     * @return Number of registered services
     */
    size_t GetServiceCount() const {
        std::lock_guard<std::mutex> lock(servicesMutex_);
        return services_.size();
    }

private:
    template<typename TInterface>
    void RegisterServiceInternal(std::function<std::shared_ptr<void>()> factory,
                               ServiceLifetime lifetime,
                               const std::string& name) {
        std::lock_guard<std::mutex> lock(servicesMutex_);
        
        auto typeIndex = std::type_index(typeid(TInterface));
        services_.emplace(typeIndex, ServiceEntry(factory, lifetime, name));
        
        if (!name.empty()) {
            namedServices_[name] = typeIndex;
        }
        
        LogServiceRegistration<TInterface>(lifetime == ServiceLifetime::Singleton ? "Singleton" : "Transient", name);
    }

    template<typename TInterface>
    std::shared_ptr<TInterface> ResolveInternal(const std::type_index& typeIndex) {
        // Check for circular dependency
        {
            std::lock_guard<std::mutex> resolutionLock(resolutionMutex_);
            if (resolutionStack_.find(typeIndex) != resolutionStack_.end()) {
                throw std::runtime_error("Circular dependency detected for service");
            }
            resolutionStack_.insert(typeIndex);
        }

        std::shared_ptr<TInterface> result;
        
        try {
            std::lock_guard<std::mutex> lock(servicesMutex_);
            
            auto it = services_.find(typeIndex);
            if (it == services_.end()) {
                throw std::runtime_error("Service not registered");
            }
            
            auto& entry = it->second;
            
            if (entry.lifetime == ServiceLifetime::Singleton) {
                if (!entry.instance) {
                    entry.instance = entry.factory();
                }
                result = std::static_pointer_cast<TInterface>(entry.instance);
            } else {
                // Transient - always create new instance
                result = std::static_pointer_cast<TInterface>(entry.factory());
            }
        } catch (...) {
            // Remove from resolution stack on any error
            std::lock_guard<std::mutex> resolutionLock(resolutionMutex_);
            resolutionStack_.erase(typeIndex);
            throw;
        }
        
        // Remove from resolution stack on success
        {
            std::lock_guard<std::mutex> resolutionLock(resolutionMutex_);
            resolutionStack_.erase(typeIndex);
        }
        
        return result;
    }

    template<typename TInterface>
    void LogServiceRegistration(const std::string& type, const std::string& name) {
        std::wstring message = L"ServiceLocator: Registered " + 
                              std::wstring(type.begin(), type.end()) + L" service: " +
                              std::wstring(typeid(TInterface).name(), typeid(TInterface).name() + strlen(typeid(TInterface).name()));
        
        if (!name.empty()) {
            message += L" (named: " + std::wstring(name.begin(), name.end()) + L")";
        }
        
        Logger::GetInstance().LogInfo(message);
    }

    template<typename TInterface>
    void LogServiceUnregistration() {
        std::wstring message = L"ServiceLocator: Unregistered service: " +
                              std::wstring(typeid(TInterface).name(), typeid(TInterface).name() + strlen(typeid(TInterface).name()));
        
        Logger::GetInstance().LogInfo(message);
    }
};

} // namespace Core
} // namespace RainmeterManager
