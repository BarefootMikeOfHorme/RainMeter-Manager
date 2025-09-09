# RainmeterManager Dependency Verification Script
# Phase 1 Complete - Security Framework Implementation
# This script verifies all dependencies and system requirements

param(
    [switch]$Verbose,
    [switch]$ListOnly,
    [string]$LogPath = "$env:TEMP\RainmeterManager_Dependencies.log"
)

function Write-Log {
    param([string]$Message, [string]$Level = "INFO")
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $logEntry = "[$timestamp] [$Level] $Message"
    if ($Verbose -or $Level -eq "ERROR") {
        Write-Output $logEntry
    }
    Add-Content -Path $LogPath -Value $logEntry
}

function Test-Phase1SecurityLibraries {
    Write-Log "Checking Phase 1 Security Framework libraries..." "INFO"
    
    $securityLibs = @(
        @{Name="bcrypt.dll"; Purpose="AES-GCM encryption"},
        @{Name="wintrust.dll"; Purpose="Code signature verification"},
        @{Name="crypt32.dll"; Purpose="Certificate validation & DPAPI"},
        @{Name="dbghelp.dll"; Purpose="Crash handling"},
        @{Name="version.dll"; Purpose="Version information"}
    )
    
    $allFound = $true
    foreach ($lib in $securityLibs) {
        $libPath = "$env:SystemRoot\System32\$($lib.Name)"
        if (Test-Path $libPath) {
            $version = (Get-Item $libPath).VersionInfo.FileVersion
            Write-Log "✅ $($lib.Name): Found (Version: $version) - $($lib.Purpose)" "SUCCESS"
        } else {
            Write-Log "❌ $($lib.Name): Not found - $($lib.Purpose)" "ERROR"
            $allFound = $false
        }
    }
    
    return $allFound
}

function Test-BuildTools {
    Write-Log "Checking build tools..." "INFO"
    
    $tools = @(
        @{Name="cl"; Command="cl"; Purpose="Visual C++ Compiler"},
        @{Name="rc"; Command="rc"; Purpose="Resource Compiler"},
        @{Name="link"; Command="link"; Purpose="Linker"},
        @{Name="cmake"; Command="cmake --version"; Purpose="CMake Build System"; Optional=$true}
    )
    
    $allFound = $true
    foreach ($tool in $tools) {
        try {
            $result = & cmd.exe /c "where $($tool.Command.Split(' ')[0]) >nul 2>&1 && echo FOUND || echo NOT_FOUND"
            if ($result -eq "FOUND") {
                Write-Log "✅ $($tool.Name): Available - $($tool.Purpose)" "SUCCESS"
            } else {
                $level = if ($tool.Optional) { "WARNING" } else { "ERROR" }
                Write-Log "$(if($tool.Optional){'⚠️'}else{'❌'}) $($tool.Name): Not found - $($tool.Purpose)" $level
                if (-not $tool.Optional) {
                    $allFound = $false
                }
            }
        } catch {
            $level = if ($tool.Optional) { "WARNING" } else { "ERROR" }
            Write-Log "$(if($tool.Optional){'⚠️'}else{'❌'}) $($tool.Name): Check failed - $($tool.Purpose)" $level
            if (-not $tool.Optional) {
                $allFound = $false
            }
        }
    }
    
    return $allFound
}

function Test-ConfigurationFiles {
    Write-Log "Checking configuration files..." "INFO"
    
    $configFiles = @(
        @{Path="dependencies.json"; Required=$true},
        @{Path="config\api_providers.json"; Required=$true},
        @{Path="LICENSE.txt"; Required=$true}
    )
    
    $allFound = $true
    $currentDir = Get-Location
    
    foreach ($file in $configFiles) {
        $fullPath = Join-Path $currentDir $file.Path
        if (Test-Path $fullPath) {
            $fileSize = (Get-Item $fullPath).Length
            Write-Log "✅ $($file.Path): Found ($fileSize bytes)" "SUCCESS"
        } else {
            $level = if ($file.Required) { "ERROR" } else { "WARNING" }
            Write-Log "$(if($file.Required){'❌'}else{'⚠️'}) $($file.Path): Not found" $level
            if ($file.Required) {
                $allFound = $false
            }
        }
    }
    
    return $allFound
}

function Test-Phase2ApplicationCore {
    Write-Log "Checking Phase 2 Application Core Layer requirements..." "INFO"
    
    $coreFiles = @(
        @{Path="src\app\rainmgrapp.cpp"; Purpose="RAINMGRApp singleton class"; Phase="2"},
        @{Path="src\app\rainmgrapp.h"; Purpose="RAINMGRApp singleton header"; Phase="2"},
        @{Path="src\core\service_locator.cpp"; Purpose="Service Locator pattern"; Phase="2"},
        @{Path="src\core\service_locator.h"; Purpose="Service Locator header"; Phase="2"},
        @{Path="src\config\config_manager.cpp"; Purpose="Configuration management"; Phase="2"},
        @{Path="src\config\config_manager.h"; Purpose="Configuration management header"; Phase="2"},
        @{Path="src\core\message_loop.cpp"; Purpose="Enhanced Windows message loop"; Phase="2"; Optional=$true},
        @{Path="src\ui\splash_screen.cpp"; Purpose="Application splash screen"; Phase="2"; Optional=$true}
    )
    
    $implementedCount = 0
    $totalRequired = ($coreFiles | Where-Object { -not $_.Optional }).Count
    $currentDir = Get-Location
    
    Write-Log "Phase 2 Application Core Implementation Status:" "INFO"
    foreach ($file in $coreFiles) {
        $fullPath = Join-Path $currentDir $file.Path
        if (Test-Path $fullPath) {
            Write-Log "✅ $($file.Path): IMPLEMENTED - $($file.Purpose)" "SUCCESS"
            if (-not $file.Optional) { $implementedCount++ }
        } else {
            $level = if ($file.Optional) { "INFO" } else { "WARNING" }
            $icon = if ($file.Optional) { "💡" } else { "⚠️" }
            Write-Log "$icon $($file.Path): NOT IMPLEMENTED (Phase $($file.Phase) target) - $($file.Purpose)" $level
        }
    }
    
    $percentComplete = [math]::Round(($implementedCount / $totalRequired) * 100, 1)
    Write-Log "Phase 2 Progress: $implementedCount/$totalRequired required components ($percentComplete% complete)" "INFO"
    
    # Phase 2 is not required for basic functionality, so return true but log status
    return $true
}

