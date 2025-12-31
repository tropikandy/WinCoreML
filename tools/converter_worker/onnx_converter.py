"""
ONNX Pass-through Converter

Handles ONNX models (no conversion needed, just validation and metadata extraction).
"""

from pathlib import Path
from typing import List, Optional
import logging
import shutil

from .base_converter import BaseConverter, ConversionResult, ModelInfo


logger = logging.getLogger(__name__)


class ONNXConverter(BaseConverter):
    """
    Converter for ONNX models.

    Since the model is already in ONNX format, this just validates
    and copies the file to the output location.
    """

    @property
    def name(self) -> str:
        return "ONNX"

    @property
    def supported_extensions(self) -> List[str]:
        return [".onnx"]

    def can_convert(self, model_path: Path) -> bool:
        """Check if file is an ONNX model."""
        if not model_path.exists():
            return False

        if model_path.suffix.lower() != ".onnx":
            return False

        # Try to load as ONNX
        try:
            import onnx
            _ = onnx.load(str(model_path))
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
        'Convert' ONNX model (just copy and validate).

        Kwargs:
            optimize: bool - Run ONNX optimizer (default: False)
        """
        try:
            import onnx

            warnings = []

            # Load and validate
            logger.info(f"Loading ONNX model from {model_path}")
            model = onnx.load(str(model_path))
            onnx.checker.check_model(model)

            # Check if optimization requested
            if kwargs.get("optimize", False):
                logger.info("Optimizing ONNX model")
                try:
                    from onnxruntime.transformers import optimizer

                    # Basic optimization
                    optimized = optimizer.optimize_model(
                        str(model_path),
                        model_type="bert",  # Generic
                        num_heads=0,
                        hidden_size=0,
                    )

                    optimized.save_model_to_file(str(output_path))
                    warnings.append("Applied ONNX optimization")

                except Exception as e:
                    logger.warning(f"Optimization failed, copying original: {e}")
                    shutil.copy2(model_path, output_path)
                    warnings.append("Optimization failed, using original model")
            else:
                # Just copy
                logger.info(f"Copying ONNX model to {output_path}")
                shutil.copy2(model_path, output_path)

            # Extract metadata
            model_info = self._extract_metadata_from_onnx(model, model_path)

            return ConversionResult(
                success=True,
                onnx_path=output_path,
                model_info=model_info,
                error_message=None,
                warnings=warnings
            )

        except Exception as e:
            logger.exception("ONNX validation failed")
            return ConversionResult(
                success=False,
                onnx_path=None,
                model_info=None,
                error_message=f"ONNX validation failed: {str(e)}",
                warnings=[]
            )

    def extract_metadata(self, model_path: Path) -> Optional[ModelInfo]:
        """Extract metadata from ONNX model."""
        try:
            import onnx

            model = onnx.load(str(model_path))
            return self._extract_metadata_from_onnx(model, model_path)

        except Exception as e:
            logger.error(f"Failed to extract ONNX metadata: {e}")
            return None

    def _extract_metadata_from_onnx(self, model, model_path: Path) -> ModelInfo:
        """Extract metadata from ONNX model proto."""
        input_names = [inp.name for inp in model.graph.input]
        output_names = [out.name for out in model.graph.output]

        input_shapes = {}
        for inp in model.graph.input:
            shape = [
                dim.dim_value if dim.dim_value > 0 else -1
                for dim in inp.type.tensor_type.shape.dim
            ]
            input_shapes[inp.name] = shape

        output_shapes = {}
        for out in model.graph.output:
            shape = [
                dim.dim_value if dim.dim_value > 0 else -1
                for dim in out.type.tensor_type.shape.dim
            ]
            output_shapes[out.name] = shape

        # Extract additional metadata
        metadata = {
            "onnx_opset": model.opset_import[0].version if model.opset_import else None,
            "ir_version": model.ir_version,
            "producer_name": model.producer_name,
            "producer_version": model.producer_version,
        }

        return ModelInfo(
            model_path=model_path,
            model_format="onnx",
            input_names=input_names,
            output_names=output_names,
            input_shapes=input_shapes,
            output_shapes=output_shapes,
            metadata=metadata
        )
