#include "options_window.h"
#include "ui_theme.h"
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

using namespace RainmeterManager::UI;

static const wchar_t* kOptionsClass = L"RainmeterManagerOptionsWindow";

namespace {
    ThemeColors gTheme{};
    HBRUSH gBgBrush = nullptr;
    HFONT gTabFont = nullptr;
}

OptionsWindow::OptionsWindow(HINSTANCE hInstance)
    : hInstance_(hInstance) {}

OptionsWindow::~OptionsWindow() {}

bool OptionsWindow::Create() {
    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_TAB_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASS wc{};
    wc.lpfnWndProc = OptionsWindow::WndProc;
    wc.hInstance = hInstance_;
    wc.lpszClassName = kOptionsClass;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr; // Custom dark background
    if (!RegisterClass(&wc)) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) return false;
    }

    if (!gBgBrush) gBgBrush = CreateSolidBrush(gTheme.Background);
    if (!gTabFont) gTabFont = CreateUIFont(10, FW_SEMIBOLD);

    hwnd_ = CreateWindowEx(
        WS_EX_APPWINDOW | WS_EX_COMPOSITED,
        kOptionsClass,
        L"RainmeterManager - Options",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 700,
        nullptr, nullptr, hInstance_, this);

    return hwnd_ != nullptr;
}

void OptionsWindow::Show(int nCmdShow) {
    if (!hwnd_) return;
    ShowWindow(hwnd_, nCmdShow);
    UpdateWindow(hwnd_);
}

void OptionsWindow::Hide() {
    if (hwnd_) ShowWindow(hwnd_, SW_HIDE);
}

LRESULT CALLBACK OptionsWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
    }
    auto self = reinterpret_cast<OptionsWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (self) return self->HandleMessage(hwnd, msg, wParam, lParam);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT OptionsWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateTabs();
        return 0;
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc; GetClientRect(hwnd_, &rc);
        FillRect(hdc, &rc, gBgBrush);
        return 1;
    }
    case WM_SIZE:
        ResizeChildren();
        return 0;
    case WM_NOTIFY:
        if (((LPNMHDR)lParam)->hwndFrom == hTab_ && ((LPNMHDR)lParam)->code == TCN_SELCHANGE) {
            ResizeChildren();
            return 0;
        }
        break;
    case WM_MEASUREITEM: {
        auto mis = reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);
        mis->itemHeight = 28; // fixed height tabs
        return TRUE;
    }
    case WM_DRAWITEM: {
        auto dis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
        if (dis->CtlType == ODT_TAB && dis->hwndItem == hTab_) {
            int idx = (int)dis->itemID;
            wchar_t text[64]{};
            TCITEM tie{}; tie.mask = TCIF_TEXT; tie.pszText = text; tie.cchTextMax = 63;
            TabCtrl_GetItem(hTab_, idx, &tie);

            HDC hdc = dis->hDC;
            RECT r = dis->rcItem;
            bool selected = (dis->itemState & ODS_SELECTED) != 0;

            // Background
            HBRUSH bg = CreateSolidBrush(gTheme.Background);
            FillRect(hdc, &r, bg);
            DeleteObject(bg);

            // Bottom border / accent for selected
            if (selected) {
                HPEN pen = CreatePen(PS_SOLID, 3, gTheme.Accent);
                HGDIOBJ oldPen = SelectObject(hdc, pen);
                MoveToEx(hdc, r.left + 6, r.bottom - 2, nullptr);
                LineTo(hdc, r.right - 6, r.bottom - 2);
                SelectObject(hdc, oldPen);
                DeleteObject(pen);
            }

            // Text
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, selected ? gTheme.TextPrimary : gTheme.TextSecondary);
            HFONT old = (HFONT)SelectObject(hdc, gTabFont);
            DrawTextW(hdc, text, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            SelectObject(hdc, old);
            return TRUE;
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

void OptionsWindow::CreateTabs() {
    RECT rc; GetClientRect(hwnd_, &rc);
    hTab_ = CreateWindowEx(0, WC_TABCONTROL, L"",
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | TCS_OWNERDRAWFIXED,
        0, 0, rc.right - rc.left, rc.bottom - rc.top,
        hwnd_, nullptr, hInstance_, nullptr);

    // Apply tab font
    SendMessageW(hTab_, WM_SETFONT, (WPARAM)gTabFont, TRUE);

    TCITEM tie{}; tie.mask = TCIF_TEXT;
    tie.pszText = const_cast<LPWSTR>(L"Dashboard");
    TabCtrl_InsertItem(hTab_, 0, &tie);
    tie.pszText = const_cast<LPWSTR>(L"Task Manager");
    TabCtrl_InsertItem(hTab_, 1, &tie);

    dashboardTab_ = std::make_unique<DashboardTab>(hInstance_, hTab_);
    taskTab_ = std::make_unique<TaskManagerTab>(hInstance_, hTab_);

    dashboardTab_->Create();
    taskTab_->Create();

    TabCtrl_SetCurSel(hTab_, 0);
    dashboardTab_->Show();
    taskTab_->Hide();
}

void OptionsWindow::ResizeChildren() {
    if (!hTab_) return;
    RECT rc; GetClientRect(hwnd_, &rc);
    MoveWindow(hTab_, 0, 0, rc.right - rc.left, rc.bottom - rc.top, TRUE);
    RECT tabRc; GetClientRect(hTab_, &tabRc);
    RECT dispRc{ 10, 36, tabRc.right - 10, tabRc.bottom - 10 };
    dashboardTab_->Resize(dispRc);
    taskTab_->Resize(dispRc);

    // Show correct child based on selected tab
    int sel = TabCtrl_GetCurSel(hTab_);
    if (sel == 0) { dashboardTab_->Show(); taskTab_->Hide(); }
    else { taskTab_->Show(); dashboardTab_->Hide(); }
}

