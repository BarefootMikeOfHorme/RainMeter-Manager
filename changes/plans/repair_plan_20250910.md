# Repair plan (2025-09-10)

Goal
- Get both C# (renderprocess) and C++ builds green on Windows, with repeatable logs in changes/build_reports/ and a PR open for community review.

Group 1 — Environment setup
- Install Visual Studio 2022 Build Tools C++ workload (vctools, CMake project, Windows 10 SDK)
- Verify MSBuild and CMake (optional) availability

Group 2 — C# renderprocess triage
- Resolve duplicate types by keeping canonical definitions in RenderDataStructures.cs; rename legacy duplicates in IRenderBackend.cs with a Legacy suffix (no deletions)
- Replace C++-style uint32_t with C# types (uint)
- Add missing usings/references (System.Collections.Generic, System.Drawing, Microsoft.Web.WebView2)
- Rebuild and capture report

Group 3 — C++ native build
- Re-run MSBuild after toolchain is present
- If using CMake-first: configure with VS 2022 generator and build Release x64

Group 4 — CI and docs (follow-up)
- Add GitHub Actions workflow for Windows build
- Document developer setup and use of scripts/collect_build_errors.ps1

Tracking
- Place all build reports under changes/build_reports/
- Open issues for remaining errors with pointers to latest report

