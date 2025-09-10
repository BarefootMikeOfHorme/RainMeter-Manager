using Microsoft.Extensions.Logging;
using RenderProcess.Interfaces;
using System;
using System.Collections.Concurrent;
using System.IO;
using System.IO.MemoryMappedFiles;
using System.IO.Pipes;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

namespace RenderProcess.Communication;

/// <summary>
/// Enterprise-grade IPC message handler for C# render process
/// Manages communication with C++ core via shared memory and named pipes
/// </summary>
public class IPCMessageHandler : IDisposable
{
    private readonly ILogger<IPCMessageHandler> _logger;
    
    // Communication channels
    private MemoryMappedFile? _sharedMemory;
    private MemoryMappedViewAccessor? _memoryAccessor;
    private NamedPipeClientStream? _namedPipe;
    
    // Synchronization
    private EventWaitHandle? _commandEvent;
    private EventWaitHandle? _resultEvent;
    private Mutex? _accessMutex;
    
    // Threading
    private CancellationTokenSource? _cancellationTokenSource;
    private Task? _messageProcessingTask;
    private readonly object _disposeLock = new();
    
    // State
    private bool _isConnected;
    private bool _isDisposed;
    private string _sharedMemoryName = "RainmeterRenderSharedMemory";
    private string _namedPipeName = "RainmeterRenderPipe";
    
    // Message processing
    private readonly ConcurrentQueue<RenderCommand> _commandQueue;
    private readonly SemaphoreSlim _commandSemaphore;
    
    // Event handlers
    public event EventHandler<RenderCommandReceivedEventArgs>? CommandReceived;
    public event EventHandler<IPCErrorEventArgs>? IPCError;
    public event EventHandler<ConnectionStatusEventArgs>? ConnectionStatusChanged;

    public IPCMessageHandler(ILogger<IPCMessageHandler> logger)
    {
        _logger = logger ?? throw new ArgumentNullException(nameof(logger));
        _commandQueue = new ConcurrentQueue<RenderCommand>();
        _commandSemaphore = new SemaphoreSlim(0);
        
        _logger.LogInformation("IPCMessageHandler initialized");
    }

    #region Public Interface

    /// <summary>
    /// Initialize IPC communication with C++ core
    /// </summary>
    public async Task<bool> InitializeAsync(string? sharedMemoryName = null, string? namedPipeName = null)
    {
        try
        {
            if (_isConnected)
            {
                _logger.LogWarning("IPC already initialized");
                return true;
            }

            _sharedMemoryName = sharedMemoryName ?? _sharedMemoryName;
            _namedPipeName = namedPipeName ?? _namedPipeName;

            _logger.LogInformation($"Initializing IPC communication: SharedMemory={_sharedMemoryName}, NamedPipe={_namedPipeName}");

            // Initialize shared memory
            if (!await InitializeSharedMemory())
            {
                _logger.LogError("Failed to initialize shared memory");
                return false;
            }

            // Initialize named pipe
            if (!await InitializeNamedPipe())
            {
                _logger.LogWarning("Failed to initialize named pipe, continuing with shared memory only");
            }

            // Start message processing
            _cancellationTokenSource = new CancellationTokenSource();
            _messageProcessingTask = ProcessMessagesAsync(_cancellationTokenSource.Token);

            _isConnected = true;
            ConnectionStatusChanged?.Invoke(this, new ConnectionStatusEventArgs(true));

            _logger.LogInformation("IPC communication initialized successfully");
            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to initialize IPC communication");
            await CleanupAsync();
            return false;
        }
    }

    /// <summary>
    /// Send render result back to C++ core
    /// </summary>
    public async Task<bool> SendResultAsync(RenderResult result)
    {
        try
        {
            if (!_isConnected)
            {
                _logger.LogWarning("Cannot send result - IPC not connected");
                return false;
            }

            _logger.LogDebug($"Sending result for command {result.CommandId}: {result.Status}");

            // Try shared memory first (faster)
            if (_memoryAccessor != null && await SendResultViaSharedMemory(result))
            {
                return true;
            }

            // Fallback to named pipe
            if (_namedPipe != null && _namedPipe.IsConnected && await SendResultViaNamedPipe(result))
            {
                return true;
            }

            _logger.LogError($"Failed to send result via any communication channel");
            return false;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Exception sending result for command {result.CommandId}");
            IPCError?.Invoke(this, new IPCErrorEventArgs($"Send result failed: {ex.Message}", ex));
            return false;
        }
    }

