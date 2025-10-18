#pragma once

#include "../interfaces/render_command.h"
#include "../../core/logger.h"
#include "../../core/logger_adapter.h"
#include <windows.h>
#include <memory>
#include <functional>
#include <future>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <map>

namespace RainmeterManager::Render {

    // Forward declarations
    class SharedMemoryManager;
    class NamedPipeChannel;

    // IPC communication modes
    enum class IPCMode {
        SharedMemory,   // High-performance shared memory
        NamedPipes,     // Reliable named pipe communication
        Hybrid          // Combination of both for best performance
    };

    // IPC event callbacks
    using ProcessStartedCallback = std::function<void(DWORD processId)>;
    using ProcessExitedCallback = std::function<void(DWORD exitCode)>;
    using MessageReceivedCallback = std::function<void(const RenderResult&)>;

    /**
     * @brief Main IPC bridge for render process communication
     * 
     * This class manages the lifecycle of the C# rendering process,
     * handles IPC communication, and provides a reliable interface
     * for sending commands and receiving results.
     */
    class RenderIPCBridge {
    public:
        explicit RenderIPCBridge(IPCMode mode = IPCMode::Hybrid);
        ~RenderIPCBridge();

        // === Process Management ===

        /**
         * @brief Start the render process
         * @param renderProcessPath Path to RenderProcess.exe
         * @param arguments Additional command line arguments
         * @return True if process started successfully
         */
        bool StartRenderProcess(
            const std::wstring& renderProcessPath,
            const std::wstring& arguments = L""
        );

        /**
         * @brief Stop the render process gracefully
         * @param timeoutMs Timeout for graceful shutdown
         * @return True if process stopped gracefully
         */
        bool StopRenderProcess(uint32_t timeoutMs = 5000);

        /**
         * @brief Check if render process is running
         * @return True if process is alive and responsive
         */
        bool IsProcessAlive() const;

        /**
         * @brief Get render process ID
         * @return Process ID (0 if not running)
         */
        DWORD GetProcessId() const;

        // === IPC Communication ===

        /**
         * @brief Initialize IPC communication channels
         * @return True if initialization succeeded
         */
        bool InitializeIPC();

        /**
         * @brief Cleanup IPC resources
         */
        void CleanupIPC();

        /**
         * @brief Send command to render process
         * @param command Command to send
         * @return Future for the command result
         */
        std::future<RenderResult> SendCommandAsync(const RenderCommand& command);

        /**
         * @brief Send command synchronously
         * @param command Command to send
         * @param timeoutMs Timeout in milliseconds
         * @return Command result
         */
        RenderResult SendCommand(const RenderCommand& command, uint32_t timeoutMs = 5000);

        /**
         * @brief Send command without waiting for result
         * @param command Command to send
         * @return True if command was sent successfully
         */
        bool SendCommandFireAndForget(const RenderCommand& command);

        // === System Capabilities ===

        /**
         * @brief Query system rendering capabilities
         * @param timeoutMs Query timeout
         * @return System capabilities structure
         */
        SystemCapabilities QuerySystemCapabilities(uint32_t timeoutMs = 3000);

        /**
         * @brief Get available render backends
         * @return Vector of supported backend types
         */
        std::vector<RenderBackendType> GetSupportedBackends();

        // === Performance Monitoring ===

        /**
         * @brief Get IPC performance statistics
         */
        struct IPCStats {
            uint64_t totalCommandsSent = 0;
            uint64_t totalResultsReceived = 0;
            uint64_t averageRoundTripMs = 0;
            uint64_t failedCommands = 0;
            uint64_t timeoutCommands = 0;
            size_t queuedCommands = 0;
            uint64_t totalBytesTransferred = 0;
        };

        IPCStats GetIPCStatistics() const;

        /**
         * @brief Reset IPC statistics
         */
        void ResetIPCStatistics();

        // === Event Callbacks ===

        /**
         * @brief Set process event callbacks
         */
        void SetProcessStartedCallback(ProcessStartedCallback callback);
        void SetProcessExitedCallback(ProcessExitedCallback callback);
        void SetMessageReceivedCallback(MessageReceivedCallback callback);

        // === Configuration ===

        /**
         * @brief Set IPC communication mode
         * @param mode New IPC mode
         */
        void SetIPCMode(IPCMode mode);

        /**
         * @brief Get current IPC mode
         * @return Current IPC communication mode
         */
        IPCMode GetIPCMode() const;

        /**
         * @brief Set command timeout
         * @param timeoutMs Default timeout for commands
         */
        void SetDefaultTimeout(uint32_t timeoutMs);

        /**
         * @brief Get command timeout
         * @return Default timeout in milliseconds
         */
        uint32_t GetDefaultTimeout() const;

        // === Error Handling ===

        /**
         * @brief Get last error message
         * @return Last error description
         */
        std::string GetLastError() const;

        /**
         * @brief Check if IPC bridge is healthy
         * @return True if all systems are operational
         */
        bool IsHealthy() const;

        /**
         * @brief Attempt to recover from communication errors
         * @return True if recovery succeeded
         */
        bool AttemptRecovery();

    private:
        // === Internal Implementation ===

        struct PendingCommand {
            uint64_t commandId;
            RenderCommand command;
            std::promise<RenderResult> promise;
            std::chrono::steady_clock::time_point sentTime;
            uint32_t timeoutMs;
        };

        // Process management
        PROCESS_INFORMATION processInfo_;
        std::wstring renderProcessPath_;
        std::atomic<bool> processRunning_{false};

        // IPC components
        IPCMode ipcMode_;
        std::unique_ptr<SharedMemoryManager> sharedMemory_;
        std::unique_ptr<NamedPipeChannel> namedPipe_;
        
        // Communication threads
        std::thread messageReceiveThread_;
        std::thread processMonitorThread_;
        std::atomic<bool> shouldStop_{false};

        // Command queue management
        std::mutex pendingCommandsMutex_;
        std::map<uint64_t, std::unique_ptr<PendingCommand>> pendingCommands_;
        std::atomic<uint64_t> nextCommandId_{1};

        // Statistics
        mutable std::mutex statsMutex_;
        IPCStats stats_;

        // Event callbacks
        ProcessStartedCallback processStartedCallback_;
        ProcessExitedCallback processExitedCallback_;
        MessageReceivedCallback messageReceivedCallback_;

        // Logging
        Core::Logger& logger_;
        
        // Configuration
        uint32_t defaultTimeoutMs_{5000};
        mutable std::string lastError_;

        // === Internal Methods ===
        void MessageReceiveLoop();
        void ProcessMonitorLoop();
        void HandleReceivedMessage(const RenderResult& result);
        void TimeoutPendingCommands();
        bool StartProcessInternal(const std::wstring& path, const std::wstring& args);
        void CleanupProcess();
        uint64_t GenerateCommandId();
        void UpdateStats(const RenderCommand& command, const RenderResult& result, 
                        uint64_t roundTripMs);
        void SetLastError(const std::string& error) const;

        // Non-copyable
        RenderIPCBridge(const RenderIPCBridge&) = delete;
        RenderIPCBridge& operator=(const RenderIPCBridge&) = delete;
    };

} // namespace RainmeterManager::Render
