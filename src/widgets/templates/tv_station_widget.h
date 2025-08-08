#ifndef TV_STATION_WIDGET_H
#define TV_STATION_WIDGET_H

#include "../../widget_framework.h"

/**
 * TVStationEmbedWidget - Template for embedding free TV station streams
 * 
 * This widget demonstrates integration with TV station streams like:
 * - Free-to-air digital channels
 * - Public broadcasting service streams
 * - Local news stations
 * 
 * Features:
 * - Embedding live TV streams
 * - Channel switching
 * - EPG (Electronic Program Guide)
 * - Regional availability checks
 * - Customizable layouts for different screen sizes
 * - Fallback VOD content
 */
class TVStationEmbedWidget : public EmbedWidget {
public:
    struct TVStationConfig {
        std::string primaryStreamUrl;
        std::map<std::string, std::string> backupStreams; // Quality options
        std::string programGuideUrl;
        bool enableEPG = true;
        std::vector<std::string> availableChannels;
        std::string region;
        std::map<std::string, std::string> layoutOptions;
    };

    TVStationEmbedWidget(const std::string& id, const TVStationConfig& config);
    ~TVStationEmbedWidget() = default;

    // Configuration
    void setTVStationConfig(const TVStationConfig& config);
    void switchToChannel(const std::string& channelId);
    bool isChannelAvailable(const std::string& channelId) const;

    // EPG management
    void refreshEPG();
    std::pair<std::string, std::string> getCurrentProgramInfo() const;

    // Plugin extension points
    static bool registerCustomChannelHandler(const std::string& protocol, const std::string& streamUrlTemplate);

protected:
    // BaseWidget overrides
    bool initialize() override;
    void refresh() override;
    bool showFallback() override;

    // EmbedWidget overrides
    bool loadContent(const std::string& url) override; 
    void setFallbackContent(const std::string& content) override;

    // Event handlers
    void onChannelSwitch(const std::string& fromChannel, const std::string& toChannel);

private:
    TVStationConfig stationConfig;
    std::map<std::string, std::string> channelMap;
    std::mutex configMutex;

    // Network and data handling
    bool fetchEPGData();
    std::string buildChannelUrl(const std::string& channelId);

    // Helper methods
    void renderTVStream();
    std::string generateFallbackContent();

    // Provider implementations
    std::string generateEPGUrl();
};

// Utility functions for TV widget creation
namespace TVStationWidgetUtils {
    // Configuration builders
    TVStationEmbedWidget::TVStationConfig createDefaultConfig(const std::string& region);

    // Channel utilities
    bool isChannelSupported(const std::string& channelId);
    std::string formatProgramInfo(const std::pair<std::string, std::string>& program);
}

// Plugin registration macros for community extensions
#define REGISTER_TV_CHANNEL_HANDLER(protocol_name, stream_url_template) \
    static bool _tv_channel_handler_##protocol_name##_registered = \
        TVStationEmbedWidget::registerCustomChannelHandler(protocol_name, stream_url_template);

#endif // TV_STATION_WIDGET_H

