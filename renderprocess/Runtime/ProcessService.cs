using Microsoft.Extensions.Logging;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Management;
using System.Threading;

namespace RenderProcess.Runtime;

public record ProcessInfoSnapshot(
    int Pid,
    int ParentPid,
    string Name,
    float CpuPercent,
    ulong WorkingSetMB,
    int Threads
);

/// <summary>
/// Provides read-only snapshots of running processes with CPU%, memory and thread counts.
/// CPU% is computed by sampling deltas over an interval.
/// </summary>
public class ProcessService : IDisposable
{
    private readonly ILogger<ProcessService> _logger;
    private readonly Timer _timer;
    private readonly object _lock = new();
    private readonly Dictionary<int, TimeSpan> _lastCpu = new();
    private DateTime _lastSample = DateTime.UtcNow;
    private readonly int _logicalCores = Environment.ProcessorCount;

    private List<ProcessInfoSnapshot> _latest = new();

    public ProcessService(ILogger<ProcessService> logger)
    {
        _logger = logger;
        _timer = new Timer(Sample, null, TimeSpan.FromMilliseconds(800), TimeSpan.FromMilliseconds(800));
        _logger.LogInformation("ProcessService started");
    }

    public List<ProcessInfoSnapshot> GetSnapshot()
    {
        lock (_lock)
        {
            return _latest;
        }
    }

    public void Dispose()
    {
        try { _timer?.Dispose(); } catch { }
    }

    private void Sample(object? state)
    {
        try
        {
            var now = DateTime.UtcNow;
            var dt = (now - _lastSample).TotalSeconds;
            if (dt <= 0.1) return;
            _lastSample = now;

            var procList = Process.GetProcesses();
            var parentMap = GetParentPidMap();
            var nextCpu = new Dictionary<int, TimeSpan>();
            var results = new List<ProcessInfoSnapshot>(procList.Length);

            foreach (var p in procList)
            {
                try
                {
                    var cpuTime = p.TotalProcessorTime;
                    var wsMB = (ulong)(p.WorkingSet64 / 1024 / 1024);
                    var threads = p.Threads.Count;
                    nextCpu[p.Id] = cpuTime;

                    float cpu = 0f;
                    if (_lastCpu.TryGetValue(p.Id, out var prev))
                    {
                        var delta = (cpuTime - prev).TotalSeconds;
                        cpu = (float)(delta / dt * 100.0 / _logicalCores);
                        if (cpu < 0) cpu = 0;
                    }

                    int ppid = 0;
                    if (parentMap.TryGetValue(p.Id, out var parent)) ppid = parent;

                    results.Add(new ProcessInfoSnapshot(
                        Pid: p.Id,
                        ParentPid: ppid,
                        Name: SafeName(p),
                        CpuPercent: cpu,
                        WorkingSetMB: wsMB,
                        Threads: threads
                    ));
                }
                catch { }
            }

            lock (_lock)
            {
                _latest = results.OrderByDescending(x => x.CpuPercent).ToList();
                _lastCpu.Clear();
                foreach (var kv in nextCpu) _lastCpu[kv.Key] = kv.Value;
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "ProcessService Sample failed");
        }
    }

    private static string SafeName(Process p)
    {
        try { return string.IsNullOrWhiteSpace(p.ProcessName) ? $"pid_{p.Id}" : p.ProcessName; } catch { return $"pid_{p.Id}"; }
    }

    private static Dictionary<int, int> GetParentPidMap()
    {
        var map = new Dictionary<int, int>();
        try
        {
            using var searcher = new ManagementObjectSearcher("SELECT ProcessId, ParentProcessId FROM Win32_Process");
            foreach (var obj in searcher.Get())
            {
                try
                {
                    int pid = Convert.ToInt32(obj["ProcessId"]);
                    int ppid = Convert.ToInt32(obj["ParentProcessId"]);
                    map[pid] = ppid;
                }
                catch { }
            }
        }
        catch { }
        return map;
    }
}

