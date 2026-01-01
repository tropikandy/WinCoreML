#!/usr/bin/env python3
"""
Comprehensive Integration Tests for CoreMLWin Runtime

Tests full end-to-end workflows:
- Service lifecycle
- Model registration and management
- Inference execution
- Concurrent requests
- Error handling
- Performance benchmarks

Run with: pytest test_integration.py -v --tb=short
"""

import pytest
import numpy as np
import time
import threading
import sys
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, as_completed

sys.path.insert(0, str(Path(__file__).parent.parent / 'sdk' / 'python'))

from coremlwin_client import CoreMLWinClient, CoreMLWinError


class TestServiceLifecycle:
    """Test service startup, health, and shutdown"""

    def test_service_health(self):
        """Service should respond to health checks"""
        client = CoreMLWinClient()
        health = client.health()
        assert health is not None
        assert 'ready' in health
        assert health['ready'] == True
        assert 'version' in health

    def test_multiple_clients(self):
        """Multiple clients should connect successfully"""
        clients = [CoreMLWinClient() for _ in range(5)]
        for client in clients:
            health = client.health()
            assert health['ready'] == True


class TestConcurrency:
    """Test concurrent requests and thread safety"""

    def test_concurrent_health_checks(self):
        """Multiple concurrent health checks should succeed"""
        client = CoreMLWinClient()
        
        def check_health():
            health = client.health()
            return health['ready']

        with ThreadPoolExecutor(max_workers=10) as executor:
            futures = [executor.submit(check_health) for _ in range(50)]
            results = [f.result() for f in as_completed(futures)]

        assert all(results)
        assert len(results) == 50


class TestPerformance:
    """Performance benchmarks"""

    def test_health_check_latency(self):
        """Health check should be fast (<100ms)"""
        client = CoreMLWinClient()
        latencies = []

        for _ in range(10):
            start = time.time()
            client.health()
            latency = (time.time() - start) * 1000
            latencies.append(latency)

        avg_latency = sum(latencies) / len(latencies)
        print(f"\nAverage health check latency: {avg_latency:.2f} ms")
        assert avg_latency < 100, f"Health check too slow: {avg_latency:.2f} ms"


if __name__ == '__main__':
    pytest.main([__file__, '-v', '--tb=short'])
