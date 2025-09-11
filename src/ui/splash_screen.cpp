#include "splash_screen.h"
#include "../core/logger_adapter.h"
#include <dwmapi.h>
#include <shellapi.h>
#include <random>
#include <algorithm>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

using namespace RainmeterManager::UI;

// Static instance for SplashManager
SplashManager* SplashManager::instance_ = nullptr;

namespace {
    // Water sound generation constants
    constexpr float PI = 3.14159265359f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr int SAMPLE_RATE = 44100;
    constexpr int CHANNELS = 2;
    constexpr int BITS_PER_SAMPLE = 16;
    
    // Random number generator for water effects
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    
    // Helper function to create smooth gradients
    D2D1_COLOR_F LerpColor(const D2D1_COLOR_F& a, const D2D1_COLOR_F& b, float t) {
        return D2D1::ColorF(
            a.r + (b.r - a.r) * t,
            a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t,
            a.a + (b.a - a.a) * t
        );
    }
    
    // Smooth step function for animations
    float SmoothStep(float edge0, float edge1, float x) {
        float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }
}

CinematicSplashScreen::CinematicSplashScreen(HINSTANCE hInstance, const Config& config)
    : hInstance_(hInstance)
    , config_(config)
    , splashWindow_(nullptr)
    , d2dFactory_(nullptr)
    , renderTarget_(nullptr)
    , writeFactory_(nullptr)
    , currentProgress_(0)
    , isVisible_(false)
    , isDismissed_(false)
    , isAnimating_(false)
    , shouldStopAnimation_(false)
    , dropY_(0)
    , dropVelocity_(0)
    , dropVisible_(false)
    , dropFalling_(false)
    , nextRippleIndex_(0)
    , audioEnumerator_(nullptr)
    , audioDevice_(nullptr)
    , audioClient_(nullptr)
    , audioRenderClient_(nullptr)
    , audioActive_(false)
    , currentVolume_(config.ambientVolume)
    , frameRate_(60.0f)
    , frameCount_(0) {
    
    // Initialize COM for audio
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    
    // Initialize DirectX components
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory_);
    if (FAILED(hr)) {
        LogEvent(L"Failed to create D2D1 factory", ::LogLevel::ERROR);
        return;
    }
    
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(writeFactory_), 
                            reinterpret_cast<IUnknown**>(&writeFactory_));
    if (FAILED(hr)) {
        LogEvent(L"Failed to create DirectWrite factory", ::LogLevel::ERROR);
        return;
    }
    
    // Initialize ripples array
    ripples_.resize(config_.physics.maxRipples);
    
    // Initialize leaves
    InitializeLeaves();
    
    LogEvent(L"CinematicSplashScreen initialized successfully");
}

CinematicSplashScreen::~CinematicSplashScreen() {
    if (isVisible_) {
        Hide();
    }
    
    CleanupResources();
    CoUninitialize();
}

bool CinematicSplashScreen::Show() {
    std::lock_guard<std::mutex> lock(stateMutex_);
    
    if (isVisible_) {
        return true;
    }
    
    LogEvent(L"Showing cinematic splash screen...");
    
    if (!Initialize()) {
        LogEvent(L"Failed to initialize splash screen", ::LogLevel::ERROR);
        return false;
    }
    
    if (!CreateSplashWindow()) {
        LogEvent(L"Failed to create splash window", ::LogLevel::ERROR);
        return false;
    }
    
    if (!LoadResources()) {
LogEvent(L"Failed to load resources", ::LogLevel::ERROR);
        return false;
    }
    
    // Initialize audio if enabled
    if (config_.enableSound) {
        InitializeAudio();
    }
    
    // Show window
    ShowWindow(splashWindow_, SW_SHOW);
    UpdateWindow(splashWindow_);
    
    // Start animation thread
    isAnimating_ = true;
    shouldStopAnimation_ = false;
    animationThread_ = std::thread(&CinematicSplashScreen::AnimationThreadFunc, this);
    
    // Start audio thread if audio is enabled
    if (config_.enableSound && audioClient_) {
        audioActive_ = true;
        audioThread_ = std::thread(&CinematicSplashScreen::AudioThreadFunc, this);
    }
    
    // Trigger initial water drop
    auto now = std::chrono::steady_clock::now();
    startTime_ = now;
    lastFrameTime_ = now;
    lastFPSUpdate_ = now;

    // Initialize and set first hint
    InitializeMessages();
    if (config_.enableAutoMessages) {
        std::lock_guard<std::mutex> lock(stateMutex_);
        auxStatus_ = L"Hint: If this takes a while, the splash stays up to 12s.";
        firstHintShown_ = false;
        lastMessageSwap_ = now;
    }
    
    // Start lifecycle watchdog (handles min/max timing and auto-dismiss)
    StartLifecycleWatchdog();

    // Start with a water drop after 1 second
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    TriggerWaterDrop();
    
    isVisible_ = true;
    LogEvent(L"Cinematic splash screen displayed successfully");
    
    return true;
}

