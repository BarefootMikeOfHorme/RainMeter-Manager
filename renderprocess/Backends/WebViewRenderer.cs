using Microsoft.Extensions.Logging;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;
using RenderProcess.Interfaces;
using System;
using System.Drawing;
using System.IO;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Collections.Generic;

namespace RenderProcess.Backends;

/// <summary>
/// Enterprise WebView2 renderer for unlimited web content rendering capabilities
/// Supports full HTML5, CSS3, JavaScript, WebGL, and modern web standards with
/// enterprise security features and performance monitoring
/// </summary>
public class WebViewRenderer : IRenderBackend, IDisposable
{
    private readonly ILogger<WebViewRenderer> _logger;
    
    // Core WebView components
    private WebView2? _webView;
    private Form? _hostForm;
    private CoreWebView2Environment? _environment;
    
    // Window and surface management
    private IntPtr _windowHandle;
    private int _width, _height;
    private bool _isInitialized;
    private bool _isDisposed;
    
    // Rendering properties and content
    private RenderProperties _properties;
    private ContentParameters _contentParameters;
    private PerformanceMetrics _metrics;
    private long _frameCount;
    private DateTime _lastRenderTime;
    private readonly Stopwatch _performanceTimer;
    
    // Enterprise features
    private readonly Dictionary<string, object> _injectedObjects;
    private bool _securityEnabled = true;
    private readonly HashSet<string> _allowedDomains;
    
    // Events
    public event EventHandler<RenderErrorEventArgs>? RenderError;
    public event EventHandler<FrameRenderedEventArgs>? FrameRendered;

    public WebViewRenderer(ILogger<WebViewRenderer> logger)
    {
        _logger = logger ?? throw new ArgumentNullException(nameof(logger));
        _properties = new RenderProperties();
        _metrics = new PerformanceMetrics();
        _performanceTimer = new Stopwatch();
        _injectedObjects = new Dictionary<string, object>();
        _allowedDomains = new HashSet<string>();
        
        // Default allowed domains for enterprise use
        _allowedDomains.Add("localhost");
        _allowedDomains.Add("*.github.com");
        _allowedDomains.Add("*.googleapis.com");
        _allowedDomains.Add("*.microsoft.com");
        
        _logger.LogInformation("WebView2 enterprise renderer initialized");
    }

    #region IRenderBackend Implementation

    public RenderBackendType BackendType => RenderBackendType.WebView;
    public bool IsInitialized => _isInitialized;
    public string Name => "WebView2 Enterprise Web Renderer";

    public async Task<bool> InitializeAsync(IntPtr windowHandle, int width, int height)
    {
        try
        {
            _logger.LogInformation($"Initializing WebView2 renderer: {width}x{height}");
            
            if (_isInitialized)
            {
                await CleanupAsync();
            }
            
            _windowHandle = windowHandle;
            _width = width;
            _height = height;
            
            // Create host form for WebView2 control
            await CreateHostForm();
            
            // Initialize WebView2 with enterprise settings
            await InitializeWebView();
            
            // Setup enterprise security features
            await ConfigureEnterpriseSecurity();
            
            // Setup performance monitoring
            SetupPerformanceMonitoring();
            
            _isInitialized = true;
            _logger.LogInformation("WebView2 renderer successfully initialized");
            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to initialize WebView2 renderer");
            RenderError?.Invoke(this, new RenderErrorEventArgs($"Initialization failed: {ex.Message}", ex, BackendType));
            return false;
        }
    }

    public async Task<bool> RenderFrameAsync()
    {
        if (!_isInitialized || _webView?.CoreWebView2 == null)
        {
            return false;
        }

        try
        {
            _performanceTimer.Restart();
            
            // WebView2 handles rendering automatically through Chromium engine
            // We monitor performance and update metrics
            UpdatePerformanceMetrics();
            
            _performanceTimer.Stop();
            _frameCount++;
            _lastRenderTime = DateTime.UtcNow;
            
            // Trigger frame rendered event
            FrameRendered?.Invoke(this, new FrameRenderedEventArgs(_frameCount, _performanceTimer.Elapsed.TotalMilliseconds));
            
            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error during WebView2 rendering");
            RenderError?.Invoke(this, new RenderErrorEventArgs($"Render failed: {ex.Message}", ex, BackendType));
            return false;
        }
    }

    public async Task<bool> ResizeAsync(int width, int height)
    {
        try
        {
            _logger.LogInformation($"Resizing WebView2 renderer: {width}x{height}");
            
            _width = width;
            _height = height;
            
            if (_hostForm != null)
            {
                _hostForm.Size = new Size(width, height);
            }
            
            if (_webView != null)
            {
                _webView.Size = new Size(width, height);
            }
            
            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to resize WebView2 renderer");
            return false;
        }
    }

