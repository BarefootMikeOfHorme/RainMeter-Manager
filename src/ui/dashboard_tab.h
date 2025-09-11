#ifndef DASHBOARD_TAB_H
#define DASHBOARD_TAB_H

#include <windows.h>
#include <string>
#include <memory>
#include "ui_theme.h"

namespace RainmeterManager {
// Forward declaration to avoid requiring full IPC header here
namespace Render { class RenderIPCBridge; }
namespace UI {

class DashboardTab {
public:
    DashboardTab(HINSTANCE hInstance, HWND hParentTab);
    ~DashboardTab();

    bool Create();
    void Show();
    void Hide();
    void Resize(const RECT& rc);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void CreateChildren();
    void LayoutChildren();

    // IPC wiring
    void StartPolling();
    void StopPolling();
    void EnsureIPC();
    void RequestAndUpdateSnapshot();
    bool ResolveRenderProcess(std::wstring& outPath, std::wstring& outArgs);

    static bool ExtractDouble(const std::string& json, const char* keyPascal, const char* keyCamel, double& outVal);
    static std::wstring FormatDouble(double v, int decimals = 1);

    HINSTANCE hInstance_;
    HWND hParentTab_;
    HWND hwnd_ = nullptr;

    HWND hTileCpu_ = nullptr;
    HWND hTileMem_ = nullptr;
    HWND hTileNet_ = nullptr;

    // State
    UINT_PTR timerId_ = 0;
    std::unique_ptr<RainmeterManager::Render::RenderIPCBridge> ipc_;
    uint64_t localCommandId_ = 1;
};

} // namespace UI
} // namespace RainmeterManager

#endif // DASHBOARD_TAB_H

