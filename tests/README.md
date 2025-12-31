# Universal ML Runtime Tests

This directory contains tests for the Universal ML Runtime.

## Integration Test

The integration test (`test_integration.py`) verifies the complete end-to-end flow:

1. Service health check
2. Model registration
3. Model listing
4. Inference execution
5. Metadata retrieval
6. Model unregistration

### Prerequisites

1. **Build the runtime service**:
   ```powershell
   cd runtime
   cmake -B build -G "Visual Studio 17 2022"
   cmake --build build --config Release
   ```

2. **Install Python SDK**:
   ```powershell
   cd sdk/python

   # Generate protobuf files
   protoc --python_out=coreml_win `
          --proto_path=../../protos `
          ../../protos/coremlwin_runtime.proto

   # Install SDK
   pip install -e .
   ```

### Running the Test

1. **Start the service** (in one terminal):
   ```powershell
   cd runtime/build/bin/Release
   .\coremlwin_service.exe
   ```

   You should see:
   ```
   ========================================
   Universal ML Runtime Service v0.1.0
   ...
   Service ready!
   ```

2. **Run the integration test** (in another terminal):
   ```powershell
   cd tests
   python test_integration.py
   ```

### Expected Output

```
============================================================
Universal ML Runtime - End-to-End Integration Test
============================================================

Creating dummy test model: tests\test_model.onnx
✓ Test model created

Connecting to runtime service...
✓ Client initialized

[1/6] Health Check
------------------------------------------------------------
✓ Service is running
  Version: 0.1.0
  Ready: True

[2/6] Model Registration
------------------------------------------------------------
Registering model: tests\test_model.onnx
✓ Model registered successfully
  Model ID: abc123def456

[3/6] List Models
------------------------------------------------------------
✓ Found 1 registered model(s)

  Model ID: abc123def456
  Format: onnx
  Inputs: ['input']
  Outputs: ['output']

[4/6] Inference Execution
------------------------------------------------------------
Creating input tensor...
Running inference on model: abc123def456
Input shapes: {'input': (1, 3, 224, 224)}
✓ Inference completed successfully
  Output 'output': shape=(1, 1000), dtype=float32

[5/6] Model Metadata Retrieval
------------------------------------------------------------
✓ Retrieved model metadata
  Format: onnx
  Input names: ['input']
  Output names: ['output']

  Benchmark Results:
    Fastest provider: DmlExecutionProvider
    Baseline latency: 45.20 ms
    Best latency: 12.50 ms
    Speedup vs CPU: 3.62x

[6/6] Model Unregistration
------------------------------------------------------------
✓ Model unregistered successfully

============================================================
TEST SUMMARY
============================================================
✓ PASS   Health Check
✓ PASS   Model Registration
✓ PASS   List Models
✓ PASS   Inference
✓ PASS   Model Metadata
✓ PASS   Unregister
------------------------------------------------------------
Results: 6/6 tests passed
============================================================
```

## Placeholder Mode

The current implementation works in **placeholder mode** when ONNX Runtime is not linked. In this mode:

- Model registration succeeds but conversion is simulated
- Inference returns random data
- All operations complete without errors

This allows testing the full IPC protocol and data flow before ONNX Runtime integration.

## With ONNX Runtime

After integrating ONNX Runtime (see [docs/ONNX_RUNTIME_INTEGRATION.md](../docs/ONNX_RUNTIME_INTEGRATION.md)):

1. Use real ONNX models instead of dummy files
2. Inference will produce actual results
3. Benchmark results will reflect real performance

Example with a real model:

```python
from coreml_win import RuntimeClient
import numpy as np

client = RuntimeClient()

# Register a real PyTorch model
model_id = client.register_model("path/to/resnet50.pt")

# Run actual inference
inputs = {"input": np.random.randn(1, 3, 224, 224).astype(np.float32)}
outputs = client.predict(model_id, inputs)

print(outputs['output'].shape)  # (1, 1000)
```

## Unit Tests (Coming Soon)

Future additions:
- `test_named_pipe.py` - IPC transport tests
- `test_model_registry.py` - Registry operations
- `test_converter.py` - Model conversion tests
- `test_executor.py` - Inference execution tests
- `test_serialization.py` - Protobuf serialization tests

## Continuous Integration

The integration test can be run in CI/CD pipelines:

```yaml
# Example GitHub Actions workflow
name: Integration Test

on: [push, pull_request]

jobs:
  test:
    runs-on: windows-latest

    steps:
      - uses: actions/checkout@v3

      - name: Setup Python
        uses: actions/setup-python@v4
        with:
          python-version: '3.10'

      - name: Build Runtime
        run: |
          cd runtime
          cmake -B build -G "Visual Studio 17 2022"
          cmake --build build --config Release

      - name: Install SDK
        run: |
          cd sdk/python
          pip install -e .

      - name: Start Service
        run: |
          Start-Process -FilePath "runtime\build\bin\Release\coremlwin_service.exe"
          Start-Sleep -Seconds 5

      - name: Run Integration Test
        run: |
          cd tests
          python test_integration.py
```

## Troubleshooting

### Test Fails with "Service not running"

**Problem:** Cannot connect to named pipe

**Solution:**
1. Verify service is running: `tasklist | findstr coremlwin_service`
2. Check pipe name matches (default: `\\.\pipe\coremlwin_runtime`)
3. Run as Administrator if needed

### Protobuf Import Error

**Problem:** `ModuleNotFoundError: No module named 'coremlwin_runtime_pb2'`

**Solution:**
```powershell
cd sdk/python
protoc --python_out=coreml_win `
       --proto_path=../../protos `
       ../../protos/coremlwin_runtime.proto
```

### Test Hangs

**Problem:** Test appears to hang during model registration

**Possible causes:**
1. Converter worker subprocess failed (check service console for errors)
2. Python not in PATH
3. Converter dependencies not installed

**Solution:**
```powershell
# Ensure Python is accessible
python --version

# Install converter dependencies
pip install torch tensorflow tf2onnx coremltools onnx
```

## Test Coverage

Current coverage:
- ✅ Named pipe communication
- ✅ Protobuf serialization/deserialization
- ✅ Model registration flow
- ✅ Inference execution
- ✅ Model listing
- ✅ Model unregistration
- ⏳ Converter subprocess (basic)
- ⏳ ONNX Runtime integration (when enabled)
- ❌ Provider benchmarking (placeholder)
- ❌ Error handling paths
- ❌ Concurrent requests
- ❌ Large tensor handling
