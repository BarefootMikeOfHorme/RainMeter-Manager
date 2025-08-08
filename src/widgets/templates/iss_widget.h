#ifndef ISS_WIDGET_H
#define ISS_WIDGET_H

#include "../../widget_framework.h"

/**
 * ISSLiveFeedWidget - Template for live ISS (International Space Station) space feeds
 * 
 * This widget demonstrates integration with space feeds such as:
 * - NASA's International Space Station live video feed
 * - The European Space Agency (ESA) live views
 * - ISS HD Earth Viewing Experiment
 * - HDEV Camera feeds
 * 
 * Features:
 * - High-definition video streams
 * - ISS locator map
 * - Astronomical data overlay
 * - Fallback to recorded ISS missions
 */
class ISSLiveFeedWidget : public StreamingWidget {
public:
    struct ISSFeedConfig {
        std::string videoStreamUrl;
        std::string backupStreamUrl;
        bool showLocationMap = true;
        std::string locationApiUrl;
        bool enableAstronomyOverlay = true;
        std::map<std::string, std::string> overlayOptions;
    };

    ISSLiveFeedWidget(const std::string& id, const ISSFeedConfig& config);
    ~ISSLiveFeedWidget() = default;

    // Configuration
    void setISSFeedConfig(const ISSFeedConfig& config);
    void enableLocationMap(bool enable);
    void showAstronomyOverlay(bool show);

    // Stream controls
    void switchToBackupStream();

    // Refresh and synchronization
    void refreshISSData();
    std::chrono::system_clock::time_point getLastPositionUpdate() const;

    // Plugin extension points
    static bool registerCustomStreamHandler(const std::string& description, const std::string& urlTemplate);

protected:
    // BaseWidget overrides
    bool initialize() override;
    void refresh() override;
    bool showFallback() override;

    // StreamingWidget overrides
    void processStreamData(const std::vector<uint8_t>& data) override;

private:
    ISSFeedConfig feedConfig;
    std::vector<uint8_t> videoBuffer;
    std::thread updateThread;
    std::mutex configMutex;
    std::chrono::system_clock::time_point lastPositionUpdate;

    // Network and data handling
    bool fetchISSLocation();
    bool updateAstronomyOverlay();
    std::string buildLocationMapUrl();

    // Helper methods
    void renderISSVideo();
    void handleServerError();
    std::string generateFallbackContent();

    // Provider implementations
    std::string generateNASAStreamUrl();
    std::string generateESAStreamUrl();
};

// Utility functions for ISS widget creation
namespace ISSWidgetUtils {
    // Configuration builders
    ISSLiveFeedWidget::ISSFeedConfig createDefaultNASAConfig();
    ISSLiveFeedWidget::ISSFeedConfig createESAConfig();

    // Location utilities
    std::pair<double, double> getISSCurrentLocation();
    std::string getAstronomicalEvents();
    std::string formatLocationMap(const std::pair<double, double>& coordinates);
}

// Plugin registration macros for community extensions
#define REGISTER_ISS_STREAM_HANDLER(description, url_template) \
    static bool _iss_stream_handler_##description##_registered = \
        ISSLiveFeedWidget::registerCustomStreamHandler(description, url_template);

#endif // ISS_WIDGET_H

