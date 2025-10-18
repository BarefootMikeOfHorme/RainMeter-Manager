Param(
  [ValidateSet("x64","x86")] [string]$Arch = "x64",
  [ValidateSet("Release","Debug")] [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

$rid = if ($Arch -eq "x64") { "win-x64" } else { "win-x86" }
$proj = Join-Path $PSScriptRoot "..\..\renderprocess\RenderProcess.csproj"
$out = Join-Path $PSScriptRoot "..\..\artifacts\render\$Arch\publish"

Write-Host "Publishing RenderProcess ($Arch, $Configuration) to $out" -ForegroundColor Cyan

New-Item -ItemType Directory -Force -Path $out | Out-Null

dotnet publish $proj -c $Configuration -r $rid --self-contained true -p:PublishSingleFile=false -o $out

Write-Host "Publish completed." -ForegroundColor Green
