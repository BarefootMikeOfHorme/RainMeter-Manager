#ifndef PRIVACY_MANAGER_H
#define PRIVACY_MANAGER_H

#include <windows.h>
#include <evntprov.h>
#include <string>
#include <mutex>
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>
#include "logger.h"

#pragma comment(lib, "advapi32.lib")

namespace RainmeterManager {
namespace Core {

/**
 * @brief Privacy levels for telemetry data collection
 */
enum class PrivacyLevel {
    NONE = 0,           // No telemetry collected
    ESSENTIAL = 1,      // Only critical error reporting
    ENHANCED = 2,       // Performance metrics + errors
    FULL = 3            // Complete telemetry including usage analytics
};

/**
 * @brief Types of telemetry data
 */
enum class TelemetryType {
    ERROR_REPORTING,     // Crash reports and errors
    PERFORMANCE_METRICS, // CPU, memory, render times
    USAGE_ANALYTICS,     // Feature usage, user interactions
    SECURITY_EVENTS,     // Security-related events
    SYSTEM_INFO         // Hardware/OS information
};

/**
 * @brief Event categories for ETW providers
 */
enum class EventCategory {
    APPLICATION = 1,
    SECURITY = 2,
    PERFORMANCE = 3,
    USER_INTERACTION = 4,
    SYSTEM = 5,
    NETWORK = 6,
    WIDGET = 7,
    CONFIGURATION = 8
};

/**
 * @brief Privacy-aware telemetry event
 */
struct TelemetryEvent {
    EventCategory category;
    TelemetryType type;
    std::string eventName;
    std::string eventData;
    PrivacyLevel requiredLevel;
    SYSTEMTIME timestamp;
    bool containsPII;           // Contains personally identifiable information
    
    TelemetryEvent() : category(EventCategory::APPLICATION), type(TelemetryType::ERROR_REPORTING),
                      requiredLevel(PrivacyLevel::ESSENTIAL), containsPII(false) {
        GetSystemTime(&timestamp);
    }
};

/**
 * @brief ETW Provider configuration
 */
struct ETWProvider {
    GUID providerId;
    std::wstring providerName;
    REGHANDLE handle;
    bool isRegistered;
    PrivacyLevel minimumLevel;
    
    ETWProvider() : handle(0), isRegistered(false), minimumLevel(PrivacyLevel::ESSENTIAL) {
        ZeroMemory(&providerId, sizeof(GUID));
    }
};

/**
 * @brief Privacy notification callback
 */
using PrivacyChangeCallback = std::function<void(PrivacyLevel oldLevel, PrivacyLevel newLevel)>;

/**
 * @brief Comprehensive privacy manager for telemetry and ETW
 * 
 * Features:
 * - Granular privacy level controls
 * - ETW provider registration and management
 * - PII detection and filtering
 * - User consent management
 * - GDPR/privacy law compliance helpers
 * - Real-time privacy level changes
 * - Telemetry data retention policies
 */
class PrivacyManager {
private:
    static PrivacyManager* instance_;
    
    // Privacy settings
    PrivacyLevel currentPrivacyLevel_;
    bool telemetryEnabled_;
    bool userHasConsented_;
    bool piiCollectionAllowed_;
    
    // ETW providers
    std::unordered_map<std::string, std::unique_ptr<ETWProvider>> etwProviders_;
    
    // Event queuing and filtering
    std::vector<TelemetryEvent> pendingEvents_;
    mutable std::mutex eventQueueMutex_;
    
    // Configuration
    std::wstring configFilePath_;
    bool autoSaveSettings_;
    
    // Callbacks
    std::vector<PrivacyChangeCallback> privacyChangeCallbacks_;
    mutable std::mutex callbackMutex_;
    
    // Data retention
    int maxEventRetentionDays_;
    size_t maxEventQueueSize_;
    
    // PII detection patterns
    std::vector<std::wstring> piiPatterns_;

public:
    /**
     * @brief Get singleton instance
     */
    static PrivacyManager& GetInstance();
    
    /**
     * @brief Destroy singleton instance
     */
    static void DestroyInstance();
    
