# Chocolatey Install Script for CoreMLWin

$ErrorActionPreference = 'Stop'

$packageName = 'coremlwin'
$toolsDir = "$(Split-Path -parent $MyInvocation.MyCommand.Definition)"
$version = '0.1.0'

$packageArgs = @{
  packageName   = $packageName
  fileType      = 'MSI'
  url64bit      = "https://github.com/user/WinCoreML/releases/download/v$version/CoreMLWin-$version-x64.msi"
  checksum64    = ''  # Will be filled during pack
  checksumType64= 'sha256'
  silentArgs    = "/qn /norestart /l*v `"$($env:TEMP)\$($packageName).$($version).MsiInstall.log`""
  validExitCodes= @(0, 3010, 1641)
}

# Check Windows version
$osVersion = [System.Environment]::OSVersion.Version
if ($osVersion.Major -lt 10 -or ($osVersion.Major -eq 10 -and $osVersion.Build -lt 18362)) {
    throw "CoreMLWin requires Windows 10 version 1903 (build 18362) or newer. Current version: $osVersion"
}

Write-Host "Installing CoreMLWin Universal ML Runtime..." -ForegroundColor Cyan

# Install MSI
Install-ChocolateyPackage @packageArgs

# Add to PATH
$binPath = Join-Path $env:ProgramFiles "CoreMLWin\bin"
Install-ChocolateyPath -PathToInstall $binPath -PathType 'Machine'

# Install Python SDK if Python is available
if (Get-Command python -ErrorAction SilentlyContinue) {
    Write-Host "Installing Python SDK..." -ForegroundColor Yellow
    $pythonSdkPath = Join-Path $env:ProgramFiles "CoreMLWin\sdk\python"
    try {
        & python -m pip install -e $pythonSdkPath
        Write-Host "✓ Python SDK installed" -ForegroundColor Green
    } catch {
        Write-Warning "Failed to install Python SDK. Install manually with: pip install coremlwin"
    }
} else {
    Write-Warning "Python not found. Install Python to use the Python SDK."
}

# Create Windows Service
Write-Host "Creating Windows Service..." -ForegroundColor Yellow
$servicePath = Join-Path $binPath "coremlwin_service.exe"

try {
    $service = Get-Service -Name "CoreMLWin" -ErrorAction SilentlyContinue
    if ($service) {
        Write-Host "Service already exists, updating..." -ForegroundColor Yellow
        sc.exe config CoreMLWin binPath= "`"$servicePath`"" start= demand | Out-Null
    } else {
        sc.exe create CoreMLWin binPath= "`"$servicePath`"" start= demand DisplayName= "CoreMLWin Universal ML Runtime" | Out-Null
        sc.exe description CoreMLWin "High-performance machine learning runtime with DirectML GPU acceleration" | Out-Null
        Write-Host "✓ Service created" -ForegroundColor Green
    }
} catch {
    Write-Warning "Failed to create service: $_"
}

# Check for DirectML support
Write-Host ""
Write-Host "Checking DirectML support..." -ForegroundColor Yellow
$osInfo = Get-CimInstance -ClassName Win32_OperatingSystem
if ($osInfo.BuildNumber -ge 18362) {
    Write-Host "✓ DirectML supported (Windows 10 $($osInfo.BuildNumber))" -ForegroundColor Green
} else {
    Write-Warning "DirectML requires Windows 10 build 18362 or newer"
}

Write-Host ""
Write-Host "======================================" -ForegroundColor Cyan
Write-Host "✓ CoreMLWin installed successfully!" -ForegroundColor Green
Write-Host "======================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Quick Start:" -ForegroundColor Cyan
Write-Host "  1. Start service: sc start CoreMLWin" -ForegroundColor White
Write-Host "  2. Check health: python -m coremlwin_client health" -ForegroundColor White
Write-Host "  3. View docs: $env:ProgramFiles\CoreMLWin\docs\README.md" -ForegroundColor White
Write-Host ""
Write-Host "Documentation: https://github.com/user/WinCoreML/tree/main/docs" -ForegroundColor Cyan
Write-Host ""
