# ============================================================================
# API Key Setup Wizard
# RainmeterManager - Interactive API Registration & Configuration
# Version: 1.0.0
# ============================================================================

param(
    [switch]$AutoOpenBrowsers = $true,
    [switch]$SetPermanent = $false,
    [switch]$TestKeys = $true
)

$ErrorActionPreference = "Continue"

Write-Host "=" * 80 -ForegroundColor Cyan
Write-Host "API Key Setup Wizard" -ForegroundColor Cyan
Write-Host "Interactive Registration & Configuration" -ForegroundColor Cyan
Write-Host "=" * 80 -ForegroundColor Cyan
Write-Host ""

# ============================================================================
# API Service Definitions
# ============================================================================

$services = @(
    @{
        Name = "NASA API"
        EnvVar = "NASA_API_KEY"
        SignupUrl = "https://api.nasa.gov/"
        TestUrl = "https://api.nasa.gov/planetary/apod?api_key={{KEY}}"
        Priority = 1
        Instant = $true
        Free = $true
        Description = "7 space sources: APOD, Mars rovers, Earth from space"
        Instructions = @"
1. Click 'Generate API Key'
2. Enter First Name, Last Name, Email
3. Check email (arrives instantly)
4. Copy the API key from email
"@
    },
    @{
        Name = "WAQI Air Quality"
        EnvVar = "WAQI_TOKEN"
        SignupUrl = "https://aqicn.org/data-platform/token/"
        TestUrl = "https://api.waqi.info/feed/washington/?token={{KEY}}"
        Priority = 2
        Instant = $true
        Free = $true
        Description = "Real-time air quality data worldwide"
        Instructions = @"
1. Enter your email address
2. Click 'Submit'
3. Check email (arrives in seconds)
4. Copy token from email
"@
    },
    @{
        Name = "WeatherAPI.com"
        EnvVar = "WEATHER_API_KEY"
        SignupUrl = "https://www.weatherapi.com/signup.aspx"
        TestUrl = "https://api.weatherapi.com/v1/current.json?key={{KEY}}&q=London"
        Priority = 3
        Instant = $true
        Free = $true
        Description = "Real-time weather forecasts"
        Instructions = @"
1. Fill registration form
2. Verify email (instant link)
3. Login to dashboard
4. Copy API key from dashboard
"@
    },
    @{
        Name = "OpenWeatherMap"
        EnvVar = "OPENWEATHER_API_KEY"
        SignupUrl = "https://home.openweathermap.org/users/sign_up"
        TestUrl = "https://api.openweathermap.org/data/2.5/weather?q=London&appid={{KEY}}&units=metric"
        Priority = 4
        Instant = $false
        ActivationWait = "10 minutes - 2 hours"
        Free = $true
        Description = "Standard weather provider"
        Instructions = @"
1. Create account with email
2. Verify email
3. Go to https://home.openweathermap.org/api_keys
4. Copy 'Default' API key
⚠️  WAIT 10 minutes - 2 hours for activation!
"@
    },
    @{
        Name = "Rijksmuseum API"
        EnvVar = "RIJKSMUSEUM_API_KEY"
        SignupUrl = "https://data.rijksmuseum.nl/object-metadata/api/"
        TestUrl = $null
        Priority = 5
        Instant = $false
        ActivationWait = "24-48 hours"
        Free = $true
        Description = "Dutch art collection (optional)"
        Instructions = @"
1. Click 'Request API key'
2. Fill application form
3. Check email for API key
⚠️  May take 24-48 hours
"@
    }
)

# ============================================================================
# Helper Functions
# ============================================================================

function Write-Step {
    param([string]$Message, [string]$Icon = "📍")
    Write-Host "$Icon $Message" -ForegroundColor White
}

function Write-Success {
    param([string]$Message)
    Write-Host "✅ $Message" -ForegroundColor Green
}

function Write-Warning {
    param([string]$Message)
    Write-Host "⚠️  $Message" -ForegroundColor Yellow
}

function Write-Info {
    param([string]$Message)
    Write-Host "ℹ️  $Message" -ForegroundColor Cyan
}

function Open-Browser {
    param([string]$Url)
    try {
        Start-Process $Url
        return $true
    } catch {
        Write-Warning "Failed to open browser automatically. Please visit: $Url"
        return $false
    }
}

function Test-ApiKey {
    param(
        [string]$ServiceName,
        [string]$TestUrl,
        [string]$Key
    )
    
    if ([string]::IsNullOrWhiteSpace($TestUrl)) {
        Write-Info "No test endpoint available for $ServiceName"
        return $true
    }
    
    $testUrl = $TestUrl.Replace("{{KEY}}", $Key)
    
    try {
        Write-Info "Testing $ServiceName..."
        $response = Invoke-WebRequest -Uri $testUrl -TimeoutSec 10 -UseBasicParsing
        
        if ($response.StatusCode -eq 200) {
            Write-Success "$ServiceName key is valid!"
            return $true
        } else {
            Write-Warning "$ServiceName returned status code: $($response.StatusCode)"
            return $false
        }
    } catch {
        $errorMsg = $_.Exception.Message
        if ($errorMsg -like "*401*" -or $errorMsg -like "*403*") {
            Write-Host "❌ $ServiceName key is INVALID or not activated yet" -ForegroundColor Red
        } elseif ($errorMsg -like "*timeout*") {
            Write-Warning "$ServiceName test timed out (might still be valid)"
        } else {
            Write-Warning "$ServiceName test failed: $errorMsg"
        }
        return $false
    }
}

