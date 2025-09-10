using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using RenderProcess.Backends;
using RenderProcess.Communication;
using RenderProcess.Interfaces;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace RenderProcess.Runtime;

/// <summary>
/// Enterprise-grade RenderManager orchestrating multiple rendering backends
/// with cinematic quality, performance monitoring, and advanced management capabilities
/// </summary>
public class RenderManager : IDisposable
{
    private readonly ILogger<RenderManager> _logger;
    private readonly IServiceProvider _serviceProvider;
    private readonly IPCMessageHandler _ipcHandler;
    private readonly PerformanceMonitor _performanceMonitor;
    private readonly ProcessService _processService;
    
    // Rendering backend management
private readonly ConcurrentDictionary<uint, IRenderBackend> _activeBackends;
    private readonly Dictionary<RenderBackendType, Func<IRenderBackend>> _backendFactories;
    private readonly object _backendLock = new();
    
    // System state
    private bool _isRunning;
    private bool _isDisposed;
    private CancellationTokenSource? _cancellationTokenSource;
    
    // Background services
    private Task? _renderLoopTask;
    private Task? _performanceMonitoringTask;
    private Task? _maintenanceTask;
    private Task? _ipcListenerTask;
    
    // Performance and statistics
    private readonly RenderStatistics _statistics;
    private DateTime _startTime;
    private readonly System.Threading.Timer _metricsTimer;
    
    // Configuration and capabilities
    private readonly RenderConfiguration _configuration;
    private readonly SystemCapabilities _systemCapabilities;
    
    // Event handling
    public event EventHandler<RenderErrorEventArgs>? RenderError;
    public event EventHandler<PerformanceUpdateEventArgs>? PerformanceUpdate;

    public RenderManager(
        ILogger<RenderManager> logger,
        IServiceProvider serviceProvider,
        IPCMessageHandler ipcHandler,
        PerformanceMonitor performanceMonitor,
        ProcessService processService)
    {
        _logger = logger ?? throw new ArgumentNullException(nameof(logger));
        _serviceProvider = serviceProvider ?? throw new ArgumentNullException(nameof(serviceProvider));
        _ipcHandler = ipcHandler ?? throw new ArgumentNullException(nameof(ipcHandler));
        _performanceMonitor = performanceMonitor ?? throw new ArgumentNullException(nameof(performanceMonitor));
        _processService = processService ?? throw new ArgumentNullException(nameof(processService));
        
_activeBackends = new ConcurrentDictionary<uint, IRenderBackend>();
        _backendFactories = new Dictionary<RenderBackendType, Func<IRenderBackend>>();
        _statistics = new RenderStatistics();
        _configuration = LoadConfiguration();
        _systemCapabilities = DetectSystemCapabilities();
        
        // Setup backend factories with dependency injection
        RegisterBackendFactories();
        
        // Initialize performance metrics timer
        _metricsTimer = new System.Threading.Timer(UpdateMetrics, null, TimeSpan.FromSeconds(1), TimeSpan.FromSeconds(1));
        
        _logger.LogInformation("RenderManager initialized with enterprise capabilities");

        // Subscribe to IPC commands
        _ipcHandler.CommandReceived += async (s, e) => await OnIpcCommandReceived(e.Command);
    }

    #region Public Interface

    /// <summary>
    /// Start the rendering system with all subsystems
    /// </summary>
    public async Task<bool> StartAsync(CancellationToken cancellationToken = default)
    {
        try
        {
            if (_isRunning)
            {
                _logger.LogWarning("RenderManager is already running");
                return true;
            }
            
            _logger.LogInformation("Starting RenderManager with cinematic rendering capabilities");
            _startTime = DateTime.UtcNow;
            
            _cancellationTokenSource = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
            
            // Initialize IPC communication
            await InitializeIPC();
            
            // Start background services
            await StartBackgroundServices();
            
            // Initialize system capabilities detection
            await InitializeSystemCapabilities();
            
            _isRunning = true;
            _statistics.StartTime = _startTime;
            
            _logger.LogInformation($"RenderManager started successfully with {_systemCapabilities.SupportedBackends?.Length ?? 0} available backends");
            
            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to start RenderManager");
            return false;
        }
    }

