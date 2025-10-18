#pragma once
// configuration_manager.h - Application configuration management
// Copyright (c) 2025 RainmeterManager. All rights reserved.

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <functional>
#include <vector>

namespace RainmeterManager {
namespace Config {

/**
 * @brief Application configuration structure
 */
struct AppConfiguration {
    // Application settings
    std::wstring version;
    std::wstring locale;
    bool firstRun;
    
    // Window settings
    int windowX;
    int windowY;
    int windowWidth;
    int windowHeight;
    bool windowMaximized;
    
    // Logging settings
    std::wstring logLevel;  // "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL", "FATAL"
    bool enableFileLogging;
    bool enableConsoleLogging;
    int logRotationSize;  // MB
    int logRotationCount;
    
    // Update settings
    int updateIntervalMs;
    bool checkForUpdates;
    
    // Widget settings
    std::wstring widgetConfigPath;
    bool enableWidgets;
    
    // Security settings
    bool validateCodeSignatures;
    bool enableSecureMode;
    
    // Performance settings
    bool enableHardwareAcceleration;
    int renderFPS;
    
    // Default constructor with sensible defaults
    AppConfiguration();
};

/**
 * @brief Configuration change callback
 */
using ConfigChangeCallback = std::function<void(const std::wstring& key, const std::wstring& value)>;

/**
 * @brief Central configuration manager for the application
 * 
 * Handles loading, saving, and managing all application configuration.
 * Supports JSON and INI formats. Thread-safe.
 */
class ConfigurationManager {
private:
    // Configuration data
    AppConfiguration config_;
    std::map<std::wstring, std::wstring> customSettings_;
    
    // File paths
    std::wstring configFilePath_;
    std::wstring configFormat_;  // "json" or "ini"
    
    // State
    bool initialized_;
    bool dirty_;  // Has config changed since last save?
    
    // Thread safety
    mutable std::mutex configMutex_;
    
    // Change notifications
    std::vector<ConfigChangeCallback> changeCallbacks_;

public:
    ConfigurationManager();
    ~ConfigurationManager();

    // Lifecycle
    bool Initialize(const std::wstring& configPath = L"");
    void Shutdown();
    
    // Configuration loading/saving
    bool Load();
    bool Load(const std::wstring& filePath);
    bool Save();
    bool Save(const std::wstring& filePath);
    bool IsLoaded() const { return initialized_; }
    bool IsDirty() const;
    
    // Get entire configuration
    AppConfiguration GetConfiguration() const;
    void SetConfiguration(const AppConfiguration& config);
    
    // Generic get/set for any key-value pair
    std::wstring GetValue(const std::wstring& key, const std::wstring& defaultValue = L"") const;
    void SetValue(const std::wstring& key, const std::wstring& value);
    bool HasValue(const std::wstring& key) const;
    void RemoveValue(const std::wstring& key);
    
    // Typed accessors for common types
    int GetInt(const std::wstring& key, int defaultValue = 0) const;
    void SetInt(const std::wstring& key, int value);
    bool GetBool(const std::wstring& key, bool defaultValue = false) const;
    void SetBool(const std::wstring& key, bool value);
    float GetFloat(const std::wstring& key, float defaultValue = 0.0f) const;
    void SetFloat(const std::wstring& key, float value);
    
    // Structured data accessors
    std::vector<std::wstring> GetArray(const std::wstring& key) const;
    void SetArray(const std::wstring& key, const std::vector<std::wstring>& values);
    
    // Configuration file management
    std::wstring GetConfigFilePath() const;
    void SetConfigFilePath(const std::wstring& path);
    std::wstring GetConfigFormat() const { return configFormat_; }
    void SetConfigFormat(const std::wstring& format); // "json" or "ini"
    
    // Change notifications
    void RegisterChangeCallback(ConfigChangeCallback callback);
    void ClearChangeCallbacks();
    
    // Export/Import
    bool ExportToFile(const std::wstring& filePath, const std::wstring& format);
    bool ImportFromFile(const std::wstring& filePath);
    
    // Reset to defaults
    void ResetToDefaults();
    
    // Migration support (for upgrading old config formats)
    bool MigrateFromVersion(const std::wstring& fromVersion);

private:
    // Format-specific loading/saving
    bool LoadJSON(const std::wstring& filePath);
    bool SaveJSON(const std::wstring& filePath);
    bool LoadINI(const std::wstring& filePath);
    bool SaveINI(const std::wstring& filePath);
    
    // Helper methods
    void NotifyChange(const std::wstring& key, const std::wstring& value);
    void MarkDirty();
    void MarkClean();
    std::wstring GetDefaultConfigPath() const;
    void ApplyConfiguration(const AppConfiguration& config);
    void ExtractConfiguration();
};

} // namespace Config
} // namespace RainmeterManager
