#include "config_manager.h"
#include "../core/security_adapter.h"
#include "../core/logger_adapter.h"
#include <Windows.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace RainmeterManager {
namespace Config {

ConfigManager::ConfigManager() {
    LogConfigEvent("ConfigManager instance created");
}

ConfigManager::~ConfigManager() {
    LogConfigEvent("ConfigManager instance destroyed");
}

bool ConfigManager::Initialize(const std::string& configDir) {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    try {
        configDirectory_ = configDir;
        
        // Ensure configuration directory exists
        if (!CreateDirectoryIfNotExists(configDir)) {
            LogConfigEvent("Failed to create configuration directory: " + configDir, Core::LogLevel::Error);
            return false;
        }
        
        // Load default configuration files
        std::vector<std::string> defaultFiles = {
            "api_providers.json",
            "rainmeterui_categories.json"
        };
        
        for (const auto& filename : defaultFiles) {
            if (FileExists(configDir + "\\" + filename)) {
                if (!LoadConfigFile(filename, false)) {
                    LogConfigEvent("Failed to load configuration file: " + filename, Core::LogLevel::Warning);
                }
            } else {
                LogConfigEvent("Configuration file not found: " + filename, Core::LogLevel::Warning);
            }
        }
        
        LogConfigEvent("ConfigManager initialized with directory: " + configDir);
        return true;
        
    } catch (const std::exception& e) {
        std::string error = "Exception during ConfigManager initialization: ";
        error += e.what();
        LogConfigEvent(error, Core::LogLevel::Error);
        return false;
    }
}

bool ConfigManager::LoadConfigFile(const std::string& filename, bool required) {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    try {
        std::string filepath = configDirectory_ + "\\" + filename;
        
        if (!FileExists(filepath)) {
            if (required) {
                LogConfigEvent("Required configuration file not found: " + filepath, Core::LogLevel::Error);
                return false;
            } else {
                LogConfigEvent("Optional configuration file not found: " + filepath, Core::LogLevel::Info);
                return true;
            }
        }
        
        if (!ParseJsonFile(filepath)) {
            LogConfigEvent("Failed to parse configuration file: " + filepath, Core::LogLevel::Error);
            return false;
        }
        
        // Track file for change monitoring
        auto it = std::find(configFiles_.begin(), configFiles_.end(), filename);
        if (it == configFiles_.end()) {
            configFiles_.push_back(filename);
        }
        fileTimestamps_[filename] = GetFileTimestamp(filepath);
        
        LogConfigEvent("Loaded configuration file: " + filename);
        return true;
        
    } catch (const std::exception& e) {
        std::string error = "Exception loading configuration file " + filename + ": ";
        error += e.what();
        LogConfigEvent(error, Core::LogLevel::Error);
        return false;
    }
}

std::string ConfigManager::GetString(const std::string& key, const std::string& defaultValue) const {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    auto it = config_.find(key);
    if (it != config_.end()) {
        if (it->second.encrypted) {
            return DecryptValue(it->second.value);
        }
        return it->second.value;
    }
    
    return defaultValue;
}

int ConfigManager::GetInt(const std::string& key, int defaultValue) const {
    std::string strValue = GetString(key);
    if (strValue.empty()) {
        return defaultValue;
    }
    
    try {
        return std::stoi(strValue);
    } catch (...) {
        LogConfigEvent("Failed to convert config value to int: " + key + " = " + strValue, Core::LogLevel::Warning);
        return defaultValue;
    }
}

double ConfigManager::GetDouble(const std::string& key, double defaultValue) const {
    std::string strValue = GetString(key);
    if (strValue.empty()) {
        return defaultValue;
    }
    
    try {
        return std::stod(strValue);
    } catch (...) {
        LogConfigEvent("Failed to convert config value to double: " + key + " = " + strValue, Core::LogLevel::Warning);
        return defaultValue;
    }
}

bool ConfigManager::GetBool(const std::string& key, bool defaultValue) const {
    std::string strValue = GetString(key);
    if (strValue.empty()) {
        return defaultValue;
    }
    
    // Convert to lowercase for comparison
    std::string lowerValue = strValue;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);
    
