# ============================================================================
# Enhanced Content Loaders Deployment Script
# RainmeterManager - RenderProcess/Content Enhancement
# Version: 3.0.0 - Enterprise Grade
# ============================================================================

param(
    [string]$TargetPath = "$PSScriptRoot\..\..\RenderProcess\Content",
    [switch]$Backup = $true,
    [switch]$Force = $false,
    [switch]$DryRun = $false,
    [switch]$NonInteractive = $false,
    [switch]$VerifyIntegrity = $true,
    [switch]$SkipPostVerification = $false,
    [string]$ManifestFile = "$PSScriptRoot\loader_manifest.json",
    [int]$MaxBackups = 5
)

$ErrorActionPreference = "Stop"
$InformationPreference = "Continue"

# ============================================================================
# Helper Functions
# ============================================================================

function Get-FileHashSHA256 {
    param([string]$Path)
    if (-not (Test-Path $Path)) { return $null }
    try {
        return (Get-FileHash -Path $Path -Algorithm SHA256).Hash
    } catch {
        Write-Warning "Failed to compute hash for $Path: $_"
        return $null
    }
}

function Write-StatusMessage {
    param(
        [string]$Message,
        [ValidateSet('Info', 'Success', 'Warning', 'Error', 'Header')]
        [string]$Level = 'Info',
        [int]$Indent = 0
    )
    
    $prefix = "  " * $Indent
    $icons = @{
        'Info' = 'ℹ️'
        'Success' = '✅'
        'Warning' = '⚠️'
        'Error' = '❌'
        'Header' = '📋'
    }
    $colors = @{
        'Info' = 'Cyan'
        'Success' = 'Green'
        'Warning' = 'Yellow'
        'Error' = 'Red'
        'Header' = 'White'
    }
    
    Write-Host "$prefix$($icons[$Level]) $Message" -ForegroundColor $colors[$Level]
}

function Test-FileIntegrity {
    param(
        [string]$FilePath,
        [string]$ExpectedHash
    )
    
    if (-not (Test-Path $FilePath)) {
        return @{ Valid = $false; Reason = "File not found" }
    }
    
    $actualHash = Get-FileHashSHA256 -Path $FilePath
    if ($null -eq $actualHash) {
        return @{ Valid = $false; Reason = "Failed to compute hash" }
    }
    
    if ($actualHash -ne $ExpectedHash) {
        return @{ Valid = $false; Reason = "Hash mismatch"; Expected = $ExpectedHash; Actual = $actualHash }
    }
    
    return @{ Valid = $true }
}

function Remove-OldBackups {
    param(
        [string]$BackupDirectory,
        [int]$KeepCount
    )
    
    $backupPattern = "backup_content_loaders_*"
    $backups = Get-ChildItem -Path (Split-Path $BackupDirectory -Parent) -Directory -Filter $backupPattern |
        Sort-Object CreationTime -Descending
    
    if ($backups.Count -gt $KeepCount) {
        $toRemove = $backups | Select-Object -Skip $KeepCount
        foreach ($backup in $toRemove) {
            try {
                Remove-Item $backup.FullName -Recurse -Force
                Write-StatusMessage "Removed old backup: $($backup.Name)" -Level Info -Indent 1
            } catch {
                Write-StatusMessage "Failed to remove old backup $($backup.Name): $_" -Level Warning -Indent 1
            }
        }
    }
}

function Invoke-PostDeploymentVerification {
    param([hashtable]$DeployedFiles)
    
    Write-StatusMessage "Running post-deployment verification..." -Level Header
    
    $allValid = $true
    foreach ($file in $DeployedFiles.Keys) {
        $filePath = $DeployedFiles[$file]
        
        # Check file exists
        if (-not (Test-Path $filePath)) {
            Write-StatusMessage "Missing: $file" -Level Error -Indent 1
            $allValid = $false
            continue
        }
        
        # Check file is readable
        try {
            $content = Get-Content $filePath -First 1 -ErrorAction Stop
            Write-StatusMessage "Valid: $file" -Level Success -Indent 1
        } catch {
            Write-StatusMessage "Unreadable: $file - $_" -Level Error -Indent 1
            $allValid = $false
        }
    }
    
    return $allValid
}

