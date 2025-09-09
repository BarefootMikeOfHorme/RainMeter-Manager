#pragma once

#include "../interfaces/render_command.h"
#include "../../core/logger.h"
#include <windows.h>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace RainmeterManager::Render {

    /**
     * @brief Reliable named pipe communication for IPC
     * 
     * This class provides robust named pipe communication between
     * the C++ core and C# rendering process with automatic
     * reconnection, buffering, and error recovery.
     */
    class NamedPipeChannel {
    public:
        explicit NamedPipeChannel(const std::wstring& pipeName);
        ~NamedPipeChannel();

        // === Server Operations (C++ Core) ===
        
        /**
         * @brief Create named pipe server
         * @param maxInstances Maximum concurrent connections
         * @param bufferSize Pipe buffer size in bytes
         * @return True if server creation succeeded
         */
        bool CreateServer(DWORD maxInstances = 1, DWORD bufferSize = 65536);
        
        /**
         * @brief Wait for client connection
         * @param timeoutMs Connection timeout in milliseconds
         * @return True if client connected
         */
        bool WaitForConnection(DWORD timeoutMs = INFINITE);
        
        /**
         * @brief Disconnect current client
         */
        void DisconnectClient();

        // === Client Operations (C# Process) ===
        
        /**
         * @brief Connect as client to server
         * @param timeoutMs Connection timeout in milliseconds
         * @param retryAttempts Number of retry attempts
         * @return True if connection succeeded
         */
        bool ConnectAsClient(DWORD timeoutMs = 5000, int retryAttempts = 3);

        // === Communication ===
        
        /**
         * @brief Send command through pipe
         * @param command Command to send
         * @return True if send succeeded
         */
        bool SendCommand(const RenderCommand& command);
        
        /**
         * @brief Receive result from pipe
         * @param result Output result structure
         * @param timeoutMs Receive timeout
         * @return True if receive succeeded
         */
        bool ReceiveResult(RenderResult& result, DWORD timeoutMs = 5000);
        
        /**
         * @brief Send result through pipe (for C# process)
         * @param result Result to send
         * @return True if send succeeded
         */
        bool SendResult(const RenderResult& result);
        
        /**
         * @brief Receive command from pipe (for C# process)
         * @param command Output command structure
         * @param timeoutMs Receive timeout
         * @return True if receive succeeded
         */
        bool ReceiveCommand(RenderCommand& command, DWORD timeoutMs = INFINITE);

        // === Status & Health ===
        
        /**
         * @brief Check if pipe is connected
         * @return True if pipe is connected and operational
         */
        bool IsConnected() const;
        
        /**
         * @brief Check if operating as server
         * @return True if this is the server instance
         */
        bool IsServer() const;
        
        /**
         * @brief Get last error message
         * @return Last error description
         */
        std::string GetLastError() const;
        
        /**
         * @brief Test pipe connection health
         * @return True if pipe is healthy and responsive
         */
        bool TestConnection();

        // === Advanced Features ===
        
        /**
         * @brief Enable asynchronous message processing
         * @param callback Callback for received messages
         * @return True if async mode enabled
         */
        bool EnableAsyncMode(std::function<void(const RenderResult&)> callback = nullptr);
        
        /**
         * @brief Disable asynchronous processing
         */
        void DisableAsyncMode();
        
        /**
         * @brief Set pipe security attributes
         * @param securityDescriptor Security descriptor string
         * @return True if security set successfully
         */
        bool SetPipeSecurity(const std::wstring& securityDescriptor);

        // === Statistics ===
        
        struct PipeStatistics {
            uint64_t bytesSent = 0;
            uint64_t bytesReceived = 0;
            uint64_t messagesSent = 0;
            uint64_t messagesReceived = 0;
            uint64_t connectionAttempts = 0;
            uint64_t disconnectionEvents = 0;
            uint64_t errorCount = 0;
            uint64_t timeoutCount = 0;
            std::chrono::steady_clock::time_point lastActivity;
            double averageLatencyMs = 0.0;
        };
        
        /**
         * @brief Get pipe communication statistics
         * @return Statistics structure
         */
        PipeStatistics GetStatistics() const;
        
        /**
         * @brief Reset statistics counters
         */
        void ResetStatistics();

        // === Cleanup ===
        
        /**
         * @brief Disconnect and cleanup all resources
         */
        void Cleanup();

    private:
        // === Internal Structures ===
        
        struct MessageHeader {
            uint32_t magic;        // Magic number for validation
            uint32_t messageType;  // Message type identifier
            uint64_t messageId;    // Unique message identifier  
            uint64_t dataSize;     // Size of following data
            uint32_t checksum;     // Data integrity checksum
            uint64_t timestamp;    // Message timestamp
        };
        
        enum class MessageType : uint32_t {
            Command = 1,
            Result = 2,
            Heartbeat = 3,
            Acknowledge = 4
        };

        // === Member Variables ===
        std::wstring pipeName_;
        std::wstring fullPipeName_;
        HANDLE pipeHandle_;
        OVERLAPPED overlapped_;
        
        // State management
        std::atomic<bool> isConnected_{false};
        std::atomic<bool> isServer_{false};
        std::atomic<bool> asyncModeEnabled_{false};
        std::atomic<bool> shouldStop_{false};
        
        // Threading
        std::thread asyncReceiveThread_;
        std::mutex pipeMutex_;
        std::condition_variable pipeCondition_;
        
        // Message handling
        std::function<void(const RenderResult&)> asyncCallback_;
        std::queue<std::vector<uint8_t>> messageQueue_;
        std::mutex queueMutex_;
        
        // Statistics
        mutable std::mutex statsMutex_;
        PipeStatistics stats_;
        
        // Error handling
        mutable std::string lastError_;
        
        // Constants
        static constexpr uint32_t MESSAGE_MAGIC = 0x52454E44; // 'REND'
        static constexpr DWORD DEFAULT_BUFFER_SIZE = 65536;
        static constexpr DWORD MAX_MESSAGE_SIZE = 16 * 1024 * 1024; // 16MB
        static constexpr int DEFAULT_RETRY_ATTEMPTS = 3;
        static constexpr DWORD HEARTBEAT_INTERVAL_MS = 5000;

        // === Internal Methods ===
        bool WriteMessage(MessageType type, const void* data, size_t dataSize);
        bool ReadMessage(MessageType expectedType, void* data, size_t maxDataSize, 
                        size_t& actualDataSize, DWORD timeoutMs = 5000);
        bool WriteData(const void* data, DWORD dataSize, DWORD timeoutMs = 5000);
        bool ReadData(void* data, DWORD dataSize, DWORD timeoutMs = 5000);
        bool FlushPipe();
        void AsyncReceiveLoop();
        uint32_t CalculateChecksum(const void* data, size_t size);
        std::wstring GenerateFullPipeName() const;
        void SetLastError(const std::string& error) const;
        void UpdateStatistics(bool isSend, size_t bytes);
        bool SendHeartbeat();
        bool HandleHeartbeat();
        
        // Serialization helpers
        size_t SerializeCommand(const RenderCommand& command, std::vector<uint8_t>& buffer);
        bool DeserializeCommand(const std::vector<uint8_t>& buffer, RenderCommand& command);
        size_t SerializeResult(const RenderResult& result, std::vector<uint8_t>& buffer);
        bool DeserializeResult(const std::vector<uint8_t>& buffer, RenderResult& result);

        // Non-copyable
        NamedPipeChannel(const NamedPipeChannel&) = delete;
        NamedPipeChannel& operator=(const NamedPipeChannel&) = delete;
    };

    /**
     * @brief Named pipe channel factory function
     * @param pipeName Name of the pipe (without \\\\.\\pipe\\ prefix)
     * @return Unique pointer to named pipe channel
     */
    std::unique_ptr<NamedPipeChannel> CreateNamedPipeChannel(const std::wstring& pipeName);

} // namespace RainmeterManager::Render
