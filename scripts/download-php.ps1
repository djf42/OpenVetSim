# ============================================================================
# download-php.ps1
#
# Downloads a self-contained static PHP 8.x CLI binary for Windows and places
# it in OpenVetSim\build\bin\PHP8.0\ where the simulation engine looks first
# when searching for PHP (see WebSrv.cpp findPhpPath()).
#
# Uses pre-built static binaries from the static-php-cli project:
#   https://github.com/crazywhalecc/static-php-cli
#
# These binaries have no external library dependencies — they work on any
# Windows installation without any additional runtime or package manager.
#
# Usage (from the repo root in PowerShell):
#   Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
#   .\scripts\download-php.ps1
#
# Run this once before:
#   npm run dist:win   (packaging a distributable installer)
#   npm start          (development mode, if PHP is not installed system-wide)
# ============================================================================

$ErrorActionPreference = 'Stop'

$ScriptDir  = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot   = Split-Path -Parent $ScriptDir
$Dest       = Join-Path $RepoRoot 'OpenVetSim\build\bin\PHP8.0'

# PHP version to bundle — update this when a new release is needed.
$PhpVersion = '8.3.11'
$SpcRelease = '2.4.0'   # static-php-cli release tag

# Windows x64 asset name
$Asset = "php-$PhpVersion-cli-win-x64.zip"
$Url   = "https://github.com/crazywhalecc/static-php-cli/releases/download/$SpcRelease/$Asset"

Write-Host "Downloading PHP $PhpVersion for Windows x64..."
Write-Host "  Source : $Url"
Write-Host "  Dest   : $Dest"
Write-Host ""

# Create destination directory
New-Item -ItemType Directory -Force -Path $Dest | Out-Null

# Download to a temp file
$Tmp     = [System.IO.Path]::GetTempPath()
$TmpFile = Join-Path $Tmp $Asset

Write-Host "Fetching $Asset ..."
Invoke-WebRequest -Uri $Url -OutFile $TmpFile -UseBasicParsing

# Extract — the archive contains a single php.exe binary
Write-Host "Extracting..."
Expand-Archive -Path $TmpFile -DestinationPath $Dest -Force

# Clean up temp file
Remove-Item $TmpFile -Force

Write-Host ""
Write-Host "Done. PHP is ready at:"
Write-Host "  $Dest\php.exe"
Write-Host ""
Write-Host "You can verify it with:"
Write-Host "  & '$Dest\php.exe' --version"
