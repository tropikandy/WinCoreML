"""
Universal ML Model Converter Worker

Main worker process for converting ML models to ONNX format.
Supports PyTorch, TensorFlow, CoreML, and ONNX models.
"""

import sys
import json
import logging
from pathlib import Path
from typing import Optional, Dict, Any

from .converter_registry import ConverterRegistry
from .pytorch_converter import PyTorchConverter
from .tensorflow_converter import TensorFlowConverter
from .coreml_converter import CoreMLConverter
from .onnx_converter import ONNXConverter
from .benchmarking import ModelBenchmarker


# Setup logging
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s"
)
logger = logging.getLogger(__name__)


class ConverterWorker:
    """
    Main converter worker process.

    Handles:
    1. Model format detection
    2. Conversion to ONNX
    3. Validation
    4. Benchmarking (optional)
    5. Metadata extraction
    """

    def __init__(self):
        self.registry = ConverterRegistry()
        self._register_converters()

    def _register_converters(self):
        """Register all available converters."""
        # Order matters - earlier converters are tried first
        self.registry.register(ONNXConverter())       # No conversion needed
        self.registry.register(CoreMLConverter())     # CoreML models
        self.registry.register(PyTorchConverter())    # PyTorch models
        self.registry.register(TensorFlowConverter()) # TensorFlow models

        logger.info(f"Registered converters: {self.registry.list_converters()}")

    def convert_model(
        self,
        model_path: str,
        output_path: str,
        benchmark: bool = True,
        opset_version: int = 14,
        **kwargs
    ) -> Dict[str, Any]:
        """
        Convert model to ONNX and optionally benchmark.

        Args:
            model_path: Path to source model
            output_path: Path for output ONNX file
            benchmark: Run benchmarking after conversion
            opset_version: ONNX opset version
            **kwargs: Converter-specific options

        Returns:
            Dict with conversion results and metadata
        """
        model_path = Path(model_path)
        output_path = Path(output_path)

        logger.info(f"Converting {model_path} to {output_path}")

        # Convert
        result = self.registry.convert(
            model_path=model_path,
            output_path=output_path,
            opset_version=opset_version,
            **kwargs
        )

        if not result.success:
            logger.error(f"Conversion failed: {result.error_message}")
            return {
                "success": False,
                "error": result.error_message,
                "warnings": result.warnings
            }

        logger.info(f"Conversion successful: {result.onnx_path}")

        # Prepare response
        response = {
            "success": True,
            "onnx_path": str(result.onnx_path),
            "warnings": result.warnings,
            "model_info": {
                "format": result.model_info.model_format,
                "input_names": result.model_info.input_names,
                "output_names": result.model_info.output_names,
                "input_shapes": result.model_info.input_shapes,
                "output_shapes": result.model_info.output_shapes,
                "metadata": result.model_info.metadata
            }
        }

        # Benchmark if requested
        if benchmark and result.onnx_path.exists():
            logger.info("Running benchmark...")
            try:
                benchmarker = ModelBenchmarker(
                    warmup_iterations=kwargs.get("warmup_iterations", 5),
                    benchmark_iterations=kwargs.get("benchmark_iterations", 50)
                )

                summary = benchmarker.benchmark_model(result.onnx_path)

                response["benchmark"] = {
                    "fastest_provider": summary.fastest_provider,
                    "baseline_latency_ms": summary.baseline_latency_ms,
                    "best_latency_ms": summary.best_latency_ms,
                    "speedup_vs_cpu": summary.speedup_vs_cpu,
                    "results": [
                        {
                            "provider": r.provider_name,
                            "success": r.success,
                            "mean_latency_ms": r.mean_latency_ms,
                            "throughput": r.throughput_inferences_per_sec,
                            "error": r.error_message
                        }
                        for r in summary.results
                    ]
                }

                logger.info(f"Benchmark complete. Fastest: {summary.fastest_provider}, "
                           f"Speedup: {summary.speedup_vs_cpu:.2f}x")

            except Exception as e:
                logger.warning(f"Benchmarking failed: {e}")
                response["benchmark_error"] = str(e)

        return response

    def extract_metadata(self, model_path: str) -> Optional[Dict[str, Any]]:
        """
        Extract metadata from model without full conversion.

        Args:
            model_path: Path to model file

        Returns:
            Dict with metadata or None
        """
        model_path = Path(model_path)

        logger.info(f"Extracting metadata from {model_path}")

        model_info = self.registry.extract_metadata(model_path)

        if model_info is None:
            return None

        return {
            "format": model_info.model_format,
            "input_names": model_info.input_names,
            "output_names": model_info.output_names,
            "input_shapes": model_info.input_shapes,
            "output_shapes": model_info.output_shapes,
            "metadata": model_info.metadata
        }

    def list_supported_formats(self) -> Dict[str, list[str]]:
        """List all supported model formats."""
        return self.registry.get_supported_formats()


def main():
    """
    Main entry point for converter worker.

    Reads JSON from stdin, writes JSON to stdout.
    Format:
        Input: {"command": "convert", "model_path": "...", "output_path": "...", ...}
        Output: {"success": true, "onnx_path": "...", ...}
    """
    worker = ConverterWorker()

    try:
        # Read request from stdin
        request = json.loads(sys.stdin.read())

        command = request.get("command")

        if command == "convert":
            result = worker.convert_model(
                model_path=request["model_path"],
                output_path=request["output_path"],
                benchmark=request.get("benchmark", True),
                opset_version=request.get("opset_version", 14),
                **request.get("options", {})
            )

        elif command == "metadata":
            metadata = worker.extract_metadata(request["model_path"])
            result = {
                "success": metadata is not None,
                "metadata": metadata
            }

        elif command == "list_formats":
            formats = worker.list_supported_formats()
            result = {
                "success": True,
                "formats": formats
            }

        else:
            result = {
                "success": False,
                "error": f"Unknown command: {command}"
            }

        # Write response to stdout
        print(json.dumps(result))
        sys.stdout.flush()

    except Exception as e:
        logger.exception("Worker error")
        result = {
            "success": False,
            "error": str(e)
        }
        print(json.dumps(result))
        sys.stdout.flush()
        sys.exit(1)


if __name__ == "__main__":
    main()
