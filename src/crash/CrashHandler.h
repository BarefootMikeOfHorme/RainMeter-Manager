#pragma once
#include <windows.h>

namespace RainmeterManager {
namespace Crash {

/**
 * @brief Crash handling subsystem for enterprise-grade crash reporting
 * 
 * Provides minidump generation, stack symbolication, and crash recovery
 */
class CrashHandler {
public:
    /**
     * @brief Install crash handler with vectored exception handling
     */
    static void Install();
    
    /**
     * @brief Handle exception and generate crash artifacts
     * @return EXCEPTION_EXECUTE_HANDLER to terminate, or EXCEPTION_CONTINUE_SEARCH
     */
    static int HandleException(EXCEPTION_POINTERS* pExceptionInfo);
    
    /**
     * @brief Generate minidump file with full memory
     * @param pExceptionInfo Exception context
     * @param filename Optional custom filename (nullptr for timestamp-based)
     * @return true if dump generated successfully
     */
    static bool GenerateMinidump(EXCEPTION_POINTERS* pExceptionInfo, const wchar_t* filename = nullptr);
    
    /**
     * @brief Log symbolicated stack trace to log file
     */
    static void LogStackTrace(EXCEPTION_POINTERS* pExceptionInfo);
    
private:
    static LONG CALLBACK VectoredExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo);
    static bool initialized_;
    static void* hVectoredHandler_;
};

} // namespace Crash
} // namespace RainmeterManager
