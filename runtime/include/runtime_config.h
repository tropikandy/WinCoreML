/**
 * Runtime Configuration
 *
 * Configuration structure for Universal ML Runtime.
 * Can be loaded from YAML file or created programmatically.
 */

#ifndef RUNTIME_CONFIG_H
#define RUNTIME_CONFIG_H

#include <string>
#include <vector>
#include <map>

namespace coremlwin {

/**
 * Service configuration
 */
struct ServiceConfig {
    std::string pipe_name = "\\\\.\\pipe\\coremlwin_runtime";
    int max_clients = 32;
    int buffer_size = 65536;  // 64KB
    int timeout_ms = 30000;
};

/**
 * Cache configuration
 */
struct CacheConfig {
    std::string directory = "./cache";
    int max_size_gb = 10;
    int cleanup_interval_hours = 24;
    bool enable_compression = true;
};

/**
 * Logging configuration
 */
struct LoggingConfig {
    std::string level = "INFO";
    bool log_to_file = true;
    bool log_to_console = true;
    std::string log_file = "";  // Auto-generated if empty
    int max_file_size_mb = 100;
    int max_backup_count = 5;
};

/**
 * Converter configuration
 */
struct ConverterConfig {
    std::string python_executable = "python";
    int timeout_seconds = 300;
    int default_opset = 14;
    bool enable_benchmarking = true;
};

/**
 * Benchmark configuration
 */
struct BenchmarkConfig {
    int warmup_iterations = 5;
    int benchmark_iterations = 50;
    std::vector<std::string> enabled_providers = {
        "CPUExecutionProvider",
        "DmlExecutionProvider",
        "CUDAExecutionProvider"
    };
    int timeout_per_provider_ms = 10000;
};

/**
 * Session pool configuration (per model)
 */
struct SessionPoolConfig {
    int min_sessions = 1;
    int max_sessions = 4;
    int initial_sessions = 1;
    int idle_timeout_ms = 60000;
    bool auto_scale = true;
};

/**
 * Session pooling configuration
 */
struct SessionPoolingConfig {
    bool enabled = false;
    SessionPoolConfig default_pool_config;
    std::map<std::string, SessionPoolConfig> model_configs;
};

/**
 * Shared memory configuration
 */
struct SharedMemoryConfig {
    bool enabled = false;
    size_t threshold_bytes = 1048576;  // 1MB
    int max_regions = 16;
    int pool_size_mb = 1024;
};

/**
 * Performance configuration
 */
struct PerformanceConfig {
    bool enable_profiling = false;
    int metrics_interval_seconds = 60;
    bool export_prometheus = false;
    int prometheus_port = 9090;
};

/**
 * Security configuration
 */
struct SecurityConfig {
    bool require_authentication = false;
    int max_model_size_mb = 2048;
    std::vector<std::string> allowed_model_formats = {
        "onnx", "pytorch", "tensorflow", "coreml"
    };
};

/**
 * Complete runtime configuration
 */
struct RuntimeConfig {
    ServiceConfig service;
    CacheConfig cache;
    LoggingConfig logging;
    ConverterConfig converter;
    BenchmarkConfig benchmark;
    SessionPoolingConfig session_pooling;
    SharedMemoryConfig shared_memory;
    PerformanceConfig performance;
    SecurityConfig security;

    /**
     * Load configuration from YAML file
     * @param config_path Path to YAML config file
     * @return true if loaded successfully
     */
    bool LoadFromFile(const std::string& config_path);

    /**
     * Load configuration from string (YAML content)
     */
    bool LoadFromString(const std::string& yaml_content);

    /**
     * Get default configuration
     */
    static RuntimeConfig GetDefault();

    /**
     * Get development configuration (verbose logging, etc.)
     */
    static RuntimeConfig GetDevelopment();

    /**
     * Validate configuration
     * @return Error message if invalid, empty string if valid
     */
    std::string Validate() const;
};

} // namespace coremlwin

#endif // RUNTIME_CONFIG_H