void CinematicSplashScreen::Hide() {
    std::lock_guard<std::mutex> lock(stateMutex_);
    
    if (!isVisible_) {
        return;
    }
    
    LogEvent(L"Hiding cinematic splash screen...");
    
    // Stop animation thread
    shouldStopAnimation_ = true;
    isAnimating_ = false;
    
    if (animationThread_.joinable()) {
        animationThread_.join();
    }
    
    // Stop audio thread
    audioActive_ = false;
    if (audioThread_.joinable()) {
        audioThread_.join();
    }
    
    // Hide window
    if (splashWindow_) {
        ShowWindow(splashWindow_, SW_HIDE);
        DestroyWindow(splashWindow_);
        splashWindow_ = nullptr;
    }
    
    isVisible_ = false;
    isDismissed_ = true;
    
    // Call completion callback
    if (completionCallback_) {
        completionCallback_();
    }
    
    LogEvent(L"Cinematic splash screen hidden successfully");
}

void CinematicSplashScreen::UpdateProgress(int percentage, const std::wstring& status) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    
    currentProgress_ = std::clamp(percentage, 0, 100);
    if (!status.empty()) {
        currentStatus_ = status;
    }
    
    // Trigger water drop at certain progress milestones
    if (percentage == 25 || percentage == 50 || percentage == 75 || percentage == 100) {
        TriggerWaterDrop();
    }
    
    // Force window update
    if (splashWindow_) {
        InvalidateRect(splashWindow_, nullptr, FALSE);
    }
    
    LogEvent(L"Progress updated: " + std::to_wstring(percentage) + L"% - " + status);
}

bool CinematicSplashScreen::Initialize() {
    // Calculate optimal window size
    CalculateOptimalSize();
    
    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW;
    wc.lpfnWndProc = SplashWindowProc;
    wc.hInstance = hInstance_;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr; // We'll handle painting ourselves
    wc.lpszClassName = SPLASH_WINDOW_CLASS;
    
    if (!RegisterClassEx(&wc)) {
        DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS) {
LogEvent(L"Failed to register splash window class, error: " + std::to_wstring(error), ::LogLevel::ERROR);
            return false;
        }
    }
    
    return true;
}

bool CinematicSplashScreen::CreateSplashWindow() {
    // Create layered window for transparency
    splashWindow_ = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        SPLASH_WINDOW_CLASS,
        L"RainmeterManager - Cinematic Splash",
        WS_POPUP,
        0, 0, config_.width, config_.height,
        nullptr, nullptr, hInstance_, this
    );
    
    if (!splashWindow_) {
        DWORD error = GetLastError();
        LogEvent(L"Failed to create splash window, error: " + std::to_wstring(error), ::LogLevel::ERROR);
        return false;
    }
    
    // Set layered window attributes for transparency
    SetLayeredWindowAttributes(splashWindow_, RGB(0, 0, 0), 
                              static_cast<BYTE>(255 * (1.0f - config_.transparency)), 
                              LWA_ALPHA);
    
    // Enable DWM blur behind for glass effect
    DWM_BLURBEHIND bb = {};
    bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    bb.fEnable = TRUE;
    bb.hRgnBlur = CreateRectRgn(0, 0, config_.width, config_.height);
    DwmEnableBlurBehindWindow(splashWindow_, &bb);
    DeleteObject(bb.hRgnBlur);
    
    // Center window
    CenterWindow();
    
    // Create Direct2D render target
    RECT rc;
    GetClientRect(splashWindow_, &rc);
    
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
    
    // Ensure factory exists; attempt to create if missing
    if (!d2dFactory_) {
        HRESULT hrFactory = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory_);
        if (FAILED(hrFactory) || !d2dFactory_) {
            LogEvent(L"D2D1 factory unavailable; cannot create render target", ::LogLevel::ERROR);
            return false;
        }
    }
    
    HRESULT hr = d2dFactory_->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(splashWindow_, size),
        &renderTarget_
    );
    
    if (FAILED(hr) || !renderTarget_) {
        LogEvent(L"Failed to create render target", ::LogLevel::ERROR);
        return false;
    }
    
    return true;
}

