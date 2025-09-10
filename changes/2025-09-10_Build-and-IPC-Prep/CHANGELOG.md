# CHANGELOG — v1.0.0.0 (built)

Date: 2025-09-10

Summary
- Focused “low-hanging fruit” pass to harden web rendering, expand curated data sources, and implement robust content loading. Prepared UI scaffolding for the new Options window with Dashboard and Task Manager tabs. Validated a clean .NET build for the render process and captured build output for traceability.

Key changes

C# RenderProcess
- FileContentLoader (renderprocess/Content/):
  - Implemented robust loader supporting text, JSON, XML, HTML, images, and binaries.
  - Encoding detection for text payloads; safe HTML preview rendering.
  - Binary and image type detection with defensive parsing.
  - Registered via DI as a transient service (consistent with project DI patterns).
- WebContentLoader:
  - Removed dependency on System.Web.HttpUtility; added a custom, broadly-compatible query parser.
  - Expanded curated, privacy-conscious sources:
    - NASA Mars Rover photos (api.nasa.gov)
    - USGS earthquakes (earthquake.usgs.gov)
    - Open-Meteo forecasts (api.open-meteo.com)
    - Wikimedia/Wikipedia media (upload.wikimedia.org, *.wikipedia.org)
    - OpenStreetMap tiles (tile.openstreetmap.org)
    - Privacy-friendly YouTube embedding (www.youtube-nocookie.com)
  - Safer embedded HTML: added iframe sandbox and referrerpolicy attributes.
- WebViewRenderer:
  - Hardened domain allowlist to include the curated sources above.
  - Fixed localhost handling to include 127.0.0.1 and ::1 in both allowlist and runtime checks.
  - Removed unsupported CoreWebView2.Stop() call.
  - Removed placeholder/redacted host checks.
- app.manifest:
  - Replaced invalid version placeholder with a valid version: 1.0.0.0.

C++ UI (native)
- Options/Dashboard/Task Manager scaffolding:
  - Added Dashboard tab (src/ui/dashboard_tab.*): window creation, message handling, sizing, and three tiles (CPU %, Memory used/total MB, Network RX/TX MB/s) with placeholder values (“--”).
  - Added Task Manager tab scaffolding (src/ui/task_manager_tab.*) for per‑process monitoring (to be wired next).
  - Integrated Options window wiring scaffolding (src/ui/options_window.*) for showing/hiding tabs (to be finalized).

IPC & managers (native + managed)
- Confirmed existing IPC building blocks (named pipes + shared memory) and identified integration points for Dashboard snapshots and Task Manager process listings (RenderIPCBridge, NamedPipeChannel, SharedMemoryManager).
- Introduced TrustedSitesConfig.cs for managing webview allowlist.

Rendering backends
- SkiaSharp-first approach retained per project preferences; introduced/organized backends scaffolding (e.g., Direct3DRenderer.cs) for future expansion.

Build and security notes
- RenderProcess built successfully (net8.0-windows; win-x64). Build output is archived at changes/build_reports/.
- Notable warnings:
  - System.Text.Json 8.0.0 vulnerability advisories (GHSA-8g4q-xg66-9fp4, GHSA-hh2w-p6rv-4g7w) — plan to bump to the latest secure 8.0.x.
  - OpenTK/OpenTK.GLControl (3.1.0) target .NET Framework TFMs — evaluate removal if unused (SkiaSharp preferred) or upgrade to 4.x if needed.

Next steps
- Wire Dashboard tiles to live snapshots via IPC, then build out Task Manager with per‑process metrics.
- Address package hygiene (update System.Text.Json; audit/upgrade/remove OpenTK).
- Finalize Options window tab wiring and update documentation accordingly.

