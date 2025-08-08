#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>
#include <vector>
#include "../core/logger.h"

namespace RainmeterManager {
namespace Config {

/**
 * @brief Configuration Manager - Centralized Settings Management
 * 
 * Enterprise-grade configuration management system providing:
 * - JSON configuration file parsing and validation
 * - Type-safe configuration value access
 * - Configuration change notifications
 * - Hot-reload support for development
 * - Encrypted storage for sensitive data
 * - Validation and schema enforcement
 * 
 * Phase 2 Implementation - Core Application Layer
 */
class ConfigManager {
public:
    // Configuration value types
    enum class ValueType {
        String,
        Integer,
        Double,
        Boolean,
        Array,
        Object
    };

    // Configuration source types
    enum class ConfigSource {
        JsonFile,
        Registry,
        Environment,
        CommandLine
    };

    // Configuration change callback
    using ChangeCallback = std::function<void(const std::string& key, const std::string& oldValue, const std::string& newValue)>;

private:
    struct ConfigValue {
        std::string value;
        ValueType type;
        ConfigSource source;
        bool encrypted;
        
        ConfigValue(const std::string& v = "", ValueType t = ValueType::String, 
                   ConfigSource s = ConfigSource::JsonFile, bool enc = false)
            : value(v), type(t), source(s), encrypted(enc) {}
    };

    // Configuration storage
    std::unordered_map<std::string, ConfigValue> config_;
    mutable std::mutex configMutex_;
    
    // File monitoring
    std::string configDirectory_;
    std::vector<std::string> configFiles_;
    std::unordered_map<std::string, FILETIME> fileTimestamps_;
    
    // Change notification
    std::vector<std::pair<std::string, ChangeCallback>> changeCallbacks_;
    std::mutex callbackMutex_;
    
    // Schema validation
    std::unordered_map<std::string, std::function<bool(const std::string&)>> validators_;

public:
    ConfigManager();
    ~ConfigManager();

    // Prevent copy/move
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    ConfigManager(ConfigManager&&) = delete;
    ConfigManager& operator=(ConfigManager&&) = delete;

    /**
     * @brief Initialize configuration manager with config directory
     * @param configDir Path to configuration directory
     * @return true if initialization successful
     */
    bool Initialize(const std::string& configDir);

    /**
     * @brief Load configuration from file
     * @param filename Configuration file name (relative to config directory)
     * @param required If true, missing file is an error
     * @return true if load successful
     */
    bool LoadConfigFile(const std::string& filename, bool required = true);

    /**
     * @brief Save configuration to file
     * @param filename Configuration file name to save
     * @return true if save successful
     */
    bool SaveConfigFile(const std::string& filename);

    /**
     * @brief Get string configuration value
     * @param key Configuration key (supports dot notation for nested values)
     * @param defaultValue Default value if key not found
     * @return Configuration value
     */
    std::string GetString(const std::string& key, const std::string& defaultValue = "") const;

    /**
     * @brief Get integer configuration value
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return Configuration value
     */
    int GetInt(const std::string& key, int defaultValue = 0) const;

    /**
     * @brief Get double configuration value
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return Configuration value
     */
    double GetDouble(const std::string& key, double defaultValue = 0.0) const;

    /**
     * @brief Get boolean configuration value
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return Configuration value
     */
    bool GetBool(const std::string& key, bool defaultValue = false) const;

    /**
     * @brief Get array of strings from configuration
     * @param key Configuration key
     * @return Vector of string values
     */
    std::vector<std::string> GetStringArray(const std::string& key) const;

    /**
     * @brief Set configuration value
     * @param key Configuration key
     * @param value Configuration value
     * @param source Configuration source
     * @param encrypted Whether value should be encrypted
     */
    void SetString(const std::string& key, const std::string& value, 
                   ConfigSource source = ConfigSource::JsonFile, bool encrypted = false);

