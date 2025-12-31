"""
Universal ML Model Converter Worker

Supports conversion of PyTorch, TensorFlow, CoreML, and ONNX models to ONNX format.
"""

from .base_converter import BaseConverter, ConversionResult, ModelInfo
from .converter_registry import ConverterRegistry
from .pytorch_converter import PyTorchConverter
from .tensorflow_converter import TensorFlowConverter
from .coreml_converter import CoreMLConverter
from .onnx_converter import ONNXConverter
from .benchmarking import ModelBenchmarker, BenchmarkResult, BenchmarkSummary
from .worker import ConverterWorker

__version__ = "0.1.0"

__all__ = [
    "BaseConverter",
    "ConversionResult",
    "ModelInfo",
    "ConverterRegistry",
    "PyTorchConverter",
    "TensorFlowConverter",
    "CoreMLConverter",
    "ONNXConverter",
    "ModelBenchmarker",
    "BenchmarkResult",
    "BenchmarkSummary",
    "ConverterWorker",
]