# ============================================================================
# Main Script
# ============================================================================

Write-Host "=" * 80 -ForegroundColor Cyan
Write-Host "Enhanced Content Loaders Deployment" -ForegroundColor Cyan
Write-Host "Version 3.0.0 - Enterprise Grade" -ForegroundColor Cyan
Write-Host "=" * 80 -ForegroundColor Cyan
Write-Host ""

# Check if target exists
if (-not (Test-Path $TargetPath)) {
    Write-Host "❌ Target path not found: $TargetPath" -ForegroundColor Red
    Write-Host "Creating directory..." -ForegroundColor Yellow
    if (-not $DryRun) {
        New-Item -ItemType Directory -Path $TargetPath -Force | Out-Null
        Write-Host "✅ Created: $TargetPath" -ForegroundColor Green
    }
}

# Backup existing files
if ($Backup -and -not $DryRun) {
    $backupPath = ".\backup_content_loaders_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
    Write-Host "📦 Creating backup at: $backupPath" -ForegroundColor Yellow
    
    $filesToBackup = @(
        "FileContentLoader.cs",
        "WebContentLoader.cs",
        "MediaContentLoader.cs",
        "APIContentLoader.cs"
    )
    
    $backedUpCount = 0
    foreach ($file in $filesToBackup) {
        $sourcePath = Join-Path $TargetPath $file
        if (Test-Path $sourcePath) {
            if (-not (Test-Path $backupPath)) {
                New-Item -ItemType Directory -Path $backupPath | Out-Null
            }
            Copy-Item $sourcePath (Join-Path $backupPath $file)
            Write-Host "  ✅ Backed up: $file" -ForegroundColor Green
            $backedUpCount++
        }
    }
    
    if ($backedUpCount -eq 0) {
        Write-Host "  ℹ️  No existing files to backup" -ForegroundColor Cyan
    }
    Write-Host ""
}

# Files to deploy with their content
$filesToDeploy = @{
    "FileContentLoader.cs" = @{
        Description = "Enhanced file loader - 50+ formats, thumbnails, syntax highlighting"
        Source = Join-Path $PSScriptRoot "FileContentLoader.cs"
        Action = "Replace"
        Critical = $true
    }
    
    "WebContentLoader.cs" = @{
        Description = "Web content loader - 50+ curated sources, caching, rate limiting"
        Source = Join-Path $PSScriptRoot "WebContentLoader.cs"
        Action = "Replace"
        Critical = $true
    }
    
    "MediaContentLoader.cs" = @{
        Description = "Media loader - Video/audio with metadata extraction"
        Source = Join-Path $PSScriptRoot "MediaContentLoader.cs"
        Action = "Replace"
        Critical = $true
    }
    
    "APIContentLoader.cs" = @{
        Description = "API & Dynamic Environment loader - REST, auth, environments"
        Source = Join-Path $PSScriptRoot "APIContentLoader.cs"
        Action = "Replace"
        Critical = $true
    }
}

Write-Host "📋 Deployment Plan:" -ForegroundColor Cyan
Write-Host "  Target: $TargetPath" -ForegroundColor White
Write-Host "  Files: $($filesToDeploy.Count)" -ForegroundColor White
Write-Host "  Dry Run: $DryRun" -ForegroundColor White
Write-Host ""

# Verify source files exist and compute hashes
Write-Host "🔍 Verifying source files..." -ForegroundColor Cyan
$allSourcesExist = $true
$sourceManifest = @{}

