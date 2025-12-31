# ONNX Runtime Integration Guide

This guide explains how to integrate ONNX Runtime with the Universal ML Runtime for actual inference execution.

## Overview

The current implementation in `runtime/execution/onnx_executor.cpp` contains placeholder code. The full ONNX Runtime integration code is present but commented out. This guide shows how to enable it.

## Step 1: Download ONNX Runtime

### Option A: Pre-built Binaries (Recommended)

```powershell
# Download latest release from GitHub
# https://github.com/microsoft/onnxruntime/releases

# For Windows x64 with DirectML support
$version = "1.17.1"
$url = "https://github.com/microsoft/onnxruntime/releases/download/v$version/onnxruntime-win-x64-gpu-$version.zip"

# Download
Invoke-WebRequest -Uri $url -OutFile onnxruntime.zip

# Extract to C:\onnxruntime
Expand-Archive -Path onnxruntime.zip -DestinationPath C:\
Rename-Item C:\onnxruntime-win-x64-gpu-$version C:\onnxruntime

# Verify installation
ls C:\onnxruntime\lib\onnxruntime.lib
ls C:\onnxruntime\lib\onnxruntime.dll
ls C:\onnxruntime\include\onnxruntime_cxx_api.h
```

### Option B: Build from Source

For custom configurations or latest features:

```powershell
# Clone ONNX Runtime
git clone --recursive https://github.com/microsoft/onnxruntime.git
cd onnxruntime

# Build with DirectML provider
.\build.bat --config Release --build_shared_lib --parallel --use_dml

# Build artifacts will be in: build\Windows\Release\Release
```

## Step 2: Update CMake Configuration

Edit `runtime/CMakeLists.txt` to configure ONNX Runtime:

```cmake
# Add after line 28 (after find_package(Protobuf))

# ONNX Runtime Configuration
set(ONNXRUNTIME_ROOT "C:/onnxruntime" CACHE PATH "ONNX Runtime installation directory")

# Find ONNX Runtime headers
find_path(ONNXRUNTIME_INCLUDE_DIR
    NAMES onnxruntime_cxx_api.h
    PATHS ${ONNXRUNTIME_ROOT}/include
    NO_DEFAULT_PATH
)

# Find ONNX Runtime library
find_library(ONNXRUNTIME_LIBRARY
    NAMES onnxruntime
    PATHS ${ONNXRUNTIME_ROOT}/lib
    NO_DEFAULT_PATH
)

if(ONNXRUNTIME_INCLUDE_DIR AND ONNXRUNTIME_LIBRARY)
    set(ONNXRUNTIME_FOUND TRUE)
    message(STATUS "Found ONNX Runtime:")
    message(STATUS "  Include: ${ONNXRUNTIME_INCLUDE_DIR}")
    message(STATUS "  Library: ${ONNXRUNTIME_LIBRARY}")
else()
    set(ONNXRUNTIME_FOUND FALSE)
    message(WARNING "ONNX Runtime not found. Inference will use placeholder.")
endif()

# Add include directories
if(ONNXRUNTIME_FOUND)
    include_directories(${ONNXRUNTIME_INCLUDE_DIR})
endif()
```

Then update the executable linking (around line 114):

```cmake
target_link_libraries(coremlwin_service
    PRIVATE
        coremlwin_core
)

# Add ONNX Runtime if found
if(ONNXRUNTIME_FOUND)
    target_link_libraries(coremlwin_service PRIVATE ${ONNXRUNTIME_LIBRARY})
    target_compile_definitions(coremlwin_service PRIVATE ONNXRUNTIME_ENABLED)
endif()

if(WIN32)
    # Windows-specific libraries
    target_link_libraries(coremlwin_service
        PRIVATE
            ws2_32
    )
endif()
```

## Step 3: Enable ONNX Runtime Code

### 3.1 Update onnx_executor.cpp Header

Replace the placeholder includes in `runtime/execution/onnx_executor.cpp`:

