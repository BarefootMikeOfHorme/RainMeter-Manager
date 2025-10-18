# ============================================================================
# Content Loaders Configuration Script
# RainmeterManager - Secure API Key & Settings Management
# Version: 1.0.0
# ============================================================================

param(
    [string]$ConfigPath = "$PSScriptRoot\..\..\RenderProcess\appsettings.json",
    [switch]$Interactive = $true,
    [switch]$GenerateTemplate = $false,
    [switch]$ValidateOnly = $false
)

$ErrorActionPreference = "Stop"

Write-Host "=" * 80 -ForegroundColor Cyan
Write-Host "Content Loaders Configuration Manager" -ForegroundColor Cyan
Write-Host "=" * 80 -ForegroundColor Cyan
Write-Host ""

# ============================================================================
# Helper Functions
# ============================================================================

function New-ConfigurationTemplate {
    return @{
        ContentLoaders = @{
            FileContentLoader = @{
                MaxFileBytes = 67108864  # 64 MB
                MaxPreviewLines = 1000
                ThumbnailMaxSize = 256
                EnableMetadataExtraction = $true
                EnableThumbnails = $true
                EnableSyntaxHighlighting = $true
                EnableContentAnalysis = $true
            }
            
            WebContentLoader = @{
                UserAgent = "RainmeterManager/1.0 (Content Loader)"
                TimeoutSeconds = 30
                MaxRetries = 3
                EnableCaching = $true
                CacheDurationMinutes = 60
                RateLimitRequestsPerMinute = 60
                MaxContentSizeBytes = 10485760  # 10 MB
                
                ApiKeys = @{
                    OpenWeatherMap = "<YOUR_API_KEY_HERE>"
                    NewsAPI = "<YOUR_API_KEY_HERE>"
                    GitHub = "<YOUR_API_KEY_HERE>"
                    Twitter = "<YOUR_API_KEY_HERE>"
                    Reddit = "<YOUR_API_KEY_HERE>"
                }
            }
            
            MediaContentLoader = @{
                MaxVideoSizeBytes = 524288000  # 500 MB
                MaxAudioSizeBytes = 104857600  # 100 MB
                EnableMetadataExtraction = $true
                EnableThumbnailGeneration = $true
                ThumbnailTimeOffset = 5.0
                SupportedVideoFormats = @("mp4", "avi", "mkv", "mov", "webm")
                SupportedAudioFormats = @("mp3", "wav", "flac", "ogg", "m4a")
            }
            
            APIContentLoader = @{
                TimeoutSeconds = 30
                MaxRetries = 3
                EnableResponseCaching = $true
                CacheDurationMinutes = 15
                
                Authentication = @{
                    DefaultScheme = "Bearer"
                    SecureStorePath = "%LOCALAPPDATA%\\RainmeterManager\\api_credentials"
                }
                
                DynamicEnvironments = @{
                    Development = @{
                        BaseUrl = "http://localhost:5000"
                        EnableDebugLogging = $true
                    }
                    Production = @{
                        BaseUrl = "https://api.production.com"
                        EnableDebugLogging = $false
                    }
                }
            }
        }
        
        Logging = @{
            LogLevel = @{
                Default = "Information"
                "RenderProcess.Content" = "Debug"
            }
        }
    }
}

