#!/usr/bin/env python3
"""
Test script for real ONNX model inference through CoreMLWin runtime.

This script tests end-to-end inference with actual ONNX models using both
CPU and DirectML (GPU) execution providers.

Requirements:
- CoreMLWin runtime service running
- ONNX Runtime configured (ONNXRUNTIME_DIR set)
- Sample ONNX models available

Usage:
    python test_real_inference.py --model path/to/model.onnx --provider DmlExecutionProvider
    python test_real_inference.py --model path/to/model.onnx --provider CPUExecutionProvider
    python test_real_inference.py --benchmark  # Run full benchmark suite
"""

import argparse
import time
import numpy as np
from pathlib import Path
import sys

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent.parent / 'sdk' / 'python'))

from coremlwin_client import CoreMLWinClient, CoreMLWinError


def create_sample_input(model_info, batch_size=1):
    """Create sample input tensors based on model input shapes."""
    inputs = {}

    for input_name, input_shape in model_info['input_shapes'].items():
        # Replace dynamic dimensions (-1) with batch_size
        shape = [batch_size if dim == -1 else dim for dim in input_shape]

        # Create random input data (normalized for image models)
        data = np.random.randn(*shape).astype(np.float32) * 0.5 + 0.5
        inputs[input_name] = data

    return inputs


def test_single_inference(client, model_path, provider):
    """Test single inference with a model."""
    print(f"\n{'='*70}")
    print(f"Testing: {Path(model_path).name}")
    print(f"Provider: {provider}")
    print(f"{'='*70}\n")

    # Register model
    print(f"[1/4] Registering model...")
    start = time.time()
    model_id = client.register_model(str(model_path), provider_hint=provider)
    register_time = time.time() - start
    print(f"  ✓ Model ID: {model_id}")
    print(f"  ✓ Registration time: {register_time*1000:.2f} ms\n")

    # Get model info
    print(f"[2/4] Getting model info...")
    model_info = client.get_model_info(model_id)
    print(f"  Model path: {model_info['model_path']}")
    print(f"  Provider: {model_info['provider']}")
    print(f"  Input shapes: {model_info['input_shapes']}")
    print(f"  Output shapes: {model_info['output_shapes']}\n")

    # Prepare inputs
    print(f"[3/4] Preparing inputs...")
    inputs = create_sample_input(model_info)
    for name, tensor in inputs.items():
        print(f"  Input '{name}': shape={tensor.shape}, dtype={tensor.dtype}")
    print()

    # Run inference
    print(f"[4/4] Running inference...")
    start = time.time()
    outputs = client.run_inference(model_id, inputs)
    inference_time = time.time() - start

    print(f"  ✓ Inference time: {inference_time*1000:.2f} ms")
    for name, tensor in outputs.items():
        print(f"  Output '{name}': shape={tensor.shape}, dtype={tensor.dtype}")
        # Show sample values
        flat = tensor.flatten()
        print(f"    Sample values: [{flat[0]:.4f}, {flat[1]:.4f}, ..., {flat[-1]:.4f}]")

    print(f"\n{'='*70}")
    print(f"✓ Test completed successfully!")
    print(f"{'='*70}\n")

    return {
        'model_id': model_id,
        'register_time': register_time,
        'inference_time': inference_time,
        'outputs': outputs
    }


def test_warmup_performance(client, model_id, warmup_iters=5, bench_iters=50):
    """Test inference performance with warmup."""
    print(f"\n{'='*70}")
    print(f"Performance Benchmark")
    print(f"{'='*70}\n")

    model_info = client.get_model_info(model_id)
    inputs = create_sample_input(model_info)

    # Warmup
    print(f"[1/2] Warmup ({warmup_iters} iterations)...")
    for i in range(warmup_iters):
        client.run_inference(model_id, inputs)
        if (i + 1) % 5 == 0:
            print(f"  Completed {i+1}/{warmup_iters} warmup iterations")
    print("  ✓ Warmup complete\n")

    # Benchmark
    print(f"[2/2] Benchmark ({bench_iters} iterations)...")
    times = []
    for i in range(bench_iters):
        start = time.time()
        client.run_inference(model_id, inputs)
        times.append(time.time() - start)

        if (i + 1) % 10 == 0:
            print(f"  Completed {i+1}/{bench_iters} iterations")

    # Statistics
    times_ms = [t * 1000 for t in times]
    print(f"\n{'='*70}")
    print(f"Performance Statistics:")
    print(f"{'='*70}")
    print(f"  Mean:   {np.mean(times_ms):.2f} ms")
    print(f"  Median: {np.median(times_ms):.2f} ms")
    print(f"  Std:    {np.std(times_ms):.2f} ms")
    print(f"  Min:    {np.min(times_ms):.2f} ms")
    print(f"  Max:    {np.max(times_ms):.2f} ms")
    print(f"  P95:    {np.percentile(times_ms, 95):.2f} ms")
    print(f"  P99:    {np.percentile(times_ms, 99):.2f} ms")
    print(f"{'='*70}\n")

    return times_ms


