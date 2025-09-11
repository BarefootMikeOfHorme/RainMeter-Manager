using Microsoft.Extensions.Logging;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Management;
using System.Threading;
using System.IO;
using System.Security.Cryptography.X509Certificates;

namespace RenderProcess.Runtime;

public record ProcessInfoSnapshot(
    int Pid,
    int ParentPid,
    string Name,
    float CpuPercent,
    ulong WorkingSetMB,
    int Threads,
    string ImagePath,
    string CommandLine,
    float IoReadMBps,
    float IoWriteMBps,
    string Publisher,
    string IntegrityLevel,
    bool IsElevated
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

    // Per-process IO counters mapping (Process category)
    private readonly Dictionary<int, (PerformanceCounter Read, PerformanceCounter Write)> _procIo = new();

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
        var meta = GetProcessMeta();
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
                string cmd = string.Empty;
                string path = string.Empty;
                if (meta.TryGetValue(p.Id, out var m))
                {
                    ppid = m.ParentPid;
                    cmd = m.CommandLine ?? string.Empty;
                    path = m.ExecutablePath ?? TryGetPath(p);
                }
                else
                {
                    path = TryGetPath(p);
                }

                var (ioR, ioW) = GetProcessIoMBps(p.Id);
                var (publisher, _) = VerifySignature(path);
                var (integrity, elevated) = GetIntegrityAndElevation(p);

                results.Add(new ProcessInfoSnapshot(
                    Pid: p.Id,
                    ParentPid: ppid,
                    Name: SafeName(p),
                    CpuPercent: cpu,
                    WorkingSetMB: wsMB,
                    Threads: threads,
                    ImagePath: path,
                    CommandLine: cmd,
                    IoReadMBps: ioR,
                    IoWriteMBps: ioW,
                    Publisher: publisher,
                    IntegrityLevel: integrity,
                    IsElevated: elevated
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

private static string TryGetPath(Process p)
{
    try { return p.MainModule?.FileName ?? string.Empty; } catch { return string.Empty; }
}

private (float ReadMBps, float WriteMBps) GetProcessIoMBps(int pid)
{
    try
    {
        if (!_procIo.TryGetValue(pid, out var counters))
        {
            var cat = new PerformanceCounterCategory("Process");
            foreach (var inst in cat.GetInstanceNames())
            {
                try
                {
                    using var idPc = new PerformanceCounter("Process", "ID Process", inst, true);
                    if ((int)idPc.NextValue() == pid)
                    {
                        var r = new PerformanceCounter("Process", "IO Read Bytes/sec", inst, true);
                        var w = new PerformanceCounter("Process", "IO Write Bytes/sec", inst, true);
                        _ = r.NextValue(); _ = w.NextValue();
                        _procIo[pid] = (r, w);
                        counters = (r, w);
                        break;
                    }
                }
                catch { }
            }
        }
        if (counters.Read != null && counters.Write != null)
        {
            float rm = counters.Read.NextValue() / (1024f * 1024f);
            float wm = counters.Write.NextValue() / (1024f * 1024f);
            return (rm, wm);
        }
    }
    catch { }
    return (0f, 0f);
}

private (string Integrity, bool Elevated) GetIntegrityAndElevation(Process p)
{
    const uint TOKEN_QUERY = 0x0008;
    const int TokenElevation = 20;
    const int TokenIntegrityLevel = 25;

    try
    {
        if (!OpenProcessToken(p.Handle, TOKEN_QUERY, out var hToken)) return ("Unknown", false);
        try
        {
            // Elevation
            int len;
            bool elevated = false;
            IntPtr elevPtr = System.Runtime.InteropServices.Marshal.AllocHGlobal(sizeof(int));
            try
            {
                if (GetTokenInformation(hToken, TokenElevation, elevPtr, sizeof(int), out len))
                {
                    elevated = System.Runtime.InteropServices.Marshal.ReadInt32(elevPtr) != 0;
                }
            }
            finally { System.Runtime.InteropServices.Marshal.FreeHGlobal(elevPtr); }

            // Integrity
            string integrity = "Unknown";
            GetTokenInformation(hToken, TokenIntegrityLevel, IntPtr.Zero, 0, out len);
            var buf = System.Runtime.InteropServices.Marshal.AllocHGlobal(len);
            try
            {
                if (GetTokenInformation(hToken, TokenIntegrityLevel, buf, len, out len))
                {
                    var sid = System.Runtime.InteropServices.Marshal.ReadIntPtr(buf, IntPtr.Size); // TOKEN_MANDATORY_LABEL->SID pointer offset depends; fallback heuristic
                    int subAuthCount = System.Runtime.InteropServices.Marshal.ReadByte(sid, 1);
                    int integrityRid = System.Runtime.InteropServices.Marshal.ReadInt32(sid, 8 + (subAuthCount - 1) * 4);
                    integrity = integrityRid switch
                    {
                        0x00001000 => "Low",
                        0x00002000 => "Medium",
                        0x00003000 => "High",
                        0x00004000 => "System",
                        _ => "Unknown"
                    };
                }
            }
            finally { System.Runtime.InteropServices.Marshal.FreeHGlobal(buf); }

            return (integrity, elevated);
        }
        finally { CloseHandle(hToken); }
    }
    catch { return ("Unknown", false); }
}

private (string Publisher, bool IsValid) VerifySignature(string path)
{
    try
    {
        if (string.IsNullOrEmpty(path) || !File.Exists(path)) return (string.Empty, false);
        try
        {
            var cert = new X509Certificate2(X509Certificate.CreateFromSignedFile(path));
            string publisher = cert.GetNameInfo(X509NameType.SimpleName, false);
            return (publisher ?? string.Empty, true);
        }
        catch { return (string.Empty, false); }
    }
    catch { return (string.Empty, false); }
}

[System.Runtime.InteropServices.DllImport("advapi32.dll", SetLastError = true)]
private static extern bool OpenProcessToken(IntPtr ProcessHandle, uint DesiredAccess, out IntPtr TokenHandle);

[System.Runtime.InteropServices.DllImport("advapi32.dll", SetLastError = true)]
private static extern bool GetTokenInformation(IntPtr TokenHandle, int TokenInformationClass, IntPtr TokenInformation, int TokenInformationLength, out int ReturnLength);

[System.Runtime.InteropServices.DllImport("kernel32.dll", SetLastError = true)]
private static extern bool CloseHandle(IntPtr hObject);

private record ProcMeta(int ParentPid, string? CommandLine, string? ExecutablePath);

private static Dictionary<int, ProcMeta> GetProcessMeta()
{
    var map = new Dictionary<int, ProcMeta>();
    try
    {
        using var searcher = new ManagementObjectSearcher("SELECT ProcessId, ParentProcessId, CommandLine, ExecutablePath FROM Win32_Process");
        foreach (var obj in searcher.Get())
        {
            try
            {
                int pid = Convert.ToInt32(obj["ProcessId"]);
                int ppid = Convert.ToInt32(obj["ParentProcessId"]);
                string? cmd = obj["CommandLine"] as string;
                string? exe = obj["ExecutablePath"] as string;
                map[pid] = new ProcMeta(ppid, cmd, exe);
            }
            catch { }
        }
    }
    catch { }
    return map;
}
}

