# Red Team Security Audit - WinCoreML Runtime

**Date**: 2024-01-15
**Scope**: Complete codebase security analysis
**Severity Levels**: CRITICAL | HIGH | MEDIUM | LOW

---

## Executive Summary

This document details security vulnerabilities, design flaws, and potential attack vectors identified in the WinCoreML Universal ML Runtime.

**Risk Summary:**
- 🔴 CRITICAL: 3 issues
- 🟠 HIGH: 5 issues
- 🟡 MEDIUM: 4 issues
- 🟢 LOW: 2 issues

---

## 🔴 CRITICAL Vulnerabilities

### C1: Buffer Overflow in Tensor Data Handling
**File**: `runtime/execution/onnx_executor.cpp:197-200`
**Severity**: CRITICAL

```cpp
auto tensor_value = Ort::Value::CreateTensor<float>(
    memory_info,
    reinterpret_cast<float*>(const_cast<uint8_t*>(tensor.data.data())),
    tensor.data.size() / sizeof(float),  // ⚠️ NO VALIDATION
    tensor.shape.data(),
    tensor.shape.size()
);
```

**Issue**: No validation that `tensor.data.size()` matches the expected size from `tensor.shape`.

**Attack Vector**:
```python
# Malicious client sends mismatched shape and data
inputs = {
    'input': np.zeros((1, 3, 224, 224)),  # Says 150K elements
}
# But actually sends only 1KB of data -> Buffer underflow
# Or sends 10MB of data -> Buffer overflow
```

**Impact**: Memory corruption, potential RCE

**Fix**: Validate tensor size matches shape:
```cpp
size_t expected_size = 1;
for (auto dim : tensor.shape) {
    expected_size *= dim;
}
if (tensor.data.size() != expected_size * sizeof(float)) {
    LOG_ERROR << "Tensor size mismatch";
    return CMW_ERROR_INVALID_ARGUMENT;
}
```

---

### C2: Path Traversal Vulnerability
**File**: `runtime/service/runtime_state.cpp` (model registration)
**Severity**: CRITICAL

```cpp
std::string model_id = client.register_model("../../../etc/passwd");
std::string model_id = client.register_model("C:\\Windows\\System32\\config\\SAM");
```

**Issue**: No validation of model paths. Attacker can read arbitrary files.

**Attack Vector**:
1. Register path to sensitive file
2. Trigger model loading
3. ONNX Runtime reads file
4. Error messages leak file contents

**Impact**: Arbitrary file read, information disclosure

**Fix**: Validate and canonicalize paths:
```cpp
// Resolve to canonical path
std::filesystem::path canonical_path = std::filesystem::canonical(model_path);

// Check it's within allowed directory
if (!canonical_path.string().starts_with(allowed_model_dir)) {
    return CMW_ERROR_INVALID_ARGUMENT;
}

// Check it's actually a file
if (!std::filesystem::is_regular_file(canonical_path)) {
    return CMW_ERROR_INVALID_ARGUMENT;
}

// Validate .onnx extension
if (canonical_path.extension() != ".onnx") {
    return CMW_ERROR_INVALID_ARGUMENT;
}
```

---

### C3: Unbounded Memory Allocation (DoS)
**File**: `runtime/service/named_pipe_server_win32.cpp`
**Severity**: CRITICAL

**Issue**: No limit on protobuf message size. Attacker can exhaust memory.

**Attack Vector**:
```python
# Send 1GB protobuf message
request = create_massive_request(size=1024*1024*1024)
send_to_pipe(request)  # Runtime allocates 1GB, crashes
```

**Impact**: Denial of Service, memory exhaustion

**Fix**: Add size limits:
```cpp
const size_t MAX_MESSAGE_SIZE = 100 * 1024 * 1024;  // 100 MB

if (message_size > MAX_MESSAGE_SIZE) {
    LOG_ERROR << "Message too large: " << message_size;
    return CMW_ERROR_INVALID_ARGUMENT;
}
```

---

## 🟠 HIGH Severity Issues

### H1: Integer Overflow in Shape Calculation
**File**: `runtime/execution/onnx_executor.cpp`
**Severity**: HIGH

```cpp
size_t num_elements = 1;
for (auto dim : output_tensor.shape) {
    num_elements *= dim;  // ⚠️ Can overflow
}
output_tensor.data.resize(num_elements * sizeof(float));  // Wrong size!
```

**Attack Vector**:
```python
# Shape that overflows size_t
shape = [2**20, 2**20, 2**20]  # = 2^60, overflows to small value
# Allocates tiny buffer, writes huge data -> overflow
```

