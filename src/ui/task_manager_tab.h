#ifndef TASK_MANAGER_TAB_H
#define TASK_MANAGER_TAB_H

#include <windows.h>
#include <string>
#include <memory>
#include <vector>
#include <deque>
#include <unordered_map>

namespace RainmeterManager {
// Forward declaration to avoid requiring full IPC header here
namespace Render { class RenderIPCBridge; }
namespace UI {

class TaskManagerTab {
public:
    TaskManagerTab(HINSTANCE hInstance, HWND hParentTab);
    ~TaskManagerTab();

    bool Create();
    void Show();
    void Hide();
    void Resize(const RECT& rc);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void CreateChildren();
    void LayoutChildren();
    void InitListView();

    // IPC wiring
    void StartPolling();
    void StopPolling();
    void EnsureIPC();
    bool ResolveRenderProcess(std::wstring& outPath, std::wstring& outArgs);
    void RequestAndUpdateProcessList();
    void RebuildDisplay();

    // UI enhancements
    void UpdateHeaderSortGlyphs();
    void ShowHeaderContextMenu(POINT ptScreen);
    void ToggleColumnVisibility(int columnIndex);
    void BuildColumns();

    // Details pane
     void CreateDetailsPane();
     void UpdateDetailsFromSelection();
 
     // Helper to seed the list with a placeholder row
     void PopulatePlaceholder();
 
     // JSON helpers
     static bool ExtractInt(const std::string& json, size_t& pos, const char* key, int& outVal);
     static bool ExtractULong(const std::string& json, size_t& pos, const char* key, unsigned long long& outVal);
     static bool ExtractFloat(const std::string& json, size_t& pos, const char* key, float& outVal);
     static bool ExtractString(const std::string& json, size_t& pos, const char* key, std::wstring& outStr);

    HINSTANCE hInstance_;
    HWND hParentTab_;
    HWND hwnd_ = nullptr;
    HWND hList_ = nullptr;
    HWND hSearch_ = nullptr;
    HWND hSummary_ = nullptr;

    // Details controls
    HWND hLblName_ = nullptr, hValName_ = nullptr;
    HWND hLblPid_ = nullptr, hValPid_ = nullptr;
    HWND hLblCpu_ = nullptr, hValCpu_ = nullptr;
    HWND hLblMem_ = nullptr, hValMem_ = nullptr;
    HWND hLblPath_ = nullptr, hValPath_ = nullptr;
    HWND hLblCmd_ = nullptr, hValCmd_ = nullptr;
    HWND hLblPublisher_ = nullptr, hValPublisher_ = nullptr;
    HWND hLblIntegrity_ = nullptr, hValIntegrity_ = nullptr;
    HWND hLblElevated_ = nullptr, hValElevated_ = nullptr;

    // State
    UINT_PTR timerId_ = 0;
    std::unique_ptr<RainmeterManager::Render::RenderIPCBridge> ipc_;
    uint64_t localCommandId_ = 10000; // separate range from dashboard

    struct Row { int pid; std::wstring name; float cpu; unsigned long long mem; int threads; std::wstring path; std::wstring cmd; float ioReadMBps; float ioWriteMBps; std::wstring publisher; std::wstring integrity; bool elevated; };
    std::vector<Row> lastRows_;
    std::vector<Row> displayRows_;
    int sortColumn_ = 2; // default CPU column
    bool sortAsc_ = false; // default descending CPU
    std::wstring filterText_;

    // Column visibility: PID, Name, CPU, Memory, Threads, IO Read, IO Write
    std::vector<bool> colVisible_ = { true, true, true, true, true, false, false };

    // History for sparklines
    std::unordered_map<int, std::deque<float>> cpuHist_;
    static constexpr int kHistLen = 60;

    // System summary histories
    std::deque<double> sysCpuHist_;
    std::deque<double> sysMemHist_;
    std::deque<double> sysNetHist_;
    double sysMemTotalMB_ = 0.0;

    // Helpers
    void FetchSystemSnapshot();
    static LRESULT CALLBACK SummaryProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void DrawSummary(HDC hdc, const RECT& rc);

    // IDs
    static constexpr int kIdSearchEdit = 2001;
    static constexpr int kIdCtxOpenLocation = 3001;
    static constexpr int kIdCtxCopyCmd = 3002;
    static constexpr int kIdCtxProperties = 3003;
};

} // namespace UI
} // namespace RainmeterManager

#endif // TASK_MANAGER_TAB_H

