/**
 * Universal ML Runtime - Unity-Friendly C API
 *
 * Simple C interface designed for use with Unity and game engines.
 * Features:
 * - C-style exports (no C++ mangling)
 * - Opaque handles (type-safe pointers)
 * - Manual memory management
 * - Simple data types (POD)
 * - Clear error codes
 */

#ifndef UMLRUNTIME_H
#define UMLRUNTIME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Platform-specific exports
// ============================================================================

#ifdef _WIN32
    #ifdef UMLRUNTIME_EXPORTS
        #define UMLRT_API __declspec(dllexport)
    #else
        #define UMLRT_API __declspec(dllimport)
    #endif
    #define UMLRT_CALL __cdecl
#else
    #define UMLRT_API
    #define UMLRT_CALL
#endif

// ============================================================================
// Version Information
// ============================================================================

#define UMLRT_VERSION_MAJOR 0
#define UMLRT_VERSION_MINOR 1
#define UMLRT_VERSION_PATCH 0

UMLRT_API const char* UMLRT_CALL UMLRTGetVersion(void);

// ============================================================================
// Error Codes (matches unified taxonomy)
// ============================================================================

typedef enum UMLRTErrorCode {
    UMLRT_SUCCESS = 0,

    // Client errors (1000-1999)
    UMLRT_ERROR_INVALID_ARGUMENT = 1000,
    UMLRT_ERROR_INVALID_MODEL_ID = 1001,
    UMLRT_ERROR_INVALID_TENSOR = 1002,
    UMLRT_ERROR_MISSING_INPUT = 1005,

    // Model errors (2000-2999)
    UMLRT_ERROR_MODEL_NOT_FOUND = 2000,
    UMLRT_ERROR_MODEL_LOAD_FAILED = 2001,
    UMLRT_ERROR_CONVERSION_FAILED = 2006,

    // Provider errors (3000-3999)
    UMLRT_ERROR_PROVIDER_UNAVAILABLE = 3000,
    UMLRT_ERROR_EXECUTION_FAILED = 3002,
    UMLRT_ERROR_OUT_OF_DEVICE_MEMORY = 3006,

    // Runtime errors (4000-4999)
    UMLRT_ERROR_INTERNAL = 4000,
    UMLRT_ERROR_NOT_IMPLEMENTED = 4001,
    UMLRT_ERROR_SERVICE_UNAVAILABLE = 4003,

    // Transient errors (5000-5999)
    UMLRT_ERROR_TIMEOUT = 5000,
    UMLRT_ERROR_CONNECTION_LOST = 5003,

} UMLRTErrorCode;

UMLRT_API const char* UMLRT_CALL UMLRTGetErrorString(UMLRTErrorCode code);

// ============================================================================
// Opaque Handles
// ============================================================================

typedef struct UMLRTRuntime* UMLRTRuntimeHandle;
typedef struct UMLRTModel* UMLRTModelHandle;
typedef struct UMLRTTensor* UMLRTTensorHandle;

// ============================================================================
// Data Types
// ============================================================================

typedef enum UMLRTDataType {
    UMLRT_DTYPE_FLOAT32 = 0,
    UMLRT_DTYPE_FLOAT16 = 1,
    UMLRT_DTYPE_INT32 = 2,
    UMLRT_DTYPE_INT64 = 3,
    UMLRT_DTYPE_INT8 = 4,
    UMLRT_DTYPE_UINT8 = 5,
    UMLRT_DTYPE_BOOL = 6,
} UMLRTDataType;

typedef enum UMLRTComputeUnits {
    UMLRT_COMPUTE_CPU_ONLY = 0,
    UMLRT_COMPUTE_CPU_AND_GPU = 1,
    UMLRT_COMPUTE_ALL = 2,
    UMLRT_COMPUTE_CPU_AND_NPU = 3,
} UMLRTComputeUnits;

// ============================================================================
// Configuration Structures (POD - safe for marshalling)
// ============================================================================

typedef struct UMLRTRuntimeConfig {
    const char* pipe_name;        // NULL = use default
    int32_t timeout_ms;           // Connection timeout
    int32_t enable_logging;       // 0 = off, 1 = on
    const char* log_file_path;    // NULL = no file logging
} UMLRTRuntimeConfig;

typedef struct UMLRTInferenceConfig {
    UMLRTComputeUnits compute_units;
    int32_t enable_profiling;     // 0 = off, 1 = on
    int32_t timeout_ms;
} UMLRTInferenceConfig;

