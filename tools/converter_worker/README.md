# Universal Model Converter Worker

Pluggable converter system for transforming ML models from various frameworks to ONNX format.

## Supported Formats

| Format | Extensions | Converter | Dependencies |
|--------|-----------|-----------|--------------|
| PyTorch | `.pt`, `.pth` | `PyTorchConverter` | torch, torch.onnx |
| TensorFlow | `SavedModel`, `.h5`, `.keras` | `TensorFlowConverter` | tensorflow, tf2onnx |
| CoreML | `.mlmodel`, `.mlpackage` | `CoreMLConverter` | coremltools |
| ONNX | `.onnx` | `ONNXConverter` | onnx (pass-through) |

## Architecture

### Converter Registry

The `ConverterRegistry` manages all converters and routes conversion requests:

```python
from tools.converter_worker import ConverterRegistry, PyTorchConverter, TensorFlowConverter

# Create registry
registry = ConverterRegistry()

# Register converters
registry.register(PyTorchConverter())
registry.register(TensorFlowConverter())

# Convert model (auto-detects format)
result = registry.convert(
    model_path=Path("model.pt"),
    output_path=Path("model.onnx"),
    opset_version=14
)

if result.success:
    print(f"Converted to {result.onnx_path}")
    print(f"Inputs: {result.model_info.input_names}")
    print(f"Outputs: {result.model_info.output_names}")
```

### Base Converter Interface

All converters implement the `BaseConverter` abstract class:

```python
class BaseConverter(ABC):
    @abstractmethod
    def can_convert(self, model_path: Path) -> bool:
        """Check if this converter can handle the model"""
        pass

    @abstractmethod
    def convert_to_onnx(
        self,
        model_path: Path,
        output_path: Path,
        opset_version: int = 14,
        **kwargs
    ) -> ConversionResult:
        """Convert model to ONNX"""
        pass

    @abstractmethod
    def extract_metadata(self, model_path: Path) -> Optional[ModelInfo]:
        """Extract metadata without full conversion"""
        pass
```

### Conversion Flow

```
1. Registry receives model path
2. Try extension-based lookup (.pt → PyTorchConverter)
3. If no match, try all converters via can_convert()
4. Selected converter:
   a. Load model
   b. Convert to ONNX
   c. Validate ONNX (onnx.checker)
   d. Extract metadata
5. Return ConversionResult with:
   - success status
   - onnx_path
   - model_info (inputs, outputs, shapes)
   - warnings
```

## Benchmarking

The `ModelBenchmarker` automatically measures inference performance after conversion:

```python
from tools.converter_worker import ModelBenchmarker

benchmarker = ModelBenchmarker(
    warmup_iterations=5,
    benchmark_iterations=50
)

summary = benchmarker.benchmark_model(
    onnx_path=Path("model.onnx"),
    providers=["CPUExecutionProvider", "DmlExecutionProvider"]
)

print(f"Fastest: {summary.fastest_provider}")
print(f"Speedup vs CPU: {summary.speedup_vs_cpu:.2f}x")

for result in summary.results:
    if result.success:
        print(f"{result.provider_name}: {result.mean_latency_ms:.2f}ms")
```

### Benchmark Output

```
Benchmark Summary: resnet50.onnx
============================================================

CPUExecutionProvider:
  Mean Latency:  45.23 ± 2.34 ms
  Min/Max:       42.10 / 52.31 ms
  Throughput:    22.10 inferences/sec

DmlExecutionProvider:
  Mean Latency:  12.45 ± 0.78 ms
  Min/Max:       11.32 / 14.67 ms
  Throughput:    80.32 inferences/sec

============================================================
Fastest Provider: DmlExecutionProvider
Best Latency:     12.45 ms
Speedup vs CPU:   3.63x
```

## Worker Process Interface

The worker runs as a subprocess, communicating via JSON over stdin/stdout:

### Convert Model

**Request:**
```json
{
  "command": "convert",
  "model_path": "models/resnet50.pt",
  "output_path": "cache/resnet50.onnx",
  "benchmark": true,
  "opset_version": 14,
  "options": {
    "input_shapes": {"input": [1, 3, 224, 224]}
  }
}
```

