# ONNX Runtime Setup for Windows

This guide covers setting up ONNX Runtime with DirectML support for GPU-accelerated inference on Windows.

## Prerequisites

- Windows 10/11 (DirectML requires Windows 10 version 1903+)
- Visual Studio 2019 or later
- CMake 3.20+
- Git

## Download ONNX Runtime

### Option 1: Prebuilt Binaries (Recommended)

Download the latest ONNX Runtime with DirectML support from the official releases:

```powershell
# Create deps directory
mkdir C:\onnxruntime
cd C:\onnxruntime

# Download ONNX Runtime with DirectML (example for version 1.16.3)
# Visit: https://github.com/microsoft/onnxruntime/releases
# Download: onnxruntime-win-x64-gpu-1.16.3.zip (includes DirectML)

# Extract to C:\onnxruntime\onnxruntime-win-x64-gpu-1.16.3
```

**Latest recommended version**: 1.16.3 or newer

### Option 2: Build from Source (Advanced)

```powershell
git clone --recursive https://github.com/microsoft/onnxruntime.git
cd onnxruntime

# Build with DirectML
.\build.bat --config Release --build_shared_lib --parallel ^
    --use_dml --cmake_generator "Visual Studio 17 2022"

# Binaries will be in build\Windows\Release\Release\
```

## Project Configuration

### 1. Set Environment Variable

```powershell
# Set ONNXRUNTIME_DIR to your installation path
setx ONNXRUNTIME_DIR "C:\onnxruntime\onnxruntime-win-x64-gpu-1.16.3"
```

### 2. Configure CMake

Update `runtime/CMakeLists.txt` to use the ONNXRUNTIME_DIR:

```cmake
# Find ONNX Runtime
if(DEFINED ENV{ONNXRUNTIME_DIR})
    set(ONNXRUNTIME_DIR $ENV{ONNXRUNTIME_DIR})
    message(STATUS "Using ONNX Runtime from: ${ONNXRUNTIME_DIR}")

    # Include directories
    include_directories(${ONNXRUNTIME_DIR}/include)

    # Link directories
    link_directories(${ONNXRUNTIME_DIR}/lib)

    set(onnxruntime_FOUND TRUE)
else()
    message(WARNING "ONNXRUNTIME_DIR not set. Looking for system installation...")
    find_package(onnxruntime CONFIG)
endif()
```

### 3. Build the Project

```powershell
cd WinCoreML
mkdir build
cd build

cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

## Verify Installation

```powershell
# Check that ONNX Runtime DLLs are accessible
dir %ONNXRUNTIME_DIR%\lib\onnxruntime.dll

# Expected files:
# - onnxruntime.dll
# - onnxruntime.lib
# - DirectML.dll (for GPU support)
```

## DirectML Requirements

DirectML is included with ONNX Runtime GPU builds. It requires:

- **Driver**: Latest GPU driver (NVIDIA, AMD, or Intel)
- **OS**: Windows 10 version 1903 (build 18362) or newer
- **API**: DirectX 12 capable GPU

### Verify DirectML Support

```python
import onnxruntime as ort

# Check available providers
print(ort.get_available_providers())
# Should include: ['DmlExecutionProvider', 'CPUExecutionProvider']
```

## Troubleshooting

### Missing DLLs

If you get "DLL not found" errors:

1. Add ONNX Runtime bin directory to PATH:
   ```powershell
   setx PATH "%PATH%;%ONNXRUNTIME_DIR%\lib"
   ```

2. Or copy DLLs to executable directory:
   ```powershell
   copy %ONNXRUNTIME_DIR%\lib\*.dll build\bin\Release\
   ```

### DirectML Not Available

- Update Windows: DirectML requires Windows 10 1903+
- Update GPU drivers
- Verify you downloaded the GPU version (not CPU-only)

### Build Errors

- Ensure Visual Studio C++ tools are installed
- Verify CMake generator matches your VS version
- Check that ONNXRUNTIME_DIR points to correct location

## Next Steps

Once ONNX Runtime is configured:

1. Build the runtime service
2. Test with sample ONNX models
3. Run provider benchmarks to verify DirectML acceleration

See [TESTING.md](TESTING.md) for testing instructions.
