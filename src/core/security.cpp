// Security implementation file - Enterprise-grade security framework
// Copyright (c) 2025 BarefootMikeOfHorme. All rights reserved.

#include "security.h"
#include "logger.h"
#include "error_handling.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <vector>
#include <iomanip>
#include <wintrust.h>
#include <softpub.h>
#include <wincrypt.h>
#include <bcrypt.h>
#include <dpapi.h>
#include <winhttp.h>

#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "winhttp.lib")

// Static crypto provider handles
static BCRYPT_ALG_HANDLE g_hAesAlg = nullptr;
static BCRYPT_ALG_HANDLE g_hSha256Alg = nullptr;
static bool g_cryptoInitialized = false;
static std::mutex g_cryptoMutex;

//=============================================================================
// Crypto Provider Management
//=============================================================================

bool Security::Initialize() {
    return initializeCrypto();
}

void Security::Cleanup() {
    cleanupCrypto();
}

bool Security::initializeCrypto() {
    std::lock_guard<std::mutex> lock(g_cryptoMutex);
    
    if (g_cryptoInitialized) {
        return true;
    }
    
    NTSTATUS status;
    
    // Initialize AES algorithm provider
    status = BCryptOpenAlgorithmProvider(
        &g_hAesAlg,
        BCRYPT_AES_ALGORITHM,
        nullptr,
        0
    );
    
    if (!BCRYPT_SUCCESS(status)) {
        LOG_ERROR("Failed to initialize AES algorithm provider: 0x" + 
                 std::to_string(status));
        return false;
    }
    
    // Set AES to GCM mode
    status = BCryptSetProperty(
        g_hAesAlg,
        BCRYPT_CHAINING_MODE,
        (PUCHAR)BCRYPT_CHAIN_MODE_GCM,
        sizeof(BCRYPT_CHAIN_MODE_GCM),
        0
    );
    
    if (!BCRYPT_SUCCESS(status)) {
        LOG_ERROR("Failed to set AES to GCM mode: 0x" + std::to_string(status));
        cleanupCrypto();
        return false;
    }
    
    // Initialize SHA-256 algorithm provider
    status = BCryptOpenAlgorithmProvider(
        &g_hSha256Alg,
        BCRYPT_SHA256_ALGORITHM,
        nullptr,
        0
    );
    
    if (!BCRYPT_SUCCESS(status)) {
        LOG_ERROR("Failed to initialize SHA-256 algorithm provider: 0x" + 
                 std::to_string(status));
        cleanupCrypto();
        return false;
    }
    
    g_cryptoInitialized = true;
    LOG_INFO("Crypto providers initialized successfully");
    return true;
}

void Security::cleanupCrypto() {
    std::lock_guard<std::mutex> lock(g_cryptoMutex);
    
    if (g_hAesAlg) {
        BCryptCloseAlgorithmProvider(g_hAesAlg, 0);
        g_hAesAlg = nullptr;
    }
    
    if (g_hSha256Alg) {
        BCryptCloseAlgorithmProvider(g_hSha256Alg, 0);
        g_hSha256Alg = nullptr;
    }
    
    g_cryptoInitialized = false;
    LOG_INFO("Crypto providers cleaned up");
}

//=============================================================================
// SHA-256 Hash Calculation
//=============================================================================

std::string Security::calculateSHA256(const std::string& filePath) {
    if (!g_cryptoInitialized) {
        LOG_ERROR("Crypto not initialized for SHA-256 calculation");
        return "";
    }
    
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file for SHA-256: " + filePath);
        return "";
    }
    
    // Read file in chunks
    const size_t BUFFER_SIZE = 8192;
    std::vector<char> buffer(BUFFER_SIZE);
    
    BCRYPT_HASH_HANDLE hHash = nullptr;
    NTSTATUS status = BCryptCreateHash(g_hSha256Alg, &hHash, nullptr, 0, nullptr, 0, 0);
    
    if (!BCRYPT_SUCCESS(status)) {
        LOG_ERROR("Failed to create SHA-256 hash object: 0x" + std::to_string(status));
        return "";
    }
    
    // Hash file content
    while (file.read(buffer.data(), BUFFER_SIZE) || file.gcount() > 0) {
        status = BCryptHashData(hHash, (PUCHAR)buffer.data(), 
                               static_cast<ULONG>(file.gcount()), 0);
        
        if (!BCRYPT_SUCCESS(status)) {
            LOG_ERROR("Failed to hash data: 0x" + std::to_string(status));
            BCryptDestroyHash(hHash);
            return "";
        }
    }
    
    // Finalize hash
    DWORD hashSize = 32; // SHA-256 produces 32-byte hash
    std::vector<BYTE> hashBytes(hashSize);
    
    status = BCryptFinishHash(hHash, hashBytes.data(), hashSize, 0);
    BCryptDestroyHash(hHash);
    
    if (!BCRYPT_SUCCESS(status)) {
        LOG_ERROR("Failed to finalize SHA-256 hash: 0x" + std::to_string(status));
        return "";
    }
    
    // Convert to hex string
    std::stringstream ss;
    for (BYTE b : hashBytes) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(b);
    }
    
    return ss.str();
}

