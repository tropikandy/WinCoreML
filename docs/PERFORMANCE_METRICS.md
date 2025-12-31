# Performance Monitoring & Metrics

## Overview

Track and analyze runtime performance to identify bottlenecks and optimize ML inference workloads.

## Key Metrics

### 1. Inference Latency
- **Definition**: Time from request received to response sent
- **Target**: < 100ms for real-time applications
- **Breakdown**:
  - Deserialization: Protobuf → Internal structures
  - Session acquisition: Pool wait time
  - Inference: Actual model execution
  - Serialization: Internal → Protobuf

### 2. Throughput
- **Definition**: Requests processed per second
- **Target**: Depends on hardware and model size
- **Formula**: `throughput = 1000 / avg_latency_ms`

### 3. Resource Utilization
- **CPU Usage**: Runtime process CPU %
- **Memory**: Working set size
- **GPU Utilization**: For DirectML/CUDA providers

### 4. Provider Performance
- **Per-provider latency**: Track which provider is fastest
- **Fallback rate**: How often fallback to CPU
- **Provider errors**: Failures per provider

## Implementation

### Metrics Collection

```cpp
// runtime/metrics/metrics_collector.h

struct InferenceMetrics {
    std::chrono::microseconds deserialization_time;
    std::chrono::microseconds session_acquisition_time;
    std::chrono::microseconds inference_time;
    std::chrono::microseconds serialization_time;
    std::chrono::microseconds total_time;

    std::string model_id;
    std::string provider_used;
    bool success;
    std::string error_code;

    size_t input_tensor_bytes;
    size_t output_tensor_bytes;
};

class MetricsCollector {
public:
    void RecordInference(const InferenceMetrics& metrics);

    // Get statistics for a time window
    MetricsStats GetStats(std::chrono::seconds window = std::chrono::seconds(60));

    // Export metrics in Prometheus format
    std::string ExportPrometheus();

private:
    std::mutex mutex_;
    std::deque<InferenceMetrics> metrics_;
    std::chrono::steady_clock::time_point start_time_;
};

struct MetricsStats {
    size_t total_requests;
    size_t successful_requests;
    size_t failed_requests;

    double avg_latency_ms;
    double p50_latency_ms;
    double p95_latency_ms;
    double p99_latency_ms;

    double throughput_rps;

    std::map<std::string, size_t> provider_usage;
    std::map<std::string, double> provider_avg_latency;
};
```

### Usage in RuntimeState

```cpp
CmwErrorCode RuntimeState::Predict(
    const PredictRequest& request,
    PredictResponse& response
) {
    InferenceMetrics metrics;
    metrics.model_id = request.model_id;

    auto total_start = std::chrono::high_resolution_clock::now();

    // Deserialization (already done by caller)
    auto deser_time = total_start;
    metrics.deserialization_time = std::chrono::duration_cast<std::chrono::microseconds>(
        total_start - deser_time
    );

    // Session acquisition
    auto acq_start = std::chrono::high_resolution_clock::now();
    std::shared_ptr<PooledSession> session;
    CmwErrorCode result = AcquireSession(request.model_id, session);
    metrics.session_acquisition_time = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - acq_start
    );

    if (result != CMW_SUCCESS) {
        metrics.success = false;
        metrics.error_code = std::to_string(result);
        metrics_collector_.RecordInference(metrics);
        return result;
    }

    // Inference
    auto inf_start = std::chrono::high_resolution_clock::now();
    result = session->executor->RunInference(request.inputs, response.outputs);
    metrics.inference_time = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - inf_start
    );

    metrics.provider_used = response.provider_used;
    metrics.success = (result == CMW_SUCCESS);

    // Total time
    metrics.total_time = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - total_start
    );

    // Record metrics
    metrics_collector_.RecordInference(metrics);

    ReleaseSession(request.model_id, session);
    return result;
}
```

## Python SDK Integration

```python
# sdk/python/coreml_win/metrics.py

from dataclasses import dataclass
from typing import Dict, List
import time

@dataclass
class InferenceMetrics:
    """Metrics for a single inference request."""
    model_id: str
    latency_ms: float
    throughput: float  # inferences/sec
    provider_used: str
    input_size_bytes: int
    output_size_bytes: int
    timestamp: float

class MetricsTracker:
    """Track client-side metrics."""

    def __init__(self, window_size: int = 100):
        self.window_size = window_size
        self.metrics: List[InferenceMetrics] = []

    def record(self, metrics: InferenceMetrics):
        """Record inference metrics."""
        self.metrics.append(metrics)

        # Keep only recent metrics
        if len(self.metrics) > self.window_size:
            self.metrics = self.metrics[-self.window_size:]

    def get_stats(self) -> Dict:
        """Get summary statistics."""
        if not self.metrics:
            return {}

        latencies = [m.latency_ms for m in self.metrics]

        return {
            "count": len(self.metrics),
            "avg_latency_ms": sum(latencies) / len(latencies),
            "min_latency_ms": min(latencies),
            "max_latency_ms": max(latencies),
            "p50_latency_ms": sorted(latencies)[len(latencies) // 2],
            "p95_latency_ms": sorted(latencies)[int(len(latencies) * 0.95)],
            "avg_throughput": sum(m.throughput for m in self.metrics) / len(self.metrics),
        }

    def print_summary(self):
        """Print metrics summary."""
        stats = self.get_stats()
        if not stats:
            print("No metrics recorded")
            return

        print("\nPerformance Metrics:")
        print(f"  Requests: {stats['count']}")
        print(f"  Avg Latency: {stats['avg_latency_ms']:.2f} ms")
        print(f"  P50: {stats['p50_latency_ms']:.2f} ms")
        print(f"  P95: {stats['p95_latency_ms']:.2f} ms")
        print(f"  Throughput: {stats['avg_throughput']:.2f} req/sec")
```

