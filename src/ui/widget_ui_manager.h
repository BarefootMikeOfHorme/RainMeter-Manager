#ifndef WIDGET_UI_MANAGER_H
#define WIDGET_UI_MANAGER_H

#include "widget_framework.h"
#include <d2d1.h>
#include <dwrite.h>
#include <map>
#include <memory>

// Widget display modes
enum class WidgetDisplayMode {
    NORMAL,           // Regular widget in dashboard
    OVERLAY,          // Floating overlay on desktop
    FULLSCREEN,       // Full screen mode
    MINIMIZED,        // Minimized to taskbar/tray
    DASHBOARD_TILE    // Part of dashboard grid
};

// Widget interaction states
enum class WidgetInteractionState {
    IDLE,
    HOVER,
    DRAGGING,
    RESIZING,
    SELECTED
};

// Widget layout information
struct WidgetLayout {
    int x, y;              // Position
    int width, height;     // Size
    int minWidth = 200;    // Minimum dimensions
    int minHeight = 150;
    int maxWidth = 1920;   // Maximum dimensions  
    int maxHeight = 1080;
    bool isResizable = true;
    bool isDraggable = true;
    WidgetDisplayMode displayMode = WidgetDisplayMode::NORMAL;
    int zOrder = 0;        // Layering order
};

// Resize handle positions
enum class ResizeHandle {
    NONE = 0,
    TOP_LEFT = 1,
    TOP = 2,
    TOP_RIGHT = 3,
    RIGHT = 4,
    BOTTOM_RIGHT = 5,
    BOTTOM = 6,
    BOTTOM_LEFT = 7,
    LEFT = 8
};

// Widget UI manager for advanced interaction
class WidgetUIManager {
public:
    static WidgetUIManager& getInstance();
    
    // Widget registration and management
    bool registerWidget(std::shared_ptr<BaseWidget> widget, const WidgetLayout& layout);
    bool unregisterWidget(const std::string& widgetId);
    std::shared_ptr<BaseWidget> getWidget(const std::string& widgetId);
    
    // Layout management
    void setWidgetLayout(const std::string& widgetId, const WidgetLayout& layout);
    WidgetLayout getWidgetLayout(const std::string& widgetId);
    void arrangeWidgetsInGrid(int columns, int rows, int spacing = 10);
    void arrangeWidgetsInStack(bool vertical = true, int spacing = 5);
    void saveLayoutToFile(const std::string& filename);
    bool loadLayoutFromFile(const std::string& filename);
    
    // Display modes
    void setWidgetDisplayMode(const std::string& widgetId, WidgetDisplayMode mode);
    void toggleFullScreen(const std::string& widgetId);
    void toggleOverlayMode(const std::string& widgetId);
    void createDashboardView();
    void minimizeWidget(const std::string& widgetId);
    void restoreWidget(const std::string& widgetId);
    
    // Interaction handling
    void enableDragAndDrop(bool enable = true);
    void enableResizing(bool enable = true);
    void setSelectionMode(bool multiSelect = false);
    void selectWidget(const std::string& widgetId, bool addToSelection = false);
    void deselectWidget(const std::string& widgetId);
    void clearSelection();
    std::vector<std::string> getSelectedWidgets();
    
    // Visual enhancements
    void setTheme(const UITheme& theme);
    void enableAnimations(bool enable = true);
    void setAnimationDuration(DWORD milliseconds = 250);
    void enableSnapToGrid(bool enable = true, int gridSize = 20);
    void showWidgetBorders(bool show = true);
    void showResizeHandles(bool show = true);
    
    // Event handling
    void setOnWidgetMoved(std::function<void(const std::string&, int, int)> callback);
    void setOnWidgetResized(std::function<void(const std::string&, int, int)> callback);
    void setOnWidgetSelected(std::function<void(const std::string&)> callback);
    void setOnDisplayModeChanged(std::function<void(const std::string&, WidgetDisplayMode)> callback);
    
    // Main window procedure integration
    LRESULT handleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Rendering
    void renderAllWidgets(ID2D1RenderTarget* renderTarget);
    void renderWidget(const std::string& widgetId, ID2D1RenderTarget* renderTarget);
    void renderSelectionIndicators(ID2D1RenderTarget* renderTarget);
    void renderResizeHandles(const std::string& widgetId, ID2D1RenderTarget* renderTarget);
    
private:
    WidgetUIManager() = default;
    static WidgetUIManager* instance;
    
    // Widget storage
    std::map<std::string, std::shared_ptr<BaseWidget>> widgets;
    std::map<std::string, WidgetLayout> widgetLayouts;
    std::map<std::string, WidgetInteractionState> widgetStates;
    std::vector<std::string> selectedWidgets;
    