bool CinematicSplashScreen::LoadResources() {
    if (!renderTarget_) {
        return false;
    }
    
    // Ensure DirectWrite factory exists
    if (!writeFactory_) {
        HRESULT hrWF = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(writeFactory_),
                                          reinterpret_cast<IUnknown**>(&writeFactory_));
        if (FAILED(hrWF) || !writeFactory_) {
            LogEvent(L"DirectWrite factory unavailable; skipping text resources", ::LogLevel::WARNING);
            // We can proceed without text; just skip text resources
        }
    }
    
    HRESULT hr;
    
    // Create brushes for water effects
    hr = renderTarget_->CreateSolidColorBrush(
        D2D1::ColorF(0.1f, 0.3f, 0.6f, 0.8f), // Deep water blue
        &waterBrush_
    );
    if (FAILED(hr)) return false;
    
    hr = renderTarget_->CreateSolidColorBrush(
        D2D1::ColorF(0.7f, 0.9f, 1.0f, 0.4f), // Light ripple color
        &rippleBrush_
    );
    if (FAILED(hr)) return false;
    
    hr = renderTarget_->CreateSolidColorBrush(
        D2D1::ColorF(0.2f, 0.5f, 0.8f, 0.9f), // Water drop color
        &dropBrush_
    );
    if (FAILED(hr)) return false;
    
    hr = renderTarget_->CreateSolidColorBrush(
        D2D1::ColorF(0.2f, 0.6f, 0.2f, 0.7f), // Leaf color
        &leafBrush_
    );
    if (FAILED(hr)) return false;
    
    // Create gradient brush for ripples
    ID2D1GradientStopCollection* gradientStops = nullptr;
    D2D1_GRADIENT_STOP stops[3] = {
        { 0.0f, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.8f) },
        { 0.5f, D2D1::ColorF(0.7f, 0.9f, 1.0f, 0.4f) },
        { 1.0f, D2D1::ColorF(0.1f, 0.3f, 0.6f, 0.0f) }
    };
    
    hr = renderTarget_->CreateGradientStopCollection(
        stops, 3, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &gradientStops
    );
    if (FAILED(hr)) return false;
    
    hr = renderTarget_->CreateRadialGradientBrush(
        D2D1::RadialGradientBrushProperties(D2D1::Point2F(0, 0), D2D1::Point2F(0, 0), 50, 50),
        gradientStops, &rippleGradientBrush_
    );
    gradientStops->Release();
    if (FAILED(hr)) return false;
    
    // Create text formats
    hr = writeFactory_->CreateTextFormat(
        L"Segoe UI Light",
        nullptr,
        DWRITE_FONT_WEIGHT_LIGHT,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        48.0f,
        L"en-US",
        &titleFormat_
    );
    if (FAILED(hr)) return false;
    
    hr = writeFactory_->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        16.0f,
        L"en-US",
        &statusFormat_
    );
    if (FAILED(hr)) return false;
    
    titleFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    titleFormat_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    statusFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    statusFormat_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    
    return true;
}

void CinematicSplashScreen::CleanupResources() {
    StopLifecycleWatchdog();
    if (rippleGradientBrush_) rippleGradientBrush_->Release();
    if (leafBrush_) leafBrush_->Release();
    if (dropBrush_) dropBrush_->Release();
    if (rippleBrush_) rippleBrush_->Release();
    if (waterBrush_) waterBrush_->Release();
    if (statusFormat_) statusFormat_->Release();
    if (titleFormat_) titleFormat_->Release();
    if (renderTarget_) renderTarget_->Release();
    if (writeFactory_) writeFactory_->Release();
    if (d2dFactory_) d2dFactory_->Release();
    
    // Cleanup audio resources
    if (audioRenderClient_) audioRenderClient_->Release();
    if (audioClient_) audioClient_->Release();
    if (audioDevice_) audioDevice_->Release();
    if (audioEnumerator_) audioEnumerator_->Release();
}

