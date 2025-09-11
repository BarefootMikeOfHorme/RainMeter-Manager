#include "CrashHandler.h"
#include <dbghelp.h>
#include <sstream>
#include <iomanip>
#include "../core/logger.h"

#pragma comment(lib, "dbghelp.lib")

namespace RainmeterManager {
namespace Crash {

// Static member definitions
bool CrashHandler::initialized_ = false;
void* CrashHandler::hVectoredHandler_ = nullptr;

void CrashHandler::Install() {
    if (initialized_) {
        return; // Already installed
    }
    
    // Add vectored exception handler (first in chain)
    hVectoredHandler_ = AddVectoredExceptionHandler(1, VectoredExceptionHandler);
    
    if (hVectoredHandler_) {
        initialized_ = true;
        LOG_INFO("CrashHandler installed successfully");
    } else {
        LOG_ERROR("Failed to install CrashHandler");
    }
}

LONG CALLBACK CrashHandler::VectoredExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo) {
    // Handle the exception
    HandleException(pExceptionInfo);
    
    // Continue search to allow other handlers (including WER) to process
    return EXCEPTION_CONTINUE_SEARCH;
}

int CrashHandler::HandleException(EXCEPTION_POINTERS* pExceptionInfo) {
    static bool handlingException = false;
    
    // Prevent recursive crashes
    if (handlingException) {
        return EXCEPTION_EXECUTE_HANDLER;
    }
    handlingException = true;
    
    try {
        DWORD exceptionCode = pExceptionInfo->ExceptionRecord->ExceptionCode;
        PVOID exceptionAddress = pExceptionInfo->ExceptionRecord->ExceptionAddress;
        
        // Log exception info
        std::stringstream ss;
        ss << "CRASH DETECTED - Code: 0x" << std::hex << std::setw(8) << std::setfill('0') << exceptionCode
           << ", Address: 0x" << std::setw(16) << reinterpret_cast<uintptr_t>(exceptionAddress);
        LOG_CRITICAL(ss.str());
        
        // Generate minidump
        bool dumpCreated = GenerateMinidump(pExceptionInfo);
        if (dumpCreated) {
            LOG_INFO("Crash minidump generated successfully");
        } else {
            LOG_ERROR("Failed to generate crash minidump");
        }
        
        // Log stack trace
        LogStackTrace(pExceptionInfo);
        
    } catch (...) {
        // If crash handling fails, just terminate
    }
    
    handlingException = false;
    return EXCEPTION_EXECUTE_HANDLER;
}

bool CrashHandler::GenerateMinidump(EXCEPTION_POINTERS* pExceptionInfo, const wchar_t* filename) {
    try {
        // Ensure dumps directory exists
        CreateDirectoryW(L"dumps", nullptr);
        
        wchar_t dumpFile[MAX_PATH];
        if (filename) {
            wcscpy_s(dumpFile, MAX_PATH, filename);
        } else {
            // Generate timestamp-based filename
            SYSTEMTIME st;
            GetLocalTime(&st);
            swprintf_s(dumpFile, MAX_PATH, L"dumps\\\\RainmeterManager_%04d%02d%02d_%02d%02d%02d.dmp",
                      st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        }
        
        // Create dump file
        HANDLE hFile = CreateFileW(dumpFile, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                                  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        
        if (hFile == INVALID_HANDLE_VALUE) {
            return false;
        }
        
        // Setup minidump parameters
        MINIDUMP_EXCEPTION_INFORMATION mdei;
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = pExceptionInfo;
        mdei.ClientPointers = FALSE;
        
        // Write minidump with full memory, handles, thread info
        MINIDUMP_TYPE dumpType = static_cast<MINIDUMP_TYPE>(
            MiniDumpWithFullMemory |
            MiniDumpWithHandleData |
            MiniDumpWithThreadInfo |
            MiniDumpWithFullMemoryInfo
        );
        
        BOOL result = MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hFile,
            dumpType,
            &mdei,
            nullptr,
            nullptr
        );
        
        CloseHandle(hFile);
        
        return result != FALSE;
        
    } catch (...) {
        return false;
    }
}

void CrashHandler::LogStackTrace(EXCEPTION_POINTERS* pExceptionInfo) {
    HANDLE hProcess = GetCurrentProcess();
    HANDLE hThread = GetCurrentThread();
    
    // Initialize symbol handler
    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);
    static bool symInitialized = false;
    if (!symInitialized) {
        if (SymInitialize(hProcess, nullptr, TRUE)) {
            symInitialized = true;
        } else {
            LOG_ERROR("Failed to initialize symbol handler");
            return;
        }
    }
    
    // Make a copy of context we can modify
    CONTEXT ctx = *pExceptionInfo->ContextRecord;
    
    // Setup stack frame for walk
#if defined(_M_X64) || defined(__x86_64__)
    DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
    STACKFRAME64 frame = {};
    frame.AddrPC.Offset = ctx.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = ctx.Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = ctx.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;
#else
    DWORD machineType = IMAGE_FILE_MACHINE_I386;
    STACKFRAME64 frame = {};
    frame.AddrPC.Offset = ctx.Eip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = ctx.Ebp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = ctx.Esp;
    frame.AddrStack.Mode = AddrModeFlat;
#endif
    
    std::stringstream stackTrace;
    stackTrace << "\\n=== STACK TRACE ===\\n";
    
    for (int frameNum = 0; frameNum < 64; ++frameNum) {
        BOOL walkResult = StackWalk64(
            machineType,
            hProcess,
            hThread,
            &frame,
            &ctx,
            nullptr,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            nullptr
        );
        
        if (!walkResult || frame.AddrPC.Offset == 0) {
            break;
        }
        
        DWORD64 addr = frame.AddrPC.Offset;
        
        // Get module info
        IMAGEHLP_MODULE64 modInfo = {};
        modInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
        const char* moduleName = "Unknown";
        if (SymGetModuleInfo64(hProcess, addr, &modInfo)) {
            moduleName = modInfo.ModuleName;
        }
        
        // Get symbol info
        BYTE symBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
        PSYMBOL_INFO pSymbol = reinterpret_cast<PSYMBOL_INFO>(symBuffer);
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME;
        
        DWORD64 displacement = 0;
        if (SymFromAddr(hProcess, addr, &displacement, pSymbol)) {
            // Try to get file/line info
            DWORD dwDisplacement = 0;
            IMAGEHLP_LINE64 line = {};
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            
            if (SymGetLineFromAddr64(hProcess, addr, &dwDisplacement, &line)) {
                stackTrace << "  [" << std::dec << frameNum << "] "
                          << moduleName << "!" << pSymbol->Name
                          << "+0x" << std::hex << displacement
                          << " (" << line.FileName << ":" << std::dec << line.LineNumber << ")\\n";
            } else {
                stackTrace << "  [" << std::dec << frameNum << "] "
                          << moduleName << "!" << pSymbol->Name
                          << "+0x" << std::hex << displacement
                          << " [0x" << addr << "]\\n";
            }
        } else {
            stackTrace << "  [" << std::dec << frameNum << "] "
                      << moduleName << " [0x" << std::hex << addr << "]\\n";
        }
    }
    
    stackTrace << "===================\\n";
    LOG_CRITICAL(stackTrace.str());
}

} // namespace Crash
} // namespace RainmeterManager
