#ifndef TICKER_WIDGET_H
#define TICKER_WIDGET_H

#include "../../widget_framework.h"
#include <json/json.h>
#include <deque>

/**
 * TickerWidget - Template for stock/sports/news tickers
 * 
 * This widget demonstrates integration with ticker feeds such as:
 * - Stock market data (Yahoo Finance, Alpha Vantage, IEX Cloud)
 * - Sports scores and updates (ESPN, The Sports DB)
 * - News headlines (RSS feeds, news APIs)
 * - Cryptocurrency prices (CoinGecko, CoinMarketCap)
 * 
 * Features:
 * - Scrolling ticker display
 * - Real-time price/score updates
 * - Multiple data sources
 * - Customizable refresh intervals
 * - Alert thresholds
 * - Fallback to cached data
 */
class TickerWidget : public StreamingWidget {
public:
    enum class TickerType {
        STOCK_PRICES,
        SPORTS_SCORES,
        NEWS_HEADLINES,
        CRYPTOCURRENCY,
        WEATHER_ALERTS,
        CUSTOM_FEED
    };

    enum class DataProvider {
        YAHOO_FINANCE,
        ALPHA_VANTAGE,
        IEX_CLOUD,
        ESPN_API,
        SPORTS_DB,
        RSS_FEED,
        COINGECKO,
        COINMARKETCAP,
        CUSTOM_API
    };

    struct TickerItem {
        std::string symbol;
        std::string displayName;
        std::string currentValue;
        std::string changeValue;
        std::string changePercent;
        std::string timestamp;
        std::map<std::string, std::string> additionalData;
        bool alertTriggered = false;
    };

    struct AlertThreshold {
        std::string symbol;
        double upperLimit = 0.0;
        double lowerLimit = 0.0;
        bool percentageBased = false;
        std::function<void(const TickerItem&)> callback;
    };

    struct TickerConfig {
        TickerType type = TickerType::STOCK_PRICES;
        DataProvider provider = DataProvider::YAHOO_FINANCE;
        std::vector<std::string> symbols;
        std::string apiKey;
        int refreshIntervalMs = 30000;
        int scrollSpeedMs = 50;
        bool enableAlerts = false;
        std::vector<AlertThreshold> alertThresholds;
        std::string customEndpoint;
        std::map<std::string, std::string> customHeaders;
        bool showTimestamp = true;
        bool showChangeIndicators = true;
        std::string dateFormat = "%H:%M:%S";
    };

    TickerWidget(const std::string& id, const TickerConfig& config);
    ~TickerWidget() = default;

    // Configuration
    void setTickerConfig(const TickerConfig& config);
    void addSymbol(const std::string& symbol);
    void removeSymbol(const std::string& symbol);
    void setProvider(DataProvider provider);
    void setRefreshInterval(int intervalMs);

    // Alert management
    void addAlertThreshold(const AlertThreshold& threshold);
    void removeAlertThreshold(const std::string& symbol);
    void clearAllAlerts();
    std::vector<AlertThreshold> getActiveAlerts() const;

    // Display controls
    void setScrollSpeed(int speedMs);
    void pauseScrolling();
    void resumeScrolling();
    void resetScrollPosition();

    // Data access
    std::vector<TickerItem> getCurrentData() const;
    TickerItem getSymbolData(const std::string& symbol) const;
    std::chrono::system_clock::time_point getLastUpdate() const;

    // Plugin extension points
    static bool registerTickerProvider(DataProvider provider,
        std::function<std::string(const TickerConfig&)> urlGenerator,
        std::function<std::vector<TickerItem>(const std::vector<uint8_t>&, TickerType)> dataParser);

    static std::vector<DataProvider> getAvailableProviders();

protected:
    // BaseWidget overrides
    bool initialize() override; 
    void refresh() override;
    bool showFallback() override;

    // StreamingWidget overrides
    void processStreamData(const std::vector<uint8_t>& data) override;

private:
    TickerConfig tickerConfig;
    std::deque<TickerItem> tickerItems;
    std::vector<TickerItem> cachedData;
    std::chrono::system_clock::time_point lastUpdate;
    std::thread scrollThread;
    std::thread alertThread;
    std::atomic<bool> scrollingActive{false};
    std::atomic<bool> scrollingPaused{false};
    int scrollOffset = 0;
    std::mutex dataMutex;

    // Provider-specific implementations
    static std::map<DataProvider, std::function<std::string(const TickerConfig&)>> urlGenerators;
    static std::map<DataProvider, std::function<std::vector<TickerItem>(const std::vector<uint8_t>&, TickerType)>> dataParsers;

