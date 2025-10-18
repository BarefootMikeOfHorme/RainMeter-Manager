// telemetry_service.cpp - Enterprise-grade Telemetry Service Implementation
#include "telemetry_service.h"
#include <sstream>
#include <iomanip>
#include <random>
#include <filesystem>
#include <windows.h>
#include <wincrypt.h>

namespace fs = std::filesystem;

namespace RainmeterManager {
namespace Services {

// Constructor
TelemetryService::TelemetryService()
    : running_(false)
    , stopRequested_(false)
    , eventsThisMinute_(0)
    , lastMinute_(std::chrono::steady_clock::now())
    , stats_{0, 0, 0, 0, 0, 0}
{
    // Set default storage directory
    char* appdata = nullptr;
    size_t len = 0;
    if (_dupenv_s(&appdata, &len, "APPDATA") == 0 && appdata != nullptr) {
        config_.storageDirectory = std::string(appdata) + "\\RainmeterManager\\Telemetry";
        free(appdata);
    } else {
        config_.storageDirectory = ".\\telemetry";
    }
    
    userId_ = GenerateUserId();
    LOG_INFO("TelemetryService created");
}

// Destructor
TelemetryService::~TelemetryService() {
    Stop();
}

// Start the telemetry service
void TelemetryService::Start() {
    if (running_) {
        LOG_WARNING("TelemetryService already running");
        return;
    }
    
    LOG_INFO("Starting TelemetryService...");
    
    EnsureStorageDirectory();
    StartSession();
    
    running_ = true;
    stopRequested_ = false;
    
    // Start worker thread for async batch processing
    workerThread_ = std::make_unique<std::thread>(&TelemetryService::WorkerThreadFunction, this);
    
    LOG_INFO("TelemetryService started successfully");
    TrackEvent("TelemetryService.Started", TelemetryEventType::SystemEvent, PrivacyLevel::Essential);
}

// Stop the telemetry service
void TelemetryService::Stop() {
    if (!running_) {
        return;
    }
    
    LOG_INFO("Stopping TelemetryService...");
    
    TrackEvent("TelemetryService.Stopped", TelemetryEventType::SystemEvent, PrivacyLevel::Essential);
    
    stopRequested_ = true;
    queueCondition_.notify_all();
    
    if (workerThread_ && workerThread_->joinable()) {
        workerThread_->join();
    }
    
    running_ = false;
    
    // Flush remaining events
    Flush();
    
    EndSession();
    
    LOG_INFO("TelemetryService stopped");
}

// Configure telemetry
void TelemetryService::Configure(const TelemetryConfig& config) {
    std::lock_guard<std::mutex> lock(configMutex_);
    config_ = config;
    EnsureStorageDirectory();
    LOG_INFO("TelemetryService configured");
}

// Track simple event
void TelemetryService::TrackEvent(
    const std::string& eventName,
    TelemetryEventType type,
    PrivacyLevel privacyLevel
) {
    TrackEventWithProperties(eventName, {}, type, privacyLevel);
}

// Track event with properties
void TelemetryService::TrackEventWithProperties(
    const std::string& eventName,
    const std::unordered_map<std::string, std::string>& properties,
    TelemetryEventType type,
    PrivacyLevel privacyLevel
) {
    if (!config_.enabled || !ShouldTrackEvent(privacyLevel) || !CheckRateLimit()) {
        return;
    }
    
    TelemetryEvent event;
    event.eventId = GenerateEventId();
    event.eventName = eventName;
    event.type = type;
    event.privacyLevel = privacyLevel;
    event.timestamp = std::chrono::system_clock::now();
    event.properties = properties;
    event.sessionId = sessionId_;
    event.userId = userId_;
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (eventQueue_.size() < static_cast<size_t>(config_.maxQueueSize)) {
            eventQueue_.push(event);
            queueCondition_.notify_one();
            
            std::lock_guard<std::mutex> statsLock(statsMutex_);
            stats_.eventsTracked++;
        } else {
            LOG_WARNING("Telemetry queue full, dropping event: " + eventName);
        }
    }
}

// Track error
void TelemetryService::TrackError(
    const std::string& errorMessage,
    const std::string& errorCode,
    const std::string& component,
    int severity
) {
    TrackErrorWithContext(errorMessage, errorCode, component, "", {}, severity);
}

// Track error with context
void TelemetryService::TrackErrorWithContext(
    const std::string& errorMessage,
    const std::string& errorCode,
    const std::string& component,
    const std::string& stackTrace,
    const std::unordered_map<std::string, std::string>& context,
    int severity
) {
    if (!config_.enabled) {
        return;
    }
    
    ErrorTelemetry error;
    error.errorId = GenerateEventId();
    error.errorMessage = errorMessage;
    error.errorCode = errorCode;
    error.stackTrace = stackTrace.empty() ? Logger::captureStackTrace(2) : stackTrace;
    error.component = component;
    error.timestamp = std::chrono::system_clock::now();
    error.context = context;
    error.severity = severity;
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        errorQueue_.push(error);
        queueCondition_.notify_one();
        
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        stats_.errorsTracked++;
    }
    
