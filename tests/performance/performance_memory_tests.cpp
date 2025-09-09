#include <gtest/gtest.h>
#include <windows.h>
#include <psapi.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <memory>
#include <atomic>
#include <map>
#include <iostream>
#include <fstream>
#include <cmath>

#include "../../src/core/logger.h"
#include "../../src/core/security.h"
#include "../../src/core/service_locator.h"
#include "../../src/core/privacy_manager.h"
#include "../../src/config/config_manager.h"
#include "../../src/ui/splash_screen.h"
#include "../../src/app/rainmgrapp.h"

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "pdh.lib")

using namespace RainmeterManager::Core;
using namespace RainmeterManager::Config;
using namespace RainmeterManager::UI;
using namespace RainmeterManager::App;

// Performance Metrics Collection
struct PerformanceMetrics {
    double executionTimeMs;
    size_t peakMemoryUsageMB;
    size_t memoryLeakBytes;
    double cpuUsagePercent;
    size_t handleCount;
    size_t threadCount;
    double throughputOpsPerSec;
    
    PerformanceMetrics() : executionTimeMs(0), peakMemoryUsageMB(0), 
                          memoryLeakBytes(0), cpuUsagePercent(0),
                          handleCount(0), threadCount(0), throughputOpsPerSec(0) {}
};

// Memory Tracking Class
class MemoryTracker {
private:
    PROCESS_MEMORY_COUNTERS_EX initialMemory_;
    PROCESS_MEMORY_COUNTERS_EX peakMemory_;
    HANDLE process_;
    
public:
    MemoryTracker() : process_(GetCurrentProcess()) {
        GetProcessMemoryInfo(process_, 
                           reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&initialMemory_),
                           sizeof(initialMemory_));
        peakMemory_ = initialMemory_;
    }
    
    void UpdatePeak() {
        PROCESS_MEMORY_COUNTERS_EX current;
        if (GetProcessMemoryInfo(process_,
                               reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&current),
                               sizeof(current))) {
            if (current.WorkingSetSize > peakMemory_.WorkingSetSize) {
                peakMemory_ = current;
            }
        }
    }
    
    size_t GetCurrentMemoryMB() {
        PROCESS_MEMORY_COUNTERS_EX current;
        GetProcessMemoryInfo(process_,
                           reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&current),
                           sizeof(current));
        return current.WorkingSetSize / (1024 * 1024);
    }
    
    size_t GetPeakMemoryMB() {
        UpdatePeak();
        return peakMemory_.WorkingSetSize / (1024 * 1024);
    }
    
    size_t GetMemoryLeakBytes() {
        PROCESS_MEMORY_COUNTERS_EX current;
        GetProcessMemoryInfo(process_,
                           reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&current),
                           sizeof(current));
        return (current.WorkingSetSize > initialMemory_.WorkingSetSize) 
               ? (current.WorkingSetSize - initialMemory_.WorkingSetSize) 
               : 0;
    }
};

// CPU Usage Tracker
class CPUTracker {
private:
    PDH_HQUERY cpuQuery_;
    PDH_HCOUNTER cpuTotal_;
    bool initialized_;
    
public:
    CPUTracker() : initialized_(false) {
        PdhOpenQuery(nullptr, 0, &cpuQuery_);
        PdhAddEnglishCounter(cpuQuery_, L"\\Processor(_Total)\\% Processor Time", 0, &cpuTotal_);
        PdhCollectQueryData(cpuQuery_);
        initialized_ = true;
    }
    
    ~CPUTracker() {
        if (initialized_) {
            PdhCloseQuery(cpuQuery_);
        }
    }
    
    double GetCPUUsage() {
        if (!initialized_) return 0.0;
        
        PdhCollectQueryData(cpuQuery_);
        PDH_FMT_COUNTERVALUE counterVal;
        PdhGetFormattedCounterValue(cpuTotal_, PDH_FMT_DOUBLE, nullptr, &counterVal);
        return counterVal.doubleValue;
    }
};

