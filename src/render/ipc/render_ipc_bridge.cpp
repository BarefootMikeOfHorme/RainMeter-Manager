#include "render_ipc_bridge.h"
#include "shared_memory_manager.h"
#include "named_pipe_channel.h"
#include "../../core/logger.h"
#include "../../core/logger_adapter.h"
#include <sstream>
#include <chrono>
#include <algorithm>

using namespace RainmeterManager::Render;
// Note: using namespace Core conflicts with member usage

namespace {
    const wchar_t* RENDER_PROCESS_NAME = L"RenderProcess.exe";
    const wchar_t* SHARED_MEMORY_NAME = L"RainmeterRenderSharedMemory";
    const wchar_t* NAMED_PIPE_NAME = L"RainmeterRenderPipe";
    const size_t DEFAULT_SHARED_MEMORY_SIZE = 4 * 1024 * 1024; // 4MB
    const uint32_t PROCESS_STARTUP_TIMEOUT = 10000; // 10 seconds
    const uint32_t COMMAND_TIMEOUT_CHECK_INTERVAL = 100; // 100ms
}

RenderIPCBridge::RenderIPCBridge(IPCMode mode)
    : ipcMode_(mode)
    , logger_(RainmeterManager::Core::Logger::GetInstance())
{
    ZeroMemory(&processInfo_, sizeof(processInfo_));
    LOG_INFO("RenderIPCBridge: Created with IPC mode " + std::to_string(static_cast<int>(mode)));
}

RenderIPCBridge::~RenderIPCBridge()
{
    if (processRunning_) {
        StopRenderProcess(5000);
    }
    CleanupIPC();
}

// ===== PROCESS MANAGEMENT =====