    /// <summary>
    /// Stop the rendering system gracefully
    /// </summary>
    public async Task StopAsync(CancellationToken cancellationToken = default)
    {
        try
        {
            if (!_isRunning)
            {
                return;
            }
            
            _logger.LogInformation("Stopping RenderManager gracefully");
            
            // Signal cancellation to all background tasks
            _cancellationTokenSource?.Cancel();
            
            // Stop all active backends
            await StopAllBackends();
            
            // Wait for background services to complete
            await StopBackgroundServices();
            
            // Cleanup IPC
            await CleanupIPC();
            
            _isRunning = false;
            
            // Log final statistics
            LogFinalStatistics();
            
            _logger.LogInformation("RenderManager stopped successfully");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error during RenderManager shutdown");
        }
    }

    /// <summary>
    /// Create a new rendering backend for a widget
    /// </summary>
public async Task<uint> CreateRenderBackendAsync(
        RenderBackendType backendType,
        IntPtr windowHandle,
        int width,
        int height)
    {
        try
        {
            _logger.LogInformation($"Creating render backend: {backendType} ({width}x{height})");
            
            // Auto-select optimal backend if requested
            if (backendType == RenderBackendType.Auto)
            {
                backendType = SelectOptimalBackend(width, height);
                _logger.LogInformation($"Auto-selected backend: {backendType}");
            }
            
            // Validate backend support
            if (!IsBackendSupported(backendType))
            {
                _logger.LogError($"Backend not supported: {backendType}");
                return 0;
            }
            
            // Create backend instance
            var backend = CreateBackendInstance(backendType);
            if (backend == null)
            {
                _logger.LogError($"Failed to create backend instance: {backendType}");
                return 0;
            }
            
            // Initialize the backend
            var success = await backend.InitializeAsync(windowHandle, width, height);
            if (!success)
            {
                _logger.LogError($"Failed to initialize backend: {backendType}");
                backend.Dispose();
                return 0;
            }
            
            // Generate unique widget ID and register backend
            var widgetId = GenerateWidgetId();
            _activeBackends[widgetId] = backend;
            
            // Setup event handlers
            backend.RenderError += OnBackendRenderError;
            backend.FrameRendered += OnBackendFrameRendered;
            
            _statistics.TotalBackendsCreated++;
            _statistics.ActiveBackends = _activeBackends.Count;
            
            _logger.LogInformation($"Render backend created successfully: Widget ID {widgetId}, Backend {backendType}");
            
            return widgetId;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Failed to create render backend: {backendType}");
            return 0;
        }
    }

    /// <summary>
    /// Destroy a rendering backend
    /// </summary>
public async Task<bool> DestroyRenderBackendAsync(uint widgetId)
    {
        try
        {
            if (!_activeBackends.TryRemove(widgetId, out var backend))
            {
                _logger.LogWarning($"Backend not found for widget ID: {widgetId}");
                return false;
            }
            
            _logger.LogInformation($"Destroying render backend for widget ID: {widgetId}");
            
            // Cleanup backend
            backend.RenderError -= OnBackendRenderError;
            backend.FrameRendered -= OnBackendFrameRendered;
            
            await backend.CleanupAsync();
            backend.Dispose();
            
            _statistics.TotalBackendsDestroyed++;
            _statistics.ActiveBackends = _activeBackends.Count;
            
            _logger.LogDebug($"Render backend destroyed successfully for widget ID: {widgetId}");
            
            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Failed to destroy render backend for widget ID: {widgetId}");
            return false;
        }
    }

    /// <summary>
    /// Render frame for specific widget
    /// </summary>
public async Task<bool> RenderFrameAsync(uint widgetId)
    {
        try
        {
            if (!_activeBackends.TryGetValue(widgetId, out var backend))
            {
                return false;
            }
            
            var result = await backend.RenderFrameAsync();
            
            if (result)
            {
                _statistics.TotalFramesRendered++;
            }
            else
            {
                _statistics.FailedFrames++;
            }
            
            return result;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Failed to render frame for widget ID: {widgetId}");
            _statistics.FailedFrames++;
            return false;
        }
    }

