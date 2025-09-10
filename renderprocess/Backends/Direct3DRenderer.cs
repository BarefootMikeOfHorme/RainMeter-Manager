using Microsoft.Extensions.Logging;
using RenderProcess.Interfaces;
using System;
using System.Threading.Tasks;

namespace RenderProcess.Backends;

/// <summary>
/// Minimal Direct3D renderer stub to satisfy DI and allow future implementation.
/// </summary>
public class Direct3DRenderer : IRenderBackend
{
    private readonly ILogger<Direct3DRenderer> _logger;

    public Direct3DRenderer(ILogger<Direct3DRenderer> logger)
    {
        _logger = logger;
        _logger.LogInformation("Direct3DRenderer initialized (stub)");
    }

    public RenderBackendType BackendType => RenderBackendType.Direct3D;
    public bool IsInitialized { get; private set; }
    public string Name => "Direct3D Renderer (stub)";

    public Task<bool> InitializeAsync(IntPtr windowHandle, int width, int height)
    {
        IsInitialized = true;
        return Task.FromResult(true);
    }

    public Task<bool> RenderFrameAsync() => Task.FromResult(true);
    public Task<bool> ResizeAsync(int width, int height) => Task.FromResult(true);
    public Task CleanupAsync() { IsInitialized = false; return Task.CompletedTask; }

    public Task<bool> LoadContentAsync(string contentSource, ContentParameters parameters) => Task.FromResult(true);
    public Task<bool> UpdateContentAsync(string contentSource) => Task.FromResult(true);
    public Task<bool> ClearContentAsync() => Task.FromResult(true);

    public void SetOpacity(float opacity) { }
    public void SetVisible(bool visible) { }
    public void SetProperties(RenderProperties properties) { }
    public RenderProperties GetProperties() => new RenderProperties();
    public PerformanceMetrics GetPerformanceMetrics() => new PerformanceMetrics();

    public event EventHandler<RenderErrorEventArgs>? RenderError;
    public event EventHandler<FrameRenderedEventArgs>? FrameRendered;

    public void Dispose() { }
}

