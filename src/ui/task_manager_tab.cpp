#include "task_manager_tab.h"
#include <commctrl.h>
#include <string>

#pragma comment(lib, "comctl32.lib")

using namespace RainmeterManager::UI;

static const wchar_t* kTaskManagerTabClass = L"RainmeterManagerTaskManagerTab";

TaskManagerTab::TaskManagerTab(HINSTANCE hInstance, HWND hParentTab)
    : hInstance_(hInstance), hParentTab_(hParentTab) {}

TaskManagerTab::~TaskManagerTab() {}

bool TaskManagerTab::Create() {
    WNDCLASS wc{};
    wc.lpfnWndProc = TaskManagerTab::WndProc;
    wc.hInstance = hInstance_;
    wc.lpszClassName = kTaskManagerTabClass;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    if (!RegisterClass(&wc)) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) return false;
    }

    hwnd_ = CreateWindowEx(0, kTaskManagerTabClass, L"",
        WS_CHILD | WS_VISIBLE,
        0, 0, 100, 100,
        hParentTab_, nullptr, hInstance_, this);

    if (!hwnd_) return false;

    CreateChildren();
    return true;
}

void TaskManagerTab::Show() { if (hwnd_) ShowWindow(hwnd_, SW_SHOW); }
void TaskManagerTab::Hide() { if (hwnd_) ShowWindow(hwnd_, SW_HIDE); }

void TaskManagerTab::Resize(const RECT& rc) {
    if (!hwnd_) return;
    MoveWindow(hwnd_, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
    LayoutChildren();
}

LRESULT CALLBACK TaskManagerTab::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
    }
    auto self = reinterpret_cast<TaskManagerTab*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (self) return self->HandleMessage(hwnd, msg, wParam, lParam);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT TaskManagerTab::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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

void TaskManagerTab::CreateChildren() {
    hList_ = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        0, 0, 100, 100, hwnd_, nullptr, hInstance_, nullptr);

    InitListView();
    PopulatePlaceholder();
}

void TaskManagerTab::InitListView() {
    ListView_SetExtendedListViewStyle(hList_, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);

    LVCOLUMN col{}; col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    int idx = 0;

    col.pszText = const_cast<LPWSTR>(L"PID"); col.cx = 80; col.iSubItem = idx; ListView_InsertColumn(hList_, idx++, &col);
    col.pszText = const_cast<LPWSTR>(L"Name"); col.cx = 220; col.iSubItem = idx; ListView_InsertColumn(hList_, idx++, &col);
    col.pszText = const_cast<LPWSTR>(L"CPU %"); col.cx = 80; col.iSubItem = idx; ListView_InsertColumn(hList_, idx++, &col);
    col.pszText = const_cast<LPWSTR>(L"Memory MB"); col.cx = 120; col.iSubItem = idx; ListView_InsertColumn(hList_, idx++, &col);
    col.pszText = const_cast<LPWSTR>(L"Threads"); col.cx = 80; col.iSubItem = idx; ListView_InsertColumn(hList_, idx++, &col);
}

void TaskManagerTab::PopulatePlaceholder() {
    LVITEM item{}; item.mask = LVIF_TEXT; item.iItem = 0; item.iSubItem = 0;
    item.pszText = const_cast<LPWSTR>(L"1234"); ListView_InsertItem(hList_, &item);
    ListView_SetItemText(hList_, 0, 1, const_cast<LPWSTR>(L"example.exe"));
    ListView_SetItemText(hList_, 0, 2, const_cast<LPWSTR>(L"0.0"));
    ListView_SetItemText(hList_, 0, 3, const_cast<LPWSTR>(L"12.3"));
    ListView_SetItemText(hList_, 0, 4, const_cast<LPWSTR>(L"8"));
}

void TaskManagerTab::LayoutChildren() {
    RECT rc; GetClientRect(hwnd_, &rc);
    MoveWindow(hList_, 10, 10, rc.right - rc.left - 20, rc.bottom - rc.top - 20, TRUE);
}