// Performance Test Base Class
class PerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        memoryTracker_ = std::make_unique<MemoryTracker>();
        cpuTracker_ = std::make_unique<CPUTracker>();
        startTime_ = std::chrono::high_resolution_clock::now();
        
        Logger::GetInstance().Log(LogLevel::Info, "PerformanceTest", "Performance test started");
    }
    
    void TearDown() override {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - startTime_).count() / 1000.0; // Convert to milliseconds
        
        PerformanceMetrics metrics;
        metrics.executionTimeMs = duration;
        metrics.peakMemoryUsageMB = memoryTracker_->GetPeakMemoryMB();
        metrics.memoryLeakBytes = memoryTracker_->GetMemoryLeakBytes();
        metrics.cpuUsagePercent = cpuTracker_->GetCPUUsage();
        
        LogPerformanceMetrics(metrics);
    }
    
    void LogPerformanceMetrics(const PerformanceMetrics& metrics) {
        Logger& logger = Logger::GetInstance();
        logger.Log(LogLevel::Info, "PerformanceMetrics", 
                  "Execution Time: " + std::to_string(metrics.executionTimeMs) + "ms");
        logger.Log(LogLevel::Info, "PerformanceMetrics",
                  "Peak Memory: " + std::to_string(metrics.peakMemoryUsageMB) + "MB");
        logger.Log(LogLevel::Info, "PerformanceMetrics",
                  "Memory Leak: " + std::to_string(metrics.memoryLeakBytes) + " bytes");
        logger.Log(LogLevel::Info, "PerformanceMetrics",
                  "CPU Usage: " + std::to_string(metrics.cpuUsagePercent) + "%");
        
        // Performance thresholds
        EXPECT_LT(metrics.memoryLeakBytes, 10 * 1024 * 1024); // 10MB leak threshold
        EXPECT_LT(metrics.peakMemoryUsageMB, 500); // 500MB peak memory threshold
    }
    
    PerformanceMetrics RunBenchmark(std::function<void()> operation, int iterations = 1000) {
        PerformanceMetrics metrics;
        MemoryTracker tracker;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            operation();
            tracker.UpdatePeak();
            
            // Sample CPU usage periodically
            if (i % 100 == 0) {
                metrics.cpuUsagePercent = std::max(metrics.cpuUsagePercent, 
                                                  cpuTracker_->GetCPUUsage());
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - startTime).count() / 1000.0;
        
        metrics.executionTimeMs = duration;
        metrics.peakMemoryUsageMB = tracker.GetPeakMemoryMB();
        metrics.memoryLeakBytes = tracker.GetMemoryLeakBytes();
        metrics.throughputOpsPerSec = (iterations * 1000.0) / duration;
        
        return metrics;
    }
    
private:
    std::unique_ptr<MemoryTracker> memoryTracker_;
    std::unique_ptr<CPUTracker> cpuTracker_;
    std::chrono::high_resolution_clock::time_point startTime_;
};

// Logger Performance Tests
class LoggerPerformanceTest : public PerformanceTest {
protected:
    void SetUp() override {
        PerformanceTest::SetUp();
        Logger::DestroyInstance();
    }
    
    void TearDown() override {
        Logger::DestroyInstance();
        PerformanceTest::TearDown();
    }
};

TEST_F(LoggerPerformanceTest, HighVolumeLogging) {
    Logger& logger = Logger::GetInstance();
    const int messageCount = 10000;
    
    auto logOperation = [&logger]() {
        static int counter = 0;
        logger.Log(LogLevel::Info, "PerfTest", 
                  "Performance test message " + std::to_string(++counter));
    };
    
    PerformanceMetrics metrics = RunBenchmark(logOperation, messageCount);
    
    EXPECT_GT(metrics.throughputOpsPerSec, 1000); // At least 1000 logs/sec
    EXPECT_LT(metrics.executionTimeMs, 5000); // Complete within 5 seconds
    
    Logger::GetInstance().Log(LogLevel::Info, "LoggerPerformance", 
        "Logged " + std::to_string(messageCount) + " messages at " + 
        std::to_string(metrics.throughputOpsPerSec) + " ops/sec");
}

