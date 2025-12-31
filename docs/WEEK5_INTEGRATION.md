# Week 5: Real Integration - ONNX Runtime & DirectML

This document covers the Week 5 milestone: integrating real ONNX Runtime with DirectML support for GPU-accelerated inference on Windows.

## Overview

Week 5 transforms CoreMLWin from a prototype to a fully functional ML runtime with:

✅ **Real ONNX Runtime Integration**
- Conditional compilation support (works with or without ONNX Runtime)
- Automatic detection and linking of ONNX Runtime libraries
- Structured logging throughout inference pipeline

✅ **DirectML GPU Acceleration**
- Windows DirectML execution provider for GPU inference
- Automatic fallback to CPU if DirectML unavailable
- Provider selection and configuration

✅ **Production-Ready Inference**
- Session management and model metadata extraction
- Proper error handling and validation
- Performance optimization (warmup, batching support)

✅ **Comprehensive Testing**
- Real model inference tests
- Provider comparison benchmarks
- CLI tools for model management

## Architecture

### Conditional Compilation

The executor supports both modes:

```cpp
#ifdef CMW_HAVE_ONNXRUNTIME
    // Real ONNX Runtime implementation
    #include <onnxruntime_cxx_api.h>
    // ... actual inference code ...
#else
    // Placeholder implementation for testing
    // ... mock inference code ...
#endif
```

**Benefits:**
- Can build and test without ONNX Runtime dependency
- Automatic switching based on ONNX Runtime availability
- No runtime performance overhead

### Execution Flow

```
Client Request
    ↓
Runtime Service (Named Pipe)
    ↓
Model Registry (Session Cache)
    ↓
ONNX Executor
    ↓
┌─────────────────────────────────┐
│  ONNX Runtime Session           │
│  ┌──────────────────────────┐   │
│  │ Provider Selection       │   │
│  │ - DirectML (GPU)         │   │
│  │ - CPU (fallback)         │   │
│  └──────────────────────────┘   │
│                                  │
│  ┌──────────────────────────┐   │
│  │ Graph Optimization       │   │
│  │ - Extended optimizations │   │
│  │ - Layout transformations │   │
│  └──────────────────────────┘   │
│                                  │
│  ┌──────────────────────────┐   │
│  │ Inference Execution      │   │
│  │ - Tensor preparation     │   │
│  │ - Session.Run()          │   │
│  │ - Output extraction      │   │
│  └──────────────────────────┘   │
└─────────────────────────────────┘
    ↓
Result (via Protobuf)
    ↓
Client
```

## Implementation Details

### CMake Configuration

The build system automatically detects ONNX Runtime:

```cmake
# Set ONNXRUNTIME_DIR environment variable
if(DEFINED ENV{ONNXRUNTIME_DIR})
    set(ONNXRUNTIME_DIR $ENV{ONNXRUNTIME_DIR})
    # Verify installation exists
    if(EXISTS "${ONNXRUNTIME_DIR}/include/onnxruntime_cxx_api.h")
        set(onnxruntime_FOUND TRUE)
        # Add include directories and link libraries
        include_directories(${ONNXRUNTIME_DIR}/include)
        target_link_libraries(coremlwin_service PRIVATE ${ONNXRUNTIME_DIR}/lib/onnxruntime.lib)
        # Define CMW_HAVE_ONNXRUNTIME for conditional compilation
        add_compile_definitions(CMW_HAVE_ONNXRUNTIME)
    endif()
endif()
```

### Model Loading

**Key features:**
- Provider selection (DirectML, CPU)
- Automatic fallback if DirectML fails
- Input/output metadata extraction
- Graph optimization configuration

```cpp
// Create session with provider
if (provider_name == "DmlExecutionProvider") {
    try {
        OrtSessionOptionsAppendExecutionProvider_DML(*session_options, 0);
    } catch (...) {
        // Fallback to CPU
        LOG_WARNING << "DirectML not available, using CPU";
    }
}

// Create session
session = std::make_unique<Ort::Session>(*env, model_path, *session_options);

// Extract metadata
for (size_t i = 0; i < session->GetInputCount(); i++) {
    auto name = session->GetInputNameAllocated(i, allocator);
    auto type_info = session->GetInputTypeInfo(i);
    auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
    // Store input name, shape, dtype...
}
```

### Inference Execution

**Key features:**
- Zero-copy tensor creation where possible
- Proper memory management
- Detailed logging for debugging

```cpp
// Create input tensors
for (const auto& input_name : input_names) {
    auto tensor_value = Ort::Value::CreateTensor<float>(
        memory_info,
        data_ptr,
        num_elements,
        shape.data(),
        shape.size()
    );
    input_tensors.push_back(std::move(tensor_value));
}

// Run inference
auto output_tensors = session->Run(
    Ort::RunOptions{nullptr},
    input_names_cstr.data(),
    input_tensors.data(),
    input_tensors.size(),
    output_names_cstr.data(),
    output_names_cstr.size()
);

// Extract outputs
for (auto& ort_tensor : output_tensors) {
    float* data_ptr = ort_tensor.GetTensorMutableData<float>();
    size_t data_size = tensor_info.GetElementCount() * sizeof(float);
    // Copy to output buffer...
}
```

## Quick Start

### 1. Install ONNX Runtime

See [ONNX_RUNTIME_SETUP.md](ONNX_RUNTIME_SETUP.md) for detailed instructions.

