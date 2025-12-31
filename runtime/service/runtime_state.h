/**
 * Runtime State Manager
 *
 * Coordinates all runtime components:
 * - Model registry
 * - ONNX executor
 * - Converter worker
 * - Provider management
 */

#ifndef RUNTIME_STATE_H
#define RUNTIME_STATE_H

#include "../include/coremlwin_errors.h"
#include "model_registry.h"
#include "../execution/onnx_executor.h"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>

/**
 * Request for model registration
 */
struct RegisterModelRequest {
    std::string model_path;
    std::string cache_key;  // Optional override
};

/**
 * Response from model registration
 */
struct RegisterModelResponse {
    CmwErrorCode error_code;
    std::string model_id;
    ModelMetadata metadata;
    std::string error_message;
};

/**
 * Request for inference
 */
struct PredictRequest {
    std::string model_id;
    std::map<std::string, TensorData> inputs;
    std::string compute_units;  // "CPU_ONLY", "CPU_AND_GPU", etc.
    int timeout_ms;
};

/**
 * Response from inference
 */
struct PredictResponse {
    CmwErrorCode error_code;
    std::map<std::string, TensorData> outputs;
    std::string provider_used;
    int64_t inference_time_us;
    std::string error_message;
};

/**
 * Runtime State Manager
 *
 * Central coordinator for all runtime operations.
 */
class RuntimeState {
public:
    RuntimeState();
    ~RuntimeState();

    /**
     * Initialize runtime
     * @param cache_dir Cache directory for converted models
     * @return Error code
     */
    CmwErrorCode Initialize(const std::string& cache_dir);

    /**
     * Shutdown runtime
     */
    void Shutdown();

    /**
     * Register a model
     * @param request Registration request
     * @param response Output response
     * @return Error code
     */
    CmwErrorCode RegisterModel(
        const RegisterModelRequest& request,
        RegisterModelResponse& response
    );

    /**
     * Unregister a model
     * @param model_id Model ID
     * @return Error code
     */
    CmwErrorCode UnregisterModel(const std::string& model_id);

    /**
     * Run inference
     * @param request Inference request
     * @param response Output response
     * @return Error code
     */
    CmwErrorCode Predict(
        const PredictRequest& request,
        PredictResponse& response
    );

    /**
     * Get model metadata
     * @param model_id Model ID
     * @param metadata Output metadata
     * @return Error code
     */
    CmwErrorCode GetModelMetadata(
        const std::string& model_id,
        ModelMetadata& metadata
    );

    /**
     * List all models
     * @return Vector of model metadata
     */
    std::vector<ModelMetadata> ListModels();

    /**
     * Check if initialized
     * @return True if initialized
     */
    bool IsInitialized() const { return initialized_; }

private:
    /**
     * Run converter worker to convert model to ONNX
     * @param model_path Input model path
     * @param output_path Output ONNX path
     * @param metadata Output metadata
     * @return Error code
     */
    CmwErrorCode RunConverterWorker(
        const std::string& model_path,
        const std::string& output_path,
        ModelMetadata& metadata
    );

    /**
     * Compute content hash for model
     * @param model_path Model path
     * @return Hash string
     */
    std::string ComputeModelHash(const std::string& model_path);

    /**
     * Load or create executor for model
     * @param model_id Model ID
     * @param executor Output executor
     * @return Error code
     */
    CmwErrorCode GetOrCreateExecutor(
        const std::string& model_id,
        std::shared_ptr<ONNXExecutor>& executor
    );

    bool initialized_;
    std::string cache_dir_;

    ModelRegistry registry_;

    // Executor cache (model_id → executor)
    std::mutex executors_mutex_;
    std::map<std::string, std::shared_ptr<ONNXExecutor>> executors_;
};

#endif // RUNTIME_STATE_H