std::string Security::calculateSHA256FromBuffer(const char* data, size_t length) {
    if (!g_cryptoInitialized || !data) {
        LOG_ERROR("Invalid parameters for SHA-256 buffer calculation");
        return "";
    }
    
    BCRYPT_HASH_HANDLE hHash = nullptr;
    NTSTATUS status = BCryptCreateHash(g_hSha256Alg, &hHash, nullptr, 0, nullptr, 0, 0);
    
    if (!BCRYPT_SUCCESS(status)) {
        LOG_ERROR("Failed to create SHA-256 hash object: 0x" + std::to_string(status));
        return "";
    }
    
    status = BCryptHashData(hHash, (PUCHAR)data, static_cast<ULONG>(length), 0);
    if (!BCRYPT_SUCCESS(status)) {
        LOG_ERROR("Failed to hash buffer data: 0x" + std::to_string(status));
        BCryptDestroyHash(hHash);
        return "";
    }
    
    DWORD hashSize = 32;
    std::vector<BYTE> hashBytes(hashSize);
    
    status = BCryptFinishHash(hHash, hashBytes.data(), hashSize, 0);
    BCryptDestroyHash(hHash);
    
    if (!BCRYPT_SUCCESS(status)) {
        LOG_ERROR("Failed to finalize buffer SHA-256 hash: 0x" + std::to_string(status));
        return "";
    }
    
    std::stringstream ss;
    for (BYTE b : hashBytes) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(b);
    }
    
    return ss.str();
}

//=============================================================================
// Code Signature Validation
//=============================================================================

bool Security::checkFileSignature(const std::string& filePath) {
    std::wstring wFilePath(filePath.begin(), filePath.end());
    
    WINTRUST_FILE_INFO fileInfo = {0};
    fileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
    fileInfo.pcwszFilePath = wFilePath.c_str();
    
    WINTRUST_DATA trustData = {0};
    trustData.cbStruct = sizeof(WINTRUST_DATA);
    trustData.dwUIChoice = WTD_UI_NONE;
    trustData.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;
    trustData.dwUnionChoice = WTD_CHOICE_FILE;
    trustData.pFile = &fileInfo;
    trustData.dwStateAction = WTD_STATEACTION_VERIFY;
    trustData.dwProvFlags = WTD_SAFER_FLAG | WTD_CACHE_ONLY_URL_RETRIEVAL;
    
    GUID actionGuid = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    
    LONG result = WinVerifyTrust(NULL, &actionGuid, &trustData);
    
    // Clean up
    trustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(NULL, &actionGuid, &trustData);
    
    bool isValid = (result == ERROR_SUCCESS);
    
    if (isValid) {
        LOG_INFO("Code signature validation successful: " + filePath);
    } else {
        LOG_WARNING("Code signature validation failed: " + filePath + 
                   " (Error: 0x" + std::to_string(result) + ")");
        Logger::logSecurityEvent("Code Signature Failure", filePath);
    }
    
    return isValid;
}

bool Security::isFromTrustedSource(const std::string& filePath) {
    // First check code signature
    if (!checkFileSignature(filePath)) {
        return false;
    }
    
    // Additional trusted source validation could include:
    // - Certificate chain validation
    // - Publisher whitelist checking
    // - Digital timestamp validation
    
    LOG_INFO("File validated as from trusted source: " + filePath);
    return true;
}

//=============================================================================
// Malicious Pattern Detection
//=============================================================================

std::vector<std::string> Security::getMaliciousPatterns() {
    return {
        // Script injection patterns
        R"(<script[^>]*>.*?</script>)",
        R"(javascript:)",
        R"(vbscript:)",
        R"(onload\s*=)",
        R"(onerror\s*=)",
        
        // SQL injection patterns
        R"(;\s*(drop|delete|insert|update|select)\s+)",
        R"(union\s+select)",
        R"('\s*or\s+'\s*=\s*')",
        
        // Path traversal patterns
        R"(\.\./)",
        R"(\\\.\.\\)",
        
        // Command injection patterns
        R"(;\s*(cmd|powershell|bash)\s+)",
        R"(&\s*[a-zA-Z]+\s*&)",
        
        // Registry manipulation
        R"(HKEY_(CURRENT_USER|LOCAL_MACHINE|CLASSES_ROOT))",
        
        // Suspicious file operations
        R"(CreateProcess[AW]?)",
        R"(ShellExecute[AW]?)",
        R"(WinExec)"
    };
}

bool Security::scanForMaliciousPatterns(const std::string& content) {
    auto patterns = getMaliciousPatterns();
    
    for (const auto& pattern : patterns) {
        try {
            std::regex regex(pattern, std::regex_constants::icase);
            if (std::regex_search(content, regex)) {
                LOG_WARNING("Malicious pattern detected: " + pattern);
                Logger::logSecurityEvent("Malicious Pattern Detection", pattern);
                return true; // Found malicious content
            }
        } catch (const std::regex_error& e) {
            LOG_ERROR("Regex error in pattern matching: " + std::string(e.what()));
        }
    }
    
    return false; // No malicious patterns found
}

