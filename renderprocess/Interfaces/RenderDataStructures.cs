using System;
using System.Collections.Generic;

namespace RenderProcess.Interfaces;

// ===== ENUMERATIONS =====

/// <summary>
/// Render backend types available in the system
/// </summary>
public enum RenderBackendType : uint
{
    SkiaSharp = 0,
    Direct3D = 1,
    WebView = 2,
    Auto = 99
}

/// <summary>
/// Content source types for rendering
/// </summary>
public enum ContentSourceType : uint
{
    Static = 0,      // Static content (text, images)
    Web = 1,         // Web URLs, HTML content
    API = 2,         // REST/GraphQL APIs
    Media = 3,       // Video/audio streams
    File = 4,        // Local files
    Office = 5,      // Excel, Word, PowerPoint
    Custom = 99      // Custom content source
}

/// <summary>
/// Render command types for IPC communication
/// </summary>
public enum RenderCommandType : uint
{
    Initialize = 0,
    Render = 1,
    Resize = 2,
    Destroy = 3,
    SwitchBackend = 4,
    UpdateContent = 5,
    SetProperty = 6,

    // Diagnostics / Data queries
    GetSystemSnapshot = 100,
    GetProcessSnapshot = 101
}

/// <summary>
/// Render result status codes
/// </summary>
public enum RenderResultStatus : uint
{
    Success = 0,
    Failure = 1,
    Pending = 2,
    BackendNotSupported = 3,
    ContentLoadError = 4,
    InvalidParameters = 5
}

// ===== DATA STRUCTURES =====

/// <summary>
/// Rectangle structure for bounds and positioning
/// </summary>
public struct RenderRect
{
    public int X { get; set; }
    public int Y { get; set; }
    public int Width { get; set; }
    public int Height { get; set; }

    public RenderRect(int x = 0, int y = 0, int width = 0, int height = 0)
    {
        X = x;
        Y = y;
        Width = width;
        Height = height;
    }

    public override string ToString() => $"({X}, {Y}, {Width}x{Height})";
}

/// <summary>
/// Color structure for rendering
/// </summary>
public struct RenderColor
{
    public byte R { get; set; }
    public byte G { get; set; }
    public byte B { get; set; }
    public byte A { get; set; }

    public RenderColor(byte r = 0, byte g = 0, byte b = 0, byte a = 255)
    {
        R = r;
        G = g;
        B = b;
        A = a;
    }

    public static RenderColor Transparent => new(0, 0, 0, 0);
    public static RenderColor Black => new(0, 0, 0, 255);
    public static RenderColor White => new(255, 255, 255, 255);

    public override string ToString() => $"RGBA({R}, {G}, {B}, {A})";
}

/// <summary>
/// Content parameters for loading and rendering content
/// </summary>
public class ContentParameters
{
    public ContentSourceType SourceType { get; set; } = ContentSourceType.Static;
    public string SourceUrl { get; set; } = string.Empty;
    public string TemplateData { get; set; } = string.Empty;
    public string AuthToken { get; set; } = string.Empty;
    public int RefreshIntervalMs { get; set; } = 0;
    public bool CacheEnabled { get; set; } = true;
    public List<KeyValuePair<string, string>> CustomHeaders { get; set; } = new();
    public List<KeyValuePair<string, string>> Parameters { get; set; } = new();

    public ContentParameters() { }

    public ContentParameters(ContentSourceType sourceType, string sourceUrl)
    {
        SourceType = sourceType;
        SourceUrl = sourceUrl;
    }
}

/// <summary>
/// Render properties for visual appearance and behavior
/// </summary>
public class RenderProperties
{
    public float Opacity { get; set; } = 1.0f;
    public bool Visible { get; set; } = true;
    public bool ClickThrough { get; set; } = false;
    public bool TopMost { get; set; } = false;
    public RenderColor BackgroundColor { get; set; } = RenderColor.Transparent;
    public int ZOrder { get; set; } = 0;
    public bool EnableAnimations { get; set; } = true;
    public int TargetFps { get; set; } = 60;
    public bool EnableVSync { get; set; } = true;

    // Advanced properties
    public float ScaleX { get; set; } = 1.0f;
    public float ScaleY { get; set; } = 1.0f;
    public float Rotation { get; set; } = 0.0f;
    public bool EnableBlur { get; set; } = false;
    public float BlurRadius { get; set; } = 0.0f;
    public bool EnableShadow { get; set; } = false;
    public RenderColor ShadowColor { get; set; } = new(0, 0, 0, 128);
    public int ShadowOffsetX { get; set; } = 2;
    public int ShadowOffsetY { get; set; } = 2;

    public RenderProperties() { }
}

/// <summary>
/// Main render command structure for IPC communication
/// </summary>
public class RenderCommand
{
    public ulong CommandId { get; set; }
    public RenderCommandType CommandType { get; set; } = RenderCommandType.Initialize;
    public uint WidgetId { get; set; }
    public IntPtr WindowHandle { get; set; }
    public RenderBackendType BackendType { get; set; } = RenderBackendType.Auto;
    public RenderRect Bounds { get; set; }
    public ContentParameters Content { get; set; } = new();
    public RenderProperties Properties { get; set; } = new();
    public ulong Timestamp { get; set; }

    public RenderCommand() { }

    public RenderCommand(RenderCommandType commandType, uint widgetId)
    {
        CommandType = commandType;
        WidgetId = widgetId;
        Timestamp = (ulong)DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
    }

    public override string ToString() => $"Command {CommandId}: {CommandType} for Widget {WidgetId}";
}