    /**
     * @brief Initialize privacy manager
     */
    bool Initialize(const std::wstring& configPath = L"");
    
    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();
    
    // Privacy Level Management
    /**
     * @brief Set privacy level
     */
    bool SetPrivacyLevel(PrivacyLevel level, bool saveToConfig = true);
    
    /**
     * @brief Get current privacy level
     */
    PrivacyLevel GetPrivacyLevel() const { return currentPrivacyLevel_; }
    
    /**
     * @brief Check if telemetry is enabled
     */
    bool IsTelemetryEnabled() const { return telemetryEnabled_; }
    
    /**
     * @brief Enable/disable telemetry completely
     */
    bool SetTelemetryEnabled(bool enabled, bool saveToConfig = true);
    
    // User Consent Management
    /**
     * @brief Set user consent status
     */
    bool SetUserConsent(bool hasConsented, bool saveToConfig = true);
    
    /**
     * @brief Check if user has consented to telemetry
     */
    bool HasUserConsent() const { return userHasConsented_; }
    
    /**
     * @brief Show privacy consent dialog
     */
    bool ShowConsentDialog(HWND parentWindow = nullptr);
    
    // ETW Provider Management
    /**
     * @brief Register ETW provider
     */
    bool RegisterETWProvider(const std::string& providerName, 
                           const GUID& providerId,
                           PrivacyLevel minimumLevel = PrivacyLevel::ESSENTIAL);
    
    /**
     * @brief Unregister ETW provider
     */
    bool UnregisterETWProvider(const std::string& providerName);
    
    /**
     * @brief Get ETW provider handle
     */
    REGHANDLE GetETWHandle(const std::string& providerName) const;
    
    /**
     * @brief Write ETW event
     */
    bool WriteETWEvent(const std::string& providerName,
                      const TelemetryEvent& event);
    
    // Event Management
    /**
     * @brief Log telemetry event (with privacy filtering)
     */
    bool LogTelemetryEvent(const TelemetryEvent& event);
    
    /**
     * @brief Check if event type is allowed at current privacy level
     */
    bool IsEventTypeAllowed(TelemetryType type) const;
    
    /**
     * @brief Check if event contains PII
     */
    bool ContainsPII(const std::string& data) const;
    
    /**
     * @brief Sanitize data by removing PII
     */
    std::string SanitizeData(const std::string& data) const;
    
    // Privacy Callbacks
    /**
     * @brief Register privacy change callback
     */
    void RegisterPrivacyChangeCallback(PrivacyChangeCallback callback);
    
    /**
     * @brief Clear all privacy change callbacks
     */
    void ClearPrivacyChangeCallbacks();
    
    // Configuration Management
    /**
     * @brief Load privacy settings from config
     */
    bool LoadPrivacySettings();
    
    /**
     * @brief Save privacy settings to config
     */
    bool SavePrivacySettings();
    
    /**
     * @brief Reset to default privacy settings
     */
    void ResetToDefaults();
    
    // Compliance Helpers
    /**
     * @brief Generate privacy report for compliance
     */
    std::string GeneratePrivacyReport() const;
    
    /**
     * @brief Export user data (GDPR compliance)
     */
    bool ExportUserData(const std::wstring& exportPath) const;
    
    /**
     * @brief Delete all user telemetry data
     */
    bool DeleteAllUserData();
    
    /**
     * @brief Check if configured for GDPR compliance
     */
    bool IsGDPRCompliant() const;
    
    // Event Queue Management
    /**
     * @brief Get pending event count
     */
    size_t GetPendingEventCount() const;
    
    /**
     * @brief Process pending events
     */
    void ProcessPendingEvents();
    
    /**
     * @brief Clear event queue
     */
    void ClearEventQueue();
    
    // Utility Methods
    /**
     * @brief Get privacy level name
     */
    static std::wstring GetPrivacyLevelName(PrivacyLevel level);
    
    /**
     * @brief Get telemetry type name
     */
    static std::wstring GetTelemetryTypeName(TelemetryType type);
    
    /**
     * @brief Create standard application ETW provider
     */
    static GUID CreateApplicationETWProvider();
    
