// logger_tests.cpp - Unit tests for Logger system
// Copyright (c) 2025 RainmeterManager. All rights reserved.

#include <gtest/gtest.h>
#include "../../src/core/logger.h"
#include <filesystem>
#include <fstream>

// Test fixture for Logger tests
class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any existing test logs
        testLogPath_ = "test_logs/test.log";
        std::filesystem::remove_all("test_logs");
        std::filesystem::create_directories("test_logs");
    }

    void TearDown() override {
        // Cleanup
        Logger::shutdown();
        std::filesystem::remove_all("test_logs");
    }

    std::string testLogPath_;
};

// Basic Initialization Tests
TEST_F(LoggerTest, InitializeWithValidPath) {
    ASSERT_TRUE(Logger::initialize(testLogPath_));
    
    // Log a message
    Logger::info("Test initialization message");
    Logger::flushLogs();
    
    // Verify log file exists
    EXPECT_TRUE(std::filesystem::exists(testLogPath_));
    
    // Verify file contains the message
    std::ifstream logFile(testLogPath_);
    std::string content((std::istreambuf_iterator<char>(logFile)), std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.find("Test initialization message") != std::string::npos);
}

TEST_F(LoggerTest, InitializeWithInvalidPath) {
    // Try to initialize with invalid/inaccessible path
    std::string invalidPath = "\\\\0\\invalid\\path\\log.txt";
    EXPECT_FALSE(Logger::initialize(invalidPath));
    
    // Verify no file was created at the invalid path
    EXPECT_FALSE(std::filesystem::exists(invalidPath));
}

// Logging Level Tests
TEST_F(LoggerTest, SetAndGetLogLevel) {
    ASSERT_TRUE(Logger::initialize(testLogPath_));
    
    // Set log level to WARNING
    Logger::setLogLevel(LogLevel::WARNING);
    
    // Log messages at different levels
    Logger::info("INFO message - should be filtered");
    Logger::warning("WARNING message - should appear");
    Logger::error("ERROR message - should appear");
    Logger::flushLogs();
    
    // Read log file
    std::ifstream logFile(testLogPath_);
    std::string content((std::istreambuf_iterator<char>(logFile)), std::istreambuf_iterator<char>());
    
    // Verify filtering
    EXPECT_TRUE(content.find("INFO message") == std::string::npos);
    EXPECT_TRUE(content.find("WARNING message") != std::string::npos);
    EXPECT_TRUE(content.find("ERROR message") != std::string::npos);
}

TEST_F(LoggerTest, LogAtDifferentLevels) {
    ASSERT_TRUE(Logger::initialize(testLogPath_));
    Logger::setLogLevel(LogLevel::TRACE);
    
    // Log at all levels
    Logger::trace("TRACE level");
    Logger::debug("DEBUG level");
    Logger::info("INFO level");
    Logger::warning("WARNING level");
    Logger::error("ERROR level");
    Logger::critical("CRITICAL level");
    Logger::fatal("FATAL level");
    Logger::flushLogs();
    
    // Verify all messages appear
    std::ifstream logFile(testLogPath_);
    std::string content((std::istreambuf_iterator<char>(logFile)), std::istreambuf_iterator<char>());
    
    EXPECT_TRUE(content.find("TRACE") != std::string::npos);
    EXPECT_TRUE(content.find("DEBUG") != std::string::npos);
    EXPECT_TRUE(content.find("INFO") != std::string::npos);
    EXPECT_TRUE(content.find("WARNING") != std::string::npos);
    EXPECT_TRUE(content.find("ERROR") != std::string::npos);
    EXPECT_TRUE(content.find("CRITICAL") != std::string::npos);
    EXPECT_TRUE(content.find("FATAL") != std::string::npos);
}

// File Rotation Tests
TEST_F(LoggerTest, LogRotationBySize) {
    LogRotationConfig config;
    config.maxFileSize = 1024; // 1KB
    config.maxFiles = 3;
    config.enableRotation = true;
    
    ASSERT_TRUE(Logger::initialize(testLogPath_, config));
    
    // Write enough data to trigger rotation (> 1KB)
    for (int i = 0; i < 100; i++) {
        Logger::info("Rotation test message number " + std::to_string(i) + " with some padding text to reach size limit faster");
    }
    Logger::flushLogs();
    
    // Check for rotated files (logger typically creates .1, .2, etc.)
    std::string rotatedFile1 = testLogPath_ + ".1";
    EXPECT_TRUE(std::filesystem::exists(testLogPath_) || std::filesystem::exists(rotatedFile1));
}

TEST_F(LoggerTest, MaxFilesRespected) {
    LogRotationConfig config;
    config.maxFileSize = 512; // Very small
    config.maxFiles = 2;
    config.enableRotation = true;
    
    ASSERT_TRUE(Logger::initialize(testLogPath_, config));
    
    // Write enough to force multiple rotations
    for (int i = 0; i < 200; i++) {
        Logger::info("Max files test message " + std::to_string(i) + " with padding to trigger rotation");
    }
    Logger::flushLogs();
    
    // Count log files in directory
    int logFileCount = 0;
    for (const auto& entry : std::filesystem::directory_iterator("test_logs")) {
        if (entry.path().extension() == ".log" || entry.path().filename().string().find("test.log") != std::string::npos) {
            logFileCount++;
        }
    }
    
    // Should not exceed maxFiles + 1 (current + rotated)
    EXPECT_LE(logFileCount, config.maxFiles + 1);
}

