using Microsoft.Extensions.Logging;
using RenderProcess.Interfaces;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace RenderProcess.Runtime;

public class PerformanceMonitorOptions
{
    public TimeSpan Interval { get; set; } = TimeSpan.FromSeconds(1);
    public bool EnableCpu { get; set; } = true;
    public bool EnableMemory { get; set; } = true;
    public bool EnableDisk { get; set; } = true;
    public bool EnableNetwork { get; set; } = true;
    public bool EnableGpu { get; set; } = true;
    public bool EnableBattery { get; set; } = true;
}

public class SystemMetrics
{
    public float CpuTotalPercent { get; set; }
    public Dictionary<string, float> CpuPerCorePercent { get; set; } = new();

    public ulong MemoryTotalMB { get; set; }
    public ulong MemoryAvailableMB { get; set; }
    public ulong MemoryUsedMB => MemoryTotalMB > MemoryAvailableMB ? MemoryTotalMB - MemoryAvailableMB : 0;

    public float DiskReadMBps { get; set; }
    public float DiskWriteMBps { get; set; }

    public float NetworkRecvMBps { get; set; }
    public float NetworkSendMBps { get; set; }

    public float GpuUtilPercent { get; set; }

    public float BatteryPercent { get; set; }
    public string PowerLineStatus { get; set; } = "Unknown";
}

/// <summary>
/// Windows system PerformanceMonitor: collects CPU, memory, disk, network, GPU, and battery metrics.
/// Safe by default, opt-in per category, disposable, can start/stop sampling.
/// </summary>
public class PerformanceMonitor : IDisposable
{
    private readonly ILogger<PerformanceMonitor> _logger;
    private readonly PerformanceMonitorOptions _options;
private System.Threading.Timer? _timer;
    private bool _disposed;

    // CPU counters
    private PerformanceCounter? _cpuTotal;
    private readonly List<(string Instance, PerformanceCounter Counter)> _cpuPerCore = new();

    // Disk counters
    private PerformanceCounter? _diskRead;
    private PerformanceCounter? _diskWrite;

    // Network counters per adapter
    private readonly List<(string Instance, PerformanceCounter Rx, PerformanceCounter Tx)> _netAdapters = new();

    // GPU counters (GPU Engine category, optional)
    private readonly List<PerformanceCounter> _gpuEngineCounters = new();

    private readonly object _lock = new();
    private SystemMetrics _latest = new();

    public event EventHandler<SystemMetrics>? MetricsUpdated;

    public PerformanceMonitor(ILogger<PerformanceMonitor> logger, PerformanceMonitorOptions? options = null)
    {
        _logger = logger;
        _options = options ?? new PerformanceMonitorOptions();
        InitializeCounters();
    }

    public void Start()
    {
        if (_timer != null) return;
        _timer = new System.Threading.Timer(async _ => await SampleAsync().ConfigureAwait(false), null, _options.Interval, _options.Interval);
        _logger.LogInformation("PerformanceMonitor started with interval {interval}", _options.Interval);
    }

    public void Stop()
    {
        _timer?.Dispose();
        _timer = null;
        _logger.LogInformation("PerformanceMonitor stopped");
    }

    public SystemMetrics GetLatest()
    {
        lock (_lock) return _latest;
    }

    private void InitializeCounters()
    {
        try
        {
            if (_options.EnableCpu)
            {
                _cpuTotal = new PerformanceCounter("Processor", "% Processor Time", "_Total", true);
                // Warm up
                _ = _cpuTotal.NextValue();

                var cat = new PerformanceCounterCategory("Processor");
                foreach (var inst in cat.GetInstanceNames().Where(n => n != "_Total"))
                {
                    var pc = new PerformanceCounter("Processor", "% Processor Time", inst, true);
                    _ = pc.NextValue();
                    _cpuPerCore.Add((inst, pc));
                }
            }
        }
        catch (Exception ex)
        {
            _logger.LogWarning(ex, "CPU counters not available");
        }

        try
        {
            if (_options.EnableDisk)
            {
                _diskRead = new PerformanceCounter("PhysicalDisk", "Disk Read Bytes/sec", "_Total", true);
                _diskWrite = new PerformanceCounter("PhysicalDisk", "Disk Write Bytes/sec", "_Total", true);
                _ = _diskRead.NextValue();
                _ = _diskWrite.NextValue();
            }
        }
        catch (Exception ex)
        {
            _logger.LogWarning(ex, "Disk counters not available");
        }

        try
        {
            if (_options.EnableNetwork)
            {
                var cat = new PerformanceCounterCategory("Network Interface");
                foreach (var inst in cat.GetInstanceNames())
                {
                    try
                    {
                        var rx = new PerformanceCounter("Network Interface", "Bytes Received/sec", inst, true);
                        var tx = new PerformanceCounter("Network Interface", "Bytes Sent/sec", inst, true);
                        _ = rx.NextValue();
                        _ = tx.NextValue();
                        _netAdapters.Add((inst, rx, tx));
                    }
                    catch { /* ignore adapter errors */ }
                }
            }
        }
        catch (Exception ex)
        {
            _logger.LogWarning(ex, "Network counters not available");
        }

        try
        {
            if (_options.EnableGpu)
            {
                // GPU Engine category (Windows 10+). Instances vary per system; sum utilization percentage.
                var catName = "GPU Engine";
                if (PerformanceCounterCategory.Exists(catName))
                {
                    var cat = new PerformanceCounterCategory(catName);
                    foreach (var inst in cat.GetInstanceNames())
                    {
                        try
                        {
                            // Consider 3D engine instances primarily (engtype_3D), but include all as fallback
                            if (inst.Contains("engtype_3D", StringComparison.OrdinalIgnoreCase) || inst.Contains("_eng", StringComparison.OrdinalIgnoreCase))
                            {
                                var c = new PerformanceCounter(catName, "Utilization Percentage", inst, true);
                                _ = c.NextValue();
                                _gpuEngineCounters.Add(c);
                            }
                        }
                        catch { /* ignore GPU counter errors */ }
                    }
                }
            }
        }
        catch (Exception ex)
        {
            _logger.LogDebug(ex, "GPU engine counters not available");
        }
    }

