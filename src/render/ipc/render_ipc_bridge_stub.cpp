#include "render_ipc_bridge.h"
#include "shared_memory_manager.h"
#include "named_pipe_channel.h"
#include "../../core/logger_adapter.h"

using namespace RainmeterManager::Render;

RenderIPCBridge::RenderIPCBridge(IPCMode mode) 
    : ipcMode_(mode)
    , logger_(RainmeterManager::Core::Logger::GetInstance())
{
    ZeroMemory(&processInfo_, sizeof(processInfo_));
}

RenderIPCBridge::~RenderIPCBridge() {
    StopRenderProcess(500);
    CleanupIPC();
}

bool RenderIPCBridge::StartRenderProcess(const std::wstring& renderProcessPath, const std::wstring& arguments) {
    renderProcessPath_ = renderProcessPath;
    (void)arguments;
    processRunning_ = false; // stub does not actually start
    return false;
}

bool RenderIPCBridge::StopRenderProcess(uint32_t /*timeoutMs*/) {
    processRunning_ = false;
    return true;
}

bool RenderIPCBridge::IsProcessAlive() const {
    return processRunning_.load();
}

DWORD RenderIPCBridge::GetProcessId() const {
    return processRunning_ ? processInfo_.dwProcessId : 0;
}

bool RenderIPCBridge::InitializeIPC() {
    return true;
}

void RenderIPCBridge::CleanupIPC() {
    sharedMemory_.reset();
    namedPipe_.reset();
}

std::future<RenderResult> RenderIPCBridge::SendCommandAsync(const RenderCommand& command) {
    std::promise<RenderResult> p;
    RenderResult r{};
    r.commandId = command.commandId;
    r.status = RenderResultStatus::Failure;
    r.errorMessage = "IPC disabled (stub)";
    p.set_value(r);
    return p.get_future();
}

RenderResult RenderIPCBridge::SendCommand(const RenderCommand& command, uint32_t /*timeoutMs*/) {
    RenderResult r{};
    r.commandId = command.commandId;
    r.status = RenderResultStatus::Failure;
    r.errorMessage = "IPC disabled (stub)";
    return r;
}

bool RenderIPCBridge::SendCommandFireAndForget(const RenderCommand& /*command*/) {
    return false;
}

SystemCapabilities RenderIPCBridge::QuerySystemCapabilities(uint32_t /*timeoutMs*/) {
    return SystemCapabilities{};
}

std::vector<RenderBackendType> RenderIPCBridge::GetSupportedBackends() {
    return { RenderBackendType::Auto };
}

RenderIPCBridge::IPCStats RenderIPCBridge::GetIPCStatistics() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_;
}

void RenderIPCBridge::ResetIPCStatistics() {
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_ = IPCStats{};
}

void RenderIPCBridge::SetProcessStartedCallback(ProcessStartedCallback callback) { processStartedCallback_ = std::move(callback); }
void RenderIPCBridge::SetProcessExitedCallback(ProcessExitedCallback callback) { processExitedCallback_ = std::move(callback); }
void RenderIPCBridge::SetMessageReceivedCallback(MessageReceivedCallback callback) { messageReceivedCallback_ = std::move(callback); }

void RenderIPCBridge::SetIPCMode(IPCMode mode) { ipcMode_ = mode; }
IPCMode RenderIPCBridge::GetIPCMode() const { return ipcMode_; }

void RenderIPCBridge::SetDefaultTimeout(uint32_t timeoutMs) { defaultTimeoutMs_ = timeoutMs; }
uint32_t RenderIPCBridge::GetDefaultTimeout() const { return defaultTimeoutMs_; }

std::string RenderIPCBridge::GetLastError() const { std::lock_guard<std::mutex> lock(statsMutex_); return lastError_; }
bool RenderIPCBridge::IsHealthy() const { return false; }
bool RenderIPCBridge::AttemptRecovery() { return false; }

