using Microsoft.Extensions.Logging;
using RenderProcess.Backends;
using RenderProcess.Interfaces;
using SkiaSharp;
using System;
using System.Threading.Tasks;

namespace RenderProcess.Examples;

/// <summary>
/// Simple clock widget for testing the RenderCore system
/// Based on the existing illustro Clock.ini but rendered through our new system
/// </summary>
public class SimpleClockWidget : IDisposable
{
    private readonly ILogger<SimpleClockWidget> _logger;
    private readonly SkiaSharpRenderer _renderer;
    private readonly RenderProperties _properties;
    private readonly Timer _updateTimer;
    
    // Visual properties (matching illustro theme)
    private readonly SKColor _backgroundColor = SKColor.Parse("#000000");
    private readonly SKColor _textColor = SKColor.Parse("#FFFFFF");
    private readonly SKColor _shadowColor = SKColor.Parse("#00000032");
    private readonly string _fontName = "Trebuchet MS";
    private readonly float _titleFontSize = 24f;
    private readonly float _textFontSize = 16f;
    
    // Paint objects for rendering
    private SKPaint _titlePaint;
    private SKPaint _textPaint;
    private SKPaint _backgroundPaint;
    
    public string WidgetId { get; }
    public bool IsInitialized { get; private set; }

    public SimpleClockWidget(ILogger<SimpleClockWidget> logger)
    {
        _logger = logger ?? throw new ArgumentNullException(nameof(logger));
        WidgetId = $"ClockWidget_{Guid.NewGuid():N[..8]}";
        
        // Initialize render properties
        _properties = new RenderProperties
        {
            BackgroundColor = new RenderColor(0, 0, 0, 180), // Semi-transparent background
            Opacity = 0.95f,
            Visible = true,
            ClickThrough = false,
            TopMost = true,
            EnableAnimations = true,
            TargetFps = 1 // Clock only needs 1 FPS
        };
        
        // Create renderer
        _renderer = new SkiaSharpRenderer(logger);
        
        // Setup update timer (update every second)
        _updateTimer = new Timer(UpdateClock, null, TimeSpan.Zero, TimeSpan.FromSeconds(1));
        
        _logger.LogInformation($"SimpleClockWidget {WidgetId} created");
    }

    public async Task<bool> InitializeAsync(IntPtr windowHandle, int width = 300, int height = 150)
    {
        try
        {
            _logger.LogInformation($"Initializing SimpleClockWidget {WidgetId} ({width}x{height})");

            // Initialize the SkiaSharp renderer
            if (!await _renderer.InitializeAsync(windowHandle, width, height))
            {
                _logger.LogError("Failed to initialize SkiaSharp renderer");
                return false;
            }

            // Create paint objects
            InitializePaints();

            IsInitialized = true;
            _logger.LogInformation($"SimpleClockWidget {WidgetId} initialized successfully");
            
            // Trigger initial render
            await RenderClockAsync();
            
            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Failed to initialize SimpleClockWidget {WidgetId}");
            return false;
        }
    }

    private void InitializePaints()
    {
        // Title paint (time display)
        _titlePaint = new SKPaint
        {
            Color = _textColor,
            IsAntialias = true,
            Typeface = SKTypeface.FromFamilyName(_fontName, SKFontStyle.Bold),
            TextSize = _titleFontSize,
            TextAlign = SKTextAlign.Center
        };

        // Add shadow effect to title
        _titlePaint.MaskFilter = SKMaskFilter.CreateBlur(
            SKBlurStyle.Normal, 
            1.0f // Shadow blur radius
        );

        // Regular text paint (date display)
        _textPaint = new SKPaint
        {
            Color = _textColor,
            IsAntialias = true,
            Typeface = SKTypeface.FromFamilyName(_fontName, SKFontStyle.Normal),
            TextSize = _textFontSize,
            TextAlign = SKTextAlign.Left
        };

        // Background paint
        _backgroundPaint = new SKPaint
        {
            Color = _backgroundColor,
            IsAntialias = true
        };
    }

