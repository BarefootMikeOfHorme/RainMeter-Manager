#pragma once
// measure_base.h - Base classes for all widget data sources
// Based on analysis of 962 Rainmeter skins
// Copyright (c) 2025 RainmeterManager. All rights reserved.

#include <string>
#include <memory>
#include <chrono>
#include <functional>
#include <map>
#include <vector>
#include <mutex>
#include <atomic>

namespace RainmeterManager {
namespace Widgets {
namespace Measures {

/**
 * @brief Base measure types discovered from Rainmeter analysis
 */
enum class MeasureType {
    CPU,                    // System CPU usage (total or per-core)
    PhysicalMemory,        // RAM usage
    SwapMemory,            // Page file usage
    FreeDiskSpace,         // Disk space monitoring
    NetIn,                 // Network download
    NetOut,                // Network upload
    Time,                  // Date/time formatting
    Uptime,                // System uptime
    Plugin,                // External plugin (HWiNFO, NowPlaying, etc.)
    WebParser,             // HTTP requests with regex parsing
    Registry,              // Windows registry access
    Calc,                  // Mathematical calculations
    Script,                // Lua/script execution
    Counter,               // Windows performance counters
    Battery,               // Battery/power state
    SysInfo                // System information (OS, computer name, etc.)
};

/**
 * @brief Update behavior for measures
 */
struct MeasureUpdatePolicy {
    DWORD baseUpdateMs = 1000;      // Base update interval
    DWORD updateDivider = 1;         // Divider (actualUpdate = base * divider)
    bool updateOnDemand = false;     // Only update when requested
    bool cacheResults = true;        // Cache last result
    
    DWORD GetEffectiveUpdateInterval() const {
        return baseUpdateMs * updateDivider;
    }
};

/**
 * @brief Base interface for all measures (data sources)
 * 
 * Measures collect data from various sources (CPU, network, web, etc.)
 * and provide it to meters (visualizations).
 */
class IMeasure {
public:
    virtual ~IMeasure() = default;
    
    // Lifecycle
    virtual bool Initialize() = 0;
    virtual void Update() = 0;
    virtual void Shutdown() = 0;
    
    // Data access
    virtual double GetValue() const = 0;
    virtual std::wstring GetStringValue() const = 0;
    virtual bool IsValid() const = 0;
    
    // Metadata
    virtual MeasureType GetType() const = 0;
    virtual std::wstring GetName() const = 0;
    virtual DWORD GetLastUpdateTime() const = 0;
    
    // Configuration
    virtual void SetMinValue(double min) = 0;
    virtual void SetMaxValue(double max) = 0;
    virtual double GetMinValue() const = 0;
    virtual double GetMaxValue() const = 0;
    
    // Update control
    virtual void SetUpdatePolicy(const MeasureUpdatePolicy& policy) = 0;
    virtual MeasureUpdatePolicy GetUpdatePolicy() const = 0;
    virtual bool ShouldUpdate() const = 0;
};

/**
 * @brief Base implementation of IMeasure with common functionality
 */
class MeasureBase : public IMeasure {
protected:
    std::wstring name_;
    MeasureType type_;
    double value_;
    std::wstring stringValue_;
    double minValue_;
    double maxValue_;
    bool initialized_;
    bool valid_;
    
    MeasureUpdatePolicy updatePolicy_;
    DWORD lastUpdateTick_;
    
    mutable std::mutex valueMutex_;
    
public:
    MeasureBase(const std::wstring& name, MeasureType type);
    virtual ~MeasureBase();
    
    // IMeasure implementation
    bool Initialize() override;
    void Update() override;
    void Shutdown() override;
    
    double GetValue() const override;
    std::wstring GetStringValue() const override;
    bool IsValid() const override { return valid_; }
    
    MeasureType GetType() const override { return type_; }
    std::wstring GetName() const override { return name_; }
    DWORD GetLastUpdateTime() const override { return lastUpdateTick_; }
    
    void SetMinValue(double min) override { minValue_ = min; }
    void SetMaxValue(double max) override { maxValue_ = max; }
    double GetMinValue() const override { return minValue_; }
    double GetMaxValue() const override { return maxValue_; }
    
    void SetUpdatePolicy(const MeasureUpdatePolicy& policy) override;
    MeasureUpdatePolicy GetUpdatePolicy() const override { return updatePolicy_; }
    bool ShouldUpdate() const override;
    
protected:
    // Override these in derived classes
    virtual bool OnInitialize() = 0;
    virtual void OnUpdate() = 0;
    virtual void OnShutdown() = 0;
    
