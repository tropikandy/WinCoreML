"""
Base Converter Interface

Defines the abstract interface for model format converters.
All converters (PyTorch, TensorFlow, CoreML) implement this interface.
"""

from abc import ABC, abstractmethod
from dataclasses import dataclass
from pathlib import Path
from typing import Optional, List, Dict, Any
import hashlib


@dataclass
class ModelInfo:
    """Information about a model."""
    model_path: Path
    model_format: str  # "pytorch", "tensorflow", "coreml", "onnx"
    input_names: List[str]
    output_names: List[str]
    input_shapes: Dict[str, List[int]]
    output_shapes: Dict[str, List[int]]
    metadata: Dict[str, Any]


@dataclass
class ConversionResult:
    """Result of model conversion."""
    success: bool
    onnx_path: Optional[Path]
    model_info: Optional[ModelInfo]
    error_message: Optional[str]
    warnings: List[str]


class BaseConverter(ABC):
    """
    Abstract base class for model converters.

    Each converter is responsible for:
    1. Detecting if it can handle a model file
    2. Converting the model to ONNX
    3. Extracting model metadata
    """

    @property
    @abstractmethod
    def name(self) -> str:
        """Converter name (e.g., 'PyTorch', 'TensorFlow')."""
        pass

    @property
    @abstractmethod
    def supported_extensions(self) -> List[str]:
        """File extensions this converter handles (e.g., ['.pt', '.pth'])."""
        pass

    @abstractmethod
    def can_convert(self, model_path: Path) -> bool:
        """
        Check if this converter can handle the given model file.

        Args:
            model_path: Path to model file

        Returns:
            True if this converter can handle the file
        """
        pass

    @abstractmethod
    def convert_to_onnx(
        self,
        model_path: Path,
        output_path: Path,
        opset_version: int = 14,
        **kwargs
    ) -> ConversionResult:
        """
        Convert model to ONNX format.

        Args:
            model_path: Path to source model
            output_path: Path for output ONNX file
            opset_version: ONNX opset version
            **kwargs: Converter-specific options

        Returns:
            ConversionResult with success status and metadata
        """
        pass

    @abstractmethod
    def extract_metadata(self, model_path: Path) -> Optional[ModelInfo]:
        """
        Extract metadata from model without full conversion.

        Args:
            model_path: Path to model file

        Returns:
            ModelInfo if successful, None otherwise
        """
        pass

    def compute_hash(self, model_path: Path) -> str:
        """
        Compute SHA256 hash of model file(s).

        Args:
            model_path: Path to model file or directory

        Returns:
            SHA256 hash string
        """
        hasher = hashlib.sha256()

        if model_path.is_dir():
            # For directories (e.g., TF SavedModel, CoreML .mlpackage)
            # Hash all files in sorted order
            files = sorted(model_path.rglob("*"))
            for file in files:
                if file.is_file():
                    hasher.update(str(file.relative_to(model_path)).encode())
                    hasher.update(file.read_bytes())
        else:
            # Single file
            hasher.update(model_path.read_bytes())

        return hasher.hexdigest()

    def validate_onnx(self, onnx_path: Path) -> tuple[bool, Optional[str]]:
        """
        Validate generated ONNX model.

        Args:
            onnx_path: Path to ONNX file

        Returns:
            (is_valid, error_message)
        """
        try:
            import onnx
            model = onnx.load(str(onnx_path))
            onnx.checker.check_model(model)
            return True, None
        except Exception as e:
            return False, f"ONNX validation failed: {str(e)}"
