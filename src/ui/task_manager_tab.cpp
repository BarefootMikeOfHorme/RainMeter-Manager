#include "task_manager_tab.h"
#include "../render/ipc/render_ipc_bridge.h"
#include "../render/interfaces/render_command.h"
#include "ui_theme.h"
#include "ui_prefs.h"
#include <commctrl.h>
#include <windowsx.h>
#include <string>
#include <vector>
#include <algorithm>
#include <deque>
#include <unordered_map>
#include "dashboard_tab.h"

#pragma comment(lib, "comctl32.lib")

using namespace RainmeterManager::UI;

static const wchar_t* kTaskManagerTabClass = L"RainmeterManagerTaskManagerTab";
static const wchar_t* kTaskSummaryClass = L"RainmeterManagerTaskSummaryPanel";

namespace {
    ThemeColors gTheme{};
}

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

    // Register summary panel
    WNDCLASS wcs{};
    wcs.lpfnWndProc = TaskManagerTab::SummaryProc;
    wcs.hInstance = hInstance_;
    wcs.lpszClassName = kTaskSummaryClass;
    wcs.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcs.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wcs);

    CreateChildren();
    StartPolling();
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
    case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == kIdSearchEdit) {
            wchar_t buf[512];
            GetWindowTextW(hSearch_, buf, 512);
            filterText_ = buf;
            RebuildDisplay();
            RainmeterManager::UI::TaskManagerPrefs p{colVisible_, sortColumn_, sortAsc_, filterText_};
            SaveTaskManagerPrefs(p);
            return 0;
        } else if (HIWORD(wParam) == 0) {
            switch (LOWORD(wParam)) {
                case kIdCtxOpenLocation: {
                    int sel = ListView_GetNextItem(hList_, -1, LVNI_SELECTED);
                    if (sel >= 0 && sel < (int)displayRows_.size()) {
                        std::wstring path = displayRows_[sel].path;
                        if (!path.empty()) {
                            std::wstring args = L"/select,\"" + path + L"\"";
                            ShellExecuteW(nullptr, L"open", L"explorer.exe", args.c_str(), nullptr, SW_SHOWNORMAL);
                        }
                    }
                    return 0;
                }
                case kIdCtxCopyCmd: {
                    int sel = ListView_GetNextItem(hList_, -1, LVNI_SELECTED);
                    if (sel >= 0 && sel < (int)displayRows_.size()) {
                        std::wstring cmd = displayRows_[sel].cmd;
                        if (OpenClipboard(hwnd_)) {
                            EmptyClipboard();
                            size_t bytes = (cmd.size() + 1) * sizeof(wchar_t);
                            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
                            if (hMem) {
                                void* p = GlobalLock(hMem);
                                memcpy(p, cmd.c_str(), bytes);
                                GlobalUnlock(hMem);
                                SetClipboardData(CF_UNICODETEXT, hMem);
                            }
                            CloseClipboard();
                        }
                    }
                    return 0;
                }
                case kIdCtxProperties: {
                    int sel = ListView_GetNextItem(hList_, -1, LVNI_SELECTED);
                    if (sel >= 0 && sel < (int)displayRows_.size()) {
                        std::wstring path = displayRows_[sel].path;
                        if (!path.empty()) {
                            SHELLEXECUTEINFOW sei{}; sei.cbSize = sizeof(sei);
                            sei.fMask = SEE_MASK_INVOKEIDLIST;
                            sei.hwnd = hwnd_;
                            sei.lpVerb = L"properties";
                            sei.lpFile = path.c_str();
                            sei.nShow = SW_SHOW;
                            ShellExecuteExW(&sei);
                        }
                    }
                    return 0;
                }
            }
        }
        break;
    case WM_NOTIFY: {
        auto hdr = reinterpret_cast<LPNMHDR>(lParam);
        if (hdr->hwndFrom == hList_) {
            if (hdr->code == LVN_COLUMNCLICK) {
                auto nmlv = reinterpret_cast<LPNMLISTVIEW>(lParam);
                int col = nmlv->iSubItem;
                if (sortColumn_ == col) sortAsc_ = !sortAsc_; else { sortColumn_ = col; sortAsc_ = (col == 1); }
                RebuildDisplay();
                RainmeterManager::UI::TaskManagerPrefs p{colVisible_, sortColumn_, sortAsc_, filterText_};
                SaveTaskManagerPrefs(p);
                return 0;
            }
            if (hdr->code == LVN_ITEMCHANGED) {
                auto nmlv = reinterpret_cast<LPNMLISTVIEW>(lParam);
                if ((nmlv->uChanged & LVIF_STATE) && ((nmlv->uNewState ^ nmlv->uOldState) & LVIS_SELECTED)) {
                    UpdateDetailsFromSelection();
                    return 0;
                }
            }
        }
        break;
    }
    case WM_TIMER:
        if (wParam == 1002) { RequestAndUpdateProcessList(); return 0; }
        break;
    case WM_CONTEXTMENU: {
        HWND from = (HWND)wParam;
        if (from == hList_) {
            // Context menu for list rows
            HMENU menu = CreatePopupMenu();
            AppendMenuW(menu, MF_STRING, kIdCtxOpenLocation, L"Open file location");
            AppendMenuW(menu, MF_STRING, kIdCtxCopyCmd, L"Copy command line");
            AppendMenuW(menu, MF_STRING, kIdCtxProperties, L"Properties");
            TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hwnd_, nullptr);
            DestroyMenu(menu);
            return 0;
        } else {
            // Possibly header; show column chooser
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ShowHeaderContextMenu(pt);
            return 0;
        }
    }
    case WM_DESTROY:
        StopPolling();
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

