#pragma once

#include "logger.h"
#include <string>
#include <mutex>
#include <windows.h>

namespace RainmeterManager {
namespace Core {

// Adapter log levels expected by Phase 2 code
enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical,
    Fatal
};

// Adapter class that maps RainmeterManager::Core::Logger calls to the existing global ::Logger
class Logger {
public:
    static Logger& GetInstance();

    // Wide-string helpers
    void LogInfo(const std::wstring& message);
    void LogWarning(const std::wstring& message);
    void LogError(const std::wstring& message);

    // Generic wide logging
    void LogWide(LogLevel level, const std::wstring& message);
    void LogWide(LogLevel level, const std::wstring& component, const std::wstring& message);

private:
    Logger() = default;

    std::string ToUtf8(const std::wstring& w) const;
    ::LogLevel ToLegacyLevel(LogLevel lvl) const;

    std::mutex mtx_;
};

} // namespace Core
} // namespace RainmeterManager

