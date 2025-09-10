using Microsoft.Extensions.Logging;
using RenderProcess.Interfaces;
using SkiaSharp;
using SkiaSharp.Views.Desktop;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Net.Http;
using System.Runtime.InteropServices;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace RenderProcess.Backends;

/// <summary>
/// Enterprise-grade SkiaSharp rendering backend with hardware acceleration,
/// advanced effects, and cinematic quality graphics capabilities
/// </summary>
public class SkiaSharpRenderer : IRenderBackend, IDisposable
{
    private readonly ILogger<SkiaSharpRenderer> _logger;
    private readonly HttpClient _httpClient;
    
    // Core rendering components
    private SKControl? _skiaControl;
    private Form? _hostForm;
    private GRContext? _grContext;
    private SKSurface? _surface;
    private SKCanvas? _canvas;
    
    // Window and surface management
    private IntPtr _windowHandle;
    private int _width, _height;
    private bool _isInitialized;
    private bool _isDisposed;
    
    // Rendering properties
    private RenderProperties _properties;
    private ContentParameters _contentParameters;
    private bool _opacityLayerPushed;
    
    // Performance monitoring
    private PerformanceMetrics _metrics;
    private Stopwatch _renderTimer;
    private long _frameCount;
    private readonly List<double> _frameTimes;
    
    // Content management
    private readonly Dictionary<string, SKBitmap> _imageCache;
    private readonly Dictionary<string, SKTypeface> _fontCache;
    private readonly Dictionary<string, object> _contentCache;
    
    // Advanced effects
    private SKPaint? _textPaint;
    private SKPaint? _shapePaint;
    private SKPaint? _imagePaint;
    private SKPaint? _effectsPaint;
    private SKImageFilter? _blurFilter;
    private SKImageFilter? _shadowFilter;
    
    // Animation and timing
    private Timer? _animationTimer;
    private DateTime _lastRenderTime;
    private float _animationProgress;
    
    // Events
    public event EventHandler<RenderErrorEventArgs>? RenderError;
    public event EventHandler<FrameRenderedEventArgs>? FrameRendered;

    public SkiaSharpRenderer(ILogger<SkiaSharpRenderer> logger)
    {
        _logger = logger ?? throw new ArgumentNullException(nameof(logger));
        _httpClient = new HttpClient();
        _renderTimer = new Stopwatch();
        _frameTimes = new List<double>(120); // Track last 120 frames for average FPS
        _imageCache = new Dictionary<string, SKBitmap>();
        _fontCache = new Dictionary<string, SKTypeface>();
        _contentCache = new Dictionary<string, object>();
        
        _properties = new RenderProperties();
        _metrics = new PerformanceMetrics();
        
        _logger.LogInformation("SkiaSharp renderer initialized");
    }

    #region IRenderBackend Implementation

    public RenderBackendType BackendType => RenderBackendType.SkiaSharp;
    public bool IsInitialized => _isInitialized;
    public string Name => "SkiaSharp Hardware-Accelerated Renderer";

    public async Task<bool> InitializeAsync(IntPtr windowHandle, int width, int height)
    {
        try
        {
            _logger.LogInformation($"Initializing SkiaSharp renderer: {width}x{height}");
            
            if (_isInitialized)
            {
                await CleanupAsync();
            }
            
            _windowHandle = windowHandle;
            _width = width;
            _height = height;
            
            // Create host form for SkiaSharp control
            await CreateHostForm();
            
            // Initialize SkiaSharp with hardware acceleration
            InitializeSkiaContext();
            
            // Setup rendering surface
            CreateRenderingSurface();
            
            // Initialize paint objects for various operations
            InitializePaintObjects();
            
            // Setup performance monitoring
            SetupPerformanceMonitoring();
            
            _isInitialized = true;
            _logger.LogInformation("SkiaSharp renderer successfully initialized");
            
            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to initialize SkiaSharp renderer");
            RenderError?.Invoke(this, new RenderErrorEventArgs($"Initialization failed: {ex.Message}", ex, BackendType));
            return false;
        }
    }

