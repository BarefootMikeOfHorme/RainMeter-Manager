#include "ui_prefs.h"
#include <shlobj.h>
#include <cstdio>
#include <fstream>

namespace RainmeterManager {
namespace UI {

static std::wstring GetPrefsDir() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, path))) {
        std::wstring dir = std::wstring(path) + L"\\RainmeterManager";
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir;
    }
    return L".";
}

static std::wstring GetPrefsPath() {
    return GetPrefsDir() + L"\\ui_prefs.json";
}

static std::wstring Escape(const std::wstring& s) {
    std::wstring o; o.reserve(s.size());
    for (wchar_t c : s) {
        if (c == L'\\' || c == L'"') o.push_back(L'\\');
        o.push_back(c);
    }
    return o;
}

bool SaveTaskManagerPrefs(const TaskManagerPrefs& prefs) {
    try {
        std::wstring path = GetPrefsPath();
        std::wofstream f(path);
        f.imbue(std::locale(""));
        if (!f.is_open()) return false;
        f << L"{\n";
        f << L"  \"sortColumn\": " << prefs.sortColumn << L",\n";
        f << L"  \"sortAsc\": " << (prefs.sortAsc ? L"true" : L"false") << L",\n";
        f << L"  \"filterText\": \"" << Escape(prefs.filterText) << L"\",\n";
        f << L"  \"columns\": [";
        for (size_t i = 0; i < prefs.columns.size(); ++i) {
            f << (prefs.columns[i] ? 1 : 0);
            if (i + 1 < prefs.columns.size()) f << L",";
        }
        f << L"]\n";
        f << L"}\n";
        return true;
    } catch (...) { return false; }
}

static bool ReadAllText(const std::wstring& path, std::wstring& out) {
    try {
        std::wifstream f(path);
        f.imbue(std::locale(""));
        if (!f.is_open()) return false;
        std::wstring s((std::istreambuf_iterator<wchar_t>(f)), std::istreambuf_iterator<wchar_t>());
        out = std::move(s);
        return true;
    } catch (...) { return false; }
}

static bool FindInt(const std::wstring& s, const wchar_t* key, int& outVal) {
    size_t pos = s.find(key);
    if (pos == std::wstring::npos) return false;
    pos = s.find(L":", pos);
    if (pos == std::wstring::npos) return false;
    ++pos;
    while (pos < s.size() && (s[pos] == L' ' || s[pos] == L'\t')) ++pos;
    size_t end = pos;
    while (end < s.size() && (s[end] == L'-' || iswdigit(s[end]))) ++end;
    try { outVal = std::stoi(s.substr(pos, end - pos)); return true; } catch (...) { return false; }
}

static bool FindBool(const std::wstring& s, const wchar_t* key, bool& outVal) {
    size_t pos = s.find(key);
    if (pos == std::wstring::npos) return false;
    pos = s.find(L":", pos);
    if (pos == std::wstring::npos) return false;
    ++pos;
    while (pos < s.size() && (s[pos] == L' ' || s[pos] == L'\t')) ++pos;
    if (s.compare(pos, 4, L"true") == 0) { outVal = true; return true; }
    if (s.compare(pos, 5, L"false") == 0) { outVal = false; return true; }
    return false;
}

static bool FindString(const std::wstring& s, const wchar_t* key, std::wstring& outVal) {
    size_t pos = s.find(key);
    if (pos == std::wstring::npos) return false;
    pos = s.find(L"\"", pos + wcslen(key));
    if (pos == std::wstring::npos) return false;
    ++pos;
    size_t start = pos;
    while (pos < s.size() && s[pos] != L'\"') {
        if (s[pos] == L'\\' && pos + 1 < s.size()) { pos += 2; continue; }
        ++pos;
    }
    if (pos >= s.size()) return false;
    outVal = s.substr(start, pos - start);
    return true;
}

static bool FindArray(const std::wstring& s, const wchar_t* key, std::vector<int>& out) {
    size_t pos = s.find(key);
    if (pos == std::wstring::npos) return false;
    pos = s.find(L"[", pos);
    if (pos == std::wstring::npos) return false;
    ++pos;
    out.clear();
    while (pos < s.size()) {
        while (pos < s.size() && (s[pos] == L' ' || s[pos] == L'\t')) ++pos;
        if (pos < s.size() && s[pos] == L']') break;
        size_t end = pos;
        while (end < s.size() && iswdigit(s[end])) ++end;
        if (end > pos) {
            try { out.push_back(std::stoi(s.substr(pos, end - pos))); } catch (...) {}
            pos = end;
        }
        size_t comma = s.find(L",", pos);
        if (comma == std::wstring::npos) break;
        pos = comma + 1;
    }
    return true;
}

bool LoadTaskManagerPrefs(TaskManagerPrefs& out) {
    try {
        std::wstring path = GetPrefsPath();
        std::wstring content;
        if (!ReadAllText(path, content)) return false;
        int sortCol = out.sortColumn;
        bool sortAsc = out.sortAsc;
        std::wstring filter = out.filterText;
        std::vector<int> cols;
        FindInt(content, L"\"sortColumn\"", sortCol);
        FindBool(content, L"\"sortAsc\"", sortAsc);
        FindString(content, L"\"filterText\": \"", filter);
        if (FindArray(content, L"\"columns\"", cols)) {
            out.columns.clear();
            for (int v : cols) out.columns.push_back(v != 0);
        }
        out.sortColumn = sortCol;
        out.sortAsc = sortAsc;
        out.filterText = filter;
        return true;
    } catch (...) { return false; }
}

static bool FindBoolField(const std::wstring& s, const wchar_t* key, bool& outVal) {
    return FindBool(s, key, outVal);
}

bool LoadGeneralPrefs(GeneralPrefs& out) {
    try {
        std::wstring path = GetPrefsPath();
        std::wstring content;
        if (!ReadAllText(path, content)) return false;
        bool v = out.enableIPC;
        if (FindBoolField(content, L"\"enableIPC\"", v)) {
            out.enableIPC = v;
        }
        return true;
    } catch (...) { return false; }
}

bool SaveGeneralPrefs(const GeneralPrefs& prefs) {
    try {
        // Merge with existing file if present
        std::wstring path = GetPrefsPath();
        std::wstring content;
        bool has = ReadAllText(path, content);
        // Very simple write: rewrite full file combining known fields
        TaskManagerPrefs tmp;
        if (has) {
            LoadTaskManagerPrefs(tmp);
        } else {
            tmp.columns = { true, true, true, true, true, false, false };
        }
        // Write minimal combined JSON
        std::wofstream f(path);
        f.imbue(std::locale(""));
        if (!f.is_open()) return false;
        f << L"{\n";
        f << L"  \"enableIPC\": " << (prefs.enableIPC ? L"true" : L"false") << L",\n";
        f << L"  \"sortColumn\": " << tmp.sortColumn << L",\n";
        f << L"  \"sortAsc\": " << (tmp.sortAsc ? L"true" : L"false") << L",\n";
        f << L"  \"filterText\": \"" << Escape(tmp.filterText) << L"\",\n";
        f << L"  \"columns\": [";
        for (size_t i = 0; i < tmp.columns.size(); ++i) {
            f << (tmp.columns[i] ? 1 : 0);
            if (i + 1 < tmp.columns.size()) f << L",";
        }
        f << L"]\n";
        f << L"}\n";
        return true;
    } catch (...) { return false; }
}

} // namespace UI
} // namespace RainmeterManager

