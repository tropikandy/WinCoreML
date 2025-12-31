# CoreML-on-Windows Architecture

## System Overview

CoreML-on-Windows is a runtime service architecture that enables CoreML model inference on Windows through native acceleration providers.

## Core Components

### 1. Runtime Service

The central service that manages model lifecycle and inference execution.

**Responsibilities:**
- Accept IPC connections from clients
- Parse and validate Protocol Buffers messages
- Manage model registry
- Route inference requests to appropriate providers
- Cache converted models

**Implementation:**
- `runtime/service/main.cpp` - Service entry point
- `runtime/service/named_pipe_server_win32.cpp` - Named pipe IPC
- `runtime/service/runtime_state.cpp` - State management

### 2. IPC Layer

Named Pipes-based communication with length-prefixed Protocol Buffers.

**Wire Protocol:**
```
┌─────────────────────┐
│ 4 bytes: length (LE)│  Message length (little-endian)
├─────────────────────┤
│ PipeRequestEnvelope │  Serialized protobuf request
│ or                  │  or
│ PipeResponseEnvelope│  Serialized protobuf response
└─────────────────────┘
```

**Messages:**
- `HealthCheckRequest/Response`
- `RegisterModelRequest/Response`
- `PredictRequest/Response`
- `ListModelsRequest/Response`
- `GetCapabilitiesRequest/Response`

**Implementation:**
- `protos/coremlwin_runtime.proto` - Protocol definitions
- Async I/O with overlapped operations
- Multiple concurrent client support

### 3. Model Registry

Tracks registered models and their metadata.

**Storage:**
```
{
  "model_id": "sha256_hash",
  "original_path": "C:\\models\\mobilenet.mlpackage",
  "onnx_path": "C:\\cache\\sha256_hash.onnx",
  "format": "coreml",
  "inputs": ["image"],
  "outputs": ["probabilities"],
  "metadata": {...}
}
```

**Operations:**
- Register model (with conversion if needed)
- Lookup by model_id
- Unregister and cleanup
- List all models

### 4. Conversion Worker

Python subprocess that converts CoreML models to ONNX.

**Process:**
1. Runtime spawns converter worker
2. Worker loads CoreML model via coremltools
3. Convert to ONNX using `ct.convert()`
4. Validate ONNX model
5. Return ONNX path to runtime
6. Runtime caches ONNX by content hash

**Implementation:**
- `tools/converter_worker/convert.py`
- IPC via stdout/stdin or named pipe
- Timeout protection (60s default)
- Sandboxed execution

### 5. Provider System

Plugin-based acceleration backends.

**Provider Interface:**
```c
typedef struct CmwProviderVTable {
    CmwErrorCode (*get_capabilities)(...);
    CmwErrorCode (*create_session)(...);
    CmwErrorCode (*run_inference)(...);
    CmwErrorCode (*destroy_session)(...);
    CmwErrorCode (*validate_model)(...);
    CmwErrorCode (*get_memory_usage)(...);
} CmwProviderVTable;
```

**Built-in Providers:**
- **CPU Provider**: ONNX Runtime CPU execution
- **DirectML Provider**: GPU/NPU via DirectML
- **OpenVINO Provider**: Intel NPU/CPU optimization

**Provider Loading:**
1. Scan provider directories
2. Load DLLs with `LoadLibrary`
3. Resolve `CmwCreateProvider` symbol
4. Call to instantiate provider
5. Query capabilities via `get_capabilities()`
6. Register in provider registry

### 6. Policy Engine

Selects optimal provider based on configuration and model.

**Selection Logic:**
```
Input: (model, compute_units, device_state)
Output: ordered_providers[]

1. Parse compute_units preference (CPU_ONLY, CPU_AND_GPU, ALL)
2. Filter providers by compute_unit type
3. Validate model compatibility (call validate_model)
4. Sort by:
   - Provider relative_performance
   - Device availability
   - Previous success rate
5. Return ordered list for fallback chain
```

**Fallback:**
- Try providers in order
- If transient error (5000-5999), retry same provider
- If provider error (3000-3999), try next provider
- Max fallback attempts configurable

**Implementation:**
- `runtime/routing/policy_engine.cpp`
- `configs/policies/default_policy.json`

### 7. Execution Backends

ONNX Runtime integration for actual inference.

**CPU Backend:**
```cpp
// runtime/execution/onnx_executor_ort.cpp
class OnnxExecutorORT {
    Ort::Env env_;
    Ort::SessionOptions session_options_;
    std::unique_ptr<Ort::Session> session_;

    CmwErrorCode CreateSession(const char* onnx_path);
    CmwErrorCode RunInference(CmwTensor* inputs, CmwTensor* outputs);
};
```

**DirectML Backend:**
- Uses ONNX Runtime with DirectML execution provider
- GPU/NPU acceleration
- Shared with DirectX 12 applications

**OpenVINO Backend:**
- Uses OpenVINO runtime
- Optimized for Intel NPUs
- CPU fallback

## Data Flow

### Model Registration Flow

