// ipc_manager.cpp - Windows Named Pipes IPC Manager Implementation
#include "ipc_manager.h"
#include "../core/logger.h"
#include <sstream>
#include <iomanip>
#include <random>

namespace RainmeterManager {
namespace IPC {

// Constructor
IPCManager::IPCManager()
    : running_(false)
    , stopRequested_(false)
    , clientPipe_(INVALID_HANDLE_VALUE)
    , clientConnected_(false)
    , stats_{0, 0, 0, 0, 0}
{
    LOG_INFO("IPCManager created");
}

// Destructor
IPCManager::~IPCManager() {
    Stop();
}

// Start IPC manager
bool IPCManager::Start(const IPCConfig& config) {
    if (running_) {
        LOG_WARNING("IPCManager already running");
        return false;
    }
    
    config_ = config;
    mode_ = config.mode;
    running_ = true;
    stopRequested_ = false;
    
    LOG_INFO("Starting IPCManager in " + std::string(mode_ == IPCMode::Server ? "Server" : "Client") + " mode...");
    
    if (mode_ == IPCMode::Server) {
        // Server mode: start listener thread
        listenerThread_ = std::make_unique<std::thread>(&IPCManager::ServerListenerLoop, this);
    } else {
        // Client mode: start connection thread
        listenerThread_ = std::make_unique<std::thread>(&IPCManager::ClientConnectionLoop, this);
    }
    
    // Start sender thread
    senderThread_ = std::make_unique<std::thread>(&IPCManager::SenderLoop, this);
    
    LOG_INFO("IPCManager started successfully");
    return true;
}

// Stop IPC manager
void IPCManager::Stop() {
    if (!running_) {
        return;
    }
    
    LOG_INFO("Stopping IPCManager...");
    
    stopRequested_ = true;
    sendQueueCV_.notify_all();
    
    // Join threads
    if (listenerThread_ && listenerThread_->joinable()) {
        listenerThread_->join();
    }
    if (senderThread_ && senderThread_->joinable()) {
        senderThread_->join();
    }
    for (auto& thread : connectionThreads_) {
        if (thread && thread->joinable()) {
            thread->join();
        }
    }
    connectionThreads_.clear();
    
    // Close connections
    if (mode_ == IPCMode::Server) {
        std::lock_guard<std::mutex> lock(connectionsMutex_);
        for (auto& conn : connections_) {
            CloseConnection(conn);
        }
        connections_.clear();
    } else {
        if (clientPipe_ != INVALID_HANDLE_VALUE) {
            CloseHandle(clientPipe_);
            clientPipe_ = INVALID_HANDLE_VALUE;
        }
        clientConnected_ = false;
    }
    
    running_ = false;
    LOG_INFO("IPCManager stopped");
}

bool IPCManager::IsRunning() const {
    return running_;
}

// Send message (uses default connection)
bool IPCManager::Send(const std::string& channel, const std::string& payload) {
    if (!running_) {
        return false;
    }
    
    IPCMessage msg;
    msg.channel = channel;
    msg.type = "notification";
    msg.messageId = GenerateMessageId();
    msg.payload = payload;
    msg.timestamp = GetTimestamp();
    
    std::lock_guard<std::mutex> lock(sendQueueMutex_);
    sendQueue_.push(msg);
    sendQueueCV_.notify_one();
    
    return true;
}

// Send message to specific process
bool IPCManager::SendTo(DWORD targetProcessId, const std::string& channel, const std::string& payload) {
    if (!running_ || mode_ != IPCMode::Server) {
        return false;
    }
    
    IPCMessage msg;
    msg.channel = channel;
    msg.type = "notification";
    msg.messageId = GenerateMessageId();
    msg.payload = payload;
    msg.timestamp = GetTimestamp();
    
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    for (auto& conn : connections_) {
        if (conn.active && conn.processId == targetProcessId) {
            return SendMessageInternal(conn.pipeHandle, msg);
        }
    }
    
    LOG_WARNING("SendTo: Process " + std::to_string(targetProcessId) + " not found");
    return false;
}

// Broadcast to all connections
bool IPCManager::Broadcast(const std::string& channel, const std::string& payload) {
    if (!running_ || mode_ != IPCMode::Server) {
        return false;
    }
    
    IPCMessage msg;
    msg.channel = channel;
    msg.type = "notification";
    msg.messageId = GenerateMessageId();
    msg.payload = payload;
    msg.timestamp = GetTimestamp();
    
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    bool anySuccess = false;
    
    for (auto& conn : connections_) {
        if (conn.active) {
            if (SendMessageInternal(conn.pipeHandle, msg)) {
                anySuccess = true;
            }
        }
    }
    
    return anySuccess;
}

// Send request and wait for response
std::string IPCManager::SendRequest(const std::string& channel, const std::string& payload, int timeoutMs) {
    if (!running_) {
        return "";
    }
    
    IPCMessage msg;
    msg.channel = channel;
    msg.type = "request";
    msg.messageId = GenerateMessageId();
    msg.payload = payload;
    msg.timestamp = GetTimestamp();
    
    // Create pending request
    auto pending = std::make_shared<PendingRequest>();
    pending->completed = false;
    
    {
        std::lock_guard<std::mutex> lock(requestsMutex_);
        pendingRequests_[msg.messageId] = pending;
    }
    
    // Send message
    {
        std::lock_guard<std::mutex> lock(sendQueueMutex_);
        sendQueue_.push(msg);
        sendQueueCV_.notify_one();
    }
    
    // Wait for response
    std::unique_lock<std::mutex> lock(pending->mutex);
    if (pending->cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&]{ return pending->completed; })) {
        std::string response = pending->response;
        
        std::lock_guard<std::mutex> reqLock(requestsMutex_);
        pendingRequests_.erase(msg.messageId);
        
        return response;
    } else {
        LOG_WARNING("Request timeout: " + msg.messageId);
        
        std::lock_guard<std::mutex> reqLock(requestsMutex_);
        pendingRequests_.erase(msg.messageId);
        
        return "";
    }
}

