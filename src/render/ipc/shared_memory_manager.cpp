#include "shared_memory_manager.h"
#include <sstream>
#include <algorithm>
#include <cstring>

using namespace RainmeterManager::Render;

namespace {
    // Serialization helpers for commands and results
    template<typename T>
    void SerializeValue(uint8_t*& buffer, const T& value) {
        memcpy(buffer, &value, sizeof(T));
        buffer += sizeof(T);
    }
    
    template<typename T>
    void DeserializeValue(const uint8_t*& buffer, T& value) {
        memcpy(&value, buffer, sizeof(T));
        buffer += sizeof(T);
    }
    
    void SerializeString(uint8_t*& buffer, const std::string& str) {
        uint32_t length = static_cast<uint32_t>(str.length());
        SerializeValue(buffer, length);
        if (length > 0) {
            memcpy(buffer, str.c_str(), length);
            buffer += length;
        }
    }
    
    void DeserializeString(const uint8_t*& buffer, std::string& str) {
        uint32_t length;
        DeserializeValue(buffer, length);
        if (length > 0) {
            str.assign(reinterpret_cast<const char*>(buffer), length);
            buffer += length;
        } else {
            str.clear();
        }
    }
}

SharedMemoryManager::SharedMemoryManager(const std::wstring& name)
    : bufferName_(name)
    , fileMapping_(nullptr)
    , mappedView_(nullptr)
    , bufferSize_(0)
    , header_(nullptr)
    , commandBuffer_(nullptr)
    , resultBuffer_(nullptr)
    , commandEvent_(nullptr)
    , resultEvent_(nullptr)
    , accessMutex_(nullptr)
{
}

SharedMemoryManager::~SharedMemoryManager()
{
    Cleanup();
}

// ===== BUFFER MANAGEMENT =====

bool SharedMemoryManager::CreateSharedBuffer(size_t bufferSize)
{
    if (bufferSize < MIN_BUFFER_SIZE || bufferSize > MAX_BUFFER_SIZE) {
        return false;
    }
    
    try {
        bufferSize_ = bufferSize;
        isServer_ = true;
        
        return InitializeBuffer(true);
        
    } catch (const std::exception& e) {
        Cleanup();
        return false;
    }
}

bool SharedMemoryManager::ConnectToSharedBuffer()
{
    try {
        isServer_ = false;
        return InitializeBuffer(false);
        
    } catch (const std::exception& e) {
        Cleanup();
        return false;
    }
}

bool SharedMemoryManager::IsReady() const
{
    return isReady_ && header_ && commandBuffer_ && resultBuffer_;
}

size_t SharedMemoryManager::GetBufferSize() const
{
    return bufferSize_;
}

void SharedMemoryManager::Cleanup()
{
    try {
        isReady_ = false;
        
        CleanupSynchronization();
        
        if (mappedView_) {
            UnmapViewOfFile(mappedView_);
            mappedView_ = nullptr;
        }
        
        if (fileMapping_) {
            CloseHandle(fileMapping_);
            fileMapping_ = nullptr;
        }
        
        header_ = nullptr;
        commandBuffer_ = nullptr;
        resultBuffer_ = nullptr;
        bufferSize_ = 0;
        
    } catch (...) {
        // Ignore cleanup exceptions
    }
}

// ===== COMMUNICATION =====

bool SharedMemoryManager::WriteCommand(const RenderCommand& command)
{
    if (!IsReady()) {
        return false;
    }
    
    try {
        // Wait for exclusive access
        DWORD waitResult = WaitForSingleObject(accessMutex_, 5000);
        if (waitResult != WAIT_OBJECT_0) {
            return false;
        }
        
        // Calculate command buffer size (half of available space minus header)
        size_t commandBufferSize = (bufferSize_ - sizeof(SharedBufferHeader)) / 2;
        
        // Serialize command
        size_t serializedSize = SerializeCommand(command, commandBuffer_, commandBufferSize);
        
        if (serializedSize == 0 || serializedSize > commandBufferSize) {
            ReleaseMutex(accessMutex_);
            return false;
        }
        
        // Update header
        header_->commandSize = serializedSize;
        header_->commandSequence++;
        header_->commandReady.store(true);
        
        // Update statistics
        UpdateStats(true, serializedSize);
        
        ReleaseMutex(accessMutex_);
        return true;
        
    } catch (const std::exception& e) {
        if (accessMutex_) {
            ReleaseMutex(accessMutex_);
        }
        return false;
    }
}

