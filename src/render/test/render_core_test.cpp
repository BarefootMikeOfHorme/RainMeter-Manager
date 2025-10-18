#include "../managers/render_coordinator.h"
#include "../../core/logger.h"
#include "../../core/service_locator.h"
#include <windows.h>
#include <thread>
#include <chrono>

using namespace RainmeterManager::Render;
// Note: Don't use "using namespace Core" to avoid Logger/LogLevel ambiguity with global ::Logger

namespace RainmeterManager::Test {

/**
 * @brief Simple test to verify RenderCore works with existing illustro clock
 * This integrates directly into the existing RAINMGRApp without external dependencies
 */
class RenderCoreIllustroTest {
public:
    RenderCoreIllustroTest() = default;
    ~RenderCoreIllustroTest() = default;

    /**
     * @brief Run the illustro clock test using our new RenderCore
     */
    bool RunTest() {
        LOG_INFO("🎯 Starting RenderCore Illustro Clock Test");
        
        try {
            // Step 1: Initialize RenderCoordinator
            if (!InitializeRenderCoordinator()) {
                LOG_ERROR("❌ Failed to initialize RenderCoordinator");
                return false;
            }

            // Step 2: Create a widget based on illustro clock
            if (!CreateIllustroClockWidget()) {
                LOG_ERROR("❌ Failed to create illustro clock widget");
                return false;
            }

            // Step 3: Run the test for 30 seconds
            if (!RunClockUpdateLoop()) {
                LOG_ERROR("❌ Clock update loop failed");
                return false;
            }

            // Step 4: Cleanup
            Cleanup();

            LOG_INFO("✅ RenderCore Illustro Clock Test completed successfully!");
            return true;

        } catch (const std::exception& e) {
            LOG_ERROR("❌ Exception in RenderCore test: " + std::string(e.what()));
            return false;
        }
    }

private:
    std::unique_ptr<RenderCoordinator> renderCoordinator_;
    uint32_t clockWidgetId_ = 0;

    bool InitializeRenderCoordinator() {
        LOG_INFO("📡 Initializing RenderCoordinator...");
        
        renderCoordinator_ = CreateRenderCoordinator();
        
        if (!renderCoordinator_->Initialize()) {
            LOG_ERROR("Failed to initialize RenderCoordinator");
            return false;
        }

        if (!renderCoordinator_->Start()) {
            LOG_ERROR("Failed to start RenderCoordinator");
            return false;
        }

        LOG_INFO("✅ RenderCoordinator initialized and started");
        return true;
    }

    bool CreateIllustroClockWidget() {
        LOG_INFO("🕐 Creating illustro clock widget...");

        // Create a test window handle (normally this would be the actual widget window)
        HWND testWindow = CreateTestWindow();
        if (!testWindow) {
            LOG_ERROR("Failed to create test window");
            return false;
        }

        // Define widget bounds matching illustro clock (300x150)
        RenderRect bounds(100, 100, 300, 150);

        // Create the widget using RenderCoordinator
        clockWidgetId_ = renderCoordinator_->CreateWidget(
            testWindow, 
            bounds, 
            RenderBackendType::Auto  // Let it choose the best backend
        );

        if (clockWidgetId_ == 0) {
            LOG_ERROR("Failed to create widget through RenderCoordinator");
            return false;
        }

        LOG_INFO("✅ Illustro clock widget created with ID: " + std::to_string(clockWidgetId_));
        return true;
    }

