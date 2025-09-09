#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <windows.h>
#include <memory>
#include <thread>
#include <chrono>

// Test subjects
#include "../../src/core/logger.h"
#include "../../src/core/security.h"
#include "../../src/core/service_locator.h"
#include "../../src/core/privacy_manager.h"
#include "../../src/config/config_manager.h"

using namespace RainmeterManager::Core;
using namespace RainmeterManager::Config;
using namespace testing;

// Test fixture for Logger
class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any existing logger instance
        Logger::DestroyInstance();
    }
    
    void TearDown() override {
        Logger::DestroyInstance();
    }
};

// Test fixture for Security
class SecurityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize security framework
        ASSERT_TRUE(Security::Initialize());
    }
    
    void TearDown() override {
        Security::Cleanup();
    }
};

// Test fixture for ServiceLocator
class ServiceLocatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        ServiceLocator::DestroyInstance();
    }
    
    void TearDown() override {
        ServiceLocator::DestroyInstance();
    }
};

// Test fixture for PrivacyManager
class PrivacyManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        PrivacyManager::DestroyInstance();
    }
    
    void TearDown() override {
        PrivacyManager::DestroyInstance();
    }
};

// Test fixture for ConfigManager
class ConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        ConfigManager::DestroyInstance();
        // Create temporary config file
        tempConfigPath_ = L"test_config.json";
    }
    
    void TearDown() override {
        ConfigManager::DestroyInstance();
        // Clean up temp files
        DeleteFile(tempConfigPath_.c_str());
    }
    
    std::wstring tempConfigPath_;
};

// Mock interfaces for testing
class MockService {
public:
    virtual ~MockService() = default;
    virtual void DoSomething() = 0;
    virtual int GetValue() const = 0;
};

class MockServiceImpl : public MockService {
public:
    MOCK_METHOD(void, DoSomething, (), (override));
    MOCK_METHOD(int, GetValue, (), (const, override));
};

// Logger Tests
TEST_F(LoggerTest, SingletonCreation) {
    Logger& logger1 = Logger::GetInstance();
    Logger& logger2 = Logger::GetInstance();
    EXPECT_EQ(&logger1, &logger2);
}

TEST_F(LoggerTest, BasicLogging) {
    Logger& logger = Logger::GetInstance();
    
    // Test different log levels
    EXPECT_NO_THROW(logger.Log(LogLevel::Info, "Test", "Info message"));
    EXPECT_NO_THROW(logger.Log(LogLevel::Warning, "Test", "Warning message"));
    EXPECT_NO_THROW(logger.Log(LogLevel::Error, "Test", "Error message"));
}

TEST_F(LoggerTest, WideStringLogging) {
    Logger& logger = Logger::GetInstance();
    
    EXPECT_NO_THROW(logger.LogWide(LogLevel::Info, L"Test", L"Wide string message"));
}

