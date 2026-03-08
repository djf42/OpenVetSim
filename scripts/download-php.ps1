# ============================================================================
# download-php.ps1
#
# Downloads a PHP 8.x NTS CLI for Windows and places it in
# OpenVetSim\build\bin\PHP8.0\ where the simulation engine looks for PHP
# (see WebSrv.cpp findPhpPath()).
#
# Downloads from the official PHP for Windows release server:
#   https://windows.php.net/downloads/releases/
#
# The ZIP contains php.exe plus its required DLLs. The electron-builder
# config bundles the entire PHP8.0\ directory, so end users get everything
# they need inside the installer with no separate runtime to install.
#
# PHP version notes:
#   PHP 8.3.x  →  php-8.3.x-nts-Win32-vs16-x64.zip  (VS2019 runtime)
#   PHP 8.4.x  →  php-8.4.x-nts-Win32-vs17-x64.zip  (VS2022 runtime)
#
# If the download server is unreachable, supply a pre-downloaded zip:
#   .\scripts\download-php.ps1 -PhpZipPath "C:\Downloads\php-8.3.x-nts-Win32-vs16-x64.zip"
#
# Usage (from the repo root in PowerShell):
#   Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
#   .\scripts\download-php.ps1
#
# Run this once before:
#   npm run dist:win   (packaging a distributable installer)
#   npm start          (development mode, if PHP is not installed system-wide)
# ============================================================================

param(
    # Optional: path to a pre-downloaded PHP NTS x64 zip
    [string] $PhpZipPath = ''
)

$ErrorActionPreference = 'Stop'

$ScriptDir  = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot   = Split-Path -Parent $ScriptDir
$Dest       = Join-Path $RepoRoot 'OpenVetSim\build\bin\PHP8.0'

# ── Manual bypass: user supplied a pre-downloaded zip ─────────────────────────
if ($PhpZipPath -ne '') {
    if (-not (Test-Path $PhpZipPath)) {
        Write-Error "Supplied zip not found: $PhpZipPath"
        exit 1
    }
    Write-Host "Using pre-downloaded PHP zip: $PhpZipPath"
    New-Item -ItemType Directory -Force -Path $Dest | Out-Null
    Expand-Archive -Path $PhpZipPath -DestinationPath $Dest -Force
    Write-Host "Extracted to $Dest"
    & "$Dest\php.exe" --version
    exit 0
}

# ── Automatic download from windows.php.net ───────────────────────────────────
# We use curl.exe (built into Windows 10+) to probe for the newest available
# release. curl exits 0 regardless of HTTP status; we capture the code to
# distinguish "not found" (404) from "no network" (000).
#
# PHP 8.4 switched to the VS2022 runtime (vs17); PHP 8.3 and earlier use vs16.
$BaseUrl = 'https://windows.php.net/downloads/releases'

# Each entry: @(minor, vsRuntime)
$Candidates = @(
    @('8.4', 'vs17'),
    @('8.3', 'vs16')
)
$PatchRange = 40..0   # probe newest-first; covers all foreseeable patch releases

Write-Host "Searching for PHP 8.x NTS x64 on windows.php.net ..."
Write-Host ""

$FoundUrl     = $null
$FoundVersion = $null
$FoundAsset   = $null

:search foreach ($c in $Candidates) {
    $Minor = $c[0]
    $Vs    = $c[1]
    Write-Host "  Trying PHP $Minor.x (nts-Win32-$Vs-x64) ..."
    foreach ($Patch in $PatchRange) {
        $Version = "$Minor.$Patch"
        $Asset   = "php-$Version-nts-Win32-$Vs-x64.zip"
        $Url     = "$BaseUrl/$Asset"

        $code = (& curl.exe --head --silent --output NUL `
                 --write-out "%{http_code}" --location `
                 --connect-timeout 10 $Url 2>$null).Trim()

        if ($code -eq '200') {
            $FoundUrl     = $Url
            $FoundVersion = $Version
            $FoundAsset   = $Asset
            break search
        }
    }
}

if (-not $FoundUrl) {
    Write-Error (
        "Could not find a PHP 8.x NTS x64 binary on windows.php.net.`n" +
        "Tried: PHP 8.4 (vs17) and PHP 8.3 (vs16), patches 0-$($PatchRange[0])`n`n" +
        "-- Quick diagnostic --`n" +
        "Run this to check server connectivity:`n" +
        "  curl.exe --head --verbose https://windows.php.net/downloads/releases/php-8.3.1-nts-Win32-vs16-x64.zip`n`n" +
        "-- Manual fallback --`n" +
        "1. Open https://windows.php.net/download/ in your browser`n" +
        "2. Download the PHP 8.x Non Thread Safe x64 ZIP`n" +
        "   (filename looks like: php-8.3.x-nts-Win32-vs16-x64.zip)`n" +
        "3. Re-run with:`n" +
        "     .\scripts\download-php.ps1 -PhpZipPath 'C:\path\to\file.zip'"
    )
    exit 1
}

Write-Host "Downloading PHP $FoundVersion NTS x64 ..."
Write-Host "  Source : $FoundUrl"
Write-Host "  Dest   : $Dest"
Write-Host ""

New-Item -ItemType Directory -Force -Path $Dest | Out-Null

$Tmp     = [System.IO.Path]::GetTempPath()
$TmpFile = Join-Path $Tmp $FoundAsset

Write-Host "Fetching $FoundAsset ..."
Invoke-WebRequest -Uri $FoundUrl -OutFile $TmpFile -UseBasicParsing

Write-Host "Extracting ..."
Expand-Archive -Path $TmpFile -DestinationPath $Dest -Force

Remove-Item $TmpFile -Force

Write-Host ""
Write-Host "Done. PHP is ready at:"
Write-Host "  $Dest\php.exe"
Write-Host ""
Write-Host "Verify with:"
Write-Host "  & '$Dest\php.exe' --version"
