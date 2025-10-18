#!/usr/bin/env pwsh
# Simple build script that actually works

param(
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

Write-Host "`nRainmeterManager Quick Build" -ForegroundColor Cyan
Write-Host "============================`n" -ForegroundColor Cyan

# Find Visual Studio
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    Write-Error "Visual Studio not found. Please install Visual Studio 2022."
    exit 1
}

$vsPath = & $vswhere -latest -property installationPath
if (-not $vsPath) {
    Write-Error "Could not locate Visual Studio installation."
    exit 1
}

Write-Host "Found Visual Studio at: $vsPath`n" -ForegroundColor Green

# Setup paths
$vcvars = "$vsPath\VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvars)) {
    Write-Error "vcvars64.bat not found at: $vcvars"
    exit 1
}
$rootDir = $PSScriptRoot
$buildDir = "$rootDir\build"
$srcDir = "$rootDir\src"
$resDir = "$rootDir\resources"
$binDir = "$buildDir\bin\x64\$Config"
$objDir = "$buildDir\obj\x64\$Config"

# Create directories
New-Item -ItemType Directory -Force -Path $binDir | Out-Null
New-Item -ItemType Directory -Force -Path $objDir | Out-Null

Write-Host "Configuration: $Config" -ForegroundColor Yellow
Write-Host "Output: $binDir`n" -ForegroundColor Yellow

# Build using cl.exe directly
Write-Host "Compiling..." -ForegroundColor Cyan

$compilerFlags = @(
    "/nologo"
    "/W4"
    "/O2"
    "/MT"
    "/EHsc"
    "/std:c++17"
    "/D_UNICODE"
    "/DUNICODE"
    "/D_WINDOWS"
    "/DNDEBUG"
    "/I`"$srcDir`""
    "/Fo`"$objDir\\`""
    "/c"
)

$linkerFlags = @(
    "/nologo"
    "/SUBSYSTEM:WINDOWS"
    "user32.lib"
    "gdi32.lib"
    "comctl32.lib"
    "shell32.lib"
    "shlwapi.lib"
    "comdlg32.lib"
    "ole32.lib"
    "advapi32.lib"
    "wininet.lib"
    "dbghelp.lib"
    "version.lib"
    "bcrypt.lib"
    "wintrust.lib"
    "crypt32.lib"
    "d2d1.lib"
    "dwrite.lib"
)

# Get all cpp files
$sourceFiles = Get-ChildItem -Path $srcDir -Filter *.cpp -Recurse | Select-Object -ExpandProperty FullName

# Build command that sets up VS env and compiles
$buildScript = @"
@echo off
echo Setting up Visual Studio environment...
call "$vcvars"
if errorlevel 1 (
    echo ERROR: Failed to setup Visual Studio environment
    exit /b 1
)
cd /d "$rootDir"

echo Compiling $($sourceFiles.Count) source files...
echo.

"@

foreach ($file in $sourceFiles) {
    $buildScript += "cl $($compilerFlags -join ' ') `"$file`"`n"
    $buildScript += "if errorlevel 1 (echo ERROR: Failed to compile $file & exit /b 1)`n"
}

$objFiles = Get-ChildItem -Path $objDir -Filter *.obj -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName
if ($objFiles) {
    $objList = ($objFiles | ForEach-Object { "`"$_`"" }) -join " "
    $buildScript += "`necho Linking...`n"
    $buildScript += "link /OUT:`"$binDir\RainmeterManager.exe`" $($linkerFlags -join ' ') $objList`n"
    $buildScript += "if errorlevel 1 (echo ERROR: Linking failed & exit /b 1)`n"
}

$buildScript += "echo Build completed successfully!`n"

# Save and run
$tempBat = "$env:TEMP\rainmeter_build_$(Get-Random).bat"
$buildScript | Out-File -FilePath $tempBat -Encoding ASCII

Write-Host "Running build...`n" -ForegroundColor Cyan
& cmd /c $tempBat

$exitCode = $LASTEXITCODE
Remove-Item $tempBat -ErrorAction SilentlyContinue

if ($exitCode -eq 0) {
    Write-Host "`n[SUCCESS] Build succeeded!" -ForegroundColor Green
    Write-Host "Executable: $binDir\RainmeterManager.exe" -ForegroundColor Green
} else {
    Write-Host "`n[ERROR] Build failed with exit code: $exitCode" -ForegroundColor Red
    exit $exitCode
}
