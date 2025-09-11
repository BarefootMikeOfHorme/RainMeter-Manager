#pragma once

#include <windows.h>
#include <string>
#include <vector>

namespace RainmeterManager {
namespace UI {

struct GeneralPrefs {
    bool enableIPC = false;
};

struct TaskManagerPrefs {
    std::vector<bool> columns; // logical order
    int sortColumn = 2;
    bool sortAsc = false;
    std::wstring filterText;
};

// Load preferences from %AppData%\\RainmeterManager\\ui_prefs.json
bool LoadTaskManagerPrefs(TaskManagerPrefs& out);

// Save preferences
bool SaveTaskManagerPrefs(const TaskManagerPrefs& prefs);

// General prefs (same file)
bool LoadGeneralPrefs(GeneralPrefs& out);
bool SaveGeneralPrefs(const GeneralPrefs& prefs);

} // namespace UI
} // namespace RainmeterManager