    // Interaction state
    bool dragDropEnabled = true;
    bool resizingEnabled = true;
    bool multiSelectMode = false;
    bool animationsEnabled = true;
    bool snapToGridEnabled = false;
    bool showBorders = true;
    bool showHandles = true;
    int gridSize = 20;
    DWORD animationDuration = 250;
    
    // Current interaction
    std::string activeWidget;
    ResizeHandle activeResizeHandle = ResizeHandle::NONE;
    POINT lastMousePos = {0, 0};
    POINT dragStartPos = {0, 0};
    WidgetLayout originalLayout;
    
    // Theme and rendering
    UITheme currentTheme;
    ID2D1Factory* d2dFactory = nullptr;
    ID2D1SolidColorBrush* borderBrush = nullptr;
    ID2D1SolidColorBrush* selectionBrush = nullptr;
    ID2D1SolidColorBrush* handleBrush = nullptr;
    IDWriteFactory* writeFactory = nullptr;
    IDWriteTextFormat* textFormat = nullptr;
    
    // Event callbacks
    std::function<void(const std::string&, int, int)> onWidgetMoved;
    std::function<void(const std::string&, int, int)> onWidgetResized;
    std::function<void(const std::string&)> onWidgetSelected;
    std::function<void(const std::string&, WidgetDisplayMode)> onDisplayModeChanged;
    
    // Helper methods
    void initializeDirect2D();
    void cleanupDirect2D();
    void updateWidgetBrushes();
    
    // Interaction helpers
    std::string getWidgetAtPoint(POINT pt);
    ResizeHandle getResizeHandleAtPoint(const std::string& widgetId, POINT pt);
    bool isPointInWidget(const std::string& widgetId, POINT pt);
    RECT getWidgetRect(const std::string& widgetId);
    RECT getResizeHandleRect(const std::string& widgetId, ResizeHandle handle);
    
    // Mouse event handlers
    void onMouseMove(POINT pt);
    void onLeftButtonDown(POINT pt);
    void onLeftButtonUp(POINT pt);
    void onRightButtonDown(POINT pt);
    void onDoubleClick(POINT pt);
    
    // Drag and drop
    void startDragging(const std::string& widgetId, POINT startPt);
    void updateDragging(POINT currentPt);
    void endDragging();
    
    // Resizing
    void startResizing(const std::string& widgetId, ResizeHandle handle, POINT startPt);
    void updateResizing(POINT currentPt);
    void endResizing();
    
    // Snapping
    POINT snapToGrid(POINT pt);
    POINT snapToOtherWidgets(const std::string& widgetId, POINT pt);
    
    // Animation
    void animateWidgetToPosition(const std::string& widgetId, int targetX, int targetY);
    void animateWidgetToSize(const std::string& widgetId, int targetWidth, int targetHeight);
    
    // Layout helpers
    void updateWidgetZOrder();
    void bringWidgetToFront(const std::string& widgetId);
    void sendWidgetToBack(const std::string& widgetId);
    
    // Context menu
    void showContextMenu(const std::string& widgetId, POINT pt);
    void handleContextMenuCommand(const std::string& widgetId, int commandId);
};

// Context menu commands
enum class WidgetContextCommand {
    TOGGLE_FULLSCREEN = 1000,
    TOGGLE_OVERLAY = 1001,
    MINIMIZE = 1002,
    CLOSE = 1003,
    PROPERTIES = 1004,
    BRING_TO_FRONT = 1005,
    SEND_TO_BACK = 1006,
    DUPLICATE = 1007,
    REFRESH = 1008
};

// Utility functions for widget UI management
namespace WidgetUIUtils {
    // Layout calculations
    WidgetLayout calculateOptimalLayout(int screenWidth, int screenHeight, int widgetCount);
    std::vector<WidgetLayout> distributeWidgetsEvenly(int screenWidth, int screenHeight, 
                                                    const std::vector<std::string>& widgetIds);
    
    // Theme utilities
    D2D1_COLOR_F COLORREFToD2D1(COLORREF color, float alpha = 1.0f);
    COLORREF D2D1ToCOLORREF(D2D1_COLOR_F color);
    
    // Animation easing functions
    float easeInOutQuad(float t);
    float easeInOutCubic(float t);
    float easeOutBounce(float t);
    
    // Collision detection
    bool rectsIntersect(const RECT& rect1, const RECT& rect2);
    RECT getIntersection(const RECT& rect1, const RECT& rect2);
    
    // Screen utilities
    RECT getPrimaryMonitorRect();
    std::vector<RECT> getAllMonitorRects();
    RECT getWorkArea();
}

#endif // WIDGET_UI_MANAGER_H