TEST_F(LoggerTest, ThreadSafety) {
    Logger& logger = Logger::GetInstance();
    const int numThreads = 10;
    const int messagesPerThread = 100;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&logger, i, messagesPerThread]() {
            for (int j = 0; j < messagesPerThread; ++j) {
                logger.Log(LogLevel::Info, "ThreadTest", 
                          "Thread " + std::to_string(i) + " Message " + std::to_string(j));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // If we get here without crashing, thread safety test passed
    SUCCEED();
}

// Security Tests
TEST_F(SecurityTest, AESEncryptionDecryption) {
    std::string plaintext = "Hello, World! This is a test message for AES encryption.";
    std::string key = "MySecretKey12345";
    
    std::string encrypted = Security::EncryptAES(plaintext, key);
    EXPECT_FALSE(encrypted.empty());
    EXPECT_NE(encrypted, plaintext);
    
    std::string decrypted = Security::DecryptAES(encrypted, key);
    EXPECT_EQ(decrypted, plaintext);
}

TEST_F(SecurityTest, AESEncryptionDifferentKeys) {
    std::string plaintext = "Test message";
    std::string key1 = "Key1234567890123";
    std::string key2 = "DifferentKey1234";
    
    std::string encrypted1 = Security::EncryptAES(plaintext, key1);
    std::string encrypted2 = Security::EncryptAES(plaintext, key2);
    
    EXPECT_NE(encrypted1, encrypted2);
}

TEST_F(SecurityTest, HashFunctions) {
    std::string input = "Test input for hashing";
    
    std::string hash1 = Security::SHA256Hash(input);
    std::string hash2 = Security::SHA256Hash(input);
    
    EXPECT_FALSE(hash1.empty());
    EXPECT_EQ(hash1, hash2); // Same input should produce same hash
    
    std::string differentInput = "Different input";
    std::string hash3 = Security::SHA256Hash(differentInput);
    EXPECT_NE(hash1, hash3); // Different inputs should produce different hashes
}

TEST_F(SecurityTest, DPAPICredentialStorage) {
    std::string testData = "TestCredentialData";
    
    bool stored = Security::StoreCredential("TestKey", testData);
    EXPECT_TRUE(stored);
    
    std::string retrieved = Security::RetrieveCredential("TestKey");
    EXPECT_EQ(retrieved, testData);
    
    bool deleted = Security::DeleteCredential("TestKey");
    EXPECT_TRUE(deleted);
    
    std::string retrievedAfterDelete = Security::RetrieveCredential("TestKey");
    EXPECT_TRUE(retrievedAfterDelete.empty());
}

// Service Locator Tests
TEST_F(ServiceLocatorTest, ServiceRegistrationAndResolution) {
    ServiceLocator& locator = ServiceLocator::GetInstance();
    
    auto mockService = std::make_shared<MockServiceImpl>();
    EXPECT_CALL(*mockService, GetValue()).WillRepeatedly(Return(42));
    
    // Register service
    EXPECT_TRUE(locator.RegisterService<MockService>(mockService));
    
    // Resolve service
    auto resolvedService = locator.ResolveService<MockService>();
    EXPECT_NE(resolvedService, nullptr);
    EXPECT_EQ(resolvedService->GetValue(), 42);
}

TEST_F(ServiceLocatorTest, SingletonLifetime) {
    ServiceLocator& locator = ServiceLocator::GetInstance();
    
    auto mockService = std::make_shared<MockServiceImpl>();
    
    EXPECT_TRUE(locator.RegisterService<MockService>(mockService, ServiceLifetime::Singleton));
    
    auto service1 = locator.ResolveService<MockService>();
    auto service2 = locator.ResolveService<MockService>();
    
    EXPECT_EQ(service1.get(), service2.get()); // Same instance for singleton
}

TEST_F(ServiceLocatorTest, NamedServices) {
    ServiceLocator& locator = ServiceLocator::GetInstance();
    
    auto service1 = std::make_shared<MockServiceImpl>();
    auto service2 = std::make_shared<MockServiceImpl>();
    
    EXPECT_CALL(*service1, GetValue()).WillRepeatedly(Return(1));
    EXPECT_CALL(*service2, GetValue()).WillRepeatedly(Return(2));
    
    EXPECT_TRUE(locator.RegisterService<MockService>("Service1", service1));
    EXPECT_TRUE(locator.RegisterService<MockService>("Service2", service2));
    
    auto resolved1 = locator.ResolveService<MockService>("Service1");
    auto resolved2 = locator.ResolveService<MockService>("Service2");
    
    EXPECT_EQ(resolved1->GetValue(), 1);
    EXPECT_EQ(resolved2->GetValue(), 2);
}

TEST_F(ServiceLocatorTest, ServiceUnregistration) {
    ServiceLocator& locator = ServiceLocator::GetInstance();
    
    auto mockService = std::make_shared<MockServiceImpl>();
    EXPECT_TRUE(locator.RegisterService<MockService>(mockService));
    
    auto resolved = locator.ResolveService<MockService>();
    EXPECT_NE(resolved, nullptr);
    
    EXPECT_TRUE(locator.UnregisterService<MockService>());
    
    auto resolvedAfterUnregister = locator.ResolveService<MockService>();
    EXPECT_EQ(resolvedAfterUnregister, nullptr);
}

// Privacy Manager Tests
TEST_F(PrivacyManagerTest, PrivacyLevelManagement) {
    PrivacyManager& privacy = PrivacyManager::GetInstance();
    EXPECT_TRUE(privacy.Initialize());
    
    // Test default privacy level
    EXPECT_EQ(privacy.GetPrivacyLevel(), PrivacyLevel::ESSENTIAL);
    
    // Test setting privacy levels
    EXPECT_TRUE(privacy.SetPrivacyLevel(PrivacyLevel::FULL, false));
    EXPECT_EQ(privacy.GetPrivacyLevel(), PrivacyLevel::FULL);
    
    EXPECT_TRUE(privacy.SetPrivacyLevel(PrivacyLevel::NONE, false));
    EXPECT_EQ(privacy.GetPrivacyLevel(), PrivacyLevel::NONE);
}

TEST_F(PrivacyManagerTest, TelemetryToggle) {
    PrivacyManager& privacy = PrivacyManager::GetInstance();
    EXPECT_TRUE(privacy.Initialize());
    
    // Test telemetry enable/disable
    EXPECT_TRUE(privacy.SetTelemetryEnabled(true, false));
    EXPECT_TRUE(privacy.IsTelemetryEnabled());
    
    EXPECT_TRUE(privacy.SetTelemetryEnabled(false, false));
    EXPECT_FALSE(privacy.IsTelemetryEnabled());
}

TEST_F(PrivacyManagerTest, EventFiltering) {
    PrivacyManager& privacy = PrivacyManager::GetInstance();
    EXPECT_TRUE(privacy.Initialize());
    
    // Set to NONE level - should block all events except essential
    privacy.SetPrivacyLevel(PrivacyLevel::NONE, false);
    EXPECT_TRUE(privacy.IsEventTypeAllowed(TelemetryType::ERROR_REPORTING));
    EXPECT_FALSE(privacy.IsEventTypeAllowed(TelemetryType::USAGE_ANALYTICS));
    
    // Set to FULL level - should allow all events
    privacy.SetPrivacyLevel(PrivacyLevel::FULL, false);
    EXPECT_TRUE(privacy.IsEventTypeAllowed(TelemetryType::ERROR_REPORTING));
    EXPECT_TRUE(privacy.IsEventTypeAllowed(TelemetryType::USAGE_ANALYTICS));
    EXPECT_TRUE(privacy.IsEventTypeAllowed(TelemetryType::PERFORMANCE_METRICS));
}

TEST_F(PrivacyManagerTest, PIIDetection) {
    PrivacyManager& privacy = PrivacyManager::GetInstance();
    EXPECT_TRUE(privacy.Initialize());
    
    // Test PII detection
    EXPECT_TRUE(privacy.ContainsPII("john.doe@example.com"));
    EXPECT_TRUE(privacy.ContainsPII("123-45-6789")); // SSN pattern
    EXPECT_TRUE(privacy.ContainsPII("4111-1111-1111-1111")); // Credit card pattern
    EXPECT_FALSE(privacy.ContainsPII("This is just normal text"));
    
    // Test PII sanitization
    std::string piiData = "User email: john.doe@example.com";
    std::string sanitized = privacy.SanitizeData(piiData);
    EXPECT_FALSE(privacy.ContainsPII(sanitized));
}

// Config Manager Tests
TEST_F(ConfigManagerTest, BasicConfigOperations) {
    ConfigManager& config = ConfigManager::GetInstance();
    EXPECT_TRUE(config.Initialize(tempConfigPath_));
    
    // Test setting and getting values
    EXPECT_TRUE(config.SetValue("test_string", "Hello World"));
    EXPECT_TRUE(config.SetValue("test_int", 42));
    EXPECT_TRUE(config.SetValue("test_bool", true));
    
    std::string stringValue = config.GetValue<std::string>("test_string", "");
    int intValue = config.GetValue<int>("test_int", 0);
    bool boolValue = config.GetValue<bool>("test_bool", false);
    
    EXPECT_EQ(stringValue, "Hello World");
    EXPECT_EQ(intValue, 42);
    EXPECT_TRUE(boolValue);
}

TEST_F(ConfigManagerTest, DefaultValues) {
    ConfigManager& config = ConfigManager::GetInstance();
    EXPECT_TRUE(config.Initialize(tempConfigPath_));
    
    // Test non-existent keys return default values
    std::string defaultString = config.GetValue<std::string>("nonexistent_key", "default");
    int defaultInt = config.GetValue<int>("nonexistent_key", 999);
    
    EXPECT_EQ(defaultString, "default");
    EXPECT_EQ(defaultInt, 999);
}

TEST_F(ConfigManagerTest, ConfigPersistence) {
    // First instance
    {
        ConfigManager& config = ConfigManager::GetInstance();
        EXPECT_TRUE(config.Initialize(tempConfigPath_));
        
        EXPECT_TRUE(config.SetValue("persistent_value", "test123"));
        EXPECT_TRUE(config.SaveConfig());
        
        ConfigManager::DestroyInstance();
    }
    
    // Second instance - should load persisted value
    {
        ConfigManager& config = ConfigManager::GetInstance();
        EXPECT_TRUE(config.Initialize(tempConfigPath_));
        
        std::string value = config.GetValue<std::string>("persistent_value", "");
        EXPECT_EQ(value, "test123");
    }
}

// Performance Tests
class PerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        startTime_ = std::chrono::high_resolution_clock::now();
    }
    
    void TearDown() override {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime_);
        
        // Log performance metrics
        Logger& logger = Logger::GetInstance();
        logger.Log(LogLevel::Info, "Performance", 
                  "Test " + std::string(::testing::UnitTest::GetInstance()->current_test_info()->name()) + 
                  " took " + std::to_string(duration.count()) + "ms");
    }
    
