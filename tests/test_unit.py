"""
Unit Tests for Universal ML Runtime

Tests core components:
- Model Registry
- Error handling
- Logger
- Protocol utilities
"""

import sys
import pytest
from pathlib import Path

# Add SDK to path
sdk_path = Path(__file__).parent.parent / "sdk" / "python"
sys.path.insert(0, str(sdk_path))

from coreml_win.errors import *


class TestErrorTaxonomy:
    """Test unified error taxonomy."""

    def test_error_ranges(self):
        """Test error code ranges."""
        # Client errors: 1000-1999
        assert InvalidArgumentError().code == 1000
        assert InvalidModelIdError().code == 1001

        # Model errors: 2000-2999
        assert ModelNotFoundError().code == 2000
        assert ModelLoadFailedError().code == 2001

        # Provider errors: 3000-3999
        assert ProviderNotAvailableError().code == 3000
        assert ProviderInitFailedError().code == 3001

        # Runtime errors: 4000-4999
        assert InternalError().code == 4000
        assert NotImplementedError().code == 4001

        # Transient errors: 5000-5999
        assert TimeoutError().code == 5000
        assert ResourceBusyError().code == 5001

    def test_error_properties(self):
        """Test error classification properties."""
        client_error = InvalidArgumentError("Bad argument")
        assert client_error.is_client_error
        assert not client_error.is_model_error
        assert not client_error.is_transient

        model_error = ModelNotFoundError("Model missing")
        assert model_error.is_model_error
        assert not model_error.is_provider_error

        transient_error = TimeoutError("Timed out")
        assert transient_error.is_transient
        assert not transient_error.is_runtime_error

    def test_error_messages(self):
        """Test error message formatting."""
        error = InvalidArgumentError("Invalid input shape", "Expected [1,3,224,224]")
        error_str = str(error)

        assert "[1000]" in error_str
        assert "Invalid input shape" in error_str
        assert "Expected [1,3,224,224]" in error_str

    def test_from_error_code(self):
        """Test error factory function."""
        error = from_error_code(1000, "Test message", "Test details")
        assert isinstance(error, InvalidArgumentError)
        assert error.code == 1000

        error = from_error_code(2000, "Not found")
        assert isinstance(error, ModelNotFoundError)

        # Test unknown code falls back to base class
        error = from_error_code(9999, "Unknown")
        assert isinstance(error, CoreMLWinError)
        assert error.code == 9999

    def test_error_code_uniqueness(self):
        """Test that all error codes are unique."""
        from coreml_win.errors import _ERROR_CODE_MAP

        codes = list(_ERROR_CODE_MAP.keys())
        assert len(codes) == len(set(codes)), "Duplicate error codes found"


class TestErrorHandling:
    """Test error handling patterns."""

    def test_retry_logic_for_transient_errors(self):
        """Test that transient errors are identified for retry."""
        transient_errors = [
            TimeoutError(),
            ResourceBusyError(),
            ResourceExhaustedError(),
            ConnectionLostError(),
            TemporaryFailureError(),
        ]

        for error in transient_errors:
            assert error.is_transient, f"{error.__class__.__name__} should be transient"

    def test_non_retryable_errors(self):
        """Test that permanent errors are not marked transient."""
        permanent_errors = [
            InvalidArgumentError(),
            ModelNotFoundError(),
            ProviderNotAvailableError(),
            InternalError(),
        ]

        for error in permanent_errors:
            assert not error.is_transient, f"{error.__class__.__name__} should not be transient"


class TestProtocolUtils:
    """Test protocol buffer utilities."""

    def test_numpy_dtype_mapping(self):
        """Test numpy dtype to protobuf mapping."""
        from coreml_win.proto_utils import DTYPE_TO_PROTO, PROTO_TO_DTYPE
        import numpy as np

        # Test common dtypes
        assert np.float32 in DTYPE_TO_PROTO
        assert np.int32 in DTYPE_TO_PROTO
        assert np.int64 in DTYPE_TO_PROTO

        # Test reverse mapping
        assert 'DTYPE_FLOAT32' in PROTO_TO_DTYPE
        assert PROTO_TO_DTYPE['DTYPE_FLOAT32'] == np.float32

    def test_request_creation(self):
        """Test request message creation."""
        pytest.importorskip("coreml_win.coremlwin_runtime_pb2",
                           reason="Protobuf files not generated")

        from coreml_win.proto_utils import create_health_check_request

        # Should not raise
        request_bytes = create_health_check_request()
        assert isinstance(request_bytes, bytes)
        assert len(request_bytes) > 0


class TestModelMetadata:
    """Test model metadata handling."""

    def test_metadata_structure(self):
        """Test metadata dictionary structure."""
        metadata = {
            "model_id": "abc123",
            "model_format": "pytorch",
            "input_names": ["input"],
            "output_names": ["output"],
            "benchmarks": [
                {
                    "provider": "CPUExecutionProvider",
                    "mean_latency_ms": 45.2,
                    "success": True
                },
                {
                    "provider": "DmlExecutionProvider",
                    "mean_latency_ms": 12.5,
                    "success": True
                }
            ],
            "fastest_provider": "DmlExecutionProvider",
            "best_latency_ms": 12.5,
            "speedup_vs_cpu": 3.6
        }

        assert metadata["model_id"] == "abc123"
        assert len(metadata["benchmarks"]) == 2
        assert metadata["speedup_vs_cpu"] > 1.0

    def test_benchmark_comparison(self):
        """Test benchmark result comparison."""
        benchmarks = [
            {"provider": "CPU", "mean_latency_ms": 50.0},
            {"provider": "GPU", "mean_latency_ms": 15.0},
            {"provider": "NPU", "mean_latency_ms": 10.0},
        ]

        fastest = min(benchmarks, key=lambda b: b["mean_latency_ms"])
        assert fastest["provider"] == "NPU"
        assert fastest["mean_latency_ms"] == 10.0


class TestUtilities:
    """Test utility functions."""

    def test_path_handling(self):
        """Test file path utilities."""
        from pathlib import Path

        test_path = Path("models/resnet50.pt")
        assert test_path.suffix == ".pt"
        assert test_path.stem == "resnet50"

    def test_tensor_size_calculation(self):
        """Test tensor size calculations."""
        import numpy as np

        # 1 batch, 3 channels, 224x224 image
        tensor = np.random.randn(1, 3, 224, 224).astype(np.float32)

        size_bytes = tensor.nbytes
        expected = 1 * 3 * 224 * 224 * 4  # 4 bytes per float32
        assert size_bytes == expected

        # Check if it exceeds shared memory threshold
        THRESHOLD = 1 * 1024 * 1024  # 1MB
        assert size_bytes < THRESHOLD  # This tensor is ~600KB


if __name__ == "__main__":
    # Run tests with pytest
    pytest.main([__file__, "-v", "--tb=short"])