    /**
     * @brief Set integer configuration value
     * @param key Configuration key
     * @param value Configuration value
     * @param source Configuration source
     */
    void SetInt(const std::string& key, int value, ConfigSource source = ConfigSource::JsonFile);

    /**
     * @brief Set double configuration value
     * @param key Configuration key
     * @param value Configuration value
     * @param source Configuration source
     */
    void SetDouble(const std::string& key, double value, ConfigSource source = ConfigSource::JsonFile);

    /**
     * @brief Set boolean configuration value
     * @param key Configuration key
     * @param value Configuration value
     * @param source Configuration source
     */
    void SetBool(const std::string& key, bool value, ConfigSource source = ConfigSource::JsonFile);

    /**
     * @brief Check if configuration key exists
     * @param key Configuration key
     * @return true if key exists
     */
    bool HasKey(const std::string& key) const;

    /**
     * @brief Remove configuration key
     * @param key Configuration key to remove
     * @return true if key was removed
     */
    bool RemoveKey(const std::string& key);

    /**
     * @brief Get all configuration keys with optional prefix filter
     * @param prefix Key prefix filter (empty for all keys)
     * @return Vector of matching keys
     */
    std::vector<std::string> GetKeys(const std::string& prefix = "") const;

    /**
     * @brief Register configuration change callback
     * @param keyPattern Key pattern to monitor (supports wildcards)
     * @param callback Callback function to invoke on changes
     * @return Callback ID for unregistration
     */
    size_t RegisterChangeCallback(const std::string& keyPattern, ChangeCallback callback);

    /**
     * @brief Unregister configuration change callback
     * @param callbackId Callback ID returned from RegisterChangeCallback
     */
    void UnregisterChangeCallback(size_t callbackId);

    /**
     * @brief Register configuration value validator
     * @param key Configuration key to validate
     * @param validator Validation function (returns true if valid)
     */
    void RegisterValidator(const std::string& key, std::function<bool(const std::string&)> validator);

    /**
     * @brief Validate all configuration values
     * @return true if all values are valid
     */
    bool ValidateConfiguration() const;

    /**
     * @brief Check for configuration file changes and reload if necessary
     * @return true if any files were reloaded
     */
    bool CheckAndReloadFiles();

    /**
     * @brief Get configuration statistics
     * @return Map of statistics (key count, file count, etc.)
     */
    std::unordered_map<std::string, std::string> GetStatistics() const;

    /**
     * @brief Export configuration as JSON string
     * @param prettify Whether to format JSON for readability
     * @return JSON string representation
     */
    std::string ExportAsJson(bool prettify = true) const;

    /**
     * @brief Import configuration from JSON string
     * @param jsonStr JSON configuration string
     * @param source Configuration source to assign
     * @return true if import successful
     */
    bool ImportFromJson(const std::string& jsonStr, ConfigSource source = ConfigSource::JsonFile);

private:
    // JSON parsing helpers
    bool ParseJsonFile(const std::string& filepath);
    bool ParseJsonObject(const std::string& json, const std::string& prefix = "");
    std::string ExtractJsonValue(const std::string& json, const std::string& key) const;
    ValueType DetermineValueType(const std::string& value) const;
    
    // File system helpers
    bool FileExists(const std::string& filepath) const;
    FILETIME GetFileTimestamp(const std::string& filepath) const;
    bool CreateDirectoryIfNotExists(const std::string& path) const;
    
    // Encryption helpers
    std::string EncryptValue(const std::string& value) const;
    std::string DecryptValue(const std::string& encryptedValue) const;
    
    // Notification helpers
    void NotifyConfigChange(const std::string& key, const std::string& oldValue, const std::string& newValue);
    bool MatchesPattern(const std::string& key, const std::string& pattern) const;
    
    // Validation helpers
    bool ValidateValue(const std::string& key, const std::string& value) const;
    
    // Logging helper
    void LogConfigEvent(const std::string& message, Core::LogLevel level = Core::LogLevel::Info) const;
};

} // namespace Config
} // namespace RainmeterManager
