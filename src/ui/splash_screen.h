#ifndef SPLASH_SCREEN_H
#define SPLASH_SCREEN_H

#include <windows.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1effects.h>
#include <dwrite.h>
#include <wincodec.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <string>
#include <chrono>
#include <thread>
#include <functional>
#include <memory>
#include <vector>
#include <cmath>
#include "ui_framework.h"
#include "../core/logger.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

namespace RainmeterManager {
namespace UI {

/**
 * @brief Cinematic 4K Water-themed Splash Screen
 * 
 * Features:
 * - 4K resolution support with transparency
 * - Animated water drop with realistic physics
 * - Ripple effects with slow-motion animation
 * - Leaf floating on water surface
 * - Ambient water sound effects
 * - Smooth fade transitions
 * - Hardware-accelerated Direct2D rendering
 */
class CinematicSplashScreen {
public:
    /**
     * @brief Water drop physics parameters
     */
    struct WaterDropPhysics {
        float dropHeight = 400.0f;           // Height to drop from
        float gravity = 9.81f;               // Gravity acceleration
        float surfaceTension = 0.072f;       // Water surface tension
        float dropRadius = 8.0f;             // Initial drop radius
        float impactRadius = 50.0f;          // Ripple starting radius
        float rippleSpeed = 300.0f;          // Ripple expansion speed
        float rippleDecay = 0.95f;           // Ripple amplitude decay
        int maxRipples = 5;                  // Maximum concurrent ripples
        float animationSpeed = 0.3f;         // Slow motion factor (0.3 = 30% speed)
    };

    /**
     * @brief Ripple effect data
     */
    struct Ripple {
        float centerX, centerY;              // Ripple center
        float radius;                        // Current radius
        float amplitude;                     // Current amplitude
        float phase;                         // Animation phase
        std::chrono::steady_clock::time_point startTime;
        bool active;
        
        Ripple() : centerX(0), centerY(0), radius(0), amplitude(0), phase(0), active(false) {}
    };

    /**
     * @brief Leaf object floating on water
     */
    struct FloatingLeaf {
        float x, y;                          // Current position
        float targetX, targetY;              // Target position
        float rotation;                      // Current rotation
        float bobOffset;                     // Vertical bobbing offset
        float bobSpeed;                      // Bobbing animation speed
        std::chrono::steady_clock::time_point startTime;
        bool visible;
        
        FloatingLeaf() : x(0), y(0), targetX(0), targetY(0), rotation(0), 
                        bobOffset(0), bobSpeed(2.0f), visible(true) {}
    };

    /**
     * @brief Cinematic splash screen configuration
     */
    struct Config {
        int width = 1920;                    // 4K width (can scale to 3840 for true 4K)
        int height = 1080;                   // 4K height (can scale to 2160 for true 4K)
        bool enable4K = true;                // Enable 4K resolution if available
        float transparency = 0.15f;          // Background transparency (0.0 = fully transparent)
        int displayTimeMs = 8000;            // Extended display time for cinematic effect
        bool enableSound = true;             // Enable ambient water sounds
        std::wstring soundPath;              // Custom sound file path
        WaterDropPhysics physics;            // Water physics parameters
        bool enableParticles = true;         // Enable water particle effects
        bool enableReflections = true;       // Enable water reflections
        float ambientVolume = 0.3f;          // Ambient sound volume (0.0-1.0)
    };

    /**
     * @brief Progress update callback function type
     */
    using ProgressCallback = std::function<void(int percentage, const std::wstring& status)>;
    
    /**
     * @brief Completion callback function type
     */
    using CompletionCallback = std::function<void()>;

private:
    // Static window class name
    static constexpr const wchar_t* SPLASH_WINDOW_CLASS = L"RainmeterManagerSplashWindow";
    
    // Internal state
    HWND splashWindow_;
    HINSTANCE hInstance_;
    Config config_;
    
    // Progress tracking
    int currentProgress_;
    std::wstring currentStatus_;
    bool isVisible_;
    bool isDismissed_;
    
    // Animation state
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point lastUpdateTime_;
    float fadeOpacity_;
    bool isAnimating_;
    
    // Threading
    mutable std::mutex stateMutex_;
    std::thread animationThread_;
    bool shouldStopAnimation_;
    
    // Callbacks
    CompletionCallback completionCallback_;
    
    // Resources
    HBITMAP logoImage_;
    HFONT titleFont_;
    HFONT statusFont_;
    HBRUSH backgroundBrush_;
    
    // Drawing state
    int logoWidth_;
    int logoHeight_;
    int progressBarWidth_;
    int progressBarHeight_;

public:
    /**
     * @brief Constructor
     */
    explicit CinematicSplashScreen(HINSTANCE hInstance, const Config& config = Config{});
    
