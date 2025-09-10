#include "logger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <winreg.h>

// Static member definitions
std::unique_ptr<std::ofstream> Logger::logFile = nullptr;
std::mutex Logger::logMutex;
LogLevel Logger::minLogLevel = LogLevel::INFO;
LogRotationConfig Logger::rotationConfig;
bool Logger::asyncLoggingEnabled = false;
bool Logger::consoleOutputEnabled = true;
std::queue<LogEntry> Logger::logQueue;
std::thread Logger::loggingThread;
std::condition_variable Logger::logCondition;
std::mutex Logger::queueMutex;
bool Logger::shutdownRequested = false;

bool Logger::initialize(const std::string& logFilePath, LogRotationConfig config) {
    {
        std::lock_guard<std::mutex> lock(logMutex);
        rotationConfig = config;
        logFile = std::make_unique<std::ofstream>(logFilePath, std::ios::app);
        if (!logFile->is_open()) {
            std::cerr << "Failed to open log file: " << logFilePath << std::endl;
            return false;
        }
        if (asyncLoggingEnabled) {
            loggingThread = std::thread(processLogQueue);
        }
    }
    // Log after releasing logMutex to avoid re-entrancy deadlock
    LOG_INFO("Logger initialized successfully");
    return true;
}

void Logger::log(LogLevel level, const std::string& message, const char* file, int line, const char* function) {
    if (!shouldLog(level)) return;
    
    LogEntry entry = {
        level,
        message,
        std::chrono::system_clock::now(),
        getThreadId(),
        std::filesystem::path(file).filename().string(),
        line,
        function
    };
    
    if (asyncLoggingEnabled) {
        std::lock_guard<std::mutex> lock(queueMutex);
        logQueue.push(entry);
        logCondition.notify_one();
    } else {
        writeToFile(entry);
        if (consoleOutputEnabled) {
            writeToConsole(entry);
        }
    }
}

ErrorResult Logger::handleError(const std::string& operation, int errorCode, const std::string& details) {
    ErrorResult result;
    result.success = false;
    result.errorCode = errorCode;
    result.errorMessage = operation + " failed with code: " + std::to_string(errorCode);
    result.contextInfo = details;
    result.timestamp = std::chrono::system_clock::now();
    result.threadId = getThreadId();
    result.stackTrace = captureStackTrace(2);
    
    LOG_ERROR(result.errorMessage + " | Details: " + details);
    return result;
}

ErrorResult Logger::handleWindowsError(const std::string& operation, DWORD errorCode) {
    char* messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer, 0, NULL);
    
    std::string message = messageBuffer ? messageBuffer : "Unknown error";
    LocalFree(messageBuffer);
    
    ErrorResult result;
    result.success = false;
    result.errorCode = errorCode;
    result.errorMessage = operation + " failed: " + message;
    result.timestamp = std::chrono::system_clock::now();
    result.threadId = getThreadId();
    result.stackTrace = captureStackTrace(2);
    
    LOG_ERROR(result.errorMessage);
    return result;
}

std::string Logger::captureStackTrace(int skipFrames) {
    const int maxFrames = 64;
    void* stack[maxFrames];
    
    WORD numberOfFrames = CaptureStackBackTrace(skipFrames, maxFrames, stack, NULL);
    
    std::stringstream ss;
    ss << "Stack trace (" << numberOfFrames << " frames):\n";
    
    for (int i = 0; i < numberOfFrames; i++) {
        ss << "  Frame " << i << ": 0x" << std::hex << (uintptr_t)stack[i] << std::dec << "\n";
    }
    
    return ss.str();
}

PerformanceMetrics Logger::startPerformanceTimer(const std::string& operationName, const std::string& context) {
    PerformanceMetrics metrics;
    metrics.startTime = std::chrono::high_resolution_clock::now();
    metrics.operationName = operationName;
    metrics.contextInfo = context;
    
    LOG_DEBUG("Performance timer started for: " + operationName);
    return metrics;
}

void Logger::endPerformanceTimer(const PerformanceMetrics& metrics) {
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - metrics.startTime);
    
    std::stringstream ss;
    ss << "Performance: " << metrics.operationName << " completed in " << duration.count() << "ms";
    if (!metrics.contextInfo.empty()) {
        ss << " | Context: " << metrics.contextInfo;
    }
    
    LOG_INFO(ss.str());
}

