#pragma once

#include "logger.h"
#include <string>
#include <vector>
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
    
    // Formatted logging (printf-style)
    template<typename... Args>
    void LogInfo(const wchar_t* format, Args&&... args) {
        LogInfo(FormatString(format, std::forward<Args>(args)...));
    }
    
    template<typename... Args>
    void LogWarning(const wchar_t* format, Args&&... args) {
        LogWarning(FormatString(format, std::forward<Args>(args)...));
    }
    
    template<typename... Args>
    void LogError(const wchar_t* format, Args&&... args) {
        LogError(FormatString(format, std::forward<Args>(args)...));
    }

    // Generic wide logging
    void LogWide(LogLevel level, const std::wstring& message);
    void LogWide(LogLevel level, const std::wstring& component, const std::wstring& message);

private:
    Logger() = default;

    std::string ToUtf8(const std::wstring& w) const;
    ::LogLevel ToLegacyLevel(LogLevel lvl) const;
    
    // Format string helper (secure version)
    template<typename... Args>
    std::wstring FormatString(const wchar_t* format, Args&&... args) const {
        int size = _snwprintf_s(nullptr, 0, _TRUNCATE, format, std::forward<Args>(args)...);
        if (size <= 0) return std::wstring(format);
        std::vector<wchar_t> buf(size + 1);
        _snwprintf_s(buf.data(), buf.size(), _TRUNCATE, format, std::forward<Args>(args)...);
        return std::wstring(buf.data());
    }

    std::mutex mtx_;
};

} // namespace Core
} // namespace RainmeterManager