bool SharedMemoryManager::ReadResult(RenderResult& result)
{
    if (!IsReady()) {
        return false;
    }
    
    try {
        // Wait for exclusive access
        DWORD waitResult = WaitForSingleObject(accessMutex_, 5000);
        if (waitResult != WAIT_OBJECT_0) {
            return false;
        }
        
        // Check if result is ready
        if (!header_->resultReady.load()) {
            ReleaseMutex(accessMutex_);
            return false;
        }
        
        // Deserialize result
        bool success = DeserializeResult(resultBuffer_, header_->resultSize, result);
        
        if (success) {
            // Mark result as read
            header_->resultReady.store(false);
            
            // Update statistics
            UpdateStats(false, header_->resultSize);
        }
        
        ReleaseMutex(accessMutex_);
        return success;
        
    } catch (const std::exception& e) {
        if (accessMutex_) {
            ReleaseMutex(accessMutex_);
        }
        return false;
    }
}

bool SharedMemoryManager::WriteResult(const RenderResult& result)
{
    if (!IsReady()) {
        return false;
    }
    
    try {
        // Wait for exclusive access
        DWORD waitResult = WaitForSingleObject(accessMutex_, 5000);
        if (waitResult != WAIT_OBJECT_0) {
            return false;
        }
        
        // Calculate result buffer size
        size_t resultBufferSize = (bufferSize_ - sizeof(SharedBufferHeader)) / 2;
        
        // Serialize result
        size_t serializedSize = SerializeResult(result, resultBuffer_, resultBufferSize);
        
        if (serializedSize == 0 || serializedSize > resultBufferSize) {
            ReleaseMutex(accessMutex_);
            return false;
        }
        
        // Update header
        header_->resultSize = serializedSize;
        header_->resultSequence++;
        header_->resultReady.store(true);
        
        // Update statistics
        UpdateStats(true, serializedSize);
        
        ReleaseMutex(accessMutex_);
        return true;
        
    } catch (const std::exception& e) {
        if (accessMutex_) {
            ReleaseMutex(accessMutex_);
        }
        return false;
    }
}

bool SharedMemoryManager::ReadCommand(RenderCommand& command)
{
    if (!IsReady()) {
        return false;
    }
    
    try {
        // Wait for exclusive access
        DWORD waitResult = WaitForSingleObject(accessMutex_, 5000);
        if (waitResult != WAIT_OBJECT_0) {
            return false;
        }
        
        // Check if command is ready
        if (!header_->commandReady.load()) {
            ReleaseMutex(accessMutex_);
            return false;
        }
        
        // Deserialize command
        bool success = DeserializeCommand(commandBuffer_, header_->commandSize, command);
        
        if (success) {
            // Mark command as read
            header_->commandReady.store(false);
            
            // Update statistics
            UpdateStats(false, header_->commandSize);
        }
        
        ReleaseMutex(accessMutex_);
        return success;
        
    } catch (const std::exception& e) {
        if (accessMutex_) {
            ReleaseMutex(accessMutex_);
        }
        return false;
    }
}

// ===== SYNCHRONIZATION =====

bool SharedMemoryManager::SignalCommandReady()
{
    if (!commandEvent_) {
        return false;
    }
    
    return SetEvent(commandEvent_) != 0;
}

bool SharedMemoryManager::WaitForCommand(uint32_t timeoutMs)
{
    if (!commandEvent_) {
        return false;
    }
    
    DWORD waitResult = WaitForSingleObject(commandEvent_, timeoutMs);
    return waitResult == WAIT_OBJECT_0;
}