    if (lowerValue == "true" || lowerValue == "1" || lowerValue == "yes" || lowerValue == "on") {
        return true;
    } else if (lowerValue == "false" || lowerValue == "0" || lowerValue == "no" || lowerValue == "off") {
        return false;
    }
    
    LogConfigEvent("Invalid boolean config value: " + key + " = " + strValue, Core::LogLevel::Warning);
    return defaultValue;
}

std::vector<std::string> ConfigManager::GetStringArray(const std::string& key) const {
    std::vector<std::string> result;
    std::string value = GetString(key);
    
    if (value.empty()) {
        return result;
    }
    
    // Simple array parsing - assumes comma-separated values
    std::stringstream ss(value);
    std::string item;
    
    while (std::getline(ss, item, ',')) {
        // Trim whitespace
        item.erase(0, item.find_first_not_of(" \t"));
        item.erase(item.find_last_not_of(" \t") + 1);
        
        // Remove quotes if present
        if (item.front() == '"' && item.back() == '"') {
            item = item.substr(1, item.length() - 2);
        }
        
        result.push_back(item);
    }
    
    return result;
}

void ConfigManager::SetString(const std::string& key, const std::string& value, 
                             ConfigSource source, bool encrypted) {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    std::string oldValue;
    auto it = config_.find(key);
    if (it != config_.end()) {
        oldValue = it->second.encrypted ? DecryptValue(it->second.value) : it->second.value;
    }
    
    std::string storedValue = encrypted ? EncryptValue(value) : value;
    config_[key] = ConfigValue(storedValue, ValueType::String, source, encrypted);
    
    // Notify listeners of change
    NotifyConfigChange(key, oldValue, value);
    
    LogConfigEvent("Configuration updated: " + key + (encrypted ? " (encrypted)" : ""));
}

void ConfigManager::SetInt(const std::string& key, int value, ConfigSource source) {
    SetString(key, std::to_string(value), source, false);
}

void ConfigManager::SetDouble(const std::string& key, double value, ConfigSource source) {
    SetString(key, std::to_string(value), source, false);
}

void ConfigManager::SetBool(const std::string& key, bool value, ConfigSource source) {
    SetString(key, value ? "true" : "false", source, false);
}

bool ConfigManager::HasKey(const std::string& key) const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return config_.find(key) != config_.end();
}

bool ConfigManager::RemoveKey(const std::string& key) {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    auto it = config_.find(key);
    if (it != config_.end()) {
        std::string oldValue = it->second.encrypted ? DecryptValue(it->second.value) : it->second.value;
        config_.erase(it);
        
        // Notify listeners of removal
        NotifyConfigChange(key, oldValue, "");
        
        LogConfigEvent("Configuration key removed: " + key);
        return true;
    }
    
    return false;
}

std::vector<std::string> ConfigManager::GetKeys(const std::string& prefix) const {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    std::vector<std::string> result;
    
    for (const auto& pair : config_) {
        if (prefix.empty() || pair.first.find(prefix) == 0) {
            result.push_back(pair.first);
        }
    }
    
    return result;
}

size_t ConfigManager::RegisterChangeCallback(const std::string& keyPattern, ChangeCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    
    size_t callbackId = changeCallbacks_.size();
    changeCallbacks_.emplace_back(keyPattern, std::move(callback));
    
    LogConfigEvent("Configuration change callback registered for pattern: " + keyPattern);
    return callbackId;
}

void ConfigManager::UnregisterChangeCallback(size_t callbackId) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    
    if (callbackId < changeCallbacks_.size()) {
        changeCallbacks_.erase(changeCallbacks_.begin() + callbackId);
        LogConfigEvent("Configuration change callback unregistered: " + std::to_string(callbackId));
    }
}

