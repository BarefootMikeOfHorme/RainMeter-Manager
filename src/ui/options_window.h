#ifndef OPTIONS_WINDOW_H
#define OPTIONS_WINDOW_H

#include <windows.h>
#include <string>
#include <memory>
#include "dashboard_tab.h"
#include "task_manager_tab.h"

namespace RainmeterManager {
namespace UI {

class OptionsWindow {
public:
    OptionsWindow(HINSTANCE hInstance);
    ~OptionsWindow();

    // Non-copyable
    OptionsWindow(const OptionsWindow&) = delete;
    OptionsWindow& operator=(const OptionsWindow&) = delete;

    bool Create();
    void Show(int nCmdShow = SW_SHOWNORMAL);
    void Hide();
    HWND Handle() const { return hwnd_; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void CreateTabs();
    void ResizeChildren();

    HINSTANCE hInstance_;
    HWND hwnd_ = nullptr;
    HWND hTab_ = nullptr;

    std::unique_ptr<DashboardTab> dashboardTab_;
    std::unique_ptr<TaskManagerTab> taskTab_;
};

} // namespace UI
} // namespace RainmeterManager

#endif // OPTIONS_WINDOW_H

