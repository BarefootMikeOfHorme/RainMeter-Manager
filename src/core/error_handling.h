#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include "logger.h"
#include <functional>
#include <string>
#include <map>
#include <mutex>
#include <chrono>

// Enterprise-level error handling macros
#define TRY_CATCH_LOG(operation, context) \
    try { \
        operation; \
    } catch (const std::exception& e) { \
        Logger::logException(e, context); \
        throw; \
    }

#define SAFE_CALL(func, errorMsg) \
    do { \
        auto result = func; \
        if (!result.success) { \
            LOG_ERROR(errorMsg + ": " + result.errorMessage); \
            return result; \
        } \
    } while(0)

#define SAFE_CALL_VOID(func, errorMsg) \
    do { \
        auto result = func; \
        if (!result.success) { \
            LOG_ERROR(errorMsg + ": " + result.errorMessage); \
            return; \
        } \
    } while(0)

#define VALIDATE_PARAM(param, paramName) \
    if (!(param)) { \
        LOG_ERROR("Invalid parameter: " + std::string(paramName)); \
        return ErrorResult{false, "Invalid parameter: " + std::string(paramName), -1, "", "", std::chrono::system_clock::now(), Logger::getThreadId()}; \
    }

#define VALIDATE_PARAM_VOID(param, paramName) \
    if (!(param)) { \
        LOG_ERROR("Invalid parameter: " + std::string(paramName)); \
        return; \
    }

// File operation safety wrappers
#define SAFE_FILE_OPEN(file, path, mode) \
    file.open(path, mode); \
    if (!file.is_open()) { \
        auto error = HANDLE_WINDOWS_ERROR("Failed to open file: " + std::string(path)); \
        return error; \
    }

#define SAFE_MEMORY_ALLOC(ptr, size, type) \
    ptr = static_cast<type*>(malloc(size)); \
    if (!ptr) { \
        LOG_CRITICAL("Memory allocation failed for size: " + std::to_string(size)); \
        return ErrorResult{false, "Memory allocation failed", -1, "", "", std::chrono::system_clock::now(), ""}; \
    }

// Security operation wrappers
#define SECURITY_CHECK(operation, resource) \
    do { \
        Logger::logSecurityEvent("Security check", "Operation: " + std::string(#operation) + ", Resource: " + resource); \
        if (!(operation)) { \
            Logger::logSecurityEvent("Security check FAILED", "Operation: " + std::string(#operation) + ", Resource: " + resource); \
            return ErrorResult{false, "Security check failed", -1, "", "", std::chrono::system_clock::now(), ""}; \
        } \
    } while(0)

// Performance monitoring wrappers
#define PERF_SCOPED_TIMER(name) \
    auto _scoped_timer = Logger::startPerformanceTimer(name); \
    struct ScopedTimerCleanup { \
        PerformanceMetrics& timer; \
        ScopedTimerCleanup(PerformanceMetrics& t) : timer(t) {} \
        ~ScopedTimerCleanup() { Logger::endPerformanceTimer(timer); } \
    } _timer_cleanup(_scoped_timer);

// Retry operation wrapper
#define RETRY_OPERATION(operation, maxRetries, delayMs) \
    Logger::retryOperation([&]() -> ErrorResult { \
        try { \
            if (operation) { \
                return ErrorResult{true, "", 0, "", "", std::chrono::system_clock::now(), ""}; \
            } else { \
                return ErrorResult{false, "Operation failed", -1, "", "", std::chrono::system_clock::now(), ""}; \
            } \
        } catch (const std::exception& e) { \
            return ErrorResult{false, e.what(), -1, "", "", std::chrono::system_clock::now(), ""}; \
        } \
    }, maxRetries, delayMs)

// Enterprise error handling utility class
class ErrorHandler {
public:
    // Global error handler setup
    static void setupGlobalErrorHandling();
    
    // Structured exception handling for Windows
    static LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo);
    
    // Critical error reporting
    static void reportCriticalError(const std::string& error, const std::string& context = "");
    
    // Application recovery
    static bool attemptRecovery(const std::string& operation);
    
    // Error metrics collection
    static void incrementErrorCount(const std::string& errorType);
    static void reportErrorMetrics();
    
private:
    static std::map<std::string, int> errorCounts;
    static std::mutex errorMetricsMutex;
};

#endif // ERROR_HANDLING_H