bool SharedMemoryManager::SignalResultReady()
{
    if (!resultEvent_) {
        return false;
    }
    
    return SetEvent(resultEvent_) != 0;
}

bool SharedMemoryManager::WaitForResult(uint32_t timeoutMs)
{
    if (!resultEvent_) {
        return false;
    }
    
    DWORD waitResult = WaitForSingleObject(resultEvent_, timeoutMs);
    return waitResult == WAIT_OBJECT_0;
}

// ===== STATISTICS =====

SharedMemoryManager::SharedMemoryStats SharedMemoryManager::GetStatistics() const
{
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_;
}

void SharedMemoryManager::ResetStatistics()
{
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_ = SharedMemoryStats{};
}

// ===== INTERNAL IMPLEMENTATION =====

bool SharedMemoryManager::InitializeBuffer(bool createNew)
{
    try {
        // Create or open file mapping
        DWORD protection = createNew ? PAGE_READWRITE : PAGE_READWRITE;
        DWORD access = createNew ? FILE_MAP_ALL_ACCESS : FILE_MAP_ALL_ACCESS;
        
        if (createNew) {
            // Server creates the mapping
            fileMapping_ = CreateFileMapping(
                INVALID_HANDLE_VALUE,
                nullptr,
                protection,
                0,
                static_cast<DWORD>(bufferSize_),
                bufferName_.c_str()
            );
            
            if (!fileMapping_ || GetLastError() == ERROR_ALREADY_EXISTS) {
                if (fileMapping_) {
                    CloseHandle(fileMapping_);
                    fileMapping_ = nullptr;
                }
                return false;
            }
        } else {
            // Client opens existing mapping
            fileMapping_ = OpenFileMapping(
                access,
                FALSE,
                bufferName_.c_str()
            );
            
            if (!fileMapping_) {
                return false;
            }
            
            // Get the size from the existing mapping
            MEMORY_BASIC_INFORMATION memInfo;
            if (VirtualQuery(fileMapping_, &memInfo, sizeof(memInfo)) == sizeof(memInfo)) {
                bufferSize_ = memInfo.RegionSize;
            } else {
                bufferSize_ = MIN_BUFFER_SIZE; // Fallback
            }
        }
        
        // Map the view
        mappedView_ = MapViewOfFile(
            fileMapping_,
            access,
            0,
            0,
            bufferSize_
        );
        
        if (!mappedView_) {
            CloseHandle(fileMapping_);
            fileMapping_ = nullptr;
            return false;
        }
        
        // Setup buffer pointers
        header_ = reinterpret_cast<SharedBufferHeader*>(mappedView_);
        uint8_t* dataStart = reinterpret_cast<uint8_t*>(mappedView_) + sizeof(SharedBufferHeader);
        size_t dataSize = bufferSize_ - sizeof(SharedBufferHeader);
        
        commandBuffer_ = dataStart;
        resultBuffer_ = dataStart + (dataSize / 2);
        
        // Initialize header if creating new
        if (createNew) {
            header_->magic = BUFFER_MAGIC;
            header_->version = PROTOCOL_VERSION;
            header_->commandSize = 0;
            header_->resultSize = 0;
            header_->commandReady.store(false);
            header_->resultReady.store(false);
            header_->commandSequence = 0;
            header_->resultSequence = 0;
        }
        
        // Validate buffer
        if (!ValidateBuffer()) {
            return false;
        }
        
        // Setup synchronization
        if (!SetupSynchronization()) {
            return false;
        }
        
        isReady_ = true;
        return true;
        
    } catch (const std::exception& e) {
        return false;
    }
}