    /// <summary>
    /// Start listening for commands from C++ core
    /// </summary>
    public async Task StartListeningAsync(CancellationToken cancellationToken = default)
    {
        try
        {
            if (!_isConnected)
            {
                throw new InvalidOperationException("IPC not initialized");
            }

            _logger.LogInformation("Starting IPC message listening");

            while (!cancellationToken.IsCancellationRequested && _isConnected)
            {
                try
                {
                    // Wait for command signal or timeout
                    bool commandAvailable = false;

                    if (_commandEvent != null)
                    {
                        commandAvailable = await Task.Run(() => 
                            _commandEvent.WaitOne(1000), cancellationToken);
                    }

                    if (commandAvailable)
                    {
                        await ProcessIncomingCommands();
                    }

                    // Process any queued commands
                    while (_commandQueue.TryDequeue(out var command))
                    {
                        await HandleCommand(command);
                    }

                    // Small delay to prevent busy waiting
                    await Task.Delay(10, cancellationToken);
                }
                catch (OperationCanceledException)
                {
                    break;
                }
                catch (Exception ex)
                {
                    _logger.LogError(ex, "Error in message listening loop");
                    IPCError?.Invoke(this, new IPCErrorEventArgs($"Listening error: {ex.Message}", ex));
                    
                    // Brief delay before retrying
                    await Task.Delay(1000, cancellationToken);
                }
            }

            _logger.LogInformation("IPC message listening stopped");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to start IPC listening");
            throw;
        }
    }

    /// <summary>
    /// Stop IPC communication
    /// </summary>
    public async Task StopAsync()
    {
        try
        {
            if (!_isConnected)
            {
                return;
            }

            _logger.LogInformation("Stopping IPC communication");

            _isConnected = false;
            ConnectionStatusChanged?.Invoke(this, new ConnectionStatusEventArgs(false));

            // Cancel background tasks
            _cancellationTokenSource?.Cancel();

            // Wait for message processing to complete
            if (_messageProcessingTask != null)
            {
                try
                {
                    await _messageProcessingTask.WaitAsync(TimeSpan.FromSeconds(5));
                }
                catch (TimeoutException)
                {
                    _logger.LogWarning("Message processing task did not complete within timeout");
                }
            }

            await CleanupAsync();

            _logger.LogInformation("IPC communication stopped");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error stopping IPC communication");
        }
    }

    /// <summary>
    /// Test IPC connection health
    /// </summary>
    public bool TestConnection()
    {
        try
        {
            if (!_isConnected)
            {
                return false;
            }

            // Test shared memory access
            if (_memoryAccessor != null)
            {
                try
                {
                    // Try to read the header magic number
                    var magic = _memoryAccessor.ReadUInt32(0);
                    if (magic == 0x524D5348) // 'RMSH'
                    {
                        return true;
                    }
                }
                catch
                {
                    // Ignore access errors
                }
            }

            // Test named pipe connection
            if (_namedPipe != null && _namedPipe.IsConnected)
            {
                return true;
            }

            return false;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error testing IPC connection");
            return false;
        }
    }

    #endregion

    #region Private Implementation

    private async Task<bool> InitializeSharedMemory()
    {
        try
        {
            _logger.LogDebug($"Opening shared memory: {_sharedMemoryName}");

            // Open existing shared memory created by C++ side
            _sharedMemory = MemoryMappedFile.OpenExisting(_sharedMemoryName);
            _memoryAccessor = _sharedMemory.CreateViewAccessor();

            // Open synchronization objects
            _commandEvent = EventWaitHandle.OpenExisting($"{_sharedMemoryName}_Command");
            _resultEvent = EventWaitHandle.OpenExisting($"{_sharedMemoryName}_Result");
            _accessMutex = Mutex.OpenExisting($"{_sharedMemoryName}_Mutex");

            _logger.LogDebug("Shared memory initialized successfully");
            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to initialize shared memory");
            return false;
        }
    }