typedef struct UMLRTTensorInfo {
    const char* name;
    UMLRTDataType dtype;
    int32_t rank;                 // Number of dimensions
    const int64_t* shape;         // Array of dimension sizes [rank]
} UMLRTTensorInfo;

typedef struct UMLRTBenchmarkResult {
    const char* provider_name;
    float mean_latency_ms;
    float speedup_vs_cpu;
} UMLRTBenchmarkResult;

// ============================================================================
// Runtime Lifecycle
// ============================================================================

/**
 * Create runtime instance and connect to service
 * @param config Configuration (can be NULL for defaults)
 * @param out_handle Output runtime handle
 * @return Error code
 */
UMLRT_API UMLRTErrorCode UMLRT_CALL UMLRTCreateRuntime(
    const UMLRTRuntimeConfig* config,
    UMLRTRuntimeHandle* out_handle
);

/**
 * Destroy runtime instance and disconnect
 * @param handle Runtime handle
 * @return Error code
 */
UMLRT_API UMLRTErrorCode UMLRT_CALL UMLRTDestroyRuntime(
    UMLRTRuntimeHandle handle
);

/**
 * Check if runtime service is available and healthy
 * @param handle Runtime handle
 * @param out_is_ready Output: 1 if ready, 0 otherwise
 * @return Error code
 */
UMLRT_API UMLRTErrorCode UMLRT_CALL UMLRTCheckHealth(
    UMLRTRuntimeHandle handle,
    int32_t* out_is_ready
);

// ============================================================================
// Model Management
// ============================================================================

/**
 * Register model from file path
 * Supports: .pt, .pth (PyTorch), SavedModel (TF), .mlmodel/.mlpackage (CoreML), .onnx
 *
 * @param handle Runtime handle
 * @param model_path Path to model file or directory
 * @param out_model Output model handle
 * @return Error code
 *
 * Note: This operation may take several seconds as it converts the model to ONNX
 *       and runs benchmarks across available providers.
 */
UMLRT_API UMLRTErrorCode UMLRT_CALL UMLRTRegisterModel(
    UMLRTRuntimeHandle handle,
    const char* model_path,
    UMLRTModelHandle* out_model
);

/**
 * Register model from memory buffer
 * @param handle Runtime handle
 * @param model_data Pointer to model data
 * @param data_size Size of model data in bytes
 * @param model_format Format hint: "onnx", "pytorch", "tensorflow", "coreml"
 * @param out_model Output model handle
 * @return Error code
 */
UMLRT_API UMLRTErrorCode UMLRT_CALL UMLRTRegisterModelFromMemory(
    UMLRTRuntimeHandle handle,
    const void* model_data,
    size_t data_size,
    const char* model_format,
    UMLRTModelHandle* out_model
);

/**
 * Unregister model and free resources
 * @param handle Runtime handle
 * @param model Model handle
 * @return Error code
 */
UMLRT_API UMLRTErrorCode UMLRT_CALL UMLRTUnregisterModel(
    UMLRTRuntimeHandle handle,
    UMLRTModelHandle model
);

/**
 * Get model input count
 * @param model Model handle
 * @param out_count Output input count
 * @return Error code
 */
UMLRT_API UMLRTErrorCode UMLRT_CALL UMLRTGetInputCount(
    UMLRTModelHandle model,
    int32_t* out_count
);

/**
 * Get model output count
 * @param model Model handle
 * @param out_count Output count
 * @return Error code
 */
UMLRT_API UMLRTErrorCode UMLRT_CALL UMLRTGetOutputCount(
    UMLRTModelHandle model,
    int32_t* out_count
);

/**
 * Get input tensor info
 * @param model Model handle
 * @param index Input index
 * @param out_info Output tensor info (caller-allocated)
 * @return Error code
 */
UMLRT_API UMLRTErrorCode UMLRT_CALL UMLRTGetInputInfo(
    UMLRTModelHandle model,
    int32_t index,
    UMLRTTensorInfo* out_info
);

/**
 * Get output tensor info
 * @param model Model handle
 * @param index Output index
 * @param out_info Output tensor info (caller-allocated)
 * @return Error code
 */
UMLRT_API UMLRTErrorCode UMLRT_CALL UMLRTGetOutputInfo(
    UMLRTModelHandle model,
    int32_t index,
    UMLRTTensorInfo* out_info
);