    /// <summary>
    /// Load content for specific widget
    /// </summary>
public async Task<bool> LoadContentAsync(
        uint widgetId,
        string contentSource,
        ContentParameters parameters)
    {
        try
        {
            if (!_activeBackends.TryGetValue(widgetId, out var backend))
            {
                _logger.LogWarning($"Backend not found for widget ID: {widgetId}");
                return false;
            }
            
            _logger.LogInformation($"Loading content for widget ID {widgetId}: {parameters.SourceType} from {contentSource}");
            
            var result = await backend.LoadContentAsync(contentSource, parameters);
            
            if (result)
            {
                _statistics.ContentLoadsSuccessful++;
            }
            else
            {
                _statistics.ContentLoadsFailed++;
            }
            
            return result;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Failed to load content for widget ID: {widgetId}");
            _statistics.ContentLoadsFailed++;
            return false;
        }
    }

    /// <summary>
    /// Get comprehensive system performance metrics
    /// </summary>
    public PerformanceMetrics GetSystemPerformanceMetrics()
    {
        var systemMetrics = new PerformanceMetrics();
        
        // Aggregate metrics from all active backends
        var backendMetrics = _activeBackends.Values
            .Select(b => b.GetPerformanceMetrics())
            .ToList();
        
        if (backendMetrics.Any())
        {
            systemMetrics.CurrentFps = backendMetrics.Average(m => m.CurrentFps);
            systemMetrics.AverageFps = backendMetrics.Average(m => m.AverageFps);
            systemMetrics.TotalFrames = (ulong)backendMetrics.Sum(m => (long)m.TotalFrames);
            systemMetrics.DroppedFrames = (ulong)backendMetrics.Sum(m => (long)m.DroppedFrames);
            systemMetrics.MemoryUsageMB = (ulong)backendMetrics.Sum(m => (long)m.MemoryUsageMB);
            systemMetrics.VramUsageMB = (ulong)backendMetrics.Sum(m => (long)m.VramUsageMB);
            systemMetrics.CpuUsagePercent = backendMetrics.Average(m => m.CpuUsagePercent);
            systemMetrics.GpuUsagePercent = backendMetrics.Average(m => m.GpuUsagePercent);
        }
        
        // Add system-level metrics
        var process = Process.GetCurrentProcess();
        systemMetrics.MemoryUsageMB = (ulong)Math.Max((long)systemMetrics.MemoryUsageMB, (long)(process.WorkingSet64 / 1024 / 1024));
        
        return systemMetrics;
    }

    /// <summary>
    /// Get detailed system diagnostics
    /// </summary>
    public string GetSystemDiagnostics()
    {
        var diagnostics = new
        {
            SystemInfo = new
            {
                StartTime = _startTime,
                Uptime = DateTime.UtcNow - _startTime,
                IsRunning = _isRunning,
                ActiveBackends = _activeBackends.Count,
                SupportedBackends = _systemCapabilities.SupportedBackends?.ToList()
            },
            Performance = GetSystemPerformanceMetrics(),
            Statistics = _statistics,
            Configuration = _configuration,
            Capabilities = _systemCapabilities,
            BackendDetails = _activeBackends.ToDictionary(
                kvp => kvp.Key,
                kvp => new
                {
                    Type = kvp.Value.BackendType,
                    Name = kvp.Value.Name,
                    IsInitialized = kvp.Value.IsInitialized,
                    Properties = kvp.Value.GetProperties(),
                    Metrics = kvp.Value.GetPerformanceMetrics()
                })
        };
        
        return System.Text.Json.JsonSerializer.Serialize(diagnostics, new System.Text.Json.JsonSerializerOptions
        {
            WriteIndented = true,
            PropertyNamingPolicy = System.Text.Json.JsonNamingPolicy.CamelCase
        });
    }

    #endregion