    LOG_ERROR("Telemetry tracked error: " + errorCode + " - " + errorMessage);
}

// Track metric
void TelemetryService::TrackMetric(
    const std::string& metricName,
    double value,
    MetricType type
) {
    TrackMetricWithTags(metricName, value, {}, type);
}

// Track metric with tags
void TelemetryService::TrackMetricWithTags(
    const std::string& metricName,
    double value,
    const std::unordered_map<std::string, std::string>& tags,
    MetricType type
) {
    if (!config_.enabled) {
        return;
    }
    
    PerformanceMetric metric;
    metric.metricName = metricName;
    metric.type = type;
    metric.value = value;
    metric.timestamp = std::chrono::system_clock::now();
    metric.tags = tags;
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        metricQueue_.push(metric);
        queueCondition_.notify_one();
        
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        stats_.metricsTracked++;
    }
}

// ScopedTimer implementation
TelemetryService::ScopedTimer::ScopedTimer(TelemetryService* service, const std::string& operationName)
    : service_(service)
    , operationName_(operationName)
    , startTime_(std::chrono::high_resolution_clock::now())
{
}

TelemetryService::ScopedTimer::~ScopedTimer() {
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime_).count();
    service_->TrackMetric(operationName_ + ".Duration", static_cast<double>(duration), MetricType::Timer);
}

std::unique_ptr<TelemetryService::ScopedTimer> TelemetryService::StartTimer(const std::string& operationName) {
    return std::make_unique<ScopedTimer>(this, operationName);
}

// Session management
void TelemetryService::StartSession() {
    sessionId_ = GenerateSessionId();
    sessionStart_ = std::chrono::system_clock::now();
    LOG_INFO("Telemetry session started: " + sessionId_);
}

void TelemetryService::EndSession() {
    if (sessionId_.empty()) {
        return;
    }
    
    auto sessionEnd = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(sessionEnd - sessionStart_).count();
    
    std::unordered_map<std::string, std::string> props;
    props["duration_seconds"] = std::to_string(duration);
    props["events_tracked"] = std::to_string(stats_.eventsTracked);
    props["errors_tracked"] = std::to_string(stats_.errorsTracked);
    
    TrackEventWithProperties("Session.Ended", props, TelemetryEventType::SystemEvent, PrivacyLevel::Essential);
    
    LOG_INFO("Telemetry session ended: " + sessionId_);
    sessionId_.clear();
}

std::string TelemetryService::GetSessionId() const {
    return sessionId_;
}

// Privacy controls
void TelemetryService::SetDiagnosticsEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(configMutex_);
    config_.allowDiagnostics = enabled;
    LOG_INFO(std::string("Diagnostics telemetry ") + (enabled ? "enabled" : "disabled"));
}

void TelemetryService::SetOptionalTelemetryEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(configMutex_);
    config_.allowOptional = enabled;
    LOG_INFO(std::string("Optional telemetry ") + (enabled ? "enabled" : "disabled"));
}

bool TelemetryService::IsDiagnosticsEnabled() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return config_.allowDiagnostics;
}

bool TelemetryService::IsOptionalTelemetryEnabled() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return config_.allowOptional;
}

// Manual control
void TelemetryService::Flush() {
    LOG_INFO("Flushing telemetry data...");
    queueCondition_.notify_all();
    
    // Give worker thread time to process
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Process any remaining items synchronously
    ProcessBatch();
}

void TelemetryService::ClearQueue() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    while (!eventQueue_.empty()) eventQueue_.pop();
    while (!errorQueue_.empty()) errorQueue_.pop();
    while (!metricQueue_.empty()) metricQueue_.pop();
    
    LOG_INFO("Telemetry queues cleared");
}

size_t TelemetryService::GetQueueSize() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return eventQueue_.size() + errorQueue_.size() + metricQueue_.size();
}

// Statistics
TelemetryService::Statistics TelemetryService::GetStatistics() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_;
}

