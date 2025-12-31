/**
 * CoreML-on-Windows Provider Plugin API
 *
 * This header defines the interface for acceleration provider plugins.
 * Providers implement hardware-accelerated inference backends (DirectML, OpenVINO, etc.)
 */

#ifndef COREMLWIN_PROVIDER_API_H
#define COREMLWIN_PROVIDER_API_H

#include "coremlwin_errors.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// API version for compatibility checking
#define CMW_PROVIDER_API_VERSION 1

// Forward declarations
typedef struct CmwProvider CmwProvider;
typedef struct CmwSession CmwSession;
typedef struct CmwTensor CmwTensor;

/**
 * Tensor data types
 */
typedef enum {
    CMW_DTYPE_FLOAT32 = 0,
    CMW_DTYPE_FLOAT16 = 1,
    CMW_DTYPE_INT32 = 2,
    CMW_DTYPE_INT64 = 3,
    CMW_DTYPE_INT8 = 4,
    CMW_DTYPE_UINT8 = 5,
    CMW_DTYPE_BOOL = 6,
} CmwDataType;

/**
 * Compute units (match CoreML enum values)
 */
typedef enum {
    CMW_COMPUTE_UNIT_CPU_ONLY = 0,
    CMW_COMPUTE_UNIT_CPU_AND_GPU = 1,
    CMW_COMPUTE_UNIT_ALL = 2,
    CMW_COMPUTE_UNIT_CPU_AND_NPU = 3,
} CmwComputeUnits;

/**
 * Tensor descriptor
 */
typedef struct {
    CmwDataType dtype;
    int32_t rank;            // Number of dimensions
    int64_t* shape;          // Dimension sizes [rank]
    void* data;              // Tensor data
    size_t data_size;        // Size in bytes
} CmwTensor;

/**
 * Provider capabilities (ENHANCED in v2)
 */
typedef struct {
    // Basic info
    const char* provider_name;         // e.g., "DirectML", "OpenVINO", "CPU"
    const char* provider_version;      // Provider version string
    int32_t api_version;               // CMW_PROVIDER_API_VERSION

    // Compute capabilities
    CmwComputeUnits compute_unit;      // CPU, GPU, NPU
    const char* device_name;           // Hardware device name
    int32_t device_id;                 // For multi-device systems

    // Supported operations
    const char** supported_ops;        // Null-terminated list of ONNX op names
    size_t supported_ops_count;

    // Tensor constraints
    int32_t max_tensor_rank;           // Maximum tensor dimensions (e.g., 8)
    int64_t max_tensor_dimension;      // Max size per dimension
    size_t max_model_size_bytes;       // Maximum model size

    // Precision support flags
    uint32_t supports_fp32 : 1;
    uint32_t supports_fp16 : 1;
    uint32_t supports_int8 : 1;
    uint32_t supports_dynamic_shapes : 1;
    uint32_t supports_batching : 1;
    uint32_t reserved_flags : 27;

    // Batch constraints
    int32_t max_batch_size;            // 0 = no batching support
    int32_t preferred_batch_size;

    // Memory limits
    size_t max_device_memory_bytes;    // Available device memory
    size_t max_tensor_size_bytes;      // Max single tensor size

    // Performance hints
    float relative_performance;        // 0.0-1.0 (1.0 = best available)
} CmwProviderCapabilities;

/**
 * Model metadata
 */
typedef struct {
    const char* model_path;            // Path to ONNX model file
    const void* model_data;            // Or in-memory model data
    size_t model_data_size;
    const char* cache_dir;             // Optional cache directory
} CmwModelInfo;

/**
 * Session configuration
 */
typedef struct {
    int32_t enable_profiling;
    int32_t enable_memory_pattern;
    int32_t inter_op_num_threads;
    int32_t intra_op_num_threads;
    const char** config_keys;          // Provider-specific configs
    const char** config_values;
    size_t config_count;
} CmwSessionConfig;

/**
 * Provider plugin interface
 */
typedef struct CmwProviderVTable {
    /**
     * Get provider capabilities
     * @param provider Provider instance
     * @param caps Output capabilities structure
     * @return Error code
     */
    CmwErrorCode (*get_capabilities)(
        CmwProvider* provider,
        CmwProviderCapabilities* caps
    );

    /**
     * Create inference session from model
     * @param provider Provider instance
     * @param model Model information
     * @param config Session configuration
     * @param session Output session handle
     * @return Error code
     */
    CmwErrorCode (*create_session)(
        CmwProvider* provider,
        const CmwModelInfo* model,
        const CmwSessionConfig* config,
        CmwSession** session
    );

    /**
     * Run inference
     * @param session Session handle
     * @param inputs Input tensors [input_count]
     * @param input_count Number of inputs
     * @param outputs Output tensors [output_count] (pre-allocated)
     * @param output_count Number of outputs
     * @return Error code
     */
    CmwErrorCode (*run_inference)(
        CmwSession* session,
        const CmwTensor* inputs,
        size_t input_count,
        CmwTensor* outputs,
        size_t output_count
    );

    /**
     * Destroy session and release resources
     * @param session Session to destroy
     * @return Error code
     */
    CmwErrorCode (*destroy_session)(
        CmwSession* session
    );

    /**
     * Validate if model is compatible with provider
     * @param provider Provider instance
     * @param model Model information
     * @param error_msg Output error message (if not compatible)
     * @return CMW_SUCCESS if compatible, error code otherwise
     */
    CmwErrorCode (*validate_model)(
        CmwProvider* provider,
        const CmwModelInfo* model,
        const char** error_msg
    );

    /**
     * Get session memory usage
     * @param session Session handle
     * @param bytes_used Output memory usage in bytes
     * @return Error code
     */
    CmwErrorCode (*get_memory_usage)(
        CmwSession* session,
        size_t* bytes_used
    );

} CmwProviderVTable;

/**
 * Provider instance
 */
struct CmwProvider {
    const CmwProviderVTable* vtable;
    void* user_data;                   // Provider-specific context
};

/**
 * Plugin entry point - each provider DLL must export this
 * @param provider Output provider instance
 * @return Error code
 */
typedef CmwErrorCode (*CmwProviderCreateFn)(CmwProvider** provider);

/**
 * Plugin cleanup - each provider DLL must export this
 * @param provider Provider to destroy
 * @return Error code
 */
typedef CmwErrorCode (*CmwProviderDestroyFn)(CmwProvider* provider);

// Standard plugin export names
#define CMW_PROVIDER_CREATE_FN_NAME "CmwCreateProvider"
#define CMW_PROVIDER_DESTROY_FN_NAME "CmwDestroyProvider"

#ifdef __cplusplus
}
#endif

#endif // COREMLWIN_PROVIDER_API_H