    private async Task OnIpcCommandReceived(RenderProcess.Interfaces.RenderCommand command)
    {
        try
        {
            switch (command.CommandType)
            {
                case RenderProcess.Interfaces.RenderCommandType.GetSystemSnapshot:
                {
                    var metrics = GetSystemPerformanceMetrics();
                    var json = System.Text.Json.JsonSerializer.Serialize(metrics, new System.Text.Json.JsonSerializerOptions { WriteIndented = false });
                    var result = new RenderProcess.Interfaces.RenderResult
                    {
                        CommandId = command.CommandId,
                        WidgetId = command.WidgetId,
                        Status = RenderProcess.Interfaces.RenderResultStatus.Success,
                        ErrorMessage = json, // temporary payload channel
                        Timestamp = (ulong)DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()
                    };
                    await _ipcHandler.SendResultAsync(result);
                    break;
                }
                case RenderProcess.Interfaces.RenderCommandType.GetProcessSnapshot:
                {
                    var list = _processService.GetSnapshot();
                    var json = System.Text.Json.JsonSerializer.Serialize(list, new System.Text.Json.JsonSerializerOptions { WriteIndented = false });
                    var result = new RenderProcess.Interfaces.RenderResult
                    {
                        CommandId = command.CommandId,
                        WidgetId = command.WidgetId,
                        Status = RenderProcess.Interfaces.RenderResultStatus.Success,
                        ErrorMessage = json, // temporary payload channel
                        Timestamp = (ulong)DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()
                    };
                    await _ipcHandler.SendResultAsync(result);
                    break;
                }
                default:
                    // ignore other commands here; handled elsewhere
                    break;
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "OnIpcCommandReceived failed");
            var result = new RenderProcess.Interfaces.RenderResult
            {
                CommandId = command.CommandId,
                WidgetId = command.WidgetId,
                Status = RenderProcess.Interfaces.RenderResultStatus.Failure,
                ErrorMessage = ex.Message,
                Timestamp = (ulong)DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()
            };
            await _ipcHandler.SendResultAsync(result);
        }
    }

    #region Private Implementation

    private void RegisterBackendFactories()
    {
        _backendFactories[RenderBackendType.SkiaSharp] = () => _serviceProvider.GetRequiredService<SkiaSharpRenderer>();
        _backendFactories[RenderBackendType.WebView] = () => _serviceProvider.GetRequiredService<WebViewRenderer>();
        
        // Note: Direct3DRenderer would be registered here when implemented
        // _backendFactories[RenderBackendType.Direct3D] = () => _serviceProvider.GetRequiredService<Direct3DRenderer>();
    }

    private async Task InitializeIPC()
    {
        try
        {
            _logger.LogInformation("Initializing IPC communication");
            await _ipcHandler.InitializeAsync();
            _logger.LogDebug("IPC communication initialized successfully");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to initialize IPC communication");
            throw;
        }
    }

    private async Task StartBackgroundServices()
    {
        var cancellationToken = _cancellationTokenSource!.Token;
        
        // Start main render loop
        _renderLoopTask = Task.Run(async () => await RenderLoopAsync(cancellationToken), cancellationToken);
        
        // Start performance monitoring
        _performanceMonitoringTask = Task.Run(async () => await PerformanceMonitoringLoopAsync(cancellationToken), cancellationToken);
        
        // Start maintenance tasks
        _maintenanceTask = Task.Run(async () => await MaintenanceLoopAsync(cancellationToken), cancellationToken);
        
        // Start IPC listener
        _ipcListenerTask = Task.Run(async () => await IPCListenerLoopAsync(cancellationToken), cancellationToken);
        
        _logger.LogDebug("All background services started");
    }

    private async Task StopBackgroundServices()
    {
        try
        {
            var tasks = new[] { _renderLoopTask, _performanceMonitoringTask, _maintenanceTask, _ipcListenerTask }
                .OfType<Task>()
                .ToArray();
            
            await Task.WhenAll(tasks);
            _logger.LogDebug("All background services stopped");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error stopping background services");
        }
    }

