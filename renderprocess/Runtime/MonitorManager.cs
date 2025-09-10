using Microsoft.Extensions.Logging;
using RenderProcess.Interfaces;
using System;
using System.Linq;
using System.Windows.Forms;

namespace RenderProcess.Runtime;

public class MonitorManager
{
    private readonly ILogger<MonitorManager> _logger;

    public MonitorManager(ILogger<MonitorManager> logger)
    {
        _logger = logger;
        _logger.LogInformation("MonitorManager initialized (stub)");
    }

    public MonitorInfo[] GetAllMonitors()
    {
        return Screen.AllScreens.Select((s, i) => new MonitorInfo
        {
            MonitorId = i,
            HMonitor = IntPtr.Zero,
            Bounds = new RenderRect(s.Bounds.X, s.Bounds.Y, s.Bounds.Width, s.Bounds.Height),
            WorkArea = new RenderRect(s.WorkingArea.X, s.WorkingArea.Y, s.WorkingArea.Width, s.WorkingArea.Height),
            DpiX = 96.0f,
            DpiY = 96.0f,
            IsPrimary = s.Primary,
            DeviceName = s.DeviceName
        }).ToArray();
    }
}

