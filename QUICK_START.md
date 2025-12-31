# Universal ML Runtime - Quick Start Guide

This guide will get you up and running with the Universal ML Runtime in minutes.

## Prerequisites

### Windows
- Windows 10/11 (build 19041+)
- Visual Studio 2022 with C++ tools
- CMake 3.20+
- Python 3.10+

### Dependencies
```powershell
# Install Python dependencies
pip install pywin32 protobuf numpy

# For model conversion (optional)
pip install torch tensorflow tf2onnx coremltools onnx onnxruntime
```

## Building the Runtime Service

### Option 1: CMake (Recommended)

```powershell
# Navigate to runtime directory
cd runtime

# Generate build files
cmake -B build -G "Visual Studio 17 2022"

# Build
cmake --build build --config Release

# Executable will be at: build/bin/Release/coremlwin_service.exe
```

### Option 2: Visual Studio

```powershell
# Generate VS solution
cd runtime
cmake -B build -G "Visual Studio 17 2022"

# Open in Visual Studio
start build/CoreMLWinRuntime.sln

# Build in Visual Studio (Ctrl+Shift+B)
```

## Running the Service

### Start the Runtime

```powershell
# From runtime directory
.\build\bin\Release\coremlwin_service.exe

# Or with custom cache directory
.\build\bin\Release\coremlwin_service.exe --cache C:\ml_cache

# Development mode
.\build\bin\Release\coremlwin_service.exe --dev
```

You should see:

```
========================================
Universal ML Runtime Service v0.1.0
Supports: PyTorch, TensorFlow, CoreML, ONNX
========================================

[1/3] Initializing runtime state...
✓ Runtime state initialized
  Cache directory: ./cache

[2/3] Starting named pipe server...
  Pipe name: \\.\pipe\coremlwin_runtime
✓ Named pipe server started
  Waiting for client connections...

[3/3] Service ready!
========================================

Universal ML Runtime is running
Press Ctrl+C to stop

Supported Model Formats:
  • PyTorch (.pt, .pth)
  • TensorFlow (SavedModel, .h5)
  • CoreML (.mlmodel, .mlpackage)
  • ONNX (.onnx)

Features:
  • Automatic conversion to ONNX
  • Multi-provider benchmarking
  • Intelligent provider selection
  • Hardware acceleration (DirectML, OpenVINO)
```

## Using the Python SDK

### Install SDK

```powershell
cd sdk/python
pip install -e .
```

### Basic Usage

```python
from coreml_win import RuntimeClient
import numpy as np

# Connect to runtime
client = RuntimeClient()

# Check health
health = client.health()
print(f"Service ready: {health['ready']}")

# Register a model (any format!)
model_id = client.register_model("path/to/model.pt")  # PyTorch
# or client.register_model("model/")  # TensorFlow SavedModel
# or client.register_model("model.mlpackage")  # CoreML
# or client.register_model("model.onnx")  # ONNX

print(f"Model registered: {model_id}")

# Get model info and benchmarks
info = client.get_model_info(model_id)
print(f"Format: {info['format']}")
print(f"Fastest provider: {info['benchmark']['fastest_provider']}")
print(f"Speedup vs CPU: {info['benchmark']['speedup_vs_cpu']:.2f}x")

# Run inference
inputs = {
    "input": np.random.randn(1, 3, 224, 224).astype(np.float32)
}

outputs = client.predict(model_id, inputs)
print(f"Output shape: {outputs['output'].shape}")

# List all models
models = client.list_models()
for model in models:
    print(f"  {model['model_id']}: {model['format']}")
```

### Using Context Manager

```python
with RuntimeClient() as client:
    model_id = client.register_model("model.onnx")
    outputs = client.predict(model_id, inputs)
    print(outputs)
```

## Testing the Converter Worker (Standalone)

The converter can be used standalone for testing:

```powershell
cd tools/converter_worker

# Convert a model
python -m tools.converter_worker.worker <<EOF
{
  "command": "convert",
  "model_path": "path/to/model.pt",
  "output_path": "output.onnx",
  "benchmark": true,
  "opset_version": 14
}
EOF
```

Response:
```json
{
  "success": true,
  "onnx_path": "output.onnx",
  "warnings": [],
  "model_info": {
    "format": "pytorch",
    "input_names": ["input"],
    "output_names": ["output"],
    "input_shapes": {"input": [1, 3, 224, 224]},
    "output_shapes": {"output": [1, 1000]}
  },
  "benchmark": {
    "fastest_provider": "DmlExecutionProvider",
    "baseline_latency_ms": 45.2,
    "best_latency_ms": 12.5,
    "speedup_vs_cpu": 3.63
  }
}
```