**Fix**: Check for overflow:
```cpp
size_t num_elements = 1;
for (auto dim : output_tensor.shape) {
    if (num_elements > SIZE_MAX / dim) {
        LOG_ERROR << "Shape too large, would overflow";
        return CMW_ERROR_INVALID_ARGUMENT;
    }
    num_elements *= dim;
}

const size_t MAX_TENSOR_SIZE = 1024 * 1024 * 1024;  // 1GB
if (num_elements > MAX_TENSOR_SIZE / sizeof(float)) {
    LOG_ERROR << "Tensor too large: " << num_elements;
    return CMW_ERROR_INVALID_ARGUMENT;
}
```

---

### H2: Race Condition in Model Registry
**File**: `runtime/service/model_registry.cpp`
**Severity**: HIGH

**Issue**: Model registry likely not thread-safe for concurrent access.

**Attack Vector**:
```python
# Thread 1: Register model
# Thread 2: Unregister same model simultaneously
# Thread 3: Run inference on that model
# -> Use-after-free
```

**Impact**: Memory corruption, crashes

**Fix**: Add mutex protection:
```cpp
class ModelRegistry {
private:
    std::mutex mutex_;
    std::map<std::string, ModelEntry> models_;

public:
    std::string RegisterModel(...) {
        std::lock_guard<std::mutex> lock(mutex_);
        // ... registration logic ...
    }
};
```

---

### H3: No Input Sanitization for Log Messages
**File**: Multiple files using LOG_* macros
**Severity**: HIGH

```cpp
LOG_INFO << "Loading model: " << user_provided_path;
// ⚠️ Path could contain ANSI escape codes for terminal injection
```

**Attack Vector**:
```python
model_path = "\x1b[2J\x1b[H HACKED \x1b[0m malicious.onnx"
# Clears terminal, displays fake message
```

**Impact**: Log injection, terminal manipulation

**Fix**: Sanitize user input in logs:
```cpp
std::string SanitizeForLog(const std::string& input) {
    std::string sanitized;
    for (char c : input) {
        if (c >= 32 && c <= 126) {  // Printable ASCII only
            sanitized += c;
        } else {
            sanitized += "?";
        }
    }
    return sanitized;
}
```

---

### H4: Unvalidated Protobuf Deserialization
**File**: `runtime/service/main.cpp`
**Severity**: HIGH

```cpp
if (!request.ParseFromArray(buffer, bytes_read)) {
    LOG_ERROR << "Failed to parse protobuf";
    // ⚠️ But what if ParseFromArray throws?
}
```

**Issue**: Malformed protobuf can cause exceptions or infinite loops.

**Fix**: Add timeout and size limits:
```cpp
google::protobuf::io::CodedInputStream coded_input(buffer, bytes_read);
coded_input.SetTotalBytesLimit(MAX_MESSAGE_SIZE);
coded_input.SetRecursionLimit(100);

if (!request.ParseFromCodedStream(&coded_input)) {
    LOG_ERROR << "Protobuf parsing failed";
    return;
}
```

---

### H5: Missing Resource Cleanup on Error Paths
**File**: `runtime/execution/onnx_executor.cpp`
**Severity**: HIGH

```cpp
try {
    impl_->session = std::make_unique<Ort::Session>(...);
    // ⚠️ If GetInputCount() throws, session not cleaned up properly
    size_t num_inputs = impl_->session->GetInputCount();
} catch (...) {
    impl_->loaded = false;
    // ⚠️ Session still exists but marked as not loaded
}
```

**Impact**: Resource leaks, dangling pointers

**Fix**: Use RAII and reset on errors:
```cpp
} catch (...) {
    impl_->session.reset();
    impl_->env.reset();
    impl_->session_options.reset();
    impl_->loaded = false;
    return CMW_ERROR_MODEL_LOAD_FAILED;
}
```

---

## 🟡 MEDIUM Severity Issues

### M1: Weak Model ID Generation
**File**: `runtime/service/model_registry.cpp` (assumed)
**Severity**: MEDIUM

**Issue**: Model IDs might be predictable (sequential, timestamp-based).

**Attack Vector**:
```python
# Guess other users' model IDs
for i in range(1000):
    try:
        inference(model_id=f"model_{i}")  # Access other models
    except:
        pass
```

**Fix**: Use cryptographically secure random IDs:
```cpp
#include <random>
#include <sstream>
#include <iomanip>

std::string GenerateModelID() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;

    uint64_t id1 = dis(gen);
    uint64_t id2 = dis(gen);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << id1 << id2;
    return oss.str();
}
```

