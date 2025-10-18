#!/usr/bin/env pwsh
# Direct compiler build - no batch files

param([string]$Config = "Release")

$ErrorActionPreference = "Stop"

Write-Host "`nRainmeterManager Direct Build" -ForegroundColor Cyan
Write-Host "=============================`n" -ForegroundColor Cyan

# Find VS using vswhere
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = & $vswhere -latest -property installationPath

# Find VC tools version
$vcToolsVersionFile = Get-ChildItem "$vsPath\VC\Auxiliary\Build\Microsoft.VCToolsVersion.*.txt" | Select-Object -First 1
if ($vcToolsVersionFile) {
    $vcToolsVersion = Get-Content $vcToolsVersionFile.FullName | Select-Object -First 1
} else {
    # Fallback: find the latest MSVC version directory
    $vcToolsVersion = Get-ChildItem "$vsPath\VC\Tools\MSVC" | Sort-Object Name -Descending | Select-Object -First 1 -ExpandProperty Name
}

$vcToolsPath = "$vsPath\VC\Tools\MSVC\$vcToolsVersion"

# Setup paths
$clPath = "$vcToolsPath\bin\Hostx64\x64\cl.exe"
$linkPath = "$vcToolsPath\bin\Hostx64\x64\link.exe"
$libPath = "$vcToolsPath\bin\Hostx64\x64\lib.exe"

if (-not (Test-Path $clPath)) {
    Write-Error "Compiler not found at: $clPath"
    exit 1
}

Write-Host "Using compiler: $clPath`n" -ForegroundColor Green

# Find Windows SDK version
$sdkPath = "${env:ProgramFiles(x86)}\Windows Kits\10"
$sdkVersion = Get-ChildItem "$sdkPath\Include" | Where-Object { $_.Name -match '^10\.' } | Sort-Object Name -Descending | Select-Object -First 1 -ExpandProperty Name

Write-Host "Windows SDK: $sdkVersion`n" -ForegroundColor Green

# Build paths - Define BEFORE using in environment variables
$rootDir = $PSScriptRoot
$srcDir = "$rootDir\src"
$binDir = "$rootDir\build\bin\x64\$Config"
$objDir = "$rootDir\build\obj\x64\$Config"

# Setup environment
$env:INCLUDE = @(
    "$srcDir"  # Add source directory first for project includes
    "$vcToolsPath\include"
    "$vsPath\VC\Auxiliary\VS\include"
    "$sdkPath\Include\$sdkVersion\ucrt"
    "$sdkPath\Include\$sdkVersion\um"
    "$sdkPath\Include\$sdkVersion\shared"
) -join ';'

$env:LIB = @(
    "$vcToolsPath\lib\x64"
    "$sdkPath\Lib\$sdkVersion\ucrt\x64"
    "$sdkPath\Lib\$sdkVersion\um\x64"
) -join ';'

New-Item -ItemType Directory -Force -Path $binDir,$objDir | Out-Null

Write-Host "Source directory: $srcDir" -ForegroundColor Yellow
Write-Host "Configuration: $Config" -ForegroundColor Yellow
Write-Host "Output: $binDir`n" -ForegroundColor Yellow

# Compiler flags
$cflags = @(
    "/nologo", "/W3", "/O2", "/MT", "/EHsc", "/std:c++17"
    "/D_UNICODE", "/DUNICODE", "/D_WINDOWS", "/DNDEBUG"
    "/Fo$objDir\", "/c"
)

# Get source files (exclude specific stubs and tests)
$sourceFiles = Get-ChildItem -Path $srcDir -Filter *.cpp -Recurse | Where-Object {
    $_.Name -ne 'render_ipc_bridge_stub.cpp' -and 
    $_.Name -notmatch '_test\.cpp$' -and
    $_.DirectoryName -notmatch '\\test$' -and
    $_.DirectoryName -notmatch '\\tests$'
}

Write-Host "Compiling $($sourceFiles.Count) files...`n" -ForegroundColor Cyan

$compiled = 0
foreach ($file in $sourceFiles) {
    $compiled++
    Write-Progress -Activity "Compiling" -Status $file.Name -PercentComplete (($compiled / $sourceFiles.Count) * 100)
    
    & $clPath $cflags $file.FullName 2>&1 | Out-Null
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "`n[ERROR] Failed to compile: $($file.Name)" -ForegroundColor Red
        Write-Host "Command: $clPath $cflags $($file.FullName)" -ForegroundColor Yellow
        Write-Host "INCLUDE path: $env:INCLUDE" -ForegroundColor Yellow
        Write-Host "`nFull compiler output:" -ForegroundColor Yellow
        & $clPath $cflags $file.FullName
        exit 1
    }
}

Write-Progress -Activity "Compiling" -Completed

# Link
Write-Host "`nLinking..." -ForegroundColor Cyan

$objFiles = Get-ChildItem -Path $objDir -Filter *.obj
$lflags = @(
    "/nologo", "/SUBSYSTEM:WINDOWS"
    "/OUT:$binDir\RainmeterManager.exe"
)

$libs = @(
    "user32.lib", "gdi32.lib", "comctl32.lib", "shell32.lib"
    "shlwapi.lib", "comdlg32.lib", "ole32.lib", "advapi32.lib"
    "wininet.lib", "dbghelp.lib", "version.lib", "bcrypt.lib"
    "wintrust.lib", "crypt32.lib", "d2d1.lib", "dwrite.lib"
)

& $linkPath $lflags $objFiles.FullName $libs 2>&1 | Out-Null

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n[SUCCESS] Build completed!" -ForegroundColor Green
    Write-Host "Executable: $binDir\RainmeterManager.exe" -ForegroundColor Green
} else {
    Write-Host "`n[ERROR] Linking failed" -ForegroundColor Red
    & $linkPath $lflags $objFiles.FullName $libs
    exit 1
}
