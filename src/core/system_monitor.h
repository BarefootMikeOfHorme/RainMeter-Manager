#pragma once
// system_monitor.h - System monitoring interface for widget data
// Copyright (c) 2025 RainmeterManager. All rights reserved.

#include <string>
#include <vector>
#include <memory>
#include <map>

namespace RainmeterManager {
namespace Core {

/**
 * @brief CPU information structure
 */
struct CPUInfo {
    std::vector<float> coreLoads;  // Load percentage per core (0-100)
    float totalLoad;                // Overall CPU load (0-100)
    float temperature;              // CPU temperature in Celsius (if available)
    int coreCount;
    std::string modelName;
};

/**
 * @brief Memory information structure
 */
struct MemoryInfo {
    uint64_t totalBytes;
    uint64_t availableBytes;
    uint64_t usedBytes;
    float usagePercent;
};

/**
 * @brief Process information structure
 */
struct ProcessInfo {
    uint32_t pid;
    std::wstring name;
    float cpuPercent;
    uint64_t memoryBytes;
    std::wstring status;  // "running", "sleeping", etc.
};

/**
 * @brief Network interface information
 */
struct NetworkInterfaceInfo {
    std::wstring name;
    std::wstring localIP;
    uint64_t bytesSent;
    uint64_t bytesReceived;
    uint64_t packetsSent;
    uint64_t packetsReceived;
    bool isUp;
    int speed;  // Mbps
};

/**
 * @brief Disk information structure
 */
struct DiskInfo {
    std::wstring drive;
    uint64_t totalBytes;
    uint64_t freeBytes;
    uint64_t usedBytes;
    float usagePercent;
};

/**
 * @brief System monitor interface
 * 
 * Provides system monitoring data for widgets.
 * Can be implemented using native Windows APIs, Python services (like manager_service.py),
 * or other data collection methods.
 */
class ISystemMonitor {
public:
    virtual ~ISystemMonitor() = default;

    // Lifecycle
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void Update() = 0;  // Refresh all data

    // CPU monitoring
    virtual CPUInfo GetCPUInfo() const = 0;
    virtual float GetCPULoad() const = 0;
    virtual std::vector<float> GetCPUCoreLoads() const = 0;

    // Memory monitoring  
    virtual MemoryInfo GetMemoryInfo() const = 0;
    virtual float GetMemoryUsagePercent() const = 0;

    // Process monitoring
    virtual std::vector<ProcessInfo> GetTopProcesses(int count = 10) const = 0;
    virtual ProcessInfo GetProcessByPID(uint32_t pid) const = 0;
    virtual std::vector<ProcessInfo> GetAllProcesses() const = 0;
    virtual bool KillProcess(uint32_t pid) = 0;

    // Network monitoring
    virtual std::vector<NetworkInterfaceInfo> GetNetworkInterfaces() const = 0;
    virtual NetworkInterfaceInfo GetNetworkInterface(const std::wstring& name) const = 0;
    virtual std::wstring GetPublicIP() const = 0;

    // Disk monitoring
    virtual std::vector<DiskInfo> GetDiskInfo() const = 0;
    virtual DiskInfo GetDiskInfo(const std::wstring& drive) const = 0;
};

/**
 * @brief Native Windows implementation of ISystemMonitor
 * 
 * Uses Windows Performance Counters, WMI, and other native APIs.
 */
class WindowsSystemMonitor : public ISystemMonitor {
private:
    // Cached data
    CPUInfo cpuInfo_;
    MemoryInfo memoryInfo_;
    std::vector<ProcessInfo> processes_;
    std::vector<NetworkInterfaceInfo> networkInterfaces_;
    std::vector<DiskInfo> disks_;

    // Update tracking
    DWORD lastUpdateTick_;
    DWORD updateIntervalMs_;

public:
    WindowsSystemMonitor();
    ~WindowsSystemMonitor() override;

    // ISystemMonitor implementation
    bool Initialize() override;
    void Shutdown() override;
    void Update() override;

    CPUInfo GetCPUInfo() const override;
    float GetCPULoad() const override;
    std::vector<float> GetCPUCoreLoads() const override;

    MemoryInfo GetMemoryInfo() const override;
    float GetMemoryUsagePercent() const override;

    std::vector<ProcessInfo> GetTopProcesses(int count = 10) const override;
    ProcessInfo GetProcessByPID(uint32_t pid) const override;
    std::vector<ProcessInfo> GetAllProcesses() const override;
    bool KillProcess(uint32_t pid) override;

    std::vector<NetworkInterfaceInfo> GetNetworkInterfaces() const override;
    NetworkInterfaceInfo GetNetworkInterface(const std::wstring& name) const override;
    std::wstring GetPublicIP() const override;

    std::vector<DiskInfo> GetDiskInfo() const override;
    DiskInfo GetDiskInfo(const std::wstring& drive) const override;

    // Configuration
    void SetUpdateInterval(DWORD intervalMs) { updateIntervalMs_ = intervalMs; }

private:
    // Internal data collection methods
    void UpdateCPUInfo();
    void UpdateMemoryInfo();
    void UpdateProcessInfo();
    void UpdateNetworkInfo();
    void UpdateDiskInfo();
};

/**
 * @brief Python service-based implementation of ISystemMonitor
 * 
 * Communicates with external Python service (like manager_service.py)
 * via HTTP or IPC to get system monitoring data.
 * 
 * TODO: Implement when Python service integration is ready
 */
class PythonServiceMonitor : public ISystemMonitor {
private:
    std::string serviceUrl_;  // e.g., "http://localhost:8000"
    
    // TODO: Add implementation
    
public:
    PythonServiceMonitor(const std::string& serviceUrl);
    ~PythonServiceMonitor() override;

    bool Initialize() override {
        // TODO: Implement - connect to Python service
        return false;  // Stub
    }
    
    void Shutdown() override {
        // TODO: Implement
    }
    
    void Update() override {
        // TODO: Implement - fetch data from service
    }

    // ISystemMonitor implementation - all return empty/default values for now
    CPUInfo GetCPUInfo() const override { return CPUInfo{}; }
    float GetCPULoad() const override { return 0.0f; }
    std::vector<float> GetCPUCoreLoads() const override { return {}; }
    MemoryInfo GetMemoryInfo() const override { return MemoryInfo{}; }
    float GetMemoryUsagePercent() const override { return 0.0f; }
    std::vector<ProcessInfo> GetTopProcesses(int count) const override { return {}; }
    ProcessInfo GetProcessByPID(uint32_t pid) const override { return ProcessInfo{}; }
    std::vector<ProcessInfo> GetAllProcesses() const override { return {}; }
    bool KillProcess(uint32_t pid) override { return false; }
    std::vector<NetworkInterfaceInfo> GetNetworkInterfaces() const override { return {}; }
    NetworkInterfaceInfo GetNetworkInterface(const std::wstring& name) const override { return NetworkInterfaceInfo{}; }
    std::wstring GetPublicIP() const override { return L""; }
    std::vector<DiskInfo> GetDiskInfo() const override { return {}; }
    DiskInfo GetDiskInfo(const std::wstring& drive) const override { return DiskInfo{}; }
};

} // namespace Core
} // namespace RainmeterManager
