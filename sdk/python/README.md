# CoreML-on-Windows Python SDK

Python client library for the CoreMLWin runtime service.

## Installation

```bash
pip install coreml-win
```

Or install from source:

```bash
cd sdk/python
pip install -e .
```

## Quick Start

```python
from coreml_win import RuntimeClient
import numpy as np

# Connect to runtime service
client = RuntimeClient()

# Check service health
health = client.health()
print(f"Service version: {health['version']}")

# Register a CoreML model
model_id = client.register_model("path/to/model.mlpackage")
print(f"Registered model: {model_id}")

# Prepare inputs
inputs = {
    "input_image": np.random.randn(1, 3, 224, 224).astype(np.float32)
}

# Run inference
outputs = client.predict(model_id, inputs)
print(f"Outputs: {outputs.keys()}")

# List all registered models
models = client.list_models()
for model in models:
    print(f"  {model['model_id']}: {model['model_format']}")

# Get provider capabilities
capabilities = client.get_capabilities()
for provider in capabilities:
    print(f"Provider: {provider['provider_name']}")
    print(f"  Device: {provider['device_name']}")
    print(f"  Compute: {provider['compute_unit']}")
```

## Error Handling

All errors are mapped to specific exception types:

```python
from coreml_win.errors import (
    ModelNotFoundError,
    ProviderNotAvailableError,
    TimeoutError,
)

try:
    outputs = client.predict(model_id, inputs)
except ModelNotFoundError as e:
    print(f"Model not found: {e}")
    print(f"Error code: {e.code}")
except ProviderNotAvailableError as e:
    print(f"No suitable provider: {e}")
    if e.is_transient:
        # Retry logic for transient errors
        pass
except Exception as e:
    print(f"Unexpected error: {e}")
```

## Configuration

The SDK reads from the same configuration as the runtime service:

- System config: `%ProgramData%\CoreMLWin\config.json`
- User config: `%LOCALAPPDATA%\CoreMLWin\config.json`

You can also specify configuration programmatically:

```python
client = RuntimeClient(
    pipe_name=r"\\.\pipe\coremlwin_runtime",
    timeout_ms=30000
)
```

## Advanced Usage

### Custom Inference Configuration

```python
from coreml_win import InferenceConfig, ComputeUnits

config = InferenceConfig(
    compute_units=ComputeUnits.CPU_AND_GPU,
    enable_profiling=True,
    timeout_ms=10000,
    provider_options={
        "directml_device_id": "0"
    }
)

outputs = client.predict(model_id, inputs, config=config)

# Check which provider was used
if outputs.debug_info:
    print(f"Provider: {outputs.debug_info['provider_used']}")
    print(f"Inference time: {outputs.debug_info['inference_time_us']}µs")
```

### Model Registration Options

```python
# Register with custom cache key
model_id = client.register_model(
    model_path="model.mlpackage",
    cache_key="my_custom_key_v1"
)

# Register from in-memory bytes
with open("model.onnx", "rb") as f:
    model_data = f.read()

model_id = client.register_model(model_data=model_data)
```

## API Reference

### RuntimeClient

**Methods:**

- `health()` - Check service health
- `register_model(model_path=None, model_data=None, cache_key=None)` - Register a model
- `unregister_model(model_id)` - Unregister a model
- `predict(model_id, inputs, config=None)` - Run inference
- `list_models()` - List all registered models
- `get_capabilities(provider_name=None)` - Get provider capabilities

### InferenceConfig

**Parameters:**

- `compute_units` - ComputeUnits enum (CPU_ONLY, CPU_AND_GPU, ALL, CPU_AND_NPU)
- `enable_profiling` - bool
- `timeout_ms` - int
- `provider_options` - dict[str, str]

### Error Classes

See `coreml_win.errors` module for complete error hierarchy.

## Development

Install development dependencies:

```bash
pip install -e ".[dev]"
```

Run tests:

```bash
pytest
```

Format code:

```bash
black coreml_win/
ruff check coreml_win/
```

Type checking:

```bash
mypy coreml_win/
```

## License

See LICENSE file in the repository root.
