using Microsoft.Extensions.Logging;
using System;

namespace RenderProcess.Runtime;

/// <summary>
/// DashboardService caches lightweight system metric snapshots for UI tiles.
/// It subscribes to PerformanceMonitor updates and provides a simple getter.
/// </summary>
public class DashboardService : IDisposable
{
    private readonly ILogger<DashboardService> _logger;
    private readonly PerformanceMonitor _perf;
    private readonly object _lock = new();
    private bool _disposed;

    public record TileSnapshot(
        float CpuPercent,
        ulong MemoryUsedMB,
        ulong MemoryTotalMB,
        float NetRxMBps,
        float NetTxMBps,
        DateTime TimestampUtc
    );

    private TileSnapshot _latest = new(0, 0, 0, 0, 0, DateTime.UtcNow);

    public DashboardService(ILogger<DashboardService> logger, PerformanceMonitor perf)
    {
        _logger = logger;
        _perf = perf;
        _perf.MetricsUpdated += OnMetricsUpdated;
        try { _perf.Start(); } catch { }
        _logger.LogInformation("DashboardService started");
    }

    private void OnMetricsUpdated(object? sender, SystemMetrics m)
    {
        lock (_lock)
        {
            _latest = new TileSnapshot(
                m.CpuTotalPercent,
                m.MemoryUsedMB,
                m.MemoryTotalMB,
                m.NetworkRecvMBps,
                m.NetworkSendMBps,
                DateTime.UtcNow
            );
        }
    }

    public TileSnapshot GetLatest()
    {
        lock (_lock) return _latest;
    }

    public void Dispose()
    {
        if (_disposed) return;
        try { _perf.MetricsUpdated -= OnMetricsUpdated; } catch { }
        _disposed = true;
    }
}

