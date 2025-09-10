param(
  [string]$Configuration = 'Release',
  [string]$Platform = 'x64'
)

$ErrorActionPreference = 'Continue'
$stamp = (Get-Date).ToUniversalTime().ToString('yyyyMMdd_HHmmssZ')
$root  = (Get-Location)
$destDir = Join-Path $root 'changes/build_reports'
if (-not (Test-Path -LiteralPath $destDir)) { New-Item -ItemType Directory -Force -Path $destDir | Out-Null }
$errFile = Join-Path $destDir ("BUILD_ERRORS_{0}.txt" -f $stamp)

# dotnet build (C#)
$dotnetOut = & dotnet build "$($root)\renderprocess\RenderProcess.csproj" -c $Configuration 2>&1 | Out-String

# MSBuild (C++) via vswhere
$vsw = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$msbuildOut = ''
if (Test-Path $vsw) {
  $inst = & $vsw -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
  if ($inst) {
    $msbuild = Join-Path $inst 'MSBuild/Current/Bin/MSBuild.exe'
    if (Test-Path $msbuild) {
      $msbuildOut = & $msbuild "$($root)\RainmeterManager.sln" /t:Build /p:Configuration=$Configuration /p:Platform=$Platform /m 2>&1 | Out-String
    } else {
      $msbuildOut = 'MSBuild.exe not found at expected path.'
    }
  } else {
    $msbuildOut = 'Visual Studio Build Tools not detected by vswhere.'
  }
} else {
  $msbuildOut = 'vswhere.exe not found; cannot locate MSBuild.'
}

# Extract errors/warnings
$dotnetLines   = $dotnetOut  -split "`r?`n"
$msbuildLines  = $msbuildOut -split "`r?`n"
$dotnetErrors  = $dotnetLines  | Where-Object { $_ -match '\berror\b' }
$dotnetWarns   = $dotnetLines  | Where-Object { $_ -match '\bwarning\b' }
$msbuildErrors = $msbuildLines | Where-Object { $_ -match '\berror\b' }
$msbuildWarns  = $msbuildLines | Where-Object { $_ -match '\bwarning\b' }

# Write report
$report = @()
$report += "Build Error Report"
$report += "Timestamp (UTC): $stamp"
$report += "Configuration: $Configuration | Platform: $Platform"
$report += ""
$report += "== Summary =="
$report += ("dotnet build errors:   {0}, warnings: {1}" -f ($dotnetErrors.Count), ($dotnetWarns.Count))
$report += ("MSBuild (C++) errors:  {0}, warnings: {1}" -f ($msbuildErrors.Count), ($msbuildWarns.Count))
$report += ""
$report += "== dotnet build (renderprocess/RenderProcess.csproj) errors =="
if ($dotnetErrors.Count) { $report += $dotnetErrors } else { $report += '(none)' }
$report += ""
$report += "== MSBuild (RainmeterManager.sln) errors =="
if ($msbuildErrors.Count) { $report += $msbuildErrors } else { $report += '(none or MSBuild not located)'; if (-not $msbuildOut) { $report += '(no output captured)' } else { $report += $msbuildOut } }
$report += ""
$report += "== Notes =="
$report += "- C++: If missing Microsoft.Cpp.Default.props, install Desktop development with C++ (VS 2022) or C++ Build Tools"
$report += "- C#: Fix duplicate type definitions and missing references (e.g., WebView2, System.Drawing)."

$reportText = $report -join "`r`n"
Set-Content -LiteralPath $errFile -Value $reportText -NoNewline -Encoding UTF8

Write-Output $errFile