/// <summary>
/// Render result structure for IPC responses
/// </summary>
public class RenderResult
{
    public ulong CommandId { get; set; }
    public uint WidgetId { get; set; }
    public RenderResultStatus Status { get; set; } = RenderResultStatus.Pending;
    public string ErrorMessage { get; set; } = string.Empty;
    public ulong RenderTimeMs { get; set; }
    public uint FrameCount { get; set; }
    public float AverageFps { get; set; }
    public ulong MemoryUsageMB { get; set; }
    public ulong Timestamp { get; set; }

    public RenderResult() 
    {
        Timestamp = (ulong)DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
    }

    public RenderResult(ulong commandId, uint widgetId, RenderResultStatus status)
    {
        CommandId = commandId;
        WidgetId = widgetId;
        Status = status;
        Timestamp = (ulong)DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
    }

    public static RenderResult Success(ulong commandId, uint widgetId) =>
        new(commandId, widgetId, RenderResultStatus.Success);

    public static RenderResult Failure(ulong commandId, uint widgetId, string errorMessage) =>
        new(commandId, widgetId, RenderResultStatus.Failure) { ErrorMessage = errorMessage };

    public override string ToString() => $"Result {CommandId}: {Status} for Widget {WidgetId}";
}

/// <summary>
/// Performance metrics for monitoring and optimization
/// </summary>
public class PerformanceMetrics
{
    public float CurrentFps { get; set; }
    public float AverageFps { get; set; }
    public ulong TotalFrames { get; set; }
    public ulong DroppedFrames { get; set; }
    public ulong MemoryUsageMB { get; set; }
    public ulong VramUsageMB { get; set; }
    public float CpuUsagePercent { get; set; }
    public float GpuUsagePercent { get; set; }
    public ulong RenderTimeMs { get; set; }
    public ulong ContentLoadTimeMs { get; set; }

    public PerformanceMetrics() { }

    public double FrameDropPercentage => TotalFrames > 0 ? (double)DroppedFrames / TotalFrames * 100.0 : 0.0;
    
    public override string ToString() => 
        $"FPS: {CurrentFps:F1} (avg: {AverageFps:F1}), Frames: {TotalFrames}, Dropped: {DroppedFrames}, Memory: {MemoryUsageMB}MB";
}

/// <summary>
/// Monitor information for multi-monitor support
/// </summary>
public class MonitorInfo
{
    public int MonitorId { get; set; }
    public IntPtr HMonitor { get; set; }
    public RenderRect Bounds { get; set; }
    public RenderRect WorkArea { get; set; }
    public float DpiX { get; set; } = 96.0f;
    public float DpiY { get; set; } = 96.0f;
    public bool IsPrimary { get; set; }
    public string DeviceName { get; set; } = string.Empty;

    public MonitorInfo() { }

    public float DpiScalingFactor => DpiX / 96.0f;

    public override string ToString() => 
        $"Monitor {MonitorId}: {Bounds.Width}x{Bounds.Height} @ {DpiX} DPI {(IsPrimary ? "(Primary)" : "")}";
}

/// <summary>
/// System capabilities for backend selection
/// </summary>
public class SystemCapabilities
{
    public bool SupportsSkiaSharp { get; set; }
    public bool SupportsDirect3D { get; set; }
    public bool SupportsWebView2 { get; set; }
    public bool SupportsHardwareAcceleration { get; set; }
    public bool SupportsMultiMonitor { get; set; }
    public bool SupportsHighDPI { get; set; }
    public string GpuName { get; set; } = string.Empty;
    public string DriverVersion { get; set; } = string.Empty;
    public ulong TotalVRAM { get; set; }
    public ulong AvailableVRAM { get; set; }

    public SystemCapabilities() { }

    public List<RenderBackendType> GetSupportedBackends()
    {
        var backends = new List<RenderBackendType>();
        
        if (SupportsSkiaSharp) backends.Add(RenderBackendType.SkiaSharp);
        if (SupportsDirect3D) backends.Add(RenderBackendType.Direct3D);
        if (SupportsWebView2) backends.Add(RenderBackendType.WebView);
        
        return backends;
    }

    public override string ToString() =>
        $"Backends: {string.Join(", ", GetSupportedBackends())}, GPU: {GpuName}, VRAM: {AvailableVRAM}/{TotalVRAM}MB";
}

// ===== EVENT ARGUMENT CLASSES =====

/// <summary>
/// Event arguments for render errors
/// </summary>
public class RenderErrorEventArgs : EventArgs
{
    public string ErrorMessage { get; }
    public Exception? Exception { get; }
    public RenderBackendType BackendType { get; }

    public RenderErrorEventArgs(string errorMessage, Exception? exception = null, 
                               RenderBackendType backendType = RenderBackendType.SkiaSharp)
    {
        ErrorMessage = errorMessage;
        Exception = exception;
        BackendType = backendType;
    }
}

/// <summary>
/// Event arguments for frame rendered notifications
/// </summary>
public class FrameRenderedEventArgs : EventArgs
{
    public long FrameNumber { get; }
    public double RenderTimeMs { get; }

    public FrameRenderedEventArgs(long frameNumber, double renderTimeMs)
    {
        FrameNumber = frameNumber;
        RenderTimeMs = renderTimeMs;
    }
}

/// <summary>
/// Event arguments for performance updates
/// </summary>
public class PerformanceUpdateEventArgs : EventArgs
{
    public PerformanceMetrics Metrics { get; }

    public PerformanceUpdateEventArgs(PerformanceMetrics metrics)
    {
        Metrics = metrics;
    }
}
