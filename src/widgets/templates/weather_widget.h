#ifndef WEATHER_WIDGET_H
#define WEATHER_WIDGET_H

#include "../../widget_framework.h"
#include <json/json.h>
#include <wininet.h>

/**
 * WeatherRadarWidget - Template for weather radar and globe animations
 * 
 * This widget demonstrates integration with weather services like:
 * - OpenWeatherMap radar layers
 * - Weather.gov radar feeds
 * - Windy.com animated maps
 * - AccuWeather radar overlays
 * 
 * Features:
 * - Animated radar loops
 * - Globe/satellite views
 * - Multiple weather data sources
 * - Customizable overlays (temperature, precipitation, wind)
 * - Fallback to static weather data
 */
class WeatherRadarWidget : public StreamingWidget {
public:
    enum class WeatherDataType {
        RADAR_PRECIPITATION,
        RADAR_CLOUDS,
        SATELLITE_INFRARED,
        SATELLITE_VISIBLE,
        TEMPERATURE_MAP,
        WIND_OVERLAY,
        PRESSURE_MAP,
        LIGHTNING_STRIKES
    };

    enum class WeatherProvider {
        OPENWEATHERMAP,
        WEATHER_GOV,
        WINDY_COM,
        ACCUWEATHER,
        WEATHER_UNDERGROUND,
        CUSTOM_API
    };

    struct WeatherConfig {
        WeatherProvider provider = WeatherProvider::OPENWEATHERMAP;
        WeatherDataType dataType = WeatherDataType::RADAR_PRECIPITATION;
        std::string apiKey;
        std::string location; // lat,lon or city name
        int animationFrames = 10;
        int frameDelayMs = 500;
        bool showTimestamp = true;
        bool enableZoom = true;
        double zoomLevel = 1.0;
        std::pair<double, double> centerCoords = {0.0, 0.0};
        std::string customEndpoint;
        std::map<std::string, std::string> overlayLayers;
    };

    WeatherRadarWidget(const std::string& id, const WeatherConfig& config);
    ~WeatherRadarWidget() = default;

    // Configuration
    void setWeatherConfig(const WeatherConfig& config);
    void setLocation(const std::string& location);
    void setDataType(WeatherDataType type);
    void setProvider(WeatherProvider provider);
    void enableOverlay(const std::string& layerName, bool enable);

    // Animation controls
    void setAnimationSpeed(int frameDelayMs);
    void setLoopCount(int loops); // -1 for infinite
    void pauseAnimation();
    void resumeAnimation();
    void resetAnimation();

    // Zoom and pan controls
    void setZoomLevel(double zoom);
    void panTo(double lat, double lon);
    void centerOnLocation(const std::string& location);

    // Data refresh
    void refreshWeatherData();
    std::chrono::system_clock::time_point getLastDataUpdate() const;

    // Plugin extension points
    static bool registerWeatherProvider(WeatherProvider provider, 
        std::function<std::vector<std::string>(const WeatherConfig&)> urlGenerator,
        std::function<bool(const std::vector<uint8_t>&, WeatherDataType)> dataParser);

    static std::vector<WeatherProvider> getAvailableProviders();

protected:
    // BaseWidget overrides
    bool initialize() override;
    void refresh() override;
    bool showFallback() override;

    // StreamingWidget overrides  
    void processStreamData(const std::vector<uint8_t>& data) override;

private:
    WeatherConfig weatherConfig;
    std::vector<std::vector<uint8_t>> animationFrames;
    int currentFrame = 0;
    std::chrono::system_clock::time_point lastDataUpdate;
    std::thread animationThread;
    std::atomic<bool> animationActive{false};
    std::atomic<bool> animationPaused{false};

    // Provider-specific URL generators
    static std::map<WeatherProvider, std::function<std::vector<std::string>(const WeatherConfig&)>> urlGenerators;
    static std::map<WeatherProvider, std::function<bool(const std::vector<uint8_t>&, WeatherDataType)>> dataParsers;

    // Helper methods
    std::vector<std::string> generateUrls();
    bool downloadWeatherFrames();
    void renderCurrentFrame();
    void animationLoop();
    void parseWeatherData(const std::vector<uint8_t>& data);
    std::string buildApiUrl();
    Json::Value parseWeatherJson(const std::vector<uint8_t>& data);
    bool validateWeatherData(const Json::Value& data);
    std::string generateFallbackWeatherContent();

    // Provider implementations
    std::vector<std::string> generateOpenWeatherMapUrls();
    std::vector<std::string> generateWeatherGovUrls();
    std::vector<std::string> generateWindyUrls();
    std::vector<std::string> generateAccuWeatherUrls();
    
    bool parseOpenWeatherMapData(const std::vector<uint8_t>& data);
    bool parseWeatherGovData(const std::vector<uint8_t>& data);
    bool parseWindyData(const std::vector<uint8_t>& data);
    bool parseAccuWeatherData(const std::vector<uint8_t>& data);
};

// Utility functions for weather widget creation
namespace WeatherWidgetUtils {
    // Configuration builders for popular weather services
    WeatherRadarWidget::WeatherConfig createOpenWeatherMapConfig(
        const std::string& apiKey, 
        const std::string& location,
        WeatherRadarWidget::WeatherDataType dataType = WeatherRadarWidget::WeatherDataType::RADAR_PRECIPITATION);

    WeatherRadarWidget::WeatherConfig createWeatherGovConfig(
        const std::string& location,
        WeatherRadarWidget::WeatherDataType dataType = WeatherRadarWidget::WeatherDataType::RADAR_PRECIPITATION);

    WeatherRadarWidget::WeatherConfig createWindyConfig(
        const std::string& location,
        WeatherRadarWidget::WeatherDataType dataType = WeatherRadarWidget::WeatherDataType::RADAR_PRECIPITATION);

    // Location utilities
    std::pair<double, double> geocodeLocation(const std::string& location);
    std::string reverseGeocode(double lat, double lon);
    bool isValidCoordinates(double lat, double lon);

    // Weather data utilities
    std::string formatWeatherTimestamp(const std::chrono::system_clock::time_point& time);
    std::string weatherTypeToString(WeatherRadarWidget::WeatherDataType type);
    std::string providerToString(WeatherRadarWidget::WeatherProvider provider);

    // Sample configurations
    WeatherRadarWidget::WeatherConfig createUSWeatherRadarConfig(const std::string& zipCode);
    WeatherRadarWidget::WeatherConfig createEuropeanWeatherConfig(const std::string& city);
    WeatherRadarWidget::WeatherConfig createGlobalSatelliteConfig();
}

// Plugin registration macros for community extensions
#define REGISTER_WEATHER_PROVIDER(provider_name, url_gen_func, parser_func) \
    static bool _weather_provider_##provider_name##_registered = \
        WeatherRadarWidget::registerWeatherProvider( \
            WeatherRadarWidget::WeatherProvider::CUSTOM_API, \
            url_gen_func, \
            parser_func)

// Example custom provider registration:
/*
REGISTER_WEATHER_PROVIDER(MyCustomWeather,
    [](const WeatherRadarWidget::WeatherConfig& config) -> std::vector<std::string> {
        return {"https://api.mycustomweather.com/radar?loc=" + config.location};
    },
    [](const std::vector<uint8_t>& data, WeatherRadarWidget::WeatherDataType type) -> bool {
        // Custom parsing logic
        return true;
    }
);
*/

#endif // WEATHER_WIDGET_H
