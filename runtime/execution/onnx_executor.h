/**
 * ONNX Runtime Executor
 *
 * Wraps ONNX Runtime for CPU inference.
 */

#ifndef ONNX_EXECUTOR_H
#define ONNX_EXECUTOR_H

#include "../include/coremlwin_errors.h"
#include "../include/coremlwin_provider_api.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

// Forward declarations
namespace Ort {
    class Env;
    class Session;
    class SessionOptions;
}

/**
 * Tensor data for inference
 */
struct TensorData {
    std::string name;
    CmwDataType dtype;
    std::vector<int64_t> shape;
    std::vector<uint8_t> data;
};

/**
 * ONNX Runtime Executor
 *
 * Handles ONNX model loading and inference using ONNX Runtime.
 */
class ONNXExecutor {
public:
    ONNXExecutor();
    ~ONNXExecutor();

    /**
     * Load ONNX model
     * @param model_path Path to ONNX file
     * @param provider_name Provider name ("CPUExecutionProvider", "DmlExecutionProvider", etc.)
     * @return Error code
     */
    CmwErrorCode LoadModel(
        const std::string& model_path,
        const std::string& provider_name = "CPUExecutionProvider"
    );

    /**
     * Run inference
     * @param inputs Input tensors (name → tensor data)
     * @param outputs Output tensors (filled by function)
     * @return Error code
     */
    CmwErrorCode RunInference(
        const std::map<std::string, TensorData>& inputs,
        std::map<std::string, TensorData>& outputs
    );

    /**
     * Get input names
     * @return Vector of input names
     */
    std::vector<std::string> GetInputNames() const;

    /**
     * Get output names
     * @return Vector of output names
     */
    std::vector<std::string> GetOutputNames() const;

    /**
     * Get input shapes
     * @return Map of input name to shape
     */
    std::map<std::string, std::vector<int64_t>> GetInputShapes() const;

    /**
     * Get output shapes
     * @return Map of output name to shape
     */
    std::map<std::string, std::vector<int64_t>> GetOutputShapes() const;

    /**
     * Check if model is loaded
     * @return True if loaded
     */
    bool IsLoaded() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

#endif // ONNX_EXECUTOR_H
