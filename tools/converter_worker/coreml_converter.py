"""
CoreML to ONNX Converter

Converts CoreML models (.mlmodel, .mlpackage) to ONNX format using coremltools.
"""

from pathlib import Path
from typing import List, Optional, Dict, Any
import logging

from .base_converter import BaseConverter, ConversionResult, ModelInfo


logger = logging.getLogger(__name__)


class CoreMLConverter(BaseConverter):
    """
    Converter for CoreML models.

    Supports:
    - .mlmodel files (CoreML models)
    - .mlpackage directories (CoreML packages)
    """

    @property
    def name(self) -> str:
        return "CoreML"

    @property
    def supported_extensions(self) -> List[str]:
        return [".mlmodel", ".mlpackage"]

    def can_convert(self, model_path: Path) -> bool:
        """Check if path is a CoreML model."""
        if not model_path.exists():
            return False

        # .mlmodel file
        if model_path.suffix.lower() == ".mlmodel":
            return True

        # .mlpackage directory
        if model_path.is_dir() and model_path.suffix.lower() == ".mlpackage":
            # Check for Data directory or model file
            if (model_path / "Data").exists():
                return True

        return False

    def convert_to_onnx(
        self,
        model_path: Path,
        output_path: Path,
        opset_version: int = 14,
        **kwargs
    ) -> ConversionResult:
        """
        Convert CoreML model to ONNX.

        Kwargs:
            minimum_deployment_target: str - iOS deployment target (e.g., "iOS13")
        """
        try:
            import coremltools as ct

            warnings = []

            # Load CoreML model
            logger.info(f"Loading CoreML model from {model_path}")
            mlmodel = ct.models.MLModel(str(model_path))

            # Get model spec
            spec = mlmodel.get_spec()

            # Extract input/output info
            input_names = [inp.name for inp in spec.description.input]
            output_names = [out.name for out in spec.description.output]

            logger.info(f"CoreML model inputs: {input_names}")
            logger.info(f"CoreML model outputs: {output_names}")

            # Convert to ONNX
            # Note: coremltools doesn't directly export to ONNX anymore
            # We need to use an intermediate step through PyTorch or TF

            # Strategy: Use coremltools to convert to torch.nn.Module first
            try:
                # Try direct ONNX export if available
                import onnx
                from coremltools.converters import onnx as ct_onnx

                logger.info("Attempting direct CoreML -> ONNX conversion")

                # This may not be available in newer coremltools versions
                onnx_model = ct_onnx.convert(mlmodel)
                onnx.save(onnx_model, str(output_path))

            except (ImportError, AttributeError):
                # Fallback: Use torch backend
                logger.warning("Direct ONNX export not available, using PyTorch bridge")

                import torch
                import torch.onnx

                # Get a representative input
                input_dict = self._get_representative_input(spec)

                # Run inference to get output
                predictions = mlmodel.predict(input_dict)

                # Create a wrapper torch model
                class CoreMLWrapper(torch.nn.Module):
                    def __init__(self, coreml_model):
                        super().__init__()
                        self.mlmodel = coreml_model

                    def forward(self, *args):
                        # Convert torch tensors to numpy
                        import numpy as np
                        input_dict = {}
                        for i, (name, arg) in enumerate(zip(input_names, args)):
                            input_dict[name] = arg.cpu().numpy()

                        # Run CoreML prediction
                        output = self.mlmodel.predict(input_dict)

                        # Convert back to torch
                        results = []
                        for name in output_names:
                            results.append(torch.from_numpy(output[name]))

                        if len(results) == 1:
                            return results[0]
                        return tuple(results)

                wrapper = CoreMLWrapper(mlmodel)
                wrapper.eval()

                # Create dummy inputs
                dummy_inputs = []
                for inp in spec.description.input:
                    shape = self._get_input_shape(inp)
                    dummy_inputs.append(torch.randn(*shape))

                if len(dummy_inputs) == 1:
                    dummy_inputs = dummy_inputs[0]

                # Export to ONNX
                torch.onnx.export(
                    wrapper,
                    dummy_inputs,
                    str(output_path),
                    opset_version=opset_version,
                    input_names=input_names,
                    output_names=output_names,
                    verbose=kwargs.get("verbose", False),
                )

                warnings.append("Used PyTorch bridge for CoreML -> ONNX conversion")

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

            # Extract metadata
            model_info = self._extract_metadata_from_spec(spec, model_path)

            return ConversionResult(
                success=True,
                onnx_path=output_path,
                model_info=model_info,
                error_message=None,
                warnings=warnings
            )

        except Exception as e:
            logger.exception("CoreML conversion failed")
            return ConversionResult(
                success=False,
                onnx_path=None,
                model_info=None,
                error_message=f"Conversion failed: {str(e)}",
                warnings=[]
            )

    def extract_metadata(self, model_path: Path) -> Optional[ModelInfo]:
        """Extract metadata from CoreML model."""
        try:
            import coremltools as ct

            mlmodel = ct.models.MLModel(str(model_path))
            spec = mlmodel.get_spec()

            return self._extract_metadata_from_spec(spec, model_path)

        except Exception as e:
            logger.error(f"Failed to extract CoreML metadata: {e}")
            return None

    def _extract_metadata_from_spec(
        self,
        spec,
        model_path: Path
    ) -> ModelInfo:
        """Extract metadata from CoreML spec."""
        input_names = [inp.name for inp in spec.description.input]
        output_names = [out.name for out in spec.description.output]

        input_shapes = {}
        for inp in spec.description.input:
            input_shapes[inp.name] = self._get_input_shape(inp)

        output_shapes = {}
        for out in spec.description.output:
            output_shapes[out.name] = self._get_output_shape(out)

        return ModelInfo(
            model_path=model_path,
            model_format="coreml",
            input_names=input_names,
            output_names=output_names,
            input_shapes=input_shapes,
            output_shapes=output_shapes,
            metadata={
                "description": spec.description.metadata.shortDescription,
                "author": spec.description.metadata.author,
            }
        )

    def _get_input_shape(self, input_desc) -> List[int]:
        """Extract input shape from CoreML input description."""
        if input_desc.type.HasField("multiArrayType"):
            return list(input_desc.type.multiArrayType.shape)
        elif input_desc.type.HasField("imageType"):
            img = input_desc.type.imageType
            return [1, 3, img.height, img.width]  # Assuming RGB
        else:
            return []

    def _get_output_shape(self, output_desc) -> List[int]:
        """Extract output shape from CoreML output description."""
        if output_desc.type.HasField("multiArrayType"):
            return list(output_desc.type.multiArrayType.shape)
        else:
            return []

    def _get_representative_input(self, spec) -> Dict[str, Any]:
        """Create representative input for CoreML model."""
        import numpy as np

        input_dict = {}
        for inp in spec.description.input:
            if inp.type.HasField("multiArrayType"):
                shape = list(inp.type.multiArrayType.shape)
                input_dict[inp.name] = np.random.randn(*shape).astype(np.float32)
            elif inp.type.HasField("imageType"):
                img = inp.type.imageType
                from PIL import Image
                # Create dummy image
                dummy_img = Image.new("RGB", (img.width, img.height))
                input_dict[inp.name] = dummy_img
            else:
                # Default: scalar
                input_dict[inp.name] = np.array([0.0])

        return input_dict