**Quick version:**
```powershell
# Download ONNX Runtime with DirectML
# From: https://github.com/microsoft/onnxruntime/releases
# Get: onnxruntime-win-x64-gpu-1.16.3.zip

# Extract to C:\onnxruntime\
# Set environment variable
setx ONNXRUNTIME_DIR "C:\onnxruntime\onnxruntime-win-x64-gpu-1.16.3"
```

### 2. Build the Runtime

```powershell
cd WinCoreML
mkdir build
cd build

cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

**Verify ONNX Runtime is enabled:**
```
-- CoreMLWin Runtime Configuration:
--   ONNX Runtime: ENABLED
--     Location: C:\onnxruntime\onnxruntime-win-x64-gpu-1.16.3
```

### 3. Test with Sample Model

```powershell
# Terminal 1: Start service
cd build\bin\Release
.\coremlwin_service.exe

# Terminal 2: Run tests
cd ..\..\..\tests

# Download sample model
python test_real_inference.py --download-sample

# Test inference
python test_real_inference.py --model ..\models\mobilenetv2-7.onnx --provider DmlExecutionProvider
```

## Performance Characteristics

### Expected Performance

Based on MobileNetV2 (224x224 input) on typical hardware:

| Provider | Hardware | Mean Latency | Throughput |
|----------|----------|--------------|------------|
| CPU | Intel i7-10700K | ~11 ms | ~90 fps |
| DirectML | NVIDIA RTX 3060 | ~3.5 ms | ~285 fps |
| DirectML | AMD RX 6700 XT | ~4.2 ms | ~238 fps |

**Speedup:** Typically 2-4x for small models, 5-10x for large models (ResNet50, BERT)

### Optimization Tips

1. **Use warmup iterations**
   - First inference is slow due to initialization
   - Run 5-10 warmup iterations before benchmarking

2. **Batch inference**
   - DirectML excels with batch sizes > 1
   - Use dynamic batch dimension in ONNX model

3. **Model optimization**
   - Use ONNX graph optimizations
   - Quantize to FP16 or INT8 for DirectML

4. **Session pooling**
   - Reuse sessions across requests
   - Implement session pool (Week 6 feature)

## Known Limitations

### Current Limitations

1. **Single data type support**
   - Currently assumes float32 inputs/outputs
   - TODO: Add support for int64, float16, etc.

2. **No dynamic shape handling**
   - Static shapes only (no -1 dimensions in runtime)
   - TODO: Add dynamic shape support

3. **Limited provider options**
   - CPU and DirectML only
   - TODO: Add CUDA provider for NVIDIA GPUs on Linux

4. **No model optimization**
   - No automatic quantization or pruning
   - TODO: Integrate ONNX optimizer

### Future Enhancements (Week 6+)

- [ ] Session pooling for concurrent requests
- [ ] Shared memory transport for large tensors
- [ ] Multi-model batching
- [ ] Automatic model optimization
- [ ] TensorRT provider support
- [ ] Model quantization (FP16, INT8)
- [ ] Async inference API

## Troubleshooting

### Common Issues

**Issue:** Build fails with "onnxruntime_cxx_api.h not found"
```
Solution: Verify ONNXRUNTIME_DIR is set correctly:
  echo %ONNXRUNTIME_DIR%
  dir %ONNXRUNTIME_DIR%\include\onnxruntime_cxx_api.h
```

**Issue:** Runtime fails with "DirectML.dll not found"
```
Solution: Copy DirectML.dll to executable directory:
  copy %ONNXRUNTIME_DIR%\lib\DirectML.dll build\bin\Release\
```

**Issue:** DirectML provider fails to initialize
```
Solution: Check DirectML requirements:
  1. Windows 10 version 1903+ (build 18362)
     Run: winver
  2. Latest GPU drivers
  3. DirectX 12 capable GPU
```

**Issue:** Inference returns incorrect results
```
Solution: Check input preprocessing:
  1. Verify input shapes match model expectations
  2. Check normalization (e.g., [0,1] vs [-1,1])
  3. Verify channel order (RGB vs BGR)
```

### Debugging

Enable debug logging to see detailed inference information:

```powershell
# Set environment variable for debug mode
set COREMLWIN_DEV_MODE=1

# Start service
.\coremlwin_service.exe
```

**Debug output includes:**
- Model loading details
- Input/output shapes and types
- Provider initialization
- Inference timing
- Tensor data statistics

## Testing

See [TESTING.md](TESTING.md) for comprehensive testing guide.

**Quick test:**
```powershell
# Unit tests
cd tests
pytest test_unit.py

# Integration test
python test_real_inference.py --download-sample
python test_real_inference.py --model ..\models\mobilenetv2-7.onnx --compare
```

## Documentation

- [ONNX Runtime Setup](ONNX_RUNTIME_SETUP.md) - Installation and configuration
- [Testing Guide](TESTING.md) - Comprehensive testing instructions
- [API Reference](../sdk/python/README.md) - Python SDK documentation

## Next Steps

With Week 5 complete, you now have:

✅ Real ONNX inference with CPU and GPU acceleration
✅ Production-ready error handling and logging
✅ Comprehensive testing framework
✅ Model management tools

**Week 6 Preview:** Advanced features for production deployment
- Session pooling for concurrent requests
- Shared memory transport for zero-copy inference
- Performance monitoring and metrics
- Production deployment guide
- Kubernetes/Docker containerization
