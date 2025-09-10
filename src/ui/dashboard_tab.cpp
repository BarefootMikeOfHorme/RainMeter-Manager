#include "dashboard_tab.h"
#include <commctrl.h>
#include <string>

#pragma comment(lib, "comctl32.lib")

using namespace RainmeterManager::UI;

static const wchar_t* kDashboardTabClass = L"RainmeterManagerDashboardTab";

DashboardTab::DashboardTab(HINSTANCE hInstance, HWND hParentTab)
    : hInstance_(hInstance), hParentTab_(hParentTab) {}

DashboardTab::~DashboardTab() {}

bool DashboardTab::Create() {
    WNDCLASS wc{};
    wc.lpfnWndProc = DashboardTab::WndProc;
    wc.hInstance = hInstance_;
    wc.lpszClassName = kDashboardTabClass;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    if (!RegisterClass(&wc)) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) return false;
    }

    hwnd_ = CreateWindowEx(0, kDashboardTabClass, L"",
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
    case WM_SIZE:
        LayoutChildren();
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

void DashboardTab::CreateChildren() {
    // Simple tiles for CPU / Memory / Network (placeholders)
    hTileCpu_ = CreateWindowEx(WS_EX_CLIENTEDGE, L"STATIC", L"CPU: --%",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        0, 0, 100, 24, hwnd_, nullptr, hInstance_, nullptr);

    hTileMem_ = CreateWindowEx(WS_EX_CLIENTEDGE, L"STATIC", L"Memory: --/-- MB",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        0, 0, 100, 24, hwnd_, nullptr, hInstance_, nullptr);

    hTileNet_ = CreateWindowEx(WS_EX_CLIENTEDGE, L"STATIC", L"Network: Rx -- MB/s | Tx -- MB/s",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        0, 0, 100, 24, hwnd_, nullptr, hInstance_, nullptr);
}

void DashboardTab::LayoutChildren() {
    RECT rc; GetClientRect(hwnd_, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    int tileW = (w - 40) / 3;
    int tileH = 120;
    int y = 20;

    MoveWindow(hTileCpu_, 10, y, tileW, tileH, TRUE);
    MoveWindow(hTileMem_, 20 + tileW, y, tileW, tileH, TRUE);
    MoveWindow(hTileNet_, 30 + tileW * 2, y, tileW, tileH, TRUE);
}

