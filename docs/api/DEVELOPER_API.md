# RainmeterManager Developer API Reference

## Overview

The RainmeterManager Developer API provides advanced programmatic access to the widget framework, allowing developers to create custom widgets, integrate new data sources, and build extensions with premium Skia-based visual effects.

## Core API Components

### Widget Manager API

The `WidgetManager` is the central hub for all widget operations:

```cpp
#include "widget_framework.h"

// Get the singleton instance
WidgetManager& manager = WidgetManager::getInstance();

// Create and register widgets
auto ticker = manager.createStreamingWidget("stock_ticker", 
    WidgetType::CUSTOM_STREAM, tickerConfig);
manager.registerWidget(ticker);

// Global operations
manager.startAllWidgets();
manager.setGlobalTheme(cyberpunkTheme);
```

### Widget Development API

#### Base Widget Class

All custom widgets inherit from `BaseWidget`:

```cpp
class MyCustomWidget : public BaseWidget {
public:
    MyCustomWidget(const std::string& id) : BaseWidget(WidgetType::CUSTOM_STREAM, id) {}
    
    // Required implementations
    bool initialize() override;
    bool start() override;
    bool stop() override;
    bool pause() override;
    bool resume() override;
    void refresh() override;
    bool showFallback() override;
    void setFallbackContent(const std::string& content) override;
    
    // Custom Skia rendering
    void render(SkCanvas* canvas) {
        // Premium visual effects using Skia
        renderGlassmorphismBackground(canvas);
        renderNeonBorders(canvas);
        renderDataVisualization(canvas);
    }
};
```

#### Skia Integration API

Premium visual effects using Skia graphics:

```cpp
#include <skia/include/core/SkCanvas.h>
#include <skia/include/effects/SkGradientShader.h>
#include <skia/include/effects/SkBlurMaskFilter.h>

class SkiaRenderer {
public:
    // Create premium gradients
    sk_sp<SkShader> createPremiumGradient(const std::vector<SkColor>& colors, 
                                         const SkPoint& start, 
                                         const SkPoint& end) {
        return SkGradientShader::MakeLinear(
            &start, colors.data(), nullptr, colors.size(), 
            SkTileMode::kClamp);
    }
    
    // Apply glassmorphism effect
    void applyGlassmorphism(SkPaint& paint, float blurRadius = 5.0f) {
        paint.setAlpha(30);
        paint.setMaskFilter(SkBlurMaskFilter::Make(kNormal_SkBlurStyle, blurRadius));
    }
    
    // Create neon glow effect
    void createNeonGlow(SkPaint& paint, SkColor color, float glowRadius = 8.0f) {
        paint.setColor(color);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setMaskFilter(SkBlurMaskFilter::Make(kNormal_SkBlurStyle, glowRadius));
    }
};
```

### Data Source Integration API

#### Custom Data Providers

Register custom data providers for widgets:

```cpp
// Stock ticker provider example
REGISTER_TICKER_PROVIDER(MyStockAPI,
    [](const TickerWidget::TickerConfig& config) -> std::string {
        return buildApiUrl(config);
    },
    [](const std::vector<uint8_t>& data, TickerWidget::TickerType type) {
        return parseStockData(data);
    }
);

// Weather data provider
REGISTER_WEATHER_PROVIDER(MyWeatherAPI,
    [](const WeatherRadarWidget::WeatherConfig& config) {
        return generateWeatherUrls(config);
    },
    [](const std::vector<uint8_t>& data, WeatherRadarWidget::WeatherDataType type) {
        return validateWeatherData(data);
    }
);
```

#### Stream Handler Registration

Create custom protocol handlers:

```cpp
manager.registerCustomStreamHandler("crypto://", 
    [](const std::string& url) -> std::vector<uint8_t> {
        // Parse custom URL: crypto://BTC,ETH,ADA
        auto symbols = parseSymbols(url);
        return fetchCryptoData(symbols);
    }
);

// Usage: crypto://BTC,ETH,ADA?refresh=30s
```

### Configuration API

#### Theme Management

```cpp
class ThemeManager {
public:
    // Register custom themes
    void registerTheme(const std::string& name, const UITheme& theme) {
        themes[name] = theme;
    }
    
    // Apply theme globally
    void applyTheme(const std::string& name) {
        if (themes.find(name) != themes.end()) {
            WidgetManager::getInstance().setGlobalTheme(themes[name]);
        }
    }
    
    // Create theme from JSON
    UITheme createThemeFromJson(const Json::Value& themeData) {
        UITheme theme;
        theme.primaryColor = parseColor(themeData["colors"]["primary"]);
        theme.backgroundColor = parseColor(themeData["colors"]["background"]);
        theme.effectsEnabled = themeData["effects"]["enabled"].asBool();
        theme.blurRadius = themeData["effects"]["blur_radius"].asFloat();
        return theme;
    }
};
```

