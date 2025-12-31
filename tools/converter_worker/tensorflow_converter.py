"""
TensorFlow to ONNX Converter

Converts TensorFlow SavedModel to ONNX format using tf2onnx.
"""

from pathlib import Path
from typing import List, Optional, Dict, Any
import logging

from .base_converter import BaseConverter, ConversionResult, ModelInfo


logger = logging.getLogger(__name__)


class TensorFlowConverter(BaseConverter):
    """
    Converter for TensorFlow models.

    Supports:
    - SavedModel format (directory with saved_model.pb)
    - Keras .h5 models (via SavedModel conversion)
    """

    @property
    def name(self) -> str:
        return "TensorFlow"

    @property
    def supported_extensions(self) -> List[str]:
        return [".savedmodel", ".h5", ".keras"]

    def can_convert(self, model_path: Path) -> bool:
        """Check if path is a TensorFlow model."""
        if not model_path.exists():
            return False

        # SavedModel directory
        if model_path.is_dir():
            if (model_path / "saved_model.pb").exists():
                return True
            if (model_path / "saved_model.pbtxt").exists():
                return True

        # Keras .h5 or .keras file
        if model_path.suffix.lower() in [".h5", ".keras"]:
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
        Convert TensorFlow model to ONNX using tf2onnx.

        Kwargs:
            signature_def: str - Signature to use (default: "serving_default")
            tag: str - Tag for SavedModel (default: "serve")
            concrete_function: int - Index of concrete function to convert
        """
        try:
            import tensorflow as tf
            import tf2onnx

            warnings = []

            # Handle Keras models - convert to SavedModel first
            if model_path.suffix.lower() in [".h5", ".keras"]:
                logger.info("Loading Keras model, converting to SavedModel first")

                model = tf.keras.models.load_model(str(model_path))

                # Create temp SavedModel
                temp_dir = output_path.parent / f"_temp_savedmodel_{output_path.stem}"
                temp_dir.mkdir(exist_ok=True)

                tf.saved_model.save(model, str(temp_dir))
                savedmodel_path = temp_dir
                warnings.append(f"Converted Keras to SavedModel: {temp_dir}")
            else:
                savedmodel_path = model_path

            # Convert SavedModel to ONNX
            signature_def = kwargs.get("signature_def", "serving_default")
            tag = kwargs.get("tag", "serve")

            logger.info(f"Converting SavedModel to ONNX (opset {opset_version})")

            # Use tf2onnx
            model_proto, external_tensor_storage = tf2onnx.convert.from_saved_model(
                str(savedmodel_path),
                input_names=None,
                output_names=None,
                tag=tag,
                signature_def=signature_def,
                opset=opset_version,
            )

            # Save ONNX model
            import onnx
            onnx.save(model_proto, str(output_path))

            # Validate
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
            model_info = self._extract_metadata_from_onnx(
                output_path,
                model_path
            )

            return ConversionResult(
                success=True,
                onnx_path=output_path,
                model_info=model_info,
                error_message=None,
                warnings=warnings
            )

        except Exception as e:
            logger.exception("TensorFlow conversion failed")
            return ConversionResult(
                success=False,
                onnx_path=None,
                model_info=None,
                error_message=f"Conversion failed: {str(e)}",
                warnings=[]
            )

    def extract_metadata(self, model_path: Path) -> Optional[ModelInfo]:
        """Extract metadata from TensorFlow model."""
        try:
            import tensorflow as tf

            if model_path.suffix.lower() in [".h5", ".keras"]:
                # Keras model
                model = tf.keras.models.load_model(str(model_path))

                input_names = [inp.name for inp in model.inputs]
                output_names = [out.name for out in model.outputs]

                input_shapes = {
                    inp.name: inp.shape.as_list()
                    for inp in model.inputs
                }
                output_shapes = {
                    out.name: out.shape.as_list()
                    for out in model.outputs
                }

            else:
                # SavedModel
                loaded = tf.saved_model.load(str(model_path))

                # Get signature
                try:
                    signature = loaded.signatures["serving_default"]
                except KeyError:
                    # Try first available signature
                    signature = list(loaded.signatures.values())[0]

                input_names = list(signature.structured_input_signature[1].keys())
                output_names = list(signature.structured_outputs.keys())

                input_shapes = {
                    name: spec.shape.as_list()
                    for name, spec in signature.structured_input_signature[1].items()
                }
                output_shapes = {
                    name: tensor.shape.as_list()
                    for name, tensor in signature.structured_outputs.items()
                }

            return ModelInfo(
                model_path=model_path,
                model_format="tensorflow",
                input_names=input_names,
                output_names=output_names,
                input_shapes=input_shapes,
                output_shapes=output_shapes,
                metadata={}
            )

        except Exception as e:
            logger.error(f"Failed to extract TensorFlow metadata: {e}")
            return None

    def _extract_metadata_from_onnx(
        self,
        onnx_path: Path,
        original_path: Path
    ) -> ModelInfo:
        """Extract metadata from converted ONNX model."""
        import onnx

        model = onnx.load(str(onnx_path))

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

        return ModelInfo(
            model_path=original_path,
            model_format="tensorflow",
            input_names=input_names,
            output_names=output_names,
            input_shapes=input_shapes,
            output_shapes=output_shapes,
            metadata={
                "onnx_opset": model.opset_import[0].version if model.opset_import else None
            }
        )
