#ifndef WIDGET_FRAMEWORK_H
#define WIDGET_FRAMEWORK_H

#include "ui_framework.h"
#include "community_feedback.h"
#include "security.h"
#include "error_handling.h"
#include <Windows.h>
#include <skia/include/core/SkCanvas.h>
#include <skia/include/core/SkPaint.h>
#include <WebView2.h>
#include <wil/com.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <map>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

// Forward declarations
class StreamingWidget;
class EmbedWidget;
class WidgetManager;

// Widget types supported by the framework
enum class WidgetType {
    IFRAME_EMBED,           // Web-based iframe embeds
    WEBVIEW_EMBED,          // Native WebView2 embeds  
    VIDEO_STREAM,           // Direct video streams
    AUDIO_STREAM,           // Direct audio streams
    IMAGE_FEED,             // Image refresh feeds
    ANIMATED_OVERLAY,       // Animated data overlays
    CUSTOM_STREAM           // User-defined streams
};

// Widget state management
enum class WidgetState {
    INITIALIZING,
    LOADING,
    ACTIVE,
    PAUSED,
    ERROR,
    UNAVAILABLE,
    GEO_BLOCKED
};

// Stream configuration structure
struct StreamConfig {
    std::string url;
    std::string fallbackUrl;
    std::map<std::string, std::string> headers;
    int refreshIntervalMs = 1000;
    int timeoutMs = 30000;
    bool autoRetry = true;
    int maxRetries = 3;
    bool requiresAuth = false;
    std::string authToken;
    bool enableCaching = true;
    std::string proxyUrl;
    std::vector<std::string> allowedDomains;
};

// Embed configuration structure  
struct EmbedConfig {
    std::string embedUrl;
    std::string fallbackContent;
    bool allowScripts = false;
    bool allowPlugins = false;
    std::map<std::string, std::string> parameters;
    std::vector<std::string> allowedOrigins;
    bool enableSandbox = true;
    int width = 400;
    int height = 300;
    bool responsive = true;
};

// Widget status information
struct WidgetStatus {
    WidgetState state;
    std::string statusMessage;
    std::chrono::system_clock::time_point lastUpdate;
    std::string errorDetails;
    bool isGeoBlocked = false;
    bool isSourceAvailable = true;
    double loadProgress = 0.0;
    size_t bytesTransferred = 0;
    std::string mimeType;
};

// Base widget class - all widgets inherit from this
class BaseWidget : public UIComponent {
public:
    BaseWidget(WidgetType type, const std::string& id);
    virtual ~BaseWidget() = default;

    // Core widget interface
    virtual bool initialize() = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool pause() = 0;
    virtual bool resume() = 0;
    virtual void refresh() = 0;

    // Layout enhancements
    void toggleFullScreenMode();
    void toggleOverlayMode();
    void setPosition(int x, int y);
    void setSize(int width, int height);

    // UIComponent overrides
    bool create(HWND parent, int x, int y, int width, int height) override;
    void setTheme(const UITheme& theme) override;
    void show(bool visible = true) override;
    HWND getHandle() const override { return hwnd; }

    // Widget-specific methods
    WidgetType getType() const { return widgetType; }
    std::string getId() const { return widgetId; }
    WidgetStatus getStatus() const { return currentStatus; }
    
    // Event handlers
    void setOnStateChanged(std::function<void(WidgetState, WidgetState)> callback);
    void setOnError(std::function<void(const std::string&)> callback);
    void setOnDataReceived(std::function<void(const std::vector<uint8_t>&)> callback);
    
    // Fallback handling
    virtual bool showFallback() = 0;
    virtual void setFallbackContent(const std::string& content) = 0;
    
    // Security and validation
    bool validateSource(const std::string& url);
    bool isSourceAllowed(const std::string& domain);
    
protected:
    WidgetType widgetType;
    std::string widgetId;
    WidgetStatus currentStatus;
    UITheme widgetTheme;
    
    // Event callbacks
    std::function<void(WidgetState, WidgetState)> onStateChanged;
    std::function<void(const std::string&)> onError;
    std::function<void(const std::vector<uint8_t>&)> onDataReceived;
    
    // Helper methods
    void updateStatus(WidgetState newState, const std::string& message = "");
    void triggerError(const std::string& error);
    void logWidgetEvent(const std::string& event, const std::string& details = "");
    
    // Security validation
    std::vector<std::string> allowedDomains;
    bool performSecurityCheck(const std::string& url);
};