function Test-ConfigurationValid {
    param([hashtable]$Config)
    
    $errors = @()
    
    # Check for placeholder API keys
    if ($Config.ContentLoaders.WebContentLoader.ApiKeys) {
        foreach ($key in $Config.ContentLoaders.WebContentLoader.ApiKeys.Keys) {
            $value = $Config.ContentLoaders.WebContentLoader.ApiKeys[$key]
            if ($value -match "YOUR_API_KEY|PLACEHOLDER|XXX") {
                $errors += "WebContentLoader: $key still has placeholder value"
            }
        }
    }
    
    # Validate numeric limits
    $numericSettings = @(
        @{ Path = "FileContentLoader.MaxFileBytes"; Min = 1024; Max = 1GB }
        @{ Path = "WebContentLoader.TimeoutSeconds"; Min = 5; Max = 300 }
        @{ Path = "MediaContentLoader.MaxVideoSizeBytes"; Min = 1MB; Max = 5GB }
    )
    
    foreach ($setting in $numericSettings) {
        $pathParts = $setting.Path -split '\.'
        $value = $Config.ContentLoaders
        foreach ($part in $pathParts) {
            if ($value.ContainsKey($part)) {
                $value = $value[$part]
            } else {
                break
            }
        }
        
        if ($value -is [int64] -or $value -is [int]) {
            if ($value -lt $setting.Min -or $value -gt $setting.Max) {
                $errors += "$($setting.Path) out of range (${$setting.Min}-${$setting.Max})"
            }
        }
    }
    
    return $errors
}

function Set-ApiKey {
    param(
        [string]$Service,
        [string]$Key,
        [hashtable]$Config
    )
    
    if (-not $Config.ContentLoaders.WebContentLoader.ApiKeys) {
        $Config.ContentLoaders.WebContentLoader.ApiKeys = @{}
    }
    
    # Don't store in plain text - this is a template reminder
    Write-Host "⚠️  Note: Consider using secure credential storage for production" -ForegroundColor Yellow
    $Config.ContentLoaders.WebContentLoader.ApiKeys[$Service] = $Key
}

# ============================================================================
# Main Script
# ============================================================================

# Generate template if requested
if ($GenerateTemplate) {
    Write-Host "📝 Generating configuration template..." -ForegroundColor Cyan
    
    $template = New-ConfigurationTemplate
    $templateJson = $template | ConvertTo-Json -Depth 10
    $templatePath = Join-Path (Split-Path $ConfigPath) "appsettings.template.json"
    
    $templateJson | Set-Content -Path $templatePath -Encoding UTF8
    
    Write-Host "  ✅ Template saved: $templatePath" -ForegroundColor Green
    Write-Host ""
    Write-Host "📋 Next steps:" -ForegroundColor Cyan
    Write-Host "  1. Copy template to appsettings.json" -ForegroundColor White
    Write-Host "  2. Replace API key placeholders" -ForegroundColor White
    Write-Host "  3. Run with -ValidateOnly to check configuration" -ForegroundColor White
    Write-Host ""
    exit 0
}

# Check if config exists
if (-not (Test-Path $ConfigPath)) {
    Write-Host "❌ Configuration file not found: $ConfigPath" -ForegroundColor Red
    Write-Host ""
    Write-Host "Run with -GenerateTemplate to create a template:" -ForegroundColor Yellow
    Write-Host "  .\configure_loaders.ps1 -GenerateTemplate" -ForegroundColor Cyan
    Write-Host ""
    exit 1
}

# Load existing configuration
Write-Host "📖 Loading configuration from: $ConfigPath" -ForegroundColor Cyan
try {
    $configJson = Get-Content $ConfigPath -Raw
    $config = $configJson | ConvertFrom-Json -AsHashtable
    Write-Host "  ✅ Loaded successfully" -ForegroundColor Green
} catch {
    Write-Host "  ❌ Failed to parse configuration: $_" -ForegroundColor Red
    exit 1
}

Write-Host ""

# Validate configuration
Write-Host "🔍 Validating configuration..." -ForegroundColor Cyan
$validationErrors = Test-ConfigurationValid -Config $config

if ($validationErrors.Count -eq 0) {
    Write-Host "  ✅ Configuration is valid" -ForegroundColor Green
} else {
    Write-Host "  ⚠️  Found $($validationErrors.Count) issue(s):" -ForegroundColor Yellow
    foreach ($error in $validationErrors) {
        Write-Host "     • $error" -ForegroundColor Yellow
    }
}

Write-Host ""

if ($ValidateOnly) {
    Write-Host "=" * 80 -ForegroundColor Cyan
    exit $(if ($validationErrors.Count -eq 0) { 0 } else { 1 })
}

