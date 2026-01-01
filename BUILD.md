# Building Universal ML Runtime

Complete build instructions for the Universal ML Runtime service and SDK.

## Prerequisites

### Windows

- **Windows 10/11** (build 19041+)
- **Visual Studio 2022** with C++ Desktop Development workload
- **CMake 3.20+**
- **Python 3.10+**
- **Protocol Buffers Compiler** (`protoc`)

### Install Tools

```powershell
# Install Chocolatey (if not installed)
Set-ExecutionPolicy Bypass -Scope Process -Force
[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

# Install build tools
choco install cmake protoc visualstudio2022-workload-nativecross -y

# Install Python dependencies
pip install protobuf numpy pywin32
```

## Step 1: Generate Protobuf Files

The runtime uses Protocol Buffers for IPC communication. You must generate the language-specific bindings before building.

### Generate C++ Protobuf Files

```powershell
# Navigate to project root
cd WinCoreML

# Create output directory
mkdir -p runtime/build/generated

# Generate C++ bindings
protoc --cpp_out=runtime/build/generated `
       --proto_path=protos `
       protos/coremlwin_runtime.proto

# Verify files were generated
ls runtime/build/generated/coremlwin_runtime.pb.h
ls runtime/build/generated/coremlwin_runtime.pb.cc
```

### Generate Python Protobuf Files

```powershell
# Generate Python bindings
protoc --python_out=sdk/python/coreml_win `
       --proto_path=protos `
       protos/coremlwin_runtime.proto

# Verify file was generated
ls sdk/python/coreml_win/coremlwin_runtime_pb2.py
```

**Note:** CMake will automatically generate C++ protobuf files during the build process if `protoc` is available. Python files must be generated manually before installing the SDK.

## Step 2: Build C++ Runtime Service

### Option A: CMake Command Line (Recommended)

```powershell
cd runtime

# Generate build files
cmake -B build -G "Visual Studio 17 2022" -A x64

# Build Release
cmake --build build --config Release

# Build Debug (for development)
cmake --build build --config Debug

# Executable location
ls build/bin/Release/coremlwin_service.exe
```

### Option B: Visual Studio IDE

```powershell
# Generate VS solution
cd runtime
cmake -B build -G "Visual Studio 17 2022"

# Open solution
start build/CoreMLWinRuntime.sln

# In Visual Studio:
# 1. Select "Release" or "Debug" configuration
# 2. Build → Build Solution (Ctrl+Shift+B)
# 3. Run → Start Without Debugging (Ctrl+F5)
```

### Build Options

```powershell
# Disable tests
cmake -B build -DCMW_BUILD_TESTS=OFF

# Disable provider plugins
cmake -B build -DCMW_BUILD_PROVIDERS=OFF

# Custom installation prefix
cmake -B build -DCMAKE_INSTALL_PREFIX=C:/ml_runtime

# Specify vcpkg toolchain (if using vcpkg)
cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

## Step 3: Install Dependencies (Optional)

### Using vcpkg (Recommended for Protobuf)

```powershell
# Clone vcpkg
git clone https://github.com/Microsoft/vcpkg.git C:/vcpkg
cd C:/vcpkg

# Bootstrap
.\bootstrap-vcpkg.bat

# Install protobuf
.\vcpkg install protobuf:x64-windows

# Build with vcpkg toolchain
cd WinCoreML/runtime
cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

### ONNX Runtime (Required for Inference)

The runtime currently has placeholder ONNX Runtime integration. To enable actual inference:

1. **Download ONNX Runtime**:
   ```powershell
   # Download from https://github.com/microsoft/onnxruntime/releases
   # Extract to C:/onnxruntime
   ```

2. **Update CMakeLists.txt**:
   ```cmake
   # In runtime/CMakeLists.txt, add:
   set(ONNXRUNTIME_DIR "C:/onnxruntime")

   include_directories(${ONNXRUNTIME_DIR}/include)
   link_directories(${ONNXRUNTIME_DIR}/lib)

   target_link_libraries(coremlwin_service PRIVATE onnxruntime)
   ```

