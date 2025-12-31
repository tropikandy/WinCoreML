"""
Converter Registry

Central registry for all model format converters.
Dispatches conversion requests to the appropriate converter based on file format.
"""

from pathlib import Path
from typing import List, Optional, Dict
import logging

from .base_converter import BaseConverter, ConversionResult, ModelInfo


logger = logging.getLogger(__name__)


class ConverterRegistry:
    """
    Registry for model format converters.

    Manages converter instances and routes conversion requests
    to the appropriate converter based on file extension and format.
    """

    def __init__(self):
        self._converters: List[BaseConverter] = []
        self._extension_map: Dict[str, List[BaseConverter]] = {}

    def register(self, converter: BaseConverter) -> None:
        """
        Register a converter.

        Args:
            converter: Converter instance to register
        """
        self._converters.append(converter)

        # Build extension mapping
        for ext in converter.supported_extensions:
            if ext not in self._extension_map:
                self._extension_map[ext] = []
            self._extension_map[ext].append(converter)

        logger.info(f"Registered converter: {converter.name}")

    def get_converter(self, model_path: Path) -> Optional[BaseConverter]:
        """
        Get appropriate converter for a model file.

        Args:
            model_path: Path to model file

        Returns:
            Converter instance or None if no suitable converter found
        """
        # Try extension-based lookup first
        ext = model_path.suffix.lower()

        # Handle special cases
        if model_path.is_dir():
            # Could be TensorFlow SavedModel or CoreML .mlpackage
            if (model_path / "saved_model.pb").exists():
                ext = ".savedmodel"
            elif model_path.suffix == ".mlpackage":
                ext = ".mlpackage"

        candidates = self._extension_map.get(ext, [])

        # Filter by can_convert check
        for converter in candidates:
            if converter.can_convert(model_path):
                logger.debug(f"Selected converter: {converter.name} for {model_path}")
                return converter

        # Fallback: try all converters
        for converter in self._converters:
            if converter.can_convert(model_path):
                logger.debug(f"Selected converter (fallback): {converter.name}")
                return converter

        logger.error(f"No converter found for: {model_path}")
        return None

    def convert(
        self,
        model_path: Path,
        output_path: Path,
        opset_version: int = 14,
        **kwargs
    ) -> ConversionResult:
        """
        Convert model to ONNX using appropriate converter.

        Args:
            model_path: Path to source model
            output_path: Path for output ONNX file
            opset_version: ONNX opset version
            **kwargs: Converter-specific options

        Returns:
            ConversionResult
        """
        converter = self.get_converter(model_path)

        if converter is None:
            return ConversionResult(
                success=False,
                onnx_path=None,
                model_info=None,
                error_message=f"No converter available for: {model_path}",
                warnings=[]
            )

        logger.info(f"Converting {model_path} using {converter.name}")

        try:
            result = converter.convert_to_onnx(
                model_path=model_path,
                output_path=output_path,
                opset_version=opset_version,
                **kwargs
            )
            return result
        except Exception as e:
            logger.exception(f"Conversion failed with {converter.name}")
            return ConversionResult(
                success=False,
                onnx_path=None,
                model_info=None,
                error_message=f"Conversion error: {str(e)}",
                warnings=[]
            )

    def extract_metadata(self, model_path: Path) -> Optional[ModelInfo]:
        """
        Extract metadata from model.

        Args:
            model_path: Path to model file

        Returns:
            ModelInfo or None
        """
        converter = self.get_converter(model_path)

        if converter is None:
            return None

        try:
            return converter.extract_metadata(model_path)
        except Exception as e:
            logger.exception(f"Metadata extraction failed")
            return None

    def list_converters(self) -> List[str]:
        """Get list of registered converter names."""
        return [c.name for c in self._converters]

    def get_supported_formats(self) -> Dict[str, List[str]]:
        """
        Get mapping of converter names to supported extensions.

        Returns:
            Dict mapping converter name to list of extensions
        """
        return {
            converter.name: converter.supported_extensions
            for converter in self._converters
        }
