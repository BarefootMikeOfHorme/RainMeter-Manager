#pragma once

// Debug utilities for RainmeterManager

#include <string>
#include <iostream>
#include <chrono>
#include <cassert>
#include "logger.h"

// Debug build detection
#if defined(_DEBUG) || defined(DEBUG)
    #define RM_DEBUG_BUILD 1
#else
    #define RM_DEBUG_BUILD 0
#endif

// Debug assertion with message
#define RM_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            LOG_ERROR("Assertion failed: " #condition ". " message); \
            assert(condition); \
        } \
    } while (0)

// Debug-only code block
#define RM_DEBUG_ONLY(code) \
    do { \
        if (RM_DEBUG_BUILD) { \
            code; \
        } \
    } while (0)

// Debug logging macros
#define RM_DEBUG_LOG(message) \
    RM_DEBUG_ONLY(LOG_DEBUG(message))

#define RM_DEBUG_LOG_VERBOSE(message) \
    RM_DEBUG_ONLY(LOG_DEBUG("[VERBOSE] " + std::string(message)))

// Performance timer for debug builds
class DebugTimer {
public:
    DebugTimer(const std::string& operation) : operation_(operation) {
        if (RM_DEBUG_BUILD) {
            LOG_DEBUG("Starting timer: " + operation_);
            start_ = std::chrono::high_resolution_clock::now();
        }
    }
    
    ~DebugTimer() {
        if (RM_DEBUG_BUILD) {
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> duration = end - start_;
            LOG_DEBUG("Timer ended: " + operation_ + " took " + 
                     std::to_string(duration.count()) + " ms");
        }
    }

private:
    std::string operation_;
    std::chrono::high_resolution_clock::time_point start_;
};

// Memory tracking in debug builds
class DebugMemoryTracker {
public:
    static void TrackAllocation(void* ptr, size_t size, const char* file, int line) {
        #if RM_DEBUG_BUILD
            // Implementation would go here - this is just a placeholder
            LOG_DEBUG("Memory allocated: " + std::to_string(size) + 
                     " bytes at " + std::to_string((uintptr_t)ptr) +
                     " (" + std::string(file) + ":" + std::to_string(line) + ")");
        #endif
    }
    
    static void TrackDeallocation(void* ptr, const char* file, int line) {
        #if RM_DEBUG_BUILD
            // Implementation would go here - this is just a placeholder
            LOG_DEBUG("Memory freed at " + std::to_string((uintptr_t)ptr) +
                     " (" + std::string(file) + ":" + std::to_string(line) + ")");
        #endif
    }
    
    static void PrintMemoryLeaks() {
        #if RM_DEBUG_BUILD
            // Implementation would go here - this is just a placeholder
            LOG_DEBUG("Memory leak report would go here");
        #endif
    }
};

// Usage: Use MEASURE_FUNCTION at the start of a function to time its execution
#define MEASURE_FUNCTION DebugTimer _debug_timer_##__LINE__(__FUNCTION__)

// Usage: MEASURE_BLOCK("Description of operation")
#define MEASURE_BLOCK(description) DebugTimer _debug_timer_##__LINE__(description)