    private async Task RenderLoopAsync(CancellationToken cancellationToken)
    {
        _logger.LogInformation("Render loop started");
        
        var frameTimer = Stopwatch.StartNew();
        const double targetFrameTime = 1000.0 / 60.0; // 60 FPS target
        
        try
        {
            while (!cancellationToken.IsCancellationRequested)
            {
                frameTimer.Restart();
                
                // Render all active backends
                var renderTasks = _activeBackends.Values
                    .Select(backend => backend.RenderFrameAsync())
                    .ToArray();
                
                if (renderTasks.Length > 0)
                {
                    await Task.WhenAll(renderTasks);
                }
                
                // Frame rate limiting
                var frameTime = frameTimer.Elapsed.TotalMilliseconds;
                if (frameTime < targetFrameTime)
                {
                    var sleepTime = (int)(targetFrameTime - frameTime);
                    await Task.Delay(sleepTime, cancellationToken);
                }
            }
        }
        catch (OperationCanceledException)
        {
            // Expected when cancellation is requested
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error in render loop");
        }
        
        _logger.LogInformation("Render loop stopped");
    }

    private async Task PerformanceMonitoringLoopAsync(CancellationToken cancellationToken)
    {
        _logger.LogDebug("Performance monitoring started");
        
        try
        {
            while (!cancellationToken.IsCancellationRequested)
            {
                // Update performance metrics
                var systemMetrics = GetSystemPerformanceMetrics();
                PerformanceUpdate?.Invoke(this, new PerformanceUpdateEventArgs(systemMetrics));
                
                // Log performance warnings if needed
                if (systemMetrics.CurrentFps < 30.0f)
                {
                    _logger.LogWarning($"Low FPS detected: {systemMetrics.CurrentFps:F1}");
                }
                
                if ((long)systemMetrics.MemoryUsageMB > (long)_configuration.MemoryThresholdMB)
                {
                    _logger.LogWarning($"High memory usage detected: {systemMetrics.MemoryUsageMB} MB");
                }
                
                await Task.Delay(TimeSpan.FromSeconds(_configuration.PerformanceMonitoringIntervalSeconds), cancellationToken);
            }
        }
        catch (OperationCanceledException)
        {
            // Expected when cancellation is requested
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error in performance monitoring loop");
        }
        
        _logger.LogDebug("Performance monitoring stopped");
    }

    private async Task MaintenanceLoopAsync(CancellationToken cancellationToken)
    {
        _logger.LogDebug("Maintenance loop started");
        
        try
        {
            while (!cancellationToken.IsCancellationRequested)
            {
                // Cleanup disposed backends
                CleanupDisposedBackends();
                
                // Garbage collection if memory usage is high
                var systemMetrics = GetSystemPerformanceMetrics();
                if ((long)systemMetrics.MemoryUsageMB > (long)_configuration.MemoryThresholdMB)
                {
                    _logger.LogDebug("Triggering garbage collection due to high memory usage");
                    GC.Collect();
                    GC.WaitForPendingFinalizers();
                    GC.Collect();
                }
                
                await Task.Delay(TimeSpan.FromMinutes(1), cancellationToken);
            }
        }
        catch (OperationCanceledException)
        {
            // Expected when cancellation is requested
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error in maintenance loop");
        }
        
        _logger.LogDebug("Maintenance loop stopped");
    }

    private async Task IPCListenerLoopAsync(CancellationToken cancellationToken)
    {
        _logger.LogDebug("IPC listener started");
        
        try
        {
            await _ipcHandler.StartListeningAsync(cancellationToken);
        }
        catch (OperationCanceledException)
        {
            // Expected when cancellation is requested
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error in IPC listener loop");
        }
        
        _logger.LogDebug("IPC listener stopped");
    }

    private RenderBackendType SelectOptimalBackend(int width, int height)
    {
        // Intelligent backend selection based on content and capabilities
        
        if (_systemCapabilities.SupportsWebView2)
        {
            return RenderBackendType.WebView;
        }
        
        if (_systemCapabilities.SupportsSkiaSharp && _systemCapabilities.SupportsHardwareAcceleration)
        {
            return RenderBackendType.SkiaSharp;
        }
        
        // Fallback to most compatible backend
        return RenderBackendType.SkiaSharp;
    }

    private bool IsBackendSupported(RenderBackendType backendType)
    {
        return backendType switch
        {
            RenderBackendType.SkiaSharp => _systemCapabilities.SupportsSkiaSharp,
            RenderBackendType.Direct3D => _systemCapabilities.SupportsDirect3D,
            RenderBackendType.WebView => _systemCapabilities.SupportsWebView2,
            _ => false
        };
    }