## Monitoring Dashboard (CLI)

```python
# tools/metrics_monitor.py

import time
from coreml_win import RuntimeClient

def monitor_metrics(interval_sec=5):
    """Monitor runtime metrics in real-time."""
    client = RuntimeClient()

    while True:
        stats = client.get_metrics()

        print("\n" + "=" * 60)
        print(f"Timestamp: {time.strftime('%H:%M:%S')}")
        print("=" * 60)

        print(f"\nRequests (last 60s):")
        print(f"  Total:      {stats['total_requests']}")
        print(f"  Successful: {stats['successful_requests']}")
        print(f"  Failed:     {stats['failed_requests']}")

        print(f"\nLatency:")
        print(f"  Average: {stats['avg_latency_ms']:.2f} ms")
        print(f"  P50:     {stats['p50_latency_ms']:.2f} ms")
        print(f"  P95:     {stats['p95_latency_ms']:.2f} ms")
        print(f"  P99:     {stats['p99_latency_ms']:.2f} ms")

        print(f"\nThroughput: {stats['throughput_rps']:.2f} req/sec")

        print(f"\nProvider Usage:")
        for provider, count in stats['provider_usage'].items():
            avg_lat = stats['provider_avg_latency'][provider]
            pct = (count / stats['total_requests']) * 100
            print(f"  {provider:25s}: {count:4d} ({pct:5.1f}%)  {avg_lat:6.2f} ms avg")

        time.sleep(interval_sec)


if __name__ == "__main__":
    monitor_metrics()
```

## Prometheus Export

```cpp
std::string MetricsCollector::ExportPrometheus() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ostringstream oss;

    // Request count
    oss << "# HELP coremlwin_requests_total Total number of requests\n";
    oss << "# TYPE coremlwin_requests_total counter\n";
    oss << "coremlwin_requests_total " << metrics_.size() << "\n\n";

    // Success rate
    size_t success_count = std::count_if(metrics_.begin(), metrics_.end(),
        [](const InferenceMetrics& m) { return m.success; });

    oss << "# HELP coremlwin_requests_success Successful requests\n";
    oss << "# TYPE coremlwin_requests_success counter\n";
    oss << "coremlwin_requests_success " << success_count << "\n\n";

    // Latency histogram
    oss << "# HELP coremlwin_latency_seconds Inference latency\n";
    oss << "# TYPE coremlwin_latency_seconds histogram\n";

    // Calculate percentiles
    std::vector<double> latencies;
    for (const auto& m : metrics_) {
        latencies.push_back(m.total_time.count() / 1e6);  // Convert to seconds
    }
    std::sort(latencies.begin(), latencies.end());

    oss << "coremlwin_latency_seconds{quantile=\"0.5\"} "
        << latencies[latencies.size() / 2] << "\n";
    oss << "coremlwin_latency_seconds{quantile=\"0.95\"} "
        << latencies[static_cast<size_t>(latencies.size() * 0.95)] << "\n";
    oss << "coremlwin_latency_seconds{quantile=\"0.99\"} "
        << latencies[static_cast<size_t>(latencies.size() * 0.99)] << "\n";

    return oss.str();
}
```

## Performance Testing

```python
def benchmark_model(model_path: str, num_requests: int = 100):
    """Benchmark a model."""
    client = RuntimeClient()
    model_id = client.register_model(model_path)

    inputs = {"input": np.random.randn(1, 3, 224, 224).astype(np.float32)}

    latencies = []

    # Warmup
    for _ in range(10):
        client.predict(model_id, inputs)

    # Benchmark
    for i in range(num_requests):
        start = time.perf_counter()
        client.predict(model_id, inputs)
        latencies.append((time.perf_counter() - start) * 1000)

    # Statistics
    avg = sum(latencies) / len(latencies)
    p50 = sorted(latencies)[len(latencies) // 2]
    p95 = sorted(latencies)[int(len(latencies) * 0.95)]
    p99 = sorted(latencies)[int(len(latencies) * 0.99)]

    print(f"\nBenchmark Results ({num_requests} requests):")
    print(f"  Avg:  {avg:.2f} ms")
    print(f"  P50:  {p50:.2f} ms")
    print(f"  P95:  {p95:.2f} ms")
    print(f"  P99:  {p99:.2f} ms")
    print(f"  Throughput: {1000 / avg:.2f} req/sec")
```

## Alerting Rules

```yaml
# configs/alerting.yaml

alerts:
  - name: HighLatency
    condition: p95_latency_ms > 100
    severity: warning
    message: "P95 latency exceeded 100ms"

  - name: LowThroughput
    condition: throughput_rps < 10
    severity: warning
    message: "Throughput below 10 req/sec"

  - name: HighErrorRate
    condition: (failed_requests / total_requests) > 0.05
    severity: critical
    message: "Error rate above 5%"

  - name: ProviderFallback
    condition: provider_usage['CPUExecutionProvider'] > 0.9
    severity: info
    message: "Falling back to CPU for >90% of requests"
```

## Best Practices

1. **Always measure in production-like conditions**
   - Real data distributions
   - Actual hardware
   - Production load patterns

2. **Track trends over time**
   - Daily/weekly reports
   - Detect regressions early

3. **A/B testing**
   - Compare providers
   - Test optimizations

4. **Profile end-to-end**
   - Not just inference time
   - Include serialization, IPC, etc.

## References

- [Prometheus Best Practices](https://prometheus.io/docs/practices/)
- [Performance Testing](https://en.wikipedia.org/wiki/Software_performance_testing)
- [Latency Numbers Every Programmer Should Know](https://gist.github.com/jboner/2841832)