    bool RunClockUpdateLoop() {
        LOG_INFO("⏰ Starting clock update loop (30 seconds)...");

        const auto startTime = std::chrono::steady_clock::now();
        const auto testDuration = std::chrono::seconds(30);
        int frameCount = 0;

        while (std::chrono::steady_clock::now() - startTime < testDuration) {
            // Create content matching illustro clock format
            auto clockContent = CreateIllustroClockContent();
            
            // Update the widget content
            auto updateFuture = renderCoordinator_->UpdateWidgetContentAsync(clockWidgetId_, clockContent);
            
            // Wait for update to complete (with timeout)
            if (updateFuture.wait_for(std::chrono::milliseconds(1000)) == std::future_status::ready) {
                auto result = updateFuture.get();
                
                if (result.status == RenderResultStatus::Success) {
                    frameCount++;
                    
                    // Log every 5th frame to show progress
                    if (frameCount % 5 == 0) {
                        LOG_INFO("🕐 Clock updated successfully - Frame " + std::to_string(frameCount));
                        
                        // Get performance metrics
                        auto metrics = renderCoordinator_->GetWidgetPerformanceMetrics(clockWidgetId_);
                        LOG_INFO("📊 Performance: FPS=" + std::to_string(metrics.currentFps) + 
                                ", Memory=" + std::to_string(metrics.memoryUsageMB) + "MB");
                    }
                } else {
                    LOG_WARNING("⚠️ Clock update failed: " + result.errorMessage);
                }
            } else {
                LOG_WARNING("⚠️ Clock update timed out");
            }

            // Wait 1 second between updates (like a real clock)
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        LOG_INFO("✅ Clock update loop completed - " + std::to_string(frameCount) + " frames rendered");
        return frameCount > 0;  // Success if we rendered at least one frame
    }

    ContentParameters CreateIllustroClockContent() {
        // Get current time in illustro format
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        struct tm timeinfo;
        localtime_s(&timeinfo, &time_t);
        
        char timeStr[16];
        char dateStr[16]; 
        char dayStr[16];
        
        strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
        strftime(dateStr, sizeof(dateStr), "%d.%m.%Y", &timeinfo);
        strftime(dayStr, sizeof(dayStr), "%A", &timeinfo);

        // Create HTML content matching illustro style
        std::string htmlContent = R"(
<!DOCTYPE html>
<html>
<head>
    <style>
        body {
            font-family: 'Trebuchet MS', sans-serif;
            background: rgba(0, 0, 0, 0.8);
            color: rgba(255, 255, 255, 0.8);
            margin: 0;
            padding: 20px;
            text-align: center;
            height: 110px;
            display: flex;
            flex-direction: column;
            justify-content: center;
        }
        .time {
            font-size: 28px;
            font-weight: bold;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.5);
            margin-bottom: 8px;
        }
        .separator {
            height: 1px;
            background: rgba(255,255,255,0.15);
            margin: 8px auto;
            width: 180px;
        }
        .day {
            font-size: 12px;
            text-transform: uppercase;
            font-weight: bold;
        }
        .date {
            font-size: 11px;
            opacity: 0.8;
        }
        .watermark {
            position: absolute;
            bottom: 5px;
            right: 8px;
            font-size: 8px;
            opacity: 0.3;
        }
    </style>
</head>
<body>
    <div class="time">)" + std::string(timeStr) + R"(</div>
    <div class="separator"></div>
    <div class="day">)" + std::string(dayStr) + R"(</div>
    <div class="date">)" + std::string(dateStr) + R"(</div>
    <div class="watermark">RenderCore</div>
</body>
</html>)";

        ContentParameters content;
        content.sourceType = ContentSourceType::Static;
        content.sourceUrl = "data:text/html;charset=utf-8," + htmlContent;
        content.templatePath = "";
        content.refreshIntervalMs = 1000;  // Update every second
        content.cacheEnabled = false;      // Don't cache time-sensitive content

        return content;
    }

    HWND CreateTestWindow() {
        // Create a simple hidden window for testing
        HWND testWindow = CreateWindowExW(
            0,
            L"STATIC",
            L"RenderCore Test Window", 
            WS_POPUP,
            0, 0, 300, 150,
            nullptr,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr
        );

        return testWindow;
    }

    void Cleanup() {
        LOG_INFO("🧹 Cleaning up RenderCore test...");

        if (clockWidgetId_ != 0 && renderCoordinator_) {
            renderCoordinator_->DestroyWidget(clockWidgetId_);
        }

        if (renderCoordinator_) {
            renderCoordinator_->Stop();
        }

        LOG_INFO("✅ RenderCore test cleanup completed");
    }
};

} // namespace RainmeterManager::Test

// Export function for integration with main app
extern "C" {
    /**
     * @brief Run the RenderCore illustro clock test
     * Can be called from the main application to test RenderCore functionality
     */
    __declspec(dllexport) bool RunRenderCoreIllustroTest() {
        RainmeterManager::Test::RenderCoreIllustroTest test;
        return test.RunTest();
    }
}
