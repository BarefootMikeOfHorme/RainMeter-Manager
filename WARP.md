# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

Repository quick facts
- Primary workflow: CMake (use this for builds and tests). Visual Studio/MSBuild are optional for local debugging only.
- Primary languages and toolchains:
  - C++17 (MSVC v143). Build via CMake or Visual Studio 2022 solution.
  - .NET 8 (Windows) for a separate render process (WinForms + SkiaSharp).
  - Optional Python utilities under scripts/scraper.
- Testing: CTest is enabled in CMake; prefer CMake/CTest to run tests.
- Rendering: Preferred renderer is Skia/SkiaSharp; fallback to Direct2D/Direct3D when Skia is unavailable.

Common commands
- Verify dependencies (PowerShell)
  - Run from repo root to validate required build tools and Windows security libraries:
    powershell -ExecutionPolicy Bypass -File .\scripts\verify_dependencies.ps1
  - List dependencies without validating:
    powershell -ExecutionPolicy Bypass -File .\scripts\verify_dependencies.ps1 -ListOnly
  - Verbose diagnostics:
    powershell -ExecutionPolicy Bypass -File .\scripts\verify_dependencies.ps1 -Verbose

- Build C++ app with CMake (Visual Studio 2022 generator, x64)
  - Configure:
    cmake -S . -B build -G "Visual Studio 17 2022" -A x64
  - Build:
    cmake --build build --config Debug
    cmake --build build --config Release
  - Build only the main executable target:
    cmake --build build --config Debug --target RainmeterManager
  - Optional install:
    cmake --install build --config Release --prefix install

- Optional (debug-only) Visual Studio
  - Open RainmeterManager.sln in Visual Studio 2022 and build Debug/Release (x64)

- Scripted builds (C++ app; alternate non-CMake pipeline)
  - Default build (detects cl.exe, rc.exe, etc.; writes logs/artifacts under build/):
    .\scripts\buildscript.bat --config Release --platform x64
  - Debug without tests:
    .\scripts\buildscript.bat --config Debug --platform x64 --skip-tests
  - CI flow (versioning, changelog, archive):
    .\scripts\ci_build.bat --increment
  - Enterprise wrapper (enhanced logging/structure):
    .\scripts\enterprise_build.bat --config Release --platform x64

- Run tests (CTest after a CMake build)
  - All tests (Debug config):
    ctest --test-dir build -C Debug --output-on-failure
  - Run a single test by name regex:
    ctest --test-dir build -C Debug -R NAME_REGEX
  - List available tests (no run):
    ctest --test-dir build -C Debug -N
  - Re-run only failed tests from last run:
    ctest --test-dir build -C Debug --rerun-failed --output-on-failure
  - Verbose test output for a filtered set:
    ctest --test-dir build -C Debug -R NAME_REGEX -VV
  - Note: tests/CMakeLists.txt only adds unit/ or integration/ if their CMakeLists.txt exist; a placeholder test runs otherwise.

- .NET render process (SkiaSharp/Direct3D/WebView2 backends)
  - Build:
    dotnet build .\renderprocess\RenderProcess.csproj -c Debug
  - Run:
    dotnet run --project .\renderprocess\RenderProcess.csproj -c Debug
  - Publish single-file, self-contained (win-x64):
    dotnet publish .\renderprocess\RenderProcess.csproj -c Release -r win-x64

- Example app
  - Run the Illustro clock sample:
    dotnet run --project .\examples\IllustroClockTest.csproj -c Debug

Big-picture architecture overview
- Native C++ application (RainmeterManager.exe)
  - Bootstrap and lifecycle: A Win32 app orchestrated by a RAINMGRApp singleton using a ServiceLocator to register and manage subsystems (SecurityManager, TelemetryService, UIFramework, WidgetManager, ConfigurationManager). Structured exception handling (SEH), health checks, and graceful shutdown are specified in design_docs/bootstrap.md.
  - Source layout (CMake and vcxproj agree on these):
    - src/app: entry/bootstrap (e.g., main, rainmgrapp)
    - src/core: logging, security, service locator, configuration
    - src/ui: UI framework integration and message loop
    - src/widgets: widget framework, templates, and community plugins
  - Security framework (Phase 1 complete): AES-GCM via bcrypt, DPAPI for secrets, WinTrust-based code signing verification, malware pattern scanning hooks. Windows SDK libraries are linked in both CMake (target_link_libraries) and the .vcxproj.

- Rendering model
  - Preferred: Skia/SkiaSharp rendering.
  - Fallback: Direct2D/Direct3D paths when Skia/SkiaSharp isn’t available. The native build defines NO_SKIASHARP if SkiaSharp isn’t found; the .NET render process composes SkiaSharpRenderer, Direct3DRenderer, and WebViewRenderer via Microsoft.Extensions hosting/DI (see renderprocess/Program.cs and RenderProcess.csproj package references).

- Tests
  - CTest harness enabled by CMake. tests/CMakeLists.txt conditionally adds unit and integration subtrees only when those directories contain their own CMakeLists.txt; otherwise a placeholder executable is added so the test phase doesn’t fail. Use ctest -R to filter individual tests.

- Configuration and runtime data
  - User settings: %APPDATA%\RainmeterManager\settings.ini
  - Cache: %LOCALAPPDATA%\RainmeterManager\cache

Useful docs (read for deeper context)
- README.md: multiple build paths (CMake, VS, scripts), dependency verification, security framework status.
- design_docs/bootstrap.md: application bootstrap, services, error handling, graceful shutdown, and rendering bootstrap.
- docs/api/DEVELOPER_API.md: widget framework, plugin interface, rendering and security APIs.
- docs/guides/INTEGRATION_GUIDE.md: adding data providers, Skia-based widget development.
- docs/guides/LEGAL_AND_CONFIG.md: legal/attribution requirements and configuration best practices.

Notes and quirks
- For .bat-based builds, use a Visual Studio Developer Command Prompt (or ensure cl.exe, rc.exe, link.exe are in PATH). The verify_dependencies.ps1 script checks for these and required Windows libraries.
- CMake sets MSVC toolset v143; prefer generator "Visual Studio 17 2022" and target x64.
- If SkiaSharp isn’t detected in the native path, builds compile fallback drawing paths by defining NO_SKIASHARP.
- CI script (scripts/ci_build.bat) writes build metadata, version history, and optional changelogs; artifacts are versioned in metadata, not filenames. C++ build products land under build/bin/x64/{Debug|Release}.
- Standardize on CMake for builds and tests; Visual Studio/MSBuild and enterprise scripts are optional aids.

