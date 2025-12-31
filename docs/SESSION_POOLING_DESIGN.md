# ONNX Session Pooling Design

## Overview

Implement session pooling to handle concurrent inference requests efficiently without creating/destroying ONNX Runtime sessions for each request.

## Problem Statement

Current implementation:
- One executor per model_id (created on first use)
- Sequential request processing
- No concurrent inference support
- Session creation overhead on first prediction

## Goals

1. Support concurrent inference requests
2. Reuse ONNX Runtime sessions across requests
3. Minimize session creation overhead
4. Automatic scaling based on load

## Architecture

```
┌──────────────────────────────────────────────────────────┐
│                    Runtime State                          │
│  ┌────────────────────────────────────────────────────┐  │
│  │         Session Pool Manager                        │  │
│  │  ┌───────────────────────────────────────────────┐ │  │
│  │  │  Model: model_abc                             │ │  │
│  │  │  ┌─────────┐  ┌─────────┐  ┌─────────┐       │ │  │
│  │  │  │Session 1│  │Session 2│  │Session 3│       │ │  │
│  │  │  │  IDLE   │  │  BUSY   │  │  IDLE   │       │ │  │
│  │  │  └─────────┘  └─────────┘  └─────────┘       │ │  │
│  │  │  Min: 1, Max: 8, Current: 3                  │ │  │
│  │  └───────────────────────────────────────────────┘ │  │
│  │  ┌───────────────────────────────────────────────┐ │  │
│  │  │  Model: model_xyz                             │ │  │
│  │  │  ┌─────────┐  ┌─────────┐                    │ │  │
│  │  │  │Session 1│  │Session 2│                    │ │  │
│  │  │  │  BUSY   │  │  BUSY   │                    │ │  │
│  │  │  └─────────┘  └─────────┘                    │ │  │
│  │  │  Min: 1, Max: 4, Current: 2                  │ │  │
│  │  └───────────────────────────────────────────────┘ │  │
│  └────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────┘
```

## Design

### 1. Session Pool Per Model

```cpp
// runtime/execution/session_pool.h

enum class SessionState {
    IDLE,
    BUSY,
    INITIALIZING,
    ERROR
};

struct PooledSession {
    std::unique_ptr<ONNXExecutor> executor;
    SessionState state;
    std::chrono::steady_clock::time_point last_used;
    size_t use_count;
    std::thread::id thread_id;  // For debugging
};

class SessionPool {
public:
    /**
     * Create session pool for a model
     * @param model_path Path to ONNX model
     * @param provider_name Execution provider
     * @param config Pool configuration
     */
    SessionPool(
        const std::string& model_path,
        const std::string& provider_name,
        const PoolConfig& config
    );

    /**
     * Acquire a session for inference
     * Blocks if all sessions busy and max_size reached
     * @param timeout_ms Max wait time
     * @return Session handle or nullptr on timeout
     */
    std::shared_ptr<PooledSession> Acquire(int timeout_ms = 5000);

    /**
     * Release session back to pool
     * @param session Session to release
     */
    void Release(std::shared_ptr<PooledSession> session);

    /**
     * Get pool statistics
     */
    PoolStats GetStats() const;

    /**
     * Resize pool (add or remove sessions)
     */
    void Resize(size_t new_size);

private:
    void CreateSession();
    void DestroyIdleSessions(size_t keep_count);
    void MonitorAndScale();

    std::string model_path_;
    std::string provider_name_;
    PoolConfig config_;

    std::mutex mutex_;
    std::condition_variable session_available_;
    std::vector<std::shared_ptr<PooledSession>> sessions_;

    // Statistics
    size_t total_acquisitions_ = 0;
    size_t total_waits_ = 0;
    std::chrono::microseconds total_wait_time_{0};
};

struct PoolConfig {
    size_t min_size = 1;          // Minimum sessions to keep
    size_t max_size = 4;          // Maximum sessions
    size_t initial_size = 1;      // Sessions created on startup
    int idle_timeout_ms = 60000;  // Destroy idle sessions after this
    bool auto_scale = true;       // Auto-create sessions on demand
};

struct PoolStats {
    size_t total_sessions;
    size_t idle_sessions;
    size_t busy_sessions;
    size_t total_acquisitions;
    size_t total_waits;
    double avg_wait_time_ms;
    size_t use_count_max;
    size_t use_count_min;
};
```

### 2. Implementation