bool SharedMemoryManager::SetupSynchronization()
{
    try {
        std::wstring commandEventName = GetEventName(L"_Command");
        std::wstring resultEventName = GetEventName(L"_Result");
        std::wstring mutexName = GetEventName(L"_Mutex");
        
        if (isServer_) {
            // Server creates synchronization objects
            commandEvent_ = CreateEvent(nullptr, FALSE, FALSE, commandEventName.c_str());
            resultEvent_ = CreateEvent(nullptr, FALSE, FALSE, resultEventName.c_str());
            accessMutex_ = CreateMutex(nullptr, FALSE, mutexName.c_str());
        } else {
            // Client opens existing synchronization objects
            commandEvent_ = OpenEvent(EVENT_ALL_ACCESS, FALSE, commandEventName.c_str());
            resultEvent_ = OpenEvent(EVENT_ALL_ACCESS, FALSE, resultEventName.c_str());
            accessMutex_ = OpenMutex(MUTEX_ALL_ACCESS, FALSE, mutexName.c_str());
        }
        
        return commandEvent_ && resultEvent_ && accessMutex_;
        
    } catch (const std::exception& e) {
        CleanupSynchronization();
        return false;
    }
}

void SharedMemoryManager::CleanupSynchronization()
{
    if (commandEvent_) {
        CloseHandle(commandEvent_);
        commandEvent_ = nullptr;
    }
    
    if (resultEvent_) {
        CloseHandle(resultEvent_);
        resultEvent_ = nullptr;
    }
    
    if (accessMutex_) {
        CloseHandle(accessMutex_);
        accessMutex_ = nullptr;
    }
}

bool SharedMemoryManager::ValidateBuffer() const
{
    if (!header_) {
        return false;
    }
    
    return header_->magic == BUFFER_MAGIC && 
           header_->version == PROTOCOL_VERSION &&
           bufferSize_ >= MIN_BUFFER_SIZE &&
           bufferSize_ <= MAX_BUFFER_SIZE;
}

size_t SharedMemoryManager::SerializeCommand(const RenderCommand& command, uint8_t* buffer, size_t bufferSize)
{
    try {
        uint8_t* originalBuffer = buffer;
        uint8_t* bufferEnd = buffer + bufferSize;
        
        // Check if we have enough space for basic fields
        if (buffer + sizeof(command.commandId) + sizeof(command.commandType) + 
            sizeof(command.widgetId) > bufferEnd) {
            return 0;
        }
        
        // Serialize basic fields
        SerializeValue(buffer, command.commandId);
        SerializeValue(buffer, command.commandType);
        SerializeValue(buffer, command.widgetId);
        SerializeValue(buffer, command.windowHandle);
        SerializeValue(buffer, command.backendType);
        SerializeValue(buffer, command.bounds);
        SerializeValue(buffer, command.timestamp);
        
        // Serialize content parameters
        SerializeValue(buffer, command.content.sourceType);
        SerializeString(buffer, command.content.sourceUrl);
        SerializeString(buffer, command.content.templatePath);
        SerializeString(buffer, command.content.authToken);
        SerializeValue(buffer, command.content.refreshIntervalMs);
        SerializeValue(buffer, command.content.cacheEnabled);
        
        // Serialize custom headers
        uint32_t headerCount = static_cast<uint32_t>(command.content.customHeaders.size());
        SerializeValue(buffer, headerCount);
        for (const auto& header : command.content.customHeaders) {
            SerializeString(buffer, header.first);
            SerializeString(buffer, header.second);
        }
        
        // Serialize parameters
        uint32_t paramCount = static_cast<uint32_t>(command.content.parameters.size());
        SerializeValue(buffer, paramCount);
        for (const auto& param : command.content.parameters) {
            SerializeString(buffer, param.first);
            SerializeString(buffer, param.second);
        }
        
        // Serialize render properties
        SerializeValue(buffer, command.properties);
        
        return buffer - originalBuffer;
        
    } catch (const std::exception& e) {
        return 0;
    }
}

