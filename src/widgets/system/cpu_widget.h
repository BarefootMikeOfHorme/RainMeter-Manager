#pragma once
// cpu_widget.h - Complete CPU monitoring widget
// Combines patterns from illustro\System.ini and Dashboard\CPU
// Copyright (c) 2025 RainmeterManager. All rights reserved.

#include "../framework/widget_base.h"
#include "../measures/measure_base.h"
#include "../meters/meter_base.h"
#include <vector>
#include <memory>

namespace RainmeterManager {
namespace Widgets {
namespace System {

/**
 * @brief CPU widget display styles discovered from 962 skins
 */
enum class CPUDisplayStyle {
    Minimal,            // Single text line (CPU: 45%)
    Bar,                // Horizontal/vertical bar
    CircularGauge,      // Roundline meter (Dashboard style)
    LineGraph,          // Historical graph
    MultiCore,          // Per-core display
    Combined            // Multiple visualizations
};

/**
 * @brief CPU widget theme
 * Pattern: All skins use variable-based theming
 */
struct CPUWidgetTheme {
    // Colors
    D2D1_COLOR_F backgroundColor = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.8f);
    D2D1_COLOR_F textColor = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.8f);
    D2D1_COLOR_F accentColor = D2D1::ColorF(0.92f, 0.67f, 0.0f, 1.0f);  // #eaab00
    D2D1_COLOR_F warningColor = D2D1::ColorF(1.0f, 1.0f, 0.0f, 1.0f);
    D2D1_COLOR_F criticalColor = D2D1::ColorF(1.0f, 0.0f, 0.0f, 1.0f);
    
    // Fonts
    std::wstring fontName = L"Trebuchet MS";
    float fontSize = 10.0f;
    float titleFontSize = 12.0f;
    
    // Thresholds for color changes
    float warningThreshold = 75.0f;     // Yellow above this
    float criticalThreshold = 90.0f;    // Red above this
    
    // Effects
    bool showShadow = true;
    bool antiAlias = true;
    float cornerRadius = 4.0f;
};

/**
 * @brief Complete CPU monitoring widget
 * 
 * Features:
 * - Multiple display styles
 * - Per-core or average monitoring
 * - Historical tracking
 * - Dynamic coloring based on load
 * - Mouse interaction (click for Task Manager)
 * - Tooltip with CPU name
 * - Animation support
 */
class CPUWidget : public WidgetBase {
private:
    // Configuration
    CPUDisplayStyle displayStyle_;
    CPUWidgetTheme theme_;
    int coreCount_;
    bool showAverage_;
    bool showPerCore_;
    size_t historySize_;
    
    // Measures (data sources)
    std::unique_ptr<Measures::CPUMeasure> cpuAverage_;
    std::vector<std::unique_ptr<Measures::CPUMeasure>> cpuCores_;
    std::unique_ptr<Measures::RegistryMeasure> cpuName_;
    
    // Meters (visualizations)
    std::unique_ptr<Meters::StringMeter> titleMeter_;
    std::unique_ptr<Meters::StringMeter> valueMeter_;
    std::unique_ptr<Meters::BarMeter> barMeter_;
    std::unique_ptr<Meters::RoundlineMeter> gaugeMeter_;
    std::unique_ptr<Meters::LineMeter> graphMeter_;
    std::unique_ptr<Meters::ShapeMeter> backgroundMeter_;
    
    // Per-core meters (if enabled)
    struct CoreVisual {
        std::unique_ptr<Meters::StringMeter> label;
        std::unique_ptr<Meters::StringMeter> value;
        std::unique_ptr<Meters::BarMeter> bar;
    };
    std::vector<CoreVisual> coreVisuals_;
    
    // Animation state
    float animationPhase_;
    DWORD lastAnimationTick_;
    
    // Layout
    int titleHeight_;
    int graphHeight_;
    int barHeight_;
    int coreSpacing_;
    
public:
    CPUWidget(const std::wstring& name, CPUDisplayStyle style = CPUDisplayStyle::Combined);
    ~CPUWidget();
    
    // Configuration
    void SetDisplayStyle(CPUDisplayStyle style);
    void SetTheme(const CPUWidgetTheme& theme);
    void SetShowPerCore(bool show);
    void SetHistorySize(size_t size);
    
    // WidgetBase overrides
    bool OnInitialize() override;
    void OnUpdate() override;
    void OnRender(ID2D1RenderTarget* renderTarget) override;
    void OnShutdown() override;
    void OnResize(UINT width, UINT height) override;
    void OnMouseClick(int x, int y, bool leftButton) override;
    