private:
    std::chrono::high_resolution_clock::time_point startTime_;
};

TEST_F(PerformanceTest, LoggerThroughput) {
    Logger& logger = Logger::GetInstance();
    const int messageCount = 10000;
    
    for (int i = 0; i < messageCount; ++i) {
        logger.Log(LogLevel::Info, "PerfTest", "Message " + std::to_string(i));
    }
    
    // Test should complete in reasonable time (logged in TearDown)
    SUCCEED();
}

TEST_F(PerformanceTest, EncryptionPerformance) {
    const int iterations = 1000;
    std::string data = "This is a test message for encryption performance testing";
    std::string key = "TestKey123456789";
    
    for (int i = 0; i < iterations; ++i) {
        std::string encrypted = Security::EncryptAES(data, key);
        std::string decrypted = Security::DecryptAES(encrypted, key);
        EXPECT_EQ(decrypted, data);
    }
    
    SUCCEED();
}

// Integration-style tests
class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up all singletons
        Logger::DestroyInstance();
        ServiceLocator::DestroyInstance();
        PrivacyManager::DestroyInstance();
        ConfigManager::DestroyInstance();
        
        // Initialize security
        ASSERT_TRUE(Security::Initialize());
    }
    
    void TearDown() override {
        // Clean up everything
        Logger::DestroyInstance();
        ServiceLocator::DestroyInstance();
        PrivacyManager::DestroyInstance();
        ConfigManager::DestroyInstance();
        Security::Cleanup();
    }
};