std::vector<std::string> Security::getAllowedExtensions() {
    return {
        ".rmskin",   // Rainmeter skin packages
        ".ini",      // Configuration files
        ".txt",      // Text files
        ".xml",      // XML configuration
        ".json",     // JSON data
        ".png",      // Images
        ".jpg",
        ".jpeg",
        ".bmp",
        ".gif",
        ".ico",
        ".wav",      // Audio
        ".mp3",
        ".ogg"
    };
}

bool Security::validateFileExtension(const std::string& filePath) {
    auto allowedExtensions = getAllowedExtensions();
    
    // Extract file extension
    size_t dotPos = filePath.find_last_of(".");
    if (dotPos == std::string::npos) {
        LOG_WARNING("File has no extension: " + filePath);
        return false;
    }
    
    std::string extension = filePath.substr(dotPos);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    bool isAllowed = std::find(allowedExtensions.begin(), allowedExtensions.end(), 
                              extension) != allowedExtensions.end();
    
    if (!isAllowed) {
        LOG_WARNING("Disallowed file extension: " + extension + " in " + filePath);
        Logger::logSecurityEvent("Disallowed File Extension", filePath);
    }
    
    return isAllowed;
}

//=============================================================================
// Security Sweep Operations
//=============================================================================

SecurityResult Security::performSecuritySweep(const std::string& filePath) {
    SecurityResult result = {false, "", {}, ""};
    
    try {
        // Step 1: Validate file extension
        if (!validateFileExtension(filePath)) {
            result.threats.push_back("Disallowed file extension");
        }
        
        // Step 2: Calculate file hash
        result.hash = calculateSHA256(filePath);
        if (result.hash.empty()) {
            result.threats.push_back("Failed to calculate file hash");
        }
        
        // Step 3: Check code signature (for executable files)
        std::string ext = filePath.substr(filePath.find_last_of("."));
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext == ".exe" || ext == ".dll" || ext == ".msi") {
            if (!checkFileSignature(filePath)) {
                result.threats.push_back("Invalid or missing code signature");
            }
        }
        
        // Step 4: Scan file content for malicious patterns
        std::ifstream file(filePath, std::ios::binary);
        if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            
            if (scanForMaliciousPatterns(content)) {
                result.threats.push_back("Malicious patterns detected in file content");
            }
        } else {
            result.threats.push_back("Unable to read file for content analysis");
        }
        
        // Determine overall security status
        result.isSecure = result.threats.empty();
        
        if (result.isSecure) {
            LOG_INFO("Security sweep passed: " + filePath);
        } else {
            std::stringstream ss;
            ss << "Security sweep failed for " << filePath << ": ";
            for (size_t i = 0; i < result.threats.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << result.threats[i];
            }
            LOG_WARNING(ss.str());
            Logger::logSecurityEvent("Security Sweep Failed", filePath);
        }
        
    } catch (const std::exception& e) {
        result.errorMessage = "Security sweep exception: " + std::string(e.what());
        result.threats.push_back("Security sweep failed with exception");
        LOG_ERROR(result.errorMessage);
    }
    
    return result;
}

bool Security::validateFileIntegrity(const std::string& filePath, const std::string& expectedHash) {
    std::string actualHash = calculateSHA256(filePath);
    
    if (actualHash.empty()) {
        LOG_ERROR("Failed to calculate hash for integrity check: " + filePath);
        return false;
    }
    
    bool isValid = (actualHash == expectedHash);
    
    if (isValid) {
        LOG_INFO("File integrity validation passed: " + filePath);
    } else {
        LOG_CRITICAL("File integrity validation FAILED: " + filePath);
        LOG_CRITICAL("Expected: " + expectedHash);
        LOG_CRITICAL("Actual:   " + actualHash);
        Logger::logSecurityEvent("File Integrity Failure", filePath);
    }
    
    return isValid;
}

//=============================================================================
// Retry Mechanisms
//=============================================================================

bool Security::retryOperation(std::function<bool()> operation, int maxRetries) {
    for (int attempt = 1; attempt <= maxRetries; ++attempt) {
        try {
            if (operation()) {
                if (attempt > 1) {
                    LOG_INFO("Security operation succeeded on attempt " + std::to_string(attempt));
                }
                return true;
            }
        } catch (const std::exception& e) {
            LOG_WARNING("Security operation attempt " + std::to_string(attempt) + 
                       " failed with exception: " + e.what());
        }
        
        if (attempt < maxRetries) {
            LOG_INFO("Retrying security operation (attempt " + 
                    std::to_string(attempt + 1) + "/" + std::to_string(maxRetries) + ")");
            Sleep(1000 * attempt); // Progressive backoff
        }
    }
    
    LOG_ERROR("Security operation failed after " + std::to_string(maxRetries) + " attempts");
    return false;
}

//=============================================================================
// Helper Methods
//=============================================================================

bool Security::isFileSafe(const std::string& content) {
    return !scanForMaliciousPatterns(content);
}
