#ifndef SECURITY_H
#define SECURITY_H

#include <string>
#include <fstream>
#include <vector>
#include <functional>
#include <windows.h>
#include <wincrypt.h>  // Windows Crypto API
#include <memory>
#include <mutex>  // For thread safety
#include "logger.h"  // Include logger for integrated error handling

// Security result structure
struct SecurityResult {
    bool isSecure;
    std::string hash;
    std::vector<std::string> threats;
    std::string errorMessage;
};

class Security {
public:
    // SHA-256 hash calculation
    static std::string calculateSHA256(const std::string& filePath);
    static std::string calculateSHA256FromBuffer(const char* data, size_t length);
    
    // Security sweep operations
    static SecurityResult performSecuritySweep(const std::string& filePath);
    static bool validateFileIntegrity(const std::string& filePath, const std::string& expectedHash);
    
    // File content security checks
    static bool scanForMaliciousPatterns(const std::string& content);
    static bool validateFileExtension(const std::string& filePath);
    
    // Windows-specific security
    static bool checkFileSignature(const std::string& filePath);
    static bool isFromTrustedSource(const std::string& filePath);
    
    // Retry mechanisms
    static bool retryOperation(std::function<bool()> operation, int maxRetries = 3);
    
private:
    // Helper methods
    static bool isFileSafe(const std::string& content);
    static std::vector<std::string> getMaliciousPatterns();
    static std::vector<std::string> getAllowedExtensions();
    static bool initializeCrypto();
    static void cleanupCrypto();
};

#endif // SECURITY_H
