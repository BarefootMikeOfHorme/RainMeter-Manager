# RainmeterManager Integration & Customization Guide

## Overview

RainmeterManager provides extensive customization capabilities through its Skia-native widget framework, allowing developers to create premium, modern-looking widgets and integrate custom data sources. This guide covers integration paths, API extensions, and customization options.

## Table of Contents

1. [Adding Custom Feeds and Data Sources](#custom-feeds)
2. [Widget Development with Skia](#widget-development)
3. [API Extension Points](#api-extensions)
4. [Configuration and Customization](#configuration)
5. [Legal and Usage Guidelines](#legal-usage)
6. [Advanced Development](#advanced-development)

---

## Custom Feeds and Data Sources {#custom-feeds}

### Swapping Out Default Sources

RainmeterManager's widget framework supports multiple data providers through a plugin-based architecture. You can easily swap or add new data sources without modifying core code.

#### Stock/Financial Data Sources

```cpp
// Example: Adding a custom financial data provider
#include "src/widgets/templates/ticker_widget.h"

// Register your custom provider
REGISTER_TICKER_PROVIDER(MyFinanceAPI,
    [](const TickerWidget::TickerConfig& config) -> std::string {
        return "https://api.myfinance.com/quotes?symbols=" + 
               joinSymbols(config.symbols, ",") + "&key=" + config.apiKey;
    },
    [](const std::vector<uint8_t>& data, TickerWidget::TickerType type) -> std::vector<TickerWidget::TickerItem> {
        Json::Value root;
        Json::Reader reader;
        
        if (!reader.parse(std::string(data.begin(), data.end()), root)) {
            return {};
        }
        
        std::vector<TickerWidget::TickerItem> items;
        for (const auto& quote : root["quotes"]) {
            TickerWidget::TickerItem item;
            item.symbol = quote["symbol"].asString();
            item.currentValue = quote["price"].asString();
            item.changeValue = quote["change"].asString();
            item.changePercent = quote["changePercent"].asString();
            items.push_back(item);
        }
        
        return items;
    }
);
```

#### Weather Data Integration

```cpp
// Example: Custom weather provider
#include "src/widgets/templates/weather_widget.h"

REGISTER_WEATHER_PROVIDER(MyWeatherService,
    [](const WeatherRadarWidget::WeatherConfig& config) -> std::vector<std::string> {
        std::vector<std::string> urls;
        for (int frame = 0; frame < config.animationFrames; ++frame) {
            urls.push_back("https://api.myweather.com/radar/" + config.location + 
                          "?frame=" + std::to_string(frame) + "&key=" + config.apiKey);
        }
        return urls;
    },
    [](const std::vector<uint8_t>& data, WeatherRadarWidget::WeatherDataType type) -> bool {
        // Validate and process weather image data
        return data.size() > 0 && data[0] == 0x89 && data[1] == 0x50; // PNG header
    }
);
```

### Configuration File Integration

Create custom feed configurations in `%APPDATA%/RainmeterManager/custom_feeds.json`:

```json
{
  "feeds": {
    "custom_news": {
      "type": "rss",
      "url": "https://your-news-source.com/feed.xml",
      "refresh_interval": 300000,
      "fallback_content": "News temporarily unavailable"
    },
    "crypto_prices": {
      "type": "ticker",
      "provider": "custom_api",
      "endpoint": "https://api.yourcrypto.com/prices",
      "symbols": ["BTC", "ETH", "ADA"],
      "api_key_env": "CRYPTO_API_KEY"
    },
    "custom_weather": {
      "type": "weather",
      "provider": "custom_weather",
      "location": "40.7128,-74.0060",
      "data_type": "radar_precipitation"
    }
  }
}
```

---

## Widget Development with Skia {#widget-development}

### Premium Visual Effects with Skia

RainmeterManager uses Skia for high-quality, modern rendering. Here's how to create visually stunning widgets:

#### Basic Skia Widget Template

```cpp
#include <skia/include/core/SkCanvas.h>
#include <skia/include/core/SkPaint.h>
#include <skia/include/effects/SkGradientShader.h>
#include <skia/include/effects/SkBlurMaskFilter.h>

class PremiumWidget : public BaseWidget {
private:
    sk_sp<SkSurface> surface;
    SkPaint glassPaint;
    SkPaint neonPaint;
    
public:
    void initializeSkiaEffects() {
        // Glass morphism effect
        glassPaint.setColor(SK_ColorWHITE);
        glassPaint.setAlpha(30);
        glassPaint.setMaskFilter(SkBlurMaskFilter::Make(kNormal_SkBlurStyle, 5.0f));
        
        // Neon glow effect
        neonPaint.setColor(SkColorSetARGB(255, 0, 255, 255));
        neonPaint.setStyle(SkPaint::kStroke_Style);
        neonPaint.setStrokeWidth(2.0f);
        neonPaint.setMaskFilter(SkBlurMaskFilter::Make(kNormal_SkBlurStyle, 8.0f));
    }
    
    void renderPremiumBackground(SkCanvas* canvas, int width, int height) {
        // Gradient background
        SkPoint points[2] = {{0, 0}, {0, (SkScalar)height}};
        SkColor colors[3] = {
            SkColorSetARGB(255, 20, 20, 40),   // Dark blue
            SkColorSetARGB(255, 40, 20, 60),   // Purple
            SkColorSetARGB(255, 20, 20, 40)    // Dark blue
        };
        SkScalar positions[3] = {0, 0.5f, 1.0f};
        
        SkPaint gradientPaint;
        gradientPaint.setShader(SkGradientShader::MakeLinear(
            points, colors, positions, 3, SkTileMode::kClamp));
            
        canvas->drawRect(SkRect::MakeWH(width, height), gradientPaint);
        
        // Glass overlay
        SkRRect glassRect = SkRRect::MakeRectXY(
            SkRect::MakeXYWH(10, 10, width-20, height-20), 15, 15);
        canvas->drawRRect(glassRect, glassPaint);
        
        // Neon border
        canvas->drawRRect(glassRect, neonPaint);
    }
    
    void renderDataVisualization(SkCanvas* canvas, const std::vector<float>& data) {
        if (data.empty()) return;
        
        SkPath path;
        SkPaint linePaint;
        linePaint.setColor(SkColorSetARGB(255, 0, 255, 128));
        linePaint.setStyle(SkPaint::kStroke_Style);
        linePaint.setStrokeWidth(3.0f);
        linePaint.setAntiAlias(true);
        
        // Create smooth curve through data points
        float stepX = (float)getWidth() / (data.size() - 1);
        path.moveTo(0, getHeight() - (data[0] * getHeight()));
        
        for (size_t i = 1; i < data.size(); ++i) {
            float x = i * stepX;
            float y = getHeight() - (data[i] * getHeight());
            path.lineTo(x, y);
        }
        
        canvas->drawPath(path, linePaint);
        
        // Add glow effect
        SkPaint glowPaint = linePaint;
        glowPaint.setMaskFilter(SkBlurMaskFilter::Make(kNormal_SkBlurStyle, 5.0f));
        glowPaint.setAlpha(100);
        canvas->drawPath(path, glowPaint);
    }
};
```

#### Advanced Animation Effects

```cpp
class AnimatedWidget : public PremiumWidget {
private:
    float animationProgress = 0.0f;
    std::chrono::steady_clock::time_point startTime;
    
public:
    void renderAnimatedElements(SkCanvas* canvas) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
        animationProgress = (elapsed.count() % 3000) / 3000.0f; // 3-second cycle
        
        // Pulsing glow effect
        float glowIntensity = (sin(animationProgress * 2 * M_PI) + 1) / 2;
        
        SkPaint pulsePaint;
        pulsePaint.setColor(SkColorSetARGB(
            (int)(255 * glowIntensity * 0.5), 255, 255, 255));
        pulsePaint.setMaskFilter(SkBlurMaskFilter::Make(
            kNormal_SkBlurStyle, 10.0f * glowIntensity));
            
        canvas->drawCircle(getWidth()/2, getHeight()/2, 
                          50 + 20 * glowIntensity, pulsePaint);
        
        // Particle system
        renderParticles(canvas, animationProgress);
    }
    
    void renderParticles(SkCanvas* canvas, float progress) {
        SkPaint particlePaint;
        particlePaint.setAntiAlias(true);
        
        for (int i = 0; i < 20; ++i) {
            float angle = (i / 20.0f) * 2 * M_PI + progress * 2 * M_PI;
            float radius = 100 + 50 * sin(progress * M_PI + i);
            
            float x = getWidth()/2 + cos(angle) * radius;
            float y = getHeight()/2 + sin(angle) * radius;
            
            float alpha = (sin(progress * M_PI * 2 + i) + 1) / 2;
            particlePaint.setColor(SkColorSetARGB(
                (int)(255 * alpha), 100, 200, 255));
                
            canvas->drawCircle(x, y, 3, particlePaint);
        }
    }
};
```

---

## API Extension Points {#api-extensions}

### Public API for Advanced Users

#### Widget Manager API

```cpp
// Access the global widget manager
WidgetManager& manager = WidgetManager::getInstance();

// Create custom widgets programmatically
auto customWidget = manager.createStreamingWidget(
    "my_custom_feed", 
    WidgetType::CUSTOM_STREAM,
    customStreamConfig
);

// Register custom stream handlers
manager.registerCustomStreamHandler("myprotocol://", 
    [](const std::string& url) -> std::vector<uint8_t> {
        // Custom protocol handler
        return downloadCustomData(url);
    }
);
```

#### Configuration API

```cpp
#include "config/configuration_manager.h"

class ConfigurationManager {
public:
    // Theme management
    void setGlobalTheme(const std::string& themeName);
    void registerCustomTheme(const std::string& name, const UITheme& theme);
    
    // Widget templates
    void registerWidgetTemplate(const std::string& name, 
                                const WidgetTemplate& template);
    
    // Data source configuration
    void addDataSource(const std::string& name, 
                       const DataSourceConfig& config);
    
    // Layout presets
    void saveLayoutPreset(const std::string& name, 
                          const std::vector<WidgetLayout>& layout);
    void loadLayoutPreset(const std::string& name);
};
```

#### Event System API

```cpp
#include "events/event_manager.h"

class EventManager {
public:
    // Widget events
    void onWidgetCreated(std::function<void(const std::string& widgetId)> callback);
    void onWidgetStateChanged(std::function<void(const std::string& widgetId, 
                                                WidgetState oldState, 
                                                WidgetState newState)> callback);
    
    // Data events
    void onDataReceived(std::function<void(const std::string& sourceId, 
                                          const std::vector<uint8_t>& data)> callback);
    
    // System events
    void onThemeChanged(std::function<void(const UITheme& newTheme)> callback);
    void onConfigurationChanged(std::function<void(const std::string& key, 
                                                  const std::string& value)> callback);
};
```

#### Plugin Development Interface

```cpp
// Plugin base class
class RainmeterPlugin {
public:
    virtual ~RainmeterPlugin() = default;
    
    // Plugin lifecycle
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    
    // Widget factory methods
    virtual std::shared_ptr<BaseWidget> createWidget(const std::string& type, 
                                                     const Json::Value& config) { return nullptr; }
    
    // Data provider methods
    virtual bool canHandleDataSource(const std::string& sourceType) { return false; }
    virtual std::vector<uint8_t> fetchData(const std::string& url, 
                                           const std::map<std::string, std::string>& params) {
        return {};
    }
    
    // Rendering extensions
    virtual void renderCustomEffects(SkCanvas* canvas, const WidgetRenderContext& context) {}
};

// Plugin registration macro
#define REGISTER_RAINMETER_PLUGIN(PluginClass) \
    extern "C" __declspec(dllexport) RainmeterPlugin* CreatePlugin() { \
        return new PluginClass(); \
    }
```

---

## Configuration and Customization {#configuration}

### Theme System

#### Custom Theme Definition

```json
{
  "themes": {
    "cyberpunk": {
      "name": "Cyberpunk",
      "colors": {
        "primary": "#00FFFF",
        "secondary": "#FF00FF",
        "background": "#0A0A0A",
        "surface": "#1A1A2E",
        "accent": "#E94560"
      },
      "effects": {
        "blur_radius": 8,
        "glow_intensity": 0.8,
        "transparency": 0.15,
        "animations": true,
        "particles": true
      },
      "typography": {
        "primary_font": "Orbitron",
        "secondary_font": "Roboto Mono",
        "size_multiplier": 1.1
      }
    }
  }
}
```

#### Widget-Specific Customization

```json
{
  "widget_customization": {
    "weather_radar": {
      "animation_speed": 500,
      "color_scheme": "temperature",
      "overlay_opacity": 0.7,
      "border_style": "neon",
      "effects": ["glow", "blur"]
    },
    "stock_ticker": {
      "scroll_speed": 30,
      "highlight_changes": true,
      "color_coding": {
        "positive": "#00FF88",
        "negative": "#FF4444",
        "neutral": "#FFFFFF"
      }
    }
  }
}
```

### Advanced Configuration Options

#### Performance Settings

```ini
[Performance]
# Rendering quality (0-100)
RenderQuality=85

# Frame rate limit
MaxFPS=60

# Hardware acceleration
UseGPUAcceleration=true

# Memory management
MaxCacheSize=512MB
PreloadWidgets=true

[Effects]
# Visual effects quality
EffectQuality=High
EnableBlur=true
EnableGlow=true
EnableParticles=true
AnimationQuality=High

[Network]
# Connection settings
RequestTimeout=30000
MaxConcurrentRequests=10
EnableCaching=true
CacheDuration=300

# Fallback behavior
EnableFallback=true
FallbackTimeout=5000
```

#### Security Configuration

```json
{
  "security": {
    "allowed_domains": [
      "api.openweathermap.org",
      "finance.yahoo.com",
      "api.coingecko.com"
    ],
    "blocked_domains": [],
    "require_https": true,
    "validate_certificates": true,
    "sandbox_widgets": true,
    "allow_scripts": false,
    "max_request_size": "10MB"
  }
}
```

---

## Legal and Usage Guidelines {#legal-usage}

### Data Source Terms of Service

**Important:** When integrating external data sources, you must comply with their respective Terms of Service and API usage policies.

#### Common API Providers and Requirements

1. **OpenWeatherMap**
   - Free tier: 1,000 calls/day
   - Attribution required
   - Commercial use requires paid plan

2. **Yahoo Finance**
   - For personal use only
   - No commercial redistribution
   - Rate limits apply

3. **Alpha Vantage**
   - Free tier: 5 calls/minute, 500 calls/day
   - Attribution recommended
   - Premium tiers available

4. **CoinGecko**
   - Free tier: 50 calls/minute
   - Attribution required for free tier
   - Commercial use allowed

### API Key Management

**Security Best Practices:**

```cpp
// Store API keys securely
class SecureAPIKeyManager {
public:
    // Never hardcode API keys
    std::string getAPIKey(const std::string& service) {
        // Read from encrypted storage or environment variables
        return readFromSecureStorage("api_key_" + service);
    }
    
    // Validate API key format
    bool validateAPIKey(const std::string& service, const std::string& key) {
        // Service-specific validation
        return isValidKeyFormat(service, key);
    }
};
```

**Environment Variables Setup:**
```bash
# Set API keys as environment variables
export OPENWEATHER_API_KEY="your_key_here"
export ALPHAVANTAGE_API_KEY="your_key_here"
export COINGECKO_API_KEY="your_key_here"
```

### Compliance Guidelines

#### Data Privacy
- User data is processed locally only
- No personal information is transmitted to external APIs
- Location data (if used) should be anonymized
- Cache files should be encrypted

#### Rate Limiting
```cpp
class RateLimiter {
    std::map<std::string, std::queue<std::chrono::steady_clock::time_point>> requestHistory;
    std::map<std::string, int> maxRequestsPerMinute;
    
public:
    bool canMakeRequest(const std::string& service) {
        auto now = std::chrono::steady_clock::now();
        auto& history = requestHistory[service];
        
        // Remove requests older than 1 minute
        while (!history.empty() && 
               (now - history.front()) > std::chrono::minutes(1)) {
            history.pop();
        }
        
        return history.size() < maxRequestsPerMinute[service];
    }
};
```

### Licensing and Attribution

#### Required Attributions

Add to your widget configuration:

```json
{
  "attributions": {
    "weather_data": "Weather data provided by OpenWeatherMap",
    "financial_data": "Financial data provided by Yahoo Finance",
    "icons": "Icons by FontAwesome (CC BY 4.0)"
  }
}
```

#### Open Source Components

RainmeterManager uses several open source libraries:
- **Skia Graphics Library** - BSD-3-Clause License
- **JsonCpp** - MIT License
- **WebView2** - Microsoft Software License
- **WinInet** - Windows SDK

---

## Advanced Development {#advanced-development}

### Custom Rendering Pipeline

#### Skia Integration Template

```cpp
class CustomSkiaRenderer {
private:
    sk_sp<SkSurface> surface;
    sk_sp<GrDirectContext> context;
    
public:
    void initializeGPUContext(HWND hwnd) {
        // Initialize GPU-accelerated Skia context
        sk_sp<const GrGLInterface> interface = GrGLMakeNativeInterface();
        context = GrDirectContext::MakeGL(interface);
        
        // Create surface
        GrBackendRenderTarget backendRT = createBackendRenderTarget(hwnd);
        surface = SkSurface::MakeFromBackendRenderTarget(
            context.get(), backendRT, kBottomLeft_GrSurfaceOrigin,
            kRGBA_8888_SkColorType, nullptr, nullptr);
    }
    
    void renderFrame() {
        SkCanvas* canvas = surface->getCanvas();
        canvas->clear(SK_ColorTRANSPARENT);
        
        // Custom rendering logic here
        renderBackground(canvas);
        renderWidgets(canvas);
        renderEffects(canvas);
        
        context->flush();
    }
};
```

### Performance Optimization

#### Memory Management
```cpp
class WidgetMemoryManager {
public:
    void optimizeMemoryUsage() {
        // Texture atlas for small icons
        createTextureAtlas();
        
        // Lazy loading for off-screen widgets
        implementLazyLoading();
        
        // Memory pool for frequent allocations
        setupMemoryPools();
    }
    
private:
    void createTextureAtlas() {
        // Combine small textures into single atlas
        // Reduces GPU state changes
    }
    
    void implementLazyLoading() {
        // Only render visible widgets
        // Unload textures for hidden widgets
    }
};
```

### Debugging and Diagnostics

#### Performance Profiling
```cpp
class PerformanceProfiler {
public:
    void startFrame() {
        frameStart = std::chrono::high_resolution_clock::now();
    }
    
    void endFrame() {
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            frameEnd - frameStart);
            
        frameTimeHistory.push_back(duration.count());
        if (frameTimeHistory.size() > 100) {
            frameTimeHistory.pop_front();
        }
        
        // Log performance metrics
        if (frameTimeHistory.size() == 100) {
            logPerformanceStats();
        }
    }
    
private:
    std::chrono::high_resolution_clock::time_point frameStart;
    std::deque<long> frameTimeHistory;
};
```

---

## Conclusion

This integration guide provides the foundation for extending RainmeterManager with custom data sources, premium Skia-based visual effects, and advanced widget functionality. The modular architecture ensures that customizations can be made without modifying core code, while the Skia rendering pipeline enables the creation of modern, visually stunning widgets that stand out from standard Windows applications.

For additional support and examples, refer to the widget templates in `src/widgets/templates/` and the sample configurations in the `examples/` directory.