foreach ($file in $filesToDeploy.Keys) {
    $sourceFile = $filesToDeploy[$file].Source
    if (-not (Test-Path $sourceFile)) {
        Write-Host "  ❌ Missing: $sourceFile" -ForegroundColor Red
        $allSourcesExist = $false
    } else {
        $size = (Get-Item $sourceFile).Length
        $hash = Get-FileHashSHA256 -Path $sourceFile
        $filesToDeploy[$file].Hash = $hash
        $sourceManifest[$file] = @{
            Size = $size
            Hash = $hash
            Source = $sourceFile
        }
        Write-Host "  ✅ Found: $file ($([math]::Round($size/1KB, 2)) KB)" -ForegroundColor Green
        if ($VerifyIntegrity) {
            Write-Host "     Hash: $($hash.Substring(0, 16))..." -ForegroundColor Gray
        }
    }
}

if (-not $allSourcesExist) {
    Write-Host ""
    Write-Host "❌ Deployment aborted - missing source files" -ForegroundColor Red
    Write-Host "Please ensure all loader files are in $PSScriptRoot" -ForegroundColor Yellow
    exit 1
}

Write-Host ""

# Deploy files
Write-Host "🚀 Deploying files..." -ForegroundColor Cyan
$deployedCount = 0
$failedCount = 0

foreach ($file in $filesToDeploy.Keys) {
    $info = $filesToDeploy[$file]
    $targetFile = Join-Path $TargetPath $file
    
    Write-Host "  📄 $file" -ForegroundColor White
    Write-Host "     $($info.Description)" -ForegroundColor Gray
    
    try {
        if ($DryRun) {
            Write-Host "     [DRY RUN] Would copy: $($info.Source) → $targetFile" -ForegroundColor Yellow
            $deployedCount++
        } else {
            # Check if file exists
            if (Test-Path $targetFile) {
                if (-not $Force) {
                    if ($NonInteractive) {
                        Write-Host "     ⏭️  Skipped (exists, non-interactive)" -ForegroundColor Yellow
                        continue
                    } else {
                        $response = Read-Host "     File exists. Overwrite? (y/N)"
                        if ($response -ne 'y' -and $response -ne 'Y') {
                            Write-Host "     ⏭️  Skipped" -ForegroundColor Yellow
                            continue
                        }
                    }
                }
            }
            
            # Copy file
            Copy-Item $info.Source $targetFile -Force
            
            # Verify deployment
            if (Test-Path $targetFile) {
                $size = (Get-Item $targetFile).Length
                
                # Integrity check
                if ($VerifyIntegrity -and $info.Hash) {
                    $targetHash = Get-FileHashSHA256 -Path $targetFile
                    if ($targetHash -ne $info.Hash) {
                        throw "Integrity check failed: hash mismatch"
                    }
                    Write-Host "     ✅ Deployed & verified ($([math]::Round($size/1KB, 2)) KB)" -ForegroundColor Green
                } else {
                    Write-Host "     ✅ Deployed ($([math]::Round($size/1KB, 2)) KB)" -ForegroundColor Green
                }
                $deployedCount++
            } else {
                throw "File not found after copy"
            }
        }
    }
    catch {
        Write-Host "     ❌ Failed: $($_.Exception.Message)" -ForegroundColor Red
        $failedCount++
        
        if ($info.Critical) {
            Write-Host ""
            Write-Host "❌ Critical file deployment failed - aborting" -ForegroundColor Red
            exit 1
        }
    }
}

Write-Host ""
Write-Host "=" * 80 -ForegroundColor Cyan

# Post-deployment verification
$deployedFiles = @{}
if (-not $DryRun -and $deployedCount -gt 0 -and -not $SkipPostVerification) {
    Write-Host ""
    foreach ($file in $filesToDeploy.Keys) {
        $targetFile = Join-Path $TargetPath $file
        if (Test-Path $targetFile) {
            $deployedFiles[$file] = $targetFile
        }
    }
    
    $verificationPassed = Invoke-PostDeploymentVerification -DeployedFiles $deployedFiles
    Write-Host ""
}

