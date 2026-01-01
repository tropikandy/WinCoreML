# CoreMLWin - Universal ML Runtime for Windows

🚀 **Apple CoreML-like experience for Windows with DirectML GPU acceleration**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Windows](https://img.shields.io/badge/Windows-10%2F11-blue.svg)](https://www.microsoft.com/windows)
[![ONNX Runtime](https://img.shields.io/badge/ONNX%20Runtime-1.16.3-green.svg)](https://onnxruntime.ai/)
[![DirectML](https://img.shields.io/badge/DirectML-GPU%20Accelerated-orange.svg)](https://docs.microsoft.com/en-us/windows/ai/directml/dml)

---

## ✨ Features

- **🎯 Universal ML Inference** - Run ONNX, PyTorch, TensorFlow models on Windows
- **⚡ GPU Acceleration** - DirectML support for NVIDIA, AMD, and Intel GPUs
- **🔄 Automatic Fallback** - CPU execution when GPU unavailable
- **🐍 Python SDK** - Easy-to-use client library
- **🛡️ Production Ready** - Security hardened, red-team tested
- **🔌 Simple Integration** - Drop-in replacement for CoreML on Windows

---

## 📦 Installation

### Option 1: Winget (Recommended)

```powershell
winget install CoreMLWin.UniversalMLRuntime
```

### Option 2: MSI Installer

Download from [Releases](https://github.com/user/WinCoreML/releases) and run:

```powershell
msiexec /i CoreMLWin-0.1.0-x64.msi
```

### Option 3: Portable (No Installation)

Download ZIP from [Releases](https://github.com/user/WinCoreML/releases), extract, and run:

```powershell
.\start-service.bat
```

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Your Application                         │
│                  (Python, C++, etc.)                        │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      │ Client Library
                      ▼
┌─────────────────────────────────────────────────────────────┐
│              CoreMLWin Runtime Service                      │
│  ┌─────────────────────────────────────────────────────┐   │
│  │            Named Pipe IPC Server                    │   │
│  └──────────────────────┬──────────────────────────────┘   │
│                         │                                   │
│  ┌──────────────────────▼──────────────────────────────┐   │
│  │          Model Registry & Manager                   │   │
│  └──────────────────────┬──────────────────────────────┘   │
│                         │                                   │
│  ┌──────────────────────▼──────────────────────────────┐   │
│  │           ONNX Runtime Executor                     │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │   │
│  │  │ DirectML    │  │     CPU     │  │   CUDA    │  │   │
│  │  │   (GPU)     │  │  Provider   │  │ Provider  │  │   │
│  │  └─────────────┘  └─────────────┘  └────────────┘  │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │   GPU / CPU          │
              │   Hardware           │
              └──────────────────────┘
```

## 🚀 Quick Start

### 1. Install Python SDK

```bash
pip install coremlwin
```

### 2. Start Service (if using portable)

```bash
coremlwin-service
```

### 3. Run Inference

```python
from coremlwin_client import CoreMLWinClient
import numpy as np

# Connect to runtime
client = CoreMLWinClient()

# Check service health
print(client.health())

# Register a model
model_id = client.register_model("path/to/model.onnx")

# Prepare input
input_data = {
    "input": np.random.randn(1, 3, 224, 224).astype(np.float32)
}

# Run inference
output = client.run_inference(model_id, input_data)
print(output)
```

---

## 💡 Use Cases

- **🖼️ Computer Vision** - Image classification, object detection, segmentation
- **📝 NLP** - Text classification, sentiment analysis, translation
- **🎵 Audio Processing** - Speech recognition, audio classification
- **🎮 Gaming** - Real-time ML inference in games
- **🏢 Enterprise** - Production ML deployments on Windows servers

---

## 📋 Requirements

### Minimum
- Windows 10 version 1903 (build 18362) or newer
- Visual C++ Redistributable 2022
- Python 3.8+ (for Python SDK)

### Recommended
- Windows 11
- DirectX 12 capable GPU (NVIDIA, AMD, or Intel)
- Latest GPU drivers
- 8GB+ RAM

---

## 🔧 Building from Source

### Prerequisites

- CMake 3.15+
- Visual Studio 2019/2022
- ONNX Runtime 1.16.3
- Python 3.8+

### Build Steps

```powershell
# 1. Clone repository
git clone https://github.com/user/WinCoreML.git
cd WinCoreML

# 2. Download ONNX Runtime
$version = "1.16.3"
Invoke-WebRequest -Uri "https://github.com/microsoft/onnxruntime/releases/download/v$version/onnxruntime-win-x64-gpu-$version.zip" -OutFile "onnxruntime.zip"
Expand-Archive -Path "onnxruntime.zip" -DestinationPath "."
$env:ONNXRUNTIME_DIR = (Get-ChildItem -Directory -Filter "onnxruntime-win-*").FullName

# 3. Configure with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release -DONNXRUNTIME_DIR=$env:ONNXRUNTIME_DIR

# 4. Build
cmake --build build --config Release --parallel

# 5. Run tests
pytest tests/test_integration.py -v
```

---

## 📦 Distribution

### For Developers

See [RELEASE_CHECKLIST.md](RELEASE_CHECKLIST.md) for release process.

**One-command release**:
```powershell
.\tools\release.ps1 -Version 0.1.0
```

This will:
- Run tests
- Build MSI and portable packages
- Create git tag
- Trigger GitHub Actions
- Auto-publish GitHub Release

### For End Users

Multiple installation options available:

| Method | Command | Best For |
|--------|---------|----------|
| **Winget** | `winget install CoreMLWin.UniversalMLRuntime` | Most users |
| **Chocolatey** | `choco install coremlwin` | IT admins |
| **MSI** | Download from releases | Enterprise |
| **Portable** | Extract and run | No admin rights |

See [docs/INSTALLATION.md](docs/INSTALLATION.md) for details.

---

## 🛡️ Security

CoreMLWin has been thoroughly security tested:

- ✅ **Red team audited** - Comprehensive penetration testing
- ✅ **All P0 critical fixes applied** - Zero known critical vulnerabilities
- ✅ **Input validation** - Protection against buffer overflows, path traversal
- ✅ **Secure hashing** - SHA-256 content-based model identification
- ✅ **35+ security tests** - Full test coverage for security features

See [docs/RED_TEAM_AUDIT.md](docs/RED_TEAM_AUDIT.md) for the full security audit report.

---

## 📚 Documentation

- **[Installation Guide](docs/INSTALLATION.md)** - Detailed installation instructions
- **[Distribution Guide](docs/DISTRIBUTION_GUIDE.md)** - How to distribute your own builds
- **[Release Checklist](RELEASE_CHECKLIST.md)** - Release automation guide
- **[Security Audit](docs/RED_TEAM_AUDIT.md)** - Security assessment report
- **[Security Improvements](docs/SECURITY_IMPROVEMENTS.md)** - Implemented security fixes

---

## 🧪 Testing

### Run All Tests

```powershell
# Start service
start build\bin\Release\coremlwin_service.exe

# Run integration tests
pytest tests/test_integration.py -v

# Run security tests
pytest tests/test_security.py -v

# Run C++ security unit tests
.\tests\test_security_utils.exe
```

### Test Coverage

- ✅ **Integration Tests** - End-to-end workflows
- ✅ **Security Tests** - Adversarial testing
- ✅ **Performance Tests** - Latency benchmarks
- ✅ **Concurrency Tests** - Thread safety
- ✅ **C++ Unit Tests** - Security utilities (35 tests)

---

## 📊 Performance

**Latency Benchmarks** (ResNet50, 224x224 input):

| Provider | Latency | Throughput |
|----------|---------|------------|
| DirectML (RTX 3080) | ~5ms | 200 FPS |
| CPU (Intel i9) | ~45ms | 22 FPS |

**Service Overhead**: <100ms health check latency

---

## 🗺️ Roadmap

### v0.2.0
- [ ] Model caching with LRU eviction
- [ ] Shared memory transport for zero-copy
- [ ] Connection pooling
- [ ] Google Test C++ unit testing framework

### v0.3.0
- [ ] WebSocket API support
- [ ] REST API endpoint
- [ ] Model versioning
- [ ] A/B testing support

### v1.0.0
- [ ] Microsoft Store distribution
- [ ] GUI management tool
- [ ] Performance profiler
- [ ] Model optimization tools

---

## 🤝 Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgments

- **[ONNX Runtime](https://onnxruntime.ai/)** - High-performance ML inference engine
- **[DirectML](https://docs.microsoft.com/en-us/windows/ai/directml/)** - GPU acceleration on Windows
- **[Protocol Buffers](https://protobuf.dev/)** - Efficient serialization
- **Microsoft** - For the winget package manager

---

## 📞 Support

- **🐛 Bug Reports**: [GitHub Issues](https://github.com/user/WinCoreML/issues)
- **💬 Discussions**: [GitHub Discussions](https://github.com/user/WinCoreML/discussions)
- **📧 Email**: Contact via GitHub profile

---

## 🌟 Star History

If you find CoreMLWin useful, please consider giving it a star ⭐

---

<p align="center">
  <strong>Made with ❤️ for the Windows ML community</strong>
</p>

<p align="center">
  <a href="https://github.com/user/WinCoreML">GitHub</a> •
  <a href="docs/INSTALLATION.md">Install</a> •
  <a href="https://github.com/user/WinCoreML/releases">Download</a> •
  <a href="https://github.com/user/WinCoreML/issues">Report Bug</a>
</p>