    private async Task<bool> InitializeNamedPipe()
    {
        try
        {
            _logger.LogDebug($"Connecting to named pipe: {_namedPipeName}");

            _namedPipe = new NamedPipeClientStream(".", _namedPipeName, PipeDirection.InOut, PipeOptions.Asynchronous);
            
            // Try to connect with timeout
            await _namedPipe.ConnectAsync(5000);

            _logger.LogDebug("Named pipe connected successfully");
            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to connect to named pipe");
            return false;
        }
    }

    private async Task ProcessMessagesAsync(CancellationToken cancellationToken)
    {
        _logger.LogDebug("Message processing task started");

        try
        {
            while (!cancellationToken.IsCancellationRequested && _isConnected)
            {
                try
                {
                    // Wait for commands with timeout
                    await _commandSemaphore.WaitAsync(1000, cancellationToken);

                    // Process queued commands
                    while (_commandQueue.TryDequeue(out var command))
                    {
                        await HandleCommand(command);
                    }
                }
                catch (OperationCanceledException)
                {
                    break;
                }
                catch (Exception ex)
                {
                    _logger.LogError(ex, "Error in message processing task");
                    await Task.Delay(1000, cancellationToken);
                }
            }
        }
        catch (OperationCanceledException)
        {
            // Expected when cancellation is requested
        }

        _logger.LogDebug("Message processing task stopped");
    }

