"""
End-to-End Integration Test

Tests the complete flow of the Universal ML Runtime:
1. Service health check
2. Model registration
3. Inference execution
4. Model listing
5. Model unregistration

Usage:
    # Start the service first:
    .\\runtime\\build\\bin\\Release\\coremlwin_service.exe

    # Then run this test:
    python tests/test_integration.py

Note: This test works with placeholder mode (when ONNX Runtime is not linked).
"""

import sys
import time
import numpy as np
from pathlib import Path

# Add SDK to path
sdk_path = Path(__file__).parent.parent / "sdk" / "python"
sys.path.insert(0, str(sdk_path))

try:
    from coreml_win import RuntimeClient
    from coreml_win.errors import CoreMLWinError
except ImportError as e:
    print(f"Error importing SDK: {e}")
    print("Make sure protobuf files are generated:")
    print("  cd sdk/python")
    print("  protoc --python_out=coreml_win --proto_path=../../protos ../../protos/coremlwin_runtime.proto")
    sys.exit(1)


def print_step(step_num, total, description):
    """Print test step with formatting."""
    print(f"\n[{step_num}/{total}] {description}")
    print("-" * 60)


def test_health_check(client):
    """Test 1: Health check."""
    print_step(1, 6, "Health Check")

    try:
        health = client.health()
        print(f"✓ Service is running")
        print(f"  Version: {health.get('version', 'unknown')}")
        print(f"  Ready: {health.get('ready', False)}")
        return True
    except Exception as e:
        print(f"✗ Health check failed: {e}")
        return False


def test_register_model(client, model_path):
    """Test 2: Register a model."""
    print_step(2, 6, "Model Registration")

    try:
        print(f"Registering model: {model_path}")
        model_id = client.register_model(str(model_path))

        print(f"✓ Model registered successfully")
        print(f"  Model ID: {model_id}")
        return model_id
    except CoreMLWinError as e:
        print(f"✗ Registration failed: {e}")
        return None
    except Exception as e:
        print(f"✗ Unexpected error: {e}")
        return None


def test_list_models(client):
    """Test 3: List registered models."""
    print_step(3, 6, "List Models")

    try:
        models = client.list_models()
        print(f"✓ Found {len(models)} registered model(s)")

        for model in models:
            print(f"\n  Model ID: {model.get('model_id', 'unknown')}")
            print(f"  Format: {model.get('model_format', 'unknown')}")
            print(f"  Inputs: {model.get('input_names', [])}")
            print(f"  Outputs: {model.get('output_names', [])}")

        return True
    except Exception as e:
        print(f"✗ List models failed: {e}")
        return False


def test_predict(client, model_id):
    """Test 4: Run inference."""
    print_step(4, 6, "Inference Execution")

    try:
        # Create dummy input
        print("Creating input tensor...")
        inputs = {
            "input": np.random.randn(1, 3, 224, 224).astype(np.float32)
        }

        print(f"Running inference on model: {model_id}")
        print(f"Input shapes: {{{k}: {v.shape} for k, v in inputs.items()}}}")

        outputs = client.predict(model_id, inputs)

        print(f"✓ Inference completed successfully")
        for name, tensor in outputs.items():
            print(f"  Output '{name}': shape={tensor.shape}, dtype={tensor.dtype}")

        return True
    except CoreMLWinError as e:
        print(f"✗ Inference failed: {e}")
        return False
    except Exception as e:
        print(f"✗ Unexpected error: {e}")
        return False


def test_get_model_info(client, model_id):
    """Test 5: Get model metadata."""
    print_step(5, 6, "Model Metadata Retrieval")

    try:
        info = client.get_model_info(model_id)

        print(f"✓ Retrieved model metadata")
        print(f"  Format: {info.get('format', 'unknown')}")
        print(f"  Input names: {info.get('input_names', [])}")
        print(f"  Output names: {info.get('output_names', [])}")

        if 'benchmark' in info:
            bench = info['benchmark']
            print(f"\n  Benchmark Results:")
            print(f"    Fastest provider: {bench.get('fastest_provider', 'unknown')}")
            print(f"    Baseline latency: {bench.get('baseline_latency_ms', 0):.2f} ms")
            print(f"    Best latency: {bench.get('best_latency_ms', 0):.2f} ms")
            print(f"    Speedup vs CPU: {bench.get('speedup_vs_cpu', 1):.2f}x")

        return True
    except Exception as e:
        print(f"✗ Get model info failed: {e}")
        return False


def test_unregister_model(client, model_id):
    """Test 6: Unregister model."""
    print_step(6, 6, "Model Unregistration")

    try:
        success = client.unregister_model(model_id)

        if success:
            print(f"✓ Model unregistered successfully")
            return True
        else:
            print(f"✗ Unregister returned False")
            return False
    except Exception as e:
        print(f"✗ Unregister failed: {e}")
        return False


def create_test_model():
    """Create a dummy model file for testing."""
    test_dir = Path(__file__).parent
    model_path = test_dir / "test_model.onnx"

    # Create minimal ONNX file (just for testing placeholder mode)
    # In real tests, this would be a valid ONNX model
    if not model_path.exists():
        print(f"\nCreating dummy test model: {model_path}")
        with open(model_path, 'wb') as f:
            f.write(b'ONNX')  # Minimal header
        print(f"✓ Test model created")

    return model_path


def main():
    """Run complete integration test suite."""
    print("=" * 60)
    print("Universal ML Runtime - End-to-End Integration Test")
    print("=" * 60)

    # Create test model
    try:
        model_path = create_test_model()
    except Exception as e:
        print(f"Failed to create test model: {e}")
        return False

    # Connect to service
    print("\nConnecting to runtime service...")
    try:
        client = RuntimeClient()
        print("✓ Client initialized")
    except Exception as e:
        print(f"✗ Failed to initialize client: {e}")
        print("\nMake sure the service is running:")
        print("  .\\runtime\\build\\bin\\Release\\coremlwin_service.exe")
        return False

    # Run tests
    results = []

    # Test 1: Health check
    results.append(("Health Check", test_health_check(client)))

    # Test 2: Register model
    model_id = test_register_model(client, model_path)
    results.append(("Model Registration", model_id is not None))

    if model_id:
        # Test 3: List models
        results.append(("List Models", test_list_models(client)))

        # Test 4: Predict
        results.append(("Inference", test_predict(client, model_id)))

        # Test 5: Get model info
        results.append(("Model Metadata", test_get_model_info(client, model_id)))

        # Test 6: Unregister
        results.append(("Unregister", test_unregister_model(client, model_id)))
    else:
        # Skip dependent tests
        results.extend([
            ("List Models", False),
            ("Inference", False),
            ("Model Metadata", False),
            ("Unregister", False),
        ])

    # Print summary
    print("\n" + "=" * 60)
    print("TEST SUMMARY")
    print("=" * 60)

    passed = sum(1 for _, result in results if result)
    total = len(results)

    for test_name, result in results:
        status = "✓ PASS" if result else "✗ FAIL"
        print(f"{status:8} {test_name}")

    print("-" * 60)
    print(f"Results: {passed}/{total} tests passed")
    print("=" * 60)

    return passed == total


if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)