## Unity Integration (C#)

See `sdk/native/unity_example.cs` for complete example.

### Quick Unity Setup

1. Copy `sdk/native/umlruntime.h` to your Unity project
2. Build the runtime DLL
3. Copy DLL to Unity `Assets/Plugins/`
4. Use P/Invoke:

```csharp
using System.Runtime.InteropServices;

[DllImport("umlruntime")]
private static extern int UMLRTCreateRuntime(
    ref RuntimeConfig config,
    out RuntimeHandle handle
);

RuntimeHandle runtime;
RuntimeConfig config = new RuntimeConfig { ... };
UMLRTCreateRuntime(ref config, out runtime);
```

## Common Issues

### "Service not running" Error

**Problem:** Python client can't connect

**Solution:**
1. Check if service is running
2. Verify pipe name matches (default: `\\.\pipe\coremlwin_runtime`)
3. Check Windows firewall/permissions

```python
# Custom pipe name
client = RuntimeClient(pipe_name=r"\\.\pipe\my_custom_pipe")
```

### Build Errors

**Problem:** Missing dependencies

**Solution:**
```powershell
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install protobuf
.\vcpkg install protobuf:x64-windows

# Build with vcpkg
cmake -B build -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### Model Conversion Fails

**Problem:** Converter worker not found

**Solution:**
1. Install converter dependencies: `pip install torch tensorflow tf2onnx coremltools`
2. Ensure Python is in PATH
3. Test converter standalone (see above)

## Performance Tips

### 1. Use Appropriate Compute Units

```python
# CPU only (fastest startup)
outputs = client.predict(model_id, inputs, compute_units="CPU_ONLY")

# GPU preferred (best performance)
outputs = client.predict(model_id, inputs, compute_units="CPU_AND_GPU")

# Try all providers (auto-select best)
outputs = client.predict(model_id, inputs, compute_units="ALL")
```

### 2. Reuse Model IDs

Model registration is expensive (conversion + benchmarking). Cache model IDs:

```python
# First run - slow
model_id = client.register_model("model.pt")
save_model_id(model_id)  # Save to file/db

# Subsequent runs - fast
model_id = load_model_id()
outputs = client.predict(model_id, inputs)
```

### 3. Batch Inputs (Future)

Currently single-input only. Batching support coming in Phase 2.

## Next Steps

- Read [Architecture](docs/ARCHITECTURE.md) for system design
- Check [Converter README](tools/converter_worker/README.md) for format details
- See [Provider Guide](docs/PROVIDERS.md) for hardware acceleration
- Review [Unity Example](sdk/native/unity_example.cs) for game integration

## Getting Help

- **GitHub Issues**: https://github.com/yourusername/WinCoreML/issues
- **Logs**: Check `./cache/logs/` for runtime logs
- **Verbose Mode**: Run service with `--dev` flag

## What's Working (v0.1.0)

✅ **Fully Implemented:**
- Named Pipe IPC server (Windows)
- Model registry with metadata storage
- ONNX executor wrapper
- Python SDK client
- Runtime state management
- Service lifecycle
- Error handling

⚠️ **Placeholder/Partial:**
- Protobuf serialization (using simple byte protocol)
- ONNX Runtime integration (needs library linking)
- Converter worker subprocess (needs Python integration)
- Provider benchmarking (framework ready)

🚧 **Coming Soon (Phase 2):**
- DirectML provider (GPU/NPU)
- OpenVINO provider (Intel NPU)
- INT8 quantization
- Actual benchmark execution
- Shared memory for large tensors

## Example Output

```
# Start service
$ ./coremlwin_service.exe
Universal ML Runtime Service v0.1.0
...
Service ready!

# Python client
>>> from coreml_win import RuntimeClient
>>> client = RuntimeClient()
>>> model_id = client.register_model("resnet50.pt")
Registering model: resnet50.pt
Converting model: resnet50.pt → cache/abc123.onnx
Model registered successfully: abc123
>>> outputs = client.predict(model_id, inputs)
Running inference on model: abc123
Inference complete: 15234 µs
>>> print(outputs['output'].shape)
(1, 1000)
```

**You're ready to go! 🚀**