function Show-Summary {
    param([hashtable]$TestResults)
    
    Write-Output "`n" + "="*60
    Write-Output "RAINMETERMANAGER DEPENDENCY VERIFICATION"
    Write-Output "Phase 1 Complete - Security Framework Implementation"
    Write-Output "="*60
    
    Write-Output "`nSECURITY LIBRARIES (PHASE 1):"
    Write-Output "  Status: $(if($TestResults.SecurityLibraries) {'✅ ALL PRESENT'} else {'❌ MISSING LIBRARIES'})"
    
    Write-Output "`nBUILD TOOLS:"
    Write-Output "  Status: $(if($TestResults.BuildTools) {'✅ ALL PRESENT'} else {'❌ MISSING TOOLS'})"
    
    Write-Output "`nCONFIGURATION:"
    Write-Output "  Status: $(if($TestResults.ConfigFiles) {'✅ ALL PRESENT'} else {'❌ MISSING FILES'})"
    
    $totalTests = $TestResults.Values.Count
    $passedTests = ($TestResults.Values | Where-Object { $_ -eq $true }).Count
    $failedTests = $totalTests - $passedTests
    
    Write-Output "`nOVERALL RESULT:"
    Write-Output "  Tests Passed: $passedTests/$totalTests"
    
    if ($failedTests -eq 0) {
        Write-Output "  Status: ✅ READY TO BUILD" 
        Write-Output "`n✨ All dependencies satisfied! You can now run the build scripts."
    } else {
        Write-Output "  Status: ❌ MISSING DEPENDENCIES"
        Write-Output "`n❌ Please install missing dependencies before building."
        Write-Output "`nTo build after resolving dependencies:"
        Write-Output "  - For basic build: scripts\buildscript.bat"
        Write-Output "  - For CI build: scripts\ci_build.bat"
        Write-Output "  - For enterprise build: scripts\enterprise_build.bat"
    }
    
    Write-Output "`nFor detailed information, see: $LogPath"
    Write-Output "="*60
}

# Main execution
Write-Log "Starting RainmeterManager Dependency Verification" "INFO"
Write-Log "Version: Phase 1 Complete - Security Framework Implementation" "INFO"

if ($ListOnly) {
    Write-Output "RainmeterManager Dependencies (Phase 1 Complete):"
    Write-Output "================================================"
    Write-Output ""
    Write-Output "✅ PHASE 1 SECURITY LIBRARIES (Required):"
    Write-Output "  • bcrypt.dll - AES-GCM encryption"
    Write-Output "  • wintrust.dll - Code signature verification" 
    Write-Output "  • crypt32.dll - Certificate validation & DPAPI"
    Write-Output "  • dbghelp.dll - Crash handling & stack traces"
    Write-Output "  • version.dll - Version information API"
    Write-Output ""
    Write-Output "BUILD TOOLS (Required):"
    Write-Output "  • cl.exe - Visual C++ Compiler"
    Write-Output "  • rc.exe - Resource Compiler"
    Write-Output "  • link.exe - Linker"
    Write-Output ""
    Write-Output "BUILD TOOLS (Optional):"
    Write-Output "  • cmake.exe - CMake build system"
    Write-Output "  • makensis.exe - NSIS installer creator"
    Write-Output ""
    Write-Output "CONFIGURATION FILES:"
    Write-Output "  • dependencies.json - Dependency specification"
    Write-Output "  • config\api_providers.json - API configuration"
    Write-Output "  • LICENSE.txt - License information"
    Write-Output ""
    Write-Output "To verify all dependencies: .\scripts\verify_dependencies.ps1"
    exit 0
}

# Run all verification tests
$testResults = @{
    SecurityLibraries = Test-Phase1SecurityLibraries
    BuildTools = Test-BuildTools
    ConfigFiles = Test-ConfigurationFiles
    Phase2ApplicationCore = Test-Phase2ApplicationCore
}

# Show summary
Show-Summary -TestResults $testResults

# Return appropriate exit code
$criticalFailures = @('SecurityLibraries', 'BuildTools', 'ConfigFiles')
$criticalFailed = $criticalFailures | Where-Object { -not $testResults[$_] }

if ($criticalFailed.Count -eq 0) {
    exit 0  # Success
} else {
    exit 1  # Error
}
