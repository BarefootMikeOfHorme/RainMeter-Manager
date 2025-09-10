using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using RenderProcess.Runtime;
using RenderProcess.Communication;
using RenderProcess.Backends;
using RenderProcess.Content;
using System;
using System.Threading;
using System.Threading.Tasks;

namespace RenderProcess;

public class Program
{
    public static async Task<int> Main(string[] args)
    {
        try
        {
            Console.WriteLine("RainmeterManager Render Process v1.0.0 Starting...");
            
            // Set up dependency injection and hosting
            var host = CreateHostBuilder(args).Build();
            
            // Start the render manager
            var renderManager = host.Services.GetRequiredService<RenderManager>();
            var cancellationToken = new CancellationTokenSource();
            
            // Handle graceful shutdown
            Console.CancelKeyPress += (sender, e) => {
                e.Cancel = true;
                cancellationToken.Cancel();
            };
            
            // Run the render process
            await renderManager.StartAsync(cancellationToken.Token);
            
            Console.WriteLine("Render process started successfully");
            
            // Keep running until cancelled
            await Task.Delay(Timeout.Infinite, cancellationToken.Token);
            
            return 0;
        }
        catch (OperationCanceledException)
        {
            Console.WriteLine("Render process shutting down gracefully...");
            return 0;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Fatal error: {ex.Message}");
            return 1;
        }
    }
    
    private static IHostBuilder CreateHostBuilder(string[] args) =>
        Host.CreateDefaultBuilder(args)
            .ConfigureServices((context, services) => {
                // Register render services
                services.AddSingleton<RenderManager>();
                services.AddSingleton<IPCMessageHandler>();
                services.AddSingleton<PerformanceMonitor>();
                services.AddSingleton<DashboardService>();
                services.AddSingleton<MonitorManager>();
                services.AddSingleton<ProcessService>();
                
                // Register backend services
                services.AddTransient<SkiaSharpRenderer>();
                services.AddTransient<Direct3DRenderer>();
                services.AddTransient<WebViewRenderer>();
                
                // Register content loaders
                services.AddTransient<WebContentLoader>();
                services.AddTransient<APIContentLoader>();
                services.AddTransient<MediaContentLoader>();
                services.AddTransient<FileContentLoader>();
            })
            .ConfigureLogging(logging => {
                logging.ClearProviders();
                logging.AddConsole();
                logging.SetMinimumLevel(LogLevel.Information);
            });
}