function Set-EnvironmentVariablePermanent {
    param(
        [string]$Name,
        [string]$Value
    )
    
    try {
        [System.Environment]::SetEnvironmentVariable($Name, $Value, 'User')
        Write-Success "Set $Name permanently (restart terminal to apply)"
        return $true
    } catch {
        Write-Warning "Failed to set permanent variable: $_"
        return $false
    }
}

# ============================================================================
# Main Wizard
# ============================================================================

Write-Host "This wizard will help you register for and configure API keys." -ForegroundColor White
Write-Host "Total time required: ~10 minutes for all keys" -ForegroundColor Gray
Write-Host ""

$response = Read-Host "Ready to start? (y/N)"
if ($response -ne 'y' -and $response -ne 'Y') {
    Write-Host "Setup cancelled." -ForegroundColor Yellow
    exit 0
}

Write-Host ""
Write-Host "=" * 80 -ForegroundColor Cyan

$configuredKeys = @{}
$skippedServices = @()

foreach ($service in ($services | Sort-Object Priority)) {
    Write-Host ""
    Write-Host "━" * 80 -ForegroundColor DarkCyan
    Write-Host "📦 $($service.Name)" -ForegroundColor Cyan
    Write-Host "━" * 80 -ForegroundColor DarkCyan
    Write-Host ""
    
    Write-Info $service.Description
    Write-Host ""
    
    # Check if already configured
    $existing = [System.Environment]::GetEnvironmentVariable($service.EnvVar)
    if ($existing) {
        Write-Success "Already configured: $($service.EnvVar)"
        Write-Host "Current value: $($existing.Substring(0, [Math]::Min(16, $existing.Length)))..." -ForegroundColor Gray
        Write-Host ""
        
        $reconfigure = Read-Host "Reconfigure this key? (y/N)"
        if ($reconfigure -ne 'y' -and $reconfigure -ne 'Y') {
            $configuredKeys[$service.EnvVar] = $existing
            continue
        }
    }
    
    Write-Host "Priority: " -NoNewline
    if ($service.Priority -le 2) {
        Write-Host "⭐⭐⭐ ESSENTIAL" -ForegroundColor Green
    } elseif ($service.Priority -le 4) {
        Write-Host "⭐⭐ RECOMMENDED" -ForegroundColor Yellow
    } else {
        Write-Host "⭐ OPTIONAL" -ForegroundColor Gray
    }
    
    Write-Host "Activation: " -NoNewline
    if ($service.Instant) {
        Write-Host "✅ INSTANT" -ForegroundColor Green
    } else {
        Write-Host "⏳ $($service.ActivationWait)" -ForegroundColor Yellow
    }
    
    Write-Host ""
    Write-Host "Instructions:" -ForegroundColor White
    Write-Host $service.Instructions -ForegroundColor Gray
    Write-Host ""
    
    $response = Read-Host "Setup this API key now? (Y/n/skip)"
    
    if ($response -eq 'skip') {
        Write-Info "Skipping all remaining services..."
        break
    }
    
    if ($response -eq 'n' -or $response -eq 'N') {
        Write-Info "Skipped $($service.Name)"
        $skippedServices += $service.Name
        continue
    }
    
    # Open browser
    if ($AutoOpenBrowsers) {
        Write-Step "Opening $($service.SignupUrl) in browser..."
        Open-Browser -Url $service.SignupUrl | Out-Null
        Start-Sleep -Seconds 2
    } else {
        Write-Info "Visit: $($service.SignupUrl)"
    }
    
    Write-Host ""
    Write-Host "⌛ Complete registration and get your API key..." -ForegroundColor Yellow
    Write-Host "Press Enter when you have your API key" -ForegroundColor White
    $null = Read-Host
    
    # Get API key from user
    Write-Host ""
    $apiKey = Read-Host "Paste your $($service.Name) API key"
    
    if ([string]::IsNullOrWhiteSpace($apiKey)) {
        Write-Warning "No key entered, skipping"
        $skippedServices += $service.Name
        continue
    }
    
    $apiKey = $apiKey.Trim()
    
    # Test the key
    if ($TestKeys -and $service.TestUrl) {
        Write-Host ""
        $testResult = Test-ApiKey -ServiceName $service.Name -TestUrl $service.TestUrl -Key $apiKey
        
        if (-not $testResult) {
            Write-Host ""
            $continue = Read-Host "Key test failed. Save anyway? (y/N)"
            if ($continue -ne 'y' -and $continue -ne 'Y') {
                Write-Info "Skipped $($service.Name)"
                $skippedServices += $service.Name
                continue
            }
        }
    }
    
    # Set environment variable
    Write-Host ""
    Write-Step "Configuring environment variable..."
    
    # Session variable (always set)
    Set-Item -Path "Env:$($service.EnvVar)" -Value $apiKey
    Write-Success "Set for current session: $($service.EnvVar)"
    
    # Permanent variable (if requested)
    if ($SetPermanent) {
        Set-EnvironmentVariablePermanent -Name $service.EnvVar -Value $apiKey
    }
    
    $configuredKeys[$service.EnvVar] = $apiKey
    Write-Success "✅ $($service.Name) configured!"
}