LRESULT CALLBACK CinematicSplashScreen::SplashWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return 0;
    }
    
    CinematicSplashScreen* splash = reinterpret_cast<CinematicSplashScreen*>(
        GetWindowLongPtr(hwnd, GWLP_USERDATA)
    );
    
    if (splash) {
        return splash->HandleMessage(hwnd, msg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CinematicSplashScreen::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT:
            OnPaint();
            return 0;
            
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_KEYDOWN:
            if (config_.allowManualDismiss) {
                Dismiss();
            }
            return 0;
            
        case WM_TIMER:
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

void CinematicSplashScreen::OnPaint() {
    if (!renderTarget_) {
        return;
    }
    
    renderTarget_->BeginDraw();
    
    // Clear with transparent background
    renderTarget_->Clear(D2D1::ColorF(0.0f, 0.1f, 0.2f, config_.transparency));
    
    // Render water background
    RenderWaterBackground();
    
    // Render floating leaves
    RenderFloatingLeaves();
    
    // Render water drop
    RenderWaterDrop();
    
    // Render ripples
    RenderRipples();
    
    // Render UI elements
    RenderUI();

    // Draw auto message (auxStatus_) below status if present
    if (statusFormat_ && !auxStatus_.empty()) {
        ID2D1SolidColorBrush* hintBrush = nullptr;
        renderTarget_->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.6f), &hintBrush);
        if (hintBrush) {
            D2D1_SIZE_F size = renderTarget_->GetSize();
            float progressY = size.height * 0.8f;
            D2D1_RECT_F hintRect = D2D1::RectF(0, progressY + 40, size.width, progressY + 70);
            renderTarget_->DrawText(
                auxStatus_.c_str(),
                static_cast<UINT32>(auxStatus_.length()),
                statusFormat_,
                hintRect,
                hintBrush
            );
            hintBrush->Release();
        }
    }
    
    HRESULT hr = renderTarget_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        // Need to recreate render target
        renderTarget_->Release();
        renderTarget_ = nullptr;
        LoadResources();
    }
}

void CinematicSplashScreen::RenderWaterBackground() {
    if (!waterBrush_) return;
    
    D2D1_SIZE_F size = renderTarget_->GetSize();
    
    // Create subtle water movement effect
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - startTime_).count();
    
    // Draw water surface with gentle waves
    for (int i = 0; i < 20; ++i) {
        float y = size.height * 0.7f + std::sin(elapsed * 0.5f + i * 0.3f) * 5.0f;
        float alpha = 0.1f + 0.1f * std::sin(elapsed * 0.3f + i * 0.5f);
        
        waterBrush_->SetOpacity(alpha);
        renderTarget_->DrawLine(
            D2D1::Point2F(0, y),
            D2D1::Point2F(size.width, y),
            waterBrush_, 2.0f
        );
    }
}

void CinematicSplashScreen::RenderFloatingLeaves() {
    if (!leafBrush_) return;
    
    auto now = std::chrono::steady_clock::now();
    D2D1_SIZE_F size = renderTarget_->GetSize();
    
    for (auto& leaf : leaves_) {
        if (!leaf.visible) continue;
        
        auto elapsed = std::chrono::duration<float>(now - leaf.startTime).count();
        
        // Update leaf position with gentle floating motion
        leaf.bobOffset = std::sin(elapsed * leaf.bobSpeed) * 3.0f;
        
        D2D1_ELLIPSE leafEllipse = D2D1::Ellipse(
            D2D1::Point2F(leaf.x, leaf.y + leaf.bobOffset),
            15.0f, 8.0f
        );
        
        leafBrush_->SetOpacity(0.7f);
        renderTarget_->FillEllipse(leafEllipse, leafBrush_);
    }
}

void CinematicSplashScreen::RenderWaterDrop() {
    if (!dropVisible_ || !dropBrush_) return;
    
    D2D1_ELLIPSE dropEllipse = D2D1::Ellipse(
        D2D1::Point2F(config_.width * 0.5f, dropY_),
        config_.physics.dropRadius,
        config_.physics.dropRadius
    );
    
    dropBrush_->SetOpacity(0.8f);
    renderTarget_->FillEllipse(dropEllipse, dropBrush_);
}

void CinematicSplashScreen::RenderRipples() {
    if (!rippleGradientBrush_) return;
    
    auto now = std::chrono::steady_clock::now();
    
    for (const auto& ripple : ripples_) {
        if (!ripple.active) continue;
        
        auto elapsed = std::chrono::duration<float>(now - ripple.startTime).count();
        float normalizedTime = elapsed / 3.0f; // Ripple lasts 3 seconds
        
        if (normalizedTime >= 1.0f) continue;
        
        // Update gradient center
        rippleGradientBrush_->SetCenter(D2D1::Point2F(ripple.centerX, ripple.centerY));
        rippleGradientBrush_->SetRadiusX(ripple.radius);
        rippleGradientBrush_->SetRadiusY(ripple.radius);
        
        float alpha = ripple.amplitude * (1.0f - normalizedTime);
        rippleGradientBrush_->SetOpacity(alpha);
        
        D2D1_ELLIPSE rippleEllipse = D2D1::Ellipse(
            D2D1::Point2F(ripple.centerX, ripple.centerY),
            ripple.radius, ripple.radius
        );
        
        renderTarget_->DrawEllipse(rippleEllipse, rippleGradientBrush_, 2.0f);
    }
}