    // Helpers
    void SetValue(double value);
    void SetStringValue(const std::wstring& value);
    double Normalize(double value) const;  // Normalize to 0-1 range
};

/**
 * @brief CPU usage measure
 * Pattern from illustro\System.ini and Dashboard\CPU
 */
class CPUMeasure : public MeasureBase {
private:
    int processor_;  // 0 = average, 1-N = specific core
    PDH_HQUERY cpuQuery_;
    PDH_HCOUNTER cpuCounter_;
    
public:
    explicit CPUMeasure(const std::wstring& name, int processor = 0);
    ~CPUMeasure();
    
    void SetProcessor(int processor) { processor_ = processor; }
    int GetProcessor() const { return processor_; }
    
protected:
    bool OnInitialize() override;
    void OnUpdate() override;
    void OnShutdown() override;
};

/**
 * @brief Memory usage measure
 * Pattern from illustro\System.ini - PhysicalMemory and SwapMemory
 */
class MemoryMeasure : public MeasureBase {
public:
    enum class MemoryType {
        Physical,       // RAM
        Swap,          // Page file
        Virtual        // Virtual memory
    };
    
private:
    MemoryType memoryType_;
    bool returnPercentage_;
    
public:
    MemoryMeasure(const std::wstring& name, MemoryType type);
    ~MemoryMeasure() = default;
    
    void SetReturnPercentage(bool percentage) { returnPercentage_ = percentage; }
    bool GetReturnPercentage() const { return returnPercentage_; }
    
protected:
    bool OnInitialize() override;
    void OnUpdate() override;
    void OnShutdown() override;
};

/**
 * @brief Network traffic measure
 * Pattern from illustro\Network.ini and Evolucion\Red
 */
class NetworkMeasure : public MeasureBase {
public:
    enum class NetworkType {
        NetIn,          // Download speed
        NetOut,         // Upload speed
        NetTotal        // Total bandwidth (cumulative)
    };
    
private:
    NetworkType networkType_;
    int interface_;             // 0 = default, specific adapter
    ULONGLONG netInSpeed_;      // Max download in bytes/sec
    ULONGLONG netOutSpeed_;     // Max upload in bytes/sec
    bool cumulative_;           // Track total bytes
    
    ULONGLONG lastBytes_;
    DWORD lastSampleTime_;
    
public:
    NetworkMeasure(const std::wstring& name, NetworkType type);
    ~NetworkMeasure() = default;
    
    void SetInterface(int iface) { interface_ = iface; }
    void SetNetInSpeed(ULONGLONG speed) { netInSpeed_ = speed; }
    void SetNetOutSpeed(ULONGLONG speed) { netOutSpeed_ = speed; }
    void SetCumulative(bool cumulative) { cumulative_ = cumulative; }
    
protected:
    bool OnInitialize() override;
    void OnUpdate() override;
    void OnShutdown() override;
};

/**
 * @brief Disk space measure
 */
class DiskMeasure : public MeasureBase {
public:
    enum class DiskMetric {
        FreeSpace,
        UsedSpace,
        TotalSpace,
        FreePercent,
        UsedPercent
    };
    
private:
    std::wstring drive_;
    DiskMetric metric_;
    
public:
    DiskMeasure(const std::wstring& name, const std::wstring& drive);
    ~DiskMeasure() = default;
    
    void SetDrive(const std::wstring& drive) { drive_ = drive; }
    void SetMetric(DiskMetric metric) { metric_ = metric; }
    
protected:
    bool OnInitialize() override;
    void OnUpdate() override;
    void OnShutdown() override;
};

/**
 * @brief Time/date measure
 * Pattern from illustro\Clock.ini
 */
class TimeMeasure : public MeasureBase {
private:
    std::wstring format_;  // strftime format string
    
public:
    explicit TimeMeasure(const std::wstring& name);
    ~TimeMeasure() = default;
    
    void SetFormat(const std::wstring& format) { format_ = format; }
    std::wstring GetFormat() const { return format_; }
    
protected:
    bool OnInitialize() override;
    void OnUpdate() override;
    void OnShutdown() override;
};

/**
 * @brief WebParser measure for HTTP requests
 * Pattern from Evolucion\Clima and illustro\Network (external IP)
 */
class WebParserMeasure : public MeasureBase {
private:
    std::wstring url_;
    std::wstring regExp_;
    int stringIndex_;
    std::map<std::wstring, std::wstring> headers_;
    std::wstring substitutePattern_;
    