    /**
     * @brief Destructor
     */
    ~SplashScreen();
    
    // Prevent copy/move
    SplashScreen(const SplashScreen&) = delete;
    SplashScreen& operator=(const SplashScreen&) = delete;
    SplashScreen(SplashScreen&&) = delete;
    SplashScreen& operator=(SplashScreen&&) = delete;
    
    /**
     * @brief Show the splash screen
     * @return true if successful
     */
    bool Show();
    
    /**
     * @brief Hide the splash screen
     */
    void Hide();
    
    /**
     * @brief Update progress (thread-safe)
     * @param percentage Progress percentage (0-100)
     * @param status Status message to display
     */
    void UpdateProgress(int percentage, const std::wstring& status = L"");
    
    /**
     * @brief Check if splash screen is currently visible
     */
    bool IsVisible() const;
    
    /**
     * @brief Check if splash screen has been dismissed
     */
    bool IsDismissed() const;
    
    /**
     * @brief Set completion callback (called when splash is dismissed)
     */
    void SetCompletionCallback(CompletionCallback callback);
    
    /**
     * @brief Force dismiss the splash screen
     */
    void Dismiss();
    
    /**
     * @brief Get the splash window handle
     */
    HWND GetWindowHandle() const { return splashWindow_; }
    
    /**
     * @brief Process Windows messages for the splash screen
     * Call this from your main message loop
     */
    bool ProcessMessages();

private:
    /**
     * @brief Initialize resources and register window class
     */
    bool Initialize();
    
    /**
     * @brief Create the splash window
     */
    bool CreateSplashWindow();
    
    /**
     * @brief Load resources (logo, fonts, etc.)
     */
    bool LoadResources();
    
    /**
     * @brief Clean up resources
     */
    void CleanupResources();
    
    /**
     * @brief Window procedure for splash screen
     */
    static LRESULT CALLBACK SplashWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    /**
     * @brief Handle window messages
     */
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    /**
     * @brief Paint the splash screen
     */
    void OnPaint(HDC hdc);
    
    /**
     * @brief Draw the logo
     */
    void DrawLogo(HDC hdc, const RECT& clientRect);
    
    /**
     * @brief Draw the progress bar
     */
    void DrawProgressBar(HDC hdc, const RECT& clientRect);
    
    /**
     * @brief Draw the status text
     */
    void DrawStatusText(HDC hdc, const RECT& clientRect);
    
    /**
     * @brief Draw the application title
     */
    void DrawTitle(HDC hdc, const RECT& clientRect);
    
    /**
     * @brief Animation thread function
     */
    void AnimationThreadFunc();
    
    /**
     * @brief Update animation state
     */
    void UpdateAnimation();
    
    /**
     * @brief Calculate current fade opacity
     */
    float CalculateFadeOpacity() const;
    
    /**
     * @brief Center the window on screen
     */
    void CenterWindow();
    
    /**
     * @brief Check if minimum display time has elapsed
     */
    bool HasMinimumTimeElapsed() const;
    
    /**
     * @brief Check if maximum timeout has been reached
     */
    bool HasMaximumTimeoutElapsed() const;
    
    /**
     * @brief Log splash screen events
     */
    void LogEvent(const std::wstring& event, Core::LogLevel level = Core::LogLevel::Info) const;
};

/**
 * @brief Splash screen manager for easy integration
 * 
 * This class provides a simple interface for showing a splash screen
 * during application startup with automatic progress tracking.
 */
class SplashManager {
private:
    static SplashManager* instance_;
    std::unique_ptr<SplashScreen> splashScreen_;
    bool isActive_;

public:
    /**
     * @brief Get singleton instance
     */
    static SplashManager& GetInstance();
    
    /**
     * @brief Destroy singleton instance
     */
    static void DestroyInstance();
    
    /**
     * @brief Show splash screen with default configuration
     */
    bool ShowSplash(HINSTANCE hInstance);
    
    /**
     * @brief Show splash screen with custom configuration
     */
    bool ShowSplash(HINSTANCE hInstance, const SplashScreen::Config& config);
    
    /**
     * @brief Update splash progress
     */
    void UpdateProgress(int percentage, const std::wstring& status = L"");
    
    /**
     * @brief Hide splash screen
     */
    void HideSplash();
    
    /**
     * @brief Check if splash is currently active
     */
    bool IsActive() const { return isActive_; }
    
    /**
     * @brief Process splash screen messages
     */
    bool ProcessMessages();

private:
    SplashManager() = default;
    ~SplashManager() = default;
    
    // Prevent copy/move
    SplashManager(const SplashManager&) = delete;
    SplashManager& operator=(const SplashManager&) = delete;
};

} // namespace UI
} // namespace RainmeterManager

#endif // SPLASH_SCREEN_H