// WebView2-based embed widget for rich web content
class EmbedWidget : public BaseWidget {
public:
    EmbedWidget(const std::string& id, const EmbedConfig& config);
    ~EmbedWidget();

    // BaseWidget overrides
    bool initialize() override;
    bool start() override;
    bool stop() override;
    bool pause() override;
    bool resume() override;
    void refresh() override;
    bool showFallback() override;
    void setFallbackContent(const std::string& content) override;

    // Embed-specific methods
    bool loadContent(const std::string& url);
    bool loadHtml(const std::string& html);
    bool executeScript(const std::string& script);
    void setEmbedConfig(const EmbedConfig& config);
    
    // Navigation control
    bool canGoBack() const;
    bool canGoForward() const;
    void goBack();
    void goForward();
    void reload();
    
private:
    EmbedConfig embedConfig;
    std::string fallbackHtml;
    
    // WebView2 components
    wil::com_ptr<ICoreWebView2Environment> webViewEnvironment;
    wil::com_ptr<ICoreWebView2Controller> webViewController;
    wil::com_ptr<ICoreWebView2> webView;
    
    // WebView2 event handlers
    EventRegistrationToken navigationCompletedToken;
    EventRegistrationToken documentTitleChangedToken;
    EventRegistrationToken permissionRequestedToken;
    
    // Helper methods
    HRESULT createWebViewEnvironment();
    HRESULT createWebViewController();
    void setupWebViewEventHandlers();
    void applySandboxSettings();
    void handleNavigationCompleted(HRESULT result);
    void handlePermissionRequest(ICoreWebView2PermissionRequestedEventArgs* args);
};

// Streaming widget for real-time data feeds
class StreamingWidget : public BaseWidget {
public:
    StreamingWidget(const std::string& id, WidgetType streamType, const StreamConfig& config);
    ~StreamingWidget();

    // BaseWidget overrides  
    bool initialize() override;
    bool start() override;
    bool stop() override;
    bool pause() override;
    bool resume() override;
    void refresh() override;
    bool showFallback() override;
    void setFallbackContent(const std::string& content) override;

    // Streaming-specific methods
    void setStreamConfig(const StreamConfig& config);
    bool connectToStream();
    void disconnectFromStream();
    
    // Data handling
    void processStreamData(const std::vector<uint8_t>& data);
    void renderFrame(const std::vector<uint8_t>& frameData);
    void playAudioBuffer(const std::vector<uint8_t>& audioData);
    void updateImageFeed(const std::vector<uint8_t>& imageData);
    
    // Stream statistics
    struct StreamStats {
        size_t totalBytesReceived = 0;
        double averageBitrate = 0.0;
        int droppedFrames = 0;
        std::chrono::milliseconds latency{0};
        double fps = 0.0;
    };
    StreamStats getStreamStats() const { return streamStats; }
    
private:
    StreamConfig streamConfig;
    std::string fallbackContent;
    StreamStats streamStats;
    
    // Threading for stream processing
    std::thread streamThread;
    std::mutex streamMutex;
    std::condition_variable streamCondition;
    std::atomic<bool> streamActive{false};
    std::atomic<bool> shouldStop{false};
    
    // Stream buffer management
    std::queue<std::vector<uint8_t>> dataBuffer;
    std::mutex bufferMutex;
    size_t maxBufferSize = 1024 * 1024 * 10; // 10MB buffer
    
    // Network handling
    HINTERNET hSession = nullptr;
    HINTERNET hConnect = nullptr;
    HINTERNET hRequest = nullptr;
    
    // Helper methods
    void streamWorkerThread();
    bool establishConnection();
    void cleanupConnection();
    bool downloadData(std::vector<uint8_t>& data);
    void processDataBuffer();
    void updateStreamStats(size_t bytesReceived);
    bool handleGeoBlocking();
    bool checkSourceAvailability();
    void performRetryLogic();
    
    // Format-specific handlers
    void handleVideoStream(const std::vector<uint8_t>& data);
    void handleAudioStream(const std::vector<uint8_t>& data);
    void handleImageFeed(const std::vector<uint8_t>& data);
    void handleAnimatedOverlay(const std::vector<uint8_t>& data);
};

// Widget manager for coordinating multiple widgets
class WidgetManager {
public:
    static WidgetManager& getInstance();
    
    // Widget lifecycle management
    std::shared_ptr<BaseWidget> createWidget(WidgetType type, const std::string& id);
    std::shared_ptr<EmbedWidget> createEmbedWidget(const std::string& id, const EmbedConfig& config);
    std::shared_ptr<StreamingWidget> createStreamingWidget(const std::string& id, WidgetType streamType, const StreamConfig& config);
    