```cpp
// At the top of the file, uncomment:
#ifdef ONNXRUNTIME_ENABLED
#include <onnxruntime_cxx_api.h>
#endif
```

### 3.2 Update Implementation Structure

The file already contains the full implementation in comments. You need to:

1. **Uncomment the ONNXExecutorImpl structure** (lines ~30-40):
```cpp
#ifdef ONNXRUNTIME_ENABLED
struct ONNXExecutor::ONNXExecutorImpl {
    std::unique_ptr<Ort::Env> env;
    std::unique_ptr<Ort::SessionOptions> session_options;
    std::unique_ptr<Ort::Session> session;
    std::vector<std::string> input_names;
    std::vector<std::string> output_names;
    bool loaded = false;
};
#else
// Placeholder implementation
struct ONNXExecutor::ONNXExecutorImpl {
    bool loaded = false;
};
#endif
```

2. **Uncomment LoadModel implementation** (replace placeholder):
```cpp
CmwErrorCode ONNXExecutor::LoadModel(
    const std::string& model_path,
    const std::string& provider_name
) {
#ifdef ONNXRUNTIME_ENABLED
    // Initialize ONNX Runtime environment
    impl_->env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "CoreMLWinRuntime");

    // Configure session options
    impl_->session_options = std::make_unique<Ort::SessionOptions>();
    impl_->session_options->SetIntraOpNumThreads(4);
    impl_->session_options->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

    // Configure execution provider
    if (provider_name == "DmlExecutionProvider") {
        Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_DML(
            *impl_->session_options, 0  // GPU device ID
        ));
    } else if (provider_name == "CUDAExecutionProvider") {
        OrtCUDAProviderOptions cuda_options{};
        cuda_options.device_id = 0;
        impl_->session_options->AppendExecutionProvider_CUDA(cuda_options);
    }
    // CPU provider is default fallback

    // Load the model
    try {
        impl_->session = std::make_unique<Ort::Session>(
            *impl_->env,
            model_path.c_str(),
            *impl_->session_options
        );

        // Get input/output metadata
        Ort::AllocatorWithDefaultOptions allocator;

        size_t input_count = impl_->session->GetInputCount();
        for (size_t i = 0; i < input_count; i++) {
            auto name = impl_->session->GetInputNameAllocated(i, allocator);
            impl_->input_names.push_back(std::string(name.get()));
        }

        size_t output_count = impl_->session->GetOutputCount();
        for (size_t i = 0; i < output_count; i++) {
            auto name = impl_->session->GetOutputNameAllocated(i, allocator);
            impl_->output_names.push_back(std::string(name.get()));
        }

        impl_->loaded = true;
        std::cout << "ONNX model loaded successfully with " << provider_name << std::endl;
        return CMW_SUCCESS;

    } catch (const Ort::Exception& e) {
        std::cerr << "ONNX Runtime error: " << e.what() << std::endl;
        return CMW_ERROR_MODEL_LOAD_FAILED;
    }
#else
    // Placeholder when ONNX Runtime not available
    std::cout << "PLACEHOLDER: LoadModel (ONNX Runtime not linked)" << std::endl;
    impl_->loaded = true;
    return CMW_SUCCESS;
#endif
}
```

