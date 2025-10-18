// configuration_manager.cpp - Implementation of ConfigurationManager
// Copyright (c) 2025 RainmeterManager. All rights reserved.

#include "configuration_manager.h"
#include "../core/logger.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <shlobj.h>
#include <windows.h>

namespace RainmeterManager {
namespace Config {

// AppConfiguration default constructor
AppConfiguration::AppConfiguration()
    : version(L"1.0.0")
    , locale(L"en-US")
    , firstRun(true)
    , windowX(100)
    , windowY(100)
    , windowWidth(1280)
    , windowHeight(800)
    , windowMaximized(false)
    , logLevel(L"INFO")
    , enableFileLogging(true)
    , enableConsoleLogging(false)
    , logRotationSize(10)  // 10MB
    , logRotationCount(5)
    , updateIntervalMs(16)  // ~60fps
    , checkForUpdates(true)
    , widgetConfigPath(L"config/widgets.json")
    , enableWidgets(true)
    , validateCodeSignatures(false)  // Off by default for development
    , enableSecureMode(true)
    , enableHardwareAcceleration(true)
    , renderFPS(60)
{
}

// ConfigurationManager implementation
ConfigurationManager::ConfigurationManager()
    : configFormat_(L"json")
    , initialized_(false)
    , dirty_(false)
{
    LOG_INFO("ConfigurationManager created");
}

ConfigurationManager::~ConfigurationManager() {
    if (initialized_) {
        // Auto-save if dirty
        if (dirty_) {
            LOG_INFO("Auto-saving configuration on destruction");
            Save();
        }
        Shutdown();
    }
    LOG_INFO("ConfigurationManager destroyed");
}

bool ConfigurationManager::Initialize(const std::wstring& configPath) {
    std::lock_guard<std::mutex> lock(configMutex_);

    if (initialized_) {
        LOG_WARNING("ConfigurationManager already initialized");
        return true;
    }

    LOG_INFO("Initializing ConfigurationManager");

    // Set config file path
    if (configPath.empty()) {
        configFilePath_ = GetDefaultConfigPath();
    } else {
        configFilePath_ = configPath;
    }

    // Determine format from extension
    if (configFilePath_.ends_with(L".json")) {
        configFormat_ = L"json";
    } else if (configFilePath_.ends_with(L".ini") || configFilePath_.ends_with(L".cfg")) {
        configFormat_ = L"ini";
    }

    LOG_INFO("Config file: " + std::string(configFilePath_.begin(), configFilePath_.end()));
    LOG_INFO("Config format: " + std::string(configFormat_.begin(), configFormat_.end()));

    // Try to load existing config
    if (std::filesystem::exists(configFilePath_)) {
        if (!Load()) {
            LOG_WARNING("Failed to load config, using defaults");
            config_ = AppConfiguration();
        }
    } else {
        LOG_INFO("No existing config found, using defaults");
        config_ = AppConfiguration();
        
        // Create config directory if it doesn't exist
        std::filesystem::path configDir = std::filesystem::path(configFilePath_).parent_path();
        if (!configDir.empty() && !std::filesystem::exists(configDir)) {
            std::filesystem::create_directories(configDir);
        }
        
        // Save default config
        Save();
    }

    initialized_ = true;
    LOG_INFO("ConfigurationManager initialized successfully");
    return true;
}

void ConfigurationManager::Shutdown() {
    std::lock_guard<std::mutex> lock(configMutex_);

    if (!initialized_) {
        return;
    }

    LOG_INFO("Shutting down ConfigurationManager");

    // Save if dirty
    if (dirty_) {
        Save();
    }

    initialized_ = false;
    LOG_INFO("ConfigurationManager shut down");
}

bool ConfigurationManager::Load() {
    return Load(configFilePath_);
}

bool ConfigurationManager::Load(const std::wstring& filePath) {
    std::lock_guard<std::mutex> lock(configMutex_);

    LOG_INFO("Loading configuration from: " + std::string(filePath.begin(), filePath.end()));

    if (!std::filesystem::exists(filePath)) {
        LOG_ERROR("Configuration file not found: " + std::string(filePath.begin(), filePath.end()));
        return false;
    }

    bool success = false;
    if (configFormat_ == L"json") {
        success = LoadJSON(filePath);
    } else if (configFormat_ == L"ini") {
        success = LoadINI(filePath);
    } else {
        LOG_ERROR("Unknown config format: " + std::string(configFormat_.begin(), configFormat_.end()));
        return false;
    }

    if (success) {
        MarkClean();
        LOG_INFO("Configuration loaded successfully");
    } else {
        LOG_ERROR("Failed to load configuration");
    }

    return success;
}

bool ConfigurationManager::Save() {
    return Save(configFilePath_);
}

bool ConfigurationManager::Save(const std::wstring& filePath) {
    std::lock_guard<std::mutex> lock(configMutex_);

    LOG_INFO("Saving configuration to: " + std::string(filePath.begin(), filePath.end()));

    // Extract current configuration to customSettings_
    ExtractConfiguration();

    bool success = false;
    if (configFormat_ == L"json") {
        success = SaveJSON(filePath);
    } else if (configFormat_ == L"ini") {
        success = SaveINI(filePath);
    } else {
        LOG_ERROR("Unknown config format: " + std::string(configFormat_.begin(), configFormat_.end()));
        return false;
    }

    if (success) {
        MarkClean();
        LOG_INFO("Configuration saved successfully");
    } else {
        LOG_ERROR("Failed to save configuration");
    }

    return success;
}

bool ConfigurationManager::IsDirty() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return dirty_;
}

AppConfiguration ConfigurationManager::GetConfiguration() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return config_;
}

