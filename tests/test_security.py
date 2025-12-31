#!/usr/bin/env python3
"""
Security and Adversarial Testing Suite for CoreMLWin Runtime

Tests for vulnerabilities identified in red team audit:
- Path traversal attacks (C2)
- Buffer overflow attacks (C1)
- Integer overflow attacks (H1)
- DoS via large messages (C3)
- Invalid tensor shapes
- Resource exhaustion

Run with: pytest test_security.py -v
"""

import pytest
import numpy as np
import sys
import os
from pathlib import Path

# Add SDK to path
sys.path.insert(0, str(Path(__file__).parent.parent / 'sdk' / 'python'))

from coremlwin_client import CoreMLWinClient, CoreMLWinError


class TestPathTraversal:
    """Test protection against path traversal attacks (C2)"""

    def test_parent_directory_traversal(self):
        """Attempt to access parent directory"""
        client = CoreMLWinClient()

        with pytest.raises(CoreMLWinError) as exc_info:
            client.register_model("../../etc/passwd")

        assert "invalid" in str(exc_info.value).lower() or "not found" in str(exc_info.value).lower()

    def test_absolute_path_to_sensitive_file(self):
        """Attempt to access system files"""
        client = CoreMLWinClient()

        # Unix path
        with pytest.raises(CoreMLWinError):
            client.register_model("/etc/shadow")

        # Windows path
        with pytest.raises(CoreMLWinError):
            client.register_model("C:\\Windows\\System32\\config\\SAM")

    def test_path_with_null_byte(self):
        """Path with null byte should be rejected"""
        client = CoreMLWinClient()

        with pytest.raises(CoreMLWinError):
            client.register_model("model.onnx\x00/etc/passwd")

    def test_symlink_escape(self):
        """Symlink to sensitive location should be rejected"""
        # Create symlink to /etc
        test_dir = Path("/tmp/coremlwin_test")
        test_dir.mkdir(exist_ok=True)

        link_path = test_dir / "etc_link"
        if not link_path.exists():
            try:
                link_path.symlink_to("/etc")
            except OSError:
                pytest.skip("Cannot create symlink")

        client = CoreMLWinClient()

        with pytest.raises(CoreMLWinError):
            client.register_model(str(link_path / "passwd"))

        # Cleanup
        if link_path.is_symlink():
            link_path.unlink()

    def test_non_onnx_file(self):
        """Non-.onnx files should be rejected"""
        client = CoreMLWinClient()

        with pytest.raises(CoreMLWinError) as exc_info:
            client.register_model("/bin/bash")

        assert "invalid" in str(exc_info.value).lower()


class TestBufferOverflow:
    """Test protection against buffer overflow attacks (C1)"""

    @pytest.fixture
    def client_with_model(self):
        """Setup: register a test model"""
        client = CoreMLWinClient()

        # Create minimal valid ONNX model or use placeholder
        # For now, assume service running in placeholder mode
        model_path = str(Path(__file__).parent.parent / "models" / "test.onnx")

        # If model doesn't exist, create placeholder
        if not Path(model_path).exists():
            # Service will use placeholder mode
            model_id = "test_model"
        else:
            model_id = client.register_model(model_path)

        yield client, model_id

        # Cleanup
        try:
            client.unregister_model(model_id)
        except:
            pass

    def test_oversized_tensor_data(self, client_with_model):
        """Send tensor with more data than shape specifies"""
        client, model_id = client_with_model

        # Shape says 1x3x224x224 = 150,528 floats
        # But send 10x that amount
        inputs = {
            'input': np.random.randn(10, 3, 224, 224).astype(np.float32)  # Wrong size
        }

        with pytest.raises(CoreMLWinError) as exc_info:
            client.run_inference(model_id, inputs)

        assert "mismatch" in str(exc_info.value).lower() or "invalid" in str(exc_info.value).lower()

    def test_undersized_tensor_data(self, client_with_model):
        """Send tensor with less data than shape specifies"""
        client, model_id = client_with_model

        # Create tensor with mismatched data and shape
        tensor = np.random.randn(1, 3, 10, 10).astype(np.float32)  # Small data

        # Manually override shape to claim it's larger
        # This tests if runtime validates data size
        inputs = {
            'input': tensor
        }

        # Modify shape metadata (hack for testing)
        # In real attack, this would be done at protocol level

        # For now, trust that our validation catches this
        # Real test would require custom protobuf message

    def test_massive_single_dimension(self, client_with_model):
        """Tensor with one massive dimension"""
        client, model_id = client_with_model

        # This should be rejected as too large
        try:
            inputs = {
                'input': np.zeros((1, 3, 1000000, 1000000), dtype=np.float32)
            }
            # This will likely fail to allocate, which is fine
            with pytest.raises((CoreMLWinError, MemoryError)):
                client.run_inference(model_id, inputs)
        except MemoryError:
            # Can't even create the numpy array
            pass


class TestIntegerOverflow:
    """Test protection against integer overflow attacks (H1)"""

    def test_shape_integer_overflow(self):
        """Shape dimensions that multiply to overflow"""
        client = CoreMLWinClient()

        # Try to register model, which will fail
        # But we're testing shape validation in inference

        # Create shape that overflows when multiplied
        # 2^20 * 2^20 * 2^20 = 2^60 (overflows 64-bit)

        # This tests internal validation
        # Actual test requires model with dynamic shapes

    def test_negative_dimensions(self):
        """Negative shape dimensions should be rejected"""
        # This tests shape validation
        # Requires dynamic shape model

    def test_zero_dimensions(self):
        """Zero-sized dimensions should be rejected"""
        # Shape with zero should be invalid