```cpp
// runtime/execution/session_pool.cpp

SessionPool::SessionPool(
    const std::string& model_path,
    const std::string& provider_name,
    const PoolConfig& config
) : model_path_(model_path),
    provider_name_(provider_name),
    config_(config)
{
    // Create initial sessions
    for (size_t i = 0; i < config_.initial_size; i++) {
        CreateSession();
    }

    // Start monitor thread if auto-scaling enabled
    if (config_.auto_scale) {
        monitor_thread_ = std::thread(&SessionPool::MonitorAndScale, this);
    }
}

std::shared_ptr<PooledSession> SessionPool::Acquire(int timeout_ms) {
    std::unique_lock<std::mutex> lock(mutex_);

    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeout_ms);

    while (true) {
        // Find idle session
        for (auto& session : sessions_) {
            if (session->state == SessionState::IDLE) {
                session->state = SessionState::BUSY;
                session->last_used = std::chrono::steady_clock::now();
                session->use_count++;
                session->thread_id = std::this_thread::get_id();

                total_acquisitions_++;
                return session;
            }
        }

        // No idle sessions available

        // Can we create more?
        if (sessions_.size() < config_.max_size) {
            CreateSession();
            continue;  // Try again with new session
        }

        // Wait for a session to become available
        total_waits_++;
        auto wait_start = std::chrono::steady_clock::now();

        if (session_available_.wait_until(lock, deadline) == std::cv_status::timeout) {
            // Timeout
            return nullptr;
        }

        total_wait_time_ += std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - wait_start
        );
    }
}

void SessionPool::Release(std::shared_ptr<PooledSession> session) {
    std::lock_guard<std::mutex> lock(mutex_);

    session->state = SessionState::IDLE;
    session->thread_id = std::thread::id();

    // Notify waiting threads
    session_available_.notify_one();
}

void SessionPool::CreateSession() {
    // Must be called with mutex held

    auto session = std::make_shared<PooledSession>();
    session->executor = std::make_unique<ONNXExecutor>();
    session->state = SessionState::INITIALIZING;
    session->use_count = 0;

    sessions_.push_back(session);

    // Initialize in background to avoid blocking
    auto result = session->executor->LoadModel(model_path_, provider_name_);

    if (result == CMW_SUCCESS) {
        session->state = SessionState::IDLE;
    } else {
        session->state = SessionState::ERROR;
    }
}

void SessionPool::MonitorAndScale() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(10));

        std::lock_guard<std::mutex> lock(mutex_);

        // Calculate load
        size_t busy_count = 0;
        for (const auto& session : sessions_) {
            if (session->state == SessionState::BUSY) {
                busy_count++;
            }
        }

        float load = static_cast<float>(busy_count) / sessions_.size();

        // Scale up if heavily loaded
        if (load > 0.8 && sessions_.size() < config_.max_size) {
            CreateSession();
            std::cout << "SessionPool: Scaled up to " << sessions_.size() << " sessions\n";
        }

        // Scale down if underutilized
        if (load < 0.2 && sessions_.size() > config_.min_size) {
            DestroyIdleSessions(config_.min_size);
            std::cout << "SessionPool: Scaled down to " << sessions_.size() << " sessions\n";
        }

        // Destroy idle sessions that haven't been used
        auto now = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(config_.idle_timeout_ms);

        for (auto it = sessions_.begin(); it != sessions_.end();) {
            if ((*it)->state == SessionState::IDLE &&
                now - (*it)->last_used > timeout &&
                sessions_.size() > config_.min_size)
            {
                it = sessions_.erase(it);
                std::cout << "SessionPool: Removed idle session\n";
            } else {
                ++it;
            }
        }
    }
}
```

### 3. Pool Manager

```cpp
// runtime/execution/pool_manager.h

class PoolManager {
public:
    /**
     * Get or create session pool for a model
     */
    std::shared_ptr<SessionPool> GetPool(
        const std::string& model_id,
        const std::string& model_path,
        const std::string& provider_name
    );

    /**
     * Remove pool for a model
     */
    void RemovePool(const std::string& model_id);

    /**
     * Get statistics for all pools
     */
    std::map<std::string, PoolStats> GetAllStats() const;

private:
    std::mutex mutex_;
    std::map<std::string, std::shared_ptr<SessionPool>> pools_;
};
```

### 4. Integration with RuntimeState

```cpp
// runtime/service/runtime_state.h

class RuntimeState {
public:
    // Replace GetOrCreateExecutor with:
    CmwErrorCode AcquireSession(
        const std::string& model_id,
        std::shared_ptr<PooledSession>& session
    );

    void ReleaseSession(
        const std::string& model_id,
        std::shared_ptr<PooledSession> session
    );

private:
    PoolManager pool_manager_;
};
```