/**
 * Get benchmark results for model
 * @param model Model handle
 * @param out_results Array of benchmark results (caller-allocated)
 * @param max_results Size of out_results array
 * @param out_count Actual number of results returned
 * @return Error code
 */
UMLRT_API UMLRTErrorCode UMLRT_CALL UMLRTGetBenchmarkResults(
    UMLRTModelHandle model,
    UMLRTBenchmarkResult* out_results,
    int32_t max_results,
    int32_t* out_count
);

// ============================================================================
// Tensor Operations
// ============================================================================

/**
 * Create tensor from data
 * @param name Tensor name
 * @param dtype Data type
 * @param rank Number of dimensions
 * @param shape Dimension sizes [rank]
 * @param data Pointer to tensor data
 * @param data_size Size of data in bytes
 * @param out_tensor Output tensor handle
 * @return Error code
 *
 * Note: Data is copied, caller retains ownership of input data
 */
UMLRT_API UMLRTErrorCode UMLRT_CALL UMLRTCreateTensor(
    const char* name,
    UMLRTDataType dtype,
    int32_t rank,
    const int64_t* shape,
    const void* data,
    size_t data_size,
    UMLRTTensorHandle* out_tensor
);

/**
 * Destroy tensor and free resources
 * @param tensor Tensor handle
 * @return Error code
 */
UMLRT_API UMLRTErrorCode UMLRT_CALL UMLRTDestroyTensor(
    UMLRTTensorHandle tensor
);

/**
 * Get tensor data pointer (read-only)
 * @param tensor Tensor handle
 * @param out_data Output data pointer
 * @param out_size Output data size in bytes
 * @return Error code
 *
 * Note: Pointer valid until tensor is destroyed
 */
UMLRT_API UMLRTErrorCode UMLRT_CALL UMLRTGetTensorData(
    UMLRTTensorHandle tensor,
    const void** out_data,
    size_t* out_size
);

/**
 * Copy tensor data to user buffer
 * @param tensor Tensor handle
 * @param buffer User buffer
 * @param buffer_size Buffer size in bytes
 * @return Error code
 */
UMLRT_API UMLRTErrorCode UMLRT_CALL UMLRTCopyTensorData(
    UMLRTTensorHandle tensor,
    void* buffer,
    size_t buffer_size
);

// ============================================================================
// Inference
// ============================================================================

/**
 * Run inference
 * @param handle Runtime handle
 * @param model Model handle
 * @param inputs Array of input tensors
 * @param input_count Number of inputs
 * @param outputs Array to receive output tensors (caller-allocated handles)
 * @param output_count Number of outputs (must match model)
 * @param config Inference config (can be NULL for defaults)
 * @return Error code
 *
 * Note: Caller must destroy output tensors with UMLRTDestroyTensor
 */
UMLRT_API UMLRTErrorCode UMLRT_CALL UMLRTRunInference(
    UMLRTRuntimeHandle handle,
    UMLRTModelHandle model,
    const UMLRTTensorHandle* inputs,
    int32_t input_count,
    UMLRTTensorHandle* outputs,
    int32_t output_count,
    const UMLRTInferenceConfig* config
);

/**
 * Run inference with named inputs/outputs (for flexibility)
 * @param handle Runtime handle
 * @param model Model handle
 * @param input_names Array of input names
 * @param inputs Array of input tensors
 * @param input_count Number of inputs
 * @param output_names Array of output names
 * @param outputs Array to receive output tensors
 * @param output_count Number of outputs
 * @param config Inference config (can be NULL for defaults)
 * @return Error code
 */
UMLRT_API UMLRTErrorCode UMLRT_CALL UMLRTRunInferenceNamed(
    UMLRTRuntimeHandle handle,
    UMLRTModelHandle model,
    const char** input_names,
    const UMLRTTensorHandle* inputs,
    int32_t input_count,
    const char** output_names,
    UMLRTTensorHandle* outputs,
    int32_t output_count,
    const UMLRTInferenceConfig* config
);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Get size in bytes for a data type
 * @param dtype Data type
 * @return Size in bytes, or 0 if unknown
 */
UMLRT_API size_t UMLRT_CALL UMLRTGetDataTypeSize(UMLRTDataType dtype);

/**
 * Get last error message (thread-local)
 * @return Error message string (valid until next call)
 */
UMLRT_API const char* UMLRT_CALL UMLRTGetLastErrorMessage(void);

#ifdef __cplusplus
}
#endif

#endif // UMLRUNTIME_H
