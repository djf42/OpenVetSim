# ============================================================================
# download-php.ps1
#
# Downloads a self-contained static PHP 8.x CLI binary for Windows and places
# it in OpenVetSim\build\bin\PHP8.0\ where the simulation engine looks first
# when searching for PHP (see WebSrv.cpp findPhpPath()).
#
# Uses pre-built static binaries from the static-php-cli project CDN:
#   https://dl.static-php.dev/static-php-cli/common/
#
# NOTE: The CDN redirects through DigitalOcean Spaces, which is blocked on
# some university/corporate networks. Run this script from a home network,
# phone hotspot, or VPN if you are on campus.
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

$PhpMinor  = '8.3'
$BaseUrl   = 'https://dl.static-php.dev/static-php-cli/common'
$Candidates = 17..0   # walk backwards from most recent patch

Write-Host "Looking for PHP $PhpMinor.x CLI binary for Windows x64..."
Write-Host ""

$FoundUrl     = $null
$FoundVersion = $null

foreach ($Patch in $Candidates) {
    $Version  = "$PhpMinor.$Patch"
    $Asset    = "php-$Version-cli-windows-x64.zip"
    $Url      = "$BaseUrl/$Asset"

    try {
        $response = Invoke-WebRequest -Uri $Url -Method Head -UseBasicParsing -ErrorAction Stop
        if ($response.StatusCode -eq 200) {
            $FoundUrl     = $Url
            $FoundVersion = $Version
            break
        }
    } catch { <# not found — try next patch #> }
}

if (-not $FoundUrl) {
    Write-Error "Could not find a PHP $PhpMinor.x binary for Windows x64.`nCheck https://dl.static-php.dev/static-php-cli/common/ for available versions."
    exit 1
}

$Asset = "php-$FoundVersion-cli-windows-x64.zip"
Write-Host "Downloading PHP $FoundVersion for Windows x64..."
Write-Host "  Source : $FoundUrl"
Write-Host "  Dest   : $Dest"
Write-Host ""

New-Item -ItemType Directory -Force -Path $Dest | Out-Null

$Tmp     = [System.IO.Path]::GetTempPath()
$TmpFile = Join-Path $Tmp $Asset

Write-Host "Fetching $Asset ..."
Invoke-WebRequest -Uri $FoundUrl -OutFile $TmpFile -UseBasicParsing

Write-Host "Extracting..."
Expand-Archive -Path $TmpFile -DestinationPath $Dest -Force

Remove-Item $TmpFile -Force

Write-Host ""
Write-Host "Done. PHP is ready at:"
Write-Host "  $Dest\php.exe"
Write-Host ""
Write-Host "Verify with:"
Write-Host "  & '$Dest\php.exe' --version"