```cpp
// runtime/service/runtime_state.cpp

CmwErrorCode RuntimeState::Predict(
    const PredictRequest& request,
    PredictResponse& response
) {
    auto start_time = std::chrono::high_resolution_clock::now();

    // Acquire session from pool
    std::shared_ptr<PooledSession> session;
    CmwErrorCode result = AcquireSession(request.model_id, session);

    if (result != CMW_SUCCESS) {
        response.error_code = result;
        response.error_message = "Failed to acquire session";
        return result;
    }

    // RAII-style release
    auto release_guard = std::make_unique<ScopeGuard>([this, &request, &session]() {
        ReleaseSession(request.model_id, session);
    });

    // Run inference
    result = session->executor->RunInference(request.inputs, response.outputs);

    // ... rest of implementation
}
```

## Configuration

```yaml
# configs/session_pooling.yaml

models:
  # Default configuration
  default:
    min_sessions: 1
    max_sessions: 4
    initial_sessions: 1
    idle_timeout_ms: 60000
    auto_scale: true

  # Per-model overrides
  high_traffic_model:
    min_sessions: 2
    max_sessions: 8
    initial_sessions: 4
    idle_timeout_ms: 120000

  # Low-frequency model
  batch_model:
    min_sessions: 0  # Create on demand
    max_sessions: 2
    idle_timeout_ms: 30000
```

## Performance Benefits

### Scenario: Concurrent Requests

**Before (Sequential)**:
- 10 concurrent requests
- Each takes 50ms (model inference)
- Total time: 500ms (10 × 50ms)
- Throughput: 20 req/sec

**After (4 sessions pool)**:
- 10 concurrent requests
- 4 run in parallel (50ms)
- Next 4 run (50ms)
- Next 2 run (50ms)
- Total time: 150ms (3 batches)
- Throughput: 67 req/sec
- **Speedup: 3.3x**

### Scenario: Bursty Traffic

```
Time:    0ms     100ms    200ms    300ms    400ms
Before:  |■■■■■| |■■■■■| |■■■■■| |■■■■■| |■■■■■|
         (Create + Infer each time)

After:   |■■■■■|■|■|■|■|■|■|■|■|■|■|■|■|■|
Pool:    ▲       ▲ ▲ ▲ ▲ ▲ ▲ ▲ ▲ ▲ ▲ ▲ ▲ ▲
         (Reuse sessions, no creation overhead)
```

## Monitoring & Metrics

```python
# Get pool statistics
stats = client.get_pool_stats()

print(f"Total sessions: {stats['total_sessions']}")
print(f"Idle: {stats['idle_sessions']}")
print(f"Busy: {stats['busy_sessions']}")
print(f"Acquisitions: {stats['total_acquisitions']}")
print(f"Avg wait time: {stats['avg_wait_time_ms']:.2f}ms")

# Output:
# Total sessions: 4
# Idle: 2
# Busy: 2
# Acquisitions: 1523
# Avg wait time: 1.2ms
```

## Thread Safety

```cpp
// Example concurrent usage
void HandleRequests(const std::vector<PredictRequest>& requests) {
    std::vector<std::future<PredictResponse>> futures;

    for (const auto& request : requests) {
        futures.push_back(std::async(std::launch::async, [&]() {
            PredictResponse response;
            runtime_state.Predict(request, response);
            return response;
        }));
    }

    // Wait for all to complete
    for (auto& future : futures) {
        auto response = future.get();
        // Process response
    }
}
```

## Testing

```python
def test_concurrent_inference():
    """Test session pooling with concurrent requests."""
    import concurrent.futures

    client = RuntimeClient()
    model_id = client.register_model("model.onnx")

    def run_inference():
        inputs = {"input": np.random.randn(1, 3, 224, 224).astype(np.float32)}
        return client.predict(model_id, inputs)

    # Run 10 concurrent requests
    with concurrent.futures.ThreadPoolExecutor(max_workers=10) as executor:
        futures = [executor.submit(run_inference) for _ in range(10)]

        start = time.time()
        results = [f.result() for f in futures]
        duration = time.time() - start

    print(f"Completed 10 requests in {duration:.2f}s")
    print(f"Throughput: {10 / duration:.2f} req/sec")

    # Check pool stats
    stats = client.get_pool_stats()
    assert stats['total_sessions'] >= 1
    assert stats['total_sessions'] <= 4  # Max pool size
```

## Implementation Priority

### Week 2 (Core)
1. ✅ SessionPool class with acquire/release
2. ✅ Basic pooling (fixed size)
3. ✅ Integration with RuntimeState

### Week 3 (Advanced)
1. Auto-scaling logic
2. Idle timeout and cleanup
3. Per-model configuration

### Week 4 (Optimization)
1. Advanced metrics
2. Pool warm-up strategies
3. Session affinity optimization

## References

- [Object Pool Pattern](https://en.wikipedia.org/wiki/Object_pool_pattern)
- [Thread Pool Design](https://en.wikipedia.org/wiki/Thread_pool)
- [ONNX Runtime Session Management](https://onnxruntime.ai/docs/performance/tune-performance.html)