#### Widget Templates

```cpp
class TemplateManager {
public:
    // Register widget templates
    void registerTemplate(const std::string& name, const WidgetTemplate& tmpl) {
        templates[name] = tmpl;
    }
    
    // Create widget from template
    std::shared_ptr<BaseWidget> createFromTemplate(const std::string& templateName,
                                                   const std::string& widgetId,
                                                   const Json::Value& config) {
        auto& tmpl = templates[templateName];
        return tmpl.createWidget(widgetId, config);
    }
};
```

### Event System API

#### Widget Events

```cpp
class EventManager {
public:
    // Widget lifecycle events
    void onWidgetCreated(std::function<void(const std::string&)> callback);
    void onWidgetDestroyed(std::function<void(const std::string&)> callback);
    void onWidgetStateChanged(std::function<void(const std::string&, 
                                                WidgetState, WidgetState)> callback);
    
    // Data events
    void onDataReceived(std::function<void(const std::string&, 
                                          const std::vector<uint8_t>&)> callback);
    void onDataError(std::function<void(const std::string&, 
                                       const std::string&)> callback);
    
    // System events
    void onThemeChanged(std::function<void(const UITheme&)> callback);
    void onConfigurationChanged(std::function<void(const std::string&, 
                                                  const std::string&)> callback);
};

// Usage example
EventManager::getInstance().onWidgetStateChanged(
    [](const std::string& id, WidgetState oldState, WidgetState newState) {
        if (newState == WidgetState::ERROR) {
            // Handle widget error
            showErrorNotification(id);
        }
    }
);
```

### Performance Monitoring API

#### Metrics Collection

```cpp
class MetricsCollector {
public:
    struct WidgetMetrics {
        std::string widgetId;
        double renderTime;
        size_t memoryUsage;
        double cpuUsage;
        int frameRate;
        size_t dataTransferred;
    };
    
    // Collect metrics
    std::vector<WidgetMetrics> collectMetrics();
    
    // Performance thresholds
    void setPerformanceThreshold(const std::string& metric, double threshold);
    
    // Callbacks for performance issues
    void onPerformanceIssue(std::function<void(const std::string&, 
                                              const std::string&)> callback);
};
```

### Security API (Phase 1 Complete)

#### Security Framework Implementation

**Phase 1 Complete:** Enterprise-grade security framework with AES-GCM encryption, code signature verification, and Windows DPAPI integration.

```cpp
#include "src/core/security.h"

class SecurityManager {
public:
    // PHASE 1 IMPLEMENTED: Domain whitelist management
    void addAllowedDomain(const std::string& domain);
    void removeAllowedDomain(const std::string& domain);
    bool isDomainAllowed(const std::string& domain);
    
    // PHASE 1 IMPLEMENTED: AES-GCM encryption for sensitive data
    bool encryptData(const std::vector<uint8_t>& plaintext, 
                     const std::string& password,
                     std::vector<uint8_t>& encrypted);
    bool decryptData(const std::vector<uint8_t>& encrypted,
                     const std::string& password, 
                     std::vector<uint8_t>& plaintext);
    
    // PHASE 1 IMPLEMENTED: Windows DPAPI for API key storage
    bool storeAPIKeySecure(const std::string& service, const std::string& key);
    std::string getAPIKeySecure(const std::string& service);
    
    // PHASE 1 IMPLEMENTED: Code signature verification
    bool verifyCodeSignature(const std::string& filePath);
    bool verifyExecutableIntegrity(const std::string& exePath);
    
    // PHASE 1 IMPLEMENTED: Malware pattern detection
    bool scanForMaliciousPatterns(const std::vector<uint8_t>& data);
    bool validateFileExtension(const std::string& filePath, 
                              const std::vector<std::string>& allowedExtensions);
    
    // PHASE 1 IMPLEMENTED: Security configuration
    bool initializeSecurityFramework();
    bool performSecuritySweep(const std::string& directory);
    
    // Content security validation
    bool validateContent(const std::vector<uint8_t>& data, 
                        const std::string& expectedType);
                        
    // Legacy API key management (deprecated - use secure versions)
    [[deprecated("Use storeAPIKeySecure instead")]]
    void setAPIKey(const std::string& service, const std::string& key);
    [[deprecated("Use getAPIKeySecure instead"]]]
    std::string getAPIKey(const std::string& service);
    bool validateAPIKey(const std::string& service, const std::string& key);
};

// PHASE 1 SECURITY FEATURES USAGE EXAMPLES

// Example 1: Secure API key storage
SecurityManager security;
security.storeAPIKeySecure("openweathermap", "your_api_key_here");
std::string key = security.getAPIKeySecure("openweathermap");

// Example 2: File integrity verification
if (!security.verifyCodeSignature("downloaded_plugin.dll")) {
    LOG_ERROR("Plugin failed signature verification");
    return false;
}

// Example 3: Data encryption for local cache
std::vector<uint8_t> sensitiveData = getWeatherData();
std::vector<uint8_t> encrypted;
if (security.encryptData(sensitiveData, userPassword, encrypted)) {
    saveToDisk(encrypted);
}

// Example 4: Malware scanning
std::vector<uint8_t> downloadedData = fetchFromAPI();
if (!security.scanForMaliciousPatterns(downloadedData)) {
    LOG_ERROR("Malicious content detected!");
    return false;
}
```
```

## Advanced Features

### Plugin Development

#### Plugin Interface

```cpp
class RainmeterPlugin {
public:
    virtual ~RainmeterPlugin() = default;
    
