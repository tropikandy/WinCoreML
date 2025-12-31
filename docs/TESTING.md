# Testing Guide

Comprehensive testing guide for CoreMLWin Universal ML Runtime.

## Prerequisites

### 1. ONNX Runtime Setup

Follow [ONNX_RUNTIME_SETUP.md](ONNX_RUNTIME_SETUP.md) to install and configure ONNX Runtime with DirectML support.

**Quick check:**
```powershell
# Verify ONNXRUNTIME_DIR is set
echo %ONNXRUNTIME_DIR%

# Should output something like:
# C:\onnxruntime\onnxruntime-win-x64-gpu-1.16.3
```

### 2. Build the Runtime

```powershell
cd WinCoreML
mkdir build
cd build

# Configure with CMake
cmake -G "Visual Studio 17 2022" -A x64 ..

# Build
cmake --build . --config Release

# Verify ONNX Runtime is linked
# Check build output for:
#   ONNX Runtime: ENABLED
#   Location: C:\onnxruntime\...
```

### 3. Install Python SDK

```powershell
cd sdk\python
pip install -e .
```

## Running Tests

### Unit Tests

Python SDK unit tests (no runtime service required):

```powershell
cd tests
pytest test_unit.py -v
```

**Expected output:**
```
test_unit.py::TestErrorTaxonomy::test_error_ranges PASSED
test_unit.py::TestErrorTaxonomy::test_error_properties PASSED
test_unit.py::TestProtocolUtils::test_dtype_mapping PASSED
...
==================== 15 passed in 0.5s ====================
```

### Integration Tests

Test with the runtime service:

#### 1. Start the Runtime Service

```powershell
# Terminal 1: Start service
cd build\bin\Release
.\coremlwin_service.exe

# Expected output:
# [2024-01-15 10:30:45.123] [12345] INFO main.cpp:45 - Universal ML Runtime Service starting
# [2024-01-15 10:30:45.234] [12345] INFO main.cpp:67 - ONNX Runtime: ENABLED
# [2024-01-15 10:30:45.345] [12345] INFO main.cpp:89 - Waiting for connections...
```

#### 2. Run Integration Tests

```powershell
# Terminal 2: Run tests
cd tests
python test_integration.py
```

### Real Inference Tests

Test with actual ONNX models:

#### Option 1: Download Sample Model

```powershell
cd tests
python test_real_inference.py --download-sample
```

This downloads MobileNetV2 from the ONNX Model Zoo (~14 MB).

#### Option 2: Use Your Own Model

```powershell
# Test with CPU provider
python test_real_inference.py --model path\to\model.onnx --provider CPUExecutionProvider

# Test with DirectML (GPU) provider
python test_real_inference.py --model path\to\model.onnx --provider DmlExecutionProvider
```

#### Option 3: Compare Providers

```powershell
# Compare CPU vs DirectML performance
python test_real_inference.py --model path\to\model.onnx --compare
```

**Expected output:**
```
======================================================================
Provider Comparison: mobilenetv2-7.onnx
======================================================================

Testing CPUExecutionProvider...
----------------------------------------------------------------------
[1/4] Registering model...
  ✓ Model ID: abc123...
  ✓ Registration time: 145.23 ms

[2/4] Getting model info...
  Model path: C:\models\mobilenetv2-7.onnx
  Provider: CPUExecutionProvider
  Input shapes: {'input': [1, 3, 224, 224]}
  Output shapes: {'output': [1, 1000]}

[3/4] Preparing inputs...
  Input 'input': shape=(1, 3, 224, 224), dtype=float32

[4/4] Running inference...
  ✓ Inference time: 12.34 ms
  Output 'output': shape=(1, 1000), dtype=float32

======================================================================
Performance Benchmark
======================================================================
[1/2] Warmup (5 iterations)...
  ✓ Warmup complete

[2/2] Benchmark (30 iterations)...
  Completed 10/30 iterations
  Completed 20/30 iterations
  Completed 30/30 iterations

Performance Statistics:
  Mean:   11.23 ms
  Median: 11.15 ms
  P95:    12.45 ms

Testing DmlExecutionProvider...
----------------------------------------------------------------------
[Similar output with DirectML timings]

======================================================================
Comparison Summary
======================================================================
CPUExecutionProvider:
  Mean:   11.23 ms
  Median: 11.15 ms
  P95:    12.45 ms

DmlExecutionProvider:
  Mean:   3.45 ms
  Median: 3.42 ms
  P95:    3.89 ms

DirectML Speedup: 3.25x

======================================================================
```

