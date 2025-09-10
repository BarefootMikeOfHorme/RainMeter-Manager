#ifndef COMMUNITY_FEEDBACK_H
#define COMMUNITY_FEEDBACK_H

#include "../../core/feature_flags.h"

#if RM_ENABLE_COMMUNITY_WIDGETS

#include "ui_framework.h"
#include "security.h"
#include "error_handling.h"
#include <Windows.h>
#include <winhttp.h>
#include <json/json.h>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <chrono>
#include <mutex>
#include <thread>
#include <queue>

#pragma comment(lib, "winhttp.lib")

// Forward declarations
class FeedbackSubmissionDialog;
class CommunityFeedManager;
class SourceRecommendationWidget;

// Source recommendation types
enum class SourceType {
    RSS_FEED,
    JSON_API,
    XML_FEED,
    WEBSOCKET_STREAM,
    IMAGE_FEED,
    VIDEO_STREAM,
    WEATHER_API,
    NEWS_API,
    FINANCIAL_API,
    SOCIAL_MEDIA,
    SPORTS_API,
    CUSTOM_API
};

// Source recommendation priority levels
enum class RecommendationPriority {
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    CRITICAL = 4
};

// Source validation status
enum class ValidationStatus {
    PENDING,
    VALIDATING,
    VALID,
    INVALID,
    RATE_LIMITED,
    BLOCKED,
    REQUIRES_AUTH
};

// Community feedback types
enum class FeedbackType {
    SOURCE_RECOMMENDATION,
    SOURCE_ISSUE_REPORT,
    SOURCE_UPDATE_REQUEST,
    FEATURE_REQUEST,
    BUG_REPORT,
    QUALITY_RATING
};

// Source recommendation structure
struct SourceRecommendation {
    std::string id;
    std::string name;
    std::string title;  // Display title
    std::string description;
    std::string url;
    SourceType type;
    std::string category;
    RecommendationPriority priority;
    ValidationStatus status;
    
    // Metadata
    std::string submitterInfo;
    std::string submittedBy;  // Username/ID of submitter
    std::chrono::system_clock::time_point submissionTime;
    std::vector<std::string> tags;
    std::string sampleData;
    
    // Validation results
    bool isAccessible = false;
    bool isValidated = false;  // Whether recommendation has been validated
    bool requiresApiKey = false;
    bool isRateLimited = false;
    std::string validationErrors;
    int upvotes = 0;
    int downvotes = 0;
    double qualityScore = 0.0;  // Quality score (0.0-1.0)
    
    // Technical details
    std::map<std::string, std::string> headers;
    std::string authMethod;
    int updateFrequency = 60000; // milliseconds
    int refreshRate = 60000;     // alias for updateFrequency
    std::string dataFormat;
    std::vector<std::string> dependencies;
    
    enum class DataType {
        RSS_FEED,
        JSON_API,
        XML_FEED,
        VIDEO_STREAM,
        AUDIO_STREAM,
        IMAGE_FEED,
        WEB_EMBED,
        WEBSOCKET_STREAM,
        CUSTOM
    } dataType = DataType::RSS_FEED;
};

// Community feedback structure
struct CommunityFeedback {
    std::string id;
    FeedbackType type;
    std::string title;
    std::string description;
    std::string contactInfo;
    RecommendationPriority priority;
    
    // Associated source (if applicable)
    std::string sourceId;
    SourceRecommendation* relatedSource = nullptr;
    
    // Metadata
    std::chrono::system_clock::time_point submissionTime;
    std::vector<std::string> attachments;
    std::map<std::string, std::string> systemInfo;
    
    // Community interaction
    int helpfulVotes = 0;
    std::vector<std::string> comments;
    std::string moderatorNotes;
    bool isResolved = false;
};

// Source curation information
struct SourceCuration {
    std::string sourceId;
    std::string curatorId;
    std::chrono::system_clock::time_point lastUpdated;
    std::string updateNotes;
    bool isActive = true;
    bool isRecommended = false;
    int qualityScore = 0; // 0-100
    std::vector<std::string> knownIssues;
    std::string maintenanceNotes;
};

// Feed list management
class CommunityFeedManager {
public:
    static CommunityFeedManager& getInstance();
    
    // Source recommendation management
    bool submitSourceRecommendation(const SourceRecommendation& recommendation);
    std::vector<SourceRecommendation> getRecommendations(SourceType type = SourceType::RSS_FEED);
    std::vector<SourceRecommendation> getRecommendationsByCategory(const std::string& category);
    bool validateRecommendation(const std::string& recommendationId);
    bool approveRecommendation(const std::string& recommendationId);
    bool rejectRecommendation(const std::string& recommendationId, const std::string& reason);
    
    // Community feedback management
    bool submitFeedback(const CommunityFeedback& feedback);
    std::vector<CommunityFeedback> getFeedback(FeedbackType type = FeedbackType::SOURCE_RECOMMENDATION);
    bool markFeedbackResolved(const std::string& feedbackId, const std::string& resolution);
    bool voteFeedback(const std::string& feedbackId, bool isHelpful);
    