    public async Task<bool> RenderFrameAsync()
    {
        if (!_isInitialized || _canvas == null)
        {
            _logger.LogWarning("Cannot render frame - renderer not initialized");
            return false;
        }

        try
        {
            _renderTimer.Restart();
            
            // Clear the canvas
            _canvas.Clear(_properties.BackgroundColor.ToSkiaColor());
            
            // Apply global transformations
            ApplyGlobalTransforms();
            
            // Render content based on current parameters
            await RenderContentAsync();
            
            // Apply post-processing effects
            ApplyPostProcessingEffects();
            
            // Flush graphics operations
            _surface?.Flush();
            _grContext?.Flush();
            
            // Update performance metrics
            _renderTimer.Stop();
            UpdatePerformanceMetrics(_renderTimer.Elapsed.TotalMilliseconds);
            
            // Trigger frame rendered event
            FrameRendered?.Invoke(this, new FrameRenderedEventArgs(_frameCount, _renderTimer.Elapsed.TotalMilliseconds));
            
            _frameCount++;
            _lastRenderTime = DateTime.UtcNow;
            
            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error during frame rendering");
            RenderError?.Invoke(this, new RenderErrorEventArgs($"Render frame failed: {ex.Message}", ex, BackendType));
            return false;
        }
    }

    public async Task<bool> ResizeAsync(int width, int height)
    {
        try
        {
            _logger.LogInformation($"Resizing SkiaSharp renderer: {width}x{height}");
            
            _width = width;
            _height = height;
            
            // Recreate rendering surface with new dimensions
            CreateRenderingSurface();
            
            // Update host form size if needed
            if (_hostForm != null)
            {
                _hostForm.Size = new Size(width, height);
            }
            
            _logger.LogDebug("SkiaSharp renderer resized successfully");
            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to resize SkiaSharp renderer");
            RenderError?.Invoke(this, new RenderErrorEventArgs($"Resize failed: {ex.Message}", ex, BackendType));
            return false;
        }
    }

    public async Task CleanupAsync()
    {
        try
        {
            _logger.LogInformation("Cleaning up SkiaSharp renderer");
            
            // Stop animation timer
            _animationTimer?.Dispose();
            _animationTimer = null;
            
            // Dispose paint objects
            DisposePaintObjects();
            
            // Dispose rendering surface
            _surface?.Dispose();
            _surface = null;
            _canvas = null;
            
            // Dispose graphics context
            _grContext?.Dispose();
            _grContext = null;
            
            // Dispose SkiaSharp control
            _skiaControl?.Dispose();
            _skiaControl = null;
            
            // Dispose host form
            _hostForm?.Dispose();
            _hostForm = null;
            
            // Clear caches
            ClearAllCaches();
            
            _isInitialized = false;
            _logger.LogInformation("SkiaSharp renderer cleanup completed");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error during SkiaSharp renderer cleanup");
        }
    }

    public async Task<bool> LoadContentAsync(string contentSource, ContentParameters parameters)
    {
        try
        {
            _logger.LogInformation($"Loading content: {parameters.SourceType} from {contentSource}");
            
            _contentParameters = parameters;
            
            switch (parameters.SourceType)
            {
                case ContentSourceType.Static:
                    return await LoadStaticContentAsync(contentSource);
                    
                case ContentSourceType.Web:
                    return await LoadWebContentAsync(contentSource);
                    
                case ContentSourceType.API:
                    return await LoadAPIContentAsync(contentSource);
                    
                case ContentSourceType.Media:
                    return await LoadMediaContentAsync(contentSource);
                    
                case ContentSourceType.File:
                    return await LoadFileContentAsync(contentSource);
                    
                default:
                    _logger.LogWarning($"Unsupported content type: {parameters.SourceType}");
                    return false;
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Failed to load content from {contentSource}");
            RenderError?.Invoke(this, new RenderErrorEventArgs($"Content loading failed: {ex.Message}", ex, BackendType));
            return false;
        }
    }

    public async Task<bool> UpdateContentAsync(string contentSource)
    {
        return await LoadContentAsync(contentSource, _contentParameters);
    }

    public async Task<bool> ClearContentAsync()
    {
        try
        {
            ClearAllCaches();
            _contentParameters = new ContentParameters();
            _logger.LogDebug("Content cleared successfully");
            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to clear content");
            return false;
        }
    }

    public void SetOpacity(float opacity)
    {
        _properties.Opacity = Math.Clamp(opacity, 0.0f, 1.0f);
        _logger.LogDebug($"Opacity set to {_properties.Opacity}");
    }

    public void SetVisible(bool visible)
    {
        _properties.Visible = visible;
        if (_hostForm != null)
        {
            _hostForm.Visible = visible;
        }
        _logger.LogDebug($"Visibility set to {visible}");
    }

    public void SetProperties(RenderProperties properties)
    {
        _properties = properties;
        UpdateRenderingProperties();
        _logger.LogDebug("Render properties updated");
    }

    public RenderProperties GetProperties()
    {
        return _properties;
    }

    public PerformanceMetrics GetPerformanceMetrics()
    {
        UpdateCurrentMetrics();
        return _metrics;
    }

    #endregion

    #region Core Rendering Methods

    private async Task CreateHostForm()
    {
        if (Application.OpenForms.Count == 0)
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
        }

        _hostForm = new Form
        {
            FormBorderStyle = FormBorderStyle.None,
            ShowInTaskbar = false,
            TopMost = _properties.TopMost,
            Size = new Size(_width, _height),
            StartPosition = FormStartPosition.Manual,
            BackColor = System.Drawing.Color.Transparent,
            AllowTransparency = true,
            Opacity = _properties.Opacity
        };

        // Set window handle if provided
        if (_windowHandle != IntPtr.Zero)
        {
            SetParent(_hostForm.Handle, _windowHandle);
        }

        _hostForm.Show();
    }

