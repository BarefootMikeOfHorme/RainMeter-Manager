// telemetry_service.h - Enterprise-grade Telemetry Service
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <fstream>
#include "../core/logger.h"

namespace RainmeterManager {
namespace Services {

// Telemetry event types
enum class TelemetryEventType {
    UserAction,
    SystemEvent,
    PerformanceMetric,
    Error,
    Navigation,
    FeatureUsage,
    Diagnostic
};

// Metric types for performance tracking
enum class MetricType {
    Counter,    // Incrementing value (e.g., button clicks)
    Gauge,      // Point-in-time value (e.g., memory usage)
    Timer       // Duration measurement (e.g., operation time)
};

// Privacy level for event data
enum class PrivacyLevel {
    Essential,      // Critical for functionality, always sent
    Diagnostic,     // Helps improve quality, sent if enabled
    Optional        // User must opt-in explicitly
};

// Telemetry event structure
struct TelemetryEvent {
    std::string eventId;
    std::string eventName;
    TelemetryEventType type;
    PrivacyLevel privacyLevel;
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> properties;
    std::unordered_map<std::string, double> measurements;
    std::string sessionId;
    std::string userId;  // Anonymous, hashed ID
};

// Error telemetry structure
struct ErrorTelemetry {
    std::string errorId;
    std::string errorMessage;
    std::string errorCode;
    std::string stackTrace;
    std::string component;
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> context;
    int severity;  // 1=Low, 2=Medium, 3=High, 4=Critical
};

// Performance metric structure
struct PerformanceMetric {
    std::string metricName;
    MetricType type;
    double value;
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> tags;
};

// Telemetry configuration
struct TelemetryConfig {
    bool enabled = true;
    bool allowDiagnostics = true;
    bool allowOptional = false;
    int batchSize = 50;                    // Events per batch
    int flushIntervalSeconds = 60;         // Auto-flush interval
    int maxQueueSize = 10000;              // Max events in queue
    int maxRetries = 3;                    // Retry failed submissions
    int rateLimit = 1000;                  // Max events per minute
    std::string storageDirectory;
    std::string endpoint;                  // Optional remote endpoint
    bool offlineStorage = true;            // Store locally if offline
};

/**
 * @brief Enterprise-grade telemetry service
 * 
 * Features:
 * - Structured event tracking with privacy controls
 * - Batched async submission to reduce overhead
 * - Rate limiting to prevent flooding
 * - Offline storage with automatic retry
 * - Thread-safe operations
 * - JSON-based local storage
 * - Performance metric aggregation
 * - Error tracking with context
 */
class TelemetryService {
public:
    TelemetryService();
    ~TelemetryService();

    // Service lifecycle
    void Start();
    void Stop();
    void Configure(const TelemetryConfig& config);

    // Event tracking
    void TrackEvent(
        const std::string& eventName,
        TelemetryEventType type,
        PrivacyLevel privacyLevel = PrivacyLevel::Diagnostic
    );

    void TrackEventWithProperties(
        const std::string& eventName,
        const std::unordered_map<std::string, std::string>& properties,
        TelemetryEventType type = TelemetryEventType::UserAction,
        PrivacyLevel privacyLevel = PrivacyLevel::Diagnostic
    );

    // Error tracking
    void TrackError(
        const std::string& errorMessage,
        const std::string& errorCode,
        const std::string& component,
        int severity = 2
    );

    void TrackErrorWithContext(
        const std::string& errorMessage,
        const std::string& errorCode,
        const std::string& component,
        const std::string& stackTrace,
        const std::unordered_map<std::string, std::string>& context,
        int severity = 2
    );

    // Performance metrics
    void TrackMetric(
        const std::string& metricName,
        double value,
        MetricType type = MetricType::Gauge
    );

    void TrackMetricWithTags(
        const std::string& metricName,
        double value,
        const std::unordered_map<std::string, std::string>& tags,
        MetricType type = MetricType::Gauge
    );

    // Timer helpers
    class ScopedTimer {
    public:
        ScopedTimer(TelemetryService* service, const std::string& operationName);
        ~ScopedTimer();
    private:
        TelemetryService* service_;
        std::string operationName_;
        std::chrono::high_resolution_clock::time_point startTime_;
    };

    std::unique_ptr<ScopedTimer> StartTimer(const std::string& operationName);

    // Session management
    void StartSession();
    void EndSession();
    std::string GetSessionId() const;

    // Privacy controls
    void SetDiagnosticsEnabled(bool enabled);
    void SetOptionalTelemetryEnabled(bool enabled);
    bool IsDiagnosticsEnabled() const;
    bool IsOptionalTelemetryEnabled() const;

    // Manual control
    void Flush();  // Force immediate submission
    void ClearQueue();  // Clear pending events
    size_t GetQueueSize() const;

    // Statistics
    struct Statistics {
        uint64_t eventsTracked;
        uint64_t eventsSent;
        uint64_t eventsFailed;
        uint64_t errorsTracked;
        uint64_t metricsTracked;
        uint64_t rateLimitHits;
    };
    Statistics GetStatistics() const;

private:
    // Configuration
    TelemetryConfig config_;
    mutable std::mutex configMutex_;

    // Session tracking
    std::string sessionId_;
    std::string userId_;  // Anonymous hashed ID
    std::chrono::system_clock::time_point sessionStart_;

    // Event queue and processing
    std::queue<TelemetryEvent> eventQueue_;
    std::queue<ErrorTelemetry> errorQueue_;
    std::queue<PerformanceMetric> metricQueue_;
    mutable std::mutex queueMutex_;
    std::condition_variable queueCondition_;

    // Worker thread
    std::unique_ptr<std::thread> workerThread_;
    std::atomic<bool> running_;
    std::atomic<bool> stopRequested_;

    // Rate limiting
    std::chrono::steady_clock::time_point lastMinute_;
    std::atomic<int> eventsThisMinute_;
    mutable std::mutex rateLimitMutex_;

    // Statistics
    Statistics stats_;
    mutable std::mutex statsMutex_;

    // Internal methods
    void WorkerThreadFunction();
    void ProcessBatch();
    bool ShouldTrackEvent(PrivacyLevel privacyLevel) const;
    bool CheckRateLimit();
    void SaveToLocalStorage(const std::vector<TelemetryEvent>& events);
    void SaveErrorToLocalStorage(const ErrorTelemetry& error);
    void SaveMetricToLocalStorage(const PerformanceMetric& metric);
    std::string GenerateEventId() const;
    std::string GenerateSessionId() const;
    std::string GenerateUserId() const;
    std::string SerializeEventToJson(const TelemetryEvent& event) const;
    std::string SerializeErrorToJson(const ErrorTelemetry& error) const;
    std::string SerializeMetricToJson(const PerformanceMetric& metric) const;
    void EnsureStorageDirectory();
    std::string GetStorageFilePath(const std::string& prefix) const;
};

// Convenience macros
#define TELEMETRY_TRACK_EVENT(service, name) \
    (service).TrackEvent(name, TelemetryEventType::UserAction)

#define TELEMETRY_TRACK_ERROR(service, msg, code, component) \
    (service).TrackError(msg, code, component, 2)

#define TELEMETRY_TRACK_METRIC(service, name, value) \
    (service).TrackMetric(name, value, MetricType::Gauge)

#define TELEMETRY_TIMED_SCOPE(service, name) \
    auto __timer_##name = (service).StartTimer(#name)

} // namespace Services
} // namespace RainmeterManager