void ConfigurationManager::SetConfiguration(const AppConfiguration& config) {
    std::lock_guard<std::mutex> lock(configMutex_);
    config_ = config;
    MarkDirty();
    LOG_INFO("Configuration updated");
}

std::wstring ConfigurationManager::GetValue(const std::wstring& key, const std::wstring& defaultValue) const {
    std::lock_guard<std::mutex> lock(configMutex_);

    auto it = customSettings_.find(key);
    if (it != customSettings_.end()) {
        return it->second;
    }

    return defaultValue;
}

void ConfigurationManager::SetValue(const std::wstring& key, const std::wstring& value) {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    customSettings_[key] = value;
    MarkDirty();
    NotifyChange(key, value);
}

bool ConfigurationManager::HasValue(const std::wstring& key) const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return customSettings_.find(key) != customSettings_.end();
}

void ConfigurationManager::RemoveValue(const std::wstring& key) {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    customSettings_.erase(key);
    MarkDirty();
}

int ConfigurationManager::GetInt(const std::wstring& key, int defaultValue) const {
    std::wstring value = GetValue(key);
    if (value.empty()) {
        return defaultValue;
    }

    try {
        return std::stoi(value);
    } catch (...) {
        return defaultValue;
    }
}

void ConfigurationManager::SetInt(const std::wstring& key, int value) {
    SetValue(key, std::to_wstring(value));
}

bool ConfigurationManager::GetBool(const std::wstring& key, bool defaultValue) const {
    std::wstring value = GetValue(key);
    if (value.empty()) {
        return defaultValue;
    }

    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    return (value == L"true" || value == L"1" || value == L"yes");
}

void ConfigurationManager::SetBool(const std::wstring& key, bool value) {
    SetValue(key, value ? L"true" : L"false");
}

float ConfigurationManager::GetFloat(const std::wstring& key, float defaultValue) const {
    std::wstring value = GetValue(key);
    if (value.empty()) {
        return defaultValue;
    }

    try {
        return std::stof(value);
    } catch (...) {
        return defaultValue;
    }
}

void ConfigurationManager::SetFloat(const std::wstring& key, float value) {
    SetValue(key, std::to_wstring(value));
}

std::vector<std::wstring> ConfigurationManager::GetArray(const std::wstring& key) const {
    std::wstring value = GetValue(key);
    std::vector<std::wstring> result;

    if (value.empty()) {
        return result;
    }

    // Parse comma-separated values
    std::wstringstream ss(value);
    std::wstring item;
    while (std::getline(ss, item, L',')) {
        // Trim whitespace
        item.erase(0, item.find_first_not_of(L" \t"));
        item.erase(item.find_last_not_of(L" \t") + 1);
        result.push_back(item);
    }

    return result;
}

void ConfigurationManager::SetArray(const std::wstring& key, const std::vector<std::wstring>& values) {
    std::wstring combined;
    for (size_t i = 0; i < values.size(); ++i) {
        combined += values[i];
        if (i < values.size() - 1) {
            combined += L",";
        }
    }
    SetValue(key, combined);
}

std::wstring ConfigurationManager::GetConfigFilePath() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return configFilePath_;
}

void ConfigurationManager::SetConfigFilePath(const std::wstring& path) {
    std::lock_guard<std::mutex> lock(configMutex_);
    configFilePath_ = path;
}