// Send response to request
void IPCManager::SendResponse(const std::string& messageId, const std::string& payload) {
    if (!running_) {
        return;
    }
    
    IPCMessage msg;
    msg.channel = "response";
    msg.type = "response";
    msg.messageId = messageId;
    msg.payload = payload;
    msg.timestamp = GetTimestamp();
    
    std::lock_guard<std::mutex> lock(sendQueueMutex_);
    sendQueue_.push(msg);
    sendQueueCV_.notify_one();
}

// Register message handler for channel
void IPCManager::OnMessage(const std::string& channel, MessageHandler handler) {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    messageHandlers_[channel] = std::move(handler);
    LOG_INFO("Registered message handler for channel: " + channel);
}

// Register connection handler
void IPCManager::OnConnection(ConnectionHandler handler) {
    connectionHandler_ = std::move(handler);
}

// Register error handler
void IPCManager::OnError(ErrorHandler handler) {
    errorHandler_ = std::move(handler);
}

// Get connected processes
std::vector<DWORD> IPCManager::GetConnectedProcesses() const {
    std::vector<DWORD> processes;
    
    if (mode_ == IPCMode::Server) {
        std::lock_guard<std::mutex> lock(connectionsMutex_);
        for (const auto& conn : connections_) {
            if (conn.active) {
                processes.push_back(conn.processId);
            }
        }
    }
    
    return processes;
}

// Check if process is connected
bool IPCManager::IsConnected(DWORD processId) const {
    if (mode_ != IPCMode::Server) {
        return clientConnected_;
    }
    
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    for (const auto& conn : connections_) {
        if (conn.active && conn.processId == processId) {
            return true;
        }
    }
    
    return false;
}

// Disconnect process
void IPCManager::DisconnectProcess(DWORD processId) {
    if (mode_ != IPCMode::Server) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    for (auto& conn : connections_) {
        if (conn.active && conn.processId == processId) {
            CloseConnection(conn);
            break;
        }
    }
}

