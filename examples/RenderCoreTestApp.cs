using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using RenderProcess.Communication;
using RenderProcess.Runtime;
using RenderProcess.Examples;
using RenderProcess.Interfaces;
using System;
using System.Threading;
using System.Threading.Tasks;

namespace RenderProcess.Examples;

/// <summary>
/// Complete test application demonstrating the RenderCore system
/// Tests IPC communication, widget rendering, and system integration
/// </summary>
public class RenderCoreTestApp : BackgroundService
{
    private readonly ILogger<RenderCoreTestApp> _logger;
    private readonly IPCMessageHandler _ipcHandler;
    private readonly RenderManager _renderManager;
    private readonly IServiceProvider _serviceProvider;
    
    // Test widgets
    private SimpleClockWidget? _clockWidget;
    private uint _clockWidgetId;
    
    public RenderCoreTestApp(
        ILogger<RenderCoreTestApp> logger,
        IPCMessageHandler ipcHandler,
        RenderManager renderManager,
        IServiceProvider serviceProvider)
    {
        _logger = logger ?? throw new ArgumentNullException(nameof(logger));
        _ipcHandler = ipcHandler ?? throw new ArgumentNullException(nameof(ipcHandler));
        _renderManager = renderManager ?? throw new ArgumentNullException(nameof(renderManager));
        _serviceProvider = serviceProvider ?? throw new ArgumentNullException(nameof(serviceProvider));
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        try
        {
            _logger.LogInformation("🚀 Starting RenderCore Test Application");
            
            // Step 1: Initialize IPC Communication
            await InitializeIPCAsync();
            
            // Step 2: Start RenderManager
            await StartRenderManagerAsync();
            
            // Step 3: Create test widgets
            await CreateTestWidgetsAsync();
            
            // Step 4: Run test scenarios
            await RunTestScenariosAsync(stoppingToken);
            
        }
        catch (Exception ex)
        {
            _logger.LogCritical(ex, "Critical error in RenderCore test application");
        }
        finally
        {
            await CleanupAsync();
        }
    }