3. **Uncomment RunInference implementation** (replace placeholder):
```cpp
CmwErrorCode ONNXExecutor::RunInference(
    const std::map<std::string, TensorData>& inputs,
    std::map<std::string, TensorData>& outputs
) {
#ifdef ONNXRUNTIME_ENABLED
    if (!impl_->loaded) {
        return CMW_ERROR_MODEL_NOT_FOUND;
    }

    try {
        Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(
            OrtArenaAllocator, OrtMemTypeDefault
        );

        // Prepare input tensors
        std::vector<Ort::Value> input_tensors;
        std::vector<const char*> input_names_cstr;

        for (const auto& name : impl_->input_names) {
            auto it = inputs.find(name);
            if (it == inputs.end()) {
                std::cerr << "Missing input: " << name << std::endl;
                return CMW_ERROR_MISSING_REQUIRED_INPUT;
            }

            const TensorData& tensor = it->second;

            // Convert shape
            std::vector<int64_t> shape_int64(tensor.shape.begin(), tensor.shape.end());

            // Create ONNX tensor
            input_tensors.push_back(Ort::Value::CreateTensor<float>(
                memory_info,
                const_cast<float*>(reinterpret_cast<const float*>(tensor.data.data())),
                tensor.data.size() / sizeof(float),
                shape_int64.data(),
                shape_int64.size()
            ));

            input_names_cstr.push_back(name.c_str());
        }

        // Prepare output names
        std::vector<const char*> output_names_cstr;
        for (const auto& name : impl_->output_names) {
            output_names_cstr.push_back(name.c_str());
        }

        // Run inference
        auto output_tensors = impl_->session->Run(
            Ort::RunOptions{nullptr},
            input_names_cstr.data(),
            input_tensors.data(),
            input_tensors.size(),
            output_names_cstr.data(),
            output_names_cstr.size()
        );

        // Extract outputs
        for (size_t i = 0; i < output_tensors.size(); i++) {
            TensorData output_tensor;
            output_tensor.dtype = DTYPE_FLOAT32;  // TODO: Get actual dtype

            // Get shape
            auto tensor_info = output_tensors[i].GetTensorTypeAndShapeInfo();
            auto shape = tensor_info.GetShape();
            output_tensor.shape.assign(shape.begin(), shape.end());

            // Get data
            float* data = output_tensors[i].GetTensorMutableData<float>();
            size_t data_size = tensor_info.GetElementCount() * sizeof(float);
            output_tensor.data.assign(
                reinterpret_cast<uint8_t*>(data),
                reinterpret_cast<uint8_t*>(data) + data_size
            );

            outputs[impl_->output_names[i]] = output_tensor;
        }

        return CMW_SUCCESS;

    } catch (const Ort::Exception& e) {
        std::cerr << "ONNX Runtime inference error: " << e.what() << std::endl;
        return CMW_ERROR_PROVIDER_EXECUTION_FAILED;
    }
#else
    // Placeholder
    std::cout << "PLACEHOLDER: RunInference (ONNX Runtime not linked)" << std::endl;
    // Create dummy output
    TensorData dummy_output;
    dummy_output.dtype = DTYPE_FLOAT32;
    dummy_output.shape = {1, 1000};
    dummy_output.data.resize(1000 * sizeof(float));
    outputs["output"] = dummy_output;
    return CMW_SUCCESS;
#endif
}
```

## Step 4: Copy ONNX Runtime DLL

After building, copy the runtime DLL to your executable directory:

```powershell
# Copy DLL to build output
cp C:\onnxruntime\lib\onnxruntime.dll runtime\build\bin\Release\

# Or add to system PATH
$env:PATH += ";C:\onnxruntime\lib"
```

## Step 5: Rebuild Project

```powershell
cd runtime

# Configure with ONNX Runtime path
cmake -B build -G "Visual Studio 17 2022" -DONNXRUNTIME_ROOT=C:/onnxruntime

# Build
cmake --build build --config Release

# Verify ONNX Runtime was found (check CMake output)
# Should see: "Found ONNX Runtime:"
```

## Step 6: Test Integration

Create a simple test:

```cpp
// test_onnx.cpp
#include "execution/onnx_executor.h"
#include <iostream>

int main() {
    ONNXExecutor executor;

    // Load a simple ONNX model
    auto result = executor.LoadModel("test_model.onnx", "CPUExecutionProvider");

    if (result == CMW_SUCCESS) {
        std::cout << "✓ ONNX Runtime integration successful!" << std::endl;
    } else {
        std::cerr << "✗ ONNX Runtime integration failed" << std::endl;
        return 1;
    }

    return 0;
}
```

## Provider Configuration

### CPU Provider (Default)

Always available, no additional configuration needed.

