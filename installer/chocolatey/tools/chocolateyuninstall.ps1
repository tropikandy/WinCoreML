# Chocolatey Uninstall Script for CoreMLWin

$ErrorActionPreference = 'Stop'

$packageName = 'coremlwin'
$version = '0.1.0'

Write-Host "Uninstalling CoreMLWin Universal ML Runtime..." -ForegroundColor Cyan

# Stop and remove Windows Service
Write-Host "Removing Windows Service..." -ForegroundColor Yellow
try {
    $service = Get-Service -Name "CoreMLWin" -ErrorAction SilentlyContinue
    if ($service) {
        if ($service.Status -eq 'Running') {
            Stop-Service -Name "CoreMLWin" -Force
            Write-Host "  Service stopped" -ForegroundColor Green
        }
        sc.exe delete CoreMLWin | Out-Null
        Write-Host "✓ Service removed" -ForegroundColor Green
    }
} catch {
    Write-Warning "Failed to remove service: $_"
}

# Uninstall Python SDK
Write-Host "Uninstalling Python SDK..." -ForegroundColor Yellow
if (Get-Command python -ErrorAction SilentlyContinue) {
    try {
        & python -m pip uninstall coremlwin -y
        Write-Host "✓ Python SDK uninstalled" -ForegroundColor Green
    } catch {
        Write-Warning "Failed to uninstall Python SDK"
    }
}

# Uninstall MSI
$packageArgs = @{
  packageName   = $packageName
  fileType      = 'MSI'
  silentArgs    = "/qn /norestart"
  validExitCodes= @(0, 3010, 1641)
  file          = ''
}

$uninstallKey = Get-ItemProperty -Path "HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*" |
                Where-Object { $_.DisplayName -like "*CoreMLWin*" } |
                Select-Object -First 1

if ($uninstallKey) {
    $packageArgs.file = $uninstallKey.UninstallString
    Uninstall-ChocolateyPackage @packageArgs
}

Write-Host ""
Write-Host "======================================" -ForegroundColor Cyan
Write-Host "✓ CoreMLWin uninstalled successfully!" -ForegroundColor Green
Write-Host "======================================" -ForegroundColor Cyan
Write-Host ""