    // Plugin metadata
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getDescription() const = 0;
    
    // Lifecycle
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    
    // Widget factory
    virtual std::shared_ptr<BaseWidget> createWidget(const std::string& type,
                                                     const Json::Value& config) {
        return nullptr;
    }
    
    // Data providers
    virtual bool canHandleDataSource(const std::string& sourceType) { 
        return false; 
    }
    virtual std::vector<uint8_t> fetchData(const std::string& url,
                                          const std::map<std::string, std::string>& params) {
        return {};
    }
    
    // Custom rendering
    virtual void renderCustomEffects(SkCanvas* canvas, 
                                    const WidgetRenderContext& context) {}
};

// Plugin registration
#define REGISTER_RAINMETER_PLUGIN(PluginClass) \
    extern "C" __declspec(dllexport) RainmeterPlugin* CreatePlugin() { \
        return new PluginClass(); \
    }
```

#### Example Plugin Implementation

```cpp
class CryptoPricePlugin : public RainmeterPlugin {
public:
    std::string getName() const override { return "CryptoPricePlugin"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override { 
        return "Cryptocurrency price tracking widget"; 
    }
    
    bool initialize() override {
        // Initialize API connections
        return initializeCryptoAPI();
    }
    
    void shutdown() override {
        // Cleanup resources
        cleanupConnections();
    }
    
    std::shared_ptr<BaseWidget> createWidget(const std::string& type,
                                           const Json::Value& config) override {
        if (type == "crypto_ticker") {
            return std::make_shared<CryptoTickerWidget>(config);
        }
        return nullptr;
    }
    
    bool canHandleDataSource(const std::string& sourceType) override {
        return sourceType == "cryptocurrency";
    }
    
    std::vector<uint8_t> fetchData(const std::string& url,
                                  const std::map<std::string, std::string>& params) override {
        return fetchCryptoPrices(url, params);
    }
};

REGISTER_RAINMETER_PLUGIN(CryptoPricePlugin);
```

### Memory Management

#### Resource Optimization

```cpp
class ResourceManager {
public:
    // Texture management
    void createTextureAtlas(const std::vector<std::string>& imageFiles);
    sk_sp<SkImage> getTextureFromAtlas(const std::string& imageName);
    
    // Memory pools
    void* allocateFromPool(size_t size, const std::string& poolName);
    void deallocateToPool(void* ptr, const std::string& poolName);
    
    // Cache management
    void setCacheLimit(size_t maxSize);
    void evictLeastRecentlyUsed();
    void clearCache();
    
    // Performance monitoring
    struct MemoryStats {
        size_t totalAllocated;
        size_t totalCached;
        size_t poolUtilization;
        double fragmentationRatio;
    };
    MemoryStats getMemoryStats();
};
```

### Animation System

#### Skia-based Animations

```cpp
class AnimationSystem {
public:
    struct Animation {
        std::string id;
        float duration;
        std::function<void(float)> updateFunction;
        std::function<void()> completeCallback;
        bool loop;
        float progress;
    };
    
    // Animation management
    void startAnimation(const Animation& animation);
    void stopAnimation(const std::string& id);
    void pauseAnimation(const std::string& id);
    void resumeAnimation(const std::string& id);
    
    // Easing functions
    float easeInOut(float t);
    float easeElastic(float t);
    float easeBounce(float t);
    
    // Common animations
    void animateFloat(float& value, float target, float duration,
                     std::function<float(float)> easing = nullptr);
    void animateColor(SkColor& color, SkColor target, float duration);
    void animateTransform(SkMatrix& transform, const SkMatrix& target, float duration);
};

// Usage example
AnimationSystem::getInstance().animateFloat(widgetOpacity, 1.0f, 0.5f,
    [](float t) { return AnimationSystem::easeInOut(t); });
```

## API Usage Examples

### Creating a Custom Stock Widget

```cpp
class CustomStockWidget : public StreamingWidget {
private:
    std::vector<StockData> stockData;
    AnimationSystem::Animation priceAnimation;
    
public:
    CustomStockWidget(const std::string& id, const std::vector<std::string>& symbols) 
        : StreamingWidget(id, WidgetType::CUSTOM_STREAM, createStockConfig(symbols)) {
        initializeSkiaRendering();
    }
    