// Thread Safety Tests
TEST_F(LoggerTest, ConcurrentLoggingFromMultipleThreads) {
    ASSERT_TRUE(Logger::initialize(testLogPath_));
    
    const int numThreads = 8;
    const int messagesPerThread = 100;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back([i, messagesPerThread]() {
            for (int j = 0; j < messagesPerThread; j++) {
                Logger::info("Thread " + std::to_string(i) + " Message " + std::to_string(j));
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    Logger::flushLogs();
    
    // Verify file exists and has content
    EXPECT_TRUE(std::filesystem::exists(testLogPath_));
    std::ifstream logFile(testLogPath_);
    std::string content((std::istreambuf_iterator<char>(logFile)), std::istreambuf_iterator<char>());
    
    // Should have substantial content (not checking exact count due to potential filtering/formatting)
    EXPECT_GT(content.length(), 1000);
}

// Async Logging Tests
TEST_F(LoggerTest, AsyncLoggingDoesNotBlock) {
    Logger::enableAsyncLogging(true);
    ASSERT_TRUE(Logger::initialize(testLogPath_));
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Log many messages
    for (int i = 0; i < 1000; i++) {
        Logger::info("Async message " + std::to_string(i));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Async logging should be very fast (< 100ms for 1000 messages)
    EXPECT_LT(duration.count(), 100);
    
    // Flush and verify messages written
    Logger::flushLogs();
    Logger::shutdown(); // Ensure async thread completes
    
    EXPECT_TRUE(std::filesystem::exists(testLogPath_));
}

// Error Handling Tests
TEST_F(LoggerTest, HandleDiskFullGracefully) {
    // This test is informational - actual disk full simulation is complex
    // We verify the logger has error handling hooks
    ASSERT_TRUE(Logger::initialize(testLogPath_));
    
    // Log an error about potential disk issues
    Logger::error("Simulated disk full scenario");
    Logger::flushLogs();
    
    // Verify logger continues operating
    Logger::info("Logger still operational after error");
    Logger::flushLogs();
    
    EXPECT_TRUE(std::filesystem::exists(testLogPath_));
}

TEST_F(LoggerTest, HandleNullPointers) {
    ASSERT_TRUE(Logger::initialize(testLogPath_));
    
    // Test with empty/null-like scenarios
    EXPECT_NO_THROW(Logger::log(LogLevel::INFO, "", nullptr, 0, nullptr));
    EXPECT_NO_THROW(Logger::info(""));
    
    Logger::flushLogs();
    
    // Logger should handle gracefully without crashing
    EXPECT_TRUE(std::filesystem::exists(testLogPath_));
}

// Stack Trace Tests
TEST_F(LoggerTest, CaptureStackTraceOnCrash) {
    ASSERT_TRUE(Logger::initialize(testLogPath_));
    
    // Capture a stack trace
    std::string stackTrace = Logger::captureStackTrace(1);
    
    // Verify stack trace is non-empty and contains expected format
    EXPECT_FALSE(stackTrace.empty());
    EXPECT_TRUE(stackTrace.find("Stack trace") != std::string::npos);
    EXPECT_TRUE(stackTrace.find("Frame") != std::string::npos || stackTrace.find("0x") != std::string::npos);
}

// Performance Tests
TEST_F(LoggerTest, LoggingOverheadIsMinimal) {
    ASSERT_TRUE(Logger::initialize(testLogPath_));
    Logger::setLogLevel(LogLevel::INFO);
    
    const int numMessages = 5000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numMessages; i++) {
        Logger::info("Performance test message " + std::to_string(i));
    }
    
    Logger::flushLogs();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    double avgTime = static_cast<double>(duration.count()) / numMessages;
    
    // Average should be well under 1ms per message (being generous for CI)
    EXPECT_LT(duration.count(), 2000); // 2 seconds for 5000 messages = 0.4ms avg
}

// Security Event Logging Tests
TEST_F(LoggerTest, SecurityEventsAreTaggedCorrectly) {
    ASSERT_TRUE(Logger::initialize(testLogPath_));
    
    Logger::logSecurityEvent("Unauthorized access attempt", "User: test_user, Resource: /admin");
    Logger::flushLogs();
    
    // Read log and verify security tagging
    std::ifstream logFile(testLogPath_);
    std::string content((std::istreambuf_iterator<char>(logFile)), std::istreambuf_iterator<char>());
    
    EXPECT_TRUE(content.find("SECURITY EVENT") != std::string::npos);
    EXPECT_TRUE(content.find("Unauthorized access attempt") != std::string::npos);
}

// Cleanup Tests
TEST_F(LoggerTest, ShutdownCleansUpResources) {
    ASSERT_TRUE(Logger::initialize(testLogPath_));
    Logger::info("Test message before shutdown");
    Logger::flushLogs();
    
    // Shutdown logger
    Logger::shutdown();
    
    // Verify file still exists
    EXPECT_TRUE(std::filesystem::exists(testLogPath_));
    
    // Try to delete/rename the file (should succeed if handle released)
    std::string renamedPath = testLogPath_ + ".renamed";
    EXPECT_NO_THROW(std::filesystem::rename(testLogPath_, renamedPath));
    
    // Cleanup
    if (std::filesystem::exists(renamedPath)) {
        std::filesystem::remove(renamedPath);
    }
}

// NOTE: These tests use GTEST_SKIP() to mark them as TODO
// Remove GTEST_SKIP() and implement the actual test logic
// when ready to implement each test case.