ErrorResult Logger::retryOperation(std::function<ErrorResult()> operation, int maxRetries, int delayMs) {
    ErrorResult lastResult;
    
    for (int attempt = 1; attempt <= maxRetries; attempt++) {
        LOG_DEBUG("Retry attempt " + std::to_string(attempt) + " of " + std::to_string(maxRetries));
        
        lastResult = operation();
        if (lastResult.success) {
            if (attempt > 1) {
                LOG_INFO("Operation succeeded on attempt " + std::to_string(attempt));
            }
            return lastResult;
        }
        
        if (attempt < maxRetries) {
            LOG_WARNING("Attempt " + std::to_string(attempt) + " failed, retrying in " + std::to_string(delayMs) + "ms");
            Sleep(delayMs);
        }
    }
    
    LOG_ERROR("Operation failed after " + std::to_string(maxRetries) + " attempts");
    return lastResult;
}

void Logger::logSecurityEvent(const std::string& event, const std::string& details) {
    std::stringstream ss;
    ss << "SECURITY EVENT: " << event;
    if (!details.empty()) {
        ss << " | Details: " << details;
    }
    LOG_CRITICAL(ss.str());
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm localTm{};
#if defined(_WIN32)
    localtime_s(&localTm, &t);
#else
    localtime_r(&t, &localTm);
#endif

    std::stringstream ss;
    ss << std::put_time(&localTm, "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Logger::getThreadId() {
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

std::string Logger::logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRIT";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

bool Logger::shouldLog(LogLevel level) {
    return level >= minLogLevel;
}

std::string Logger::formatLogEntry(const LogEntry& entry) {
    std::stringstream ss;
    ss << "[" << getCurrentTimestamp() << "] "
       << "[" << logLevelToString(entry.level) << "] "
       << "[" << entry.threadId << "] "
       << "[" << entry.file << ":" << entry.line << "] "
       << entry.message;
    return ss.str();
}

void Logger::writeToFile(const LogEntry& entry) {
    if (logFile && logFile->is_open()) {
        std::lock_guard<std::mutex> lock(logMutex);
        *logFile << formatLogEntry(entry) << std::endl;
        logFile->flush();
        
        if (rotationConfig.enableRotation) {
            rotateLogFile();
        }
    }
}

void Logger::writeToConsole(const LogEntry& entry) {
    std::cout << formatLogEntry(entry) << std::endl;
}

void Logger::processLogQueue() {
    while (!shutdownRequested) {
        std::unique_lock<std::mutex> lock(queueMutex);
        logCondition.wait(lock, [] { return !logQueue.empty() || shutdownRequested; });
        
        while (!logQueue.empty()) {
            LogEntry entry = logQueue.front();
            logQueue.pop();
            lock.unlock();
            
            writeToFile(entry);
            if (consoleOutputEnabled) {
                writeToConsole(entry);
            }
            
            lock.lock();
        }
    }
}

void Logger::rotateLogFile() {
    // Implementation for log rotation based on file size
    if (logFile) {
        auto pos = logFile->tellp();
        if (pos > static_cast<std::streampos>(rotationConfig.maxFileSize)) {
            // Rotate log files
            logFile->close();
            // Implement rotation logic here
            logFile = std::make_unique<std::ofstream>("rotated_log.txt", std::ios::app);
        }
    }
}

void Logger::shutdown() {
    if (asyncLoggingEnabled) {
        shutdownRequested = true;
        logCondition.notify_all();
        if (loggingThread.joinable()) {
            loggingThread.join();
        }
    }
    
    if (logFile) {
        logFile->close();
    }
    
    LOG_INFO("Logger shutdown completed");
}

//=============================================================================
// Missing Method Implementations
//=============================================================================

void Logger::setLogLevel(LogLevel minLevel) {
    std::string msg;
    {
        std::lock_guard<std::mutex> lock(logMutex);
        Logger::minLogLevel = minLevel;
        msg = std::string("Log level set to: ") + logLevelToString(minLevel);
    }
    LOG_INFO(msg);
}

void Logger::enableAsyncLogging(bool enable) {
    bool justEnabled = false;
    bool justDisabled = false;
    {
        std::lock_guard<std::mutex> lock(logMutex);
        if (asyncLoggingEnabled != enable) {
            asyncLoggingEnabled = enable;
            if (enable && !loggingThread.joinable()) {
                loggingThread = std::thread(processLogQueue);
                justEnabled = true;
            } else if (!enable && loggingThread.joinable()) {
                shutdownRequested = true;
                logCondition.notify_all();
                loggingThread.join();
                shutdownRequested = false;
                justDisabled = true;
            }
        }
    }
    if (justEnabled) LOG_INFO("Async logging enabled");
    if (justDisabled) LOG_INFO("Async logging disabled");
}

void Logger::enableConsoleOutput(bool enable) {
    bool now;
    {
        std::lock_guard<std::mutex> lock(logMutex);
        consoleOutputEnabled = enable;
        now = consoleOutputEnabled;
    }
    LOG_INFO(std::string("Console output ") + (now ? "enabled" : "disabled"));
}

void Logger::trace(const std::string& message) {
    log(LogLevel::TRACE, message);
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::critical(const std::string& message) {
    log(LogLevel::CRITICAL, message);
}

void Logger::fatal(const std::string& message) {
    log(LogLevel::FATAL, message);
}

void Logger::logException(const std::exception& e, const std::string& context) {
    std::string message = "Exception: " + std::string(e.what());
    if (!context.empty()) {
        message += " | Context: " + context;
    }
    message += " | Stack: " + captureStackTrace(2);
    LOG_ERROR(message);
}

void Logger::logSystemError(const std::string& operation) {
    DWORD errorCode = GetLastError();
    handleWindowsError(operation, errorCode);
}

ErrorResult Logger::safeFileOperation(std::function<bool()> operation, const std::string& operationName) {
    return retryOperation([&]() -> ErrorResult {
        try {
            if (operation()) {
                return {true, "", 0, "", operationName, std::chrono::system_clock::now(), getThreadId()};
            } else {
                return handleError(operationName, -1, "Operation returned false");
            }
        } catch (const std::exception& e) {
            return {false, e.what(), -1, captureStackTrace(1), operationName, 
                   std::chrono::system_clock::now(), getThreadId()};
        }
    });
}

void Logger::logAccessAttempt(const std::string& resource, bool success, const std::string& user) {
    std::string message = "Access attempt: " + resource + 
                         " | Result: " + (success ? "SUCCESS" : "DENIED");
    if (!user.empty()) {
        message += " | User: " + user;
    }
    
    if (success) {
        LOG_INFO(message);
    } else {
        logSecurityEvent("Access Denied", message);
    }
}

void Logger::dumpSystemInfo() {
    LOG_INFO("=== System Information Dump ===");
    
    // OS Version (query via registry to avoid deprecated APIs)
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                      0,
                      KEY_READ | KEY_WOW64_64KEY,
                      &hKey) == ERROR_SUCCESS) {
        DWORD major = 0, minor = 0;
        DWORD size = sizeof(DWORD);
        (void)RegGetValueW(hKey, nullptr, L"CurrentMajorVersionNumber", RRF_RT_DWORD, nullptr, &major, &size);
        size = sizeof(DWORD);
        (void)RegGetValueW(hKey, nullptr, L"CurrentMinorVersionNumber", RRF_RT_DWORD, nullptr, &minor, &size);
        wchar_t build[64] = {0};
        DWORD buildSize = sizeof(build);
        if (RegGetValueW(hKey, nullptr, L"CurrentBuildNumber", RRF_RT_REG_SZ, nullptr, &build, &buildSize) != ERROR_SUCCESS) {
            build[0] = L'\0';
        }
        RegCloseKey(hKey);
        std::wstring wbuild(build);
        std::string sbuild(wbuild.begin(), wbuild.end());
        LOG_INFO("OS Version: " + std::to_string(major) + "." + std::to_string(minor) + " Build " + sbuild);
    }
    
    // Memory Information
    MEMORYSTATUSEX memInfo = {0};
    memInfo.dwLength = sizeof(memInfo);
    if (GlobalMemoryStatusEx(&memInfo)) {
        LOG_INFO("Total RAM: " + std::to_string(memInfo.ullTotalPhys / (1024*1024)) + " MB");
        LOG_INFO("Available RAM: " + std::to_string(memInfo.ullAvailPhys / (1024*1024)) + " MB");
        LOG_INFO("Memory Load: " + std::to_string(memInfo.dwMemoryLoad) + "%");
    }
    
    // CPU Information
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    LOG_INFO("CPU Cores: " + std::to_string(sysInfo.dwNumberOfProcessors));
    LOG_INFO("Page Size: " + std::to_string(sysInfo.dwPageSize));
    
    // Current Process Info
    LOG_INFO("Process ID: " + std::to_string(GetCurrentProcessId()));
    LOG_INFO("Thread ID: " + getThreadId());
    
    LOG_INFO("=== End System Information ===");
}

void Logger::flushLogs() {
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile && logFile->is_open()) {
        logFile->flush();
    }
    
    // Also flush console
    std::cout.flush();
    std::cerr.flush();
}