    void render(SkCanvas* canvas) override {
        // Premium background with glassmorphism
        renderPremiumBackground(canvas);
        
        // Animated stock prices
        renderStockPrices(canvas);
        
        // Neon accent lines
        renderAccentLines(canvas);
    }
    
private:
    void renderPremiumBackground(SkCanvas* canvas) {
        SkPaint glassPaint;
        glassPaint.setColor(SK_ColorWHITE);
        glassPaint.setAlpha(20);
        glassPaint.setMaskFilter(SkBlurMaskFilter::Make(kNormal_SkBlurStyle, 8.0f));
        
        SkRRect roundedRect = SkRRect::MakeRectXY(
            SkRect::MakeWH(getWidth(), getHeight()), 15, 15);
        canvas->drawRRect(roundedRect, glassPaint);
    }
    
    void renderStockPrices(SkCanvas* canvas) {
        SkPaint textPaint;
        textPaint.setColor(SK_ColorWHITE);
        textPaint.setTextSize(16);
        textPaint.setAntiAlias(true);
        
        float y = 30;
        for (const auto& stock : stockData) {
            // Animated price changes
            SkColor priceColor = stock.change >= 0 ? 
                SkColorSetRGB(0, 255, 136) : SkColorSetRGB(255, 68, 68);
            textPaint.setColor(priceColor);
            
            std::string displayText = stock.symbol + ": $" + 
                std::to_string(stock.price);
            canvas->drawSimpleText(displayText.c_str(), displayText.length(),
                                 SkTextEncoding::kUTF8, 20, y, SkFont(), textPaint);
            y += 25;
        }
    }
};

// Register and use the widget
auto stockWidget = std::make_shared<CustomStockWidget>("stocks", 
    std::vector<std::string>{"AAPL", "GOOGL", "MSFT"});
WidgetManager::getInstance().registerWidget(stockWidget);
stockWidget->start();
```

### Integrating Custom Data Source

```cpp
// Custom RSS feed integration
class CustomRSSProvider {
public:
    static bool registerProvider() {
        return WidgetManager::getInstance().registerCustomStreamHandler("rss://",
            [](const std::string& url) -> std::vector<uint8_t> {
                // Parse RSS URL: rss://feeds.example.com/news.xml
                std::string actualUrl = url.substr(6); // Remove "rss://"
                
                // Fetch RSS data
                HttpClient client;
                auto response = client.get(actualUrl);
                
                // Parse RSS and return processed data
                return processRSSFeed(response);
            });
    }
    
private:
    static std::vector<uint8_t> processRSSFeed(const std::vector<uint8_t>& rawXML) {
        // Parse XML and extract news items
        // Convert to JSON format for easier processing
        Json::Value newsItems;
        parseRSSXML(rawXML, newsItems);
        
        // Serialize to bytes
        Json::StreamWriterBuilder builder;
        std::string jsonString = Json::writeString(builder, newsItems);
        return std::vector<uint8_t>(jsonString.begin(), jsonString.end());
    }
};
```

## Error Handling and Debugging

### Logging API

```cpp
class Logger {
public:
    enum Level { DEBUG, INFO, WARNING, ERROR, CRITICAL };
    
    static void log(Level level, const std::string& component, 
                   const std::string& message);
    static void logWidgetEvent(const std::string& widgetId, 
                              const std::string& event);
    static void logPerformanceMetric(const std::string& metric, 
                                   double value);
    
    // Structured logging
    static void logStructured(Level level, const Json::Value& data);
};

// Usage
Logger::log(Logger::INFO, "StockWidget", "Price data updated");
Logger::logPerformanceMetric("render_time_ms", 16.7);
```

### Debugging Tools

```cpp
class DebugTools {
public:
    // Widget inspection
    static Json::Value inspectWidget(const std::string& widgetId);
    static void dumpWidgetHierarchy();
    
    // Performance profiling
    static void startProfiling(const std::string& widgetId);
    static void stopProfiling(const std::string& widgetId);
    static PerformanceReport getProfilingReport(const std::string& widgetId);
    
    // Memory debugging
    static void enableMemoryTracking();
    static MemoryReport getMemoryReport();
    static void detectMemoryLeaks();
};
```

This Developer API Reference provides comprehensive access to RainmeterManager's widget framework, enabling creation of premium, visually stunning widgets with advanced data integration capabilities.