void ConfigurationManager::SetConfigFormat(const std::wstring& format) {
    std::lock_guard<std::mutex> lock(configMutex_);
    configFormat_ = format;
}

void ConfigurationManager::RegisterChangeCallback(ConfigChangeCallback callback) {
    std::lock_guard<std::mutex> lock(configMutex_);
    changeCallbacks_.push_back(callback);
}

void ConfigurationManager::ClearChangeCallbacks() {
    std::lock_guard<std::mutex> lock(configMutex_);
    changeCallbacks_.clear();
}

bool ConfigurationManager::ExportToFile(const std::wstring& filePath, const std::wstring& format) {
    // Temporarily change format, save, then restore
    std::wstring originalFormat = configFormat_;
    configFormat_ = format;
    bool result = Save(filePath);
    configFormat_ = originalFormat;
    return result;
}

bool ConfigurationManager::ImportFromFile(const std::wstring& filePath) {
    return Load(filePath);
}

void ConfigurationManager::ResetToDefaults() {
    std::lock_guard<std::mutex> lock(configMutex_);

    LOG_INFO("Resetting configuration to defaults");
    config_ = AppConfiguration();
    customSettings_.clear();
    MarkDirty();
}

bool ConfigurationManager::MigrateFromVersion(const std::wstring& fromVersion) {
    LOG_INFO("Migration from version: " + std::string(fromVersion.begin(), fromVersion.end()));
    
    // TODO: Implement version-specific migration logic
    // For now, just log and return success
    LOG_WARNING("MigrateFromVersion not yet implemented");
    return true;
}

// Private helper methods

bool ConfigurationManager::LoadJSON(const std::wstring& filePath) {
    // Simple JSON parser (basic implementation)
    // For production, consider using a JSON library like nlohmann/json
    
    std::wifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    std::wstring line;
    std::wstring currentKey;
    
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(L" \t\r\n"));
        line.erase(line.find_last_not_of(L" \t\r\n") + 1);

        // Skip empty lines and comments
        if (line.empty() || line[0] == L'#' || line[0] == L'/') {
            continue;
        }

        // Simple key:value parsing
        size_t colonPos = line.find(L':');
        if (colonPos != std::wstring::npos) {
            std::wstring key = line.substr(0, colonPos);
            std::wstring value = line.substr(colonPos + 1);

            // Trim quotes and whitespace
            key.erase(0, key.find_first_not_of(L" \t\""));
            key.erase(key.find_last_not_of(L" \t\",") + 1);
            value.erase(0, value.find_first_not_of(L" \t\""));
            value.erase(value.find_last_not_of(L" \t\",") + 1);

            customSettings_[key] = value;
        }
    }

    file.close();

    // Apply to config structure
    ApplyConfiguration(config_);
    return true;
}

bool ConfigurationManager::SaveJSON(const std::wstring& filePath) {
    std::wofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    file << L"{\n";
    file << L"  \"version\": \"" << config_.version << L"\",\n";
    file << L"  \"locale\": \"" << config_.locale << L"\",\n";
    file << L"  \"firstRun\": " << (config_.firstRun ? L"true" : L"false") << L",\n";
    file << L"  \"windowX\": " << config_.windowX << L",\n";
    file << L"  \"windowY\": " << config_.windowY << L",\n";
    file << L"  \"windowWidth\": " << config_.windowWidth << L",\n";
    file << L"  \"windowHeight\": " << config_.windowHeight << L",\n";
    file << L"  \"windowMaximized\": " << (config_.windowMaximized ? L"true" : L"false") << L",\n";
    file << L"  \"logLevel\": \"" << config_.logLevel << L"\",\n";
    file << L"  \"enableFileLogging\": " << (config_.enableFileLogging ? L"true" : L"false") << L",\n";
    file << L"  \"enableConsoleLogging\": " << (config_.enableConsoleLogging ? L"true" : L"false") << L",\n";
    file << L"  \"logRotationSize\": " << config_.logRotationSize << L",\n";
    file << L"  \"logRotationCount\": " << config_.logRotationCount << L",\n";
    file << L"  \"updateIntervalMs\": " << config_.updateIntervalMs << L",\n";
    file << L"  \"checkForUpdates\": " << (config_.checkForUpdates ? L"true" : L"false") << L",\n";
    file << L"  \"widgetConfigPath\": \"" << config_.widgetConfigPath << L"\",\n";
    file << L"  \"enableWidgets\": " << (config_.enableWidgets ? L"true" : L"false") << L",\n";
    file << L"  \"validateCodeSignatures\": " << (config_.validateCodeSignatures ? L"true" : L"false") << L",\n";
    file << L"  \"enableSecureMode\": " << (config_.enableSecureMode ? L"true" : L"false") << L",\n";
    file << L"  \"enableHardwareAcceleration\": " << (config_.enableHardwareAcceleration ? L"true" : L"false") << L",\n";
    file << L"  \"renderFPS\": " << config_.renderFPS << L"\n";
    file << L"}\n";

    file.close();
    return true;
}