// Worker thread function
void TelemetryService::WorkerThreadFunction() {
    LOG_INFO("Telemetry worker thread started");
    
    auto lastFlush = std::chrono::steady_clock::now();
    
    while (!stopRequested_) {
        std::unique_lock<std::mutex> lock(queueMutex_);
        
        // Wait for events or flush interval
        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastFlush = std::chrono::duration_cast<std::chrono::seconds>(now - lastFlush).count();
        
        if (timeSinceLastFlush >= config_.flushIntervalSeconds || 
            eventQueue_.size() >= static_cast<size_t>(config_.batchSize)) {
            lock.unlock();
            ProcessBatch();
            lastFlush = std::chrono::steady_clock::now();
        } else {
            queueCondition_.wait_for(lock, std::chrono::seconds(1));
        }
    }
    
    LOG_INFO("Telemetry worker thread stopped");
}

// Process a batch of telemetry data
void TelemetryService::ProcessBatch() {
    std::vector<TelemetryEvent> events;
    std::vector<ErrorTelemetry> errors;
    std::vector<PerformanceMetric> metrics;
    
    // Collect batch
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        
        int batchSize = config_.batchSize;
        while (!eventQueue_.empty() && batchSize-- > 0) {
            events.push_back(eventQueue_.front());
            eventQueue_.pop();
        }
        
        batchSize = config_.batchSize;
        while (!errorQueue_.empty() && batchSize-- > 0) {
            errors.push_back(errorQueue_.front());
            errorQueue_.pop();
        }
        
        batchSize = config_.batchSize;
        while (!metricQueue_.empty() && batchSize-- > 0) {
            metrics.push_back(metricQueue_.front());
            metricQueue_.pop();
        }
    }
    
    // Save to local storage
    if (!events.empty()) {
        SaveToLocalStorage(events);
        
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        stats_.eventsSent += events.size();
    }
    
    for (const auto& error : errors) {
        SaveErrorToLocalStorage(error);
    }
    
    for (const auto& metric : metrics) {
        SaveMetricToLocalStorage(metric);
    }
}

// Check privacy settings
bool TelemetryService::ShouldTrackEvent(PrivacyLevel privacyLevel) const {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    switch (privacyLevel) {
        case PrivacyLevel::Essential:
            return true;
        case PrivacyLevel::Diagnostic:
            return config_.allowDiagnostics;
        case PrivacyLevel::Optional:
            return config_.allowOptional;
        default:
            return false;
    }
}

// Rate limiting check
bool TelemetryService::CheckRateLimit() {
    std::lock_guard<std::mutex> lock(rateLimitMutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastMinute_).count();
    
    if (elapsed >= 60) {
        // Reset counter every minute
        lastMinute_ = now;
        eventsThisMinute_ = 0;
    }
    
    if (eventsThisMinute_ >= config_.rateLimit) {
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        stats_.rateLimitHits++;
        return false;
    }
    
    eventsThisMinute_++;
    return true;
}

// Save events to local JSON storage
void TelemetryService::SaveToLocalStorage(const std::vector<TelemetryEvent>& events) {
    if (!config_.offlineStorage || events.empty()) {
        return;
    }
    
    try {
        std::string filePath = GetStorageFilePath("events");
        std::ofstream file(filePath, std::ios::app);
        
        if (file.is_open()) {
            for (const auto& event : events) {
                file << SerializeEventToJson(event) << std::endl;
            }
            file.close();
        } else {
            LOG_ERROR("Failed to open telemetry storage file: " + filePath);
        }
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Error saving telemetry to storage: ") + e.what());
    }
}

void TelemetryService::SaveErrorToLocalStorage(const ErrorTelemetry& error) {
    if (!config_.offlineStorage) {
        return;
    }
    
    try {
        std::string filePath = GetStorageFilePath("errors");
        std::ofstream file(filePath, std::ios::app);
        
        if (file.is_open()) {
            file << SerializeErrorToJson(error) << std::endl;
            file.close();
        }
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Error saving error telemetry: ") + e.what());
    }
}

void TelemetryService::SaveMetricToLocalStorage(const PerformanceMetric& metric) {
    if (!config_.offlineStorage) {
        return;
    }
    
    try {
        std::string filePath = GetStorageFilePath("metrics");
        std::ofstream file(filePath, std::ios::app);
        
        if (file.is_open()) {
            file << SerializeMetricToJson(metric) << std::endl;
            file.close();
        }
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Error saving metric telemetry: ") + e.what());
    }
}

