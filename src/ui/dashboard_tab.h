#ifndef DASHBOARD_TAB_H
#define DASHBOARD_TAB_H

#include <windows.h>
#include <string>
#include "ui_theme.h"

namespace RainmeterManager {
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

    HINSTANCE hInstance_;
    HWND hParentTab_;
    HWND hwnd_ = nullptr;

    HWND hTileCpu_ = nullptr;
    HWND hTileMem_ = nullptr;
    HWND hTileNet_ = nullptr;
};

} // namespace UI
} // namespace RainmeterManager

#endif // DASHBOARD_TAB_H