    std::vector<std::wstring> parsedResults_;
    std::thread downloadThread_;
    std::atomic<bool> downloading_;
    
public:
    explicit WebParserMeasure(const std::wstring& name);
    ~WebParserMeasure();
    
    void SetUrl(const std::wstring& url) { url_ = url; }
    void SetRegExp(const std::wstring& regExp) { regExp_ = regExp; }
    void SetStringIndex(int index) { stringIndex_ = index; }
    void AddHeader(const std::wstring& key, const std::wstring& value);
    void SetSubstitute(const std::wstring& pattern) { substitutePattern_ = pattern; }
    
protected:
    bool OnInitialize() override;
    void OnUpdate() override;
    void OnShutdown() override;
    
private:
    void DownloadAndParse();
    bool ParseResponse(const std::string& response);
};

/**
 * @brief Plugin measure interface
 * Pattern from Dashboard GPU monitoring (HWiNFO), Player (NowPlaying)
 * 
 * Plugins are loaded dynamically from DLLs and expose this interface
 */
class IPluginMeasure {
public:
    virtual ~IPluginMeasure() = default;
    
    virtual bool Initialize(const std::map<std::wstring, std::wstring>& config) = 0;
    virtual void Update() = 0;
    virtual double GetValue() = 0;
    virtual std::wstring GetStringValue() = 0;
    virtual void ExecuteCommand(const std::wstring& command) = 0;
    virtual void Shutdown() = 0;
};

/**
 * @brief Plugin measure wrapper
 * Loads external DLL plugins and wraps their interface
 */
class PluginMeasure : public MeasureBase {
private:
    std::wstring pluginName_;
    std::wstring pluginPath_;
    HMODULE pluginModule_;
    IPluginMeasure* pluginInstance_;
    std::map<std::wstring, std::wstring> config_;
    
public:
    PluginMeasure(const std::wstring& name, const std::wstring& pluginName);
    ~PluginMeasure();
    
    void SetConfigValue(const std::wstring& key, const std::wstring& value);
    void ExecutePluginCommand(const std::wstring& command);
    
protected:
    bool OnInitialize() override;
    void OnUpdate() override;
    void OnShutdown() override;
    
private:
    bool LoadPlugin();
    void UnloadPlugin();
};

/**
 * @brief Calc measure for mathematical expressions
 * Pattern: used extensively for conditional logic and value transformation
 */
class CalcMeasure : public MeasureBase {
private:
    std::wstring formula_;
    std::map<std::wstring, IMeasure*> variableReferences_;
    
public:
    explicit CalcMeasure(const std::wstring& name);
    ~CalcMeasure() = default;
    
    void SetFormula(const std::wstring& formula) { formula_ = formula; }
    void BindVariable(const std::wstring& varName, IMeasure* measure);
    
protected:
    bool OnInitialize() override;
    void OnUpdate() override;
    void OnShutdown() override;
    
private:
    double EvaluateFormula();
};

/**
 * @brief Registry measure
 * Pattern from Dashboard CPU (CPU name from registry)
 */
class RegistryMeasure : public MeasureBase {
private:
    HKEY rootKey_;
    std::wstring regKey_;
    std::wstring regValue_;
    std::wstring substitutePattern_;
    
public:
    explicit RegistryMeasure(const std::wstring& name);
    ~RegistryMeasure() = default;
    
    void SetRootKey(const std::wstring& root);
    void SetKey(const std::wstring& key) { regKey_ = key; }
    void SetValue(const std::wstring& value) { regValue_ = value; }
    void SetSubstitute(const std::wstring& pattern) { substitutePattern_ = pattern; }
    
protected:
    bool OnInitialize() override;
    void OnUpdate() override;
    void OnShutdown() override;
};

/**
 * @brief Measure factory for creating measures by type
 */
class MeasureFactory {
public:
    static std::unique_ptr<IMeasure> CreateMeasure(
        MeasureType type,
        const std::wstring& name,
        const std::map<std::wstring, std::wstring>& config
    );
    
    static std::unique_ptr<IMeasure> CreateFromConfig(
        const std::wstring& configString
    );
};

} // namespace Measures
} // namespace Widgets
} // namespace RainmeterManager
