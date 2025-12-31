/**
 * Model Registry
 *
 * Manages registered models, their metadata, and benchmark results.
 */

#ifndef MODEL_REGISTRY_H
#define MODEL_REGISTRY_H

#include "../include/coremlwin_errors.h"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>

/**
 * Benchmark result for a specific provider
 */
struct ProviderBenchmark {
    std::string provider_name;
    float mean_latency_ms;
    float std_latency_ms;
    float speedup_vs_cpu;
    bool success;
    std::string error_message;
};

/**
 * Model metadata
 */
struct ModelMetadata {
    std::string model_id;           // Unique ID (content hash)
    std::string original_path;      // Original model path
    std::string onnx_path;          // Converted ONNX path
    std::string model_format;       // "pytorch", "tensorflow", "coreml", "onnx"

    std::vector<std::string> input_names;
    std::vector<std::string> output_names;

    std::map<std::string, std::vector<int64_t>> input_shapes;
    std::map<std::string, std::vector<int64_t>> output_shapes;

    std::vector<ProviderBenchmark> benchmarks;
    std::string fastest_provider;
    float best_latency_ms;

    std::map<std::string, std::string> extra_metadata;
};

/**
 * Model Registry
 *
 * Thread-safe registry for managing models.
 */
class ModelRegistry {
public:
    ModelRegistry();
    ~ModelRegistry();

    /**
     * Register a model
     * @param metadata Model metadata
     * @return Error code
     */
    CmwErrorCode RegisterModel(const ModelMetadata& metadata);

    /**
     * Unregister a model
     * @param model_id Model ID
     * @return Error code
     */
    CmwErrorCode UnregisterModel(const std::string& model_id);

    /**
     * Get model metadata
     * @param model_id Model ID
     * @param out_metadata Output metadata
     * @return Error code
     */
    CmwErrorCode GetModel(
        const std::string& model_id,
        ModelMetadata& out_metadata
    ) const;

    /**
     * Check if model exists
     * @param model_id Model ID
     * @return True if model exists
     */
    bool HasModel(const std::string& model_id) const;

    /**
     * List all registered models
     * @return Vector of model IDs
     */
    std::vector<std::string> ListModels() const;

    /**
     * Get all model metadata
     * @return Vector of all metadata
     */
    std::vector<ModelMetadata> GetAllModels() const;

    /**
     * Clear all models
     */
    void Clear();

private:
    mutable std::mutex mutex_;
    std::map<std::string, ModelMetadata> models_;
};

#endif // MODEL_REGISTRY_H