    // Accessors
    double GetCurrentCPUUsage() const;
    double GetCoreCPUUsage(int core) const;
    std::wstring GetCPUName() const;
    CPUDisplayStyle GetDisplayStyle() const { return displayStyle_; }
    
private:
    // Initialization helpers
    void InitializeMeasures();
    void InitializeMeters(ID2D1RenderTarget* renderTarget);
    void InitializeMinimalStyle(ID2D1RenderTarget* renderTarget);
    void InitializeBarStyle(ID2D1RenderTarget* renderTarget);
    void InitializeCircularGaugeStyle(ID2D1RenderTarget* renderTarget);
    void InitializeLineGraphStyle(ID2D1RenderTarget* renderTarget);
    void InitializeMultiCoreStyle(ID2D1RenderTarget* renderTarget);
    void InitializeCombinedStyle(ID2D1RenderTarget* renderTarget);
    
    // Layout helpers
    void CalculateLayout();
    void LayoutMinimal();
    void LayoutBar();
    void LayoutCircularGauge();
    void LayoutLineGraph();
    void LayoutMultiCore();
    void LayoutCombined();
    
    // Rendering helpers
    void RenderBackground(ID2D1RenderTarget* renderTarget);
    void RenderTitle(ID2D1RenderTarget* renderTarget);
    void RenderValue(ID2D1RenderTarget* renderTarget);
    void RenderBar(ID2D1RenderTarget* renderTarget);
    void RenderGauge(ID2D1RenderTarget* renderTarget);
    void RenderGraph(ID2D1RenderTarget* renderTarget);
    void RenderCores(ID2D1RenderTarget* renderTarget);
    
    // Dynamic coloring based on CPU load
    D2D1_COLOR_F GetColorForValue(double value) const;
    
    // Animation
    void UpdateAnimation();
    
    // System info
    int GetSystemCoreCount();
};

/**
 * @brief Factory for creating CPU widgets with presets
 */
class CPUWidgetFactory {
public:
    // Create with specific style
    static std::unique_ptr<CPUWidget> CreateMinimal(const std::wstring& name);
    static std::unique_ptr<CPUWidget> CreateBar(const std::wstring& name);
    static std::unique_ptr<CPUWidget> CreateCircularGauge(const std::wstring& name);
    static std::unique_ptr<CPUWidget> CreateLineGraph(const std::wstring& name);
    static std::unique_ptr<CPUWidget> CreateMultiCore(const std::wstring& name);
    static std::unique_ptr<CPUWidget> CreateDashboard(const std::wstring& name);
    
    // Create from config
    static std::unique_ptr<CPUWidget> CreateFromConfig(
        const std::wstring& name,
        const std::map<std::wstring, std::wstring>& config
    );
    
    // Theme presets
    static CPUWidgetTheme GetDarkTheme();
    static CPUWidgetTheme GetLightTheme();
    static CPUWidgetTheme GetMatrixTheme();      // Green on black
    static CPUWidgetTheme GetCyberpunkTheme();   // Neon colors
};

/**
 * @brief Example usage patterns
 */
namespace Examples {

// Pattern 1: Simple CPU percentage display
inline std::unique_ptr<CPUWidget> CreateSimpleCPU() {
    auto widget = std::make_unique<CPUWidget>(L"CPUSimple", CPUDisplayStyle::Minimal);
    widget->SetSize(200, 50);
    return widget;
}

// Pattern 2: Dashboard-style circular gauge (Dashboard suite pattern)
inline std::unique_ptr<CPUWidget> CreateDashboardCPU() {
    auto widget = std::make_unique<CPUWidget>(L"CPUGauge", CPUDisplayStyle::CircularGauge);
    widget->SetSize(200, 200);
    widget->SetTheme(CPUWidgetFactory::GetDarkTheme());
    return widget;
}

// Pattern 3: Multi-core with per-core bars (Evolucion pattern)
inline std::unique_ptr<CPUWidget> CreateMultiCoreCPU() {
    auto widget = std::make_unique<CPUWidget>(L"CPUCores", CPUDisplayStyle::MultiCore);
    widget->SetShowPerCore(true);
    widget->SetSize(300, 400);
    return widget;
}

// Pattern 4: Combined view with graph (NXT-OS pattern)
inline std::unique_ptr<CPUWidget> CreateAdvancedCPU() {
    auto widget = std::make_unique<CPUWidget>(L"CPUAdvanced", CPUDisplayStyle::Combined);
    widget->SetHistorySize(100);
    widget->SetSize(400, 300);
    
    CPUWidgetTheme theme = CPUWidgetFactory::GetCyberpunkTheme();
    widget->SetTheme(theme);
    
    return widget;
}

} // namespace Examples

} // namespace System
} // namespace Widgets
} // namespace RainmeterManager