```
Client                Runtime              Converter         Cache
  │                     │                     │               │
  ├─RegisterModel──────>│                     │               │
  │                     ├─Detect Format       │               │
  │                     │  (.mlmodel/.onnx)   │               │
  │                     │                     │               │
  │                     ├─Spawn Worker───────>│               │
  │                     │                     ├─Load CoreML   │
  │                     │                     ├─Convert ONNX  │
  │                     │<──ONNX Path─────────┤               │
  │                     │                     │               │
  │                     ├─Validate ONNX       │               │
  │                     ├─Compute Hash        │               │
  │                     ├─Store────────────────────────────>│
  │                     ├─Add to Registry     │               │
  │<─Model ID───────────┤                     │               │
```

### Inference Flow

```
Client                Runtime         Policy Engine    Provider
  │                     │                   │              │
  ├─Predict(model_id)──>│                   │              │
  │                     ├─Lookup Model      │              │
  │                     ├─Select Provider──>│              │
  │                     │                   ├─Filter       │
  │                     │                   ├─Rank         │
  │                     │<─Provider List────┤              │
  │                     │                   │              │
  │                     ├─Create Session─────────────────>│
  │                     │                   │              ├─Load ONNX
  │                     │                   │              ├─Compile
  │                     │<─Session Handle──────────────────┤
  │                     │                   │              │
  │                     ├─Run Inference─────────────────>│
  │                     │                   │              ├─Execute
  │                     │<─Outputs──────────────────────────┤
  │<─Results────────────┤                   │              │
```

## Error Handling

### Error Propagation

```
Provider Error
    │
    ├─Map to CmwErrorCode (3000-3999)
    │
    ├─Return to Runtime
    │
    ├─Check if transient (5000-5999)
    │   ├─Yes: Retry same provider
    │   └─No: Try next provider (fallback)
    │
    ├─Map to Protobuf ErrorStatus
    │
    ├─Send via IPC
    │
    └─SDK raises typed exception
```

### Retry Logic

**Transient Errors (5000-5999):**
- Automatic retry with exponential backoff
- Max 3 attempts by default
- Backoff: 100ms, 500ms, 2000ms

**Provider Errors (3000-3999):**
- Immediate fallback to next provider
- No retry on same provider
- Max 2 fallback attempts

## Threading Model

### Service Thread Pool
- Main thread: Accept named pipe connections
- Worker threads: Handle client requests (pool size: 4-8)
- I/O completion threads: Async I/O callbacks

### Provider Thread Safety
- Each session is thread-safe
- Multiple concurrent inferences allowed
- Provider manages internal locking

## Memory Management

### Model Cache
- LRU eviction when cache exceeds max size
- Content-addressed storage (SHA256)
- Shared between provider sessions

### Tensor Memory
- Client provides input tensors
- Runtime allocates output tensors
- Zero-copy where possible (shared memory future)

### Provider Memory
- Providers manage device memory
- Memory usage reported via `get_memory_usage()`
- Runtime enforces per-model limits

## Security Considerations

### IPC Security
- Named pipe with restricted ACLs
- Local-only connections
- No network exposure

### Model Validation
- Parse ONNX before loading
- Validate tensor shapes/types
- Enforce size limits

### Worker Sandboxing
- Converter runs in separate process
- Limited filesystem access
- Timeout protection

## Configuration Hierarchy

```
1. System Config (%ProgramData%\CoreMLWin\config.json)
   ↓
2. User Override (%LOCALAPPDATA%\CoreMLWin\config.json)
   ↓
3. Runtime Defaults (compiled-in)
```

User settings override system settings which override defaults.

## Future Extensions

### Shared Memory (Phase 4)
- Large tensor transfer via shared memory
- Reduces IPC overhead for >1MB tensors
- `shm_win32.cpp` implementation

### gRPC Transport (Optional)
- Alternative to named pipes
- HTTP/2-based
- Better cross-platform support

### Streaming Inference (Phase 5)
- Async inference with progress events
- Batch processing
- Multi-model pipelines

## Performance Characteristics

### Latency Breakdown (Typical)

| Component              | Time     | Notes                          |
|------------------------|----------|--------------------------------|
| IPC Overhead           | 0.5-1ms  | Pipe + serialization           |
| Provider Selection     | 0.1ms    | Cached after first call        |
| Session Creation       | 50-200ms | One-time cost (cached)         |
| Inference (CPU)        | 5-100ms  | Model-dependent                |
| Inference (DirectML)   | 2-50ms   | GPU acceleration               |
| Total First Call       | 60-300ms | Includes session creation      |
| Total Cached Call      | 3-100ms  | Amortized cost                 |

### Throughput
- Single model: 10-100 inferences/sec (model-dependent)
- Concurrent models: Limited by provider capacity
- Batching: Not yet supported (Phase 5)

## Monitoring and Telemetry

### Metrics Collected
- Inference latency (p50, p95, p99)
- Provider success/failure rates
- Model cache hit rate
- Memory usage per provider
- IPC connection count

### Logging
- Structured JSON logs
- Configurable log levels (debug, info, warn, error)
- Log rotation (10MB per file, 5 files max)
- Optional console output

## Development Tools

### CLI Tools
- `coremlwin-cli register` - Register model
- `coremlwin-cli predict` - Run inference
- `coremlwin-cli list` - List models
- `coremlwin-cli capabilities` - Show providers

### GUI (Dear ImGui)
- Model registry viewer
- Real-time inference testing
- Provider status dashboard
- Log viewer