    private void InitializeSkiaContext()
    {
        try
        {
            // Try to create hardware-accelerated context
            var glInterface = GRGlInterface.Create();
            if (glInterface != null)
            {
                _grContext = GRContext.CreateGl(glInterface);
                _logger.LogInformation("Hardware-accelerated SkiaSharp context created");
            }
            else
            {
                _logger.LogWarning("Hardware acceleration not available, using software rendering");
            }
        }
        catch (Exception ex)
        {
            _logger.LogWarning(ex, "Failed to create hardware-accelerated context, falling back to software");
        }
    }

    private void CreateRenderingSurface()
    {
        try
        {
            // Dispose previous surface
            _surface?.Dispose();
            
            if (_grContext != null)
            {
                // Create hardware-accelerated surface
                var renderTarget = new GRBackendRenderTarget(_width, _height, 4, 8, new GRGlFramebufferInfo(0, 0x8058));
                _surface = SKSurface.Create(_grContext, renderTarget, GRSurfaceOrigin.BottomLeft, SKColorType.Rgba8888);
            }
            else
            {
                // Create software surface
                var imageInfo = new SKImageInfo(_width, _height, SKColorType.Rgba8888, SKAlphaType.Premul);
                _surface = SKSurface.Create(imageInfo);
            }
            
            if (_surface == null)
            {
                throw new InvalidOperationException("Failed to create SkiaSharp surface");
            }
            
            _canvas = _surface.Canvas;
            _logger.LogDebug($"SkiaSharp surface created: {_width}x{_height}");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to create rendering surface");
            throw;
        }
    }

    private void InitializePaintObjects()
    {
        // Text rendering paint
        _textPaint = new SKPaint
        {
            IsAntialias = true,
            FilterQuality = SKFilterQuality.High,
            HintingLevel = SKPaintHinting.Full,
            SubpixelText = true,
            Style = SKPaintStyle.Fill,
            Typeface = SKTypeface.FromFamilyName("Segoe UI"),
            TextSize = 14
        };

        // Shape rendering paint
        _shapePaint = new SKPaint
        {
            IsAntialias = true,
            FilterQuality = SKFilterQuality.High,
            Style = SKPaintStyle.Fill
        };

        // Image rendering paint
        _imagePaint = new SKPaint
        {
            IsAntialias = true,
            FilterQuality = SKFilterQuality.High,
            IsDither = true
        };

        // Effects paint
        _effectsPaint = new SKPaint
        {
            IsAntialias = true,
            FilterQuality = SKFilterQuality.High
        };

        _logger.LogDebug("Paint objects initialized");
    }

    private void DisposePaintObjects()
    {
        _textPaint?.Dispose();
        _shapePaint?.Dispose();
        _imagePaint?.Dispose();
        _effectsPaint?.Dispose();
        _blurFilter?.Dispose();
        _shadowFilter?.Dispose();

        _textPaint = null;
        _shapePaint = null;
        _imagePaint = null;
        _effectsPaint = null;
        _blurFilter = null;
        _shadowFilter = null;
    }

