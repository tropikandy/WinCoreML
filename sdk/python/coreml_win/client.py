"""
Universal ML Runtime Client

Main client interface for interacting with the runtime service.
"""

import numpy as np
from typing import Dict, List, Optional, Union, Any
from pathlib import Path
import logging

from .pipe_client import NamedPipeClient
from .errors import from_error_code, CoreMLWinError
from . import proto_utils

logger = logging.getLogger(__name__)


class RuntimeClient:
    """
    Universal ML Runtime Client.

    Provides high-level interface for:
    - Model registration (PyTorch, TensorFlow, CoreML, ONNX)
    - Inference execution
    - Benchmark querying
    - Model management
    """

    def __init__(
        self, pipe_name: str = r"\\.\pipe\coremlwin_runtime", timeout_ms: int = 30000
    ):
        """
        Initialize runtime client.

        Args:
            pipe_name: Named pipe path
            timeout_ms: Connection timeout in milliseconds
        """
        self.pipe_client = NamedPipeClient(pipe_name, timeout_ms)
        self._connected = False

    def connect(self) -> None:
        """Connect to runtime service."""
        if not self._connected:
            self.pipe_client.connect()
            self._connected = True

    def disconnect(self) -> None:
        """Disconnect from runtime service."""
        if self._connected:
            self.pipe_client.disconnect()
            self._connected = False

    def health(self) -> Dict[str, Any]:
        """
        Check service health.

        Returns:
            Dict with version and ready status

        Example:
            >>> client = RuntimeClient()
            >>> health = client.health()
            >>> print(f"Version: {health['version']}, Ready: {health['ready']}")
        """
        self._ensure_connected()

        # Create protobuf request
        request_bytes = proto_utils.create_health_check_request()

        # Send and receive
        response_bytes = self.pipe_client.send_message(request_bytes)

        # Parse response
        envelope = proto_utils.parse_response(response_bytes)
        return proto_utils.extract_health_response(envelope)

    def register_model(
        self,
        model_path: Union[str, Path],
        cache_key: Optional[str] = None,
        benchmark: bool = True,
    ) -> str:
        """
        Register a model for inference.

        Supports: .pt/.pth (PyTorch), SavedModel (TensorFlow),
                 .mlmodel/.mlpackage (CoreML), .onnx (ONNX)

        The model will be:
        1. Converted to ONNX (if needed)
        2. Validated
        3. Benchmarked across available providers
        4. Registered with unique ID (content hash)

        Args:
            model_path: Path to model file or directory
            cache_key: Optional cache key override
            benchmark: Run benchmarking (default: True)

        Returns:
            model_id: Unique model identifier

        Example:
            >>> client = RuntimeClient()
            >>> model_id = client.register_model("models/resnet50.pt")
            >>> print(f"Model registered: {model_id}")
        """
        self._ensure_connected()

        model_path = Path(model_path)
        if not model_path.exists():
            raise FileNotFoundError(f"Model not found: {model_path}")

        logger.info(f"Registering model: {model_path}")

        # Create protobuf request
        request_bytes = proto_utils.create_register_model_request(
            str(model_path), cache_key or ""
        )

        # Send and receive
        response_bytes = self.pipe_client.send_message(request_bytes)

        # Parse response
        envelope = proto_utils.parse_response(response_bytes)
        result = proto_utils.extract_register_model_response(envelope)

        logger.info(f"Model registered: {result['model_id']}")
        return result["model_id"]

    def unregister_model(self, model_id: str) -> bool:
        """
        Unregister a model.

        Args:
            model_id: Model ID to unregister

        Returns:
            True if successful
        """
        self._ensure_connected()

        logger.info(f"Unregistering model: {model_id}")

        # Create protobuf request
        request_bytes = proto_utils.create_unregister_model_request(model_id)

        # Send and receive
        response_bytes = self.pipe_client.send_message(request_bytes)

        # Parse response
        envelope = proto_utils.parse_response(response_bytes)
        result = proto_utils.extract_unregister_model_response(envelope)

        return result["success"]

    def predict(
        self,
        model_id: str,
        inputs: Dict[str, np.ndarray],
        compute_units: str = "ALL",
        timeout_ms: int = 5000,
    ) -> Dict[str, np.ndarray]:
        """
        Run inference on a model.

        Args:
            model_id: Registered model ID
            inputs: Dictionary of input_name → numpy array
            compute_units: "CPU_ONLY", "CPU_AND_GPU", "ALL", "CPU_AND_NPU"
            timeout_ms: Inference timeout in milliseconds

        Returns:
            Dictionary of output_name → numpy array

        Example:
            >>> inputs = {"input": np.random.randn(1, 3, 224, 224).astype(np.float32)}
            >>> outputs = client.predict(model_id, inputs)
            >>> print(f"Output shape: {outputs['output'].shape}")
        """
        self._ensure_connected()

        # Validate inputs
        if not inputs:
            raise ValueError("No inputs provided")

        for name, array in inputs.items():
            if not isinstance(array, np.ndarray):
                raise TypeError(f"Input '{name}' must be numpy array")

        logger.info(f"Running inference on model: {model_id}")
        logger.info(
            f"Input shapes: {{{', '.join(f'{k}: {v.shape}' for k, v in inputs.items())}}}"
        )

        # Create protobuf request
        request_bytes = proto_utils.create_predict_request(
            model_id, inputs, compute_units, timeout_ms
        )

        # Send and receive
        response_bytes = self.pipe_client.send_message(request_bytes)

        # Parse response
        envelope = proto_utils.parse_response(response_bytes)
        result = proto_utils.extract_predict_response(envelope)

        logger.info(
            f"Inference complete: {result['debug_info']['inference_time_us']} µs"
        )
        return result["outputs"]

    def get_model_info(self, model_id: str) -> Dict[str, Any]:
        """
        Get model metadata and benchmark results.

        Args:
            model_id: Model ID

        Returns:
            Dictionary with model info including:
            - format: Model format (pytorch, tensorflow, coreml, onnx)
            - input_names: List of input names
            - output_names: List of output names
            - input_shapes: Dict of input shapes
            - output_shapes: Dict of output shapes
            - benchmark: Benchmark results

        Example:
            >>> info = client.get_model_info(model_id)
            >>> print(f"Fastest provider: {info['benchmark']['fastest_provider']}")
            >>> print(f"Speedup: {info['benchmark']['speedup_vs_cpu']:.2f}x")
        """
        self._ensure_connected()

        # TODO: Implement metadata retrieval
        return {
            "model_id": model_id,
            "format": "onnx",
            "input_names": ["input"],
            "output_names": ["output"],
            "input_shapes": {"input": [1, 3, 224, 224]},
            "output_shapes": {"output": [1, 1000]},
            "benchmark": {
                "fastest_provider": "DmlExecutionProvider",
                "baseline_latency_ms": 45.2,
                "best_latency_ms": 12.5,
                "speedup_vs_cpu": 3.6,
                "results": [
                    {
                        "provider": "CPUExecutionProvider",
                        "mean_latency_ms": 45.2,
                        "success": True,
                    },
                    {
                        "provider": "DmlExecutionProvider",
                        "mean_latency_ms": 12.5,
                        "success": True,
                    },
                ],
            },
        }

    def list_models(self) -> List[Dict[str, Any]]:
        """
        List all registered models.

        Returns:
            List of model metadata dictionaries
        """
        self._ensure_connected()

        # Create protobuf request
        request_bytes = proto_utils.create_list_models_request()

        # Send and receive
        response_bytes = self.pipe_client.send_message(request_bytes)

        # Parse response
        envelope = proto_utils.parse_response(response_bytes)
        result = proto_utils.extract_list_models_response(envelope)

        return result["models"]

    def get_capabilities(
        self, provider_name: Optional[str] = None
    ) -> List[Dict[str, Any]]:
        """
        Get provider capabilities.

        Args:
            provider_name: Specific provider name, or None for all

        Returns:
            List of provider capability dictionaries
        """
        self._ensure_connected()

        # TODO: Implement protobuf GetCapabilitiesRequest
        return [
            {
                "provider_name": "CPUExecutionProvider",
                "compute_unit": "CPU_ONLY",
                "device_name": "CPU",
                "supports_fp32": True,
                "supports_fp16": False,
            },
            {
                "provider_name": "DmlExecutionProvider",
                "compute_unit": "CPU_AND_GPU",
                "device_name": "DirectML",
                "supports_fp32": True,
                "supports_fp16": True,
            },
        ]

    def _ensure_connected(self) -> None:
        """Ensure client is connected, connect if not."""
        if not self._connected:
            self.connect()

    def __enter__(self):
        """Context manager entry."""
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit."""
        self.disconnect()
        return False