    bool registerWidget(std::shared_ptr<BaseWidget> widget);
    bool unregisterWidget(const std::string& id);
    std::shared_ptr<BaseWidget> getWidget(const std::string& id);
    std::vector<std::shared_ptr<BaseWidget>> getAllWidgets();
    
    // Batch operations
    void startAllWidgets();
    void stopAllWidgets();
    void pauseAllWidgets();
    void resumeAllWidgets();
    void refreshAllWidgets();
    
    // Layout management
    void setWidgetLayout(const std::string& id, int x, int y, int width, int height);
    void arrangeWidgetsGrid(int columns, int spacing = 10);
    void arrangeWidgetsStack(bool vertical = true, int spacing = 5);
    
    // Global settings
    void setGlobalTheme(const UITheme& theme);
    void setGlobalSecurityPolicy(const std::vector<std::string>& allowedDomains);
    void enableGlobalFallback(bool enable);
    
    // Monitoring and diagnostics
    std::map<std::string, WidgetStatus> getWidgetStatuses();
    void logWidgetMetrics();
    void performHealthCheck();
    
    // User-defined streams support
    bool registerCustomStreamHandler(const std::string& protocol, std::function<std::vector<uint8_t>(const std::string&)> handler);
    bool addUserDefinedStream(const std::string& id, const std::string& streamUrl, const StreamConfig& config);
    
    // Event aggregation
    void setOnWidgetStateChanged(std::function<void(const std::string&, WidgetState, WidgetState)> callback);
    void setOnWidgetError(std::function<void(const std::string&, const std::string&)> callback);
    
    // Community feedback integration
    bool createWidgetFromRecommendation(const SourceRecommendation& recommendation);
    void showFeedbackDialog(const std::string& sourceUrl = "", FeedbackType type = FeedbackType::SOURCE_RECOMMENDATION);
    std::shared_ptr<SourceRecommendationWidget> createRecommendationWidget();
    void updateFeedListFromCommunity();
    void enableCommunityFeatures(bool enable = true);
    
private:
    WidgetManager() = default;
    static WidgetManager* instance;
    
    std::map<std::string, std::shared_ptr<BaseWidget>> widgets;
    std::mutex widgetsMutex;
    
    UITheme globalTheme;
    std::vector<std::string> globalAllowedDomains;
    bool globalFallbackEnabled = true;
    
    // Custom stream handlers
    std::map<std::string, std::function<std::vector<uint8_t>(const std::string&)>> customStreamHandlers;
    
    // Global event callbacks
    std::function<void(const std::string&, WidgetState, WidgetState)> globalStateChangedCallback;
    std::function<void(const std::string&, const std::string&)> globalErrorCallback;
    
    // Helper methods
    void propagateThemeChange();
    void notifyStateChange(const std::string& widgetId, WidgetState oldState, WidgetState newState);
    void notifyError(const std::string& widgetId, const std::string& error);
};

// Utility functions for widget creation and configuration
namespace WidgetUtils {
    // Configuration builders
    EmbedConfig createYouTubeEmbedConfig(const std::string& videoId, int width = 560, int height = 315);
    EmbedConfig createTwitchEmbedConfig(const std::string& channel, int width = 400, int height = 300);
    StreamConfig createImageFeedConfig(const std::string& url, int refreshMs = 5000);
    StreamConfig createVideoStreamConfig(const std::string& url, bool enableCache = true);
    StreamConfig createAudioStreamConfig(const std::string& url, int bufferSize = 4096);

// Skia visual enhancement functions
    void drawWidgetBorder(SkCanvas* canvas, const UITheme& theme, const SkRect& rect);
    void drawRoundedRectangle(SkCanvas* canvas, const SkRect& rect, float radius, SkColor color);
    void drawAnimatedProgressBar(SkCanvas* canvas, const SkRect& rect, float progress, const UITheme& theme);
    
    // URL validation and parsing
    bool isValidStreamUrl(const std::string& url);
    bool isValidEmbedUrl(const std::string& url);
    std::string extractDomainFromUrl(const std::string& url);
    
    // Format detection
    WidgetType detectStreamType(const std::string& url);
    std::string detectMimeType(const std::vector<uint8_t>& data);
    
    // Fallback content generators
    std::string generateErrorFallback(const std::string& error, const UITheme& theme);
    std::string generateLoadingFallback(const UITheme& theme);
    std::string generateGeoBlockedFallback(const UITheme& theme);
}

#endif // WIDGET_FRAMEWORK_H
