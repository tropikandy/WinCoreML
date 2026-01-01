# Create CoreMLWin Portable Package
# No installation required - just extract and run

param(
    [string]$Version = "0.1.0",
    [string]$BuildDir = "build\bin\Release",
    [string]$ONNXRuntimeDir = $env:ONNXRUNTIME_DIR,
    [string]$OutputDir = "installer\portable",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "CoreMLWin Portable Package Builder" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Validate build directory
if (-not (Test-Path $BuildDir)) {
    Write-Host "✗ Build directory not found: $BuildDir" -ForegroundColor Red
    Write-Host "  Run CMake build first!" -ForegroundColor Yellow
    exit 1
}
Write-Host "✓ Build directory found" -ForegroundColor Green

# Validate ONNX Runtime
if (-not $ONNXRuntimeDir -or -not (Test-Path $ONNXRuntimeDir)) {
    Write-Host "✗ ONNX Runtime not found!" -ForegroundColor Red
    Write-Host "  Set ONNXRUNTIME_DIR environment variable" -ForegroundColor Yellow
    exit 1
}
Write-Host "✓ ONNX Runtime found" -ForegroundColor Green

# Create staging directory
$stagingDir = "$OutputDir\staging\CoreMLWin-$Version"
if ($Clean -and (Test-Path $stagingDir)) {
    Remove-Item -Path $stagingDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $stagingDir | Out-Null

Write-Host ""
Write-Host "Copying files to staging..." -ForegroundColor Cyan

# Copy runtime executables
Write-Host "  • Runtime binaries..." -ForegroundColor Yellow
Copy-Item "$BuildDir\coremlwin_service.exe" -Destination "$stagingDir\"
Copy-Item "$BuildDir\coremlwin_converter.exe" -Destination "$stagingDir\"

# Copy ONNX Runtime dependencies
Write-Host "  • ONNX Runtime libraries..." -ForegroundColor Yellow
Copy-Item "$ONNXRuntimeDir\lib\onnxruntime.dll" -Destination "$stagingDir\"
Copy-Item "$ONNXRuntimeDir\lib\DirectML.dll" -Destination "$stagingDir\" -ErrorAction SilentlyContinue

# Copy configuration files
Write-Host "  • Configuration files..." -ForegroundColor Yellow
New-Item -ItemType Directory -Force -Path "$stagingDir\config" | Out-Null
Copy-Item "runtime\config\*.yaml" -Destination "$stagingDir\config\" -ErrorAction SilentlyContinue
if (-not (Test-Path "$stagingDir\config\*.yaml")) {
    # Create default config if none exist
    @"
runtime:
  default_provider: "DmlExecutionProvider"
  log_level: "info"
  num_threads: 4

server:
  pipe_name: "CoreMLWin"
  max_connections: 100
"@ | Out-File "$stagingDir\config\runtime_config.yaml" -Encoding UTF8
}

# Copy Python SDK
Write-Host "  • Python SDK..." -ForegroundColor Yellow
New-Item -ItemType Directory -Force -Path "$stagingDir\sdk\python" | Out-Null
Copy-Item -Recurse "sdk\python\*" -Destination "$stagingDir\sdk\python\" -Exclude "__pycache__","*.pyc","*.egg-info"

# Create README
Write-Host "  • Documentation..." -ForegroundColor Yellow
@"
# CoreMLWin v$Version - Portable Edition

Universal Machine Learning Runtime for Windows with DirectML GPU acceleration.

## 🚀 Quick Start

1. **Start the service**:
   ```
   .\coremlwin_service.exe
   ```

2. **Install Python SDK** (optional):
   ```
   pip install -e sdk/python
   ```

3. **Test installation**:
   ```python
   from coremlwin_client import CoreMLWinClient
   client = CoreMLWinClient()
   print(client.health())
   ```

## 📋 Requirements

- Windows 10 version 1903 (build 18362) or newer
- DirectX 12 capable GPU (for GPU acceleration)
- Visual C++ Redistributable 2022: https://aka.ms/vs/17/release/vc_redist.x64.exe

## 🔧 Configuration

Edit `config/runtime_config.yaml` to customize settings:

- `default_provider`: CPU or DmlExecutionProvider (GPU)
- `log_level`: debug, info, warning, error
- `num_threads`: Number of CPU threads for inference

## 📁 Directory Structure

```
CoreMLWin-$Version/
├── coremlwin_service.exe      # Main runtime service
├── coremlwin_converter.exe    # Model conversion tool
├── onnxruntime.dll            # ONNX Runtime library
├── DirectML.dll               # DirectML GPU acceleration
├── config/                    # Configuration files
│   └── runtime_config.yaml
└── sdk/                       # SDKs
    └── python/                # Python client library
```

## 🐛 Troubleshooting

**Service won't start**:
- Check if another instance is running
- Verify VC++ Redistributable is installed
- Check Windows Event Viewer for errors

**GPU not detected**:
- Update GPU drivers to latest version
- Verify DirectX 12 support: `dxdiag`
- Check `config/runtime_config.yaml` has `DmlExecutionProvider`

**Python SDK errors**:
- Ensure service is running first
- Check Python version >= 3.8
- Install dependencies: `pip install numpy protobuf pywin32`

## 📚 Documentation

- Full documentation: https://github.com/user/WinCoreML
- Installation guide: https://github.com/user/WinCoreML/blob/main/docs/INSTALLATION.md
- Security audit: https://github.com/user/WinCoreML/blob/main/docs/RED_TEAM_AUDIT.md

## 📞 Support

- Report issues: https://github.com/user/WinCoreML/issues
- Discussions: https://github.com/user/WinCoreML/discussions

## 📄 License

MIT License - see LICENSE file for details

---

**Version**: $Version
**Build Date**: $(Get-Date -Format "yyyy-MM-dd")
**Platform**: Windows x64
"@ | Out-File "$stagingDir\README.md" -Encoding UTF8

# Copy LICENSE
if (Test-Path "LICENSE") {
    Copy-Item "LICENSE" -Destination "$stagingDir\"
}

# Create startup script
Write-Host "  • Startup scripts..." -ForegroundColor Yellow
@"
@echo off
echo ========================================
echo CoreMLWin Runtime Service v$Version
echo ========================================
echo.

REM Check if already running
tasklist /FI "IMAGENAME eq coremlwin_service.exe" 2>NUL | find /I /N "coremlwin_service.exe">NUL
if "%ERRORLEVEL%"=="0" (
    echo [!] Service is already running
    echo     Use stop-service.bat to stop it first
    pause
    exit /b 1
)

echo [*] Starting CoreMLWin service...
start "CoreMLWin Service" coremlwin_service.exe

REM Wait a moment for startup
timeout /t 2 /nobreak >NUL

REM Check if started successfully
tasklist /FI "IMAGENAME eq coremlwin_service.exe" 2>NUL | find /I /N "coremlwin_service.exe">NUL
if "%ERRORLEVEL%"=="0" (
    echo [✓] Service started successfully!
    echo.
    echo     The service is running in the background.
    echo     Check Task Manager to see coremlwin_service.exe
    echo.
) else (
    echo [✗] Failed to start service
    echo     Check logs for errors
)

pause
"@ | Out-File "$stagingDir\start-service.bat" -Encoding ASCII

@"
@echo off
echo Stopping CoreMLWin service...
taskkill /IM coremlwin_service.exe /F 2>NUL
if "%ERRORLEVEL%"=="0" (
    echo [✓] Service stopped
) else (
    echo [!] Service was not running
)
pause
"@ | Out-File "$stagingDir\stop-service.bat" -Encoding ASCII

@"
@echo off
echo ========================================
echo CoreMLWin Service Status
echo ========================================
echo.

tasklist /FI "IMAGENAME eq coremlwin_service.exe" 2>NUL | find /I /N "coremlwin_service.exe">NUL
if "%ERRORLEVEL%"=="0" (
    echo Status: RUNNING
    echo.
    tasklist /FI "IMAGENAME eq coremlwin_service.exe" /V
) else (
    echo Status: STOPPED
)

echo.
pause
"@ | Out-File "$stagingDir\check-status.bat" -Encoding ASCII

# Create ZIP archive
Write-Host ""
Write-Host "Creating ZIP archive..." -ForegroundColor Cyan

$zipPath = "$OutputDir\CoreMLWin-$Version-portable-x64.zip"
if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}

# Use .NET compression
Add-Type -Assembly System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory($stagingDir, $zipPath, 'Optimal', $false)

# Verify ZIP
if (Test-Path $zipPath) {
    $zipSize = (Get-Item $zipPath).Length / 1MB
    Write-Host "✓ ZIP created: $zipPath" -ForegroundColor Green
    Write-Host "  Size: $([math]::Round($zipSize, 2)) MB" -ForegroundColor Cyan
} else {
    Write-Host "✗ Failed to create ZIP!" -ForegroundColor Red
    exit 1
}

# Generate SHA256
Write-Host ""
Write-Host "Generating checksums..." -ForegroundColor Cyan
$hash = (Get-FileHash $zipPath -Algorithm SHA256).Hash
$hash | Out-File "$zipPath.sha256" -Encoding ASCII
Write-Host "✓ SHA256: $hash" -ForegroundColor Green

# Cleanup staging
if (-not $DebugNoCleanup) {
    Remove-Item -Path "$OutputDir\staging" -Recurse -Force
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "✓ Portable package created!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Package location:" -ForegroundColor Cyan
Write-Host "  $zipPath" -ForegroundColor White
Write-Host ""
Write-Host "To test:" -ForegroundColor Cyan
Write-Host "  1. Extract ZIP to any folder" -ForegroundColor White
Write-Host "  2. Run start-service.bat" -ForegroundColor White
Write-Host "  3. Check status with check-status.bat" -ForegroundColor White
Write-Host ""
Write-Host "SHA256 Hash:" -ForegroundColor Cyan
Write-Host "  $hash" -ForegroundColor White
Write-Host ""
