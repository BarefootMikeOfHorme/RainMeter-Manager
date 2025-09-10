# Build reports (errors and logs)

Purpose
- Central place to collect build error reports and related logs so they’re tracked in Git and easy to reference in issues/PRs.

Conventions
- Files are named `BUILD_ERRORS_YYYYMMDD_HHMMSSZ.txt` (UTC timestamps) for easy sorting.
- Use the collector script to generate reports consistently.

Generate a new report
- Run the script from repo root:
  - PowerShell:
    - `scripts/collect_build_errors.ps1`

What’s captured
- dotnet build errors for renderprocess/RenderProcess.csproj (Release)
- MSBuild (C++) build output for RainmeterManager.sln (Release x64)
- Notes on common environment issues (e.g., missing MSVC targets)

Notes
- If MSBuild tools are missing, install "Desktop development with C++" workload for VS 2022.
- Large outputs may be summarized; keep detailed logs near the report if needed.