bool ConfigurationManager::LoadINI(const std::wstring& filePath) {
    std::wifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    std::wstring line;
    std::wstring currentSection;

    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(L" \t\r\n"));
        line.erase(line.find_last_not_of(L" \t\r\n") + 1);

        // Skip empty lines and comments
        if (line.empty() || line[0] == L';' || line[0] == L'#') {
            continue;
        }

        // Section header
        if (line[0] == L'[' && line.back() == L']') {
            currentSection = line.substr(1, line.length() - 2);
            continue;
        }

        // Key=Value pair
        size_t equalPos = line.find(L'=');
        if (equalPos != std::wstring::npos) {
            std::wstring key = line.substr(0, equalPos);
            std::wstring value = line.substr(equalPos + 1);

            // Trim
            key.erase(0, key.find_first_not_of(L" \t"));
            key.erase(key.find_last_not_of(L" \t") + 1);
            value.erase(0, value.find_first_not_of(L" \t"));
            value.erase(value.find_last_not_of(L" \t") + 1);

            // Store with section prefix if present
            if (!currentSection.empty()) {
                key = currentSection + L"." + key;
            }

            customSettings_[key] = value;
        }
    }

    file.close();

    // Apply to config structure
    ApplyConfiguration(config_);
    return true;
}

bool ConfigurationManager::SaveINI(const std::wstring& filePath) {
    std::wofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    file << L"[Application]\n";
    file << L"version=" << config_.version << L"\n";
    file << L"locale=" << config_.locale << L"\n";
    file << L"firstRun=" << (config_.firstRun ? L"true" : L"false") << L"\n\n";

    file << L"[Window]\n";
    file << L"x=" << config_.windowX << L"\n";
    file << L"y=" << config_.windowY << L"\n";
    file << L"width=" << config_.windowWidth << L"\n";
    file << L"height=" << config_.windowHeight << L"\n";
    file << L"maximized=" << (config_.windowMaximized ? L"true" : L"false") << L"\n\n";

    file << L"[Logging]\n";
    file << L"logLevel=" << config_.logLevel << L"\n";
    file << L"enableFileLogging=" << (config_.enableFileLogging ? L"true" : L"false") << L"\n";
    file << L"enableConsoleLogging=" << (config_.enableConsoleLogging ? L"true" : L"false") << L"\n";
    file << L"logRotationSize=" << config_.logRotationSize << L"\n";
    file << L"logRotationCount=" << config_.logRotationCount << L"\n\n";

    file << L"[Performance]\n";
    file << L"updateIntervalMs=" << config_.updateIntervalMs << L"\n";
    file << L"enableHardwareAcceleration=" << (config_.enableHardwareAcceleration ? L"true" : L"false") << L"\n";
    file << L"renderFPS=" << config_.renderFPS << L"\n\n";

    file << L"[Widgets]\n";
    file << L"widgetConfigPath=" << config_.widgetConfigPath << L"\n";
    file << L"enableWidgets=" << (config_.enableWidgets ? L"true" : L"false") << L"\n\n";

    file << L"[Security]\n";
    file << L"validateCodeSignatures=" << (config_.validateCodeSignatures ? L"true" : L"false") << L"\n";
    file << L"enableSecureMode=" << (config_.enableSecureMode ? L"true" : L"false") << L"\n\n";

    file << L"[Updates]\n";
    file << L"checkForUpdates=" << (config_.checkForUpdates ? L"true" : L"false") << L"\n";

    file.close();
    return true;
}

void ConfigurationManager::NotifyChange(const std::wstring& key, const std::wstring& value) {
    for (const auto& callback : changeCallbacks_) {
        try {
            callback(key, value);
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in config change callback: " + std::string(e.what()));
        }
    }
}

void ConfigurationManager::MarkDirty() {
    dirty_ = true;
}

