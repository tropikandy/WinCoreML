"""
Model Benchmarking Module

Benchmarks ONNX models across available providers to estimate latency and speedup.
Runs after model registration to provide performance insights.
"""

from dataclasses import dataclass
from pathlib import Path
from typing import List, Dict, Optional, Any
import time
import logging
import numpy as np

logger = logging.getLogger(__name__)


@dataclass
class BenchmarkResult:
    """Results from benchmarking a model on a provider."""
    provider_name: str
    success: bool
    mean_latency_ms: float
    std_latency_ms: float
    min_latency_ms: float
    max_latency_ms: float
    throughput_inferences_per_sec: float
    memory_usage_mb: Optional[float]
    error_message: Optional[str]
    warmup_iterations: int
    benchmark_iterations: int


@dataclass
class BenchmarkSummary:
    """Summary of benchmarks across all providers."""
    model_path: Path
    results: List[BenchmarkResult]
    fastest_provider: Optional[str]
    baseline_latency_ms: float  # CPU baseline
    best_latency_ms: float
    speedup_vs_cpu: float
    timestamp: float


class ModelBenchmarker:
    """
    Benchmarks ONNX models on different execution providers.

    Measures:
    - Inference latency (mean, std, min, max)
    - Throughput (inferences/sec)
    - Memory usage
    - Speedup vs CPU baseline
    """

    def __init__(
        self,
        warmup_iterations: int = 5,
        benchmark_iterations: int = 50,
        enable_profiling: bool = False
    ):
        """
        Initialize benchmarker.

        Args:
            warmup_iterations: Number of warmup runs (not measured)
            benchmark_iterations: Number of measured inference runs
            enable_profiling: Enable detailed profiling
        """
        self.warmup_iterations = warmup_iterations
        self.benchmark_iterations = benchmark_iterations
        self.enable_profiling = enable_profiling

    def benchmark_model(
        self,
        onnx_path: Path,
        providers: Optional[List[str]] = None
    ) -> BenchmarkSummary:
        """
        Benchmark model on specified providers.

        Args:
            onnx_path: Path to ONNX model
            providers: List of ONNX Runtime provider names to test
                      If None, uses ["CPUExecutionProvider", "DmlExecutionProvider"]

        Returns:
            BenchmarkSummary with results
        """
        try:
            import onnxruntime as ort
        except ImportError:
            logger.error("onnxruntime not available, cannot benchmark")
            return BenchmarkSummary(
                model_path=onnx_path,
                results=[],
                fastest_provider=None,
                baseline_latency_ms=0.0,
                best_latency_ms=0.0,
                speedup_vs_cpu=1.0,
                timestamp=time.time()
            )

        if providers is None:
            # Default providers to test
            available_providers = ort.get_available_providers()
            providers = []

            # Prioritize: DML (GPU/NPU), OpenVINO, CPU
            for provider in ["DmlExecutionProvider", "OpenVINOExecutionProvider", "CPUExecutionProvider"]:
                if provider in available_providers:
                    providers.append(provider)

        logger.info(f"Benchmarking model on providers: {providers}")

        results = []
        for provider in providers:
            result = self._benchmark_provider(onnx_path, provider)
            results.append(result)

        # Compute summary
        summary = self._compute_summary(onnx_path, results)

        logger.info(f"Benchmark complete. Fastest: {summary.fastest_provider}, "
                   f"Speedup: {summary.speedup_vs_cpu:.2f}x")

        return summary

    def _benchmark_provider(
        self,
        onnx_path: Path,
        provider: str
    ) -> BenchmarkResult:
        """Benchmark model on a specific provider."""
        try:
            import onnxruntime as ort

            logger.info(f"Benchmarking on {provider}")

            # Create session
            session_options = ort.SessionOptions()
            if self.enable_profiling:
                session_options.enable_profiling = True

            session = ort.InferenceSession(
                str(onnx_path),
                session_options,
                providers=[provider]
            )

            # Get input info
            input_info = {
                inp.name: {
                    "shape": inp.shape,
                    "type": inp.type
                }
                for inp in session.get_inputs()
            }

            # Create dummy inputs
            dummy_inputs = self._create_dummy_inputs(input_info)

            # Warmup
            logger.debug(f"Warmup: {self.warmup_iterations} iterations")
            for _ in range(self.warmup_iterations):
                _ = session.run(None, dummy_inputs)

            # Benchmark
            logger.debug(f"Benchmark: {self.benchmark_iterations} iterations")
            latencies = []

            for _ in range(self.benchmark_iterations):
                start = time.perf_counter()
                _ = session.run(None, dummy_inputs)
                end = time.perf_counter()
                latencies.append((end - start) * 1000)  # Convert to ms

            # Compute statistics
            mean_latency = np.mean(latencies)
            std_latency = np.std(latencies)
            min_latency = np.min(latencies)
            max_latency = np.max(latencies)
            throughput = 1000.0 / mean_latency  # inferences/sec

            # Try to get memory usage (if available)
            memory_mb = None
            try:
                # This is provider-specific and may not work
                profiling_data = session.end_profiling()
                # Parse profiling data if needed
            except:
                pass

            return BenchmarkResult(
                provider_name=provider,
                success=True,
                mean_latency_ms=mean_latency,
                std_latency_ms=std_latency,
                min_latency_ms=min_latency,
                max_latency_ms=max_latency,
                throughput_inferences_per_sec=throughput,
                memory_usage_mb=memory_mb,
                error_message=None,
                warmup_iterations=self.warmup_iterations,
                benchmark_iterations=self.benchmark_iterations
            )

        except Exception as e:
            logger.error(f"Benchmark failed on {provider}: {e}")
            return BenchmarkResult(
                provider_name=provider,
                success=False,
                mean_latency_ms=0.0,
                std_latency_ms=0.0,
                min_latency_ms=0.0,
                max_latency_ms=0.0,
                throughput_inferences_per_sec=0.0,
                memory_usage_mb=None,
                error_message=str(e),
                warmup_iterations=self.warmup_iterations,
                benchmark_iterations=self.benchmark_iterations
            )

    def _create_dummy_inputs(self, input_info: Dict[str, Dict]) -> Dict[str, np.ndarray]:
        """Create dummy inputs for benchmarking."""
        dummy_inputs = {}

        for name, info in input_info.items():
            shape = info["shape"]

            # Handle dynamic dimensions
            concrete_shape = []
            for dim in shape:
                if isinstance(dim, str) or dim < 0:
                    # Dynamic dimension, use reasonable default
                    concrete_shape.append(1)
                else:
                    concrete_shape.append(dim)

            # Create random tensor
            # TODO: Support different dtypes based on info["type"]
            dummy_inputs[name] = np.random.randn(*concrete_shape).astype(np.float32)

        return dummy_inputs

    def _compute_summary(
        self,
        model_path: Path,
        results: List[BenchmarkResult]
    ) -> BenchmarkSummary:
        """Compute benchmark summary."""
        # Find CPU baseline
        cpu_result = None
        for result in results:
            if "CPU" in result.provider_name and result.success:
                cpu_result = result
                break

        baseline_latency = cpu_result.mean_latency_ms if cpu_result else 0.0

        # Find fastest provider
        successful_results = [r for r in results if r.success]
        if successful_results:
            fastest = min(successful_results, key=lambda r: r.mean_latency_ms)
            fastest_provider = fastest.provider_name
            best_latency = fastest.mean_latency_ms
        else:
            fastest_provider = None
            best_latency = 0.0

        # Compute speedup
        if baseline_latency > 0 and best_latency > 0:
            speedup = baseline_latency / best_latency
        else:
            speedup = 1.0

        return BenchmarkSummary(
            model_path=model_path,
            results=results,
            fastest_provider=fastest_provider,
            baseline_latency_ms=baseline_latency,
            best_latency_ms=best_latency,
            speedup_vs_cpu=speedup,
            timestamp=time.time()
        )

    def format_summary(self, summary: BenchmarkSummary) -> str:
        """Format benchmark summary as human-readable string."""
        lines = []
        lines.append(f"\nBenchmark Summary: {summary.model_path.name}")
        lines.append("=" * 60)

        for result in summary.results:
            if result.success:
                lines.append(f"\n{result.provider_name}:")
                lines.append(f"  Mean Latency:  {result.mean_latency_ms:.2f} ± {result.std_latency_ms:.2f} ms")
                lines.append(f"  Min/Max:       {result.min_latency_ms:.2f} / {result.max_latency_ms:.2f} ms")
                lines.append(f"  Throughput:    {result.throughput_inferences_per_sec:.2f} inferences/sec")
                if result.memory_usage_mb:
                    lines.append(f"  Memory Usage:  {result.memory_usage_mb:.2f} MB")
            else:
                lines.append(f"\n{result.provider_name}: FAILED")
                lines.append(f"  Error: {result.error_message}")

        lines.append("\n" + "=" * 60)
        if summary.fastest_provider:
            lines.append(f"Fastest Provider: {summary.fastest_provider}")
            lines.append(f"Best Latency:     {summary.best_latency_ms:.2f} ms")
            lines.append(f"Speedup vs CPU:   {summary.speedup_vs_cpu:.2f}x")
        else:
            lines.append("No successful benchmarks")

        return "\n".join(lines)