# Interactive configuration
if ($Interactive -and $validationErrors.Count -gt 0) {
    Write-Host "🔧 Interactive Configuration Mode" -ForegroundColor Cyan
    Write-Host ""
    
    $response = Read-Host "Would you like to configure API keys now? (y/N)"
    if ($response -eq 'y' -or $response -eq 'Y') {
        
        $apiServices = @("OpenWeatherMap", "NewsAPI", "GitHub", "Twitter", "Reddit")
        
        foreach ($service in $apiServices) {
            $currentKey = $config.ContentLoaders.WebContentLoader.ApiKeys[$service]
            
            if ($currentKey -match "YOUR_API_KEY|PLACEHOLDER") {
                Write-Host ""
                Write-Host "Service: $service" -ForegroundColor White
                Write-Host "Current: $currentKey" -ForegroundColor Gray
                
                $newKey = Read-Host "Enter API key (or press Enter to skip)"
                
                if ($newKey -and $newKey.Trim() -ne "") {
                    Set-ApiKey -Service $service -Key $newKey.Trim() -Config $config
                    Write-Host "  ✅ Updated" -ForegroundColor Green
                }
            }
        }
        
        Write-Host ""
        Write-Host "💾 Saving updated configuration..." -ForegroundColor Cyan
        
        try {
            # Backup existing config
            $backupPath = "$ConfigPath.backup_$(Get-Date -Format 'yyyyMMddHHmmss')"
            Copy-Item $ConfigPath $backupPath
            Write-Host "  📦 Backup: $backupPath" -ForegroundColor Gray
            
            # Save new config
            $updatedJson = $config | ConvertTo-Json -Depth 10
            $updatedJson | Set-Content -Path $ConfigPath -Encoding UTF8
            
            Write-Host "  ✅ Configuration saved" -ForegroundColor Green
        } catch {
            Write-Host "  ❌ Failed to save: $_" -ForegroundColor Red
            exit 1
        }
    }
}

Write-Host ""
Write-Host "=" * 80 -ForegroundColor Cyan
Write-Host "📋 Configuration Summary" -ForegroundColor White
Write-Host "=" * 80 -ForegroundColor Cyan

# Display key settings
Write-Host "FileContentLoader:" -ForegroundColor Cyan
Write-Host "  Max file size: $([math]::Round($config.ContentLoaders.FileContentLoader.MaxFileBytes / 1MB, 2)) MB" -ForegroundColor White
Write-Host "  Thumbnails: $($config.ContentLoaders.FileContentLoader.EnableThumbnails)" -ForegroundColor White

Write-Host ""
Write-Host "WebContentLoader:" -ForegroundColor Cyan
Write-Host "  Timeout: $($config.ContentLoaders.WebContentLoader.TimeoutSeconds)s" -ForegroundColor White
Write-Host "  Caching: $($config.ContentLoaders.WebContentLoader.EnableCaching)" -ForegroundColor White
$configuredKeys = ($config.ContentLoaders.WebContentLoader.ApiKeys.Values | Where-Object { $_ -notmatch "YOUR_API_KEY|PLACEHOLDER" }).Count
$totalKeys = $config.ContentLoaders.WebContentLoader.ApiKeys.Count
Write-Host "  API Keys: $configuredKeys/$totalKeys configured" -ForegroundColor $(if ($configuredKeys -eq $totalKeys) { "Green" } else { "Yellow" })

Write-Host ""
Write-Host "MediaContentLoader:" -ForegroundColor Cyan
Write-Host "  Max video size: $([math]::Round($config.ContentLoaders.MediaContentLoader.MaxVideoSizeBytes / 1MB, 2)) MB" -ForegroundColor White
Write-Host "  Metadata extraction: $($config.ContentLoaders.MediaContentLoader.EnableMetadataExtraction)" -ForegroundColor White

Write-Host ""
Write-Host "=" * 80 -ForegroundColor Cyan
Write-Host ""

exit 0
