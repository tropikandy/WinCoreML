# Universal ML Runtime for Windows

A high-performance, universal ML inference runtime for Windows that supports multiple model formats with native hardware acceleration (DirectML, OpenVINO, ONNX Runtime).

## Overview

The Universal ML Runtime provides a unified interface for running ML models from any framework on Windows, with automatic optimization and hardware acceleration:

- **Multi-format support**: PyTorch (.pt), TensorFlow (SavedModel), CoreML (.mlmodel), ONNX - all run seamlessly
- **Automatic conversion**: All models converted to ONNX at registration with validation and optimization
- **Hardware acceleration**: Leverage DirectML (GPU/NPU), OpenVINO (Intel NPU), and ONNX Runtime (CPU)
- **Auto-benchmarking**: Measure expected speedup across providers at model registration time
- **Intelligent routing**: Provider selection with automatic fallback and retry logic
- **Unity-ready**: C API designed for Unity and game engine integration
- **Production-ready**: Robust error handling, telemetry, and configuration

## Architecture

```
┌────────────────────────────────────────────────────────────┐
│                   Client Applications                       │
│   (Python SDK, .NET SDK, Unity Native Plugin, C API)       │
└────────────────────┬───────────────────────────────────────┘
                     │ Named Pipes (IPC)
┌────────────────────▼───────────────────────────────────────┐
│              Universal ML Runtime Service                   │
│  ┌──────────────┬──────────────┬──────────────────────┐   │
│  │ Model Registry│Policy Engine │Provider Registry     │   │
│  │ + Benchmarks  │+ Auto-select │+ Dynamic Loading     │   │
│  └──────────────┴──────────────┴──────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐ │
│  │      Universal Model Converter Worker                 │ │
│  │  ┌────────────┬────────────┬────────────┬─────────┐  │ │
│  │  │  PyTorch   │TensorFlow  │  CoreML    │  ONNX   │  │ │
│  │  │ (.pt/pth)  │(SavedModel)│(.mlpackage)│(native) │  │ │
│  │  └─────┬──────┴──────┬─────┴─────┬──────┴────┬────┘  │ │
│  │        └─────────────┴───────────┴───────────┘       │ │
│  │                     ↓ ONNX + Validation               │ │
│  └──────────────────────────────────────────────────────┘ │
│  ┌──────────────────────────────────────────────────────┐ │
│  │     Benchmarking Engine (Auto-measures speedup)       │ │
│  └──────────────────────────────────────────────────────┘ │
└────────────────────┬───────────────────────────────────────┘
                     │
      ┌──────────────┼──────────────┬─────────────┐
      │              │              │             │
┌─────▼─────┐  ┌────▼─────┐  ┌────▼─────┐  ┌────▼─────┐
│ DirectML  │  │ OpenVINO │  │   CPU    │  │  Custom  │
│ (GPU/NPU) │  │(NPU/CPU) │  │  (ORT)   │  │ Provider │
└───────────┘  └──────────┘  └──────────┘  └──────────┘
```

## Key Design Decisions (v2)

### IPC Transport: Named Pipes
- **Primary**: Windows Named Pipes (`\\.\pipe\coremlwin_runtime`)
- **Wire Format**: 4-byte little-endian length prefix + Protocol Buffers
- **Secondary**: gRPC (opt-in via config)

### Universal Model Support via ONNX
- **PyTorch**: Converted via torch.onnx.export
- **TensorFlow**: Converted via tf2onnx
- **CoreML**: Converted via coremltools
- **ONNX**: Native support (no conversion)
- All providers consume ONNX - no custom IR to maintain
- Automatic validation and optimization during conversion
- Benchmarking runs after conversion to show expected speedup

### Error Taxonomy
Unified error codes with clear categories:
- `0`: Success
- `1000-1999`: Client errors (invalid input, bad config)
- `2000-2999`: Model errors (parsing, validation, conversion)
- `3000-3999`: Provider errors (hardware, driver issues)
- `4000-4999`: Runtime errors (internal failures)
- `5000-5999`: Transient errors (retry-able)

## Project Structure

```
WinCoreML/
├── runtime/
│   ├── include/              # Public headers
│   │   ├── coremlwin_errors.h           # Error taxonomy
│   │   └── coremlwin_provider_api.h     # Provider interface
│   ├── service/              # Runtime service implementation
│   ├── execution/            # ONNX execution backends
│   ├── routing/              # Provider selection logic
│   └── providers/            # Built-in providers
├── sdk/
│   ├── python/               # Python SDK
│   ├── dotnet/               # .NET SDK
│   └── native/               # C/C++ SDK
├── tools/
│   ├── cli/                  # Command-line tools
│   ├── gui/                  # Dear ImGui GUI (optional)
│   └── converter_worker/     # CoreML→ONNX worker
├── protos/                   # Protocol Buffers definitions
├── configs/                  # Configuration and policies
└── docs/                     # Documentation
```