    private IRenderBackend? CreateBackendInstance(RenderBackendType backendType)
    {
        try
        {
            if (_backendFactories.TryGetValue(backendType, out var factory))
            {
                return factory();
            }
            
            _logger.LogError($"No factory registered for backend type: {backendType}");
            return null;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Failed to create backend instance: {backendType}");
            return null;
        }
    }

private uint GenerateWidgetId()
    {
return (uint)(Environment.TickCount & 0x7FFFFFFF);
    }

    private void CleanupDisposedBackends()
    {
var toRemove = new List<uint>();
        
        foreach (var kvp in _activeBackends)
        {
            if (!kvp.Value.IsInitialized)
            {
                toRemove.Add(kvp.Key);
            }
        }
        
        foreach (var widgetId in toRemove)
        {
            if (_activeBackends.TryRemove(widgetId, out var backend))
            {
                backend.Dispose();
                _logger.LogDebug($"Cleaned up disposed backend for widget ID: {widgetId}");
            }
        }
    }

    private async Task StopAllBackends()
    {
        _logger.LogInformation($"Stopping {_activeBackends.Count} active backends");
        
        var stopTasks = _activeBackends.ToArray()
            .Select(async kvp =>
            {
                try
                {
                    await kvp.Value.CleanupAsync();
                    kvp.Value.Dispose();
                }
                catch (Exception ex)
                {
                    _logger.LogError(ex, $"Error stopping backend for widget ID: {kvp.Key}");
                }
            });
        
        await Task.WhenAll(stopTasks);
        _activeBackends.Clear();
    }

    private async Task CleanupIPC()
    {
        try
        {
            await _ipcHandler.StopAsync();
            _logger.LogDebug("IPC communication cleaned up");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error cleaning up IPC communication");
        }
    }

    private void UpdateMetrics(object? state)
    {
        try
        {
            _statistics.ActiveBackends = _activeBackends.Count;
            _statistics.UptimeSeconds = (int)(DateTime.UtcNow - _startTime).TotalSeconds;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error updating metrics");
        }
    }

    private void LogFinalStatistics()
    {
        var uptime = DateTime.UtcNow - _startTime;
        
        _logger.LogInformation($"RenderManager Final Statistics:");
        _logger.LogInformation($"  Uptime: {uptime:hh\\:mm\\:ss}");
        _logger.LogInformation($"  Backends Created: {_statistics.TotalBackendsCreated}");
        _logger.LogInformation($"  Backends Destroyed: {_statistics.TotalBackendsDestroyed}");
        _logger.LogInformation($"  Frames Rendered: {_statistics.TotalFramesRendered}");
        _logger.LogInformation($"  Failed Frames: {_statistics.FailedFrames}");
        _logger.LogInformation($"  Content Loads Successful: {_statistics.ContentLoadsSuccessful}");
        _logger.LogInformation($"  Content Loads Failed: {_statistics.ContentLoadsFailed}");
    }

    #endregion

    #region Event Handlers

    private void OnBackendRenderError(object? sender, RenderErrorEventArgs e)
    {
        _logger.LogError($"Render error from backend: {e.ErrorMessage}");
        RenderError?.Invoke(this, e);
    }

    private void OnBackendFrameRendered(object? sender, FrameRenderedEventArgs e)
    {
        // Aggregate frame statistics
        _statistics.TotalFramesRendered++;
    }

    #endregion

    #region Configuration and Capabilities

    private RenderConfiguration LoadConfiguration()
    {
        // Load configuration from file or use defaults
        return new RenderConfiguration
        {
            MaxConcurrentBackends = 50,
            MemoryThresholdMB = 500,
            PerformanceMonitoringIntervalSeconds = 5,
            EnableDebugLogging = false,
            EnablePerformanceOptimization = true
        };
    }