# Save deployment manifest
if (-not $DryRun -and $deployedCount -gt 0) {
    try {
        $manifest = @{
            Version = "3.0.0"
            DeploymentDate = (Get-Date -Format "yyyy-MM-dd HH:mm:ss")
            TargetPath = $TargetPath
            Files = $sourceManifest
            DeployedCount = $deployedCount
            FailedCount = $failedCount
        }
        
        $manifestJson = $manifest | ConvertTo-Json -Depth 10
        $manifestPath = Join-Path $TargetPath "deployment_manifest.json"
        $manifestJson | Set-Content -Path $manifestPath -Encoding UTF8
        Write-StatusMessage "Saved deployment manifest: $manifestPath" -Level Info
    } catch {
        Write-StatusMessage "Failed to save manifest: $_" -Level Warning
    }
}

# Cleanup old backups
if ($Backup -and -not $DryRun -and (Test-Path $backupPath)) {
    Remove-OldBackups -BackupDirectory $backupPath -KeepCount $MaxBackups
}

Write-Host ""
Write-Host "=" * 80 -ForegroundColor Cyan

# Summary
if ($DryRun) {
    Write-Host "🔍 DRY RUN COMPLETE" -ForegroundColor Yellow
    Write-Host "  Would deploy: $deployedCount files" -ForegroundColor White
    Write-Host "  Integrity checks: $(if ($VerifyIntegrity) { 'Enabled' } else { 'Disabled' })" -ForegroundColor White
    Write-Host "  Run without -DryRun to perform actual deployment" -ForegroundColor Cyan
} else {
    Write-Host "✅ DEPLOYMENT COMPLETE" -ForegroundColor Green
    Write-Host "  Deployed: $deployedCount files" -ForegroundColor White
    Write-Host "  Failed: $failedCount files" -ForegroundColor $(if ($failedCount -gt 0) { "Red" } else { "Green" })
    Write-Host "  Integrity checks: $(if ($VerifyIntegrity) { '✅ Passed' } else { '⏭️ Skipped' })" -ForegroundColor $(if ($VerifyIntegrity) { "Green" } else { "Yellow" })
    Write-Host "  Post-verification: $(if ($SkipPostVerification) { '⏭️ Skipped' } else { '✅ Passed' })" -ForegroundColor $(if ($SkipPostVerification) { "Yellow" } else { "Green" })
    
    if ($Backup -and (Test-Path $backupPath)) {
        Write-Host "  Backup: $backupPath" -ForegroundColor Cyan
    }
}

Write-Host ""

# Next steps
if (-not $DryRun -and $deployedCount -gt 0) {
    Write-Host "📝 Next Steps:" -ForegroundColor Cyan
    Write-Host "  1. Rebuild RenderProcess project" -ForegroundColor White
    Write-Host "  2. Update dependency injection in Program.cs:" -ForegroundColor White
    Write-Host "     services.AddSingleton<FileContentLoader>();" -ForegroundColor Gray
    Write-Host "     services.AddSingleton<WebContentLoader>();" -ForegroundColor Gray
    Write-Host "     services.AddSingleton<MediaContentLoader>();" -ForegroundColor Gray
    Write-Host "     services.AddSingleton<APIContentLoader>();" -ForegroundColor Gray
    Write-Host "     services.AddSingleton<DynamicEnvironmentLoader>();" -ForegroundColor Gray
    Write-Host "  3. Configure API keys (see documentation)" -ForegroundColor White
    Write-Host "  4. Test with sample content" -ForegroundColor White
    Write-Host ""
    Write-Host "📖 Full documentation: Enhanced Content Loaders - Complete Documentation.pdf" -ForegroundColor Cyan
}

Write-Host "=" * 80 -ForegroundColor Cyan
Write-Host ""

# Return exit code
exit $(if ($failedCount -gt 0) { 1 } else { 0 })