---

### M2: No Rate Limiting
**File**: Named pipe server
**Severity**: MEDIUM

**Issue**: No protection against rapid-fire requests.

**Attack Vector**:
```python
while True:
    client.run_inference(model_id, huge_input)  # CPU/GPU exhaustion
```

**Fix**: Add per-client rate limiting:
```cpp
class RateLimiter {
    std::map<ClientID, std::deque<std::chrono::steady_clock::time_point>> requests_;

    bool AllowRequest(ClientID client) {
        auto now = std::chrono::steady_clock::now();
        auto& times = requests_[client];

        // Remove old requests
        while (!times.empty() && now - times.front() > std::chrono::seconds(60)) {
            times.pop_front();
        }

        if (times.size() >= 100) {  // 100 req/min limit
            return false;
        }

        times.push_back(now);
        return true;
    }
};
```

---

### M3: Insufficient Error Information Disclosure
**File**: Multiple
**Severity**: MEDIUM (privacy concern)

**Issue**: Error messages might leak sensitive information.

```cpp
LOG_ERROR << "Failed to load model from: " << full_path;
// Leaks internal directory structure
```

**Fix**: Sanitize error messages for clients:
```cpp
// Internal logging (detailed)
LOG_ERROR << "Failed to load: " << full_path << " - " << error;

// Client response (sanitized)
response.set_error_message("Model loading failed");
response.set_error_code(CMW_ERROR_MODEL_LOAD_FAILED);
// Don't send full_path to client
```

---

### M4: No Session Timeout
**File**: ONNX executor
**Severity**: MEDIUM

**Issue**: Long-running inference can hang forever.

**Fix**: Add inference timeout:
```cpp
Ort::RunOptions run_options;
run_options.SetRunTimeout(30000);  // 30 second timeout

auto outputs = session->Run(run_options, ...);
```

---

## 🟢 LOW Severity Issues

### L1: Verbose Logging in Production
**File**: Multiple
**Severity**: LOW

**Issue**: DEBUG logs in production can leak information and impact performance.

**Fix**: Ensure production builds use INFO level minimum.

---

### L2: No Metrics/Monitoring
**File**: N/A
**Severity**: LOW

**Issue**: No way to detect attacks or anomalies.

**Fix**: Add metrics collection:
- Request counts
- Error rates
- Inference latencies
- Memory usage

---

## Attack Scenarios

### Scenario 1: Remote Code Execution
```
1. Attacker sends malicious ONNX model with buffer overflow
2. Model loaded via path traversal to bypass validation
3. Inference triggered with crafted input
4. Buffer overflow in tensor handling
5. RCE achieved
```

### Scenario 2: Denial of Service
```
1. Attacker sends 1GB protobuf message -> Memory exhaustion
2. Or sends requests with overflow shapes -> Crash
3. Or sends rapid-fire requests -> CPU exhaustion
4. Service becomes unavailable
```

### Scenario 3: Information Disclosure
```
1. Attacker registers "../../../sensitive_file"
2. ONNX Runtime tries to load it
3. Error message contains file content snippets
4. Sensitive data leaked
```

---

## Recommendations Priority

### Immediate (Week 5.5):
1. ✅ Fix C1: Add tensor size validation
2. ✅ Fix C2: Add path validation and canonicalization
3. ✅ Fix C3: Add message size limits
4. ✅ Fix H1: Add overflow checks
5. ✅ Fix H2: Add mutex protection

### Short-term (Week 6):
6. Fix H3: Sanitize log inputs
7. Fix H4: Harden protobuf parsing
8. Fix H5: Improve error cleanup
9. Fix M1: Use secure random IDs
10. Fix M2: Add rate limiting

### Long-term:
11. Add comprehensive fuzzing
12. Add security test suite
13. Add metrics and monitoring
14. Security audit by external team

---

## Testing Recommendations

### Adversarial Tests
- Malformed protobuf messages
- Oversized inputs (1GB, 10GB)
- Invalid paths (traversal, special chars)
- Concurrent access stress tests
- Integer overflow edge cases
- Out-of-memory scenarios

### Fuzzing
- AFL/libFuzzer on protobuf parsing
- ONNX model fuzzing
- Tensor shape/data fuzzing

---

## Conclusion

The current implementation has several critical security vulnerabilities that must be addressed before production deployment. The primary concerns are:

1. **Buffer overflow** from unvalidated tensor sizes
2. **Path traversal** allowing arbitrary file access
3. **DoS** from unbounded allocations

Recommended to implement all CRITICAL and HIGH fixes immediately.