    private void ApplyGlobalTransforms()
    {
        if (_canvas == null) return;

        _canvas.Save();

        // Apply scaling
        if (_properties.ScaleX != 1.0f || _properties.ScaleY != 1.0f)
        {
            _canvas.Scale(_properties.ScaleX, _properties.ScaleY);
        }

        // Apply rotation
        if (_properties.Rotation != 0.0f)
        {
            _canvas.RotateDegrees(_properties.Rotation, _width / 2.0f, _height / 2.0f);
        }

        // Apply opacity
        if (_properties.Opacity < 1.0f)
        {
            var alpha = (byte)(_properties.Opacity * 255);
            var paint = new SKPaint { Color = SKColors.White.WithAlpha(alpha) };
            _canvas.SaveLayer(paint);
            paint.Dispose();
            _opacityLayerPushed = true;
        }
    }

    private async Task RenderContentAsync()
    {
        if (_canvas == null) return;

        switch (_contentParameters.SourceType)
        {
            case ContentSourceType.Static:
                RenderStaticContent();
                break;
                
            case ContentSourceType.Web:
                await RenderWebContent();
                break;
                
            case ContentSourceType.API:
                await RenderAPIContent();
                break;
                
            case ContentSourceType.Media:
                await RenderMediaContent();
                break;
                
            case ContentSourceType.File:
                await RenderFileContent();
                break;
                
            default:
                RenderPlaceholderContent();
                break;
        }
    }

    private void ApplyPostProcessingEffects()
    {
        if (_canvas == null || _effectsPaint == null) return;

        // Apply blur effect
        if (_properties.EnableBlur && _properties.BlurRadius > 0)
        {
            ApplyBlurEffect();
        }

        // Apply shadow effect
        if (_properties.EnableShadow)
        {
            ApplyShadowEffect();
        }

        // Restore opacity layer if pushed
        if (_opacityLayerPushed)
        {
            _canvas.Restore();
            _opacityLayerPushed = false;
        }
        _canvas.Restore();
    }

    #endregion

    #region Content Rendering Methods

    private async Task<bool> LoadStaticContentAsync(string content)
    {
        try
        {
            _contentCache["static_text"] = content;
            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to load static content");
            return false;
        }
    }

    private async Task<bool> LoadWebContentAsync(string url)
    {
        try
        {
            var response = await _httpClient.GetStringAsync(url);
            _contentCache["web_content"] = response;
            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Failed to load web content from {url}");
            return false;
        }
    }

    private async Task<bool> LoadAPIContentAsync(string endpoint)
    {
        try
        {
            var response = await _httpClient.GetStringAsync(endpoint);
            _contentCache["api_data"] = response;
            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Failed to load API content from {endpoint}");
            return false;
        }
    }

    private async Task<bool> LoadMediaContentAsync(string mediaPath)
    {
        try
        {
            if (File.Exists(mediaPath))
            {
                var bitmap = SKBitmap.Decode(mediaPath);
                if (bitmap != null)
                {
                    _imageCache["media_content"] = bitmap;
                    return true;
                }
            }
            
            // Try loading from URL
            var imageBytes = await _httpClient.GetByteArrayAsync(mediaPath);
            var urlBitmap = SKBitmap.Decode(imageBytes);
            if (urlBitmap != null)
            {
                _imageCache["media_content"] = urlBitmap;
                return true;
            }
            
            return false;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Failed to load media content from {mediaPath}");
            return false;
        }
    }

    private async Task<bool> LoadFileContentAsync(string filePath)
    {
        try
        {
            if (!File.Exists(filePath))
                return false;

            var extension = Path.GetExtension(filePath).ToLowerInvariant();
            switch (extension)
            {
                case ".txt":
                case ".json":
                case ".xml":
                    _contentCache["file_content"] = await File.ReadAllTextAsync(filePath);
                    break;
                    
                case ".png":
                case ".jpg":
                case ".jpeg":
                case ".bmp":
                case ".gif":
                    var bitmap = SKBitmap.Decode(filePath);
                    if (bitmap != null)
                    {
                        _imageCache["file_image"] = bitmap;
                    }
                    break;
                    
                default:
                    _logger.LogWarning($"Unsupported file type: {extension}");
                    return false;
            }
            
            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Failed to load file content from {filePath}");
            return false;
        }
    }