bool SharedMemoryManager::DeserializeCommand(const uint8_t* buffer, size_t bufferSize, RenderCommand& command)
{
    try {
        const uint8_t* bufferEnd = buffer + bufferSize;
        
        // Deserialize basic fields
        DeserializeValue(buffer, command.commandId);
        DeserializeValue(buffer, command.commandType);
        DeserializeValue(buffer, command.widgetId);
        DeserializeValue(buffer, command.windowHandle);
        DeserializeValue(buffer, command.backendType);
        DeserializeValue(buffer, command.bounds);
        DeserializeValue(buffer, command.timestamp);
        
        // Deserialize content parameters
        DeserializeValue(buffer, command.content.sourceType);
        DeserializeString(buffer, command.content.sourceUrl);
        DeserializeString(buffer, command.content.templatePath);
        DeserializeString(buffer, command.content.authToken);
        DeserializeValue(buffer, command.content.refreshIntervalMs);
        DeserializeValue(buffer, command.content.cacheEnabled);
        
        // Deserialize custom headers
        uint32_t headerCount;
        DeserializeValue(buffer, headerCount);
        command.content.customHeaders.clear();
        command.content.customHeaders.reserve(headerCount);
        
        for (uint32_t i = 0; i < headerCount; ++i) {
            std::string key, value;
            DeserializeString(buffer, key);
            DeserializeString(buffer, value);
            command.content.customHeaders.emplace_back(std::move(key), std::move(value));
        }
        
        // Deserialize parameters
        uint32_t paramCount;
        DeserializeValue(buffer, paramCount);
        command.content.parameters.clear();
        command.content.parameters.reserve(paramCount);
        
        for (uint32_t i = 0; i < paramCount; ++i) {
            std::string key, value;
            DeserializeString(buffer, key);
            DeserializeString(buffer, value);
            command.content.parameters.emplace_back(std::move(key), std::move(value));
        }
        
        // Deserialize render properties
        DeserializeValue(buffer, command.properties);
        
        return buffer <= bufferEnd;
        
    } catch (const std::exception& e) {
        return false;
    }
}

size_t SharedMemoryManager::SerializeResult(const RenderResult& result, uint8_t* buffer, size_t bufferSize)
{
    try {
        uint8_t* originalBuffer = buffer;
        uint8_t* bufferEnd = buffer + bufferSize;
        
        // Check if we have enough space for basic fields
        if (buffer + sizeof(result.commandId) + sizeof(result.status) > bufferEnd) {
            return 0;
        }
        
        // Serialize basic fields
        SerializeValue(buffer, result.commandId);
        SerializeValue(buffer, result.widgetId);
        SerializeValue(buffer, result.status);
        SerializeString(buffer, result.errorMessage);
        SerializeValue(buffer, result.renderTimeMs);
        SerializeValue(buffer, result.frameCount);
        SerializeValue(buffer, result.averageFps);
        SerializeValue(buffer, result.memoryUsageMB);
        SerializeValue(buffer, result.timestamp);
        
        return buffer - originalBuffer;
        
    } catch (const std::exception& e) {
        return 0;
    }
}

bool SharedMemoryManager::DeserializeResult(const uint8_t* buffer, size_t bufferSize, RenderResult& result)
{
    try {
        const uint8_t* bufferEnd = buffer + bufferSize;
        
        // Deserialize basic fields
        DeserializeValue(buffer, result.commandId);
        DeserializeValue(buffer, result.widgetId);
        DeserializeValue(buffer, result.status);
        DeserializeString(buffer, result.errorMessage);
        DeserializeValue(buffer, result.renderTimeMs);
        DeserializeValue(buffer, result.frameCount);
        DeserializeValue(buffer, result.averageFps);
        DeserializeValue(buffer, result.memoryUsageMB);
        DeserializeValue(buffer, result.timestamp);
        
        return buffer <= bufferEnd;
        
    } catch (const std::exception& e) {
        return false;
    }
}

std::wstring SharedMemoryManager::GetEventName(const std::wstring& suffix) const
{
    return bufferName_ + suffix;
}

void SharedMemoryManager::UpdateStats(bool isWrite, size_t bytes)
{
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    if (isWrite) {
        stats_.bytesWritten += bytes;
        stats_.writeOperations++;
    } else {
        stats_.bytesRead += bytes;
        stats_.readOperations++;
    }
}
