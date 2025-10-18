#ifndef UI_THEME_H
#define UI_THEME_H

#include <windows.h>

namespace RainmeterManager {
namespace UI {

// Simple design tokens for a premium enterprise look
struct ThemeColors {
    COLORREF Background = RGB(18, 18, 20);       // Window background
    COLORREF Card = RGB(32, 34, 37);             // Tile/card surface
    COLORREF CardBorder = RGB(58, 60, 65);       // Card border outline
    COLORREF Accent = RGB(0, 122, 204);          // Accent (links/indicators)
    COLORREF TextPrimary = RGB(230, 234, 239);   // Primary text
    COLORREF TextSecondary = RGB(160, 165, 175); // Secondary text
};

inline int DpiScale(HWND hwnd, int value) {
    UINT dpi = 96;
    if (hwnd) {
        // GetDpiForWindow is available on Windows 10 1607+
        typedef UINT (WINAPI *GetDpiForWindowFunc)(HWND);
        static GetDpiForWindowFunc pGetDpiForWindow = 
            (GetDpiForWindowFunc)GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow");
        if (pGetDpiForWindow) {
            dpi = pGetDpiForWindow(hwnd);
        }
    }
    return MulDiv(value, dpi, 96);
}

// Font helpers
inline HFONT CreateUIFont(int pt, int weight = FW_SEMIBOLD, bool italic = false) {
    // Convert pt to logical height: -MulDiv(PointSize, DPI, 72)
    HDC hdc = GetDC(nullptr);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(nullptr, hdc);
    const int height = -MulDiv(pt, dpi, 72);
    return CreateFontW(height, 0, 0, 0, weight, italic, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_NATURAL_QUALITY, VARIABLE_PITCH, L"Segoe UI");
}

} // namespace UI
} // namespace RainmeterManager

#endif // UI_THEME_H

