#include <gtest/gtest.h>
#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <string>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>

#include "../../src/ui/splash_screen.h"
#include "../../src/ui/ui_framework.h"
#include "../../src/app/rainmgrapp.h"
#include "../../src/core/logger.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")

using namespace RainmeterManager::UI;
using namespace RainmeterManager::App;
using namespace RainmeterManager::Core;

// UI Automation Test Framework
class UIAutomationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize common controls
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_COOL_CLASSES;
        InitCommonControlsEx(&icex);
        
        // Initialize COM for automation
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        
        Logger::GetInstance().Log(LogLevel::Info, "UIAutomationTest", "Test setup completed");
        
        testStartTime_ = std::chrono::steady_clock::now();
    }
    
    void TearDown() override {
        // Cleanup any windows created during tests
        CleanupTestWindows();
        
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - testStartTime_).count();
        
        Logger::GetInstance().Log(LogLevel::Info, "UIAutomationTest", 
            "Test completed in " + std::to_string(duration) + "ms");
        
        CoUninitialize();
    }
    
    // Helper to find window by class name and title
    HWND FindTestWindow(const std::wstring& className, const std::wstring& title = L"") {
        return FindWindow(className.c_str(), title.empty() ? nullptr : title.c_str());
    }
    
    // Helper to wait for window to appear
    bool WaitForWindow(const std::wstring& className, int timeoutMs = 5000) {
        auto startTime = std::chrono::steady_clock::now();
        
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - startTime).count() < timeoutMs) {
            
            HWND window = FindWindow(className.c_str(), nullptr);
            if (window && IsWindow(window)) {
                testWindows_.push_back(window);
                return true;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        return false;
    }
    
    // Helper to get window bounds
    RECT GetWindowBounds(HWND window) {
        RECT rect = {};
        GetWindowRect(window, &rect);
        return rect;
    }
    
    // Helper to check if window is visible
    bool IsWindowActuallyVisible(HWND window) {
        if (!IsWindow(window) || !IsWindowVisible(window)) {
            return false;
        }
        
        // Check if window has non-zero size
        RECT rect = GetWindowBounds(window);
        return (rect.right - rect.left > 0) && (rect.bottom - rect.top > 0);
    }
    
    // Helper to simulate mouse click
    void ClickWindow(HWND window, int x = -1, int y = -1) {
        RECT rect = GetWindowBounds(window);
        
        int clickX = (x >= 0) ? x : (rect.left + (rect.right - rect.left) / 2);
        int clickY = (y >= 0) ? y : (rect.top + (rect.bottom - rect.top) / 2);
        
        SetCursorPos(clickX, clickY);
        
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;
        SendInput(1, &input, sizeof(INPUT));
    }
    
    // Helper to simulate key press
    void SendKeyToWindow(HWND window, WORD key) {
        SetForegroundWindow(window);
        
        INPUT input[2] = {};
        
        // Key down
        input[0].type = INPUT_KEYBOARD;
        input[0].ki.wVk = key;
        
        // Key up
        input[1].type = INPUT_KEYBOARD;
        input[1].ki.wVk = key;
        input[1].ki.dwFlags = KEYEVENTF_KEYUP;
        
        SendInput(2, input, sizeof(INPUT));
    }
    
    // Helper to capture screenshot
    bool CaptureScreenshot(HWND window, const std::wstring& filename) {
        RECT rect = GetWindowBounds(window);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        
        if (width <= 0 || height <= 0) return false;
        
        HDC windowDC = GetWindowDC(window);
        HDC memoryDC = CreateCompatibleDC(windowDC);
        HBITMAP bitmap = CreateCompatibleBitmap(windowDC, width, height);
        
        HGDIOBJ oldBitmap = SelectObject(memoryDC, bitmap);
        
        bool success = BitBlt(memoryDC, 0, 0, width, height, windowDC, 0, 0, SRCCOPY);
        
        // Save bitmap to file (simplified - in real implementation would use proper image encoding)
        if (success) {
            Logger::GetInstance().Log(LogLevel::Info, "UIAutomationTest", 
                "Screenshot captured: " + std::string(filename.begin(), filename.end()));
        }
        
        // Cleanup
        SelectObject(memoryDC, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(memoryDC);
        ReleaseDC(window, windowDC);
        
        return success;
    }
    
    // Cleanup test windows
    void CleanupTestWindows() {
        for (HWND window : testWindows_) {
            if (IsWindow(window)) {
                PostMessage(window, WM_CLOSE, 0, 0);
            }
        }
        testWindows_.clear();
    }
    
private:
    std::chrono::steady_clock::time_point testStartTime_;
    std::vector<HWND> testWindows_;
};

// Splash Screen UI Tests
class SplashScreenUITest : public UIAutomationTest {
protected:
    void SetUp() override {
        UIAutomationTest::SetUp();
        
        // Create splash screen configuration for testing
        splashConfig_.width = 800;
        splashConfig_.height = 600;
        splashConfig_.displayTimeMs = 2000;
        splashConfig_.enableSound = false; // Disable sound for testing
        splashConfig_.enable4K = false; // Use standard resolution for testing
    }
    
    CinematicSplashScreen::Config splashConfig_;
};

TEST_F(SplashScreenUITest, SplashScreenCreation) {
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    CinematicSplashScreen splashScreen(hInstance, splashConfig_);
    
    EXPECT_TRUE(splashScreen.Show());
    EXPECT_TRUE(splashScreen.IsVisible());
    
    // Wait for splash window to appear
    EXPECT_TRUE(WaitForWindow(L"RainmeterCinematicSplash"));
    
    HWND splashWindow = splashScreen.GetWindowHandle();
    EXPECT_NE(splashWindow, nullptr);
    EXPECT_TRUE(IsWindowActuallyVisible(splashWindow));
    
    splashScreen.Hide();
    EXPECT_FALSE(splashScreen.IsVisible());
}

TEST_F(SplashScreenUITest, SplashScreenDimensions) {
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    CinematicSplashScreen splashScreen(hInstance, splashConfig_);
    
    EXPECT_TRUE(splashScreen.Show());
    EXPECT_TRUE(WaitForWindow(L"RainmeterCinematicSplash"));
    
    HWND splashWindow = splashScreen.GetWindowHandle();
    RECT rect = GetWindowBounds(splashWindow);
    
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    EXPECT_EQ(width, splashConfig_.width);
    EXPECT_EQ(height, splashConfig_.height);
    
    splashScreen.Hide();
}

TEST_F(SplashScreenUITest, SplashScreenProgressUpdates) {
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    CinematicSplashScreen splashScreen(hInstance, splashConfig_);
    
    EXPECT_TRUE(splashScreen.Show());
    EXPECT_TRUE(WaitForWindow(L"RainmeterCinematicSplash"));
    
    // Test progress updates
    std::vector<std::pair<int, std::wstring>> progressUpdates = {
        {25, L"Loading components..."},
        {50, L"Initializing services..."},
        {75, L"Starting application..."},
        {100, L"Ready!"}
    };
    
    for (const auto& update : progressUpdates) {
        splashScreen.UpdateProgress(update.first, update.second);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Verify window is still visible during updates
        EXPECT_TRUE(IsWindowActuallyVisible(splashScreen.GetWindowHandle()));
    }
    
    splashScreen.Hide();
}

TEST_F(SplashScreenUITest, SplashScreenWaterDropAnimation) {
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    CinematicSplashScreen splashConfig = splashConfig_;
    splashConfig.displayTimeMs = 5000; // Longer display for animation testing
    
    CinematicSplashScreen splashScreen(hInstance, splashConfig);
    
    EXPECT_TRUE(splashScreen.Show());
    EXPECT_TRUE(WaitForWindow(L"RainmeterCinematicSplash"));
    
    // Trigger water drop animations
    splashScreen.TriggerWaterDrop();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    splashScreen.TriggerWaterDrop(400, 100); // Specific position
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Verify window remains responsive during animations
    EXPECT_TRUE(IsWindowActuallyVisible(splashScreen.GetWindowHandle()));
    
    splashScreen.Hide();
}

TEST_F(SplashScreenUITest, SplashScreenManualDismiss) {
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    splashConfig_.allowManualDismiss = true;
    
    CinematicSplashScreen splashScreen(hInstance, splashConfig_);
    
    EXPECT_TRUE(splashScreen.Show());
    EXPECT_TRUE(WaitForWindow(L"RainmeterCinematicSplash"));
    
    HWND splashWindow = splashScreen.GetWindowHandle();
    
    // Test manual dismiss with mouse click
    ClickWindow(splashWindow);
    
    // Give time for dismiss to process
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_TRUE(splashScreen.IsDismissed());
}

// UI Framework Component Tests
class UIFrameworkTest : public UIAutomationTest {
protected:
    void SetUp() override {
        UIAutomationTest::SetUp();
        uiManager_ = &UIManager::getInstance();
    }
    
    UIManager* uiManager_;
};

TEST_F(UIFrameworkTest, MainWindowCreation) {
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    
    EXPECT_TRUE(uiManager_->createMainWindow(hInstance, SW_SHOW));
    
    // Wait for main window
    EXPECT_TRUE(WaitForWindow(L"RainmeterManagerMainWindow"));
}

TEST_F(UIFrameworkTest, ModernButtonComponent) {
    // Create a test window for button testing
    HWND testWindow = CreateWindow(
        L"STATIC", L"Button Test Window",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, 400, 300,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr
    );
    
    ASSERT_NE(testWindow, nullptr);
    testWindows_.push_back(testWindow);
    
    // Create modern button
    bool buttonClicked = false;
    auto button = UIBuilder::createButton("Test Button");
    
    // Test button creation and properties
    EXPECT_TRUE(button->create(testWindow, 50, 50, 100, 30));
    EXPECT_NE(button->getHandle(), nullptr);
    
    button->show(true);
    EXPECT_TRUE(IsWindowVisible(button->getHandle()));
    
    // Test button interaction
    ClickWindow(button->getHandle());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(UIFrameworkTest, ModernListViewComponent) {
    HWND testWindow = CreateWindow(
        L"STATIC", L"ListView Test Window",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        150, 150, 500, 400,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr
    );
    
    ASSERT_NE(testWindow, nullptr);
    testWindows_.push_back(testWindow);
    
    auto listView = UIBuilder::createListView();
    EXPECT_TRUE(listView->create(testWindow, 10, 10, 300, 200));
    
    // Add test items
    ModernListView::ListItem item1;
    item1.text = "Item 1";
    item1.subText = "Description 1";
    listView->addItem(item1);
    
    ModernListView::ListItem item2;
    item2.text = "Item 2";
    item2.subText = "Description 2";
    listView->addItem(item2);
    
    listView->show(true);
    EXPECT_TRUE(IsWindowVisible(listView->getHandle()));
}

TEST_F(UIFrameworkTest, ThemeApplication) {
    UITheme darkTheme;
    darkTheme.backgroundColor = RGB(45, 45, 48);
    darkTheme.foregroundColor = RGB(255, 255, 255);
    darkTheme.accentColor = RGB(0, 120, 215);
    darkTheme.darkMode = true;
    
    uiManager_->setGlobalTheme(darkTheme);
    
    UITheme& appliedTheme = uiManager_->getGlobalTheme();
    EXPECT_EQ(appliedTheme.backgroundColor, darkTheme.backgroundColor);
    EXPECT_EQ(appliedTheme.foregroundColor, darkTheme.foregroundColor);
    EXPECT_EQ(appliedTheme.accentColor, darkTheme.accentColor);
    EXPECT_TRUE(appliedTheme.darkMode);
}

// RAINMGRApp UI Integration Tests
class RAINMGRAppUITest : public UIAutomationTest {
protected:
    void SetUp() override {
        UIAutomationTest::SetUp();
        RAINMGRApp::DestroyInstance();
    }
    
    void TearDown() override {
        RAINMGRApp::DestroyInstance();
        UIAutomationTest::TearDown();
    }
};

TEST_F(RAINMGRAppUITest, ApplicationWindowCreation) {
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    RAINMGRApp& app = RAINMGRApp::GetInstance(hInstance);
    
    EXPECT_TRUE(app.Initialize());
    
    // The app creates its main window during initialization
    HWND mainWindow = app.GetMainWindow();
    EXPECT_NE(mainWindow, nullptr);
    
    if (mainWindow) {
        EXPECT_TRUE(IsWindow(mainWindow));
        
        // Test window properties
        RECT rect = GetWindowBounds(mainWindow);
        EXPECT_GT(rect.right - rect.left, 0);
        EXPECT_GT(rect.bottom - rect.top, 0);
        
        // Test window title
        wchar_t title[256];
        GetWindowText(mainWindow, title, 256);
        std::wstring windowTitle(title);
        EXPECT_FALSE(windowTitle.empty());
    }
}

TEST_F(RAINMGRAppUITest, ApplicationMessageLoop) {
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    RAINMGRApp& app = RAINMGRApp::GetInstance(hInstance);
    
    EXPECT_TRUE(app.Initialize());
    
    // Test message processing
    bool messageProcessed = app.ProcessMessages();
    EXPECT_TRUE(messageProcessed); // Should return true if app is running
    
    // Send a test message to the main window
    HWND mainWindow = app.GetMainWindow();
    if (mainWindow) {
        PostMessage(mainWindow, WM_USER + 1, 0, 0);
        
        // Process the message
        app.ProcessMessages();
        
        // Application should still be running
        EXPECT_FALSE(app.IsShutdownRequested());
    }
}

// Performance UI Tests
class UIPerformanceTest : public UIAutomationTest {
protected:
    void SetUp() override {
        UIAutomationTest::SetUp();
        performanceStartTime_ = std::chrono::high_resolution_clock::now();
    }
    
    void TearDown() override {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - performanceStartTime_).count();
        
        Logger::GetInstance().Log(LogLevel::Info, "UIPerformanceTest", 
            "Performance test completed in " + std::to_string(duration) + "ms");
        
        UIAutomationTest::TearDown();
    }
    
private:
    std::chrono::high_resolution_clock::time_point performanceStartTime_;
};

TEST_F(UIPerformanceTest, SplashScreenRenderingPerformance) {
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    
    CinematicSplashScreen::Config config;
    config.width = 1920;
    config.height = 1080;
    config.enableSound = false;
    config.displayTimeMs = 3000;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    CinematicSplashScreen splashScreen(hInstance, config);
    EXPECT_TRUE(splashScreen.Show());
    
    auto showTime = std::chrono::high_resolution_clock::now();
    auto showDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        showTime - startTime).count();
    
    // Splash screen should show quickly (within 1 second)
    EXPECT_LT(showDuration, 1000);
    
    // Test animation performance
    for (int i = 0; i < 10; ++i) {
        splashScreen.TriggerWaterDrop();
        splashScreen.UpdateProgress(i * 10, L"Performance test " + std::to_wstring(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    auto animationTime = std::chrono::high_resolution_clock::now();
    auto animationDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        animationTime - showTime).count();
    
    Logger::GetInstance().Log(LogLevel::Info, "UIPerformanceTest", 
        "Animation sequence took " + std::to_string(animationDuration) + "ms");
    
    splashScreen.Hide();
    
    auto hideTime = std::chrono::high_resolution_clock::now();
    auto hideDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        hideTime - animationTime).count();
    
    // Hide should be fast
    EXPECT_LT(hideDuration, 500);
}

TEST_F(UIPerformanceTest, UIComponentCreationPerformance) {
    const int componentCount = 100;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    HWND testWindow = CreateWindow(
        L"STATIC", L"Performance Test Window",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, 800, 600,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr
    );
    
    ASSERT_NE(testWindow, nullptr);
    testWindows_.push_back(testWindow);
    
    std::vector<std::shared_ptr<ModernButton>> buttons;
    
    for (int i = 0; i < componentCount; ++i) {
        auto button = UIBuilder::createButton("Button " + std::to_string(i));
        EXPECT_TRUE(button->create(testWindow, (i % 10) * 70, (i / 10) * 30, 60, 25));
        button->show(true);
        buttons.push_back(button);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();
    
    Logger::GetInstance().Log(LogLevel::Info, "UIPerformanceTest", 
        "Created " + std::to_string(componentCount) + " UI components in " + 
        std::to_string(duration) + "ms");
    
    // Should create components reasonably quickly
    EXPECT_LT(duration, 5000); // 5 seconds max for 100 components
}

// Accessibility Tests
class UIAccessibilityTest : public UIAutomationTest {
public:
    void TestWindowAccessibility(HWND window) {
        // Test if window supports accessibility features
        EXPECT_TRUE(IsWindow(window));
        
        // Test keyboard navigation
        SetFocus(window);
        EXPECT_EQ(GetFocus(), window);
        
        // Test screen reader compatibility (basic check)
        wchar_t windowText[256];
        int textLength = GetWindowText(window, windowText, 256);
        EXPECT_GT(textLength, 0); // Window should have accessible text
    }
};

TEST_F(UIAccessibilityTest, SplashScreenAccessibility) {
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    
    CinematicSplashScreen::Config config;
    config.allowManualDismiss = true;
    
    CinematicSplashScreen splashScreen(hInstance, config);
    EXPECT_TRUE(splashScreen.Show());
    EXPECT_TRUE(WaitForWindow(L"RainmeterCinematicSplash"));
    
    HWND splashWindow = splashScreen.GetWindowHandle();
    TestWindowAccessibility(splashWindow);
    
    // Test keyboard dismiss
    SendKeyToWindow(splashWindow, VK_ESCAPE);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    splashScreen.Hide();
}

// Main function for UI automation tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize COM and common controls
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    
    int result = RUN_ALL_TESTS();
    
    CoUninitialize();
    return result;
}