    private void RenderStaticContent()
    {
        if (_canvas == null || _textPaint == null) return;
        
        if (_contentCache.TryGetValue("static_text", out var content) && content is string text)
        {
            _textPaint.Color = SKColors.White;
            _canvas.DrawText(text, 20, 50, _textPaint);
        }
    }

    private async Task RenderWebContent()
    {
        if (_canvas == null || _textPaint == null) return;
        
        if (_contentCache.TryGetValue("web_content", out var content) && content is string html)
        {
            // For SkiaSharp, we render a simplified representation
            // Full HTML rendering would require WebView2 backend
            _textPaint.Color = SKColors.LightBlue;
            _canvas.DrawText("Web Content Loaded", 20, 50, _textPaint);
            
            // Could parse and render simple HTML elements here
            RenderSimpleHTMLContent(html);
        }
    }

    private async Task RenderAPIContent()
    {
        if (_canvas == null || _textPaint == null) return;
        
        if (_contentCache.TryGetValue("api_data", out var content) && content is string jsonData)
        {
            _textPaint.Color = SKColors.LightGreen;
            _canvas.DrawText("API Data:", 20, 50, _textPaint);
            
            // Parse and render JSON data
            RenderJSONContent(jsonData);
        }
    }

    private async Task RenderMediaContent()
    {
        if (_canvas == null || _imagePaint == null) return;
        
        if (_imageCache.TryGetValue("media_content", out var bitmap))
        {
            // Calculate scaling to fit within render area
            var scaleX = (float)_width / bitmap.Width;
            var scaleY = (float)_height / bitmap.Height;
            var scale = Math.Min(scaleX, scaleY);
            
            var destRect = SKRect.Create(
                (_width - bitmap.Width * scale) / 2,
                (_height - bitmap.Height * scale) / 2,
                bitmap.Width * scale,
                bitmap.Height * scale);
            
            _canvas.DrawBitmap(bitmap, destRect, _imagePaint);
        }
    }

    private async Task RenderFileContent()
    {
        if (_canvas == null) return;
        
        // Render image files
        if (_imageCache.TryGetValue("file_image", out var bitmap))
        {
            _canvas.DrawBitmap(bitmap, SKRect.Create(0, 0, _width, _height), _imagePaint);
        }
        
        // Render text files
        if (_contentCache.TryGetValue("file_content", out var content) && content is string text && _textPaint != null)
        {
            _textPaint.Color = SKColors.White;
            var lines = text.Split('\n');
            for (int i = 0; i < Math.Min(lines.Length, 20); i++) // Show first 20 lines
            {
                _canvas.DrawText(lines[i], 20, 30 + i * 20, _textPaint);
            }
        }
    }

    private void RenderPlaceholderContent()
    {
        if (_canvas == null || _textPaint == null || _shapePaint == null) return;
        
        // Draw placeholder background
        _shapePaint.Color = SKColors.DarkGray;
        _canvas.DrawRect(0, 0, _width, _height, _shapePaint);
        
        // Draw placeholder text
        _textPaint.Color = SKColors.White;
        _textPaint.TextSize = 24;
        var placeholderText = "RainmeterManager";
        var textBounds = new SKRect();
        _textPaint.MeasureText(placeholderText, ref textBounds);
        
        var x = (_width - textBounds.Width) / 2;
        var y = (_height - textBounds.Height) / 2;
        
        _canvas.DrawText(placeholderText, x, y, _textPaint);
    }

    #endregion

    #region Helper Methods

    private void RenderSimpleHTMLContent(string html)
    {
        // Simplified HTML rendering - extract text content
        var text = System.Text.RegularExpressions.Regex.Replace(html, "<.*?>", string.Empty);
        if (_canvas != null && _textPaint != null)
        {
            _textPaint.TextSize = 12;
            var lines = text.Split('\n');
            for (int i = 0; i < Math.Min(lines.Length, 10); i++)
            {
                _canvas.DrawText(lines[i], 20, 80 + i * 16, _textPaint);
            }
        }
    }