    private async Task InitializeIPCAsync()
    {
        try
        {
            _logger.LogInformation("📡 Initializing IPC communication...");
            
            // Setup event handlers
            _ipcHandler.CommandReceived += OnCommandReceived;
            _ipcHandler.IPCError += OnIPCError;
            _ipcHandler.ConnectionStatusChanged += OnConnectionStatusChanged;
            
            // Initialize IPC with default names (matching C++ implementation)
            bool success = await _ipcHandler.InitializeAsync(
                "RainmeterRenderSharedMemory", 
                "RainmeterRenderPipe"
            );
            
            if (success)
            {
                _logger.LogInformation("✅ IPC communication initialized successfully");
            }
            else
            {
                _logger.LogError("❌ Failed to initialize IPC communication");
                throw new InvalidOperationException("IPC initialization failed");
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to initialize IPC");
            throw;
        }
    }

    private async Task StartRenderManagerAsync()
    {
        try
        {
            _logger.LogInformation("🎨 Starting RenderManager...");
            
            bool success = await _renderManager.StartAsync();
            
            if (success)
            {
                _logger.LogInformation("✅ RenderManager started successfully");
                
                // Log system capabilities
                var capabilities = await DetectSystemCapabilitiesAsync();
                _logger.LogInformation($"📊 System Capabilities: {capabilities}");
            }
            else
            {
                _logger.LogError("❌ Failed to start RenderManager");
                throw new InvalidOperationException("RenderManager startup failed");
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to start RenderManager");
            throw;
        }
    }

    private async Task CreateTestWidgetsAsync()
    {
        try
        {
            _logger.LogInformation("🕐 Creating test clock widget...");
            
            // Create clock widget backend
            _clockWidgetId = await _renderManager.CreateRenderBackendAsync(
                RenderBackendType.WebView,  // Use WebView for HTML content
                IntPtr.Zero,               // No parent window for now
                300,                       // Width
                150                        // Height
            );
            
            if (_clockWidgetId > 0)
            {
                _logger.LogInformation($"✅ Clock widget backend created with ID: {_clockWidgetId}");
                
                // Create the widget wrapper
                var clockLogger = _serviceProvider.GetRequiredService<ILogger<SimpleClockWidget>>();
                _clockWidget = new SimpleClockWidget(clockLogger);
                
                // Initialize the widget
                await _clockWidget.InitializeAsync(IntPtr.Zero, 300, 150);
                
                _logger.LogInformation("✅ Clock widget initialized successfully");
            }
            else
            {
                _logger.LogError("❌ Failed to create clock widget backend");
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to create test widgets");
        }
    }

    private async Task RunTestScenariosAsync(CancellationToken stoppingToken)
    {
        try
        {
            _logger.LogInformation("🧪 Running test scenarios...");
            
            int testCount = 0;
            var startTime = DateTime.UtcNow;
            
            // Start IPC listening
            var listeningTask = _ipcHandler.StartListeningAsync(stoppingToken);
            
            while (!stoppingToken.IsCancellationRequested)
            {
                testCount++;
                
                // Test 1: Clock Widget Rendering
                await TestClockWidgetRenderingAsync(testCount);
                
                // Test 2: Performance Metrics
                await TestPerformanceMetricsAsync();
                
                // Test 3: IPC Communication Test
                await TestIPCCommunicationAsync();
                
                // Test 4: Widget Lifecycle
                if (testCount % 10 == 0) // Every 10th cycle
                {
                    await TestWidgetLifecycleAsync();
                }
                
                // Log summary every 30 seconds
                if (testCount % 30 == 0)
                {
                    var uptime = DateTime.UtcNow - startTime;
                    _logger.LogInformation($"📈 Test Summary - Count: {testCount}, Uptime: {uptime:hh\\:mm\\:ss}");
                    
                    // Get system performance metrics
                    var metrics = _renderManager.GetSystemPerformanceMetrics();
                    _logger.LogInformation($"📊 Performance: {metrics}");
                }
                
                // Wait 1 second before next test cycle
                await Task.Delay(1000, stoppingToken);
            }
            
            await listeningTask;
        }
        catch (OperationCanceledException)
        {
            _logger.LogInformation("Test scenarios cancelled by user");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error running test scenarios");
        }
    }

    private async Task TestClockWidgetRenderingAsync(int testNumber)
    {
        try
        {
            if (_clockWidget == null || _clockWidgetId == 0)
                return;
            
            _logger.LogDebug($"🕐 Test {testNumber}: Clock widget rendering");
            
            // Create render command
            var renderCommand = _clockWidget.CreateRenderCommand(_clockWidgetId);
            
            // Send command via RenderManager
            bool success = await _renderManager.LoadContentAsync(
                _clockWidgetId,
                renderCommand.Content.TemplateData,
                renderCommand.Content
            );
            
            if (success)
            {
                _logger.LogDebug($"✅ Clock render command sent successfully");
                
                // Render the frame
                await _renderManager.RenderFrameAsync(_clockWidgetId);
            }
            else
            {
                _logger.LogWarning($"⚠️ Clock render command failed");
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error testing clock widget rendering");
        }
    }

    private async Task TestPerformanceMetricsAsync()
    {
        try
        {
            var systemMetrics = _renderManager.GetSystemPerformanceMetrics();
            
            // Log performance warnings if needed
            if (systemMetrics.CurrentFps < 30.0f && systemMetrics.TotalFrames > 60)
            {
                _logger.LogWarning($"⚠️ Low FPS detected: {systemMetrics.CurrentFps:F1}");
            }
            
            if (systemMetrics.MemoryUsageMB > 200)
            {
                _logger.LogWarning($"⚠️ High memory usage: {systemMetrics.MemoryUsageMB}MB");
            }
            
            // Log success every 10 checks
            if (Environment.TickCount % 10000 < 1000)
            {
                _logger.LogDebug($"📊 Performance check: FPS={systemMetrics.CurrentFps:F1}, Memory={systemMetrics.MemoryUsageMB}MB");
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error testing performance metrics");
        }
    }

    private async Task TestIPCCommunicationAsync()
    {
        try
        {
            // Test IPC connection health
            bool isHealthy = _ipcHandler.TestConnection();
            
            if (!isHealthy)
            {
                _logger.LogWarning("⚠️ IPC connection health check failed");
                
                // Attempt to reinitialize
                await _ipcHandler.InitializeAsync();
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error testing IPC communication");
        }
    }

    private async Task TestWidgetLifecycleAsync()
    {
        try
        {
            _logger.LogInformation("🔄 Testing widget lifecycle...");
            
            if (_clockWidget != null)
            {
                // Test resize operation
                await _clockWidget.ResizeAsync(400, 200);
                _logger.LogDebug("✅ Widget resize test completed");
                
                // Restore original size
                await Task.Delay(1000);
                await _clockWidget.ResizeAsync(300, 150);
                _logger.LogDebug("✅ Widget resize restore completed");
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error testing widget lifecycle");
        }
    }

    private async Task<SystemCapabilities> DetectSystemCapabilitiesAsync()
    {
        try
        {
            // This would normally be queried from the C++ core
            // For now, simulate basic capability detection
            var capabilities = new SystemCapabilities
            {
                SupportsSkiaSharp = true,
                SupportsWebView2 = IsWebView2Available(),
                SupportsDirect3D = false, // Would need actual detection
                SupportsHardwareAcceleration = true,
                SupportsMultiMonitor = true,
                SupportsHighDPI = true,
                GpuName = "Simulated GPU",
                DriverVersion = "1.0.0",
                TotalVRAM = 1024,
                AvailableVRAM = 512
            };
            
            return capabilities;
        }
        catch
        {
            return new SystemCapabilities();
        }
    }

    private bool IsWebView2Available()
    {
        try
        {
            // Simple check for WebView2 availability
            var commonPaths = new[]
            {
                @"C:\Program Files (x86)\Microsoft\EdgeWebView\Application",
                @"C:\Program Files\Microsoft\EdgeWebView\Application"
            };
            
            return Array.Exists(commonPaths, System.IO.Directory.Exists);
        }
        catch
        {
            return false;
        }
    }

    #region Event Handlers

    private async void OnCommandReceived(object? sender, RenderCommandReceivedEventArgs e)
    {
        try
        {
            _logger.LogDebug($"📨 Received command: {e.Command}");
            
            // Process the command and send result back
            var result = await ProcessCommand(e.Command);
            await _ipcHandler.SendResultAsync(result);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error handling received command");
        }
    }

    private void OnIPCError(object? sender, IPCErrorEventArgs e)
    {
        _logger.LogError($"🚨 IPC Error: {e.ErrorMessage}");
        if (e.Exception != null)
        {
            _logger.LogError(e.Exception, "IPC Exception details");
        }
    }

    private void OnConnectionStatusChanged(object? sender, ConnectionStatusEventArgs e)
    {
        if (e.IsConnected)
        {
            _logger.LogInformation("✅ IPC connection established");
        }
        else
        {
            _logger.LogWarning("⚠️ IPC connection lost");
        }
    }

    #endregion

    private async Task<RenderResult> ProcessCommand(RenderCommand command)
    {
        try
        {
            _logger.LogDebug($"Processing command {command.CommandId} of type {command.CommandType}");
            
            // Simulate command processing
            await Task.Delay(10); // Simulate processing time
            
            return RenderResult.Success(command.CommandId, command.WidgetId);
        }
        catch (Exception ex)
        {
            return RenderResult.Failure(command.CommandId, command.WidgetId, ex.Message);
        }
    }

    private async Task CleanupAsync()
    {
        try
        {
            _logger.LogInformation("🧹 Cleaning up test application...");
            
            // Cleanup widgets
            if (_clockWidget != null)
            {
                await _clockWidget.CleanupAsync();
                _clockWidget.Dispose();
            }
            
            // Destroy widget backends
            if (_clockWidgetId > 0)
            {
                await _renderManager.DestroyRenderBackendAsync(_clockWidgetId);
            }
            
            // Stop services
            await _renderManager.StopAsync();
            await _ipcHandler.StopAsync();
            
            _logger.LogInformation("✅ Cleanup completed");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error during cleanup");
        }
    }
}

/// <summary>
/// Program entry point for the RenderCore test application
/// </summary>
public class Program
{
    public static async Task Main(string[] args)
    {
        try
        {
            Console.WriteLine("🎯 RainmeterManager RenderCore Test Application");
            Console.WriteLine("================================================");
            
            var host = CreateHostBuilder(args).Build();
            
            // Run the test application
            await host.RunAsync();
        }
        catch (Exception ex)
        {
            Console.WriteLine($"❌ Fatal error: {ex.Message}");
            Console.WriteLine(ex.ToString());
        }
    }

    private static IHostBuilder CreateHostBuilder(string[] args) =>
        Host.CreateDefaultBuilder(args)
            .ConfigureServices((context, services) =>
            {
                // Configure logging
                services.AddLogging(builder =>
                {
                    builder.AddConsole();
                    builder.SetMinimumLevel(LogLevel.Information);
                });
                
                // Register core services
                services.AddSingleton<IPCMessageHandler>();
                services.AddSingleton<RenderManager>();
                
                // Register the test application
                services.AddHostedService<RenderCoreTestApp>();
            });
}