// JSON serialization helpers
std::string TelemetryService::SerializeEventToJson(const TelemetryEvent& event) const {
    std::ostringstream json;
    json << "{";
    json << "\"eventId\":\"" << event.eventId << "\",";
    json << "\"eventName\":\"" << event.eventName << "\",";
    json << "\"type\":" << static_cast<int>(event.type) << ",";
    json << "\"privacyLevel\":" << static_cast<int>(event.privacyLevel) << ",";
    json << "\"timestamp\":" << std::chrono::system_clock::to_time_t(event.timestamp) << ",";
    json << "\"sessionId\":\"" << event.sessionId << "\",";
    json << "\"userId\":\"" << event.userId << "\"";
    
    if (!event.properties.empty()) {
        json << ",\"properties\":{";
        bool first = true;
        for (const auto& [key, value] : event.properties) {
            if (!first) json << ",";
            json << "\"" << key << "\":\"" << value << "\"";
            first = false;
        }
        json << "}";
    }
    
    if (!event.measurements.empty()) {
        json << ",\"measurements\":{";
        bool first = true;
        for (const auto& [key, value] : event.measurements) {
            if (!first) json << ",";
            json << "\"" << key << "\":" << value;
            first = false;
        }
        json << "}";
    }
    
    json << "}";
    return json.str();
}

std::string TelemetryService::SerializeErrorToJson(const ErrorTelemetry& error) const {
    std::ostringstream json;
    json << "{";
    json << "\"errorId\":\"" << error.errorId << "\",";
    json << "\"errorMessage\":\"" << error.errorMessage << "\",";
    json << "\"errorCode\":\"" << error.errorCode << "\",";
    json << "\"component\":\"" << error.component << "\",";
    json << "\"severity\":" << error.severity << ",";
    json << "\"timestamp\":" << std::chrono::system_clock::to_time_t(error.timestamp);
    
    if (!error.stackTrace.empty()) {
        json << ",\"stackTrace\":\"" << error.stackTrace << "\"";
    }
    
    if (!error.context.empty()) {
        json << ",\"context\":{";
        bool first = true;
        for (const auto& [key, value] : error.context) {
            if (!first) json << ",";
            json << "\"" << key << "\":\"" << value << "\"";
            first = false;
        }
        json << "}";
    }
    
    json << "}";
    return json.str();
}

std::string TelemetryService::SerializeMetricToJson(const PerformanceMetric& metric) const {
    std::ostringstream json;
    json << "{";
    json << "\"metricName\":\"" << metric.metricName << "\",";
    json << "\"type\":" << static_cast<int>(metric.type) << ",";
    json << "\"value\":" << metric.value << ",";
    json << "\"timestamp\":" << std::chrono::system_clock::to_time_t(metric.timestamp);
    
    if (!metric.tags.empty()) {
        json << ",\"tags\":{";
        bool first = true;
        for (const auto& [key, value] : metric.tags) {
            if (!first) json << ",";
            json << "\"" << key << "\":\"" << value << "\"";
            first = false;
        }
        json << "}";
    }
    
    json << "}";
    return json.str();
}

// ID generation
std::string TelemetryService::GenerateEventId() const {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << dis(gen);
    return oss.str();
}

std::string TelemetryService::GenerateSessionId() const {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream oss;
    oss << "session_" << timestamp << "_" << GenerateEventId();
    return oss.str();
}

std::string TelemetryService::GenerateUserId() const {
    // Generate anonymous user ID based on machine characteristics
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    
    if (GetComputerNameA(computerName, &size)) {
        // Hash the computer name for anonymity
        std::hash<std::string> hasher;
        size_t hash = hasher(std::string(computerName));
        
        std::ostringstream oss;
        oss << "user_" << std::hex << hash;
        return oss.str();
    }
    
    return "user_unknown";
}

// Storage helpers
void TelemetryService::EnsureStorageDirectory() {
    try {
        fs::path storagePath(config_.storageDirectory);
        if (!fs::exists(storagePath)) {
            fs::create_directories(storagePath);
            LOG_INFO("Created telemetry storage directory: " + config_.storageDirectory);
        }
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Failed to create storage directory: ") + e.what());
    }
}

std::string TelemetryService::GetStorageFilePath(const std::string& prefix) const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::tm tm;
    localtime_s(&tm, &time);
    
    std::ostringstream filename;
    filename << prefix << "_" 
             << std::put_time(&tm, "%Y%m%d") 
             << ".jsonl";  // JSON Lines format
    
    fs::path fullPath = fs::path(config_.storageDirectory) / filename.str();
    return fullPath.string();
}

} // namespace Services
} // namespace RainmeterManager
