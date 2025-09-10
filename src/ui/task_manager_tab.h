#ifndef TASK_MANAGER_TAB_H
#define TASK_MANAGER_TAB_H

#include <windows.h>
#include <string>

namespace RainmeterManager {
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
    void PopulatePlaceholder();

    HINSTANCE hInstance_;
    HWND hParentTab_;
    HWND hwnd_ = nullptr;
    HWND hList_ = nullptr;
};

} // namespace UI
} // namespace RainmeterManager

#endif // TASK_MANAGER_TAB_H