// Get statistics
IPCManager::Statistics IPCManager::GetStatistics() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_;
}

// Server listener loop
void IPCManager::ServerListenerLoop() {
    LOG_INFO("Server listener thread started");
    
    while (!stopRequested_) {
        // Create named pipe instance
        HANDLE pipeHandle = CreateNamedPipeA(
            config_.pipeName.c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            config_.maxConnections,
            config_.bufferSize,
            config_.bufferSize,
            config_.timeoutMs,
            nullptr
        );
        
        if (pipeHandle == INVALID_HANDLE_VALUE) {
            NotifyError("Failed to create named pipe", GetLastError());
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }
        
        // Wait for client connection
        if (ConnectNamedPipe(pipeHandle, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED) {
            DWORD clientProcessId = 0;
            GetNamedPipeClientProcessId(pipeHandle, &clientProcessId);
            
            LOG_INFO("Client connected: PID " + std::to_string(clientProcessId));
            
            // Store connection
            {
                std::lock_guard<std::mutex> lock(connectionsMutex_);
                ConnectionInfo conn;
                conn.pipeHandle = pipeHandle;
                conn.processId = clientProcessId;
                conn.active = true;
                conn.lastActivity = std::chrono::steady_clock::now();
                connections_.push_back(conn);
            }
            
            // Notify connection handler
            if (connectionHandler_) {
                connectionHandler_(clientProcessId, true);
            }
            
            // Handle connection in separate thread
            connectionThreads_.push_back(std::make_unique<std::thread>(
                &IPCManager::HandleConnection, this, pipeHandle, clientProcessId
            ));
        } else {
            CloseHandle(pipeHandle);
        }
        
        // Clean up finished connection threads
        connectionThreads_.erase(
            std::remove_if(connectionThreads_.begin(), connectionThreads_.end(),
                [](const std::unique_ptr<std::thread>& t) {
                    return !t || !t->joinable();
                }),
            connectionThreads_.end()
        );
    }
    
    LOG_INFO("Server listener thread stopped");
}

// Handle individual connection
void IPCManager::HandleConnection(HANDLE pipeHandle, DWORD processId) {
    LOG_INFO("Handling connection for PID " + std::to_string(processId));
    
    while (!stopRequested_) {
        IPCMessage message;
        if (ReceiveMessage(pipeHandle, message)) {
            ProcessIncomingMessage(message);
        } else {
            // Connection lost
            LOG_WARNING("Connection lost with PID " + std::to_string(processId));
            break;
        }
    }
    
    // Mark connection as inactive
    {
        std::lock_guard<std::mutex> lock(connectionsMutex_);
        for (auto& conn : connections_) {
            if (conn.processId == processId && conn.pipeHandle == pipeHandle) {
                CloseConnection(conn);
                break;
            }
        }
    }
    
    // Notify disconnection
    if (connectionHandler_) {
        connectionHandler_(processId, false);
    }
}

// Client connection loop
void IPCManager::ClientConnectionLoop() {
    LOG_INFO("Client connection thread started");
    
    int reconnectAttempts = 0;
    
    while (!stopRequested_) {
        if (ConnectToServer()) {
            reconnectAttempts = 0;
            clientConnected_ = true;
            
            if (connectionHandler_) {
                connectionHandler_(GetCurrentProcessId(), true);
            }
            
            // Start receiving messages
            ClientReceiveLoop();
            
            clientConnected_ = false;
            
            if (connectionHandler_) {
                connectionHandler_(GetCurrentProcessId(), false);
            }
        } else {
            reconnectAttempts++;
            
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats_.connectionsFailed++;
            
            if (!config_.enableReconnect || reconnectAttempts >= config_.maxReconnectAttempts) {
                LOG_ERROR("Failed to connect after " + std::to_string(reconnectAttempts) + " attempts");
                break;
            }
            
            LOG_WARNING("Reconnect attempt " + std::to_string(reconnectAttempts) + " in " + 
                       std::to_string(config_.reconnectDelayMs) + "ms");
            std::this_thread::sleep_for(std::chrono::milliseconds(config_.reconnectDelayMs));
            
            stats_.reconnectAttempts++;
        }
    }
    
    LOG_INFO("Client connection thread stopped");
}

// Connect to server
bool IPCManager::ConnectToServer() {
    LOG_INFO("Connecting to server: " + config_.pipeName);
    
    clientPipe_ = CreateFileA(
        config_.pipeName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );
    
    if (clientPipe_ == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error != ERROR_PIPE_BUSY) {
            NotifyError("Failed to connect to server", error);
            return false;
        }
        
        // Wait for pipe to become available
        if (!WaitNamedPipeA(config_.pipeName.c_str(), config_.timeoutMs)) {
            NotifyError("Timeout waiting for pipe", GetLastError());
            return false;
        }
        
        return ConnectToServer();  // Retry
    }
    
    // Set pipe mode
    DWORD mode = PIPE_READMODE_MESSAGE;
    if (!SetNamedPipeHandleState(clientPipe_, &mode, nullptr, nullptr)) {
        NotifyError("Failed to set pipe mode", GetLastError());
        CloseHandle(clientPipe_);
        clientPipe_ = INVALID_HANDLE_VALUE;
        return false;
    }
    
    LOG_INFO("Connected to server successfully");
    return true;
}

