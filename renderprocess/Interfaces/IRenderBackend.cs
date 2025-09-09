using System;
using System.Threading.Tasks;

namespace RenderProcess.Interfaces;

public enum RenderBackendType
{
    SkiaSharp = 0,
    Direct3D = 1,
    WebView = 2,
    Auto = 99
}

public enum ContentSourceType
{
    Static = 0,      // Static content (text, images)
    Web = 1,         // Web URLs, HTML content  
    API = 2,         // REST/GraphQL APIs
    Media = 3,       // Video/audio streams
    File = 4,        // Local files
    Office = 5,      // Excel, Word, PowerPoint
    Custom = 99      // Custom content source
}

public interface IRenderBackend : IDisposable
{
    RenderBackendType BackendType { get; }
    bool IsInitialized { get; }
    string Name { get; }
    
    // === Lifecycle Management ===
    Task<bool> InitializeAsync(IntPtr windowHandle, int width, int height);
    Task<bool> RenderFrameAsync();
    Task<bool> ResizeAsync(int width, int height);
    Task CleanupAsync();
    
    // === Content Management ===
    Task<bool> LoadContentAsync(string contentSource, ContentParameters parameters);
    Task<bool> UpdateContentAsync(string contentSource);
    Task<bool> ClearContentAsync();
    
    // === Properties ===
    void SetOpacity(float opacity);
    void SetVisible(bool visible);
    void SetProperties(RenderProperties properties);
    RenderProperties GetProperties();
    
    // === Performance ===
    PerformanceMetrics GetPerformanceMetrics();
    
    // === Events ===
    event EventHandler<RenderErrorEventArgs>? RenderError;
    event EventHandler<FrameRenderedEventArgs>? FrameRendered;
}

public struct ContentParameters
{
    public ContentSourceType SourceType { get; set; }
    public string SourceUrl { get; set; }
    public string TemplatePath { get; set; }
    public string AuthToken { get; set; }
    public int RefreshIntervalMs { get; set; }
    public bool CacheEnabled { get; set; }
    public Dictionary<string, string> CustomHeaders { get; set; }
    public Dictionary<string, string> Parameters { get; set; }
    
    public ContentParameters()
    {
        SourceType = ContentSourceType.Static;
        SourceUrl = string.Empty;
        TemplatePath = string.Empty;
        AuthToken = string.Empty;
        RefreshIntervalMs = 0;
        CacheEnabled = true;
        CustomHeaders = new Dictionary<string, string>();
        Parameters = new Dictionary<string, string>();
    }
}

public struct RenderProperties
{
    public float Opacity { get; set; }
    public bool Visible { get; set; }
    public bool ClickThrough { get; set; }
    public bool TopMost { get; set; }
    public int TargetFps { get; set; }
    public bool EnableVSync { get; set; }
    
    // Advanced properties
    public float ScaleX { get; set; }
    public float ScaleY { get; set; }
    public float Rotation { get; set; }
    public bool EnableBlur { get; set; }
    public float BlurRadius { get; set; }
    public bool EnableShadow { get; set; }
    public Color ShadowColor { get; set; }
    public int ShadowOffsetX { get; set; }
    public int ShadowOffsetY { get; set; }
    
    public RenderProperties()
    {
        Opacity = 1.0f;
        Visible = true;
        ClickThrough = false;
        TopMost = false;
        TargetFps = 60;
        EnableVSync = true;
        ScaleX = 1.0f;
        ScaleY = 1.0f;
        Rotation = 0.0f;
        EnableBlur = false;
        BlurRadius = 0.0f;
        EnableShadow = false;
        ShadowColor = Color.FromArgb(128, 0, 0, 0);
        ShadowOffsetX = 2;
        ShadowOffsetY = 2;
    }
}

public struct PerformanceMetrics
{
    public float CurrentFps { get; set; }
    public float AverageFps { get; set; }
    public long TotalFrames { get; set; }
    public long DroppedFrames { get; set; }
    public long MemoryUsageMB { get; set; }
    public long VramUsageMB { get; set; }
    public float CpuUsagePercent { get; set; }
    public float GpuUsagePercent { get; set; }
    public long RenderTimeMs { get; set; }
    public long ContentLoadTimeMs { get; set; }
    
    public PerformanceMetrics()
    {
        CurrentFps = 0.0f;
        AverageFps = 0.0f;
        TotalFrames = 0;
        DroppedFrames = 0;
        MemoryUsageMB = 0;
        VramUsageMB = 0;
        CpuUsagePercent = 0.0f;
        GpuUsagePercent = 0.0f;
        RenderTimeMs = 0;
        ContentLoadTimeMs = 0;
    }
}

public class RenderErrorEventArgs : EventArgs
{
    public string ErrorMessage { get; }
    public Exception? Exception { get; }
    public RenderBackendType BackendType { get; }
    
    public RenderErrorEventArgs(string errorMessage, Exception? exception, RenderBackendType backendType)
    {
        ErrorMessage = errorMessage;
        Exception = exception;
        BackendType = backendType;
    }
}

public class FrameRenderedEventArgs : EventArgs
{
    public long FrameNumber { get; }
    public double RenderTimeMs { get; }
    public DateTime Timestamp { get; }
    
    public FrameRenderedEventArgs(long frameNumber, double renderTimeMs)
    {
        FrameNumber = frameNumber;
        RenderTimeMs = renderTimeMs;
        Timestamp = DateTime.UtcNow;
    }
}