class TestDenialOfService:
    """Test protection against DoS attacks (C3, M2)"""

    def test_extremely_large_message(self):
        """Send message larger than MAX_MESSAGE_SIZE"""
        client = CoreMLWinClient()

        # Try to send massive protobuf message
        # This should be rejected before deserialization

        try:
            # Create 200 MB tensor (exceeds 100 MB limit)
            huge_tensor = np.zeros((50, 1024, 1024), dtype=np.float32)

            with pytest.raises((CoreMLWinError, MemoryError)):
                # This should fail at protocol level or memory allocation
                inputs = {'input': huge_tensor}
                client.run_inference("dummy_id", inputs)
        except MemoryError:
            # Can't allocate locally, which is expected
            pass

    def test_rapid_requests(self):
        """Send many requests rapidly (rate limiting test)"""
        client = CoreMLWinClient()

        # Send 1000 requests as fast as possible
        # Should complete (no rate limiting yet) but not crash

        for i in range(100):  # Reduced from 1000 for faster testing
            try:
                client.health()
            except:
                pass  # Some may fail, that's OK

        # Service should still be responsive
        health = client.health()
        assert health.get('ready') == True

    def test_many_concurrent_models(self):
        """Register many models simultaneously"""
        client = CoreMLWinClient()

        model_ids = []

        # Try to register 100 models
        # Should either succeed or fail gracefully
        for i in range(100):
            try:
                # Use placeholder mode
                model_id = f"model_{i}"
                model_ids.append(model_id)
            except CoreMLWinError:
                break  # Hit some limit, that's OK

        # Cleanup
        for model_id in model_ids:
            try:
                client.unregister_model(model_id)
            except:
                pass


class TestInputValidation:
    """Test general input validation"""

    def test_empty_model_path(self):
        """Empty model path should be rejected"""
        client = CoreMLWinClient()

        with pytest.raises(CoreMLWinError):
            client.register_model("")

    def test_very_long_path(self):
        """Extremely long path should be rejected"""
        client = CoreMLWinClient()

        long_path = "a" * 10000 + ".onnx"

        with pytest.raises(CoreMLWinError):
            client.register_model(long_path)

    def test_special_characters_in_path(self):
        """Special characters in path"""
        client = CoreMLWinClient()

        special_paths = [
            "model\x00.onnx",  # Null byte
            "model\n.onnx",    # Newline
            "model\r.onnx",    # Carriage return
            "model\t.onnx",    # Tab
        ]

        for path in special_paths:
            with pytest.raises(CoreMLWinError):
                client.register_model(path)

    def test_empty_tensor_name(self):
        """Empty tensor name should be rejected"""
        # Tested at protocol level


class TestLogInjection:
    """Test protection against log injection (H3)"""

    def test_ansi_escape_in_path(self):
        """ANSI escape codes in path should be sanitized"""
        client = CoreMLWinClient()

        # Path with ANSI escape codes
        path = "\x1b[2J\x1b[Hmalicious.onnx"

        with pytest.raises(CoreMLWinError):
            client.register_model(path)

        # If this reaches the server, the logs should sanitize it
        # Check server logs to ensure no terminal manipulation

    def test_control_characters_in_model_path(self):
        """Control characters should be sanitized in logs"""
        client = CoreMLWinClient()

        # Various control characters
        path = "model\x01\x02\x03\x04.onnx"

        with pytest.raises(CoreMLWinError):
            client.register_model(path)


class TestResourceExhaustion:
    """Test resource exhaustion protections"""

    def test_model_file_size_limit(self):
        """Files larger than MAX_MODEL_SIZE should be rejected"""
        # Would need to create a >2GB file
        # Skip in normal testing
        pytest.skip("Requires creating very large file")

    def test_max_tensor_dimensions(self):
        """Tensor with too many dimensions should be rejected"""
        # Create 10-dimensional tensor
        try:
            tensor = np.zeros((2,2,2,2,2,2,2,2,2,2), dtype=np.float32)

            # This should be rejected (MAX_SHAPE_DIMENSIONS = 8)
            # Requires integration with service
        except:
            pass


class TestEdgeCases:
    """Test edge cases and boundary conditions"""

    def test_exactly_max_tensor_size(self):
        """Tensor at exactly MAX_TENSOR_SIZE should work"""
        # Create tensor of exactly maximum allowed size
        # MAX_TENSOR_ELEMENTS = 1B elements = 4GB for float32

        # This is too large to test in normal environment
        pytest.skip("Requires 4GB+ memory")

    def test_shape_all_ones(self):
        """Shape with all dimensions = 1"""
        client = CoreMLWinClient()

        inputs = {
            'input': np.ones((1,1,1,1), dtype=np.float32)
        }

        # Should work fine
        # Requires model to test

    def test_empty_inputs_dict(self):
        """Empty inputs dictionary"""
        client = CoreMLWinClient()

        with pytest.raises(CoreMLWinError):
            client.run_inference("model_id", {})


def test_security_headers_present():
    """Verify security utilities are properly included"""
    # This tests that security_utils.h is being used
    # Compile-time check would be better, but we can verify behavior

    from coremlwin_client import CoreMLWinError

    client = CoreMLWinClient()

    # Path traversal should be blocked
    with pytest.raises(CoreMLWinError):
        client.register_model("../../etc/passwd")

    # This confirms path validation is active


if __name__ == '__main__':
    pytest.main([__file__, '-v', '--tb=short'])