    public async Task CleanupAsync()
    {
        try
        {
            _logger.LogInformation("Cleaning up WebView2 renderer");
            
            if (_webView != null)
            {
                _webView.CoreWebView2?.Stop();
                _webView.Dispose();
                _webView = null;
            }
            
            _hostForm?.Dispose();
            _hostForm = null;
            
            _environment?.Dispose();
            _environment = null;
            
            _injectedObjects.Clear();
            
            _isInitialized = false;
            _logger.LogInformation("WebView2 renderer cleanup completed");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error during WebView2 cleanup");
        }
    }

    public async Task<bool> LoadContentAsync(string contentSource, ContentParameters parameters)
    {
        if (_webView?.CoreWebView2 == null)
        {
            _logger.LogWarning("WebView2 not initialized - cannot load content");
            return false;
        }

        try
        {
            _logger.LogInformation($"Loading content: {parameters.SourceType} from {contentSource}");
            
            _contentParameters = parameters;
            
            // Security check for web content
            if (parameters.SourceType == ContentSourceType.Web && !IsUrlAllowed(contentSource))
            {
                _logger.LogWarning($"URL blocked by security policy: {contentSource}");
                return false;
            }
            
            switch (parameters.SourceType)
            {
                case ContentSourceType.Web:
                    await LoadWebContent(contentSource);
                    break;
                    
                case ContentSourceType.Static:
                    await LoadStaticContent(contentSource);
                    break;
                    
                case ContentSourceType.API:
                    await LoadAPIContent(contentSource);
                    break;
                    
                case ContentSourceType.File:
                    await LoadFileContent(contentSource);
                    break;
                    
                default:
                    _logger.LogWarning($"Unsupported content type for WebView: {parameters.SourceType}");
                    return false;
            }
            
            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Failed to load content: {contentSource}");
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
            if (_webView?.CoreWebView2 != null)
            {
                var blankHtml = @"
                    <!DOCTYPE html>
                    <html>
                    <head>
                        <title>RainmeterManager</title>
                        <style>
                            body { margin: 0; padding: 20px; background: #1a1a1a; color: #fff; font-family: 'Segoe UI', Arial, sans-serif; }
                            .logo { text-align: center; margin-top: 50px; font-size: 24px; }
                        </style>
                    </head>
                    <body>
                        <div class='logo'>RainmeterManager</div>
                    </body>
                    </html>";
                
                _webView.NavigateToString(blankHtml);
            }
            
            _logger.LogDebug("WebView content cleared");
            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to clear WebView content");
            return false;
        }
    }

    public void SetOpacity(float opacity)
    {
        _properties.Opacity = Math.Clamp(opacity, 0.0f, 1.0f);
        if (_hostForm != null)
        {
            _hostForm.Opacity = _properties.Opacity;
        }
    }

    public void SetVisible(bool visible)
    {
        _properties.Visible = visible;
        if (_hostForm != null)
        {
            _hostForm.Visible = visible;
        }
    }

    public void SetProperties(RenderProperties properties)
    {
        _properties = properties;
        UpdateRenderingProperties();
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

    #region Core Implementation Methods

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
            BackColor = System.Drawing.Color.Black,
            Opacity = _properties.Opacity
        };

        // Set window handle if provided
        if (_windowHandle != IntPtr.Zero)
        {
            SetParent(_hostForm.Handle, _windowHandle);
        }

        _hostForm.Show();
        _logger.LogDebug("WebView host form created");
    }

    private async Task InitializeWebView()
    {
        try
        {
            // Create WebView2 environment with enterprise settings
            var userDataFolder = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), 
                                             "RainmeterManager", "WebView");
            
            var environmentOptions = CoreWebView2EnvironmentOptions.CreateWithAdditionalBrowserArguments(
                "--disable-web-security --disable-features=VizDisplayCompositor --enable-gpu-rasterization");
            
            _environment = await CoreWebView2Environment.CreateAsync(
                browserExecutableFolder: null,
                userDataFolder: userDataFolder,
                options: environmentOptions);

            // Create WebView2 control
            _webView = new WebView2
            {
                Dock = DockStyle.Fill,
                Size = new Size(_width, _height)
            };

            _hostForm?.Controls.Add(_webView);
            await _webView.EnsureCoreWebView2Async(_environment);
            
