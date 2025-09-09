#include "logger_adapter.h"
#include <sstream>

namespace RainmeterManager {
namespace Core {

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

std::string Logger::ToUtf8(const std::wstring& w) const {
    if (w.empty()) return std::string();
    int sz = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    if (sz <= 0) return std::string();
    std::string out(static_cast<size_t>(sz), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), out.data(), sz, nullptr, nullptr);
    return out;
}

::LogLevel Logger::ToLegacyLevel(LogLevel lvl) const {
    switch (lvl) {
        case LogLevel::Trace:    return ::LogLevel::TRACE;
        case LogLevel::Debug:    return ::LogLevel::DEBUG;
        case LogLevel::Info:     return ::LogLevel::INFO;
        case LogLevel::Warning:  return ::LogLevel::WARNING;
        case LogLevel::Error:    return ::LogLevel::ERROR;
        case LogLevel::Critical: return ::LogLevel::CRITICAL;
        case LogLevel::Fatal:    return ::LogLevel::FATAL;
        default:                 return ::LogLevel::INFO;
    }
}

void Logger::LogInfo(const std::wstring& message) {
    std::lock_guard<std::mutex> lock(mtx_);
    ::Logger::info(ToUtf8(message));
}

void Logger::LogWarning(const std::wstring& message) {
    std::lock_guard<std::mutex> lock(mtx_);
    ::Logger::warning(ToUtf8(message));
}

void Logger::LogError(const std::wstring& message) {
    std::lock_guard<std::mutex> lock(mtx_);
    ::Logger::error(ToUtf8(message));
}

void Logger::LogWide(LogLevel level, const std::wstring& message) {
    std::lock_guard<std::mutex> lock(mtx_);
    ::Logger::log(ToLegacyLevel(level), ToUtf8(message));
}

void Logger::LogWide(LogLevel level, const std::wstring& component, const std::wstring& message) {
    std::wstringstream ws;
    ws << L"[" << component << L"] " << message;
    LogWide(level, ws.str());
}

} // namespace Core
} // namespace RainmeterManager