## Quick Start

### Prerequisites

- **Windows 10/11** (build 19041+)
- **Visual Studio 2022** with C++ tools
- **CMake 3.20+**
- **Python 3.10+** (for SDK and converter worker)
- **vcpkg** (for C++ dependencies)

### Build Runtime Service

```powershell
# Install dependencies via vcpkg
vcpkg install protobuf:x64-windows onnxruntime:x64-windows

# Build runtime
cd runtime/service
cmake -B build -G "Visual Studio 17 2022" -DCMAKE_TOOLCHAIN_FILE="[vcpkg root]/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release

# Run service
.\build\Release\coremlwin_service.exe
```

### Install Python SDK

```powershell
cd sdk/python
pip install -e .
```

### Example Usage

```python
from coreml_win import RuntimeClient
import numpy as np

# Connect to runtime
client = RuntimeClient()

# Check service health
print(client.health())

# Register models from any framework - all automatically converted to ONNX
pytorch_model = client.register_model("models/resnet50.pt")
tf_model = client.register_model("models/mobilenet_savedmodel/")
coreml_model = client.register_model("models/classifier.mlpackage")
onnx_model = client.register_model("models/yolov8.onnx")

# Check benchmark results (auto-measured at registration)
info = client.get_model_info(pytorch_model)
print(f"Fastest provider: {info['benchmark']['fastest_provider']}")
print(f"Expected speedup vs CPU: {info['benchmark']['speedup_vs_cpu']:.2f}x")

# Run inference - runtime auto-selects best provider
inputs = {"input": np.random.randn(1, 3, 224, 224).astype(np.float32)}
outputs = client.predict(pytorch_model, inputs)

# Or force a specific provider
outputs = client.predict(pytorch_model, inputs, compute_units="CPU_AND_GPU")

print(f"Output: {outputs}")
```

## Configuration

### System Configuration
`%ProgramData%\CoreMLWin\config.json` - System-wide settings

### User Overrides
`%LOCALAPPDATA%\CoreMLWin\config.json` - User-specific settings

### Routing Policies
`%ProgramData%\CoreMLWin\policies\` - Provider selection policies

### Model Cache
`%LOCALAPPDATA%\CoreMLWin\cache\` - Converted model cache

### Logs
`%LOCALAPPDATA%\CoreMLWin\logs\` - Runtime logs

## Development Roadmap

### Phase 0: Foundation ✅
- [x] Repository setup
- [x] Protocol definitions (protobuf)
- [x] Error taxonomy (unified error codes)
- [x] Provider interface (enhanced capabilities)
- [x] **Universal converter architecture** (PyTorch, TF, CoreML, ONNX)
- [x] **Benchmarking module** (auto-measure speedup)

### Phase 1: Service MVP 🚧
- [ ] Named pipe IPC server (Windows async I/O)
- [ ] Model registry (with benchmark results storage)
- [ ] CPU provider (ONNX Runtime)
- [ ] Python SDK client (pipe communication)
- [ ] **Converter worker integration** (subprocess management)

### Phase 2: Acceleration
- [ ] DirectML provider (GPU/NPU)
- [ ] OpenVINO provider (Intel NPU)
- [ ] Provider selection engine (with benchmarks)
- [ ] INT8 quantization (NPU optimization)

### Phase 3: Unity & Gaming
- [ ] **Unity-friendly C API** (marshalling-safe)
- [ ] Unity native plugin (.dll)
- [ ] Example Unity project
- [ ] Shared memory transport (large tensors)

### Phase 4: Hardening
- [ ] Caching layer (converted models + sessions)
- [ ] Telemetry (latency, provider usage)
- [ ] Configuration system (policies)
- [ ] Windows installer (MSI)

### Phase 5: Extensibility
- [ ] Plugin system (custom providers)
- [ ] CLI tools (model management)
- [ ] Web dashboard (optional)

## Contributing

See [CONTRIBUTING.md](docs/CONTRIBUTING.md) for development guidelines.

## License

See [LICENSE](LICENSE) for license information.

## Documentation

- [Design Document](docs/CoreML_on_Windows_Design_Doc_v2.pdf)
- [API Reference](docs/API.md)
- [Provider Guide](docs/PROVIDERS.md)
- [Architecture](docs/ARCHITECTURE.md)

## Contact

For questions or issues, please open a GitHub issue or discussion.