    private Task SampleAsync()
    {
        var snapshot = new SystemMetrics();

        // CPU
        if (_cpuTotal != null)
        {
            try { snapshot.CpuTotalPercent = _cpuTotal.NextValue(); } catch { }
            foreach (var (inst, pc) in _cpuPerCore)
            {
                try { snapshot.CpuPerCorePercent[inst] = pc.NextValue(); } catch { }
            }
        }

        // Memory
        if (_options.EnableMemory)
        {
            try
            {
                MEMORYSTATUSEX mem = new MEMORYSTATUSEX();
                if (GlobalMemoryStatusEx(ref mem))
                {
                    snapshot.MemoryTotalMB = (ulong)(mem.ullTotalPhys / (1024 * 1024));
                    snapshot.MemoryAvailableMB = (ulong)(mem.ullAvailPhys / (1024 * 1024));
                }
            }
            catch { }
        }

        // Disk
        if (_diskRead != null && _diskWrite != null)
        {
            try
            {
                snapshot.DiskReadMBps = _diskRead.NextValue() / (1024f * 1024f);
                snapshot.DiskWriteMBps = _diskWrite.NextValue() / (1024f * 1024f);
            }
            catch { }
        }

        // Network
        if (_netAdapters.Count > 0)
        {
            float rx = 0, tx = 0;
            foreach (var (_, rxc, txc) in _netAdapters)
            {
                try { rx += rxc.NextValue(); } catch { }
                try { tx += txc.NextValue(); } catch { }
            }
            snapshot.NetworkRecvMBps = rx / (1024f * 1024f);
            snapshot.NetworkSendMBps = tx / (1024f * 1024f);
        }

        // GPU
        if (_gpuEngineCounters.Count > 0)
        {
            float sum = 0;
            int count = 0;
            foreach (var c in _gpuEngineCounters)
            {
                try { sum += c.NextValue(); count++; } catch { }
            }
            snapshot.GpuUtilPercent = count > 0 ? Math.Min(100f, sum) : 0f; // engine utils often sum across engines
        }

        // Battery
        if (_options.EnableBattery)
        {
            try
            {
                var p = SystemInformation.PowerStatus;
                snapshot.BatteryPercent = p.BatteryLifePercent * 100f;
                snapshot.PowerLineStatus = p.PowerLineStatus.ToString();
            }
            catch { }
        }

        lock (_lock) _latest = snapshot;
        MetricsUpdated?.Invoke(this, snapshot);
        return Task.CompletedTask;
    }

    #region PInvoke
    [StructLayout(LayoutKind.Sequential)]
    private struct MEMORYSTATUSEX
    {
        public uint dwLength;
        public uint dwMemoryLoad;
        public ulong ullTotalPhys;
        public ulong ullAvailPhys;
        public ulong ullTotalPageFile;
        public ulong ullAvailPageFile;
        public ulong ullTotalVirtual;
        public ulong ullAvailVirtual;
        public ulong ullAvailExtendedVirtual;
    }

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool GlobalMemoryStatusEx(ref MEMORYSTATUSEX lpBuffer);

    #endregion

    public void Dispose()
    {
        if (_disposed) return;
        Stop();

        try { _cpuTotal?.Dispose(); } catch { }
        foreach (var (_, pc) in _cpuPerCore) { try { pc.Dispose(); } catch { } }
        try { _diskRead?.Dispose(); } catch { }
        try { _diskWrite?.Dispose(); } catch { }
        foreach (var (_, rx, tx) in _netAdapters) { try { rx.Dispose(); } catch { } try { tx.Dispose(); } catch { } }
        foreach (var c in _gpuEngineCounters) { try { c.Dispose(); } catch { } }

        _disposed = true;
    }
}