TEST_F(IntegrationTest, FullSystemInitialization) {
    // Test complete system startup sequence
    Logger& logger = Logger::GetInstance();
    ServiceLocator& serviceLocator = ServiceLocator::GetInstance();
    PrivacyManager& privacy = PrivacyManager::GetInstance();
    ConfigManager& config = ConfigManager::GetInstance();
    
    // Initialize in order
    EXPECT_TRUE(config.Initialize(L"test_integration_config.json"));
    EXPECT_TRUE(privacy.Initialize());
    
    logger.Log(LogLevel::Info, "Integration", "System initialization test completed");
    
    // Clean up test file
    DeleteFile(L"test_integration_config.json");
}

TEST_F(IntegrationTest, ServiceLocatorWithRealServices) {
    ServiceLocator& locator = ServiceLocator::GetInstance();
    
    // Register logger as a service (it's a real singleton)
    auto loggerPtr = std::shared_ptr<Logger>(&Logger::GetInstance(), [](Logger*){});
    EXPECT_TRUE(locator.RegisterService<Logger>(loggerPtr));
    
    // Resolve and use the service
    auto resolvedLogger = locator.ResolveService<Logger>();
    EXPECT_NE(resolvedLogger, nullptr);
    
    resolvedLogger->Log(LogLevel::Info, "Integration", "Logger resolved through service locator");
}

// Memory leak detection test (basic)
TEST(MemoryTest, NoObviousLeaks) {
    // Get initial memory usage
    PROCESS_MEMORY_COUNTERS_EX memCounters1;
    GetProcessMemoryInfo(GetCurrentProcess(), 
                        reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memCounters1), 
                        sizeof(memCounters1));
    
    // Perform operations that could leak memory
    for (int i = 0; i < 1000; ++i) {
        Logger& logger = Logger::GetInstance();
        logger.Log(LogLevel::Info, "MemTest", "Memory test iteration " + std::to_string(i));
        
        ServiceLocator& locator = ServiceLocator::GetInstance();
        auto mockService = std::make_shared<MockServiceImpl>();
        locator.RegisterService<MockService>("MemTestService", mockService);
        locator.UnregisterService<MockService>("MemTestService");
    }
    
    // Force garbage collection and get memory usage again
    Sleep(100); // Give time for cleanup
    
    PROCESS_MEMORY_COUNTERS_EX memCounters2;
    GetProcessMemoryInfo(GetCurrentProcess(), 
                        reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memCounters2), 
                        sizeof(memCounters2));
    
    // Memory should not have grown significantly
    SIZE_T memoryGrowth = memCounters2.WorkingSetSize - memCounters1.WorkingSetSize;
    
    // Allow for some growth (1MB) but flag excessive growth
    EXPECT_LT(memoryGrowth, 1024 * 1024) << "Memory growth: " << memoryGrowth << " bytes";
}

// Main function for running tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize COM for Windows APIs
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    
    int result = RUN_ALL_TESTS();
    
    CoUninitialize();
    return result;
}