    private async Task InitializeSystemCapabilities()
    {
        _logger.LogInformation("System capabilities detected:");
        _logger.LogInformation($"  SkiaSharp: {_systemCapabilities.SupportsSkiaSharp}");
        _logger.LogInformation($"  Direct3D: {_systemCapabilities.SupportsDirect3D}");
        _logger.LogInformation($"  WebView2: {_systemCapabilities.SupportsWebView2}");
        _logger.LogInformation($"  Hardware Acceleration: {_systemCapabilities.SupportsHardwareAcceleration}");
    }

    private SystemCapabilities DetectSystemCapabilities()
    {
        var capabilities = new SystemCapabilities();
        
        try
        {
            // Detect SkiaSharp support
            capabilities.SupportsSkiaSharp = true; // SkiaSharp is included in dependencies
            
            // Detect WebView2 support
            capabilities.SupportsWebView2 = IsWebView2Available();
            
            // Detect hardware acceleration
            capabilities.SupportsHardwareAcceleration = DetectHardwareAcceleration();
            
            // Set supported backends list
            var supportedBackends = new List<RenderBackendType>();
            if (capabilities.SupportsSkiaSharp) supportedBackends.Add(RenderBackendType.SkiaSharp);
            if (capabilities.SupportsDirect3D) supportedBackends.Add(RenderBackendType.Direct3D);
            if (capabilities.SupportsWebView2) supportedBackends.Add(RenderBackendType.WebView);
            capabilities.SupportedBackends = supportedBackends.ToArray();
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error detecting system capabilities");
        }
        
        return capabilities;
    }

    private bool IsWebView2Available()
    {
        try
        {
            // Check if WebView2 runtime is available
            var runtimePath = Environment.GetEnvironmentVariable("WEBVIEW2_RUNTIME_PATH");
            if (!string.IsNullOrEmpty(runtimePath) && Directory.Exists(runtimePath))
            {
                return true;
            }
            
            // Check common installation locations
            var commonPaths = new[]
            {
                @"C:\Program Files (x86)\Microsoft\EdgeWebView\Application",
                @"C:\Program Files\Microsoft\EdgeWebView\Application"
            };
            
            return commonPaths.Any(path => Directory.Exists(path));
        }
        catch
        {
            return false;
        }
    }

    private bool DetectHardwareAcceleration()
    {
        try
        {
            // Simple check - assume hardware acceleration is available on modern systems
            return Environment.OSVersion.Version.Major >= 6; // Windows Vista and later
        }
        catch
        {
            return false;
        }
    }

    #endregion

    #region Supporting Classes

    public class RenderStatistics
    {
        public DateTime StartTime { get; set; }
        public int UptimeSeconds { get; set; }
        public int ActiveBackends { get; set; }
        public long TotalBackendsCreated { get; set; }
        public long TotalBackendsDestroyed { get; set; }
        public long TotalFramesRendered { get; set; }
        public long FailedFrames { get; set; }
        public long ContentLoadsSuccessful { get; set; }
        public long ContentLoadsFailed { get; set; }
    }

    public class RenderConfiguration
    {
        public int MaxConcurrentBackends { get; set; } = 50;
        public long MemoryThresholdMB { get; set; } = 500;
        public int PerformanceMonitoringIntervalSeconds { get; set; } = 5;
        public bool EnableDebugLogging { get; set; } = false;
        public bool EnablePerformanceOptimization { get; set; } = true;
    }

    public class SystemCapabilities
    {
        public bool SupportsSkiaSharp { get; set; }
        public bool SupportsDirect3D { get; set; }
        public bool SupportsWebView2 { get; set; }
        public bool SupportsHardwareAcceleration { get; set; }
        public RenderBackendType[]? SupportedBackends { get; set; }
    }

    public class PerformanceUpdateEventArgs : EventArgs
    {
        public PerformanceMetrics Metrics { get; }
        
        public PerformanceUpdateEventArgs(PerformanceMetrics metrics)
        {
            Metrics = metrics;
        }
    }

    #endregion

    #region IDisposable

    public void Dispose()
    {
        if (!_isDisposed)
        {
            StopAsync().Wait();
            _metricsTimer?.Dispose();
            _cancellationTokenSource?.Dispose();
            _isDisposed = true;
        }
    }

    #endregion
}