# ============================================================================
# Summary
# ============================================================================

Write-Host ""
Write-Host "=" * 80 -ForegroundColor Cyan
Write-Host "SETUP COMPLETE" -ForegroundColor Green
Write-Host "=" * 80 -ForegroundColor Cyan
Write-Host ""

Write-Host "📊 Summary:" -ForegroundColor Cyan
Write-Host "  ✅ Configured: $($configuredKeys.Count)/$($services.Count) services" -ForegroundColor Green
Write-Host "  ⏭️  Skipped: $($skippedServices.Count) services" -ForegroundColor Yellow

if ($configuredKeys.Count -gt 0) {
    Write-Host ""
    Write-Host "🔑 Configured Keys:" -ForegroundColor Cyan
    foreach ($key in $configuredKeys.Keys) {
        $value = $configuredKeys[$key]
        $preview = $value.Substring(0, [Math]::Min(16, $value.Length)) + "..."
        Write-Host "  • $key = $preview" -ForegroundColor White
    }
}

if ($skippedServices.Count -gt 0) {
    Write-Host ""
    Write-Host "⏭️  Skipped Services:" -ForegroundColor Yellow
    foreach ($service in $skippedServices) {
        Write-Host "  • $service" -ForegroundColor Gray
    }
}

Write-Host ""

if (-not $SetPermanent -and $configuredKeys.Count -gt 0) {
    Write-Warning "Keys are set for THIS SESSION ONLY"
    Write-Host ""
    Write-Host "To make permanent, re-run with -SetPermanent:" -ForegroundColor White
    Write-Host "  .\setup_api_keys.ps1 -SetPermanent" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Or manually set permanent variables:" -ForegroundColor White
    foreach ($key in $configuredKeys.Keys) {
        Write-Host "  [System.Environment]::SetEnvironmentVariable('$key', 'YOUR_KEY', 'User')" -ForegroundColor Gray
    }
}

Write-Host ""
Write-Host "=" * 80 -ForegroundColor Cyan
Write-Host ""

# ============================================================================
# Next Steps
# ============================================================================

Write-Host "📝 Next Steps:" -ForegroundColor Cyan
Write-Host ""

if ($services.Where({-not $_.Instant -and $configuredKeys.ContainsKey($_.EnvVar)})) {
    Write-Warning "Some keys require activation time:"
    foreach ($service in $services.Where({-not $_.Instant -and $configuredKeys.ContainsKey($_.EnvVar)})) {
        Write-Host "  • $($service.Name): $($service.ActivationWait)" -ForegroundColor Yellow
    }
    Write-Host ""
}

Write-Host "1. ✅ Verify loaders are deployed:" -ForegroundColor White
Write-Host "   .\verify_loaders.ps1" -ForegroundColor Cyan
Write-Host ""

Write-Host "2. ✅ Test your API keys manually:" -ForegroundColor White
Write-Host "   Get-ChildItem Env: | Where-Object { `$_.Name -like '*API*' -or `$_.Name -like '*TOKEN*' }" -ForegroundColor Cyan
Write-Host ""

Write-Host "3. ✅ Start RenderProcess and test sources" -ForegroundColor White
Write-Host ""

Write-Host "4. ✅ Check API_KEY_GUIDE.md for detailed info:" -ForegroundColor White
Write-Host "   code API_KEY_GUIDE.md" -ForegroundColor Cyan
Write-Host ""

Write-Host "=" * 80 -ForegroundColor Cyan
Write-Host ""

# Show current environment variables
Write-Host "🔍 Current API Environment Variables:" -ForegroundColor Cyan
$apiVars = Get-ChildItem Env: | Where-Object { 
    $_.Name -like '*API*' -or 
    $_.Name -like '*TOKEN*' -or 
    $_.Name -like '*NASA*' -or 
    $_.Name -like '*WEATHER*' -or 
    $_.Name -like '*WAQI*' -or 
    $_.Name -like '*RIJKS*'
}

if ($apiVars) {
    foreach ($var in $apiVars) {
        $preview = if ($var.Value.Length -gt 20) {
            $var.Value.Substring(0, 20) + "..."
        } else {
            $var.Value
        }
        Write-Host "  $($var.Name) = $preview" -ForegroundColor Gray
    }
} else {
    Write-Host "  (none found)" -ForegroundColor Gray
}

Write-Host ""
Write-Host "✅ Setup wizard complete!" -ForegroundColor Green
Write-Host ""

exit 0