    /**
     * @brief Create standard security ETW provider
     */
    static GUID CreateSecurityETWProvider();

private:
    PrivacyManager();
    ~PrivacyManager();
    
    // Prevent copy/move
    PrivacyManager(const PrivacyManager&) = delete;
    PrivacyManager& operator=(const PrivacyManager&) = delete;
    
    /**
     * @brief Initialize PII detection patterns
     */
    void InitializePIIPatterns();
    
    /**
     * @brief Initialize default ETW providers
     */
    bool InitializeDefaultETWProviders();
    
    /**
     * @brief Notify privacy change callbacks
     */
    void NotifyPrivacyChange(PrivacyLevel oldLevel, PrivacyLevel newLevel);
    
    /**
     * @brief Apply privacy level to ETW providers
     */
    void ApplyPrivacyLevelToETW();
    
    /**
     * @brief Clean up expired events
     */
    void CleanupExpiredEvents();
    
    /**
     * @brief Validate event against privacy settings
     */
    bool ValidateEvent(const TelemetryEvent& event) const;
    
    /**
     * @brief Log privacy manager events
     */
    void LogEvent(const std::wstring& message, LogLevel level = LogLevel::Info) const;
};

/**
 * @brief Privacy-aware ETW event writer
 */
class PrivacyETWWriter {
private:
    std::string providerName_;
    PrivacyManager& privacyManager_;

public:
    explicit PrivacyETWWriter(const std::string& providerName);
    
    /**
     * @brief Write application event
     */
    bool WriteApplicationEvent(const std::string& eventName, 
                             const std::string& eventData,
                             TelemetryType type = TelemetryType::ERROR_REPORTING);
    
    /**
     * @brief Write security event
     */
    bool WriteSecurityEvent(const std::string& eventName,
                          const std::string& eventData);
    
    /**
     * @brief Write performance event
     */
    bool WritePerformanceEvent(const std::string& eventName,
                             const std::string& metrics);
    
    /**
     * @brief Write user interaction event
     */
    bool WriteUserInteractionEvent(const std::string& eventName,
                                 const std::string& interactionData);
};

/**
 * @brief RAII class for privacy-aware telemetry sessions
 */
class TelemetrySession {
private:
    std::string sessionName_;
    PrivacyManager& privacyManager_;
    bool sessionActive_;
    SYSTEMTIME sessionStart_;

public:
    explicit TelemetrySession(const std::string& sessionName);
    ~TelemetrySession();
    
    /**
     * @brief Log event in this session
     */
    bool LogEvent(const TelemetryEvent& event);
    
    /**
     * @brief End session explicitly
     */
    void EndSession();
    
    /**
     * @brief Check if session is active
     */
    bool IsActive() const { return sessionActive_; }
};

// Convenience macros for privacy-aware logging
#define PRIVACY_LOG_EVENT(category, type, name, data) \
    do { \
        if (PrivacyManager::GetInstance().IsEventTypeAllowed(type)) { \
            TelemetryEvent event; \
            event.category = category; \
            event.type = type; \
            event.eventName = name; \
            event.eventData = data; \
            PrivacyManager::GetInstance().LogTelemetryEvent(event); \
        } \
    } while(0)

#define PRIVACY_LOG_ERROR(name, data) \
    PRIVACY_LOG_EVENT(EventCategory::APPLICATION, TelemetryType::ERROR_REPORTING, name, data)

#define PRIVACY_LOG_PERFORMANCE(name, data) \
    PRIVACY_LOG_EVENT(EventCategory::PERFORMANCE, TelemetryType::PERFORMANCE_METRICS, name, data)

#define PRIVACY_LOG_USAGE(name, data) \
    PRIVACY_LOG_EVENT(EventCategory::USER_INTERACTION, TelemetryType::USAGE_ANALYTICS, name, data)

#define PRIVACY_LOG_SECURITY(name, data) \
    PRIVACY_LOG_EVENT(EventCategory::SECURITY, TelemetryType::SECURITY_EVENTS, name, data)

} // namespace Core
} // namespace RainmeterManager

#endif // PRIVACY_MANAGER_H
