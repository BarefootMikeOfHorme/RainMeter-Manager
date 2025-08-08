#ifndef UI_FRAMEWORK_H
#define UI_FRAMEWORK_H

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "logger.h"
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

// Modern UI Theme Structure
struct UITheme {
    COLORREF backgroundColor = RGB(45, 45, 48);      // Dark theme background
    COLORREF foregroundColor = RGB(255, 255, 255);   // White text
    COLORREF accentColor = RGB(0, 120, 215);         // Windows blue accent
    COLORREF borderColor = RGB(76, 76, 76);          // Border color
    COLORREF hoverColor = RGB(62, 62, 64);           // Hover state
    COLORREF selectedColor = RGB(51, 153, 255);      // Selection color
    std::string fontFamily = "Segoe UI";
    int fontSize = 12;
    bool darkMode = true;
};

// UI Component Base Class
class UIComponent {
public:
    virtual ~UIComponent() = default;
    virtual bool create(HWND parent, int x, int y, int width, int height) = 0;
    virtual void setTheme(const UITheme& theme) = 0;
    virtual void show(bool visible = true) = 0;
    virtual HWND getHandle() const = 0;
    
protected:
    HWND hwnd = nullptr;
    UITheme currentTheme;
};

// Modern Button Component
class ModernButton : public UIComponent {
public:
    ModernButton(const std::string& text, std::function<void()> onClick = nullptr);
    bool create(HWND parent, int x, int y, int width, int height) override;
    void setTheme(const UITheme& theme) override;
    void show(bool visible = true) override;
    HWND getHandle() const override { return hwnd; }
    void setText(const std::string& text);
    void setIcon(HICON icon);
    
private:
    std::string buttonText;
    std::function<void()> clickHandler;
    HICON buttonIcon = nullptr;
    static LRESULT CALLBACK buttonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void drawButton(HDC hdc, RECT& rect, bool isHovered, bool isPressed);
};

// Modern List View Component
class ModernListView : public UIComponent {
public:
    struct ListItem {
        std::string text;
        std::string subText;
        HICON icon;
        void* userData;
    };
    
    ModernListView();
    bool create(HWND parent, int x, int y, int width, int height) override;
    void setTheme(const UITheme& theme) override;
    void show(bool visible = true) override;
    HWND getHandle() const override { return hwnd; }
    
    void addItem(const ListItem& item);
    void clearItems();
    ListItem* getSelectedItem();
    void setOnSelectionChanged(std::function<void(ListItem*)> callback);
    
private:
    std::vector<ListItem> items;
    std::function<void(ListItem*)> selectionCallback;
    static LRESULT CALLBACK listViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

// Modern Text Editor Component (with syntax highlighting capability)
class ModernTextEditor : public UIComponent {
public:
    ModernTextEditor();
    bool create(HWND parent, int x, int y, int width, int height) override;
    void setTheme(const UITheme& theme) override;
    void show(bool visible = true) override;
    HWND getHandle() const override { return hwnd; }
    
    void setText(const std::string& text);
    std::string getText() const;
    void setSyntaxHighlighting(bool enable);
    void setReadOnly(bool readOnly);
    
private:
    bool syntaxHighlightingEnabled = false;
    static LRESULT CALLBACK editorProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

// Modern Progress Bar Component
class ModernProgressBar : public UIComponent {
public:
    ModernProgressBar();
    bool create(HWND parent, int x, int y, int width, int height) override;
    void setTheme(const UITheme& theme) override;
    void show(bool visible = true) override;
    HWND getHandle() const override { return hwnd; }
    
    void setProgress(int percentage);
    void setIndeterminate(bool indeterminate);
    void setText(const std::string& text);
    
private:
    int currentProgress = 0;
    bool isIndeterminate = false;
    std::string progressText;
    static LRESULT CALLBACK progressProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

// Main UI Manager Class
class UIManager {
public:
    static UIManager& getInstance();
    
    // Theme management
    void setGlobalTheme(const UITheme& theme);
    UITheme& getGlobalTheme() { return globalTheme; }
    void toggleDarkMode();
    
    // Window management
    bool createMainWindow(HINSTANCE hInstance, int cmdShow);
    void setupLayout();
    void updateLayout();
    
    // Component management
    void registerComponent(std::shared_ptr<UIComponent> component);
    void applyThemeToAll();
    
    // Event handling
    static LRESULT CALLBACK mainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Animation support
    void animateComponent(HWND hwnd, int fromX, int fromY, int toX, int toY, DWORD duration);
    
    // Accessibility support
    void enableAccessibility();
    
private:
    UIManager() = default;
    static UIManager* instance;
    
    HWND mainWindow = nullptr;
    UITheme globalTheme;
    std::vector<std::shared_ptr<UIComponent>> components;
    
    // Layout management
    void calculateLayout(int windowWidth, int windowHeight);
};

// UI Builder Pattern for easy component creation
class UIBuilder {
public:
    static std::shared_ptr<ModernButton> createButton(const std::string& text);
    static std::shared_ptr<ModernListView> createListView();
    static std::shared_ptr<ModernTextEditor> createTextEditor();
    static std::shared_ptr<ModernProgressBar> createProgressBar();
};

#endif // UI_FRAMEWORK_H
