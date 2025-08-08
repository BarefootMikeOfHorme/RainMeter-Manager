#pragma once

#include "../interfaces/render_command.h"
#include <windows.h>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>

namespace RainmeterManager::Render {

    /**
     * @brief High-performance shared memory communication
     * 
     * This class provides efficient shared memory communication between
     * the C++ core and C# rendering process using Windows file mapping.
     */
    class SharedMemoryManager {
    public:
        explicit SharedMemoryManager(const std::wstring& name);
        ~SharedMemoryManager();

        // === Buffer Management ===
        
        /**
         * @brief Create shared memory buffer
         * @param bufferSize Size of buffer in bytes
         * @return True if buffer creation succeeded
         */
        bool CreateSharedBuffer(size_t bufferSize);
        
        /**
         * @brief Connect to existing shared memory buffer
         * @return True if connection succeeded
         */
        bool ConnectToSharedBuffer();
        
        /**
         * @brief Check if shared memory is ready
         * @return True if ready for read/write operations
         */
        bool IsReady() const;
        
        /**
         * @brief Get buffer size
         * @return Buffer size in bytes
         */
        size_t GetBufferSize() const;
        
        /**
         * @brief Cleanup shared memory resources
         */
        void Cleanup();

        // === Communication ===
        
        /**
         * @brief Write command to shared memory
         * @param command Command to write
         * @return True if write succeeded
         */
        bool WriteCommand(const RenderCommand& command);
        
        /**
         * @brief Read result from shared memory
         * @param result Output result structure
         * @return True if read succeeded
         */
        bool ReadResult(RenderResult& result);
        
        /**
         * @brief Write result to shared memory (for C# process)
         * @param result Result to write
         * @return True if write succeeded
         */
        bool WriteResult(const RenderResult& result);
        
        /**
         * @brief Read command from shared memory (for C# process)
         * @param command Output command structure
         * @return True if read succeeded
         */
        bool ReadCommand(RenderCommand& command);

        // === Synchronization ===
        
        /**
         * @brief Signal that command is available
         * @return True if signal succeeded
         */
        bool SignalCommandReady();
        
        /**
         * @brief Wait for command signal
         * @param timeoutMs Timeout in milliseconds
         * @return True if command is ready
         */
        bool WaitForCommand(uint32_t timeoutMs = INFINITE);
        
        /**
         * @brief Signal that result is available
         * @return True if signal succeeded
         */
        bool SignalResultReady();
        
        /**
         * @brief Wait for result signal
         * @param timeoutMs Timeout in milliseconds
         * @return True if result is ready
         */
        bool WaitForResult(uint32_t timeoutMs = INFINITE);

        // === Statistics ===
        
        struct SharedMemoryStats {
            uint64_t bytesWritten = 0;
            uint64_t bytesRead = 0;
            uint64_t writeOperations = 0;
            uint64_t readOperations = 0;
            uint64_t syncTimeouts = 0;
            uint64_t syncErrors = 0;
        };
        
        /**
         * @brief Get shared memory statistics
         * @return Statistics structure
         */
        SharedMemoryStats GetStatistics() const;
        
        /**
         * @brief Reset statistics counters
         */
        void ResetStatistics();

    private:
        // === Internal Structures ===
        
        struct SharedBufferHeader {
            uint32_t magic;           // Magic number for validation
            uint32_t version;         // Protocol version
            uint64_t commandSize;     // Size of command data
            uint64_t resultSize;      // Size of result data
            std::atomic<bool> commandReady;
            std::atomic<bool> resultReady;
            uint64_t commandSequence; // Sequence number for commands
            uint64_t resultSequence;  // Sequence number for results
        };

        // === Member Variables ===
        std::wstring bufferName_;
        HANDLE fileMapping_;
        void* mappedView_;
        size_t bufferSize_;
        SharedBufferHeader* header_;
        uint8_t* commandBuffer_;
        uint8_t* resultBuffer_;
        
        // Synchronization objects
        HANDLE commandEvent_;
        HANDLE resultEvent_;
        HANDLE accessMutex_;
        
        // State
        std::atomic<bool> isReady_{false};
        std::atomic<bool> isServer_{false};
        
        // Statistics
        mutable std::mutex statsMutex_;
        SharedMemoryStats stats_;
        
        // Constants
        static constexpr uint32_t BUFFER_MAGIC = 0x524D5348; // 'RMSH'
        static constexpr uint32_t PROTOCOL_VERSION = 1;
        static constexpr size_t MIN_BUFFER_SIZE = 64 * 1024;  // 64KB
        static constexpr size_t MAX_BUFFER_SIZE = 16 * 1024 * 1024; // 16MB

        // === Internal Methods ===
        bool InitializeBuffer(bool createNew);
        bool SetupSynchronization();
        void CleanupSynchronization();
        bool ValidateBuffer() const;
        size_t SerializeCommand(const RenderCommand& command, uint8_t* buffer, size_t bufferSize);
        bool DeserializeCommand(const uint8_t* buffer, size_t bufferSize, RenderCommand& command);
        size_t SerializeResult(const RenderResult& result, uint8_t* buffer, size_t bufferSize);
        bool DeserializeResult(const uint8_t* buffer, size_t bufferSize, RenderResult& result);
        std::wstring GetEventName(const std::wstring& suffix) const;
        void UpdateStats(bool isWrite, size_t bytes);

        // Non-copyable
        SharedMemoryManager(const SharedMemoryManager&) = delete;
        SharedMemoryManager& operator=(const SharedMemoryManager&) = delete;
    };

} // namespace RainmeterManager::Render