// Client receive loop
void IPCManager::ClientReceiveLoop() {
    LOG_INFO("Client receive loop started");
    
    while (!stopRequested_ && clientPipe_ != INVALID_HANDLE_VALUE) {
        IPCMessage message;
        if (ReceiveMessage(clientPipe_, message)) {
            ProcessIncomingMessage(message);
        } else {
            LOG_WARNING("Connection to server lost");
            break;
        }
    }
    
    if (clientPipe_ != INVALID_HANDLE_VALUE) {
        CloseHandle(clientPipe_);
        clientPipe_ = INVALID_HANDLE_VALUE;
    }
    
    LOG_INFO("Client receive loop stopped");
}

// Sender loop
void IPCManager::SenderLoop() {
    LOG_INFO("Sender thread started");
    
    while (!stopRequested_) {
        std::unique_lock<std::mutex> lock(sendQueueMutex_);
        sendQueueCV_.wait(lock, [this]{ return !sendQueue_.empty() || stopRequested_; });
        
        if (stopRequested_) break;
        
        if (!sendQueue_.empty()) {
            IPCMessage msg = sendQueue_.front();
            sendQueue_.pop();
            lock.unlock();
            
            // Send message
            if (mode_ == IPCMode::Server) {
                // Broadcast to all active connections
                std::lock_guard<std::mutex> connLock(connectionsMutex_);
                for (auto& conn : connections_) {
                    if (conn.active) {
                        SendMessageInternal(conn.pipeHandle, msg);
                    }
                }
            } else {
                // Client: send to server
                if (clientConnected_ && clientPipe_ != INVALID_HANDLE_VALUE) {
                    SendMessageInternal(clientPipe_, msg);
                }
            }
        }
    }
    
    LOG_INFO("Sender thread stopped");
}

// Send message over pipe
bool IPCManager::SendMessageInternal(HANDLE pipe, const IPCMessage& message) {
    std::string serialized = SerializeMessage(message);
    DWORD bytesWritten = 0;
    
    bool success = WriteFile(
        pipe,
        serialized.c_str(),
        static_cast<DWORD>(serialized.length()),
        &bytesWritten,
        nullptr
    );
    
    if (success) {
        std::lock_guard<std::mutex> lock(statsMutex_);
        stats_.messagesSent++;
        stats_.bytesTransferred += bytesWritten;
    } else {
        NotifyError("Failed to send message", GetLastError());
    }
    
    return success;
}

