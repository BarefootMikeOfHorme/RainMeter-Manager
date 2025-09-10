#include "dashboard_tab.h"
#include "ui_theme.h"
#include <commctrl.h>
#include <string>

#pragma comment(lib, "comctl32.lib")

using namespace RainmeterManager::UI;

static const wchar_t* kDashboardTabClass = L"RainmeterManagerDashboardTab";
static const wchar_t* kTileClass = L"RainmeterManagerTile";

namespace {
    ThemeColors gTheme{};
    HBRUSH gBgBrush = nullptr;
    HFONT gTileTitleFont = nullptr;
    HFONT gTileMetricFont = nullptr;

    LRESULT CALLBACK TileProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_NCCREATE:
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
            return TRUE;
        case WM_SETFONT:
            // Allow WM_SETFONT to pass through
            return 0;
        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wParam;
            RECT rc; GetClientRect(hwnd, &rc);
            HBRUSH hbr = CreateSolidBrush(gTheme.Background);
            FillRect(hdc, &rc, hbr);
            DeleteObject(hbr);
            return 1;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc; GetClientRect(hwnd, &rc);

            // Draw card background with rounded rect
            const int radius = 12;
            RECT card = rc; InflateRect(&card, -8, -8);

            HBRUSH cardBrush = CreateSolidBrush(gTheme.Card);
            HBRUSH borderBrush = CreateSolidBrush(gTheme.CardBorder);
            HPEN borderPen = CreatePen(PS_SOLID, 1, gTheme.CardBorder);
            HGDIOBJ oldPen = SelectObject(hdc, borderPen);
            HGDIOBJ oldBrush = SelectObject(hdc, cardBrush);
            RoundRect(hdc, card.left, card.top, card.right, card.bottom, radius, radius);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(cardBrush);
            DeleteObject(borderPen);
            DeleteObject(borderBrush);

            // Tile text: use window text
            wchar_t text[256];
            GetWindowTextW(hwnd, text, 256);

            // Split into title and metric by colon if present
            std::wstring s(text);
            std::wstring title = s;
            std::wstring metric;
            size_t pos = s.find(L":");
            if (pos != std::wstring::npos) {
                title = s.substr(0, pos);
                metric = s.substr(pos + 1);
                if (!metric.empty() && metric[0] == L' ') metric.erase(0, 1);
            }

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, gTheme.TextSecondary);
            HFONT oldFont = (HFONT)SelectObject(hdc, gTileTitleFont);

            RECT titleRc = card; titleRc.bottom = titleRc.top + (card.bottom - card.top) / 3;
            DrawTextW(hdc, title.c_str(), -1, &titleRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

            SetTextColor(hdc, gTheme.TextPrimary);
            SelectObject(hdc, gTileMetricFont);
            RECT metricRc = card; metricRc.top = titleRc.bottom + 6;
            DrawTextW(hdc, metric.empty() ? L"--" : metric.c_str(), -1, &metricRc, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

            SelectObject(hdc, oldFont);
            EndPaint(hwnd, &ps);
            return 0;
        }
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }
}

DashboardTab::DashboardTab(HINSTANCE hInstance, HWND hParentTab)
    : hInstance_(hInstance), hParentTab_(hParentTab) {}

DashboardTab::~DashboardTab() {}

bool DashboardTab::Create() {
    // Register parent (container) class
    WNDCLASS wc{};
    wc.lpfnWndProc = DashboardTab::WndProc;
    wc.hInstance = hInstance_;
    wc.lpszClassName = kDashboardTabClass;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr; // we handle erase to apply custom theme
    if (!RegisterClass(&wc)) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) return false;
    }

    // Register tile class once
    WNDCLASS wct{};
    wct.lpfnWndProc = TileProc;
    wct.hInstance = hInstance_;
    wct.lpszClassName = kTileClass;
    wct.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wct.hbrBackground = nullptr;
    RegisterClass(&wct);

    // Create theme resources
    if (!gBgBrush) gBgBrush = CreateSolidBrush(gTheme.Background);
    if (!gTileTitleFont) gTileTitleFont = CreateUIFont(11, FW_SEMIBOLD);
    if (!gTileMetricFont) gTileMetricFont = CreateUIFont(20, FW_BOLD);

    hwnd_ = CreateWindowEx(WS_EX_COMPOSITED, kDashboardTabClass, L"",
        WS_CHILD | WS_VISIBLE,
        0, 0, 100, 100,
        hParentTab_, nullptr, hInstance_, this);

    if (!hwnd_) return false;

    CreateChildren();
    return true;
}

void DashboardTab::Show() { if (hwnd_) ShowWindow(hwnd_, SW_SHOW); }
void DashboardTab::Hide() { if (hwnd_) ShowWindow(hwnd_, SW_HIDE); }

void DashboardTab::Resize(const RECT& rc) {
    if (!hwnd_) return;
    MoveWindow(hwnd_, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
    LayoutChildren();
}

LRESULT CALLBACK DashboardTab::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
    }
    auto self = reinterpret_cast<DashboardTab*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (self) return self->HandleMessage(hwnd, msg, wParam, lParam);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT DashboardTab::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        return 0;
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc; GetClientRect(hwnd_, &rc);
        FillRect(hdc, &rc, gBgBrush);
        return 1;
    }
    case WM_SIZE:
        LayoutChildren();
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

void DashboardTab::CreateChildren() {
    // Premium cards for CPU / Memory / Network (placeholders)
    hTileCpu_ = CreateWindowEx(0, kTileClass, L"CPU: --%",
        WS_CHILD | WS_VISIBLE,
        0, 0, 100, 24, hwnd_, nullptr, hInstance_, nullptr);

    hTileMem_ = CreateWindowEx(0, kTileClass, L"Memory: -- / -- MB",
        WS_CHILD | WS_VISIBLE,
        0, 0, 100, 24, hwnd_, nullptr, hInstance_, nullptr);

    hTileNet_ = CreateWindowEx(0, kTileClass, L"Network: Rx -- MB/s | Tx -- MB/s",
        WS_CHILD | WS_VISIBLE,
        0, 0, 100, 24, hwnd_, nullptr, hInstance_, nullptr);

    // Apply fonts
    SendMessageW(hTileCpu_, WM_SETFONT, (WPARAM)gTileTitleFont, TRUE);
    SendMessageW(hTileMem_, WM_SETFONT, (WPARAM)gTileTitleFont, TRUE);
    SendMessageW(hTileNet_, WM_SETFONT, (WPARAM)gTileTitleFont, TRUE);
}

void DashboardTab::LayoutChildren() {
    RECT rc; GetClientRect(hwnd_, &rc);
    int w = rc.right - rc.left;
    UNREFERENCED_PARAMETER(w);

    int margin = 16;
    int gap = 12;
    int tileH = 140;
    int tileW = (rc.right - rc.left - (margin * 2) - (gap * 2)) / 3;
    int y = margin;

    MoveWindow(hTileCpu_, margin, y, tileW, tileH, TRUE);
    MoveWindow(hTileMem_, margin + tileW + gap, y, tileW, tileH, TRUE);
    MoveWindow(hTileNet_, margin + (tileW + gap) * 2, y, tileW, tileH, TRUE);
}