void CinematicSplashScreen::RenderUI() {
    D2D1_SIZE_F size = renderTarget_->GetSize();
    
    // Create text brush
    ID2D1SolidColorBrush* textBrush = nullptr;
    renderTarget_->CreateSolidColorBrush(
        D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.9f),
        &textBrush
    );
    
    if (textBrush && titleFormat_) {
        // Draw title
        D2D1_RECT_F titleRect = D2D1::RectF(0, size.height * 0.2f, size.width, size.height * 0.4f);
        renderTarget_->DrawText(
            L"RainmeterManager",
            15,
            titleFormat_,
            titleRect,
            textBrush
        );
    }
    
    if (textBrush && statusFormat_) {
        // Draw progress bar background
        float progressY = size.height * 0.8f;
        float progressWidth = size.width * 0.6f;
        float progressHeight = 4.0f;
        float progressX = (size.width - progressWidth) * 0.5f;
        
        D2D1_RECT_F progressBg = D2D1::RectF(
            progressX, progressY, progressX + progressWidth, progressY + progressHeight
        );
        
        ID2D1SolidColorBrush* progressBgBrush = nullptr;
        renderTarget_->CreateSolidColorBrush(
            D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.2f),
            &progressBgBrush
        );
        
        if (progressBgBrush) {
            renderTarget_->FillRectangle(progressBg, progressBgBrush);
            progressBgBrush->Release();
        }
        
        // Draw progress bar fill
        float fillWidth = progressWidth * (currentProgress_ / 100.0f);
        D2D1_RECT_F progressFill = D2D1::RectF(
            progressX, progressY, progressX + fillWidth, progressY + progressHeight
        );
        
        ID2D1SolidColorBrush* progressFillBrush = nullptr;
        renderTarget_->CreateSolidColorBrush(
            D2D1::ColorF(0.3f, 0.7f, 1.0f, 0.8f),
            &progressFillBrush
        );
        
        if (progressFillBrush) {
            renderTarget_->FillRectangle(progressFill, progressFillBrush);
            progressFillBrush->Release();
        }
        
        // Draw status text
        if (!currentStatus_.empty()) {
            D2D1_RECT_F statusRect = D2D1::RectF(0, progressY + 20, size.width, progressY + 50);
            renderTarget_->DrawText(
                currentStatus_.c_str(),
                static_cast<UINT32>(currentStatus_.length()),
                statusFormat_,
                statusRect,
                textBrush
            );
        }
    }
    
    if (textBrush) {
        textBrush->Release();
    }
}

// Continue with remaining methods in next part due to length...
void CinematicSplashScreen::TriggerWaterDrop(float x, float y) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    
    if (x < 0 || y < 0) {
        // Use center of screen
        x = config_.width * 0.5f;
        y = config_.height * 0.1f; // Start from top
    }
    
    dropY_ = y;
    dropVelocity_ = 0;
    dropVisible_ = true;
    dropFalling_ = true;
    dropStartTime_ = std::chrono::steady_clock::now();
    
    LogEvent(L"Water drop triggered at (" + std::to_wstring(x) + L", " + std::to_wstring(y) + L")");
}

void CinematicSplashScreen::AnimationThreadFunc() {
    const auto targetFrameTime = std::chrono::microseconds(16667); // ~60 FPS
    
    while (isAnimating_ && !shouldStopAnimation_) {
        auto frameStart = std::chrono::steady_clock::now();
        
        // Calculate delta time
        float deltaTime = std::chrono::duration<float>(frameStart - lastFrameTime_).count();
        deltaTime *= config_.physics.animationSpeed; // Apply slow motion factor
        lastFrameTime_ = frameStart;
        
        // Update water drop physics
        if (dropVisible_ && dropFalling_) {
            UpdateWaterDrop(deltaTime);
        }
        
        // Update ripples
        UpdateRipples(deltaTime);
        
        // Update leaves
        UpdateLeaves(deltaTime);

        // Rotate witty messages
        if (config_.enableAutoMessages) {
            auto now2 = std::chrono::steady_clock::now();
            if (now2 - lastMessageSwap_ >= std::chrono::seconds(4)) {
                std::lock_guard<std::mutex> lock(stateMutex_);
                if (!firstHintShown_) {
                    firstHintShown_ = true;
                    wittyIndex_ = 0;
                    auxStatus_ = wittyMessages_.empty() ? L"" : wittyMessages_[wittyIndex_];
                } else if (!wittyMessages_.empty()) {
                    wittyIndex_ = (wittyIndex_ + 1) % wittyMessages_.size();
                    auxStatus_ = wittyMessages_[wittyIndex_];
                }
                lastMessageSwap_ = now2;
            }
        }
        
        // Trigger window repaint
        if (splashWindow_) {
            InvalidateRect(splashWindow_, nullptr, FALSE);
        }
        
        // Frame rate limiting
        auto frameEnd = std::chrono::steady_clock::now();
        auto frameTime = frameEnd - frameStart;
        if (frameTime < targetFrameTime) {
            std::this_thread::sleep_for(targetFrameTime - frameTime);
        }
        
        // Update FPS counter
        frameCount_++;
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration<float>(now - lastFPSUpdate_).count() >= 1.0f) {
            frameRate_ = frameCount_;
            frameCount_ = 0;
            lastFPSUpdate_ = now;
        }
    }
}