### Benchmark Suite

Run comprehensive benchmarks:

```powershell
# Full benchmark with warmup
python test_real_inference.py --model path\to\model.onnx --benchmark --warmup 10 --iters 100
```

## Using the CLI Tool

The `coremlwin_cli.py` provides convenient model management:

```powershell
# Ensure service is running, then:
cd tools

# Register a model
python coremlwin_cli.py register path\to\model.onnx

# List all models
python coremlwin_cli.py list

# Get model info
python coremlwin_cli.py info <model-id>

# Check service health
python coremlwin_cli.py health

# Monitor service (auto-refresh)
python coremlwin_cli.py monitor
```

## Sample Models

### ONNX Model Zoo

Download pre-trained models from the [ONNX Model Zoo](https://github.com/onnx/models):

**Image Classification:**
- MobileNetV2: Small, fast (~14 MB)
- ResNet50: Medium size (~100 MB)
- EfficientNet: Efficient architecture

**Object Detection:**
- YOLOv3: Real-time detection
- SSD: Single shot detector

**Download example:**
```powershell
# Download MobileNetV2
$url = "https://github.com/onnx/models/raw/main/vision/classification/mobilenet/model/mobilenetv2-7.onnx"
Invoke-WebRequest -Uri $url -OutFile models\mobilenetv2-7.onnx
```

### Converting Models to ONNX

If you have models in other formats:

**PyTorch:**
```python
import torch
import torch.onnx

# Load your model
model = YourModel()
model.eval()

# Create dummy input
dummy_input = torch.randn(1, 3, 224, 224)

# Export to ONNX
torch.onnx.export(
    model,
    dummy_input,
    "model.onnx",
    input_names=['input'],
    output_names=['output'],
    dynamic_axes={'input': {0: 'batch_size'}, 'output': {0: 'batch_size'}}
)
```

**TensorFlow:**
```python
import tensorflow as tf
import tf2onnx

# Load your model
model = tf.keras.models.load_model('model.h5')

# Convert to ONNX
spec = (tf.TensorSpec((None, 224, 224, 3), tf.float32, name="input"),)
output_path = "model.onnx"

model_proto, _ = tf2onnx.convert.from_keras(model, input_signature=spec, output_path=output_path)
```

## Troubleshooting

### Service Won't Start

**Error:** `Failed to create named pipe`
- **Solution:** Check if another instance is running. Kill with Task Manager.

**Error:** `ONNX Runtime DLL not found`
- **Solution:** Copy DLLs to executable directory:
  ```powershell
  copy %ONNXRUNTIME_DIR%\lib\*.dll build\bin\Release\
  ```

### DirectML Not Available

**Error:** `Failed to enable DirectML`

Check requirements:
1. Windows 10 version 1903+ (build 18362 or newer):
   ```powershell
   winver
   ```

2. Latest GPU drivers:
   - NVIDIA: GeForce Game Ready Driver
   - AMD: Adrenalin Software
   - Intel: Graphics Driver

3. Verify DirectML.dll exists:
   ```powershell
   dir %ONNXRUNTIME_DIR%\lib\DirectML.dll
   ```

### Inference Errors

**Error:** `Missing required input`
- **Solution:** Check model input names with:
  ```python
  python coremlwin_cli.py info <model-id>
  ```

**Error:** `Shape mismatch`
- **Solution:** Verify input shapes match model expectations. Dynamic dimensions (-1) should be replaced with actual batch size.

### Performance Issues

**Slow inference:**
1. Ensure you're running Release build (not Debug)
2. Check DirectML is actually being used:
   - Look for log message: "DirectML provider enabled"
3. Run warmup iterations before benchmarking
4. Close other GPU-intensive applications

**CPU faster than DirectML:**
- Common for very small models (<10ms inference time)
- GPU overhead dominates for small workloads
- DirectML excels with larger models (ResNet50, BERT, etc.)

## Continuous Integration

For automated testing:

```powershell
# Run all tests
cd tests

# Unit tests (no runtime required)
pytest test_unit.py -v --junit-xml=results_unit.xml

# Integration tests (requires runtime)
python test_integration.py --output results_integration.json

# Smoke test with sample model
python test_real_inference.py --download-sample
python test_real_inference.py --model ..\models\mobilenetv2-7.onnx
```

## Next Steps

- [Performance Optimization](PERFORMANCE.md) - Tuning for maximum throughput
- [Production Deployment](DEPLOYMENT.md) - Running in production environments
- [API Reference](API.md) - Complete SDK documentation
