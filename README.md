# Rainmeter Manager

> Latest build: v1.0.0.0-build (PR #1) — 2025-09-10
> - Tag: https://github.com/BarefootMikeOfHorme/RainMeter-Manager/tree/v1.0.0.0-build
> - PR: https://github.com/BarefootMikeOfHorme/RainMeter-Manager/pull/1
> - Changelog: changes/2025-09-10_Build-and-IPC-Prep/CHANGELOG.md

## Status — 2025-10-18

- Branch: `feature/semi-working-boot-2025-09-11` (ahead of `origin`)
- Build: Debug x64 builds successfully (manifest warnings are expected due to explicit `/MANIFEST:NO`).
- Security scans: Reports are in `changes/security_reports/security-toolkit/` (gitleaks, trufflehog, cppcheck).

### Recent Development Progress:

✅ **Core Infrastructure (October 2025)**:
- Complete ConfigurationManager implementation (JSON/INI support, auto-save, thread-safe)
- Service registration framework (ServiceLocator, TelemetryService, UIFramework stubs)
- IPC Manager stub for inter-process communication
- RenderProcessLauncher stub with crash handling
- GoogleTest unit test stubs for Logger, Security, ServiceLocator, ConfigManager
- NSIS installer script stub

✅ **AXIOM Scraper v2.0 Consolidation**:
- Merged 3 fragmented files into single production-ready scraper
- SQLite database + async downloads (aiohttp)
- Adaptive rate limiting + secure extraction
- Startup branding/animations preserved
- Complete metadata extraction with fallbacks

### Known Issues:
- Startup access violation (0xC0000005) shortly after `CreateMainWindow` returns.
  - See `logs/RainmeterManager.log`, `raw_trace.txt`, and minidumps in `dumps/`.
  - Temporary boot tracing (`RawTrace`) added in `src/app/rainmgrapp.cpp` to isolate crash—no features removed or disabled.

### Next Steps:
- Integrate system monitoring code (user to provide)
- Implement remaining stubs (widget system, full IPC)
- Correct icon resource load in window class (use `IDI_ICON1` from `resources/resource.rc`)
- Improve crash log formatting (hex) and add basic symbolization
- Optionally enable AddressSanitizer for Debug x64

An enterprise-grade management application for Rainmeter skins and widgets with advanced security features.

## Project Components

### Core Application (C++)
- **Configuration System**: `src/config/configuration_manager.{h,cpp}` - Thread-safe config with JSON/INI support
- **Service Framework**: `src/core/service_locator.{h,cpp}` - Dependency injection & service registration
- **IPC System**: `src/ipc/ipc_manager.{h,cpp}` - Inter-process communication stub
- **Render Process**: `src/render/render_process_launcher.{h,cpp}` - Sandboxed rendering stub
- **Security**: `src/security/` - AES-GCM encryption, code signing, DPAPI
- **Logging**: `src/core/logger.{h,cpp}` - Multi-level logging system

### Services & Infrastructure
- **Telemetry**: `src/services/telemetry_service.{h,cpp}` - Usage tracking (stub)
- **UI Framework**: `src/ui/ui_framework.{h,cpp}` - Widget/view management (stub)
- **Service Registration**: `src/core/service_registration.cpp` - Wires all services together

### Testing
- **Unit Tests**: `tests/unit/` - GoogleTest framework with stubs for all core components
- **Build Scripts**: `scripts/` - Automated build, CI, dependency verification

### AXIOM Scraper (Python)
- **Main Scraper**: `scripts/scraper/AXIOM.py` - Enterprise Rainmeter skin collection tool
  - SQLite database for efficient storage
  - Async downloads with aiohttp
  - Adaptive rate limiting
  - Secure extraction (path traversal protection)
  - Resume capability
  - Startup branding/animations
- **Utilities**: `scripts/scraper/axiom_utilities.py` - Database management CLI
- **Tests**: `scripts/scraper/axiom_tests.py` - Comprehensive test suite
- **Documentation**: Complete docs in `scripts/scraper/` (PDFs + markdown)

### Installation
- **Installer Stub**: `installer/rainmetermanager_installer.nsi` - NSIS script template

## Architecture

### Dependency Flow Diagram

```mermaid
graph TD
    A[RainmeterManager.exe] --> B[Core Engine]
    B --> C[Plugin System]
    B --> D[UI Framework]
    B --> K[Security Framework]
    D --> E[SkiaSharp Renderer]
    D --> F[DirectX Fallback]
    C --> G[Weather Plugin]
    C --> H[System Monitor Plugin]
    E --> I[Native Skia]
    F --> J[Direct2D/D3D]
    K --> L[AES-GCM / BCrypt]
    K --> M[WinTrust / Code Signing]
    K --> N[DPAPI Secrets]
```

## 🚀 **Phase 1 Complete - Security Framework Implementation**

### Dependency Flow Diagram

```mermaid
graph TD
    A[RainmeterManager.exe] --> B[Core Engine]
    B --> C[Plugin System]
    B --> D[UI Framework]
    B --> K[Security Framework]
    D --> E[SkiaSharp Renderer]
    D --> F[DirectX Fallback]
    C --> G[Weather Plugin]
    C --> H[System Monitor Plugin]
    E --> I[Native Skia]
    F --> J[Direct2D/D3D]
    K --> L[AES-GCM / BCrypt]
    K --> M[WinTrust / Code Signing]
    K --> N[DPAPI Secrets]
```

## 🚀 **Phase 1 Complete - Security Framework Implementation**

### Dependency Flow Diagram

```mermaid
graph TD
    A[RainmeterManager.exe] --> B[Core Engine]
    B --> C[Plugin System]
    B --> D[UI Framework]
    B --> K[Security Framework]
    D --> E[SkiaSharp Renderer]
    D --> F[DirectX Fallback]
    C --> G[Weather Plugin]
    C --> H[System Monitor Plugin]
    E --> I[Native Skia]
    F --> J[Direct2D/D3D]
    K --> L[AES-GCM / BCrypt]
    K --> M[WinTrust / Code Signing]
    K --> N[DPAPI Secrets]
```

## 🚀 **Phase 1 Complete - Security Framework Implementation**

✅ **WinMain Entry Point** - Enterprise-grade application bootstrap  
✅ **Security Framework** - AES-GCM encryption, code signing verification, DPAPI integration  
✅ **Build System** - Updated with security libraries (bcrypt, wintrust, crypt32)  
✅ **License Compliance** - Copyright and version information updated  
✅ **Dependencies** - Comprehensive dependency management and verification

## Prerequisites

Before building, ensure you have all required dependencies:

```powershell
# Verify all dependencies are present
.\scripts\verify_dependencies.ps1

# List all required dependencies
.\scripts\verify_dependencies.ps1 -ListOnly
```

## Build System

This project supports multiple build methods:

## Building with CMake

```bash
# Configure
cmake -S . -B build -G "Visual Studio 16 2019" -A x64

# Build Debug Configuration
cmake --build build --config Debug

# Build Release Configuration
cmake --build build --config Release

# Install (optional)
cmake --install build --config Release --prefix install
```

## Building with Visual Studio

1. Open `RainmeterManager.sln` in Visual Studio
2. Select desired configuration (Debug/Release) and platform (x64/x86)
3. Build solution (F7 or Build → Build Solution)

## Building with Batch Scripts

```bash
# For regular build
scripts/buildscript.bat

# With options
scripts/buildscript.bat --config Debug --platform x64

# CI Build
scripts/ci_build.bat --increment
```

## Debugging

### Visual Studio Debugging

1. Set RainmeterManager as the startup project
2. Select Debug configuration
3. Press F5 to start debugging

### VS Code Debugging

1. Open the project folder in VS Code
2. Select "Launch Debug (Windows)" from the debug configurations
3. Press F5 to start debugging

### Command Line Debugging

For Windows, you can use Visual Studio Developer Command Prompt:

```bash
# Build Debug
scripts/buildscript.bat --config Debug

# Debug with Visual Studio debugger
devenv /debugexe build/bin/x64/Debug/RainmeterManager.exe
```

## Debug Features

The project includes comprehensive debug utilities:

- Debug build detection
- Assert macros with message support
- Debug-only code sections
- Performance timing tools
- Memory tracking utilities

Example usage:

```cpp
#include "core/debug.h"

void SomeFunction() {
    // Times the function execution in debug builds
    MEASURE_FUNCTION;
    
    // Debug-only code
    RM_DEBUG_ONLY(
        LOG_DEBUG("This is a debug-only message");
    );
    
    // Assertions with messages
    RM_ASSERT(pointerValue != nullptr, "Expected valid pointer");
    
    // Performance measurement for blocks
    {
        MEASURE_BLOCK("Database query");
        // ... code to measure ...
    }
}
```

## Version Information

Version information is managed without affecting filenames:
- Source code: Defined in `src/version.h`
- Build: Tracked in build metadata
- Changelogs: Generated with each build

## License (Pre‑Release)

Pre‑release versions (e.g., v1.0.0-*) are licensed under the RainmeterManager Beta Non‑Commercial License (RB‑NC‑1.1). This permits free personal/internal building, testing, and evaluation, but forbids commercial use, paid distribution, or app store publication.

- Full text: see `LICENSE`
- Third‑party attributions: see `NOTICE`

A future premium store release (v1.0.0 and later) will be distributed under a separate commercial EULA.

## Continuous Integration

Use the CI script for automated builds and testing:

```bash
scripts/ci_build.bat --increment --email admin@example.com
```

The CI script:
- Builds multiple configurations
- Runs tests
- Generates changelogs
- Archives build artifacts
- Maintains version history