// Receive message from pipe
bool IPCManager::ReceiveMessage(HANDLE pipe, IPCMessage& message) {
    char buffer[65536];
    DWORD bytesRead = 0;
    
    bool success = ReadFile(
        pipe,
        buffer,
        sizeof(buffer) - 1,
        &bytesRead,
        nullptr
    );
    
    if (!success || bytesRead == 0) {
        return false;
    }
    
    buffer[bytesRead] = '\0';
    
    try {
        message = DeserializeMessage(std::string(buffer, bytesRead));
        
        std::lock_guard<std::mutex> lock(statsMutex_);
        stats_.messagesReceived++;
        stats_.bytesTransferred += bytesRead;
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Failed to deserialize message: ") + e.what());
        return false;
    }
}

// Process incoming message
void IPCManager::ProcessIncomingMessage(const IPCMessage& message) {
    if (message.type == "response") {
        // Handle response to request
        std::lock_guard<std::mutex> lock(requestsMutex_);
        auto it = pendingRequests_.find(message.messageId);
        if (it != pendingRequests_.end()) {
            std::lock_guard<std::mutex> reqLock(it->second->mutex);
            it->second->response = message.payload;
            it->second->completed = true;
            it->second->cv.notify_one();
        }
    } else {
        // Route to channel handler
        std::lock_guard<std::mutex> lock(handlersMutex_);
        auto it = messageHandlers_.find(message.channel);
        if (it != messageHandlers_.end() && it->second) {
            try {
                it->second(message);
            } catch (const std::exception& e) {
                LOG_ERROR(std::string("Message handler exception: ") + e.what());
            }
        }
    }
}

// Serialize message to JSON string
std::string IPCManager::SerializeMessage(const IPCMessage& message) const {
    std::ostringstream json;
    json << "{";
    json << R"("channel":"" << message.channel << "",";
    json << R"("type":"" << message.type << "",";
    json << R"("messageId":"" << message.messageId << "",";
    json << R"("payload":"" << message.payload << "",";
    json << R"("timestamp":)" << message.timestamp;
    json << "}";
    return json.str();
}

// Deserialize JSON string to message
IPCMessage IPCManager::DeserializeMessage(const std::string& data) const {
    // Simple JSON parsing (for production, use a proper JSON library)
    IPCMessage msg;
    
    auto extractValue = [&data](const std::string& key) -> std::string {
        std::string searchKey = "\"" + key + "\":";
        size_t pos = data.find(searchKey);
        if (pos == std::string::npos) return "";
        
        pos += searchKey.length();
        if (data[pos] == '"') {
            pos++;
            size_t endPos = data.find('"', pos);
            return data.substr(pos, endPos - pos);
        } else {
            size_t endPos = data.find_first_of(",}", pos);
            return data.substr(pos, endPos - pos);
        }
    };
    
    msg.channel = extractValue("channel");
    msg.type = extractValue("type");
    msg.messageId = extractValue("messageId");
    msg.payload = extractValue("payload");
    
    std::string timestampStr = extractValue("timestamp");
    if (!timestampStr.empty()) {
        msg.timestamp = std::stoull(timestampStr);
    }
    
    return msg;
}

// Generate unique message ID
std::string IPCManager::GenerateMessageId() const {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    
    std::ostringstream oss;
    oss << "msg_" << std::hex << dis(gen);
    return oss.str();
}

// Get current timestamp
uint64_t IPCManager::GetTimestamp() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

// Notify error
void IPCManager::NotifyError(const std::string& error, DWORD errorCode) {
    LOG_ERROR(error + " (Error code: " + std::to_string(errorCode) + ")");
    
    if (errorHandler_) {
        try {
            errorHandler_(error, errorCode);
        } catch (const std::exception& e) {
            LOG_ERROR(std::string("Error handler exception: ") + e.what());
        }
    }
}

// Close connection
void IPCManager::CloseConnection(ConnectionInfo& conn) {
    if (conn.pipeHandle != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(conn.pipeHandle);
        DisconnectNamedPipe(conn.pipeHandle);
        CloseHandle(conn.pipeHandle);
        conn.pipeHandle = INVALID_HANDLE_VALUE;
    }
    conn.active = false;
}

} // namespace IPC
} // namespace RainmeterManager