    private async void UpdateClock(object? state)
    {
        try
        {
            if (IsInitialized)
            {
                await RenderClockAsync();
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error updating clock");
        }
    }

    private async Task RenderClockAsync()
    {
        try
        {
            // This would normally be handled by the render backend, but for this example
            // we'll simulate the content that would be rendered
            
            var now = DateTime.Now;
            var timeString = now.ToString("HH:mm");
            var dateString = now.ToString("dd.MM.yyyy");
            var dayString = now.ToString("dddd");
            
            _logger.LogDebug($"Rendering clock: {timeString} - {dayString}, {dateString}");
            
            // Create content parameters that would be sent to the renderer
            var content = new ContentParameters
            {
                SourceType = ContentSourceType.Static,
                TemplateData = CreateClockHtml(timeString, dateString, dayString)
            };
            
            // In a real implementation, this would be sent through IPC to the C++ core
            // For now, we'll just log what would be rendered
            _logger.LogDebug($"Clock content generated for {WidgetId}:\n{content.TemplateData}");
            
            // Update performance metrics
            var metrics = new PerformanceMetrics
            {
                CurrentFps = 1.0f,
                AverageFps = 1.0f,
                TotalFrames = (ulong)(DateTime.Now - DateTime.Today).TotalSeconds,
                MemoryUsageMB = 15, // Estimated memory usage
                RenderTimeMs = 5    // Clock rendering is very fast
            };
            
            _logger.LogDebug($"Clock widget performance: {metrics}");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error rendering clock");
        }
    }

    private string CreateClockHtml(string time, string date, string day)
    {
        return $@"
<!DOCTYPE html>
<html>
<head>
    <style>
        body {{
            font-family: '{_fontName}', sans-serif;
            background: rgba(0, 0, 0, 0.7);
            color: white;
            margin: 0;
            padding: 20px;
            text-align: center;
            height: 110px;
            display: flex;
            flex-direction: column;
            justify-content: center;
        }}
        .time {{
            font-size: 36px;
            font-weight: bold;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.5);
            margin-bottom: 10px;
        }}
        .date {{
            font-size: 16px;
            opacity: 0.9;
            margin-bottom: 5px;
        }}
        .day {{
            font-size: 14px;
            opacity: 0.8;
            text-transform: uppercase;
        }}
        .separator {{
            height: 1px;
            background: rgba(255,255,255,0.15);
            margin: 8px 0;
        }}
    </style>
</head>
<body>
    <div class=""time"">{time}</div>
    <div class=""separator""></div>
    <div class=""day"">{day}</div>
    <div class=""date"">{date}</div>
</body>
</html>";
    }

    /// <summary>
    /// Create a RenderCommand that would be sent to the C++ core
    /// </summary>
    public RenderCommand CreateRenderCommand(uint widgetId)
    {
        var now = DateTime.Now;
        return new RenderCommand
        {
            CommandId = (ulong)Environment.TickCount,
            CommandType = RenderCommandType.Render,
            WidgetId = widgetId,
            BackendType = RenderBackendType.WebView, // Use WebView for HTML content
            Bounds = new RenderRect(100, 100, 300, 150),
            Content = new ContentParameters
            {
                SourceType = ContentSourceType.Static,
                TemplateData = CreateClockHtml(
                    now.ToString("HH:mm"),
                    now.ToString("dd.MM.yyyy"),
                    now.ToString("dddd")
                )
            },
            Properties = _properties,
            Timestamp = (ulong)DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()
        };
    }

    /// <summary>
    /// Simulate processing a render result from the C++ core
    /// </summary>
    public void ProcessRenderResult(RenderResult result)
    {
        _logger.LogInformation($"Clock widget received render result: {result}");
        
        if (result.Status == RenderResultStatus.Success)
        {
            _logger.LogDebug($"Clock rendered successfully in {result.RenderTimeMs}ms");
        }
        else
        {
            _logger.LogError($"Clock render failed: {result.ErrorMessage}");
        }
    }

    public async Task ResizeAsync(int width, int height)
    {
        try
        {
            _logger.LogInformation($"Resizing clock widget to {width}x{height}");
            
            if (_renderer != null)
            {
                await _renderer.ResizeAsync(width, height);
            }
            
            // Re-render with new size
            await RenderClockAsync();
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error resizing clock widget");
        }
    }

    public async Task CleanupAsync()
    {
        try
        {
            _logger.LogInformation($"Cleaning up SimpleClockWidget {WidgetId}");
            
            _updateTimer?.Dispose();
            
            if (_renderer != null)
            {
                await _renderer.CleanupAsync();
            }
            
            _titlePaint?.Dispose();
            _textPaint?.Dispose();
            _backgroundPaint?.Dispose();
            
            IsInitialized = false;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error cleaning up clock widget");
        }
    }

    public void Dispose()
    {
        CleanupAsync().Wait(5000);
    }
}
