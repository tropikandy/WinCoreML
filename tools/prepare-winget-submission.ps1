# Prepare Winget Submission
# Automates the winget-pkgs submission process

param(
    [Parameter(Mandatory=$true)]
    [string]$Version,

    [Parameter(Mandatory=$true)]
    [string]$InstallerUrl,

    [Parameter(Mandatory=$true)]
    [string]$SHA256Hash,

    [string]$WingetPkgsRepo = "..\winget-pkgs",

    [switch]$CreatePR
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Winget Submission Preparation" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "Package: CoreMLWin.UniversalMLRuntime" -ForegroundColor Cyan
Write-Host "Version: $Version" -ForegroundColor White
Write-Host "Installer URL: $InstallerUrl" -ForegroundColor White
Write-Host "SHA256: $SHA256Hash" -ForegroundColor White
Write-Host ""

# Check if winget-pkgs repo exists
if (-not (Test-Path $WingetPkgsRepo)) {
    Write-Host "✗ winget-pkgs repository not found at: $WingetPkgsRepo" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please clone your fork first:" -ForegroundColor Yellow
    Write-Host "  git clone https://github.com/YOUR_USERNAME/winget-pkgs.git" -ForegroundColor White
    Write-Host ""
    exit 1
}

Write-Host "✓ winget-pkgs repository found" -ForegroundColor Green

# Create manifest directory
$manifestDir = Join-Path $WingetPkgsRepo "manifests\c\CoreMLWin\UniversalMLRuntime\$Version"
Write-Host "Creating manifest directory..." -ForegroundColor Cyan
New-Item -ItemType Directory -Force -Path $manifestDir | Out-Null
Write-Host "✓ Directory created: $manifestDir" -ForegroundColor Green

# Copy and update manifests
Write-Host ""
Write-Host "Updating manifest files..." -ForegroundColor Cyan

# 1. Version manifest
$versionManifest = @"
# Windows Package Manager (Winget) Manifest
# Main manifest file

PackageIdentifier: CoreMLWin.UniversalMLRuntime
PackageVersion: $Version
DefaultLocale: en-US
ManifestType: version
ManifestVersion: 1.6.0
"@

$versionManifest | Out-File "$manifestDir\CoreMLWin.UniversalMLRuntime.yaml" -Encoding UTF8
Write-Host "  ✓ CoreMLWin.UniversalMLRuntime.yaml" -ForegroundColor Green

# 2. Installer manifest
$installerManifest = @"
# Winget Installer Manifest

PackageIdentifier: CoreMLWin.UniversalMLRuntime
PackageVersion: $Version
Platform:
- Windows.Desktop
MinimumOSVersion: 10.0.18362.0
InstallerType: wix
Scope: machine
InstallModes:
- interactive
- silent
- silentWithProgress
UpgradeBehavior: install
Dependencies:
  PackageDependencies:
  - PackageIdentifier: Microsoft.VCRedist.2022.x64
    MinimumVersion: 14.30.30704.0
Capabilities:
- internetClient
ReleaseDate: $(Get-Date -Format "yyyy-MM-dd")
Installers:
- Architecture: x64
  InstallerUrl: $InstallerUrl
  InstallerSha256: $SHA256Hash
  ProductCode: '{12345678-1234-1234-1234-123456789ABC}'
  InstallerSwitches:
    Custom: /norestart
    Silent: /quiet /qn
    SilentWithProgress: /quiet /passive
    Log: /l*v <LOGPATH>
  AppsAndFeaturesEntries:
  - DisplayName: CoreMLWin Universal ML Runtime
    DisplayVersion: $Version
    Publisher: CoreMLWin Project
    ProductCode: '{12345678-1234-1234-1234-123456789ABC}'
    InstallerType: msi
ManifestType: installer
ManifestVersion: 1.6.0
"@

$installerManifest | Out-File "$manifestDir\CoreMLWin.UniversalMLRuntime.installer.yaml" -Encoding UTF8
Write-Host "  ✓ CoreMLWin.UniversalMLRuntime.installer.yaml" -ForegroundColor Green

# 3. Locale manifest
$localeManifest = @"
# Winget Locale Manifest - English (US)

PackageIdentifier: CoreMLWin.UniversalMLRuntime
PackageVersion: $Version
PackageLocale: en-US
Publisher: CoreMLWin Project
PublisherUrl: https://github.com/user/WinCoreML
PublisherSupportUrl: https://github.com/user/WinCoreML/issues
Author: CoreMLWin Contributors
PackageName: CoreMLWin - Universal ML Runtime
PackageUrl: https://github.com/user/WinCoreML
License: MIT
LicenseUrl: https://github.com/user/WinCoreML/blob/main/LICENSE
Copyright: Copyright (c) $(Get-Date -Format "yyyy") CoreMLWin Project
ShortDescription: Universal Machine Learning Runtime for Windows with DirectML GPU acceleration
Description: |-
  CoreMLWin brings Apple CoreML-like experience to Windows with high-performance machine learning inference.

  Features:
  • ONNX Runtime integration for broad model support
  • DirectML GPU acceleration (NVIDIA, AMD, Intel)
  • Automatic CPU fallback when GPU unavailable
  • Python SDK and C++ API
  • Production-ready with comprehensive error handling
  • Red-team tested security

  Perfect for:
  • Running ML models on Windows
  • GPU-accelerated inference
  • Desktop ML applications
  • Embedded ML in Windows apps

  Requirements:
  • Windows 10 version 1903 (build 18362) or newer
  • DirectX 12 capable GPU (for GPU acceleration)
  • Latest GPU drivers

Moniker: coremlwin
Tags:
- machine-learning
- ml
- onnx
- directml
- gpu
- inference
- runtime
- python
- cpp
- windows
- ai
- deep-learning
ManifestType: defaultLocale
ManifestVersion: 1.6.0
"@

$localeManifest | Out-File "$manifestDir\CoreMLWin.UniversalMLRuntime.locale.en-US.yaml" -Encoding UTF8
Write-Host "  ✓ CoreMLWin.UniversalMLRuntime.locale.en-US.yaml" -ForegroundColor Green

# Validate manifest (if winget is available)
Write-Host ""
Write-Host "Validating manifest..." -ForegroundColor Cyan

try {
    $wingetCmd = Get-Command winget -ErrorAction Stop

    $validateOutput = winget validate --manifest $manifestDir 2>&1

    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ Manifest validation passed!" -ForegroundColor Green
    } else {
        Write-Host "✗ Manifest validation failed:" -ForegroundColor Red
        Write-Host $validateOutput -ForegroundColor Yellow
        Write-Host ""
        Write-Host "Please fix validation errors before submitting" -ForegroundColor Yellow
        exit 1
    }
} catch {
    Write-Host "⚠ winget command not found - skipping validation" -ForegroundColor Yellow
    Write-Host "  Install Windows Package Manager to validate locally" -ForegroundColor Yellow
}

