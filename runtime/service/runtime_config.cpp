/**
 * Runtime Configuration Implementation
 */

#include "../include/runtime_config.h"
#include "../include/logger.h"
#include <fstream>
#include <sstream>

namespace coremlwin {

RuntimeConfig RuntimeConfig::GetDefault() {
    RuntimeConfig config;

    // Service defaults
    config.service.pipe_name = "\\\\.\\pipe\\coremlwin_runtime";
    config.service.max_clients = 32;
    config.service.buffer_size = 65536;
    config.service.timeout_ms = 30000;

    // Cache defaults
    config.cache.directory = "./cache";
    config.cache.max_size_gb = 10;
    config.cache.cleanup_interval_hours = 24;
    config.cache.enable_compression = true;

    // Logging defaults (INFO level for production)
    config.logging.level = "INFO";
    config.logging.log_to_file = true;
    config.logging.log_to_console = true;
    config.logging.log_file = "";  // Will be set to cache/coremlwin_runtime.log
    config.logging.max_file_size_mb = 100;
    config.logging.max_backup_count = 5;

    // Converter defaults
    config.converter.python_executable = "python";
    config.converter.timeout_seconds = 300;
    config.converter.default_opset = 14;
    config.converter.enable_benchmarking = true;

    // Benchmark defaults
    config.benchmark.warmup_iterations = 5;
    config.benchmark.benchmark_iterations = 50;
    config.benchmark.enabled_providers = {
        "CPUExecutionProvider",
        "DmlExecutionProvider",
        "CUDAExecutionProvider"
    };
    config.benchmark.timeout_per_provider_ms = 10000;

    // Session pooling (disabled by default, not implemented yet)
    config.session_pooling.enabled = false;
    config.session_pooling.default_pool_config.min_sessions = 1;
    config.session_pooling.default_pool_config.max_sessions = 4;
    config.session_pooling.default_pool_config.initial_sessions = 1;
    config.session_pooling.default_pool_config.idle_timeout_ms = 60000;
    config.session_pooling.default_pool_config.auto_scale = true;

    // Shared memory (disabled by default, not implemented yet)
    config.shared_memory.enabled = false;
    config.shared_memory.threshold_bytes = 1048576;  // 1MB
    config.shared_memory.max_regions = 16;
    config.shared_memory.pool_size_mb = 1024;

    // Performance defaults
    config.performance.enable_profiling = false;
    config.performance.metrics_interval_seconds = 60;
    config.performance.export_prometheus = false;
    config.performance.prometheus_port = 9090;

    // Security defaults
    config.security.require_authentication = false;
    config.security.max_model_size_mb = 2048;
    config.security.allowed_model_formats = {
        "onnx", "pytorch", "tensorflow", "coreml"
    };

    return config;
}

RuntimeConfig RuntimeConfig::GetDevelopment() {
    RuntimeConfig config = GetDefault();

    // Development overrides
    config.logging.level = "DEBUG";  // More verbose logging
    config.performance.enable_profiling = true;  // Enable profiling

    return config;
}

std::string RuntimeConfig::Validate() const {
    std::ostringstream errors;

    // Validate service config
    if (service.max_clients <= 0) {
        errors << "service.max_clients must be positive; ";
    }
    if (service.buffer_size < 1024) {
        errors << "service.buffer_size must be at least 1024 bytes; ";
    }

    // Validate cache config
    if (cache.max_size_gb < 1) {
        errors << "cache.max_size_gb must be at least 1; ";
    }

    // Validate logging config
    if (logging.level != "TRACE" && logging.level != "DEBUG" &&
        logging.level != "INFO" && logging.level != "WARNING" &&
        logging.level != "ERROR" && logging.level != "FATAL") {
        errors << "logging.level must be one of: TRACE, DEBUG, INFO, WARNING, ERROR, FATAL; ";
    }

    // Validate converter config
    if (converter.timeout_seconds <= 0) {
        errors << "converter.timeout_seconds must be positive; ";
    }
    if (converter.default_opset < 7 || converter.default_opset > 18) {
        errors << "converter.default_opset must be between 7 and 18; ";
    }

    // Validate benchmark config
    if (benchmark.warmup_iterations < 0) {
        errors << "benchmark.warmup_iterations must be non-negative; ";
    }
    if (benchmark.benchmark_iterations <= 0) {
        errors << "benchmark.benchmark_iterations must be positive; ";
    }

    // Validate session pooling config
    if (session_pooling.enabled) {
        if (session_pooling.default_pool_config.min_sessions < 0) {
            errors << "session_pooling.min_sessions must be non-negative; ";
        }
        if (session_pooling.default_pool_config.max_sessions < session_pooling.default_pool_config.min_sessions) {
            errors << "session_pooling.max_sessions must be >= min_sessions; ";
        }
    }

    // Validate shared memory config
    if (shared_memory.enabled) {
        if (shared_memory.threshold_bytes == 0) {
            errors << "shared_memory.threshold_bytes must be positive; ";
        }
        if (shared_memory.max_regions <= 0) {
            errors << "shared_memory.max_regions must be positive; ";
        }
    }

    // Validate security config
    if (security.max_model_size_mb <= 0) {
        errors << "security.max_model_size_mb must be positive; ";
    }
    if (security.allowed_model_formats.empty()) {
        errors << "security.allowed_model_formats must not be empty; ";
    }

    return errors.str();
}

bool RuntimeConfig::LoadFromFile(const std::string& config_path) {
    // TODO: Implement YAML parsing
    // For now, return false and log that YAML support is not implemented
    // To implement:
    // 1. Add yaml-cpp library (header-only or linked)
    // 2. Parse YAML file
    // 3. Populate config structure
    // 4. Validate configuration
    //
    // Example with yaml-cpp:
    // #include <yaml-cpp/yaml.h>
    // YAML::Node config_node = YAML::LoadFile(config_path);
    // service.pipe_name = config_node["service"]["pipe_name"].as<std::string>();
    // ...

    LOG_WARNING << "YAML configuration loading not implemented yet";
    LOG_INFO << "Using default configuration";

    *this = GetDefault();
    return true;
}

bool RuntimeConfig::LoadFromString(const std::string& yaml_content) {
    // TODO: Implement YAML parsing from string
    LOG_WARNING << "YAML configuration parsing not implemented yet";
    *this = GetDefault();
    return true;
}

} // namespace coremlwin
