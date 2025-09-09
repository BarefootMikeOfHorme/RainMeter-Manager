# Backlog — Prioritized (2025-09-03)

This backlog lists quick wins vs. deeper work with acceptance criteria. All changes are delivered as minimal diffs and tracked as numbered patches under changes/.

## Quick wins (low risk, 1–2 days total)
1) 0001 — Core::Logger adapter
   - Adds RainmeterManager::Core::Logger facade that maps to existing Logger.
   - Acceptance: builds succeed; existing logger untouched; splash/config/app compile.
2) 0002 — Core::Security adapter (DPAPI)
   - Exposes Initialize/Cleanup/EncryptString/DecryptString; forwards providers to existing Security.
   - Acceptance: DPAPI roundtrip test passes; builds succeed.
3) 0003 — Namespace/signature fixes
   - main.cpp qualification for RAINMGRApp; harmonize splash OnPaint signature.
   - Acceptance: compile/link errors resolved.
4) 0004 — Visual Studio project parity
   - Include missing sources and Windows libs in AdditionalDependencies (Debug/Release x64).
   - Acceptance: VS builds green in both configs.
5) 0005 — CMake parity
   - Add adapters to target and ensure Windows libs consistent with VS.
   - Acceptance: CMake Debug/Release builds green.

## Medium (2–4 days)
6) 0006 — Minimal test scaffold
   - GoogleTest + CTest; smoke tests for Logger/Security adapters and service locator.
   - Acceptance: tests pass locally and in CI.
7) 0007 — Installer stub/gating
   - Minimal NSIS script or gating to avoid build breaks if makensis absent.
   - Acceptance: buildscript.bat does not fail when NSIS missing; optional installer created when available.
8) 0008 — GitHub Actions CI baseline
   - Build Debug/Release; run tests; upload artifacts and SBOM.
   - Acceptance: CI green; artifacts retrievable.

## Deeper work (1–2 weeks)
9) 0009 — .rmskinx packaging & validator
   - JSON manifest + hashes; PowerShell validator; sample package and CI gate.
   - Acceptance: validator succeeds on sample; CI fails on invalid package.
10) Security hardening
   - HTTPS-only toggles for any remote endpoints; domain allowlists; signature verification for downloaded artifacts.
   - Acceptance: configuration toggles present; tests for allowlist/blocklist logic.
11) Logging/rotation/perf
   - Finish rotation strategy; evaluate enabling async by default (Release); measure startup/log impact.
   - Acceptance: logs rotate by size; no performance regressions.

## Later (2–4 weeks)
12) Optional SkiaSharp renderer toggle
   - Compile-time option to use Skia for 2D surface; keep Direct2D as default.
   - Acceptance: both render paths compile; feature flagged.
13) Azure DevOps pipeline parity (optional)
   - Basic pipeline aligned with GitHub Actions workflow.
   - Acceptance: pipeline runs build/tests on PRs or pushes as configured.

## Conventions & review
- Every item links to a patch number and includes a rationale & verification in notes.md
- Gate merges on builds/tests passing locally and in CI