# Create git branch and commit
Write-Host ""
Write-Host "Creating git branch and commit..." -ForegroundColor Cyan

Push-Location $WingetPkgsRepo

try {
    # Create branch
    $branchName = "coremlwin-$Version"
    git checkout -b $branchName 2>$null

    if ($LASTEXITCODE -ne 0) {
        Write-Host "  Branch already exists, switching to it..." -ForegroundColor Yellow
        git checkout $branchName
    }

    # Add files
    git add "manifests/c/CoreMLWin/*"

    # Commit
    $commitMessage = "New version: CoreMLWin.UniversalMLRuntime version $Version"
    git commit -m $commitMessage

    Write-Host "✓ Committed to branch: $branchName" -ForegroundColor Green

    if ($CreatePR) {
        Write-Host ""
        Write-Host "Pushing to your fork..." -ForegroundColor Cyan

        git push -u origin $branchName

        if ($LASTEXITCODE -eq 0) {
            Write-Host "✓ Pushed to remote!" -ForegroundColor Green
            Write-Host ""
            Write-Host "========================================" -ForegroundColor Cyan
            Write-Host "Next Steps:" -ForegroundColor Yellow
            Write-Host "========================================" -ForegroundColor Cyan
            Write-Host "1. Go to: https://github.com/microsoft/winget-pkgs" -ForegroundColor White
            Write-Host "2. GitHub will show a 'Compare & pull request' button" -ForegroundColor White
            Write-Host "3. Click it and fill out the PR template" -ForegroundColor White
            Write-Host "4. Wait for automated validation and manual review" -ForegroundColor White
            Write-Host ""
        } else {
            Write-Host "✗ Push failed!" -ForegroundColor Red
            Write-Host "  Push manually with: git push -u origin $branchName" -ForegroundColor Yellow
        }
    } else {
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host "Next Steps:" -ForegroundColor Yellow
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host "1. Review the changes:" -ForegroundColor White
        Write-Host "   cd $WingetPkgsRepo" -ForegroundColor Gray
        Write-Host "   git diff HEAD~1" -ForegroundColor Gray
        Write-Host ""
        Write-Host "2. Push to your fork:" -ForegroundColor White
        Write-Host "   git push -u origin $branchName" -ForegroundColor Gray
        Write-Host ""
        Write-Host "3. Create PR at:" -ForegroundColor White
        Write-Host "   https://github.com/microsoft/winget-pkgs" -ForegroundColor Gray
        Write-Host ""
    }

} finally {
    Pop-Location
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "✓ Preparation complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Manifest location:" -ForegroundColor Cyan
Write-Host "  $manifestDir" -ForegroundColor White
Write-Host ""