bool RenderIPCBridge::StartRenderProcess(const std::wstring& renderProcessPath, const std::wstring& arguments)
{
    try {
        if (processRunning_) {
            logger_.LogWarning(L"RenderIPCBridge: Process is already running");
            return true;
        }

        logger_.LogInfo(L"RenderIPCBridge: Starting render process: %s", renderProcessPath.c_str());
        
        renderProcessPath_ = renderProcessPath;
        
        // Initialize IPC channels first
        if (!InitializeIPC()) {
            SetLastError("Failed to initialize IPC channels");
            return false;
        }
        
        // Start the process
        if (!StartProcessInternal(renderProcessPath, arguments)) {
            SetLastError("Failed to start render process");
            CleanupIPC();
            return false;
        }
        
        // Wait for process to initialize and connect
        auto startTime = std::chrono::steady_clock::now();
        bool processReady = false;
        
        while (std::chrono::steady_clock::now() - startTime < std::chrono::milliseconds(PROCESS_STARTUP_TIMEOUT)) {
            if (IsProcessAlive()) {
                // Test connection
                RenderCommand testCommand;
                testCommand.commandType = RenderCommandType::Initialize;
                testCommand.commandId = GenerateCommandId();
                
                auto result = SendCommand(testCommand, 2000);
                if (result.status == RenderResultStatus::Success) {
                    processReady = true;
                    break;
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        if (!processReady) {
            SetLastError("Render process failed to initialize within timeout");
            StopRenderProcess(1000);
            return false;
        }
        
        // Start background threads
        shouldStop_ = false;
        messageReceiveThread_ = std::thread(&RenderIPCBridge::MessageReceiveLoop, this);
        processMonitorThread_ = std::thread(&RenderIPCBridge::ProcessMonitorLoop, this);
        
        processRunning_ = true;
        
        if (processStartedCallback_) {
            processStartedCallback_(processInfo_.dwProcessId);
        }
        
        logger_.LogInfo(L"RenderIPCBridge: Render process started successfully (PID: %lu)", processInfo_.dwProcessId);
        return true;
        
    } catch (const std::exception& e) {
        SetLastError(std::string("Exception in StartRenderProcess: ") + e.what());
        logger_.LogError(L"RenderIPCBridge: StartRenderProcess exception: %s", 
                        std::wstring(e.what(), e.what() + strlen(e.what())).c_str());
        return false;
    }
}

bool RenderIPCBridge::StopRenderProcess(uint32_t timeoutMs)
{
    try {
        if (!processRunning_) {
            return true;
        }
        
        logger_.LogInfo(L"RenderIPCBridge: Stopping render process (timeout: %u ms)", timeoutMs);
        
        // Signal threads to stop
        shouldStop_ = true;
        
        // Send shutdown command to process
        try {
            RenderCommand shutdownCommand;
            shutdownCommand.commandType = RenderCommandType::Destroy;
            shutdownCommand.commandId = GenerateCommandId();
            SendCommandFireAndForget(shutdownCommand);
        } catch (...) {
            // Ignore errors during shutdown
        }
        
        // Wait for process to exit gracefully
        if (processInfo_.hProcess) {
            DWORD waitResult = WaitForSingleObject(processInfo_.hProcess, timeoutMs);
            
            if (waitResult == WAIT_TIMEOUT) {
                logger_.LogWarning(L"RenderIPCBridge: Process didn't exit gracefully, terminating");
                TerminateProcess(processInfo_.hProcess, 1);
            }
        }
        
        // Wait for threads to complete
        if (messageReceiveThread_.joinable()) {
            messageReceiveThread_.join();
        }
        
        if (processMonitorThread_.joinable()) {
            processMonitorThread_.join();
        }
        
        // Cleanup process handles
        CleanupProcess();
        
        processRunning_ = false;
        
        if (processExitedCallback_) {
            processExitedCallback_(0);
        }
        
        logger_.LogInfo(L"RenderIPCBridge: Render process stopped successfully");
        return true;
        
    } catch (const std::exception& e) {
        SetLastError(std::string("Exception in StopRenderProcess: ") + e.what());
        logger_.LogError(L"RenderIPCBridge: StopRenderProcess exception: %s",
                        std::wstring(e.what(), e.what() + strlen(e.what())).c_str());
        return false;
    }
}

bool RenderIPCBridge::IsProcessAlive() const
{
    if (!processInfo_.hProcess) {
        return false;
    }
    
    DWORD exitCode;
    if (!GetExitCodeProcess(processInfo_.hProcess, &exitCode)) {
        return false;
    }
    
    return exitCode == STILL_ACTIVE;
}

DWORD RenderIPCBridge::GetProcessId() const
{
    return processRunning_ ? processInfo_.dwProcessId : 0;
}

// ===== IPC COMMUNICATION =====

bool RenderIPCBridge::InitializeIPC()
{
    try {
        logger_.LogInfo(L"RenderIPCBridge: Initializing IPC channels");
        
        // Initialize based on mode
        bool success = true;
        
        if (ipcMode_ == IPCMode::SharedMemory || ipcMode_ == IPCMode::Hybrid) {
            sharedMemory_ = std::make_unique<SharedMemoryManager>(SHARED_MEMORY_NAME);
            if (!sharedMemory_->CreateSharedBuffer(DEFAULT_SHARED_MEMORY_SIZE)) {
                logger_.LogError(L"RenderIPCBridge: Failed to create shared memory buffer");
                success = false;
            }
        }
        
        if (ipcMode_ == IPCMode::NamedPipes || ipcMode_ == IPCMode::Hybrid) {
            namedPipe_ = std::make_unique<NamedPipeChannel>(NAMED_PIPE_NAME);
            if (!namedPipe_->CreateServer()) {
                logger_.LogError(L"RenderIPCBridge: Failed to create named pipe server");
                success = false;
            }
        }
        
        if (!success) {
            SetLastError("Failed to initialize IPC channels");
            CleanupIPC();
            return false;
        }
        
        logger_.LogInfo(L"RenderIPCBridge: IPC channels initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        SetLastError(std::string("Exception in InitializeIPC: ") + e.what());
        logger_.LogError(L"RenderIPCBridge: InitializeIPC exception: %s",
                        std::wstring(e.what(), e.what() + strlen(e.what())).c_str());
        return false;
    }
}

void RenderIPCBridge::CleanupIPC()
{
    try {
        logger_.LogInfo(L"RenderIPCBridge: Cleaning up IPC channels");
        
        if (namedPipe_) {
            namedPipe_->Cleanup();
            namedPipe_.reset();
        }
        
        if (sharedMemory_) {
            sharedMemory_->Cleanup();
            sharedMemory_.reset();
        }
        
        // Clear pending commands
        {
            std::lock_guard<std::mutex> lock(pendingCommandsMutex_);
            for (auto& command : pendingCommands_) {
                try {
                    RenderResult failureResult;
                    failureResult.commandId = command.first;
                    failureResult.status = RenderResultStatus::Failure;
                    failureResult.errorMessage = "IPC shutdown";
                    command.second->promise.set_value(failureResult);
                } catch (...) {
                    // Ignore promise exceptions during cleanup
                }
            }
            pendingCommands_.clear();
        }
        
        logger_.LogInfo(L"RenderIPCBridge: IPC cleanup completed");
        
    } catch (const std::exception& e) {
        logger_.LogError(L"RenderIPCBridge: CleanupIPC exception: %s",
                        std::wstring(e.what(), e.what() + strlen(e.what())).c_str());
    }
}

std::future<RenderResult> RenderIPCBridge::SendCommandAsync(const RenderCommand& command)
{
    std::lock_guard<std::mutex> lock(pendingCommandsMutex_);
    
    auto pendingCommand = std::make_unique<PendingCommand>();
    pendingCommand->commandId = command.commandId;
    pendingCommand->command = command;
    pendingCommand->sentTime = std::chrono::steady_clock::now();
    pendingCommand->timeoutMs = defaultTimeoutMs_;
    
    auto future = pendingCommand->promise.get_future();
    
    try {
        // Send command via appropriate channel
        bool sent = false;
        
        if (ipcMode_ == IPCMode::SharedMemory || ipcMode_ == IPCMode::Hybrid) {
            if (sharedMemory_ && sharedMemory_->WriteCommand(command)) {
                sharedMemory_->SignalCommandReady();
                sent = true;
            }
        }
        
        if (!sent && (ipcMode_ == IPCMode::NamedPipes || ipcMode_ == IPCMode::Hybrid)) {
            if (namedPipe_ && namedPipe_->SendCommand(command)) {
                sent = true;
            }
        }
        
        if (sent) {
            pendingCommands_[command.commandId] = std::move(pendingCommand);
            
            // Update statistics
            {
                std::lock_guard<std::mutex> statsLock(statsMutex_);
                stats_.totalCommandsSent++;
            }
        } else {
            // Command failed to send
            RenderResult failureResult;
            failureResult.commandId = command.commandId;
            failureResult.status = RenderResultStatus::Failure;
            failureResult.errorMessage = "Failed to send command via IPC";
            pendingCommand->promise.set_value(failureResult);
        }
        
    } catch (const std::exception& e) {
        RenderResult failureResult;
        failureResult.commandId = command.commandId;
        failureResult.status = RenderResultStatus::Failure;
        failureResult.errorMessage = std::string("Exception sending command: ") + e.what();
        pendingCommand->promise.set_value(failureResult);
    }
    
    return future;
}

RenderResult RenderIPCBridge::SendCommand(const RenderCommand& command, uint32_t timeoutMs)
{
    try {
        auto future = SendCommandAsync(command);
        
        if (future.wait_for(std::chrono::milliseconds(timeoutMs)) == std::future_status::ready) {
            return future.get();
        } else {
            // Timeout
            RenderResult timeoutResult;
            timeoutResult.commandId = command.commandId;
            timeoutResult.status = RenderResultStatus::Failure;
            timeoutResult.errorMessage = "Command timed out";
            
            // Remove from pending commands
            {
                std::lock_guard<std::mutex> lock(pendingCommandsMutex_);
                pendingCommands_.erase(command.commandId);
            }
            
            {
                std::lock_guard<std::mutex> statsLock(statsMutex_);
                stats_.timeoutCommands++;
            }
            
            return timeoutResult;
        }
        
    } catch (const std::exception& e) {
        RenderResult failureResult;
        failureResult.commandId = command.commandId;
        failureResult.status = RenderResultStatus::Failure;
        failureResult.errorMessage = std::string("Exception in SendCommand: ") + e.what();
        return failureResult;
    }
}

bool RenderIPCBridge::SendCommandFireAndForget(const RenderCommand& command)
{
    try {
        bool sent = false;
        
        if (ipcMode_ == IPCMode::SharedMemory || ipcMode_ == IPCMode::Hybrid) {
            if (sharedMemory_ && sharedMemory_->WriteCommand(command)) {
                sharedMemory_->SignalCommandReady();
                sent = true;
            }
        }
        
        if (!sent && (ipcMode_ == IPCMode::NamedPipes || ipcMode_ == IPCMode::Hybrid)) {
            if (namedPipe_ && namedPipe_->SendCommand(command)) {
                sent = true;
            }
        }
        
        if (sent) {
            std::lock_guard<std::mutex> statsLock(statsMutex_);
            stats_.totalCommandsSent++;
        }
        
        return sent;
        
    } catch (const std::exception& e) {
        logger_.LogError(L"RenderIPCBridge: SendCommandFireAndForget exception: %s",
                        std::wstring(e.what(), e.what() + strlen(e.what())).c_str());
        return false;
    }
}

// ===== SYSTEM CAPABILITIES =====

SystemCapabilities RenderIPCBridge::QuerySystemCapabilities(uint32_t timeoutMs)
{
    SystemCapabilities capabilities;
    
    try {
        if (!IsProcessAlive()) {
            return capabilities;
        }
        
        // Send capabilities query command
        RenderCommand query;
        query.commandType = RenderCommandType::Initialize; // Use as capabilities query
        query.commandId = GenerateCommandId();
        
        auto result = SendCommand(query, timeoutMs);
        
        if (result.status == RenderResultStatus::Success) {
            // Parse capabilities from result (implementation would depend on data format)
            capabilities.supportsSkiaSharp = true;
            capabilities.supportsDirect3D = true;
            capabilities.supportsWebView2 = true;
            capabilities.supportsHardwareAcceleration = true;
            capabilities.supportsMultiMonitor = true;
            capabilities.supportsHighDPI = true;
        }
        
    } catch (const std::exception& e) {
        logger_.LogError(L"RenderIPCBridge: QuerySystemCapabilities exception: %s",
                        std::wstring(e.what(), e.what() + strlen(e.what())).c_str());
    }
    
    return capabilities;
}

std::vector<RenderBackendType> RenderIPCBridge::GetSupportedBackends()
{
    std::vector<RenderBackendType> backends;
    
    auto capabilities = QuerySystemCapabilities(3000);
    
    if (capabilities.supportsSkiaSharp) {
        backends.push_back(RenderBackendType::SkiaSharp);
    }
    
    if (capabilities.supportsDirect3D) {
        backends.push_back(RenderBackendType::Direct3D);
    }
    
    if (capabilities.supportsWebView2) {
        backends.push_back(RenderBackendType::WebView);
    }
    
    return backends;
}

// ===== PERFORMANCE MONITORING =====

RenderIPCBridge::IPCStats RenderIPCBridge::GetIPCStatistics() const
{
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    IPCStats currentStats = stats_;
    currentStats.queuedCommands = pendingCommands_.size();
    
    return currentStats;
}

void RenderIPCBridge::ResetIPCStatistics()
{
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_ = IPCStats{};
    logger_.LogInfo(L"RenderIPCBridge: IPC statistics reset");
}

// ===== ERROR HANDLING =====

std::string RenderIPCBridge::GetLastError() const
{
    return lastError_;
}

bool RenderIPCBridge::IsHealthy() const
{
    return IsProcessAlive() && 
           ((sharedMemory_ && sharedMemory_->IsReady()) || 
            (namedPipe_ && namedPipe_->IsConnected()));
}

bool RenderIPCBridge::AttemptRecovery()
{
    try {
        logger_.LogInfo(L"RenderIPCBridge: Attempting recovery");
        
        if (!IsProcessAlive()) {
            // Process died, restart it
            if (!renderProcessPath_.empty()) {
                return StartRenderProcess(renderProcessPath_);
            }
        } else {
            // Process alive but IPC might be broken, reinitialize IPC
            CleanupIPC();
            return InitializeIPC();
        }
        
    } catch (const std::exception& e) {
        logger_.LogError(L"RenderIPCBridge: Recovery attempt failed: %s",
                        std::wstring(e.what(), e.what() + strlen(e.what())).c_str());
    }
    
    return false;
}

// ===== INTERNAL IMPLEMENTATION =====

bool RenderIPCBridge::StartProcessInternal(const std::wstring& path, const std::wstring& args)
{
    try {
        STARTUPINFO startupInfo;
        ZeroMemory(&startupInfo, sizeof(startupInfo));
        startupInfo.cb = sizeof(startupInfo);
        startupInfo.dwFlags = STARTF_USESHOWWINDOW;
        startupInfo.wShowWindow = SW_HIDE; // Hide render process window
        
        ZeroMemory(&processInfo_, sizeof(processInfo_));
        
        std::wstring commandLine = path;
        if (!args.empty()) {
            commandLine += L" " + args;
        }
        
        // Create process with shared memory and pipe names
        std::wstring fullArgs = args + L" --shared-memory=" + SHARED_MEMORY_NAME + 
                               L" --named-pipe=" + NAMED_PIPE_NAME;
        commandLine = path + L" " + fullArgs;
        
        BOOL result = CreateProcess(
            path.c_str(),
            const_cast<wchar_t*>(commandLine.c_str()),
            nullptr,      // Process security attributes
            nullptr,      // Thread security attributes
            FALSE,        // Inherit handles
            0,           // Creation flags
            nullptr,      // Environment
            nullptr,      // Current directory
            &startupInfo,
            &processInfo_
        );
        
        if (!result) {
            DWORD error = ::GetLastError();
            this->SetLastError("Failed to create process. Error: " + std::to_string(error));
            return false;
        }
        
        logger_.LogInfo(L"RenderIPCBridge: Process created successfully (PID: %lu)", processInfo_.dwProcessId);
        return true;
        
    } catch (const std::exception& e) {
        SetLastError(std::string("Exception in StartProcessInternal: ") + e.what());
        return false;
    }
}

void RenderIPCBridge::CleanupProcess()
{
    if (processInfo_.hProcess) {
        CloseHandle(processInfo_.hProcess);
        processInfo_.hProcess = nullptr;
    }
    
    if (processInfo_.hThread) {
        CloseHandle(processInfo_.hThread);
        processInfo_.hThread = nullptr;
    }
    
    processInfo_.dwProcessId = 0;
    processInfo_.dwThreadId = 0;
}

void RenderIPCBridge::MessageReceiveLoop()
{
    logger_.LogInfo(L"RenderIPCBridge: Message receive thread started");
    
    while (!shouldStop_) {
        try {
            RenderResult result;
            bool received = false;
            
            // Try to receive from shared memory first (faster)
            if (sharedMemory_ && sharedMemory_->WaitForResult(100)) {
                if (sharedMemory_->ReadResult(result)) {
                    received = true;
                }
            }
            
            // Try named pipe if shared memory didn't receive anything
            if (!received && namedPipe_) {
                if (namedPipe_->ReceiveResult(result, 100)) {
                    received = true;
                }
            }
            
            if (received) {
                HandleReceivedMessage(result);
            }
            
        } catch (const std::exception& e) {
            logger_.LogError(L"RenderIPCBridge: Message receive loop exception: %s",
                           std::wstring(e.what(), e.what() + strlen(e.what())).c_str());
        }
        
        // Check for timed out commands
        TimeoutPendingCommands();
    }
    
    logger_.LogInfo(L"RenderIPCBridge: Message receive thread stopped");
}

void RenderIPCBridge::ProcessMonitorLoop()
{
    logger_.LogInfo(L"RenderIPCBridge: Process monitor thread started");
    
    while (!shouldStop_) {
        try {
            if (!IsProcessAlive()) {
                logger_.LogWarning(L"RenderIPCBridge: Render process has died");
                
                if (processExitedCallback_) {
                    processExitedCallback_(1);
                }
                
                processRunning_ = false;
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
        } catch (const std::exception& e) {
            logger_.LogError(L"RenderIPCBridge: Process monitor loop exception: %s",
                           std::wstring(e.what(), e.what() + strlen(e.what())).c_str());
        }
    }
    
    logger_.LogInfo(L"RenderIPCBridge: Process monitor thread stopped");
}

void RenderIPCBridge::HandleReceivedMessage(const RenderResult& result)
{
    try {
        std::lock_guard<std::mutex> lock(pendingCommandsMutex_);
        
        auto it = pendingCommands_.find(result.commandId);
        if (it != pendingCommands_.end()) {
            // Calculate round trip time
            auto roundTripTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - it->second->sentTime).count();
            
            // Update statistics
            UpdateStats(it->second->command, result, roundTripTime);
            
            // Set the promise result
            it->second->promise.set_value(result);
            
            // Remove from pending commands
            pendingCommands_.erase(it);
        }
        
        // Trigger callback if set
        if (messageReceivedCallback_) {
            messageReceivedCallback_(result);
        }
        
    } catch (const std::exception& e) {
        logger_.LogError(L"RenderIPCBridge: HandleReceivedMessage exception: %s",
                        std::wstring(e.what(), e.what() + strlen(e.what())).c_str());
    }
}

void RenderIPCBridge::TimeoutPendingCommands()
{
    try {
        std::lock_guard<std::mutex> lock(pendingCommandsMutex_);
        
        auto now = std::chrono::steady_clock::now();
        std::vector<uint64_t> timedOutCommands;
        
        for (auto& command : pendingCommands_) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - command.second->sentTime).count();
                
            if (elapsed > command.second->timeoutMs) {
                timedOutCommands.push_back(command.first);
            }
        }
        
        // Process timed out commands
        for (auto commandId : timedOutCommands) {
            auto it = pendingCommands_.find(commandId);
            if (it != pendingCommands_.end()) {
                RenderResult timeoutResult;
                timeoutResult.commandId = commandId;
                timeoutResult.status = RenderResultStatus::Failure;
                timeoutResult.errorMessage = "Command timeout";
                
                it->second->promise.set_value(timeoutResult);
                pendingCommands_.erase(it);
                
                {
                    std::lock_guard<std::mutex> statsLock(statsMutex_);
                    stats_.timeoutCommands++;
                }
            }
        }
        
    } catch (const std::exception& e) {
        logger_.LogError(L"RenderIPCBridge: TimeoutPendingCommands exception: %s",
                        std::wstring(e.what(), e.what() + strlen(e.what())).c_str());
    }
}

uint64_t RenderIPCBridge::GenerateCommandId()
{
    return nextCommandId_++;
}

void RenderIPCBridge::UpdateStats(const RenderCommand& command, const RenderResult& result, uint64_t roundTripMs)
{
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    stats_.totalResultsReceived++;
    
    if (result.status == RenderResultStatus::Success) {
        // Update average round trip time
        if (stats_.totalResultsReceived > 1) {
            stats_.averageRoundTripMs = (stats_.averageRoundTripMs * (stats_.totalResultsReceived - 1) + roundTripMs) 
                                       / stats_.totalResultsReceived;
        } else {
            stats_.averageRoundTripMs = roundTripMs;
        }
    } else {
        stats_.failedCommands++;
    }
}

void RenderIPCBridge::SetLastError(const std::string& error) const
{
    lastError_ = error;
}

// ===== EVENT CALLBACKS =====

void RenderIPCBridge::SetProcessStartedCallback(ProcessStartedCallback callback)
{
    processStartedCallback_ = callback;
}

void RenderIPCBridge::SetProcessExitedCallback(ProcessExitedCallback callback)
{
    processExitedCallback_ = callback;
}

void RenderIPCBridge::SetMessageReceivedCallback(MessageReceivedCallback callback)
{
    messageReceivedCallback_ = callback;
}

// ===== CONFIGURATION =====

void RenderIPCBridge::SetIPCMode(IPCMode mode)
{
    ipcMode_ = mode;
    logger_.LogInfo(L"RenderIPCBridge: IPC mode set to %d", static_cast<int>(mode));
}

IPCMode RenderIPCBridge::GetIPCMode() const
{
    return ipcMode_;
}

void RenderIPCBridge::SetDefaultTimeout(uint32_t timeoutMs)
{
    defaultTimeoutMs_ = timeoutMs;
}

uint32_t RenderIPCBridge::GetDefaultTimeout() const
{
    return defaultTimeoutMs_;
}