TEST_F(LoggerPerformanceTest, ConcurrentLoggingStress) {
    Logger& logger = Logger::GetInstance();
    const int numThreads = 10;
    const int messagesPerThread = 1000;
    
    std::atomic<int> completedThreads(0);
    std::vector<std::thread> threads;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&logger, &completedThreads, t, messagesPerThread]() {
            for (int i = 0; i < messagesPerThread; ++i) {
                logger.Log(LogLevel::Info, "ConcurrentTest", 
                          "Thread " + std::to_string(t) + " Message " + std::to_string(i));
            }
            completedThreads++;
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();
    
    EXPECT_EQ(completedThreads.load(), numThreads);
    EXPECT_LT(duration, 10000); // Complete within 10 seconds
    
    double totalMessages = numThreads * messagesPerThread;
    double throughput = (totalMessages * 1000.0) / duration;
    
    Logger::GetInstance().Log(LogLevel::Info, "ConcurrentLoggingPerformance",
        "Concurrent throughput: " + std::to_string(throughput) + " messages/sec");
}

// Security Performance Tests
class SecurityPerformanceTest : public PerformanceTest {
protected:
    void SetUp() override {
        PerformanceTest::SetUp();
        ASSERT_TRUE(Security::Initialize());
    }
    
    void TearDown() override {
        Security::Cleanup();
        PerformanceTest::TearDown();
    }
};

TEST_F(SecurityPerformanceTest, EncryptionThroughput) {
    const std::string testData = "This is test data for encryption throughput testing. "
                               "It contains enough text to make the encryption meaningful.";
    const std::string key = "PerformanceTestEncryptionKey123";
    const int iterations = 1000;
    
    auto encryptOperation = [&]() {
        std::string encrypted = Security::EncryptAES(testData, key);
        EXPECT_FALSE(encrypted.empty());
        
        std::string decrypted = Security::DecryptAES(encrypted, key);
        EXPECT_EQ(decrypted, testData);
    };
    
    PerformanceMetrics metrics = RunBenchmark(encryptOperation, iterations);
    
    EXPECT_GT(metrics.throughputOpsPerSec, 100); // At least 100 encrypt/decrypt cycles per second
    
    Logger::GetInstance().Log(LogLevel::Info, "EncryptionPerformance",
        "Encryption throughput: " + std::to_string(metrics.throughputOpsPerSec) + " ops/sec");
}

TEST_F(SecurityPerformanceTest, HashingPerformance) {
    const std::vector<std::string> testInputs = {
        "Small input",
        "Medium length input with more characters for testing",
        "Large input with significantly more text content to test hashing performance "
        "with varying input sizes and complexity patterns that might affect performance"
    };
    
    for (const auto& input : testInputs) {
        auto hashOperation = [&]() {
            std::string hash = Security::SHA256Hash(input);
            EXPECT_EQ(hash.length(), 64); // SHA256 produces 64 hex characters
        };
        
        PerformanceMetrics metrics = RunBenchmark(hashOperation, 5000);
        
        Logger::GetInstance().Log(LogLevel::Info, "HashingPerformance",
            "Input size " + std::to_string(input.length()) + " bytes: " + 
            std::to_string(metrics.throughputOpsPerSec) + " hashes/sec");
        
        EXPECT_GT(metrics.throughputOpsPerSec, 1000); // At least 1000 hashes/sec
    }
}

TEST_F(SecurityPerformanceTest, DPAPIPerformance) {
    const int iterations = 100; // DPAPI is slower, use fewer iterations
    
    auto dpapiOperation = [](){ 
        static int counter = 0;
        std::string key = "TestKey" + std::to_string(counter++);
        std::string data = "TestData" + std::to_string(counter);
        
        EXPECT_TRUE(Security::StoreCredential(key, data));
        std::string retrieved = Security::RetrieveCredential(key);
        EXPECT_EQ(retrieved, data);
        EXPECT_TRUE(Security::DeleteCredential(key));
    };
    
    PerformanceMetrics metrics = RunBenchmark(dpapiOperation, iterations);
    
    EXPECT_GT(metrics.throughputOpsPerSec, 10); // DPAPI is slower - at least 10 ops/sec
    
    Logger::GetInstance().Log(LogLevel::Info, "DPAPIPerformance",
        "DPAPI throughput: " + std::to_string(metrics.throughputOpsPerSec) + " ops/sec");
}