    // Helper methods
    std::string generateApiUrl();
    void parseTickerData(const std::vector<uint8_t>& data);
    void updateTickerDisplay();
    void scrollTickerLoop();
    void monitorAlerts();
    void triggerAlert(const AlertThreshold& threshold, const TickerItem& item);
    std::string formatTickerItem(const TickerItem& item);
    std::string generateFallbackTickerContent();
    bool validateTickerData(const Json::Value& data);

    // Provider implementations
    std::string generateYahooFinanceUrl();
    std::string generateAlphaVantageUrl();
    std::string generateIEXCloudUrl();
    std::string generateESPNUrl();
    std::string generateSportsDBUrl();
    std::string generateCoinGeckoUrl();
    
    std::vector<TickerItem> parseYahooFinanceData(const std::vector<uint8_t>& data);
    std::vector<TickerItem> parseAlphaVantageData(const std::vector<uint8_t>& data);
    std::vector<TickerItem> parseIEXCloudData(const std::vector<uint8_t>& data);
    std::vector<TickerItem> parseESPNData(const std::vector<uint8_t>& data);
    std::vector<TickerItem> parseSportsDBData(const std::vector<uint8_t>& data);
    std::vector<TickerItem> parseCoinGeckoData(const std::vector<uint8_t>& data);
    std::vector<TickerItem> parseRSSFeed(const std::vector<uint8_t>& data);
};

// Utility functions for ticker widget creation
namespace TickerWidgetUtils {
    // Configuration builders for popular providers
    TickerWidget::TickerConfig createStockTickerConfig(
        const std::vector<std::string>& symbols,
        const std::string& apiKey = "",
        TickerWidget::DataProvider provider = TickerWidget::DataProvider::YAHOO_FINANCE);

    TickerWidget::TickerConfig createSportsTickerConfig(
        const std::vector<std::string>& leagues,
        TickerWidget::DataProvider provider = TickerWidget::DataProvider::ESPN_API);

    TickerWidget::TickerConfig createCryptoTickerConfig(
        const std::vector<std::string>& cryptoSymbols,
        TickerWidget::DataProvider provider = TickerWidget::DataProvider::COINGECKO);

    TickerWidget::TickerConfig createNewsTickerConfig(
        const std::vector<std::string>& rssFeedUrls);

    // Symbol validation and formatting
    bool isValidStockSymbol(const std::string& symbol);
    bool isValidCryptoSymbol(const std::string& symbol);
    std::string formatSymbolForProvider(const std::string& symbol, TickerWidget::DataProvider provider);

    // Alert utilities
    TickerWidget::AlertThreshold createPriceAlert(const std::string& symbol, double threshold, bool isUpper = true);
    TickerWidget::AlertThreshold createPercentageAlert(const std::string& symbol, double percentThreshold, bool isUpper = true);

    // Display formatting
    std::string formatPrice(double price, const std::string& currency = "$");
    std::string formatPercentChange(double change);
    std::string formatTimestamp(const std::chrono::system_clock::time_point& time);

    // Sample configurations
    TickerWidget::TickerConfig createDowJonesConfig();
    TickerWidget::TickerConfig createNFLScoresConfig();
    TickerWidget::TickerConfig createTop10CryptoConfig();
    TickerWidget::TickerConfig createTechNewsConfig();
}

// Plugin registration macros for community extensions
#define REGISTER_TICKER_PROVIDER(provider_name, url_gen_func, parser_func) \
    static bool _ticker_provider_##provider_name##_registered = \
        TickerWidget::registerTickerProvider( \
            TickerWidget::DataProvider::CUSTOM_API, \
            url_gen_func, \
            parser_func)

// Example custom provider registration:
/*
REGISTER_TICKER_PROVIDER(MyCustomProvider,
    [](const TickerWidget::TickerConfig& config) -> std::string {
        return "https://api.mycustomprovider.com/data?symbols=" + 
               std::accumulate(config.symbols.begin(), config.symbols.end(), std::string{},
                   [](const std::string& a, const std::string& b) {
                       return a.empty() ? b : a + "," + b;
                   });
    },
    [](const std::vector<uint8_t>& data, TickerWidget::TickerType type) -> std::vector<TickerWidget::TickerItem> {
        // Custom parsing logic
        std::vector<TickerWidget::TickerItem> items;
        // ... parse data and populate items
        return items;
    }
);
*/

#endif // TICKER_WIDGET_H