    private async Task ProcessIncomingCommands()
    {
        try
        {
            // Try to read from shared memory first
            if (_memoryAccessor != null && await ReadCommandFromSharedMemory())
            {
                return;
            }

            // Try to read from named pipe
            if (_namedPipe != null && _namedPipe.IsConnected && await ReadCommandFromNamedPipe())
            {
                return;
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error processing incoming commands");
        }
    }

    private async Task<bool> ReadCommandFromSharedMemory()
    {
        try
        {
            if (_memoryAccessor == null || _accessMutex == null)
            {
                return false;
            }

            // Wait for exclusive access
            if (!_accessMutex.WaitOne(5000))
            {
                _logger.LogWarning("Timeout waiting for shared memory access");
                return false;
            }

            try
            {
                // Check if command is ready
                bool commandReady = _memoryAccessor.ReadBoolean(16); // Offset for commandReady field
                if (!commandReady)
                {
                    return false;
                }

                // Read command size
                long commandSize = _memoryAccessor.ReadInt64(8); // Offset for commandSize field
                if (commandSize <= 0 || commandSize > 1024 * 1024) // Max 1MB command
                {
                    _logger.LogError($"Invalid command size: {commandSize}");
                    return false;
                }

                // Read command data
                byte[] commandData = new byte[commandSize];
                _memoryAccessor.ReadArray(48, commandData, 0, (int)commandSize); // Offset after header

                // Deserialize command
                var command = DeserializeCommand(commandData);
                if (command != null)
                {
                    _commandQueue.Enqueue(command);
                    _commandSemaphore.Release();

                    // Mark command as processed
                    _memoryAccessor.Write(16, false); // Clear commandReady flag

                    return true;
                }
            }
            finally
            {
                _accessMutex.ReleaseMutex();
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error reading command from shared memory");
        }

        return false;
    }

    private async Task<bool> ReadCommandFromNamedPipe()
    {
        try
        {
            if (_namedPipe == null || !_namedPipe.IsConnected)
            {
                return false;
            }

            // Read message header
            byte[] headerBuffer = new byte[24]; // MessageHeader size
            int bytesRead = await _namedPipe.ReadAsync(headerBuffer, 0, headerBuffer.Length);
            
            if (bytesRead != headerBuffer.Length)
            {
                return false;
            }

            // Parse header
            uint magic = BitConverter.ToUInt32(headerBuffer, 0);
            uint messageType = BitConverter.ToUInt32(headerBuffer, 4);
            ulong dataSize = BitConverter.ToUInt64(headerBuffer, 12);

            if (magic != 0x52454E44 || messageType != 1) // 'REND' magic, Command type
            {
                _logger.LogError($"Invalid message header: magic={magic:X}, type={messageType}");
                return false;
            }

            if (dataSize > 1024 * 1024) // Max 1MB
            {
                _logger.LogError($"Command too large: {dataSize} bytes");
                return false;
            }

            // Read command data
            byte[] commandData = new byte[dataSize];
            bytesRead = await _namedPipe.ReadAsync(commandData, 0, (int)dataSize);

            if (bytesRead != (int)dataSize)
            {
                _logger.LogError($"Incomplete command read: expected {dataSize}, got {bytesRead}");
                return false;
            }

            // Deserialize command
            var command = DeserializeCommand(commandData);
            if (command != null)
            {
                _commandQueue.Enqueue(command);
                _commandSemaphore.Release();
                return true;
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error reading command from named pipe");
        }

        return false;
    }

    private async Task<bool> SendResultViaSharedMemory(RenderResult result)
    {
        try
        {
            if (_memoryAccessor == null || _accessMutex == null)
            {
                return false;
            }

            // Serialize result
            byte[] resultData = SerializeResult(result);
            if (resultData.Length > 1024 * 1024) // Max 1MB
            {
                _logger.LogError($"Result too large: {resultData.Length} bytes");
                return false;
            }

            // Wait for exclusive access
            if (!_accessMutex.WaitOne(5000))
            {
                _logger.LogWarning("Timeout waiting for shared memory access");
                return false;
            }

            try
            {
                // Write result size
                _memoryAccessor.Write(12, (long)resultData.Length); // Offset for resultSize

                // Write result data (assuming result buffer starts after command buffer)
                int resultBufferOffset = 48 + (2 * 1024 * 1024); // After header + half buffer size
                _memoryAccessor.WriteArray(resultBufferOffset, resultData, 0, resultData.Length);

                // Set result ready flag
                _memoryAccessor.Write(17, true); // Offset for resultReady field

                // Signal result ready
                _resultEvent?.Set();

                return true;
            }
            finally
            {
                _accessMutex.ReleaseMutex();
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error sending result via shared memory");
        }

        return false;
    }

    private async Task<bool> SendResultViaNamedPipe(RenderResult result)
    {
        try
        {
            if (_namedPipe == null || !_namedPipe.IsConnected)
            {
                return false;
            }

            // Serialize result
            byte[] resultData = SerializeResult(result);

            // Create message header
            byte[] header = new byte[24];
            BitConverter.GetBytes(0x52454E44u).CopyTo(header, 0); // Magic 'REND'
            BitConverter.GetBytes(2u).CopyTo(header, 4); // MessageType: Result
            BitConverter.GetBytes(result.CommandId).CopyTo(header, 8); // MessageId
            BitConverter.GetBytes((ulong)resultData.Length).CopyTo(header, 12); // DataSize
            BitConverter.GetBytes(0u).CopyTo(header, 20); // Checksum (simplified)

            // Send header
            await _namedPipe.WriteAsync(header, 0, header.Length);

            // Send result data
            await _namedPipe.WriteAsync(resultData, 0, resultData.Length);

            await _namedPipe.FlushAsync();

            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error sending result via named pipe");
        }

        return false;
    }

    private async Task HandleCommand(RenderCommand command)
    {
        try
        {
            _logger.LogDebug($"Handling command {command.CommandId}: {command.CommandType}");

            // Trigger command received event
            CommandReceived?.Invoke(this, new RenderCommandReceivedEventArgs(command));
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Error handling command {command.CommandId}");

            // Send error result back
            var errorResult = new RenderResult
            {
                CommandId = command.CommandId,
                WidgetId = command.WidgetId,
                Status = RenderResultStatus.Failure,
                ErrorMessage = $"Command handling failed: {ex.Message}",
                Timestamp = (ulong)DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()
            };

            await SendResultAsync(errorResult);
        }
    }

    private RenderCommand? DeserializeCommand(byte[] data)
    {
        try
        {
            // Simple binary deserialization - in production, consider using more robust serialization
            using var stream = new MemoryStream(data);
            using var reader = new BinaryReader(stream);

            var command = new RenderCommand
            {
                CommandId = reader.ReadUInt64(),
                CommandType = (RenderCommandType)reader.ReadUInt32(),
                WidgetId = reader.ReadUInt32(),
                WindowHandle = new IntPtr(reader.ReadInt64()),
                BackendType = (RenderBackendType)reader.ReadUInt32(),
                Timestamp = reader.ReadUInt64()
            };

            // Read bounds
            command.Bounds = new RenderRect
            {
                X = reader.ReadInt32(),
                Y = reader.ReadInt32(),
                Width = reader.ReadInt32(),
                Height = reader.ReadInt32()
            };

            // Read content parameters
            command.Content = new ContentParameters
            {
                SourceType = (ContentSourceType)reader.ReadUInt32(),
                SourceUrl = reader.ReadString(),
                TemplateData = reader.ReadString(),
                RefreshIntervalMs = reader.ReadInt32(),
                CacheEnabled = reader.ReadBoolean()
            };

            // Read render properties (simplified)
            command.Properties = new RenderProperties
            {
                Opacity = reader.ReadSingle(),
                Visible = reader.ReadBoolean(),
                ClickThrough = reader.ReadBoolean(),
                TopMost = reader.ReadBoolean()
            };

            return command;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to deserialize command");
            return null;
        }
    }

    private byte[] SerializeResult(RenderResult result)
    {
        try
        {
            using var stream = new MemoryStream();
            using var writer = new BinaryWriter(stream);

            writer.Write(result.CommandId);
            writer.Write(result.WidgetId);
            writer.Write((uint)result.Status);
            writer.Write(result.ErrorMessage ?? string.Empty);
            writer.Write(result.RenderTimeMs);
            writer.Write(result.FrameCount);
            writer.Write(result.AverageFps);
            writer.Write(result.MemoryUsageMB);
            writer.Write(result.Timestamp);

            return stream.ToArray();
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to serialize result");
            return Array.Empty<byte>();
        }
    }

    private async Task CleanupAsync()
    {
        lock (_disposeLock)
        {
            try
            {
                _commandEvent?.Dispose();
                _commandEvent = null;

                _resultEvent?.Dispose();
                _resultEvent = null;

                _accessMutex?.Dispose();
                _accessMutex = null;

                _memoryAccessor?.Dispose();
                _memoryAccessor = null;

                _sharedMemory?.Dispose();
                _sharedMemory = null;

                _namedPipe?.Dispose();
                _namedPipe = null;

                _cancellationTokenSource?.Dispose();
                _cancellationTokenSource = null;
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error during cleanup");
            }
        }
    }

    #endregion

    #region IDisposable

    public void Dispose()
    {
        if (_isDisposed)
            return;

        StopAsync().Wait(5000);
        _isDisposed = true;
    }

    #endregion
}

#region Event Args Classes

public class RenderCommandReceivedEventArgs : EventArgs
{
    public RenderCommand Command { get; }

    public RenderCommandReceivedEventArgs(RenderCommand command)
    {
        Command = command;
    }
}

public class IPCErrorEventArgs : EventArgs
{
    public string ErrorMessage { get; }
    public Exception? Exception { get; }

    public IPCErrorEventArgs(string errorMessage, Exception? exception = null)
    {
        ErrorMessage = errorMessage;
        Exception = exception;
    }
}

public class ConnectionStatusEventArgs : EventArgs
{
    public bool IsConnected { get; }

    public ConnectionStatusEventArgs(bool isConnected)
    {
        IsConnected = isConnected;
    }
}

#endregion