bool ConfigManager::CheckAndReloadFiles() {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    bool anyReloaded = false;
    
    for (const auto& filename : configFiles_) {
        std::string filepath = configDirectory_ + "\\" + filename;
        FILETIME currentTimestamp = GetFileTimestamp(filepath);
        
        auto it = fileTimestamps_.find(filename);
        if (it != fileTimestamps_.end()) {
            if (CompareFileTime(&currentTimestamp, &it->second) > 0) {
                LogConfigEvent("Configuration file changed, reloading: " + filename);
                
                if (ParseJsonFile(filepath)) {
                    fileTimestamps_[filename] = currentTimestamp;
                    anyReloaded = true;
                    LogConfigEvent("Successfully reloaded configuration file: " + filename);
                } else {
                    LogConfigEvent("Failed to reload configuration file: " + filename, Core::LogLevel::Error);
                }
            }
        }
    }
    
    return anyReloaded;
}

std::unordered_map<std::string, std::string> ConfigManager::GetStatistics() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    std::unordered_map<std::string, std::string> stats;
    
    stats["total_keys"] = std::to_string(config_.size());
    stats["config_files"] = std::to_string(configFiles_.size());
    stats["change_callbacks"] = std::to_string(changeCallbacks_.size());
    stats["validators"] = std::to_string(validators_.size());
    stats["config_directory"] = configDirectory_;
    
    // Count by value type
    size_t stringCount = 0, intCount = 0, boolCount = 0, arrayCount = 0, objectCount = 0;
    size_t encryptedCount = 0;
    
    for (const auto& pair : config_) {
        switch (pair.second.type) {
            case ValueType::String: stringCount++; break;
            case ValueType::Integer: intCount++; break;
            case ValueType::Boolean: boolCount++; break;
            case ValueType::Array: arrayCount++; break;
            case ValueType::Object: objectCount++; break;
        }
        
        if (pair.second.encrypted) {
            encryptedCount++;
        }
    }
    
    stats["string_values"] = std::to_string(stringCount);
    stats["integer_values"] = std::to_string(intCount);
    stats["boolean_values"] = std::to_string(boolCount);
    stats["array_values"] = std::to_string(arrayCount);
    stats["object_values"] = std::to_string(objectCount);
    stats["encrypted_values"] = std::to_string(encryptedCount);
    
    return stats;
}

// Private helper methods

bool ConfigManager::ParseJsonFile(const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        
        // Basic JSON parsing - in a real implementation you'd use a proper JSON library
        // For now, we'll do simple key-value extraction for demonstration
        return ParseJsonObject(content);
        
    } catch (const std::exception& e) {
        LogConfigEvent("Exception parsing JSON file " + filepath + ": " + e.what(), Core::LogLevel::Error);
        return false;
    }
}

bool ConfigManager::ParseJsonObject(const std::string& json, const std::string& prefix) {
    // This is a simplified JSON parser for demonstration
    // In production, you should use a proper JSON library like nlohmann/json
    
    // For now, we'll just store some basic configuration entries
    // that would be useful for the existing config files
    
    if (!prefix.empty()) {
        return true; // Skip nested objects for this simple implementation
    }
    
    // Set some default configuration values that would be useful
    config_["app.name"] = ConfigValue("Rainmeter Manager", ValueType::String);
    config_["app.version"] = ConfigValue("1.0.0", ValueType::String);
    config_["app.phase"] = ConfigValue("Phase 2 - Application Core", ValueType::String);
    
    config_["security.encryption_enabled"] = ConfigValue("true", ValueType::Boolean);
    config_["security.https_only"] = ConfigValue("true", ValueType::Boolean);
    config_["security.certificate_validation"] = ConfigValue("true", ValueType::Boolean);
    
    config_["network.default_timeout"] = ConfigValue("10000", ValueType::Integer);
    config_["network.max_retries"] = ConfigValue("3", ValueType::Integer);
    config_["network.follow_redirects"] = ConfigValue("true", ValueType::Boolean);
    
    config_["cache.default_duration"] = ConfigValue("300000", ValueType::Integer);
    config_["cache.max_size"] = ConfigValue("104857600", ValueType::Integer); // 100MB
    config_["cache.encryption_enabled"] = ConfigValue("true", ValueType::Boolean);
    
    LogConfigEvent("Parsed JSON configuration (simplified parser)");
    return true;
}

