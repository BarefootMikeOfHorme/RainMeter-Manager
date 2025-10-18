# ============================================================================
# Content Loaders Verification Script
# RainmeterManager - RenderProcess Loader Health Check
# Version: 1.0.0
# ============================================================================

param(
    [string]$TargetPath = "$PSScriptRoot\..\..\RenderProcess\Content",
    [switch]$CheckManifest = $true,
    [switch]$Verbose = $false
)

$ErrorActionPreference = "Stop"

Write-Host "=" * 80 -ForegroundColor Cyan
Write-Host "Content Loaders Verification" -ForegroundColor Cyan
Write-Host "=" * 80 -ForegroundColor Cyan
Write-Host ""

# Required files
$requiredFiles = @(
    "FileContentLoader.cs",
    "WebContentLoader.cs", 
    "MediaContentLoader.cs",
    "APIContentLoader.cs"
)

$missingFiles = @()
$validFiles = @()
$issues = @()

# Check each required file
Write-Host "📋 Checking required files..." -ForegroundColor Cyan
foreach ($file in $requiredFiles) {
    $filePath = Join-Path $TargetPath $file
    
    if (-not (Test-Path $filePath)) {
        Write-Host "  ❌ Missing: $file" -ForegroundColor Red
        $missingFiles += $file
    } else {
        $fileInfo = Get-Item $filePath
        
        # Basic validation
        if ($fileInfo.Length -eq 0) {
            Write-Host "  ⚠️  Empty: $file" -ForegroundColor Yellow
            $issues += "Empty file: $file"
        } else {
            Write-Host "  ✅ Found: $file ($([math]::Round($fileInfo.Length/1KB, 2)) KB)" -ForegroundColor Green
            $validFiles += $file
            
            # Check for basic C# structure
            try {
                $content = Get-Content $filePath -First 50 -ErrorAction Stop
                
                $hasNamespace = $content | Where-Object { $_ -match "namespace\s+" }
                $hasClass = $content | Where-Object { $_ -match "class\s+\w+" }
                
                if (-not $hasNamespace) {
                    $issues += "$file: Missing namespace declaration"
                }
                if (-not $hasClass) {
                    $issues += "$file: Missing class declaration"
                }
                
                if ($Verbose -and $hasClass) {
                    $className = ($hasClass | Select-Object -First 1) -replace '.*class\s+(\w+).*', '$1'
                    Write-Host "     Class: $className" -ForegroundColor Gray
                }
            } catch {
                $issues += "$file: Failed to read content - $_"
            }
        }
    }
}

Write-Host ""

# Check manifest if requested
if ($CheckManifest) {
    $manifestPath = Join-Path $TargetPath "deployment_manifest.json"
    
    Write-Host "📋 Checking deployment manifest..." -ForegroundColor Cyan
    if (Test-Path $manifestPath) {
        try {
            $manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json
            
            Write-Host "  ✅ Manifest found" -ForegroundColor Green
            Write-Host "     Version: $($manifest.Version)" -ForegroundColor Gray
            Write-Host "     Deployed: $($manifest.DeploymentDate)" -ForegroundColor Gray
            Write-Host "     Files: $($manifest.DeployedCount)/$($manifest.Files.Count)" -ForegroundColor Gray
            
            # Verify manifest hashes
            if ($manifest.Files) {
                $hashMismatches = 0
                foreach ($fileName in $manifest.Files.PSObject.Properties.Name) {
                    $expectedHash = $manifest.Files.$fileName.Hash
                    $filePath = Join-Path $TargetPath $fileName
                    
                    if (Test-Path $filePath) {
                        $actualHash = (Get-FileHash -Path $filePath -Algorithm SHA256).Hash
                        
                        if ($actualHash -ne $expectedHash) {
                            Write-Host "     ⚠️  Hash mismatch: $fileName" -ForegroundColor Yellow
                            $hashMismatches++
                            $issues += "Hash mismatch: $fileName (file may have been modified)"
                        } elseif ($Verbose) {
                            Write-Host "     ✅ Hash verified: $fileName" -ForegroundColor Green
                        }
                    }
                }
                
                if ($hashMismatches -eq 0) {
                    Write-Host "     ✅ All integrity checks passed" -ForegroundColor Green
                }
            }
        } catch {
            Write-Host "  ⚠️  Failed to read manifest: $_" -ForegroundColor Yellow
            $issues += "Manifest parsing error"
        }
    } else {
        Write-Host "  ⚠️  Manifest not found (run deploy_loaders.ps1)" -ForegroundColor Yellow
    }
    
    Write-Host ""
}

# Check Program.cs registration
Write-Host "📋 Checking Program.cs integration..." -ForegroundColor Cyan
$programPath = Join-Path $PSScriptRoot "..\Program.cs"

if (Test-Path $programPath) {
    $programContent = Get-Content $programPath -Raw
    
    $loaderRegistrations = @{
        "FileContentLoader" = $programContent -match "FileContentLoader"
        "WebContentLoader" = $programContent -match "WebContentLoader"
        "MediaContentLoader" = $programContent -match "MediaContentLoader"
        "APIContentLoader" = $programContent -match "APIContentLoader"
    }
    
    $registeredCount = ($loaderRegistrations.Values | Where-Object { $_ -eq $true }).Count
    
    Write-Host "  Registered in DI: $registeredCount/4" -ForegroundColor $(if ($registeredCount -eq 4) { "Green" } else { "Yellow" })
    
    foreach ($loader in $loaderRegistrations.Keys) {
        $status = if ($loaderRegistrations[$loader]) { "✅" } else { "❌" }
        $color = if ($loaderRegistrations[$loader]) { "Green" } else { "Red" }
        Write-Host "     $status $loader" -ForegroundColor $color
        
        if (-not $loaderRegistrations[$loader]) {
            $issues += "$loader not registered in Program.cs"
        }
    }
} else {
    Write-Host "  ⚠️  Program.cs not found at expected location" -ForegroundColor Yellow
    $issues += "Cannot verify DI registration"
}

Write-Host ""

# Summary
Write-Host "=" * 80 -ForegroundColor Cyan
Write-Host "VERIFICATION SUMMARY" -ForegroundColor White
Write-Host "=" * 80 -ForegroundColor Cyan

$totalScore = $validFiles.Count
$maxScore = $requiredFiles.Count

Write-Host "  Valid files: $totalScore/$maxScore" -ForegroundColor $(if ($totalScore -eq $maxScore) { "Green" } else { "Yellow" })
Write-Host "  Missing files: $($missingFiles.Count)" -ForegroundColor $(if ($missingFiles.Count -eq 0) { "Green" } else { "Red" })
Write-Host "  Issues found: $($issues.Count)" -ForegroundColor $(if ($issues.Count -eq 0) { "Green" } else { "Yellow" })

if ($issues.Count -gt 0) {
    Write-Host ""
    Write-Host "⚠️  Issues:" -ForegroundColor Yellow
    foreach ($issue in $issues) {
        Write-Host "     • $issue" -ForegroundColor Yellow
    }
}

Write-Host ""

# Exit code
$exitCode = if ($missingFiles.Count -eq 0 -and $issues.Count -eq 0) { 0 } else { 1 }

if ($exitCode -eq 0) {
    Write-Host "✅ All checks passed - loaders are ready" -ForegroundColor Green
} else {
    Write-Host "⚠️  Some checks failed - review issues above" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=" * 80 -ForegroundColor Cyan

exit $exitCode
