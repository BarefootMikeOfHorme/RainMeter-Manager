// ipc_manager.h - Windows Named Pipes IPC Manager
#pragma once

#include <functional>
#include <string>
#include <mutex>
#include <memory>
#include <thread>
#include <unordered_map>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <windows.h>

namespace RainmeterManager {
namespace IPC {

// IPC message structure
struct IPCMessage {
    std::string channel;
    std::string type;         // "request", "response", "notification"
    std::string messageId;    // For request-response tracking
    std::string payload;      // JSON serialized data
    uint64_t timestamp;
};

// IPC connection info
struct ConnectionInfo {
    HANDLE pipeHandle;
    std::string processName;
    DWORD processId;
    bool active;
    std::chrono::steady_clock::time_point lastActivity;
};

// IPC operation mode
enum class IPCMode {
    Server,   // Listen for connections (main process)
    Client    // Connect to server (render process)
};

// IPC configuration
struct IPCConfig {
    std::string pipeName = "\\\\.\\pipe\\RainmeterManager";
    IPCMode mode = IPCMode::Server;
    int maxConnections = 10;
    int bufferSize = 65536;  // 64KB buffer
    int timeoutMs = 5000;
    bool enableReconnect = true;
    int reconnectDelayMs = 1000;
    int maxReconnectAttempts = 5;
};

/**
 * @brief Windows Named Pipes IPC Manager
 * 
 * Features:
 * - Named pipe communication (server/client modes)
 * - JSON message serialization
 * - Async I/O with overlapped operations
 * - Multiple concurrent connections
 * - Message queuing and batching
 * - Auto-reconnection on failure
 * - Channel-based routing
 * - Request-response pattern support
 */
class IPCManager {
public:
    using MessageHandler = std::function<void(const IPCMessage&)>;
    using ConnectionHandler = std::function<void(DWORD processId, bool connected)>;
    using ErrorHandler = std::function<void(const std::string&, DWORD errorCode)>;

    IPCManager();
    ~IPCManager();

    // Lifecycle
    bool Start(const IPCConfig& config);
    void Stop();
    bool IsRunning() const;

    // Messaging
    bool Send(const std::string& channel, const std::string& payload);
    bool SendTo(DWORD targetProcessId, const std::string& channel, const std::string& payload);
    bool Broadcast(const std::string& channel, const std::string& payload);
    
    // Request-response pattern
    std::string SendRequest(const std::string& channel, const std::string& payload, int timeoutMs = 5000);
    void SendResponse(const std::string& messageId, const std::string& payload);

    // Handlers
    void OnMessage(const std::string& channel, MessageHandler handler);
    void OnConnection(ConnectionHandler handler);
    void OnError(ErrorHandler handler);

    // Connection management
    std::vector<DWORD> GetConnectedProcesses() const;
    bool IsConnected(DWORD processId) const;
    void DisconnectProcess(DWORD processId);

    // Statistics
    struct Statistics {
        uint64_t messagesSent;
        uint64_t messagesReceived;
        uint64_t bytesTransferred;
        uint64_t connectionsFailed;
        uint64_t reconnectAttempts;
    };
    Statistics GetStatistics() const;

private:
    // Configuration
    IPCConfig config_;
    IPCMode mode_;
    std::atomic<bool> running_;
    std::atomic<bool> stopRequested_;

    // Server mode: multiple pipe instances
    std::vector<ConnectionInfo> connections_;
    mutable std::mutex connectionsMutex_;

    // Client mode: single pipe connection
    HANDLE clientPipe_;
    std::atomic<bool> clientConnected_;

    // Message handling
    std::unordered_map<std::string, MessageHandler> messageHandlers_;
    std::mutex handlersMutex_;
    ConnectionHandler connectionHandler_;
    ErrorHandler errorHandler_;

    // Message queues
    std::queue<IPCMessage> sendQueue_;
    std::mutex sendQueueMutex_;
    std::condition_variable sendQueueCV_;

    // Request-response tracking
    struct PendingRequest {
        std::string response;
        bool completed;
        std::mutex mutex;
        std::condition_variable cv;
    };
    std::unordered_map<std::string, std::shared_ptr<PendingRequest>> pendingRequests_;
    std::mutex requestsMutex_;

    // Worker threads
    std::unique_ptr<std::thread> listenerThread_;
    std::unique_ptr<std::thread> senderThread_;
    std::vector<std::unique_ptr<std::thread>> connectionThreads_;

    // Statistics
    Statistics stats_;
    mutable std::mutex statsMutex_;

    // Server mode functions
    void ServerListenerLoop();
    bool CreatePipeInstance();
    void HandleConnection(HANDLE pipeHandle, DWORD processId);
    
    // Client mode functions
    void ClientConnectionLoop();
    bool ConnectToServer();
    void ClientReceiveLoop();

    // Message processing
    void SenderLoop();
    bool SendMessageInternal(HANDLE pipe, const IPCMessage& message);
    bool ReceiveMessage(HANDLE pipe, IPCMessage& message);
    void ProcessIncomingMessage(const IPCMessage& message);
    
    // Serialization
    std::string SerializeMessage(const IPCMessage& message) const;
    IPCMessage DeserializeMessage(const std::string& data) const;
    
    // Utilities
    std::string GenerateMessageId() const;
    uint64_t GetTimestamp() const;
    void NotifyError(const std::string& error, DWORD errorCode);
    void CloseConnection(ConnectionInfo& conn);
};

} // namespace IPC
} // namespace RainmeterManager