    // Curation system
    bool registerCurator(const std::string& curatorId, const std::string& expertise);
    bool assignCurator(const std::string& sourceId, const std::string& curatorId);
    std::vector<SourceCuration> getCuratedSources();
    bool updateSourceCuration(const std::string& sourceId, const SourceCuration& curation);
    
    // Feed list management
    bool addToFeedList(const std::string& sourceId);
    bool removeFromFeedList(const std::string& sourceId);
    std::vector<std::string> getActiveFeedList();
    bool updateFeedList(const std::vector<std::string>& newFeedList);
    
    // Source validation
    ValidationStatus performSourceValidation(const SourceRecommendation& source);
    bool testSourceConnectivity(const std::string& url);
    bool validateDataFormat(const std::string& url, const std::string& expectedFormat);
    std::string fetchSampleData(const std::string& url, const std::map<std::string, std::string>& headers);
    
    // Community statistics
    struct CommunityStats {
        int totalRecommendations = 0;
        int pendingValidations = 0;
        int activeSources = 0;
        int totalFeedback = 0;
        int resolvedIssues = 0;
        int activeCurators = 0;
    };
    CommunityStats getCommunityStats();
    
    // Data persistence
    bool saveRecommendations(const std::string& filename);
    bool loadRecommendations(const std::string& filename);
    bool exportFeedList(const std::string& filename, const std::string& format = "json");
    bool importFeedList(const std::string& filename);
    
    // Event callbacks
    void setOnRecommendationSubmitted(std::function<void(const SourceRecommendation&)> callback);
    void setOnFeedbackSubmitted(std::function<void(const CommunityFeedback&)> callback);
    void setOnSourceValidated(std::function<void(const std::string&, ValidationStatus)> callback);
    void setOnFeedListUpdated(std::function<void(const std::vector<std::string>&)> callback);
    
    // Additional methods for integration
    bool initialize();
    std::vector<SourceRecommendation> getValidatedRecommendations();
    bool incrementRecommendationUsage(const std::string& recommendationId);
    std::map<std::string, int> getRecommendationUsageStats();
    
private:
    CommunityFeedManager() = default;
    static CommunityFeedManager* instance;
    
    std::vector<SourceRecommendation> recommendations;
    std::vector<CommunityFeedback> feedbackList;
    std::vector<SourceCuration> curations;
    std::vector<std::string> activeFeedList;
    std::map<std::string, std::string> curators; // curatorId -> expertise
    
    std::mutex recommendationsMutex;
    std::mutex feedbackMutex;
    std::mutex curationMutex;
    
    // Background validation thread
    std::thread validationThread;
    std::queue<std::string> validationQueue;
    std::mutex validationMutex;
    std::condition_variable validationCondition;
    std::atomic<bool> validationActive{false};
    
    // Event callbacks
    std::function<void(const SourceRecommendation&)> onRecommendationSubmitted;
    std::function<void(const CommunityFeedback&)> onFeedbackSubmitted;
    std::function<void(const std::string&, ValidationStatus)> onSourceValidated;
    std::function<void(const std::vector<std::string>&)> onFeedListUpdated;
    
    // Helper methods
    std::string generateId();
    Json::Value serializeRecommendation(const SourceRecommendation& rec);
    SourceRecommendation deserializeRecommendation(const Json::Value& json);
    Json::Value serializeFeedback(const CommunityFeedback& feedback);
    CommunityFeedback deserializeFeedback(const Json::Value& json);
    
    // Validation worker
    void validationWorker();
    ValidationStatus validateUrl(const std::string& url);
    bool checkRateLimit(const std::string& url);
    std::string detectDataFormat(const std::vector<uint8_t>& data);
};

// Feedback submission dialog
class FeedbackSubmissionDialog : public UIComponent {
public:
    FeedbackSubmissionDialog();
    ~FeedbackSubmissionDialog();
    
    // UIComponent overrides
    bool create(HWND parent, int x, int y, int width, int height) override;
    void setTheme(const UITheme& theme) override;
    void show(bool visible = true) override;
    HWND getHandle() const override { return hwnd; }
    
    // Dialog-specific methods
    bool showSubmissionDialog(FeedbackType type = FeedbackType::SOURCE_RECOMMENDATION);
    bool showModal();  // For modal dialog display
    void setPrefilledData(const std::string& sourceUrl, const std::string& sourceName);
    void setSourceUrl(const std::string& url);
    void setFeedbackType(FeedbackType type);
    void setOnSubmissionComplete(std::function<void(bool)> callback);
    
private:
    // UI controls
    std::shared_ptr<ModernTextEditor> titleEdit;
    std::shared_ptr<ModernTextEditor> descriptionEdit;
    std::shared_ptr<ModernTextEditor> urlEdit;
    std::shared_ptr<ModernTextEditor> contactEdit;
    std::shared_ptr<ModernListView> typeCombo;
    std::shared_ptr<ModernListView> priorityCombo;
    std::shared_ptr<ModernListView> categoryCombo;
    std::shared_ptr<ModernButton> submitButton;
    std::shared_ptr<ModernButton> cancelButton;
    std::shared_ptr<ModernButton> testUrlButton;
    std::shared_ptr<ModernProgressBar> validationProgress;
    