void ConfigurationManager::MarkClean() {
    dirty_ = false;
}

std::wstring ConfigurationManager::GetDefaultConfigPath() const {
    // Get user's AppData folder
    wchar_t appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appDataPath))) {
        std::wstring path = appDataPath;
        path += L"\\RainmeterManager\\config.json";
        return path;
    }

    // Fallback to local directory
    return L"config/config.json";
}

void ConfigurationManager::ApplyConfiguration(const AppConfiguration& config) {
    // Load values from customSettings_ into config structure
    // This is called after loading from file
    
    // Application settings
    if (HasValue(L"version")) config_.version = GetValue(L"version");
    if (HasValue(L"locale")) config_.locale = GetValue(L"locale");
    if (HasValue(L"firstRun")) config_.firstRun = GetBool(L"firstRun");

    // Window settings
    if (HasValue(L"windowX")) config_.windowX = GetInt(L"windowX");
    if (HasValue(L"windowY")) config_.windowY = GetInt(L"windowY");
    if (HasValue(L"windowWidth")) config_.windowWidth = GetInt(L"windowWidth");
    if (HasValue(L"windowHeight")) config_.windowHeight = GetInt(L"windowHeight");
    if (HasValue(L"windowMaximized")) config_.windowMaximized = GetBool(L"windowMaximized");

    // Logging settings
    if (HasValue(L"logLevel")) config_.logLevel = GetValue(L"logLevel");
    if (HasValue(L"enableFileLogging")) config_.enableFileLogging = GetBool(L"enableFileLogging");
    if (HasValue(L"enableConsoleLogging")) config_.enableConsoleLogging = GetBool(L"enableConsoleLogging");
    if (HasValue(L"logRotationSize")) config_.logRotationSize = GetInt(L"logRotationSize");
    if (HasValue(L"logRotationCount")) config_.logRotationCount = GetInt(L"logRotationCount");

    // Performance settings
    if (HasValue(L"updateIntervalMs")) config_.updateIntervalMs = GetInt(L"updateIntervalMs");
    if (HasValue(L"renderFPS")) config_.renderFPS = GetInt(L"renderFPS");
    if (HasValue(L"enableHardwareAcceleration")) config_.enableHardwareAcceleration = GetBool(L"enableHardwareAcceleration");

    // Widget settings
    if (HasValue(L"widgetConfigPath")) config_.widgetConfigPath = GetValue(L"widgetConfigPath");
    if (HasValue(L"enableWidgets")) config_.enableWidgets = GetBool(L"enableWidgets");

    // Security settings
    if (HasValue(L"validateCodeSignatures")) config_.validateCodeSignatures = GetBool(L"validateCodeSignatures");
    if (HasValue(L"enableSecureMode")) config_.enableSecureMode = GetBool(L"enableSecureMode");

    // Update settings
    if (HasValue(L"checkForUpdates")) config_.checkForUpdates = GetBool(L"checkForUpdates");
}

void ConfigurationManager::ExtractConfiguration() {
    // Extract config_ values into customSettings_ for saving
    customSettings_[L"version"] = config_.version;
    customSettings_[L"locale"] = config_.locale;
    SetBool(L"firstRun", config_.firstRun);
    SetInt(L"windowX", config_.windowX);
    SetInt(L"windowY", config_.windowY);
    SetInt(L"windowWidth", config_.windowWidth);
    SetInt(L"windowHeight", config_.windowHeight);
    SetBool(L"windowMaximized", config_.windowMaximized);
    customSettings_[L"logLevel"] = config_.logLevel;
    SetBool(L"enableFileLogging", config_.enableFileLogging);
    SetBool(L"enableConsoleLogging", config_.enableConsoleLogging);
    SetInt(L"logRotationSize", config_.logRotationSize);
    SetInt(L"logRotationCount", config_.logRotationCount);
    SetInt(L"updateIntervalMs", config_.updateIntervalMs);
    SetBool(L"checkForUpdates", config_.checkForUpdates);
    customSettings_[L"widgetConfigPath"] = config_.widgetConfigPath;
    SetBool(L"enableWidgets", config_.enableWidgets);
    SetBool(L"validateCodeSignatures", config_.validateCodeSignatures);
    SetBool(L"enableSecureMode", config_.enableSecureMode);
    SetBool(L"enableHardwareAcceleration", config_.enableHardwareAcceleration);
    SetInt(L"renderFPS", config_.renderFPS);
}

} // namespace Config
} // namespace RainmeterManager