def compare_providers(client, model_path, providers=['CPUExecutionProvider', 'DmlExecutionProvider']):
    """Compare inference performance across providers."""
    print(f"\n{'='*70}")
    print(f"Provider Comparison: {Path(model_path).name}")
    print(f"{'='*70}\n")

    results = {}

    for provider in providers:
        print(f"\nTesting {provider}...")
        print("-" * 70)

        try:
            # Test inference
            result = test_single_inference(client, model_path, provider)

            # Benchmark
            times = test_warmup_performance(result['model_id'], warmup_iters=5, bench_iters=30)

            results[provider] = {
                'mean_ms': np.mean(times),
                'median_ms': np.median(times),
                'p95_ms': np.percentile(times, 95)
            }

            # Cleanup
            client.unregister_model(result['model_id'])

        except CoreMLWinError as e:
            print(f"  ✗ Failed with {provider}: {e}")
            results[provider] = None

    # Print comparison
    print(f"\n{'='*70}")
    print(f"Comparison Summary")
    print(f"{'='*70}\n")

    for provider, stats in results.items():
        if stats:
            print(f"{provider}:")
            print(f"  Mean:   {stats['mean_ms']:.2f} ms")
            print(f"  Median: {stats['median_ms']:.2f} ms")
            print(f"  P95:    {stats['p95_ms']:.2f} ms")
        else:
            print(f"{provider}: NOT AVAILABLE")

    # Compute speedup if both providers worked
    if len([r for r in results.values() if r]) == 2:
        cpu_time = results['CPUExecutionProvider']['mean_ms']
        dml_time = results['DmlExecutionProvider']['mean_ms']
        speedup = cpu_time / dml_time
        print(f"\nDirectML Speedup: {speedup:.2f}x")

    print(f"\n{'='*70}\n")


def download_sample_model():
    """Download a sample ONNX model for testing."""
    import urllib.request

    models_dir = Path(__file__).parent.parent / 'models'
    models_dir.mkdir(exist_ok=True)

    # MobileNetV2 - small, fast model for testing
    model_url = "https://github.com/onnx/models/raw/main/vision/classification/mobilenet/model/mobilenetv2-7.onnx"
    model_path = models_dir / "mobilenetv2-7.onnx"

    if model_path.exists():
        print(f"Model already exists: {model_path}")
        return model_path

    print(f"Downloading sample model from ONNX Model Zoo...")
    print(f"  URL: {model_url}")
    print(f"  Destination: {model_path}")

    urllib.request.urlretrieve(model_url, model_path)
    print(f"  ✓ Download complete ({model_path.stat().st_size / 1024 / 1024:.1f} MB)")

    return model_path


def main():
    parser = argparse.ArgumentParser(description='Test real ONNX inference through CoreMLWin')
    parser.add_argument('--model', type=str, help='Path to ONNX model file')
    parser.add_argument('--provider', type=str, default='CPUExecutionProvider',
                       choices=['CPUExecutionProvider', 'DmlExecutionProvider'],
                       help='Execution provider to use')
    parser.add_argument('--benchmark', action='store_true',
                       help='Run full benchmark suite')
    parser.add_argument('--compare', action='store_true',
                       help='Compare CPU vs DirectML performance')
    parser.add_argument('--download-sample', action='store_true',
                       help='Download sample model from ONNX Model Zoo')
    parser.add_argument('--warmup', type=int, default=5,
                       help='Number of warmup iterations (default: 5)')
    parser.add_argument('--iters', type=int, default=50,
                       help='Number of benchmark iterations (default: 50)')

    args = parser.parse_args()

    # Download sample model if requested
    if args.download_sample:
        model_path = download_sample_model()
        print(f"\nSample model ready at: {model_path}")
        print(f"Run with: python {sys.argv[0]} --model {model_path}")
        return

    # Require model path
    if not args.model:
        parser.error("--model is required (or use --download-sample to get a test model)")

    model_path = Path(args.model)
    if not model_path.exists():
        print(f"Error: Model file not found: {model_path}")
        sys.exit(1)

    # Connect to runtime
    print("Connecting to CoreMLWin runtime service...")
    client = CoreMLWinClient()

    # Check health
    health = client.health()
    print(f"  Status: {'✓ RUNNING' if health.get('ready') else '✗ DOWN'}")
    print(f"  Version: {health.get('version', 'unknown')}\n")

    try:
        if args.compare:
            # Compare providers
            compare_providers(client, str(model_path))
        else:
            # Single test
            result = test_single_inference(client, str(model_path), args.provider)

            if args.benchmark:
                test_warmup_performance(client, result['model_id'],
                                      warmup_iters=args.warmup,
                                      bench_iters=args.iters)

            # Cleanup
            client.unregister_model(result['model_id'])

        print("\n✓ All tests completed successfully!\n")

    except CoreMLWinError as e:
        print(f"\n✗ Test failed: {e}\n")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\n\nTest interrupted by user\n")
        sys.exit(1)


if __name__ == '__main__':
    main()
