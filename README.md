# CoreML-on-Windows

A high-performance runtime service that brings CoreML model inference to Windows using native acceleration providers (DirectML, OpenVINO, ONNX Runtime).

## Overview

CoreML-on-Windows provides a transparent bridge between CoreML models and Windows hardware acceleration, enabling:

- **Zero-code compatibility**: Run CoreML models on Windows without modification
- **Hardware acceleration**: Leverage DirectML (GPU), OpenVINO (NPU/CPU), and ONNX Runtime
- **Automatic fallback**: Intelligent provider selection with graceful degradation
- **Production-ready**: Robust error handling, telemetry, and configuration

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                   Client Applications                    │
│         (Python SDK, .NET SDK, Native API)              │
└────────────────────┬────────────────────────────────────┘
                     │ Named Pipes (IPC)
┌────────────────────▼────────────────────────────────────┐
│              CoreMLWin Runtime Service                   │
│  ┌──────────────┬──────────────┬──────────────────┐    │
│  │ Model Registry│Policy Engine │Provider Registry │    │
│  └──────────────┴──────────────┴──────────────────┘    │
│  ┌──────────────────────────────────────────────────┐  │
│  │         ONNX Conversion Worker                    │  │
│  │      (CoreML → ONNX via coremltools)             │  │
│  └──────────────────────────────────────────────────┘  │
└────────────────────┬────────────────────────────────────┘
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

### Intermediate Representation: ONNX
- CoreML models converted to ONNX via coremltools worker
- DirectML and OpenVINO natively consume ONNX
- No custom IR to maintain

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

# Register a model
model_id = client.register_model("path/to/model.mlpackage")

# Run inference
inputs = {"input_image": np.random.randn(1, 3, 224, 224).astype(np.float32)}
outputs = client.predict(model_id, inputs)

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

### Phase 0: Foundation (Steps 1-4)
- [x] Repository setup
- [x] Protocol definitions
- [x] Error taxonomy
- [x] Provider interface

### Phase 1: Service MVP (Steps 5-8)
- [ ] Named pipe IPC server
- [ ] Model registry
- [ ] CPU provider (ONNX Runtime)
- [ ] Basic Python SDK

### Phase 2: Acceleration (Steps 9-12)
- [ ] DirectML provider
- [ ] OpenVINO provider
- [ ] Provider selection engine
- [ ] Model conversion worker

### Phase 3: Hardening (Steps 13-16)
- [ ] Caching layer
- [ ] Telemetry
- [ ] Configuration system
- [ ] Installer

### Phase 4: Extensibility (Steps 17-19)
- [ ] Plugin system
- [ ] CLI tools
- [ ] GUI (Dear ImGui)

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