3. **Uncomment ONNX Runtime code** in `runtime/execution/onnx_executor.cpp`

4. **Rebuild**:
   ```powershell
   cmake --build build --config Release
   ```

## Step 4: Install Python SDK

```powershell
cd sdk/python

# Generate protobuf files first (if not done)
protoc --python_out=coreml_win `
       --proto_path=../../protos `
       ../../protos/coremlwin_runtime.proto

# Install in development mode (editable)
pip install -e .

# Or install normally
pip install .

# Verify installation
python -c "from coreml_win import RuntimeClient; print('OK')"
```

### With Converter Dependencies (for model conversion)

```powershell
# Install with converter support
pip install -e ".[converter]"

# This installs: torch, tensorflow, tf2onnx, coremltools, onnx, onnxruntime
```

## Step 5: Verify Installation

### Test C++ Service

```powershell
cd runtime/build/bin/Release

# Start service
.\coremlwin_service.exe

# Should see:
# ========================================
# Universal ML Runtime Service v0.1.0
# ...
# Service ready!
```

### Test Python Client

In a new terminal:

```python
from coreml_win import RuntimeClient

# Connect to service
client = RuntimeClient()

# Check health
health = client.health()
print(f"Service ready: {health['ready']}")

# Should print: Service ready: True
```

## Common Build Issues

### 1. Protobuf Not Found

**Error:**
```
CMake Error: Could NOT find Protobuf
```

**Solution:**
```powershell
# Install via vcpkg
vcpkg install protobuf:x64-windows

# Or set PROTOBUF_ROOT
$env:PROTOBUF_ROOT = "C:/path/to/protobuf"
cmake -B build
```

### 2. Python protobuf Module Import Error

**Error:**
```python
ModuleNotFoundError: No module named 'coremlwin_runtime_pb2'
```

**Solution:**
```powershell
# Generate Python protobuf files
cd sdk/python
protoc --python_out=coreml_win `
       --proto_path=../../protos `
       ../../protos/coremlwin_runtime.proto
```

### 3. Named Pipe Connection Failed

**Error:**
```
CoreMLWinError: Service not running
```

**Solution:**
1. Check service is running: `tasklist | findstr coremlwin_service`
2. Verify pipe name matches (default: `\\.\pipe\coremlwin_runtime`)
3. Check Windows firewall/permissions

### 4. Visual Studio Compiler Not Found

**Error:**
```
CMake Error: No CMAKE_CXX_COMPILER could be found
```

**Solution:**
```powershell
# Install Visual Studio C++ workload
choco install visualstudio2022-workload-nativecross -y

# Or manually launch VS Installer and add "Desktop development with C++"
```

## Development Build

For active development:

```powershell
# Build in Debug mode
cd runtime
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug

# Run with verbose logging
.\build\bin\Debug\coremlwin_service.exe --dev

# Install Python SDK in editable mode
cd sdk/python
pip install -e ".[dev]"

# Run tests
pytest tests/
```

## Installing to System

```powershell
# Build Release
cd runtime
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=C:/Program Files/UMLRuntime
cmake --build build --config Release

# Install (requires admin)
cmake --install build

# Files installed to:
# C:/Program Files/UMLRuntime/bin/coremlwin_service.exe
# C:/Program Files/UMLRuntime/lib/coremlwin_core.lib
# C:/Program Files/UMLRuntime/include/coremlwin/*.h
```

## Next Steps

After building successfully:

1. Read [QUICK_START.md](QUICK_START.md) for usage examples
2. Check [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for system design
3. Review [tools/converter_worker/README.md](tools/converter_worker/README.md) for model conversion details
4. See [sdk/native/unity_example.cs](sdk/native/unity_example.cs) for Unity integration

## Getting Help

- **Build Issues**: https://github.com/tropikandy/WinCoreML/issues
- **Documentation**: https://github.com/tropikandy/WinCoreML/wiki
- **Logs**: Check `./cache/logs/` directory for runtime logs
