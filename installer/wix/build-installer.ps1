# Build CoreMLWin MSI Installer
# Requires WiX Toolset: https://wixtoolset.org/

param(
    [string]$BuildDir = "..\..\build\bin\Release",
    [string]$SourceDir = "..\..",
    [string]$ONNXRuntimeDir = $env:ONNXRUNTIME_DIR,
    [string]$ModelsDir = "..\..\models",
    [string]$OutputDir = ".\output",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

Write-Host "======================================" -ForegroundColor Cyan
Write-Host "CoreMLWin MSI Installer Build Script" -ForegroundColor Cyan
Write-Host "======================================" -ForegroundColor Cyan
Write-Host ""

# Check WiX installation
try {
    $candle = Get-Command candle.exe -ErrorAction Stop
    $light = Get-Command light.exe -ErrorAction Stop
    Write-Host "✓ WiX Toolset found" -ForegroundColor Green
} catch {
    Write-Host "✗ WiX Toolset not found!" -ForegroundColor Red
    Write-Host "  Install from: https://wixtoolset.org/" -ForegroundColor Yellow
    exit 1
}

# Check build directory
if (-not (Test-Path $BuildDir)) {
    Write-Host "✗ Build directory not found: $BuildDir" -ForegroundColor Red
    Write-Host "  Run CMake build first!" -ForegroundColor Yellow
    exit 1
}
Write-Host "✓ Build directory found: $BuildDir" -ForegroundColor Green

# Check ONNX Runtime
if (-not $ONNXRuntimeDir -or -not (Test-Path $ONNXRuntimeDir)) {
    Write-Host "✗ ONNX Runtime not found!" -ForegroundColor Red
    Write-Host "  Set ONNXRUNTIME_DIR environment variable" -ForegroundColor Yellow
    exit 1
}
Write-Host "✓ ONNX Runtime found: $ONNXRuntimeDir" -ForegroundColor Green

# Create output directory
if ($Clean -and (Test-Path $OutputDir)) {
    Write-Host "Cleaning output directory..." -ForegroundColor Yellow
    Remove-Item -Path $OutputDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

Write-Host ""
Write-Host "Building installer..." -ForegroundColor Cyan

# Step 1: Compile .wxs to .wixobj
Write-Host "[1/3] Compiling WiX source..." -ForegroundColor Yellow

$candleArgs = @(
    "CoreMLWin.wxs",
    "-ext", "WixUIExtension",
    "-ext", "WixUtilExtension",
    "-dBuildDir=$BuildDir",
    "-dSourceDir=$SourceDir",
    "-dONNXRuntimeDir=$ONNXRuntimeDir",
    "-dModelsDir=$ModelsDir",
    "-out", "$OutputDir\CoreMLWin.wixobj"
)

& candle.exe $candleArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "✗ Compilation failed!" -ForegroundColor Red
    exit 1
}
Write-Host "  ✓ Compiled successfully" -ForegroundColor Green

# Step 2: Link .wixobj to .msi
Write-Host "[2/3] Linking MSI..." -ForegroundColor Yellow

$lightArgs = @(
    "$OutputDir\CoreMLWin.wixobj",
    "-ext", "WixUIExtension",
    "-ext", "WixUtilExtension",
    "-out", "$OutputDir\CoreMLWin-0.1.0-x64.msi",
    "-sval"  # Suppress ICE validation for faster builds
)

& light.exe $lightArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "✗ Linking failed!" -ForegroundColor Red
    exit 1
}
Write-Host "  ✓ Linked successfully" -ForegroundColor Green

# Step 3: Verify MSI
Write-Host "[3/3] Verifying MSI..." -ForegroundColor Yellow

$msiPath = "$OutputDir\CoreMLWin-0.1.0-x64.msi"
if (Test-Path $msiPath) {
    $msiSize = (Get-Item $msiPath).Length / 1MB
    Write-Host "  ✓ MSI created: $msiPath" -ForegroundColor Green
    Write-Host "  Size: $([math]::Round($msiSize, 2)) MB" -ForegroundColor Cyan
} else {
    Write-Host "✗ MSI not created!" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "======================================" -ForegroundColor Cyan
Write-Host "✓ Build completed successfully!" -ForegroundColor Green
Write-Host "======================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Installer location:" -ForegroundColor Cyan
Write-Host "  $msiPath" -ForegroundColor White
Write-Host ""
Write-Host "To install:" -ForegroundColor Cyan
Write-Host "  msiexec /i `"$msiPath`"" -ForegroundColor White
Write-Host ""
Write-Host "To install silently:" -ForegroundColor Cyan
Write-Host "  msiexec /i `"$msiPath`" /quiet /qn" -ForegroundColor White
Write-Host ""
