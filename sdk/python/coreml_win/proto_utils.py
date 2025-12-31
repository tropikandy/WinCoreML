"""
Protobuf Utilities

Helper functions for working with CoreMLWin protobuf messages.

NOTE: This module requires generated protobuf files from:
    protoc --python_out=. --proto_path=../../../protos ../../../protos/coremlwin_runtime.proto

The generated module will be: coreml_win.coremlwin_runtime_pb2
"""

import numpy as np
from typing import Dict, Any
import uuid

# Import will work after protobuf generation
try:
    from . import coremlwin_runtime_pb2 as pb
except ImportError:
    # Placeholder for when protobuf files aren't generated yet
    pb = None
    import warnings
    warnings.warn(
        "Protobuf files not generated. Run: "
        "protoc --python_out=sdk/python/coreml_win --proto_path=protos protos/coremlwin_runtime.proto"
    )


# NumPy dtype to protobuf DataType mapping
DTYPE_TO_PROTO = {
    np.float32: 'DTYPE_FLOAT32',
    np.float16: 'DTYPE_FLOAT16',
    np.int32: 'DTYPE_INT32',
    np.int64: 'DTYPE_INT64',
    np.int8: 'DTYPE_INT8',
    np.uint8: 'DTYPE_UINT8',
    np.bool_: 'DTYPE_BOOL',
}

PROTO_TO_DTYPE = {
    'DTYPE_FLOAT32': np.float32,
    'DTYPE_FLOAT16': np.float16,
    'DTYPE_INT32': np.int32,
    'DTYPE_INT64': np.int64,
    'DTYPE_INT8': np.int8,
    'DTYPE_UINT8': np.uint8,
    'DTYPE_BOOL': np.bool_,
}


def numpy_to_tensor(name: str, array: np.ndarray):
    """Convert numpy array to protobuf Tensor."""
    if pb is None:
        raise RuntimeError("Protobuf files not generated")

    tensor = pb.Tensor()
    tensor.name = name

    # Set dtype
    dtype_key = type(array.flat[0])
    tensor.dtype = getattr(pb, DTYPE_TO_PROTO.get(dtype_key, 'DTYPE_FLOAT32'))

    # Set shape
    tensor.shape.extend(array.shape)

    # Set data (as bytes)
    tensor.data = array.tobytes()

    return tensor


def tensor_to_numpy(tensor) -> np.ndarray:
    """Convert protobuf Tensor to numpy array."""
    if pb is None:
        raise RuntimeError("Protobuf files not generated")

    # Get dtype
    dtype_name = pb.DataType.Name(tensor.dtype)
    numpy_dtype = PROTO_TO_DTYPE.get(dtype_name, np.float32)

    # Reconstruct array from bytes
    array = np.frombuffer(tensor.data, dtype=numpy_dtype)

    # Reshape
    if len(tensor.shape) > 0:
        array = array.reshape(list(tensor.shape))

    return array


def create_health_check_request() -> bytes:
    """Create HealthCheck request."""
    if pb is None:
        raise RuntimeError("Protobuf files not generated")

    envelope = pb.PipeRequestEnvelope()
    envelope.request_id = str(uuid.uuid4())
    envelope.health_check.CopyFrom(pb.HealthCheckRequest())

    return envelope.SerializeToString()


def create_register_model_request(
    model_path: str,
    cache_key: str = ""
) -> bytes:
    """Create RegisterModel request."""
    if pb is None:
        raise RuntimeError("Protobuf files not generated")

    envelope = pb.PipeRequestEnvelope()
    envelope.request_id = str(uuid.uuid4())

    req = envelope.register_model
    req.model_path = model_path
    if cache_key:
        req.cache_key = cache_key

    return envelope.SerializeToString()


def create_predict_request(
    model_id: str,
    inputs: Dict[str, np.ndarray],
    compute_units: str = "ALL",
    timeout_ms: int = 5000
) -> bytes:
    """Create Predict request."""
    if pb is None:
        raise RuntimeError("Protobuf files not generated")

    envelope = pb.PipeRequestEnvelope()
    envelope.request_id = str(uuid.uuid4())

    req = envelope.predict
    req.model_id = model_id

    # Add input tensors
    for name, array in inputs.items():
        tensor = numpy_to_tensor(name, array)
        req.inputs.append(tensor)

    # Set config
    req.config.compute_units = getattr(pb, compute_units, pb.ALL)
    req.config.timeout_ms = timeout_ms

    return envelope.SerializeToString()


def create_list_models_request() -> bytes:
    """Create ListModels request."""
    if pb is None:
        raise RuntimeError("Protobuf files not generated")

    envelope = pb.PipeRequestEnvelope()
    envelope.request_id = str(uuid.uuid4())
    envelope.list_models.CopyFrom(pb.ListModelsRequest())

    return envelope.SerializeToString()


def create_unregister_model_request(model_id: str) -> bytes:
    """Create UnregisterModel request."""
    if pb is None:
        raise RuntimeError("Protobuf files not generated")

    envelope = pb.PipeRequestEnvelope()
    envelope.request_id = str(uuid.uuid4())

    req = envelope.unregister_model
    req.model_id = model_id

    return envelope.SerializeToString()


def parse_response(response_bytes: bytes) -> Any:
    """Parse protobuf response envelope."""
    if pb is None:
        raise RuntimeError("Protobuf files not generated")

    envelope = pb.PipeResponseEnvelope()
    envelope.ParseFromString(response_bytes)

    # Check status
    if envelope.status.code != 0:  # CMW_SUCCESS
        from .errors import from_error_code
        error = from_error_code(envelope.status.code)
        raise error(envelope.status.message)

    return envelope


def extract_health_response(envelope) -> Dict[str, Any]:
    """Extract HealthCheck response."""
    resp = envelope.health_check
    return {
        "version": resp.version,
        "ready": resp.ready
    }


def extract_register_model_response(envelope) -> Dict[str, Any]:
    """Extract RegisterModel response."""
    resp = envelope.register_model
    metadata = resp.metadata

    return {
        "model_id": resp.model_id,
        "metadata": {
            "model_id": metadata.model_id,
            "model_format": metadata.model_format,
            "input_names": list(metadata.input_names),
            "output_names": list(metadata.output_names),
        }
    }


def extract_predict_response(envelope) -> Dict[str, Any]:
    """Extract Predict response."""
    resp = envelope.predict

    # Convert output tensors
    outputs = {}
    for tensor in resp.outputs:
        outputs[tensor.name] = tensor_to_numpy(tensor)

    # Extract debug info
    debug_info = {
        "provider_used": resp.debug_info.provider_used,
        "inference_time_us": resp.debug_info.inference_time_us,
    }

    return {
        "outputs": outputs,
        "debug_info": debug_info
    }


def extract_list_models_response(envelope) -> Dict[str, Any]:
    """Extract ListModels response."""
    resp = envelope.list_models

    models = []
    for metadata in resp.models:
        models.append({
            "model_id": metadata.model_id,
            "model_format": metadata.model_format,
            "input_names": list(metadata.input_names),
            "output_names": list(metadata.output_names),
        })

    return {"models": models}


def extract_unregister_model_response(envelope) -> Dict[str, Any]:
    """Extract UnregisterModel response."""
    resp = envelope.unregister_model
    return {"success": resp.success}