void CinematicSplashScreen::UpdateWaterDrop(float deltaTime) {
    const float waterSurfaceY = config_.height * 0.7f;
    
    // Apply gravity
    dropVelocity_ += config_.physics.gravity * deltaTime * 10.0f; // Scale for visual effect
    dropY_ += dropVelocity_ * deltaTime;
    
    // Check for water surface impact
    if (dropY_ >= waterSurfaceY) {
        // Impact! Create ripple
        CreateRipple(config_.width * 0.5f, waterSurfaceY, 1.0f);
        
        // Hide drop
        dropVisible_ = false;
        dropFalling_ = false;
        
        LogEvent(L"Water drop impact at surface level");
    }
}

void CinematicSplashScreen::UpdateRipples(float deltaTime) {
    auto now = std::chrono::steady_clock::now();
    
    for (auto& ripple : ripples_) {
        if (!ripple.active) continue;
        
        auto elapsed = std::chrono::duration<float>(now - ripple.startTime).count();
        
        // Update ripple properties
        ripple.radius = config_.physics.impactRadius + 
                       config_.physics.rippleSpeed * elapsed;
        ripple.amplitude = std::pow(config_.physics.rippleDecay, elapsed);
        
        // Deactivate old ripples
        if (elapsed > 5.0f || ripple.amplitude < 0.01f) {
            ripple.active = false;
        }
    }
}

void CinematicSplashScreen::UpdateLeaves(float deltaTime) {
    // Gentle floating motion for leaves is handled in rendering
    // This could be expanded for more complex leaf physics
}

void CinematicSplashScreen::CreateRipple(float x, float y, float intensity) {
    // Find next available ripple slot
    for (int i = 0; i < config_.physics.maxRipples; ++i) {
        int index = (nextRippleIndex_ + i) % config_.physics.maxRipples;
        if (!ripples_[index].active) {
            ripples_[index].centerX = x;
            ripples_[index].centerY = y;
            ripples_[index].radius = config_.physics.impactRadius;
            ripples_[index].amplitude = intensity;
            ripples_[index].phase = 0;
            ripples_[index].startTime = std::chrono::steady_clock::now();
            ripples_[index].active = true;
            nextRippleIndex_ = (index + 1) % config_.physics.maxRipples;
            break;
        }
    }
}

void CinematicSplashScreen::InitializeLeaves() {
    leaves_.resize(3); // Start with 3 leaves
    
    for (size_t i = 0; i < leaves_.size(); ++i) {
        leaves_[i].x = config_.width * (0.2f + 0.6f * dis(gen));
        leaves_[i].y = config_.height * 0.7f + dis(gen) * 20.0f;
        leaves_[i].targetX = leaves_[i].x;
        leaves_[i].targetY = leaves_[i].y;
        leaves_[i].rotation = dis(gen) * TWO_PI;
        leaves_[i].bobSpeed = 1.0f + dis(gen) * 2.0f;
        leaves_[i].startTime = std::chrono::steady_clock::now();
        leaves_[i].visible = true;
    }
}