    private void RenderJSONContent(string json)
    {
        try
        {
            // Simple JSON visualization
            if (_canvas != null && _textPaint != null)
            {
                _textPaint.TextSize = 10;
                var lines = json.Split('\n');
                for (int i = 0; i < Math.Min(lines.Length, 15); i++)
                {
                    _canvas.DrawText(lines[i], 20, 80 + i * 14, _textPaint);
                }
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to render JSON content");
        }
    }

    private void ApplyBlurEffect()
    {
        if (_blurFilter == null)
        {
            _blurFilter = SKImageFilter.CreateBlur(_properties.BlurRadius, _properties.BlurRadius);
        }
        _effectsPaint!.ImageFilter = _blurFilter;
    }

    private void ApplyShadowEffect()
    {
        if (_shadowFilter == null)
        {
            _shadowFilter = SKImageFilter.CreateDropShadow(
                _properties.ShadowOffsetX,
                _properties.ShadowOffsetY,
                5, 5,
                _properties.ShadowColor.ToSkiaColor());
        }
        _effectsPaint!.ImageFilter = _shadowFilter;
    }

    private void UpdateRenderingProperties()
    {
        if (_hostForm != null)
        {
            _hostForm.TopMost = _properties.TopMost;
            _hostForm.Opacity = _properties.Opacity;
            _hostForm.Visible = _properties.Visible;
        }
    }

    private void SetupPerformanceMonitoring()
    {
        _metrics = new PerformanceMetrics
        {
            CurrentFps = 0,
            AverageFps = 0,
            TotalFrames = 0,
            DroppedFrames = 0,
            MemoryUsageMB = 0,
            VramUsageMB = 0,
            CpuUsagePercent = 0,
            GpuUsagePercent = 0
        };
        
        _frameCount = 0;
        _frameTimes.Clear();
    }

    private void UpdatePerformanceMetrics(double renderTimeMs)
    {
        _frameTimes.Add(renderTimeMs);
        if (_frameTimes.Count > 120)
        {
            _frameTimes.RemoveAt(0);
        }
        
        _metrics.TotalFrames = (ulong)_frameCount;
        _metrics.RenderTimeMs = (ulong)Math.Max(0, (long)Math.Round(renderTimeMs));
        
        if (_frameTimes.Count > 0)
        {
            var avgRenderTime = _frameTimes.Sum() / _frameTimes.Count;
            _metrics.CurrentFps = (float)(1000.0 / avgRenderTime);
            _metrics.AverageFps = _metrics.CurrentFps;
        }
        
        // Update memory usage
        var process = Process.GetCurrentProcess();
        _metrics.MemoryUsageMB = (ulong)(process.WorkingSet64 / 1024 / 1024);
    }

    private void UpdateCurrentMetrics()
    {
        var process = Process.GetCurrentProcess();
        _metrics.MemoryUsageMB = (ulong)(process.WorkingSet64 / 1024 / 1024);
        _metrics.CpuUsagePercent = (float)process.TotalProcessorTime.TotalMilliseconds;
    }

    private void ClearAllCaches()
    {
        foreach (var bitmap in _imageCache.Values)
        {
            bitmap.Dispose();
        }
        _imageCache.Clear();
        
        foreach (var typeface in _fontCache.Values)
        {
            typeface.Dispose();
        }
        _fontCache.Clear();
        
        _contentCache.Clear();
    }

    #endregion

    #region P/Invoke

    [DllImport("user32.dll")]
    private static extern IntPtr SetParent(IntPtr hWndChild, IntPtr hWndNewParent);

    #endregion

    #region IDisposable

    public void Dispose()
    {
        if (!_isDisposed)
        {
            CleanupAsync().Wait();
            _httpClient?.Dispose();
            _renderTimer?.Stop();
            _isDisposed = true;
        }
    }

    #endregion
}

// Extension methods for color conversion
public static class ColorExtensions
{
    public static SKColor ToSkiaColor(this System.Drawing.Color color)
    {
        return new SKColor(color.R, color.G, color.B, color.A);
    }

    public static SKColor ToSkiaColor(this RenderProcess.Interfaces.RenderColor color)
    {
        return new SKColor(color.R, color.G, color.B, color.A);
    }
}
