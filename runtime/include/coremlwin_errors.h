/**
 * CoreML-on-Windows Runtime Error Codes
 *
 * Unified error taxonomy for the CoreML-on-Windows runtime.
 * Error codes are organized by category for clear diagnostics and handling.
 */

#ifndef COREMLWIN_ERRORS_H
#define COREMLWIN_ERRORS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Error code ranges:
 * 0         - Success
 * 1000-1999 - Client errors (invalid requests, bad parameters)
 * 2000-2999 - Model errors (invalid models, unsupported ops)
 * 3000-3999 - Provider errors (hardware/driver issues)
 * 4000-4999 - Runtime errors (internal failures)
 * 5000-5999 - Transient errors (retry-able failures)
 */
typedef enum {
    // Success
    CMW_SUCCESS = 0,

    // Client Errors (1000-1999)
    CMW_ERROR_INVALID_ARGUMENT = 1000,
    CMW_ERROR_INVALID_MODEL_ID = 1001,
    CMW_ERROR_INVALID_INPUT_SHAPE = 1002,
    CMW_ERROR_INVALID_INPUT_TYPE = 1003,
    CMW_ERROR_INVALID_CONFIG = 1004,
    CMW_ERROR_MISSING_REQUIRED_INPUT = 1005,
    CMW_ERROR_UNSUPPORTED_FEATURE = 1006,
    CMW_ERROR_PERMISSION_DENIED = 1007,
    CMW_ERROR_QUOTA_EXCEEDED = 1008,
    CMW_ERROR_INVALID_REQUEST = 1099,

    // Model Errors (2000-2999)
    CMW_ERROR_MODEL_NOT_FOUND = 2000,
    CMW_ERROR_MODEL_LOAD_FAILED = 2001,
    CMW_ERROR_MODEL_PARSE_FAILED = 2002,
    CMW_ERROR_MODEL_VALIDATION_FAILED = 2003,
    CMW_ERROR_UNSUPPORTED_MODEL_VERSION = 2004,
    CMW_ERROR_UNSUPPORTED_OPERATION = 2005,
    CMW_ERROR_MODEL_CONVERSION_FAILED = 2006,
    CMW_ERROR_MODEL_CORRUPTED = 2007,
    CMW_ERROR_MODEL_TOO_LARGE = 2008,
    CMW_ERROR_INCOMPATIBLE_MODEL = 2099,

    // Provider Errors (3000-3999)
    CMW_ERROR_PROVIDER_NOT_AVAILABLE = 3000,
    CMW_ERROR_PROVIDER_INIT_FAILED = 3001,
    CMW_ERROR_PROVIDER_EXECUTION_FAILED = 3002,
    CMW_ERROR_DEVICE_NOT_FOUND = 3003,
    CMW_ERROR_DEVICE_LOST = 3004,
    CMW_ERROR_DRIVER_ERROR = 3005,
    CMW_ERROR_OUT_OF_DEVICE_MEMORY = 3006,
    CMW_ERROR_UNSUPPORTED_HARDWARE = 3007,
    CMW_ERROR_PROVIDER_NOT_COMPATIBLE = 3008,
    CMW_ERROR_PROVIDER_TIMEOUT = 3009,

    // Runtime Errors (4000-4999)
    CMW_ERROR_INTERNAL = 4000,
    CMW_ERROR_NOT_IMPLEMENTED = 4001,
    CMW_ERROR_OUT_OF_MEMORY = 4002,
    CMW_ERROR_SERVICE_UNAVAILABLE = 4003,
    CMW_ERROR_SERVICE_INIT_FAILED = 4004,
    CMW_ERROR_IPC_FAILED = 4005,
    CMW_ERROR_CACHE_WRITE_FAILED = 4006,
    CMW_ERROR_CONFIG_LOAD_FAILED = 4007,
    CMW_ERROR_REGISTRY_ERROR = 4008,
    CMW_ERROR_UNSUPPORTED_PLATFORM = 4009,
    CMW_ERROR_UNKNOWN = 4099,

    // Transient Errors (5000-5999) - Retry-able
    CMW_ERROR_TIMEOUT = 5000,
    CMW_ERROR_RESOURCE_BUSY = 5001,
    CMW_ERROR_RESOURCE_EXHAUSTED = 5002,
    CMW_ERROR_CONNECTION_LOST = 5003,
    CMW_ERROR_OPERATION_CANCELLED = 5004,
    CMW_ERROR_TEMPORARY_FAILURE = 5005,
    CMW_ERROR_RATE_LIMITED = 5006,

} CmwErrorCode;

/**
 * Check if an error is transient (retry-able)
 */
inline int cmw_is_transient_error(CmwErrorCode code) {
    return code >= 5000 && code < 6000;
}

/**
 * Check if an error is a client error
 */
inline int cmw_is_client_error(CmwErrorCode code) {
    return code >= 1000 && code < 2000;
}

/**
 * Check if an error is a model error
 */
inline int cmw_is_model_error(CmwErrorCode code) {
    return code >= 2000 && code < 3000;
}

/**
 * Check if an error is a provider error
 */
inline int cmw_is_provider_error(CmwErrorCode code) {
    return code >= 3000 && code < 4000;
}

/**
 * Check if an error is a runtime error
 */
inline int cmw_is_runtime_error(CmwErrorCode code) {
    return code >= 4000 && code < 5000;
}

/**
 * Get human-readable error message
 */
const char* cmw_error_string(CmwErrorCode code);

#ifdef __cplusplus
}
#endif

#endif // COREMLWIN_ERRORS_H
