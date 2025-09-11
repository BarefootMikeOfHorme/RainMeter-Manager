#include "named_pipe_channel.h"

using namespace RainmeterManager::Render;

NamedPipeChannel::NamedPipeChannel(const std::wstring& pipeName)
    : pipeName_(pipeName), pipeHandle_(nullptr) {}

NamedPipeChannel::~NamedPipeChannel() { Cleanup(); }

bool NamedPipeChannel::CreateServer(DWORD /*maxInstances*/, DWORD /*bufferSize*/) { return false; }
bool NamedPipeChannel::WaitForConnection(DWORD /*timeoutMs*/) { return false; }
void NamedPipeChannel::DisconnectClient() {}

bool NamedPipeChannel::ConnectAsClient(DWORD /*timeoutMs*/, int /*retryAttempts*/) { return false; }

bool NamedPipeChannel::SendCommand(const RenderCommand& /*command*/) { return false; }
bool NamedPipeChannel::ReceiveResult(RenderResult& /*result*/, DWORD /*timeoutMs*/) { return false; }
bool NamedPipeChannel::SendResult(const RenderResult& /*result*/) { return false; }
bool NamedPipeChannel::ReceiveCommand(RenderCommand& /*command*/, DWORD /*timeoutMs*/) { return false; }

bool NamedPipeChannel::IsConnected() const { return false; }
bool NamedPipeChannel::IsServer() const { return isServer_; }
std::string NamedPipeChannel::GetLastError() const { return lastError_; }
bool NamedPipeChannel::TestConnection() { return false; }

bool NamedPipeChannel::EnableAsyncMode(std::function<void(const RenderResult&)> /*callback*/) { return false; }
void NamedPipeChannel::DisableAsyncMode() {}

NamedPipeChannel::PipeStatistics NamedPipeChannel::GetStatistics() const { return stats_; }
void NamedPipeChannel::ResetStatistics() { stats_ = PipeStatistics{}; }

bool NamedPipeChannel::SetPipeSecurity(const std::wstring& /*securityDescriptor*/) { return true; }

void NamedPipeChannel::Cleanup() {
    if (pipeHandle_) {
        CloseHandle(pipeHandle_);
        pipeHandle_ = nullptr;
    }
    isConnected_ = false;
}