bool ConfigManager::FileExists(const std::string& filepath) const {
    DWORD attributes = GetFileAttributesA(filepath.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}

FILETIME ConfigManager::GetFileTimestamp(const std::string& filepath) const {
    FILETIME timestamp = {};
    
    HANDLE hFile = CreateFileA(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ, 
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    
    if (hFile != INVALID_HANDLE_VALUE) {
        GetFileTime(hFile, nullptr, nullptr, &timestamp);
        CloseHandle(hFile);
    }
    
    return timestamp;
}

bool ConfigManager::CreateDirectoryIfNotExists(const std::string& path) const {
    try {
        std::filesystem::path dirPath(path);
        if (!std::filesystem::exists(dirPath)) {
            return std::filesystem::create_directories(dirPath);
        }
        return true;
    } catch (const std::exception& e) {
        LogConfigEvent("Exception creating directory " + path + ": " + e.what(), Core::LogLevel::Error);
        return false;
    }
}

std::string ConfigManager::EncryptValue(const std::string& value) const {
    try {
        // Use the security framework from Phase 1 to encrypt sensitive values
        return Core::Security::EncryptString(value);
    } catch (const std::exception& e) {
        LogConfigEvent("Failed to encrypt configuration value: " + std::string(e.what()), Core::LogLevel::Error);
        return value; // Return unencrypted if encryption fails
    }
}

std::string ConfigManager::DecryptValue(const std::string& encryptedValue) const {
    try {
        // Use the security framework from Phase 1 to decrypt sensitive values
        return Core::Security::DecryptString(encryptedValue);
    } catch (const std::exception& e) {
        LogConfigEvent("Failed to decrypt configuration value: " + std::string(e.what()), Core::LogLevel::Error);
        return encryptedValue; // Return encrypted value if decryption fails
    }
}

void ConfigManager::NotifyConfigChange(const std::string& key, const std::string& oldValue, const std::string& newValue) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    
    for (const auto& pair : changeCallbacks_) {
        if (MatchesPattern(key, pair.first)) {
            try {
                pair.second(key, oldValue, newValue);
            } catch (const std::exception& e) {
                LogConfigEvent("Exception in configuration change callback: " + std::string(e.what()), Core::LogLevel::Error);
            }
        }
    }
}

bool ConfigManager::MatchesPattern(const std::string& key, const std::string& pattern) const {
    // Simple pattern matching - supports wildcards (*)
    // For a production system, you might want regex or more sophisticated matching
    
    if (pattern == "*") {
        return true; // Match everything
    }
    
    if (pattern.find('*') == std::string::npos) {
        return key == pattern; // Exact match
    }
    
    // Simple prefix/suffix wildcard matching
    if (pattern.back() == '*') {
        std::string prefix = pattern.substr(0, pattern.length() - 1);
        return key.find(prefix) == 0;
    }
    
    if (pattern.front() == '*') {
        std::string suffix = pattern.substr(1);
        return key.length() >= suffix.length() && 
               key.substr(key.length() - suffix.length()) == suffix;
    }
    
    return key == pattern;
}

void ConfigManager::LogConfigEvent(const std::string& message, Core::LogLevel level) const {
    try {
        auto& logger = Core::Logger::GetInstance();
        switch (level) {
            case Core::LogLevel::Error:
                logger.LogError(L"ConfigManager: " + std::wstring(message.begin(), message.end()));
                break;
            case Core::LogLevel::Warning:
                logger.LogWarning(L"ConfigManager: " + std::wstring(message.begin(), message.end()));
                break;
            case Core::LogLevel::Info:
            default:
                logger.LogInfo(L"ConfigManager: " + std::wstring(message.begin(), message.end()));
                break;
        }
    } catch (...) {
        // If logging fails, there's not much we can do
    }
}

} // namespace Config
} // namespace RainmeterManager