bool CinematicSplashScreen::InitializeAudio() {
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                 CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                 reinterpret_cast<void**>(&audioEnumerator_));
    if (FAILED(hr)) return false;
    
    hr = audioEnumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &audioDevice_);
    if (FAILED(hr)) return false;
    
    hr = audioDevice_->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                               nullptr, reinterpret_cast<void**>(&audioClient_));
    if (FAILED(hr)) return false;
    
    // Initialize audio client with appropriate format
    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = CHANNELS;
    wfx.nSamplesPerSec = SAMPLE_RATE;
    wfx.wBitsPerSample = BITS_PER_SAMPLE;
    wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    
    hr = audioClient_->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 10000000, 0, &wfx, nullptr);
    if (FAILED(hr)) return false;
    
    hr = audioClient_->GetService(__uuidof(IAudioRenderClient),
                                 reinterpret_cast<void**>(&audioRenderClient_));
    if (FAILED(hr)) return false;
    
    hr = audioClient_->Start();
    return SUCCEEDED(hr);
}

void CinematicSplashScreen::AudioThreadFunc() {
    if (!audioClient_ || !audioRenderClient_) return;
    
    UINT32 bufferFrameCount;
    audioClient_->GetBufferSize(&bufferFrameCount);
    
    while (audioActive_) {
        UINT32 numFramesPadding;
        audioClient_->GetCurrentPadding(&numFramesPadding);
        UINT32 numFramesAvailable = bufferFrameCount - numFramesPadding;
        
        if (numFramesAvailable > 0) {
            BYTE* pData;
            audioRenderClient_->GetBuffer(numFramesAvailable, &pData);
            
            // Generate ambient water sounds
            GenerateWaterSounds(reinterpret_cast<float*>(pData), 
                              numFramesAvailable * CHANNELS);
            
            audioRenderClient_->ReleaseBuffer(numFramesAvailable, 0);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void CinematicSplashScreen::GenerateWaterSounds(float* buffer, int sampleCount) {
    static float phase1 = 0, phase2 = 0, phase3 = 0;
    const float freq1 = 220.0f, freq2 = 440.0f, freq3 = 110.0f;
    
    for (int i = 0; i < sampleCount; i += 2) {
        // Generate gentle water ambient sound with multiple sine waves
        float sample = 0;
        sample += 0.3f * std::sin(phase1) * currentVolume_;
        sample += 0.2f * std::sin(phase2) * currentVolume_;
        sample += 0.1f * std::sin(phase3) * currentVolume_;
        
        // Add gentle noise for water texture
        sample += (dis(gen) - 0.5f) * 0.05f * currentVolume_;
        
        // Apply to both channels
        buffer[i] = sample;     // Left
        buffer[i + 1] = sample; // Right
        
        // Update phases
        phase1 += TWO_PI * freq1 / SAMPLE_RATE;
        phase2 += TWO_PI * freq2 / SAMPLE_RATE;
        phase3 += TWO_PI * freq3 / SAMPLE_RATE;
        
        if (phase1 > TWO_PI) phase1 -= TWO_PI;
        if (phase2 > TWO_PI) phase2 -= TWO_PI;
        if (phase3 > TWO_PI) phase3 -= TWO_PI;
    }
}

void CinematicSplashScreen::CalculateOptimalSize() {
    // Get primary monitor dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    if (config_.enable4K && screenWidth >= 3840 && screenHeight >= 2160) {
        // Use true 4K resolution
        config_.width = 3840;
        config_.height = 2160;
    } else if (screenWidth >= 1920 && screenHeight >= 1080) {
        // Use 1080p
        config_.width = 1920;
        config_.height = 1080;
    } else {
        // Scale to 80% of screen size
        config_.width = static_cast<int>(screenWidth * 0.8f);
        config_.height = static_cast<int>(screenHeight * 0.8f);
    }
}

void CinematicSplashScreen::CenterWindow() {
    if (!splashWindow_) return;
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    int x = (screenWidth - config_.width) / 2;
    int y = (screenHeight - config_.height) / 2;
    
    SetWindowPos(splashWindow_, HWND_TOPMOST, x, y, config_.width, config_.height, 
                SWP_NOACTIVATE);
}

void CinematicSplashScreen::LogEvent(const std::wstring& event, ::LogLevel level) const {
    auto& logger = RainmeterManager::Core::Logger::GetInstance();
    RainmeterManager::Core::LogLevel coreLvl = RainmeterManager::Core::LogLevel::Info;
    switch (level) {
        case ::LogLevel::TRACE:    coreLvl = RainmeterManager::Core::LogLevel::Trace; break;
        case ::LogLevel::DEBUG:    coreLvl = RainmeterManager::Core::LogLevel::Debug; break;
        case ::LogLevel::INFO:     coreLvl = RainmeterManager::Core::LogLevel::Info; break;
        case ::LogLevel::WARNING:  coreLvl = RainmeterManager::Core::LogLevel::Warning; break;
        case ::LogLevel::ERROR:    coreLvl = RainmeterManager::Core::LogLevel::Error; break;
        case ::LogLevel::CRITICAL: coreLvl = RainmeterManager::Core::LogLevel::Critical; break;
        case ::LogLevel::FATAL:    coreLvl = RainmeterManager::Core::LogLevel::Fatal; break;
        default:                   coreLvl = RainmeterManager::Core::LogLevel::Info; break;
    }
    logger.LogWide(coreLvl, L"CinematicSplashScreen", event);
}

// Additional methods for compatibility and manager
bool CinematicSplashScreen::IsVisible() const {
    std::lock_guard<std::mutex> lock(stateMutex_);
    return isVisible_;
}

bool CinematicSplashScreen::IsDismissed() const {
    std::lock_guard<std::mutex> lock(stateMutex_);
    return isDismissed_;
}

void CinematicSplashScreen::SetCompletionCallback(CompletionCallback callback) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    completionCallback_ = std::move(callback);
}

void CinematicSplashScreen::Dismiss() {
    Hide();
}

void CinematicSplashScreen::StartLifecycleWatchdog() {
    if (lifecycleActive_) return;
    lifecycleActive_ = true;
    lifecycleThread_ = std::thread([this]() {
        int minMs = (config_.minDisplayTimeMs < 0) ? 0 : config_.minDisplayTimeMs;
        int maxMs = (config_.minDisplayTimeMs > config_.maxDisplayTimeMs) ? config_.minDisplayTimeMs : config_.maxDisplayTimeMs;
        const auto minSpan = std::chrono::milliseconds(minMs);
        const auto maxSpan = std::chrono::milliseconds(maxMs);
        auto start = startTime_;
        for (;;) {
            if (shouldStopAnimation_) break;
            auto now = std::chrono::steady_clock::now();
            auto elapsed = now - start;
            bool minElapsed = elapsed >= minSpan;
            bool maxElapsed = elapsed >= maxSpan;
            int progress = 0;
            {
                std::lock_guard<std::mutex> lock(stateMutex_);
                progress = currentProgress_;
            }
            if (minElapsed && (progress >= 100 || (!config_.extendIfLoading) || maxElapsed)) {
                // Safe auto-dismiss
                Dismiss();
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        lifecycleActive_ = false;
    });
}

void CinematicSplashScreen::StopLifecycleWatchdog() {
    if (lifecycleThread_.joinable()) {
        lifecycleThread_.join();
    }
}

void CinematicSplashScreen::InitializeMessages() {
    wittyMessages_.clear();
    // Short, safe quips for rotation (expandable later)
    wittyMessages_.push_back(L"Dad joke: I used to hate facial hair… but then it grew on me.");
    wittyMessages_.push_back(L"Tip: You can enable/disable providers in Options → Dashboard.");
    wittyMessages_.push_back(L"Life advice: Small steps every day beat big steps someday.");
    wittyMessages_.push_back(L"Dad joke: Why don’t skeletons fight each other? They don’t have the guts.");
    wittyMessages_.push_back(L"Pro tip: Keep monitoring on; control is opt-in and safe by default.");
}

bool CinematicSplashScreen::ProcessMessages() {
    if (!splashWindow_) return false;
    
    MSG msg;
    while (PeekMessage(&msg, splashWindow_, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        
        if (msg.message == WM_QUIT) {
            return false;
        }
    }
    
    return isVisible_;
}

// SplashManager Implementation
SplashManager& SplashManager::GetInstance() {
    if (!instance_) {
        instance_ = new SplashManager();
    }
    return *instance_;
}

void SplashManager::DestroyInstance() {
    delete instance_;
    instance_ = nullptr;
}

bool SplashManager::ShowSplash(HINSTANCE hInstance) {
    CinematicSplashScreen::Config config;
    return ShowSplash(hInstance, config);
}

bool SplashManager::ShowSplash(HINSTANCE hInstance, const CinematicSplashScreen::Config& config) {
    if (isActive_) {
        return true;
    }
    
    splashScreen_ = std::make_unique<CinematicSplashScreen>(hInstance, config);
    isActive_ = splashScreen_->Show();
    
    return isActive_;
}

void SplashManager::UpdateProgress(int percentage, const std::wstring& status) {
    if (splashScreen_) {
        splashScreen_->UpdateProgress(percentage, status);
    }
}

void SplashManager::HideSplash() {
    if (splashScreen_) {
        splashScreen_->Hide();
        splashScreen_.reset();
        isActive_ = false;
    }
}

bool SplashManager::ProcessMessages() {
    if (splashScreen_) {
        return splashScreen_->ProcessMessages();
    }
    return false;
}
