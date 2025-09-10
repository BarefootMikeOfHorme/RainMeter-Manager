#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include <functional>
#include <chrono>
#include <vector>
#include <queue>
#include <thread>
#include <condition_variable>
#include <windows.h>
#include <dbghelp.h>  // For stack traces

// Avoid name collisions with Windows macros
#ifdef ERROR
#undef ERROR
#endif
#ifdef WARNING
#undef WARNING
#endif
#ifdef CRITICAL
#undef CRITICAL
#endif
#ifdef FATAL
#undef FATAL
#endif
#ifdef DEBUG
#undef DEBUG
#endif
#ifdef TRACE
#undef TRACE
#endif

// Enterprise-level log levels
enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL,
    FATAL
};

// Log rotation settings
struct LogRotationConfig {
    size_t maxFileSize = 10 * 1024 * 1024;  // 10MB
    int maxFiles = 5;
    bool enableRotation = true;
};

// Performance metrics structure
struct PerformanceMetrics {
    std::chrono::high_resolution_clock::time_point startTime;
    std::string operationName;
    std::string contextInfo;
};

// Enhanced error handling result
struct ErrorResult {
    bool success;
    std::string errorMessage;
    int errorCode;
    std::string stackTrace;
    std::string contextInfo;
    std::chrono::system_clock::time_point timestamp;
    std::string threadId;
};

// Log entry structure for async logging
struct LogEntry {
    LogLevel level;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    std::string threadId;
    std::string file;
    int line;
    std::string function;
};

class Logger {
public:
    // Initialization and configuration
    static bool initialize(const std::string& logFilePath, LogRotationConfig config = {});
    static void setLogLevel(LogLevel minLevel);
    static void enableAsyncLogging(bool enable = true);
    static void enableConsoleOutput(bool enable = true);
    
    // Core logging methods
    static void log(LogLevel level, const std::string& message, const char* file = "", int line = 0, const char* function = nullptr);
    static void trace(const std::string& message);
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);
    static void critical(const std::string& message);
    static void fatal(const std::string& message);
    
    // Enhanced error handling
    static ErrorResult handleError(const std::string& operation, int errorCode, const std::string& details = "");
    static ErrorResult handleWindowsError(const std::string& operation, DWORD errorCode = GetLastError());
    static void logException(const std::exception& e, const std::string& context = "");
    static void logSystemError(const std::string& operation);
    
    // Performance monitoring
    static PerformanceMetrics startPerformanceTimer(const std::string& operationName, const std::string& context = "");
    static void endPerformanceTimer(const PerformanceMetrics& metrics);
    
    // File operations with comprehensive error handling
    static ErrorResult safeFileOperation(std::function<bool()> operation, const std::string& operationName);
    static ErrorResult retryOperation(std::function<ErrorResult()> operation, int maxRetries = 3, int delayMs = 1000);
    
    // Security integration
    static void logSecurityEvent(const std::string& event, const std::string& details = "");
    static void logAccessAttempt(const std::string& resource, bool success, const std::string& user = "");
    
    // Diagnostic utilities
    static std::string captureStackTrace(int skipFrames = 1);
    static void dumpSystemInfo();
    static void flushLogs();

    // Expose thread id helper for callers that need it in error macros
    static std::string getThreadId();
    
    // Cleanup and shutdown
    static void shutdown();
    
private:
    // Core logging infrastructure
    static std::unique_ptr<std::ofstream> logFile;
    static std::mutex logMutex;
    static LogLevel minLogLevel;
    static LogRotationConfig rotationConfig;
    static bool asyncLoggingEnabled;
    static bool consoleOutputEnabled;
    
    // Async logging infrastructure
    static std::queue<LogEntry> logQueue;
    static std::thread loggingThread;
    static std::condition_variable logCondition;
    static std::mutex queueMutex;
    static bool shutdownRequested;
    
    // Helper methods
    static std::string getCurrentTimestamp();
    static std::string logLevelToString(LogLevel level);
    static void processLogQueue();
    static void rotateLogFile();
    static bool shouldLog(LogLevel level);
    static void writeToFile(const LogEntry& entry);
    static void writeToConsole(const LogEntry& entry);
    static std::string formatLogEntry(const LogEntry& entry);
};

// Convenience macros for easier logging with file/line info
#define LOG_TRACE(msg) Logger::log(LogLevel::TRACE, msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_DEBUG(msg) Logger::log(LogLevel::DEBUG, msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO(msg) Logger::log(LogLevel::INFO, msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARNING(msg) Logger::log(LogLevel::WARNING, msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_ERROR(msg) Logger::log(LogLevel::ERROR, msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_CRITICAL(msg) Logger::log(LogLevel::CRITICAL, msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_FATAL(msg) Logger::log(LogLevel::FATAL, msg, __FILE__, __LINE__, __FUNCTION__)

// Performance timing macros
#define PERF_TIMER_START(name) auto _perf_timer = Logger::startPerformanceTimer(name)
#define PERF_TIMER_END() Logger::endPerformanceTimer(_perf_timer)

// Error handling macros
#define HANDLE_ERROR(operation, errorCode) Logger::handleError(operation, errorCode, __FILE__ ":" + std::to_string(__LINE__))
#define HANDLE_WINDOWS_ERROR(operation) Logger::handleWindowsError(operation, GetLastError())

#endif // LOGGER_H