```cpp
executor.LoadModel("model.onnx", "CPUExecutionProvider");
```

### DirectML Provider (GPU/NPU on Windows)

Included in ONNX Runtime GPU builds. Supports:
- DirectX 12 GPUs
- NPUs (Neural Processing Units)

```cpp
executor.LoadModel("model.onnx", "DmlExecutionProvider");
```

**Requirements:**
- Windows 10 version 1903+
- DirectX 12 compatible GPU
- ONNX Runtime GPU build

### CUDA Provider (NVIDIA GPUs)

Requires CUDA-enabled ONNX Runtime build.

```cpp
executor.LoadModel("model.onnx", "CUDAExecutionProvider");
```

**Requirements:**
- NVIDIA GPU with Compute Capability 6.0+
- CUDA Toolkit 11.x or 12.x
- cuDNN 8.x

## Troubleshooting

### DLL Not Found

**Error:**
```
The code execution cannot proceed because onnxruntime.dll was not found
```

**Solutions:**
```powershell
# Option 1: Copy DLL to executable directory
cp C:\onnxruntime\lib\onnxruntime.dll runtime\build\bin\Release\

# Option 2: Add to PATH
$env:PATH += ";C:\onnxruntime\lib"

# Option 3: Install DLL to system directory (requires admin)
cp C:\onnxruntime\lib\onnxruntime.dll C:\Windows\System32\
```

### Provider Not Available

**Error:**
```
ONNX Runtime error: Provider DmlExecutionProvider is not available
```

**Solution:**
Ensure you downloaded the GPU build of ONNX Runtime, not CPU-only:
- GPU build: `onnxruntime-win-x64-gpu-*.zip`
- CPU-only: `onnxruntime-win-x64-*.zip`

### Linking Errors

**Error:**
```
unresolved external symbol "Ort::SessionOptions::SetGraphOptimizationLevel"
```

**Solution:**
Ensure CMake found ONNX Runtime library:
```powershell
# Check CMake output for:
# -- Found ONNX Runtime:
# --   Include: C:/onnxruntime/include
# --   Library: C:/onnxruntime/lib/onnxruntime.lib

# If not found, specify path explicitly:
cmake -B build -DONNXRUNTIME_ROOT=C:/onnxruntime
```

### Model Load Fails

**Error:**
```
ONNX Runtime error: Invalid model file
```

**Checklist:**
1. Verify model file exists and is valid ONNX format
2. Check ONNX opset version is supported (opset 7-18 recommended)
3. Try loading with `onnxruntime.InferenceSession` in Python first

## Performance Tips

### 1. Graph Optimization

```cpp
session_options->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
```

Levels:
- `ORT_DISABLE_ALL`: No optimization (faster load, slower inference)
- `ORT_ENABLE_BASIC`: Basic optimizations
- `ORT_ENABLE_EXTENDED`: Extended optimizations
- `ORT_ENABLE_ALL`: All optimizations (recommended)

### 2. Thread Configuration

```cpp
// Intra-op parallelism (within operators)
session_options->SetIntraOpNumThreads(4);

// Inter-op parallelism (between operators)
session_options->SetInterOpNumThreads(2);
```

### 3. Memory Optimization

```cpp
// Enable memory pattern optimization
session_options->EnableMemPattern();

// Enable CPU memory arena
session_options->EnableCpuMemArena();
```

## Next Steps

After integration:
1. Implement provider benchmarking (compare CPU vs DirectML vs CUDA)
2. Add automatic provider fallback
3. Implement session pooling for concurrent requests
4. Add support for dynamic shapes
5. Implement INT8 quantization

## References

- [ONNX Runtime Documentation](https://onnxruntime.ai/docs/)
- [C++ API Reference](https://onnxruntime.ai/docs/api/c/)
- [Execution Providers](https://onnxruntime.ai/docs/execution-providers/)
- [DirectML Provider](https://onnxruntime.ai/docs/execution-providers/DirectML-ExecutionProvider.html)