// Service Locator Performance Tests  
class ServiceLocatorPerformanceTest : public PerformanceTest {
protected:
    void SetUp() override {
        PerformanceTest::SetUp();
        ServiceLocator::DestroyInstance();
    }
    
    void TearDown() override {
        ServiceLocator::DestroyInstance();
        PerformanceTest::TearDown();
    }
};

// Mock service for testing
class ITestService {
public:
    virtual ~ITestService() = default;
    virtual int GetValue() const = 0;
    virtual void DoWork() = 0;
};

class TestServiceImpl : public ITestService {
private:
    int value_;
public:
    TestServiceImpl(int value = 42) : value_(value) {}
    int GetValue() const override { return value_; }
    void DoWork() override { 
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
};

TEST_F(ServiceLocatorPerformanceTest, ServiceResolutionThroughput) {
    ServiceLocator& locator = ServiceLocator::GetInstance();
    
    // Register a test service
    auto testService = std::make_shared<TestServiceImpl>(123);
    locator.RegisterService<ITestService>(testService);
    
    const int iterations = 10000;
    
    auto resolveOperation = [&locator]() {
        auto service = locator.ResolveService<ITestService>();
        EXPECT_NE(service, nullptr);
        EXPECT_EQ(service->GetValue(), 123);
    };
    
    PerformanceMetrics metrics = RunBenchmark(resolveOperation, iterations);
    
    EXPECT_GT(metrics.throughputOpsPerSec, 10000); // Very fast service resolution
    
    Logger::GetInstance().Log(LogLevel::Info, "ServiceLocatorPerformance",
        "Service resolution throughput: " + std::to_string(metrics.throughputOpsPerSec) + " ops/sec");
}

TEST_F(ServiceLocatorPerformanceTest, ConcurrentServiceResolution) {
    ServiceLocator& locator = ServiceLocator::GetInstance();
    
    // Register multiple services
    for (int i = 0; i < 10; ++i) {
        auto service = std::make_shared<TestServiceImpl>(i);
        locator.RegisterService<ITestService>("Service" + std::to_string(i), service);
    }
    
    const int numThreads = 10;
    const int operationsPerThread = 1000;
    
    std::vector<std::thread> threads;
    std::atomic<int> totalOperations(0);
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&locator, &totalOperations, t, operationsPerThread]() {
            for (int i = 0; i < operationsPerThread; ++i) {
                std::string serviceName = "Service" + std::to_string(i % 10);
                auto service = locator.ResolveService<ITestService>(serviceName);
                EXPECT_NE(service, nullptr);
                service->DoWork();
                totalOperations++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    EXPECT_EQ(totalOperations.load(), numThreads * operationsPerThread);
    
    double throughput = (totalOperations.load() * 1000.0) / duration;
    Logger::GetInstance().Log(LogLevel::Info, "ConcurrentServiceResolution",
        "Concurrent service resolution throughput: " + std::to_string(throughput) + " ops/sec");
}

// Splash Screen Performance Tests
class SplashScreenPerformanceTest : public PerformanceTest {
protected:
    void SetUp() override {
        PerformanceTest::SetUp();
        config_.enableSound = false; // Disable sound for performance testing
        config_.displayTimeMs = 1000; // Short display time
        config_.enable4K = false; // Use standard resolution
    }
    
    CinematicSplashScreen::Config config_;
};

TEST_F(SplashScreenPerformanceTest, SplashScreenCreationSpeed) {
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    const int iterations = 10;
    
    auto splashOperation = [hInstance, this]() {
        CinematicSplashScreen splash(hInstance, config_);
        auto startShow = std::chrono::high_resolution_clock::now();
        
        EXPECT_TRUE(splash.Show());
        
        auto endShow = std::chrono::high_resolution_clock::now();
        auto showTime = std::chrono::duration_cast<std::chrono::milliseconds>(endShow - startShow).count();
        
        EXPECT_LT(showTime, 1000); // Should show within 1 second
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Brief display
        splash.Hide();
    };
    
    PerformanceMetrics metrics = RunBenchmark(splashOperation, iterations);
    
    Logger::GetInstance().Log(LogLevel::Info, "SplashScreenPerformance",
        "Average splash screen creation time: " + std::to_string(metrics.executionTimeMs / iterations) + "ms");
}

// Memory Leak Tests
class MemoryLeakTest : public PerformanceTest {
protected:
    void SetUp() override {
        PerformanceTest::SetUp();
        initialMemory_ = GetCurrentMemoryUsage();
    }
    
    void TearDown() override {
        size_t finalMemory = GetCurrentMemoryUsage();
        size_t memoryDifference = (finalMemory > initialMemory_) ? (finalMemory - initialMemory_) : 0;
        
        Logger::GetInstance().Log(LogLevel::Info, "MemoryLeakTest",
            "Memory change: " + std::to_string(memoryDifference) + " bytes");
        
        // Allow for some memory growth but flag excessive leaks
        EXPECT_LT(memoryDifference, 5 * 1024 * 1024) << "Possible memory leak detected";
        
        PerformanceTest::TearDown();
    }
    
private:
    size_t GetCurrentMemoryUsage() {
        PROCESS_MEMORY_COUNTERS_EX memInfo;
        GetProcessMemoryInfo(GetCurrentProcess(),
                           reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memInfo),
                           sizeof(memInfo));
        return memInfo.WorkingSetSize;
    }
    
    size_t initialMemory_;
};

TEST_F(MemoryLeakTest, LoggerMemoryLeak) {
    const int iterations = 5000;
    
    for (int i = 0; i < iterations; ++i) {
        Logger& logger = Logger::GetInstance();
        logger.Log(LogLevel::Info, "MemoryLeakTest", 
                  "Memory leak test iteration " + std::to_string(i));
        
        // Force log processing
        if (i % 100 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
    // Force cleanup
    Logger::DestroyInstance();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(MemoryLeakTest, SecurityMemoryLeak) {
    ASSERT_TRUE(Security::Initialize());
    
    const int iterations = 1000;
    const std::string testData = "Memory leak test data for encryption";
    const std::string key = "MemoryLeakTestKey123456789012345";
    
    for (int i = 0; i < iterations; ++i) {
        std::string encrypted = Security::EncryptAES(testData, key);
        EXPECT_FALSE(encrypted.empty());
        
        std::string decrypted = Security::DecryptAES(encrypted, key);
        EXPECT_EQ(decrypted, testData);
        
        // Test DPAPI operations
        if (i % 10 == 0) {
            std::string credKey = "MemTestKey" + std::to_string(i);
            Security::StoreCredential(credKey, testData);
            std::string retrieved = Security::RetrieveCredential(credKey);
            Security::DeleteCredential(credKey);
        }
    }
    
    Security::Cleanup();
}

TEST_F(MemoryLeakTest, ServiceLocatorMemoryLeak) {
    const int iterations = 1000;
    
    for (int i = 0; i < iterations; ++i) {
        ServiceLocator& locator = ServiceLocator::GetInstance();
        
        // Register and unregister services
        auto service = std::make_shared<TestServiceImpl>(i);
        std::string serviceName = "MemTestService" + std::to_string(i);
        
        locator.RegisterService<ITestService>(serviceName, service);
        
        auto resolved = locator.ResolveService<ITestService>(serviceName);
        EXPECT_NE(resolved, nullptr);
        
        locator.UnregisterService<ITestService>(serviceName);
        
        // Periodically destroy and recreate to test cleanup
        if (i % 100 == 0) {
            ServiceLocator::DestroyInstance();
        }
    }
    
    ServiceLocator::DestroyInstance();
}

TEST_F(MemoryLeakTest, ConfigManagerMemoryLeak) {
    const int iterations = 500;
    const std::wstring tempConfigPath = L"memory_test_config.json";
    
    for (int i = 0; i < iterations; ++i) {
        ConfigManager& config = ConfigManager::GetInstance();
        config.Initialize(tempConfigPath);
        
        // Set and get values
        config.SetValue("test_key_" + std::to_string(i), "test_value_" + std::to_string(i));
        config.SetValue("test_int_" + std::to_string(i), i);
        
        std::string strValue = config.GetValue<std::string>("test_key_" + std::to_string(i), "");
        int intValue = config.GetValue<int>("test_int_" + std::to_string(i), 0);
        
        EXPECT_EQ(strValue, "test_value_" + std::to_string(i));
        EXPECT_EQ(intValue, i);
        
        if (i % 50 == 0) {
            config.SaveConfig();
            ConfigManager::DestroyInstance();
        }
    }
    
    ConfigManager::DestroyInstance();
    DeleteFile(tempConfigPath.c_str());
}

// Stress Tests
class StressTest : public PerformanceTest {
protected:
    void SetUp() override {
        PerformanceTest::SetUp();
    }
};

TEST_F(StressTest, SystemResourceStressTest) {
    const int duration_seconds = 10;
    const int numThreads = 20;
    
    std::atomic<bool> running(true);
    std::vector<std::thread> threads;
    
    Logger::GetInstance().Log(LogLevel::Info, "StressTest", 
        "Starting " + std::to_string(duration_seconds) + " second stress test with " + 
        std::to_string(numThreads) + " threads");
    
    // Start stress threads
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&running, t]() {
            Logger& logger = Logger::GetInstance();
            ServiceLocator& locator = ServiceLocator::GetInstance();
            
            // Register a service for this thread
            auto service = std::make_shared<TestServiceImpl>(t);
            std::string serviceName = "StressService" + std::to_string(t);
            locator.RegisterService<ITestService>(serviceName, service);
            
            int operations = 0;
            while (running.load()) {
                // Mix of operations
                logger.Log(LogLevel::Info, "StressTest", 
                          "Thread " + std::to_string(t) + " operation " + std::to_string(operations++));
                
                auto resolvedService = locator.ResolveService<ITestService>(serviceName);
                if (resolvedService) {
                    resolvedService->DoWork();
                }
                
                // Security operations
                if (operations % 10 == 0) {
                    std::string data = "StressData" + std::to_string(operations);
                    std::string hash = Security::SHA256Hash(data);
                    EXPECT_EQ(hash.length(), 64);
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            
            locator.UnregisterService<ITestService>(serviceName);
        });
    }
    
    // Let stress test run
    std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
    running.store(false);
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    Logger::GetInstance().Log(LogLevel::Info, "StressTest", "Stress test completed successfully");
}

// Performance Report Generation
class PerformanceReportTest : public PerformanceTest {
public:
    void GeneratePerformanceReport() {
        std::ofstream report("performance_report.txt");
        report << "RainmeterManager Performance Test Report\n";
        report << "Generated: " << std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() << "\n\n";
        
        // System Info
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        report << "System Information:\n";
        report << "  Processors: " << sysInfo.dwNumberOfProcessors << "\n";
        report << "  Page Size: " << sysInfo.dwPageSize << " bytes\n\n";
        
        // Memory Info
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        report << "Memory Information:\n";
        report << "  Total Physical: " << memInfo.ullTotalPhys / (1024*1024) << " MB\n";
        report << "  Available Physical: " << memInfo.ullAvailPhys / (1024*1024) << " MB\n";
        report << "  Memory Load: " << memInfo.dwMemoryLoad << "%\n\n";
        
        report << "Performance test completed. Check individual test logs for detailed metrics.\n";
        report.close();
        
        Logger::GetInstance().Log(LogLevel::Info, "PerformanceReport", 
            "Performance report generated: performance_report.txt");
    }
};

TEST_F(PerformanceReportTest, GenerateReport) {
    GeneratePerformanceReport();
    SUCCEED();
}

// Main function for performance tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize COM for Windows APIs
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    
    // Set high priority for more accurate performance measurements
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    
    int result = RUN_ALL_TESTS();
    
    CoUninitialize();
    return result;
}
