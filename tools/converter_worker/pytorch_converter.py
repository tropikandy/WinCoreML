"""
PyTorch to ONNX Converter

Converts PyTorch models (.pt, .pth) to ONNX format.
"""

from pathlib import Path
from typing import List, Optional, Dict, Any
import logging

from .base_converter import BaseConverter, ConversionResult, ModelInfo


logger = logging.getLogger(__name__)


class PyTorchConverter(BaseConverter):
    """
    Converter for PyTorch models.

    Supports:
    - TorchScript models (.pt, .pth)
    - Regular PyTorch models (requires example inputs)
    """

    @property
    def name(self) -> str:
        return "PyTorch"

    @property
    def supported_extensions(self) -> List[str]:
        return [".pt", ".pth", ".pytorch"]

    def can_convert(self, model_path: Path) -> bool:
        """Check if file is a PyTorch model."""
        if not model_path.exists():
            return False

        if model_path.suffix.lower() not in self.supported_extensions:
            return False

        # Try to load as PyTorch model
        try:
            import torch
            # Just check if we can load it
            _ = torch.load(str(model_path), map_location="cpu")
            return True
        except Exception:
            return False

    def convert_to_onnx(
        self,
        model_path: Path,
        output_path: Path,
        opset_version: int = 14,
        **kwargs
    ) -> ConversionResult:
        """
        Convert PyTorch model to ONNX.

        Kwargs:
            input_shapes: Dict[str, List[int]] - Required for non-TorchScript models
            dynamic_axes: Dict[str, Dict[int, str]] - Dynamic dimension names
            verbose: bool - Verbose ONNX export
        """
        try:
            import torch
            import torch.onnx

            warnings = []

            # Load model
            model = torch.load(str(model_path), map_location="cpu")

            # Check if TorchScript
            is_torchscript = isinstance(model, torch.jit.ScriptModule)

            if is_torchscript:
                model.eval()

                # For TorchScript, we need to infer input shapes
                # Try to get from model if available
                input_shapes = kwargs.get("input_shapes")
                if input_shapes is None:
                    warnings.append(
                        "No input_shapes provided, using default (1, 3, 224, 224)"
                    )
                    input_shapes = {"input": [1, 3, 224, 224]}

                # Create dummy inputs
                dummy_inputs = {
                    name: torch.randn(*shape)
                    for name, shape in input_shapes.items()
                }

                # For TorchScript, input is usually a tuple
                if len(dummy_inputs) == 1:
                    dummy_input = list(dummy_inputs.values())[0]
                else:
                    dummy_input = tuple(dummy_inputs.values())

                # Export
                torch.onnx.export(
                    model,
                    dummy_input,
                    str(output_path),
                    opset_version=opset_version,
                    input_names=list(input_shapes.keys()),
                    dynamic_axes=kwargs.get("dynamic_axes"),
                    verbose=kwargs.get("verbose", False),
                )

                input_names = list(input_shapes.keys())
                output_names = ["output"]  # Default, hard to infer

            else:
                # Regular PyTorch model - needs to be a nn.Module
                if not isinstance(model, torch.nn.Module):
                    return ConversionResult(
                        success=False,
                        onnx_path=None,
                        model_info=None,
                        error_message="Model is not a nn.Module or TorchScript",
                        warnings=[]
                    )

                model.eval()

                # Must provide input_shapes
                input_shapes = kwargs.get("input_shapes")
                if input_shapes is None:
                    return ConversionResult(
                        success=False,
                        onnx_path=None,
                        model_info=None,
                        error_message="input_shapes required for nn.Module export",
                        warnings=[]
                    )

                # Create dummy inputs
                dummy_inputs = {
                    name: torch.randn(*shape)
                    for name, shape in input_shapes.items()
                }

                if len(dummy_inputs) == 1:
                    dummy_input = list(dummy_inputs.values())[0]
                else:
                    dummy_input = tuple(dummy_inputs.values())

                # Export
                torch.onnx.export(
                    model,
                    dummy_input,
                    str(output_path),
                    opset_version=opset_version,
                    input_names=list(input_shapes.keys()),
                    dynamic_axes=kwargs.get("dynamic_axes"),
                    verbose=kwargs.get("verbose", False),
                )

                input_names = list(input_shapes.keys())
                output_names = ["output"]

            # Validate ONNX
            is_valid, error = self.validate_onnx(output_path)
            if not is_valid:
                return ConversionResult(
                    success=False,
                    onnx_path=None,
                    model_info=None,
                    error_message=error,
                    warnings=warnings
                )

            # Extract metadata from ONNX
            model_info = self._extract_onnx_metadata(
                output_path,
                model_path,
                input_names,
                output_names
            )

            return ConversionResult(
                success=True,
                onnx_path=output_path,
                model_info=model_info,
                error_message=None,
                warnings=warnings
            )

        except Exception as e:
            logger.exception("PyTorch conversion failed")
            return ConversionResult(
                success=False,
                onnx_path=None,
                model_info=None,
                error_message=f"Conversion failed: {str(e)}",
                warnings=[]
            )

    def extract_metadata(self, model_path: Path) -> Optional[ModelInfo]:
        """Extract metadata from PyTorch model."""
        try:
            import torch

            model = torch.load(str(model_path), map_location="cpu")

            # Limited metadata available without conversion
            return ModelInfo(
                model_path=model_path,
                model_format="pytorch",
                input_names=["input"],  # Default
                output_names=["output"],
                input_shapes={"input": []},  # Unknown
                output_shapes={"output": []},
                metadata={
                    "is_torchscript": isinstance(model, torch.jit.ScriptModule)
                }
            )
        except Exception as e:
            logger.error(f"Failed to extract PyTorch metadata: {e}")
            return None

    def _extract_onnx_metadata(
        self,
        onnx_path: Path,
        original_path: Path,
        input_names: List[str],
        output_names: List[str]
    ) -> ModelInfo:
        """Extract metadata from converted ONNX model."""
        import onnx

        model = onnx.load(str(onnx_path))

        # Get input shapes
        input_shapes = {}
        for input_tensor in model.graph.input:
            if input_tensor.name in input_names:
                shape = [
                    dim.dim_value if dim.dim_value > 0 else -1
                    for dim in input_tensor.type.tensor_type.shape.dim
                ]
                input_shapes[input_tensor.name] = shape

        # Get output shapes
        output_shapes = {}
        for output_tensor in model.graph.output:
            if output_tensor.name in output_names:
                shape = [
                    dim.dim_value if dim.dim_value > 0 else -1
                    for dim in output_tensor.type.tensor_type.shape.dim
                ]
                output_shapes[output_tensor.name] = shape

        return ModelInfo(
            model_path=original_path,
            model_format="pytorch",
            input_names=input_names,
            output_names=output_names,
            input_shapes=input_shapes,
            output_shapes=output_shapes,
            metadata={
                "onnx_opset": model.opset_import[0].version if model.opset_import else None
            }
        )