LRESULT CALLBACK TaskManagerTab::SummaryProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
    }
    auto self = reinterpret_cast<TaskManagerTab*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        if (self) self->DrawSummary(hdc, rc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

void TaskManagerTab::DrawSummary(HDC hdc, const RECT& rc) {
    HBRUSH bg = CreateSolidBrush(RGB(32,34,37));
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    int pad = 8;
    int chartW = (w - pad * 4) / 3;

    auto drawChart = [&](int x, int y, int cw, int ch, const std::deque<double>& data, COLORREF color, const wchar_t* label, const wchar_t* value) {
        RECT box{ x, y, x + cw, y + ch };
        HBRUSH boxBg = CreateSolidBrush(RGB(45,47,51)); FillRect(hdc, &box, boxBg); DeleteObject(boxBg);
        Rectangle(hdc, box.left, box.top, box.right, box.bottom);
        // label
        SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, RGB(160,165,175));
        RECT lr{ box.left + 6, box.top + 4, box.right - 6, box.top + 20 }; DrawTextW(hdc, label, -1, &lr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        // value
        SetTextColor(hdc, RGB(230,234,239)); HFONT font = CreateUIFont(12, FW_BOLD); HGDIOBJ of = SelectObject(hdc, font);
        RECT vr{ box.left + 6, box.top + 22, box.right - 6, box.top + 40 }; DrawTextW(hdc, value, -1, &vr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(hdc, of); DeleteObject(font);
        // sparkline
        if (data.size() > 1) {
            HPEN pen = CreatePen(PS_SOLID, 1, color); HGDIOBJ op = SelectObject(hdc, pen);
            int n = (int)data.size(); int gw = cw - 12; int gh = ch - 44;
            int gx = box.left + 6; int gy = box.top + 44;
            for (int i = 0; i < n; ++i) {
                double v = data[i]; if (v < 0) v = 0; if (v > 100000) v = 100000; // mem/net scales
                int x = gx + (i * gw) / (kHistLen - 1);
                int y = gy + gh;
                // scale differently based on label
                if (wcscmp(label, L"CPU") == 0) {
                    y = gy + gh - (int)((v / 100.0) * gh);
                } else if (wcscmp(label, L"Mem") == 0 && sysMemTotalMB_ > 0) {
                    double pct = v / sysMemTotalMB_; if (pct > 1) pct = 1; y = gy + gh - (int)(pct * gh);
                } else if (wcscmp(label, L"Net") == 0) {
                    double scaled = min(1.0, v / 50.0); y = gy + gh - (int)(scaled * gh); // scale to 50 MB/s
                }
                if (i == 0) MoveToEx(hdc, x, y, nullptr); else LineTo(hdc, x, y);
            }
            SelectObject(hdc, op); DeleteObject(pen);
        }
    };

    // Compute latest values
    double cpu = sysCpuHist_.empty() ? 0.0 : sysCpuHist_.back();
    double mem = sysMemHist_.empty() ? 0.0 : sysMemHist_.back();
    double net = sysNetHist_.empty() ? 0.0 : sysNetHist_.back();

    wchar_t v1[64]; swprintf(v1, 64, L"%.1f%%", cpu);
    wchar_t v2[64]; if (sysMemTotalMB_ > 0) swprintf(v2, 64, L"%.0f / %.0f MB", mem, sysMemTotalMB_); else swprintf(v2, 64, L"--");
    wchar_t v3[64]; swprintf(v3, 64, L"%.2f MB/s", net);

    int x = rc.left + pad; int y = rc.top + pad;
    drawChart(x, y, chartW, h - pad * 2, sysCpuHist_, RGB(80,160,255), L"CPU", v1);
    drawChart(x + chartW + pad, y, chartW, h - pad * 2, sysMemHist_, RGB(0,200,140), L"Mem", v2);
    drawChart(x + (chartW + pad) * 2, y, chartW, h - pad * 2, sysNetHist_, RGB(255,160,90), L"Net", v3);
}

static bool ExtractDoubleAnywhere(const std::string& json, const char* keyPascal, const char* keyCamel, double& outVal) {
    auto findKey = [&](const char* k) -> size_t {
        std::string needle = std::string("\"") + k + "\"";
        return json.find(needle);
    };
    size_t pos = findKey(keyPascal);
    if (pos == std::string::npos && keyCamel) pos = findKey(keyCamel);
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return false;
    ++pos; while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    size_t end = pos;
    while (end < json.size() && (isdigit((unsigned char)json[end]) || json[end] == '.' || json[end] == 'e' || json[end] == 'E' || json[end] == '+' || json[end] == '-')) ++end;
    if (end <= pos) return false;
    try { outVal = std::stod(json.substr(pos, end - pos)); return true; } catch (...) { return false; }
}

void TaskManagerTab::FetchSystemSnapshot() {
    RainmeterManager::Render::RenderCommand cmd{};
    cmd.commandId = ++localCommandId_;
    cmd.commandType = RainmeterManager::Render::RenderCommandType::GetSystemSnapshot;
    cmd.widgetId = 0;
    cmd.windowHandle = hwnd_;
    cmd.backendType = RainmeterManager::Render::RenderBackendType::Auto;
    cmd.bounds = {0,0,0,0};
    cmd.timestamp = (uint64_t)GetTickCount64();
    auto result = ipc_->SendCommand(cmd, 1000);
    if (result.status != RainmeterManager::Render::RenderResultStatus::Success) return;
    const std::string& json = result.errorMessage;
    double cpu=0, memUsed=0, memTotal=0, rx=0, tx=0;
    ExtractDoubleAnywhere(json, "CpuTotalPercent", "cpuTotalPercent", cpu);
    ExtractDoubleAnywhere(json, "MemoryUsedMB", "memoryUsedMB", memUsed);
    ExtractDoubleAnywhere(json, "MemoryTotalMB", "memoryTotalMB", memTotal);
    ExtractDoubleAnywhere(json, "NetworkRecvMBps", "networkRecvMBps", rx);
    ExtractDoubleAnywhere(json, "NetworkSendMBps", "networkSendMBps", tx);
    sysMemTotalMB_ = memTotal;
    sysCpuHist_.push_back(cpu);
    sysMemHist_.push_back(memUsed);
    sysNetHist_.push_back(rx + tx);
    while ((int)sysCpuHist_.size() > kHistLen) sysCpuHist_.pop_front();
    while ((int)sysMemHist_.size() > kHistLen) sysMemHist_.pop_front();
    while ((int)sysNetHist_.size() > kHistLen) sysNetHist_.pop_front();
    if (hSummary_) InvalidateRect(hSummary_, nullptr, FALSE);
}

void TaskManagerTab::CreateChildren() {
    // Search box
    hSearch_ = CreateWindowEx(0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        0, 0, 100, 24, hwnd_, (HMENU)kIdSearchEdit, hInstance_, nullptr);

    // Summary panel
    hSummary_ = CreateWindowEx(0, kTaskSummaryClass, L"",
        WS_CHILD | WS_VISIBLE,
        0, 0, 100, 60, hwnd_, nullptr, hInstance_, this);

    // List view
    hList_ = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        0, 0, 100, 100, hwnd_, nullptr, hInstance_, nullptr);

    // Load user prefs
    RainmeterManager::UI::TaskManagerPrefs prefs;
    if (LoadTaskManagerPrefs(prefs) && !prefs.columns.empty()) {
        colVisible_ = prefs.columns;
        sortColumn_ = prefs.sortColumn;
        sortAsc_ = prefs.sortAsc;
        filterText_ = prefs.filterText;
        if (!filterText_.empty()) SetWindowTextW(hSearch_, filterText_.c_str());
    }

    InitListView();
    CreateDetailsPane();
}

void TaskManagerTab::InitListView() {
    ListView_SetExtendedListViewStyle(hList_, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);
    BuildColumns();
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
    int margin = 10;
    int searchH = 26;
    int summaryH = 64;
    int detailsH = 220;
    MoveWindow(hSearch_, margin, margin, rc.right - rc.left - margin * 2, searchH, TRUE);
    MoveWindow(hSummary_, margin, margin + searchH + 6, rc.right - rc.left - margin * 2, summaryH, TRUE);
    MoveWindow(hList_, margin, margin + searchH + 6 + summaryH + 6, rc.right - rc.left - margin * 2, rc.bottom - rc.top - (margin + searchH + 6 + summaryH + 6) - margin - detailsH, TRUE);

    // Details pane layout - simple two-column labels/values grid
    int y = rc.bottom - detailsH + 10;
    int labelW = 90; int valueW = (rc.right - rc.left) - margin * 2 - labelW - 20;

    MoveWindow(hLblName_, margin, y, labelW, 20, TRUE);   MoveWindow(hValName_, margin + labelW + 10, y, valueW, 20, TRUE); y += 22;
    MoveWindow(hLblPid_, margin, y, labelW, 20, TRUE);    MoveWindow(hValPid_, margin + labelW + 10, y, valueW, 20, TRUE); y += 22;
    MoveWindow(hLblCpu_, margin, y, labelW, 20, TRUE);    MoveWindow(hValCpu_, margin + labelW + 10, y, valueW, 20, TRUE); y += 22;
    MoveWindow(hLblMem_, margin, y, labelW, 20, TRUE);    MoveWindow(hValMem_, margin + labelW + 10, y, valueW, 20, TRUE); y += 22;
    MoveWindow(hLblPath_, margin, y, labelW, 20, TRUE);   MoveWindow(hValPath_, margin + labelW + 10, y, valueW, 20, TRUE); y += 22;
    MoveWindow(hLblCmd_, margin, y, labelW, 20, TRUE);    MoveWindow(hValCmd_, margin + labelW + 10, y, valueW, 20, TRUE); y += 22;
    MoveWindow(hLblPublisher_, margin, y, labelW, 20, TRUE); MoveWindow(hValPublisher_, margin + labelW + 10, y, valueW, 20, TRUE); y += 22;
    MoveWindow(hLblIntegrity_, margin, y, labelW, 20, TRUE); MoveWindow(hValIntegrity_, margin + labelW + 10, y, valueW, 20, TRUE); y += 22;
    MoveWindow(hLblElevated_, margin, y, labelW, 20, TRUE);  MoveWindow(hValElevated_, margin + labelW + 10, y, valueW, 20, TRUE);
}

static bool FileExistsW(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool TaskManagerTab::ResolveRenderProcess(std::wstring& outPath, std::wstring& outArgs) {
    // Similar heuristic as dashboard
    wchar_t modulePath[MAX_PATH];
    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    std::wstring exeDir(modulePath);
    size_t pos = exeDir.find_last_of(L"\\/");
    if (pos != std::wstring::npos) exeDir.erase(pos);

    std::wstring root = exeDir;
    for (int i = 0; i < 3; ++i) {
        size_t p = root.find_last_of(L"\\/");
        if (p == std::wstring::npos) break;
        root.erase(p);
    }

    std::wstring exeCandidate = root + L"\\renderprocess\\bin\\Debug\\net8.0-windows\\win-x64\\RenderProcess.exe";
    if (FileExistsW(exeCandidate)) { outPath = exeCandidate; outArgs.clear(); return true; }

    std::wstring dllCandidate = root + L"\\renderprocess\\bin\\Debug\\net8.0-windows\\win-x64\\RenderProcess.dll";
    if (FileExistsW(dllCandidate)) { outPath = L"dotnet"; outArgs = L"\"" + dllCandidate + L"\""; return true; }

    outPath = L"RenderProcess.exe"; outArgs.clear(); return true;
}

void TaskManagerTab::EnsureIPC() {
    if (!ipc_) {
        ipc_ = std::make_unique<RainmeterManager::Render::RenderIPCBridge>(RainmeterManager::Render::IPCMode::SharedMemory);
        ipc_->SetDefaultTimeout(2000);
        std::wstring path, args; ResolveRenderProcess(path, args);
        ipc_->InitializeIPC();
        ipc_->StartRenderProcess(path, args);
    }
}

void TaskManagerTab::StartPolling() {
    UI::GeneralPrefs gp{};
    bool useIPC = UI::LoadGeneralPrefs(gp) ? gp.enableIPC : false;
    if (useIPC) {
        EnsureIPC();
    }
    if (timerId_ == 0) timerId_ = SetTimer(hwnd_, 1002, 1500, nullptr);
}

void TaskManagerTab::StopPolling() {
    if (timerId_ != 0) { KillTimer(hwnd_, timerId_); timerId_ = 0; }
}

static void ListView_Clear(HWND hList) {
    ListView_DeleteAllItems(hList);
}

void TaskManagerTab::BuildColumns() {
    // remove all
    while (ListView_GetColumnWidth(hList_, 0) != 0 || ListView_GetItemCount(hList_) >= 0) {
        if (!ListView_DeleteColumn(hList_, 0)) break;
    }

    LVCOLUMN col{}; col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    int idx = 0;
    if (colVisible_[0]) { col.pszText = const_cast<LPWSTR>(L"PID"); col.cx = 80; col.iSubItem = idx; ListView_InsertColumn(hList_, idx++, &col); }
    if (colVisible_[1]) { col.pszText = const_cast<LPWSTR>(L"Name"); col.cx = 220; col.iSubItem = idx; ListView_InsertColumn(hList_, idx++, &col); }
    if (colVisible_[2]) { col.pszText = const_cast<LPWSTR>(L"CPU %"); col.cx = 80; col.iSubItem = idx; ListView_InsertColumn(hList_, idx++, &col); }
    if (colVisible_[3]) { col.pszText = const_cast<LPWSTR>(L"Memory MB"); col.cx = 120; col.iSubItem = idx; ListView_InsertColumn(hList_, idx++, &col); }
    if (colVisible_[4]) { col.pszText = const_cast<LPWSTR>(L"Threads"); col.cx = 80; col.iSubItem = idx; ListView_InsertColumn(hList_, idx++, &col); }
    if (colVisible_[5]) { col.pszText = const_cast<LPWSTR>(L"IO R MB/s"); col.cx = 100; col.iSubItem = idx; ListView_InsertColumn(hList_, idx++, &col); }
    if (colVisible_[6]) { col.pszText = const_cast<LPWSTR>(L"IO W MB/s"); col.cx = 100; col.iSubItem = idx; ListView_InsertColumn(hList_, idx++, &col); }
}

void TaskManagerTab::ToggleColumnVisibility(int columnIndex) {
    if (columnIndex < 0 || columnIndex >= (int)colVisible_.size()) return;
    colVisible_[columnIndex] = !colVisible_[columnIndex];
    BuildColumns();
    RebuildDisplay();
    RainmeterManager::UI::TaskManagerPrefs p{colVisible_, sortColumn_, sortAsc_, filterText_};
    SaveTaskManagerPrefs(p);
}

void TaskManagerTab::ShowHeaderContextMenu(POINT ptScreen) {
    HMENU menu = CreatePopupMenu();
    UINT flags = MF_STRING | (colVisible_[0] ? MF_CHECKED : 0); AppendMenuW(menu, flags, 4000, L"PID");
    flags = MF_STRING | (colVisible_[1] ? MF_CHECKED : 0); AppendMenuW(menu, flags, 4001, L"Name");
    flags = MF_STRING | (colVisible_[2] ? MF_CHECKED : 0); AppendMenuW(menu, flags, 4002, L"CPU %");
    flags = MF_STRING | (colVisible_[3] ? MF_CHECKED : 0); AppendMenuW(menu, flags, 4003, L"Memory MB");
    flags = MF_STRING | (colVisible_[4] ? MF_CHECKED : 0); AppendMenuW(menu, flags, 4004, L"Threads");
    flags = MF_STRING | (colVisible_[5] ? MF_CHECKED : 0); AppendMenuW(menu, flags, 4005, L"IO R MB/s");
    flags = MF_STRING | (colVisible_[6] ? MF_CHECKED : 0); AppendMenuW(menu, flags, 4006, L"IO W MB/s");

    int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, ptScreen.x, ptScreen.y, 0, hwnd_, nullptr);
    DestroyMenu(menu);
    if (cmd >= 4000 && cmd <= 4006) {
        int logicalCol = cmd - 4000;
        ToggleColumnVisibility(logicalCol);
    }
}

void TaskManagerTab::UpdateHeaderSortGlyphs() {
    HWND hHeader = ListView_GetHeader(hList_);
    if (!hHeader) return;
    int visibleIndex = 0;
    for (int i = 0; i < (int)colVisible_.size(); ++i) {
        if (!colVisible_[i]) continue;
        HDITEM item{}; item.mask = HDI_FORMAT;
        if (Header_GetItem(hHeader, visibleIndex, &item)) {
            item.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
            // Map logical to visible order: sortColumn_ is logical index (0..4)
            int logicalToVisible = 0; int seen = 0;
            for (int j = 0; j < (int)colVisible_.size(); ++j) {
                if (colVisible_[j]) {
                    if (j == sortColumn_) { logicalToVisible = seen; break; }
                    ++seen;
                }
            }
            if (visibleIndex == logicalToVisible) {
                item.fmt |= sortAsc_ ? HDF_SORTUP : HDF_SORTDOWN;
            }
            Header_SetItem(hHeader, visibleIndex, &item);
        }
        ++visibleIndex;
    }
}

void TaskManagerTab::CreateDetailsPane() {
    auto makeLabel = [&](const wchar_t* text) { return CreateWindowEx(0, L"STATIC", text, WS_CHILD | WS_VISIBLE, 0,0,0,0, hwnd_, nullptr, hInstance_, nullptr); };
    auto makeValue = [&]() { HWND h = CreateWindowEx(0, L"STATIC", L"--", WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP, 0,0,0,0, hwnd_, nullptr, hInstance_, nullptr); SendMessageW(h, WM_SETFONT, (WPARAM)CreateUIFont(10, FW_NORMAL), TRUE); return h; };

    hLblName_ = makeLabel(L"Name:");   hValName_ = makeValue();
    hLblPid_  = makeLabel(L"PID:");    hValPid_  = makeValue();
    hLblCpu_  = makeLabel(L"CPU %:");  hValCpu_  = makeValue();
    hLblMem_  = makeLabel(L"Memory:"); hValMem_  = makeValue();
    hLblPath_ = makeLabel(L"Path:");   hValPath_ = makeValue();
    hLblCmd_  = makeLabel(L"Command:");hValCmd_  = makeValue();
    hLblPublisher_ = makeLabel(L"Publisher:"); hValPublisher_ = makeValue();
    hLblIntegrity_ = makeLabel(L"Integrity:"); hValIntegrity_ = makeValue();
    hLblElevated_  = makeLabel(L"Elevated:");  hValElevated_  = makeValue();
}

void TaskManagerTab::UpdateDetailsFromSelection() {
    int sel = ListView_GetNextItem(hList_, -1, LVNI_SELECTED);
    if (sel < 0 || sel >= (int)displayRows_.size()) {
        SetWindowTextW(hValName_, L"--"); SetWindowTextW(hValPid_, L"--"); SetWindowTextW(hValCpu_, L"--"); SetWindowTextW(hValMem_, L"--"); SetWindowTextW(hValPath_, L"--"); SetWindowTextW(hValCmd_, L"--");
        SetWindowTextW(hValPublisher_, L"--"); SetWindowTextW(hValIntegrity_, L"--"); SetWindowTextW(hValElevated_, L"--");
        return;
    }
    const auto& r = displayRows_[sel];
    wchar_t buf[256];
    SetWindowTextW(hValName_, r.name.c_str());
    swprintf(buf, 256, L"%d", r.pid); SetWindowTextW(hValPid_, buf);
    swprintf(buf, 256, L"%.1f", r.cpu); SetWindowTextW(hValCpu_, buf);
    swprintf(buf, 256, L"%llu MB", r.mem); SetWindowTextW(hValMem_, buf);
    SetWindowTextW(hValPath_, r.path.c_str());
    SetWindowTextW(hValCmd_, r.cmd.c_str());
    SetWindowTextW(hValPublisher_, r.publisher.empty() ? L"Unknown" : r.publisher.c_str());
    SetWindowTextW(hValIntegrity_, r.integrity.empty() ? L"--" : r.integrity.c_str());
    SetWindowTextW(hValElevated_, r.elevated ? L"Yes" : L"No");
}

static size_t FindMatchingObjectEnd(const std::string& s, size_t start) {
    if (start >= s.size() || s[start] != '{') return std::string::npos;
    int depth = 1; bool inStr = false; bool escape = false;
    for (size_t i = start + 1; i < s.size(); ++i) {
        char c = s[i];
        if (inStr) {
            if (escape) { escape = false; continue; }
            if (c == '\\') { escape = true; continue; }
            if (c == '"') { inStr = false; continue; }
        } else {
            if (c == '"') { inStr = true; continue; }
            if (c == '{') { depth++; continue; }
            if (c == '}') { depth--; if (depth == 0) return i; }
        }
    }
    return std::string::npos;
}

void TaskManagerTab::RequestAndUpdateProcessList() {
    UI::GeneralPrefs gp{};
    bool useIPC = UI::LoadGeneralPrefs(gp) ? gp.enableIPC : false;
    if (!useIPC) {
        // Fill placeholder if empty
        if (ListView_GetItemCount(hList_) == 0) {
            PopulatePlaceholder();
        }
        return;
    }
    EnsureIPC();
    if (!ipc_) return;

    // Also update system summary
    FetchSystemSnapshot();

    RainmeterManager::Render::RenderCommand cmd{};
    cmd.commandId = ++localCommandId_;
    cmd.commandType = RainmeterManager::Render::RenderCommandType::GetProcessSnapshot;
    cmd.widgetId = 0;
    cmd.windowHandle = hwnd_;
    cmd.backendType = RainmeterManager::Render::RenderBackendType::Auto;
    cmd.bounds = {0,0,0,0};
    cmd.timestamp = (uint64_t)GetTickCount64();

    auto result = ipc_->SendCommand(cmd, 2000);
    if (result.status != RainmeterManager::Render::RenderResultStatus::Success) return;

    const std::string& json = result.errorMessage; // JSON array of objects

    std::vector<Row> rows; rows.reserve(256);
    size_t pos = 0;
    while (true) {
        size_t objStart = json.find('{', pos);
        if (objStart == std::string::npos) break;
        size_t objEnd = FindMatchingObjectEnd(json, objStart);
        if (objEnd == std::string::npos) break;
        size_t cursor = objStart;
        Row r{0, L"", 0.0f, 0ULL, 0, L"", L"", 0.0f, 0.0f, L"", L"", false};
        ExtractInt(json, cursor, "Pid", r.pid);
        std::wstring nm; if (ExtractString(json, cursor, "Name", nm)) r.name = nm;
        ExtractFloat(json, cursor, "CpuPercent", r.cpu);
        ExtractULong(json, cursor, "WorkingSetMB", r.mem);
        ExtractInt(json, cursor, "Threads", r.threads);
        std::wstring p; if (ExtractString(json, cursor, "ImagePath", p)) r.path = p;
        std::wstring cmd; if (ExtractString(json, cursor, "CommandLine", cmd)) r.cmd = cmd;
        float ior=0, iow=0; ExtractFloat(json, cursor, "IoReadMBps", ior); ExtractFloat(json, cursor, "IoWriteMBps", iow); r.ioReadMBps = ior; r.ioWriteMBps = iow;
        std::wstring pub; if (ExtractString(json, cursor, "Publisher", pub)) r.publisher = pub;
        std::wstring integ; if (ExtractString(json, cursor, "IntegrityLevel", integ)) r.integrity = integ;
        int elev=0; ExtractInt(json, cursor, "IsElevated", elev); r.elevated = (elev!=0);
        rows.push_back(r);
        pos = objEnd + 1;
    }

    // Update CPU history
    for (const auto& r : rows) {
        auto& dq = cpuHist_[r.pid];
        dq.push_back(r.cpu);
        while ((int)dq.size() > kHistLen) dq.pop_front();
    }

    lastRows_.swap(rows);
    RebuildDisplay();
}

void TaskManagerTab::RebuildDisplay() {
    // Filter
    displayRows_.clear();
    displayRows_.reserve(lastRows_.size());

    std::wstring filter = filterText_;
    std::transform(filter.begin(), filter.end(), filter.begin(), ::towlower);

    for (const auto& r : lastRows_) {
        if (filter.empty()) { displayRows_.push_back(r); continue; }
        std::wstring nameLower = r.name;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::towlower);
        if (nameLower.find(filter) != std::wstring::npos) displayRows_.push_back(r);
    }

    // Sort
    auto cmp = [&](const Row& a, const Row& b) {
        switch (sortColumn_) {
            case 0: return sortAsc_ ? (a.pid < b.pid) : (a.pid > b.pid);
            case 1: return sortAsc_ ? (a.name < b.name) : (a.name > b.name);
            case 2: return sortAsc_ ? (a.cpu < b.cpu) : (a.cpu > b.cpu);
            case 3: return sortAsc_ ? (a.mem < b.mem) : (a.mem > b.mem);
            case 4: return sortAsc_ ? (a.threads < b.threads) : (a.threads > b.threads);
            default: return false;
        }
    };
    std::stable_sort(displayRows_.begin(), displayRows_.end(), cmp);

    UpdateHeaderSortGlyphs();

    // Repopulate list with current visible columns
    ListView_DeleteAllItems(hList_);
    int index = 0;
    for (const auto& r : displayRows_) {
        wchar_t buf[256];
        LVITEM item{}; item.mask = LVIF_TEXT; item.iItem = index; item.iSubItem = 0;
        if (colVisible_[0]) { swprintf(buf, 256, L"%d", r.pid); item.pszText = buf; } else { item.pszText = const_cast<LPWSTR>(L""); }
        int rowIndex = ListView_InsertItem(hList_, &item);
        int col = 1;
        if (colVisible_[1]) { ListView_SetItemText(hList_, rowIndex, col++, const_cast<LPWSTR>(r.name.c_str())); }
        if (colVisible_[2]) { swprintf(buf, 256, L"%.1f", r.cpu); ListView_SetItemText(hList_, rowIndex, col++, buf); }
        if (colVisible_[3]) { swprintf(buf, 256, L"%llu", r.mem); ListView_SetItemText(hList_, rowIndex, col++, buf); }
        if (colVisible_[4]) { swprintf(buf, 256, L"%d", r.threads); ListView_SetItemText(hList_, rowIndex, col++, buf); }
        if (colVisible_[5]) { swprintf(buf, 256, L"%.2f", r.ioReadMBps); ListView_SetItemText(hList_, rowIndex, col++, buf); }
        if (colVisible_[6]) { swprintf(buf, 256, L"%.2f", r.ioWriteMBps); ListView_SetItemText(hList_, rowIndex, col++, buf); }
        ++index;
    }

    // Autosize columns
    int totalCols = 0; for (bool v : colVisible_) if (v) ++totalCols;
    for (int c = 0; c < totalCols; ++c) {
        ListView_SetColumnWidth(hList_, c, LVSCW_AUTOSIZE_USEHEADER);
    }

    UpdateDetailsFromSelection();
}