    // State management
    FeedbackType currentType;
    bool isValidating = false;
    std::function<void(bool)> submissionCallback;
    
    // Helper methods
    void setupControls();
    void setupLayout();
    void populateComboBoxes();
    void onSubmitClicked();
    void onCancelClicked();
    void onTestUrlClicked();
    void onTypeChanged();
    void validateInput();
    bool submitRecommendation();
    bool submitFeedback();
    
    // Validation
    void performUrlValidation(const std::string& url);
    std::thread validationThread;
    
    static LRESULT CALLBACK dialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

// Source recommendation widget for dashboard
class SourceRecommendationWidget : public BaseWidget {
public:
    SourceRecommendationWidget();
    SourceRecommendationWidget(const std::string& widgetId);
    ~SourceRecommendationWidget();
    
    // BaseWidget overrides
    bool initialize() override;
    bool start() override;
    bool stop() override;
    bool pause() override;
    bool resume() override;
    void refresh() override;
    bool showFallback() override;
    void setFallbackContent(const std::string& content) override;
    
    // UIComponent overrides
    bool create(HWND parent, int x, int y, int width, int height) override;
    void setTheme(const UITheme& theme) override;
    void show(bool visible = true) override;
    HWND getHandle() const override { return hwnd; }
    
    // Widget-specific methods
    void refreshRecommendations();
    void setFilterCategory(const std::string& category);
    void setFilterType(SourceType type);
    void setOnRecommendationSelected(std::function<void(const SourceRecommendation&)> callback);
    void setOnAddToFeedList(std::function<void(const std::string&)> callback);
    
private:
    // UI controls
    std::shared_ptr<ModernListView> recommendationsList;
    std::shared_ptr<ModernListView> categoryFilter;
    std::shared_ptr<ModernListView> typeFilter;
    std::shared_ptr<ModernButton> refreshButton;
    std::shared_ptr<ModernButton> submitFeedbackButton;
    std::shared_ptr<ModernButton> addToFeedButton;
    std::shared_ptr<ModernTextEditor> searchBox;
    
    // State
    std::vector<SourceRecommendation> displayedRecommendations;
    std::string currentCategory;
    SourceType currentType;
    SourceRecommendation* selectedRecommendation = nullptr;
    
    // Callbacks
    std::function<void(const SourceRecommendation&)> onRecommendationSelected;
    std::function<void(const std::string&)> onAddToFeedList;
    
    // Helper methods
    void setupControls();
    void setupLayout();
    void populateRecommendations();
    void filterRecommendations();
    void onSelectionChanged();
    void onRefreshClicked();
    void onSubmitFeedbackClicked();
    void onAddToFeedClicked();
    void onSearchChanged();
    
    static LRESULT CALLBACK widgetProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

// Utility functions for community feedback system
namespace CommunityUtils {
    // URL validation and parsing
    bool isValidUrl(const std::string& url);
    bool isRssUrl(const std::string& url);
    bool isApiUrl(const std::string& url);
    std::string extractDomain(const std::string& url);
    SourceType detectSourceType(const std::string& url);
    
    // Data format detection
    std::string detectMimeType(const std::vector<uint8_t>& data);
    bool isJsonData(const std::string& data);
    bool isXmlData(const std::string& data);
    bool isRssData(const std::string& data);
    
    // Quality scoring
    int calculateQualityScore(const SourceRecommendation& source);
    bool isHighQualitySource(const SourceRecommendation& source);
    std::vector<std::string> detectPotentialIssues(const SourceRecommendation& source);
    
    // Community moderation helpers
    bool isSpamRecommendation(const SourceRecommendation& source);
    bool isDuplicateRecommendation(const SourceRecommendation& source, const std::vector<SourceRecommendation>& existing);
    std::string sanitizeInput(const std::string& input);
    
    // Export/import utilities
    std::string exportRecommendationsToJson(const std::vector<SourceRecommendation>& recommendations);
    std::vector<SourceRecommendation> importRecommendationsFromJson(const std::string& json);
    std::string exportFeedListToOpml(const std::vector<std::string>& feedUrls);
    bool importFeedListFromOpml(const std::string& opml, std::vector<std::string>& feedUrls);
}

#endif // RM_ENABLE_COMMUNITY_WIDGETS

#else // RM_ENABLE_COMMUNITY_WIDGETS

// Community widgets are disabled at compile time. This header provides no-op
// placeholders to avoid accidental references.

#endif // RM_ENABLE_COMMUNITY_WIDGETS

#endif // COMMUNITY_FEEDBACK_H