**Response:**
```json
{
  "success": true,
  "onnx_path": "cache/resnet50.onnx",
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
    "baseline_latency_ms": 45.23,
    "best_latency_ms": 12.45,
    "speedup_vs_cpu": 3.63,
    "results": [...]
  }
}
```

### Extract Metadata

**Request:**
```json
{
  "command": "metadata",
  "model_path": "models/classifier.onnx"
}
```

**Response:**
```json
{
  "success": true,
  "metadata": {
    "format": "onnx",
    "input_names": ["input"],
    "output_names": ["probabilities"],
    "input_shapes": {"input": [1, 3, 224, 224]},
    "output_shapes": {"probabilities": [1, 1000]}
  }
}
```

### List Supported Formats

**Request:**
```json
{
  "command": "list_formats"
}
```

**Response:**
```json
{
  "success": true,
  "formats": {
    "PyTorch": [".pt", ".pth", ".pytorch"],
    "TensorFlow": [".savedmodel", ".h5", ".keras"],
    "CoreML": [".mlmodel", ".mlpackage"],
    "ONNX": [".onnx"]
  }
}
```

## Usage Examples

### Standalone Converter

```python
from pathlib import Path
from tools.converter_worker import ConverterWorker

worker = ConverterWorker()

# Convert PyTorch model
result = worker.convert_model(
    model_path="models/resnet50.pt",
    output_path="cache/resnet50.onnx",
    benchmark=True,
    input_shapes={"input": [1, 3, 224, 224]}
)

print(f"Success: {result['success']}")
if result['success']:
    print(f"ONNX saved to: {result['onnx_path']}")
    print(f"Fastest provider: {result['benchmark']['fastest_provider']}")
    print(f"Speedup: {result['benchmark']['speedup_vs_cpu']:.2f}x")
```

### Command-Line Usage

```bash
# Convert model
python -m tools.converter_worker.worker <<EOF
{
  "command": "convert",
  "model_path": "model.pt",
  "output_path": "model.onnx",
  "benchmark": true
}
EOF

# Extract metadata only
python -m tools.converter_worker.worker <<EOF
{
  "command": "metadata",
  "model_path": "model.onnx"
}
EOF
```

## Installation

```bash
cd tools/converter_worker
pip install -r requirements.txt

# Or install specific framework support
pip install torch  # PyTorch
pip install tensorflow tf2onnx  # TensorFlow
pip install coremltools  # CoreML
```

## Adding Custom Converters

To add support for a new model format:

1. Create converter class inheriting from `BaseConverter`
2. Implement required methods: `can_convert`, `convert_to_onnx`, `extract_metadata`
3. Register in `ConverterWorker._register_converters()`

Example:

```python
from .base_converter import BaseConverter, ConversionResult, ModelInfo

class MyFormatConverter(BaseConverter):
    @property
    def name(self) -> str:
        return "MyFormat"

    @property
    def supported_extensions(self) -> List[str]:
        return [".myformat"]

    def can_convert(self, model_path: Path) -> bool:
        return model_path.suffix.lower() == ".myformat"

    def convert_to_onnx(self, model_path, output_path, **kwargs):
        # 1. Load model
        # 2. Convert to ONNX
        # 3. Validate
        # 4. Return ConversionResult
        pass

    def extract_metadata(self, model_path):
        # Extract input/output info
        pass
```

## Error Handling

Converters return structured errors via `ConversionResult`:

```python
result = converter.convert_to_onnx(...)

if not result.success:
    print(f"Error: {result.error_message}")
    for warning in result.warnings:
        print(f"Warning: {warning}")
else:
    print(f"Success: {result.onnx_path}")
```

Common error codes:
- `2006`: Model conversion failed
- `2002`: Model parse failed
- `2007`: Model corrupted

## Performance Considerations

- **Conversion**: Can take 10-60 seconds depending on model size
- **Benchmarking**: Adds ~5-10 seconds (configurable iterations)
- **Caching**: Runtime caches converted ONNX models by content hash
- **Parallel**: Worker runs in separate process to avoid blocking runtime

## Future Enhancements

- [ ] Quantization support (INT8 for NPU)
- [ ] Model optimization passes (constant folding, operator fusion)
- [ ] Streaming conversion progress
- [ ] ONNX simplifier integration
- [ ] Multi-model batch conversion