// Minimal JSON helpers for simple key/value extraction within an object slice
static bool FindKey(const std::string& json, size_t& pos, const char* key) {
    std::string needle = std::string("\"") + key + "\"";
    size_t k = json.find(needle, pos);
    if (k == std::string::npos) return false;
    size_t colon = json.find(':', k + needle.size());
    if (colon == std::string::npos) return false;
    pos = colon + 1;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    return true;
}

bool TaskManagerTab::ExtractInt(const std::string& json, size_t& pos, const char* key, int& outVal) {
    size_t p = pos; if (!FindKey(json, p, key)) return false;
    size_t end = p;
    while (end < json.size() && (json[end] == '-' || isdigit((unsigned char)json[end]))) ++end;
    try { outVal = std::stoi(json.substr(p, end - p)); pos = end; return true; } catch (...) { return false; }
}

bool TaskManagerTab::ExtractULong(const std::string& json, size_t& pos, const char* key, unsigned long long& outVal) {
    size_t p = pos; if (!FindKey(json, p, key)) return false;
    size_t end = p;
    while (end < json.size() && isdigit((unsigned char)json[end])) ++end;
    try { outVal = std::stoull(json.substr(p, end - p)); pos = end; return true; } catch (...) { return false; }
}

bool TaskManagerTab::ExtractFloat(const std::string& json, size_t& pos, const char* key, float& outVal) {
    size_t p = pos; if (!FindKey(json, p, key)) return false;
    size_t end = p;
    while (end < json.size() && (json[end] == '-' || json[end] == '+' || json[end] == '.' || isdigit((unsigned char)json[end]) || json[end] == 'e' || json[end] == 'E')) ++end;
    try { outVal = std::stof(json.substr(p, end - p)); pos = end; return true; } catch (...) { return false; }
}

bool TaskManagerTab::ExtractString(const std::string& json, size_t& pos, const char* key, std::wstring& outStr) {
    size_t p = pos; if (!FindKey(json, p, key)) return false;
    if (p >= json.size() || json[p] != '"') return false;
    ++p; size_t start = p;
    while (p < json.size() && json[p] != '"') {
        if (json[p] == '\\' && p + 1 < json.size()) { p += 2; continue; }
        ++p;
    }
    if (p >= json.size()) return false;
    std::string u8 = json.substr(start, p - start);
    // Simple conversion (assumes ASCII subset); for full UTF-8 support, integrate proper decoder later
    std::wstring w; w.reserve(u8.size());
    for (unsigned char c : u8) w.push_back((wchar_t)c);
    outStr.swap(w); pos = p + 1; return true;
}