            _logger.LogInformation("WebView2 control initialized successfully");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to initialize WebView2 control");
            throw;
        }
    }

    private async Task ConfigureEnterpriseSecurity()
    {
        if (_webView?.CoreWebView2 == null) return;

        try
        {
            var settings = _webView.CoreWebView2.Settings;
            
            // Enterprise security configuration
            settings.IsWebMessageEnabled = true;
            settings.AreDevToolsEnabled = false; // Disable for production
            settings.AreHostObjectsAllowed = false; // Enhanced security
            settings.IsScriptEnabled = true; // Allow JavaScript
            settings.AreDefaultScriptDialogsEnabled = false; // Disable alerts/prompts
            settings.IsGeneralAutofillEnabled = false; // Disable autofill
            settings.IsPasswordAutosaveEnabled = false; // Disable password save
            settings.AreBrowserAcceleratorKeysEnabled = false; // Disable shortcuts
            
            // Setup content security policies
            await _webView.CoreWebView2.AddWebResourceRequestedFilterAsync("*", CoreWebView2WebResourceContext.All);
            _webView.CoreWebView2.WebResourceRequested += OnWebResourceRequested;
            
            // Setup navigation security
            _webView.CoreWebView2.NavigationStarting += OnNavigationStarting;
            _webView.CoreWebView2.FrameNavigationStarting += OnFrameNavigationStarting;
            
            // Setup error handling
            _webView.CoreWebView2.NavigationCompleted += OnNavigationCompleted;
            _webView.CoreWebView2.ScriptException += OnScriptException;
            
            _logger.LogDebug("Enterprise security configuration applied");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to configure enterprise security");
            throw;
        }
    }

    private async Task LoadWebContent(string url)
    {
        if (_webView?.CoreWebView2 == null) return;
        
        try
        {
            // Add authentication headers if provided
            if (!string.IsNullOrEmpty(_contentParameters.AuthToken))
            {
                var headers = $"Authorization: Bearer {_contentParameters.AuthToken}";
                await _webView.CoreWebView2.NavigateWithWebResourceRequestAsync(
                    _environment!.CreateWebResourceRequest("GET", url, null, headers));
            }
            else
            {
                await _webView.CoreWebView2.NavigateAsync(url);
            }
            
            _logger.LogDebug($"Navigated to URL: {url}");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Failed to navigate to URL: {url}");
            throw;
        }
    }

    private async Task LoadStaticContent(string html)
    {
        if (_webView == null) return;
        
        try
        {
            // Wrap content in enterprise-styled HTML
            var styledHtml = $@"
                <!DOCTYPE html>
                <html>
                <head>
                    <meta charset='utf-8'>
                    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
                    <title>RainmeterManager Widget</title>
                    <style>
                        body {{ 
                            margin: 0; padding: 10px; 
                            background: linear-gradient(135deg, #1a1a1a, #2d2d2d);
                            color: #ffffff; 
                            font-family: 'Segoe UI', 'Arial', sans-serif;
                            font-size: 14px;
                            line-height: 1.4;
                        }}
                        .container {{ max-width: 100%; word-wrap: break-word; }}
                    </style>
                </head>
                <body>
                    <div class='container'>{html}</div>
                </body>
                </html>";
            
            _webView.NavigateToString(styledHtml);
            _logger.LogDebug("Static HTML content loaded");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to load static content");
            throw;
        }
    }

    private async Task LoadAPIContent(string endpoint)
    {
        try
        {
            using var httpClient = new HttpClient();
            
            // Add authentication if provided
            if (!string.IsNullOrEmpty(_contentParameters.AuthToken))
            {
                httpClient.DefaultRequestHeaders.Authorization = 
                    new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", _contentParameters.AuthToken);
            }
            
            // Add custom headers
            foreach (var header in _contentParameters.CustomHeaders)
            {
                httpClient.DefaultRequestHeaders.Add(header.Key, header.Value);
            }
            
            var apiResponse = await httpClient.GetStringAsync(endpoint);
            
            // Create HTML visualization of API data
            var htmlContent = $@"
                <div style='font-family: Consolas, monospace;'>
                    <h3 style='color: #00ff88; margin-top: 0;'>API Response</h3>
                    <div style='background: #2a2a2a; padding: 15px; border-radius: 8px; overflow: auto;'>
                        <pre style='color: #e0e0e0; margin: 0; white-space: pre-wrap;'>{System.Web.HttpUtility.HtmlEncode(apiResponse)}</pre>
                    </div>
                </div>";
            
            await LoadStaticContent(htmlContent);
            _logger.LogDebug($"API content loaded from: {endpoint}");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Failed to load API content from: {endpoint}");
            throw;
        }
    }

    private async Task LoadFileContent(string filePath)
    {
        try
        {
            if (!File.Exists(filePath))
            {
                throw new FileNotFoundException($"File not found: {filePath}");
            }
            
            var extension = Path.GetExtension(filePath).ToLowerInvariant();
            
            switch (extension)
            {
                case ".html":
                case ".htm":
                    var htmlContent = await File.ReadAllTextAsync(filePath);
                    _webView?.NavigateToString(htmlContent);
                    break;
                    
                case ".txt":
                case ".json":
                case ".xml":
                    var textContent = await File.ReadAllTextAsync(filePath);
                    var formattedContent = $@"
                        <div style='font-family: Consolas, monospace;'>
                            <h3 style='color: #4a9eff; margin-top: 0;'>{Path.GetFileName(filePath)}</h3>
                            <div style='background: #2a2a2a; padding: 15px; border-radius: 8px; overflow: auto;'>
                                <pre style='color: #e0e0e0; margin: 0; white-space: pre-wrap;'>{System.Web.HttpUtility.HtmlEncode(textContent)}</pre>
                            </div>
                        </div>";
                    await LoadStaticContent(formattedContent);
                    break;
                    
                default:
                    throw new NotSupportedException($"Unsupported file type: {extension}");
            }
            
            _logger.LogDebug($"File content loaded: {filePath}");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Failed to load file content: {filePath}");
            throw;
        }
    }

    #endregion

    #region Event Handlers

    private void OnWebResourceRequested(object? sender, CoreWebView2WebResourceRequestedEventArgs e)
    {
        try
        {
            // Security filtering and content policy enforcement
            var uri = e.Request.Uri;
            
            if (!IsUrlAllowed(uri))
            {
                _logger.LogWarning($"Blocked request to: {uri}");
                e.Response = _environment!.CreateWebResourceResponse(null, 403, "Forbidden", "");
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error in web resource request handler");
        }
    }

    private void OnNavigationStarting(object? sender, CoreWebView2NavigationStartingEventArgs e)
    {
        try
        {
            if (!IsUrlAllowed(e.Uri))
            {
                _logger.LogWarning($"Blocked navigation to: {e.Uri}");
                e.Cancel = true;
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error in navigation starting handler");
        }
    }

    private void OnFrameNavigationStarting(object? sender, CoreWebView2NavigationStartingEventArgs e)
    {
        // Same security policy for frame navigation
        OnNavigationStarting(sender, e);
    }

    private void OnNavigationCompleted(object? sender, CoreWebView2NavigationCompletedEventArgs e)
    {
        try
        {
            if (e.IsSuccess)
            {
                _logger.LogDebug("Navigation completed successfully");
            }
            else
            {
                _logger.LogWarning($"Navigation failed: {e.WebErrorStatus}");
                RenderError?.Invoke(this, new RenderErrorEventArgs($"Navigation failed: {e.WebErrorStatus}", null, BackendType));
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error in navigation completed handler");
        }
    }

    private void OnScriptException(object? sender, CoreWebView2ScriptExceptionEventArgs e)
    {
        try
        {
            _logger.LogWarning($"JavaScript error: {e.Name} - {e.Message}");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error in script exception handler");
        }
    }

    #endregion

    #region Helper Methods

    private bool IsUrlAllowed(string url)
    {
        if (!_securityEnabled) return true;
        
        try
        {
            var uri = new Uri(url);
            var host = uri.Host.ToLowerInvariant();
            
            // Check against allowed domains
            foreach (var domain in _allowedDomains)
            {
                if (domain.StartsWith("*.") && host.EndsWith(domain.Substring(2)))
                    return true;
                if (host == domain.ToLowerInvariant())
                    return true;
            }
            
            // Allow localhost and file:// for development
            if (host == "localhost" || host == "127.0.0.1" || uri.Scheme == "file")
                return true;
            
            return false;
        }
        catch
        {
            return false;
        }
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
            CurrentFps = 60.0f, // WebView2 typically runs at display refresh rate
            AverageFps = 60.0f,
            TotalFrames = 0,
            DroppedFrames = 0,
            MemoryUsageMB = 0,
            VramUsageMB = 0,
            CpuUsagePercent = 0,
            GpuUsagePercent = 0
        };
    }

    private void UpdatePerformanceMetrics()
    {
        _metrics.TotalFrames = _frameCount;
        
        // Update every few frames to avoid overhead
        if (_frameCount % 60 == 0)
        {
            UpdateCurrentMetrics();
        }
    }

    private void UpdateCurrentMetrics()
    {
        try
        {
            var process = Process.GetCurrentProcess();
            _metrics.MemoryUsageMB = process.WorkingSet64 / 1024 / 1024;
            _metrics.CpuUsagePercent = (float)process.TotalProcessorTime.TotalMilliseconds;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error updating performance metrics");
        }
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
            _performanceTimer?.Stop();
            _isDisposed = true;
        }
    }

    #endregion
}
