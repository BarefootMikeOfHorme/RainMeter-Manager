#pragma once
// meter_base.h - Base classes for all widget visualizations
// Based on analysis of 962 Rainmeter skins
// Copyright (c) 2025 RainmeterManager. All rights reserved.

#include "../measures/measure_base.h"
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace RainmeterManager {
namespace Widgets {
namespace Meters {

/**
 * @brief Meter types discovered from Rainmeter analysis
 */
enum class MeterType {
    String,         // Text display
    Bar,            // Progress bar
    Line,           // Line graph
    Histogram,      // Filled graph
    Roundline,      // Circular gauge
    Rotator,        // Needle gauge
    Image,          // Static/dynamic images
    Shape,          // Direct2D vector shapes
    Button          // Interactive button
};

/**
 * @brief String alignment options
 */
enum class StringAlign {
    Left, Center, Right,
    LeftTop, CenterTop, RightTop,
    LeftCenter, CenterCenter, RightCenter,
    LeftBottom, CenterBottom, RightBottom
};

/**
 * @brief String effects
 */
enum class StringEffect {
    None,
    Shadow,
    Border
};

/**
 * @brief String case transformation
 */
enum class StringCase {
    None,
    Upper,
    Lower,
    Proper
};

/**
 * @brief Bar orientation
 */
enum class BarOrientation {
    Horizontal,
    Vertical
};

/**
 * @brief Base interface for all meters (visualizations)
 */
class IMeter {
public:
    virtual ~IMeter() = default;
    
    // Lifecycle
    virtual bool Initialize(ID2D1RenderTarget* renderTarget) = 0;
    virtual void Update() = 0;
    virtual void Render(ID2D1RenderTarget* renderTarget) = 0;
    virtual void Shutdown() = 0;
    
    // Positioning
    virtual void SetPosition(int x, int y) = 0;
    virtual void GetPosition(int& x, int& y) const = 0;
    virtual void SetSize(UINT width, UINT height) = 0;
    virtual void GetSize(UINT& width, UINT& height) const = 0;
    
    // Visibility
    virtual bool IsVisible() const = 0;
    virtual void SetVisible(bool visible) = 0;
    
    // Measure binding
    virtual void BindMeasure(Measures::IMeasure* measure) = 0;
    virtual void BindMeasure(const std::wstring& name, Measures::IMeasure* measure) = 0;
    
    // Metadata
    virtual MeterType GetType() const = 0;
    virtual std::wstring GetName() const = 0;
    
    // Mouse interaction
    virtual bool HitTest(int x, int y) const = 0;
    virtual void OnMouseMove(int x, int y) = 0;
    virtual void OnMouseClick(int x, int y, bool leftButton) = 0;
    virtual void OnMouseEnter() = 0;
    virtual void OnMouseLeave() = 0;
};

/**
 * @brief Base implementation of IMeter
 */
class MeterBase : public IMeter {
protected:
    std::wstring name_;
    MeterType type_;
    int x_, y_;
    UINT width_, height_;
    bool visible_;
    bool initialized_;
    
    // Bound measures (up to 8 as seen in Rainmeter)
    std::vector<Measures::IMeasure*> measures_;
    std::map<std::wstring, Measures::IMeasure*> namedMeasures_;
    
    // Mouse interaction state
    bool mouseOver_;
    std::function<void()> onLeftClick_;
    std::function<void()> onRightClick_;
    std::function<void(int)> onMouseScroll_;
    std::wstring tooltipText_;
    
public:
    MeterBase(const std::wstring& name, MeterType type);
    virtual ~MeterBase();
    
    // IMeter implementation
    bool Initialize(ID2D1RenderTarget* renderTarget) override;
    void Update() override;
    void Render(ID2D1RenderTarget* renderTarget) override;
    void Shutdown() override;
    
    void SetPosition(int x, int y) override;
    void GetPosition(int& x, int& y) const override;
    void SetSize(UINT width, UINT height) override;
    void GetSize(UINT& width, UINT& height) const override;
    
    bool IsVisible() const override { return visible_; }
    void SetVisible(bool visible) override { visible_ = visible; }
    
    void BindMeasure(Measures::IMeasure* measure) override;
    void BindMeasure(const std::wstring& name, Measures::IMeasure* measure) override;
    
    MeterType GetType() const override { return type_; }
    std::wstring GetName() const override { return name_; }
    
    bool HitTest(int x, int y) const override;
    void OnMouseMove(int x, int y) override;
    void OnMouseClick(int x, int y, bool leftButton) override;
    void OnMouseEnter() override;
    void OnMouseLeave() override;
    
    // Mouse action setters
    void SetLeftClickAction(std::function<void()> action) { onLeftClick_ = action; }
    void SetRightClickAction(std::function<void()> action) { onRightClick_ = action; }
    void SetScrollAction(std::function<void(int)> action) { onMouseScroll_ = action; }
    void SetTooltip(const std::wstring& tooltip) { tooltipText_ = tooltip; }
    
protected:
    // Override in derived classes
    virtual bool OnInitialize(ID2D1RenderTarget* renderTarget) = 0;
    virtual void OnUpdate() = 0;
    virtual void OnRender(ID2D1RenderTarget* renderTarget) = 0;
    virtual void OnShutdown() = 0;
    
    // Helpers
    double GetMeasureValue(size_t index = 0) const;
    std::wstring GetMeasureStringValue(size_t index = 0) const;
};

/**
 * @brief String meter for text display
 * Pattern from ALL skins - most common meter type
 */
class StringMeter : public MeterBase {
private:
    std::wstring text_;
    std::wstring fontFace_;
    float fontSize_;
    D2D1_COLOR_F fontColor_;
    StringAlign stringAlign_;
    StringEffect stringEffect_;
    StringCase stringCase_;
    D2D1_COLOR_F effectColor_;
    bool antiAlias_;
    bool clipString_;
    bool percentual_;          // Convert bytes to percentage
    bool autoScale_;           // Auto-scale bytes (1, 1k, 2k)
    int numOfDecimals_;
    
    // DirectWrite resources
    IDWriteTextFormat* textFormat_;
    ID2D1SolidColorBrush* textBrush_;
    ID2D1SolidColorBrush* shadowBrush_;
    
public:
    explicit StringMeter(const std::wstring& name);
    ~StringMeter();
    
    // Configuration
    void SetText(const std::wstring& text) { text_ = text; }
    void SetFont(const std::wstring& face, float size);
    void SetColor(D2D1_COLOR_F color) { fontColor_ = color; }
    void SetAlignment(StringAlign align) { stringAlign_ = align; }
    void SetEffect(StringEffect effect, D2D1_COLOR_F effectColor);
    void SetCase(StringCase textCase) { stringCase_ = textCase; }
    void SetPercentual(bool percentual) { percentual_ = percentual; }
    void SetAutoScale(bool autoScale) { autoScale_ = autoScale; }
    void SetNumOfDecimals(int decimals) { numOfDecimals_ = decimals; }
    
protected:
    bool OnInitialize(ID2D1RenderTarget* renderTarget) override;
    void OnUpdate() override;
    void OnRender(ID2D1RenderTarget* renderTarget) override;
    void OnShutdown() override;
    
private:
    std::wstring FormatText();
    std::wstring ApplyCase(const std::wstring& text);
    D2D1_RECT_F GetTextRect() const;
    DWRITE_TEXT_ALIGNMENT GetDWriteAlignment() const;
};

/**
 * @brief Bar meter for progress bars
 * Pattern from illustro\System.ini (CPU/RAM bars)
 */
class BarMeter : public MeterBase {
private:
    D2D1_COLOR_F barColor_;
    D2D1_COLOR_F backgroundColor_;
    BarOrientation orientation_;
    bool flip_;
    
    ID2D1SolidColorBrush* barBrush_;
    ID2D1SolidColorBrush* bgBrush_;
    
public:
    explicit BarMeter(const std::wstring& name);
    ~BarMeter();
    
    void SetBarColor(D2D1_COLOR_F color) { barColor_ = color; }
    void SetBackgroundColor(D2D1_COLOR_F color) { backgroundColor_ = color; }
    void SetOrientation(BarOrientation orientation) { orientation_ = orientation; }
    void SetFlip(bool flip) { flip_ = flip; }
    
protected:
    bool OnInitialize(ID2D1RenderTarget* renderTarget) override;
    void OnUpdate() override;
    void OnRender(ID2D1RenderTarget* renderTarget) override;
    void OnShutdown() override;
};

/**
 * @brief Line meter for graphs
 * Pattern from Dashboard (CPU/Network history graphs)
 */
class LineMeter : public MeterBase {
private:
    struct LineConfig {
        D2D1_COLOR_F color;
        float width;
        std::vector<double> history;
        size_t maxHistory;
    };
    
    std::vector<LineConfig> lines_;
    bool horizontalLines_;
    D2D1_COLOR_F horizontalLineColor_;
    bool autoScale_;
    bool antialias_;
    
    std::vector<ID2D1SolidColorBrush*> lineBrushes_;
    ID2D1SolidColorBrush* gridBrush_;
    
public:
    explicit LineMeter(const std::wstring& name);
    ~LineMeter();
    
    void AddLine(D2D1_COLOR_F color, float width = 1.0f, size_t maxHistory = 100);
    void SetHorizontalLines(bool enable, D2D1_COLOR_F color);
    void SetAutoScale(bool autoScale) { autoScale_ = autoScale; }
    
protected:
    bool OnInitialize(ID2D1RenderTarget* renderTarget) override;
    void OnUpdate() override;
    void OnRender(ID2D1RenderTarget* renderTarget) override;
    void OnShutdown() override;
};

/**
 * @brief Roundline meter for circular gauges
 * Pattern from Dashboard (circular CPU/RAM meters - MOST COMPLEX)
 */
class RoundlineMeter : public MeterBase {
private:
    float startAngle_;      // Radians
    float rotationAngle_;   // Arc length in radians
    float lineStart_;       // Inner radius
    float lineLength_;      // Arc thickness
    D2D1_COLOR_F lineColor_;
    bool solid_;            // Filled or outline
    bool antialias_;
    
    ID2D1SolidColorBrush* lineBrush_;
    
public:
    explicit RoundlineMeter(const std::wstring& name);
    ~RoundlineMeter();
    
    void SetStartAngle(float angleRadians) { startAngle_ = angleRadians; }
    void SetRotationAngle(float angleRadians) { rotationAngle_ = angleRadians; }
    void SetLineStart(float start) { lineStart_ = start; }
    void SetLineLength(float length) { lineLength_ = length; }
    void SetLineColor(D2D1_COLOR_F color) { lineColor_ = color; }
    void SetSolid(bool solid) { solid_ = solid; }
    
protected:
    bool OnInitialize(ID2D1RenderTarget* renderTarget) override;
    void OnUpdate() override;
    void OnRender(ID2D1RenderTarget* renderTarget) override;
    void OnShutdown() override;
    
private:
    void DrawArc(ID2D1RenderTarget* renderTarget, float value);
};

/**
 * @brief Image meter for static/dynamic images
 * Pattern: weather icons, album art, backgrounds
 */
class ImageMeter : public MeterBase {
private:
    std::wstring imagePath_;
    bool preserveAspectRatio_;
    bool greyscale_;
    D2D1_COLOR_F imageTint_;
    float imageAlpha_;
    float imageRotate_;
    
    ID2D1Bitmap* bitmap_;
    IWICImagingFactory* wicFactory_;
    
public:
    explicit ImageMeter(const std::wstring& name);
    ~ImageMeter();
    
    void SetImagePath(const std::wstring& path);
    void SetPreserveAspectRatio(bool preserve) { preserveAspectRatio_ = preserve; }
    void SetGreyscale(bool grey) { greyscale_ = grey; }
    void SetTint(D2D1_COLOR_F tint) { imageTint_ = tint; }
    void SetAlpha(float alpha) { imageAlpha_ = alpha; }
    void SetRotation(float degrees) { imageRotate_ = degrees; }
    
protected:
    bool OnInitialize(ID2D1RenderTarget* renderTarget) override;
    void OnUpdate() override;
    void OnRender(ID2D1RenderTarget* renderTarget) override;
    void OnShutdown() override;
    
private:
    bool LoadImage(ID2D1RenderTarget* renderTarget);
};

/**
 * @brief Shape meter for vector graphics
 * Pattern: Modern skins use Direct2D shapes extensively
 */
class ShapeMeter : public MeterBase {
public:
    enum class ShapeType {
        Rectangle,
        Ellipse,
        Line,
        Path
    };
    
    struct ShapeDefinition {
        ShapeType type;
        std::vector<D2D1_POINT_2F> points;
        D2D1_COLOR_F fillColor;
        D2D1_COLOR_F strokeColor;
        float strokeWidth;
        float cornerRadius;
        bool filled;
    };
    
private:
    std::vector<ShapeDefinition> shapes_;
    std::vector<ID2D1SolidColorBrush*> fillBrushes_;
    std::vector<ID2D1SolidColorBrush*> strokeBrushes_;
    
public:
    explicit ShapeMeter(const std::wstring& name);
    ~ShapeMeter();
    
    void AddRectangle(float x, float y, float width, float height, 
                     D2D1_COLOR_F fillColor, float cornerRadius = 0.0f);
    void AddEllipse(float cx, float cy, float radiusX, float radiusY,
                   D2D1_COLOR_F fillColor);
    void AddLine(float x1, float y1, float x2, float y2,
                D2D1_COLOR_F strokeColor, float strokeWidth = 1.0f);
    
protected:
    bool OnInitialize(ID2D1RenderTarget* renderTarget) override;
    void OnUpdate() override;
    void OnRender(ID2D1RenderTarget* renderTarget) override;
    void OnShutdown() override;
    
private:
    void RenderShape(ID2D1RenderTarget* renderTarget, const ShapeDefinition& shape, size_t index);
};

/**
 * @brief Histogram meter for filled graphs
 */
class HistogramMeter : public MeterBase {
private:
    D2D1_COLOR_F primaryColor_;
    D2D1_COLOR_F secondaryColor_;
    bool flip_;
    bool autoScale_;
    std::vector<double> history_;
    size_t maxHistory_;
    
    ID2D1SolidColorBrush* primaryBrush_;
    ID2D1SolidColorBrush* secondaryBrush_;
    
public:
    explicit HistogramMeter(const std::wstring& name);
    ~HistogramMeter();
    
    void SetPrimaryColor(D2D1_COLOR_F color) { primaryColor_ = color; }
    void SetSecondaryColor(D2D1_COLOR_F color) { secondaryColor_ = color; }
    void SetFlip(bool flip) { flip_ = flip; }
    void SetMaxHistory(size_t max) { maxHistory_ = max; }
    
protected:
    bool OnInitialize(ID2D1RenderTarget* renderTarget) override;
    void OnUpdate() override;
    void OnRender(ID2D1RenderTarget* renderTarget) override;
    void OnShutdown() override;
};

/**
 * @brief Meter factory for creating meters by type
 */
class MeterFactory {
public:
    static std::unique_ptr<IMeter> CreateMeter(
        MeterType type,
        const std::wstring& name,
        const std::map<std::wstring, std::wstring>& config
    );
    
    static std::unique_ptr<IMeter> CreateFromConfig(
        const std::wstring& configString
    );
};

} // namespace Meters
} // namespace Widgets
} // namespace RainmeterManager
