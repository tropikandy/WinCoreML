/**
 * CoreML-on-Windows Runtime Error Strings
 */

#include "../include/coremlwin_errors.h"

const char* cmw_error_string(CmwErrorCode code) {
    switch (code) {
        case CMW_SUCCESS:
            return "Success";

        // Client Errors
        case CMW_ERROR_INVALID_ARGUMENT:
            return "Invalid argument provided";
        case CMW_ERROR_INVALID_MODEL_ID:
            return "Invalid model ID";
        case CMW_ERROR_INVALID_INPUT_SHAPE:
            return "Invalid input tensor shape";
        case CMW_ERROR_INVALID_INPUT_TYPE:
            return "Invalid input tensor type";
        case CMW_ERROR_INVALID_CONFIG:
            return "Invalid configuration";
        case CMW_ERROR_MISSING_REQUIRED_INPUT:
            return "Missing required input";
        case CMW_ERROR_UNSUPPORTED_FEATURE:
            return "Unsupported feature";
        case CMW_ERROR_PERMISSION_DENIED:
            return "Permission denied";
        case CMW_ERROR_QUOTA_EXCEEDED:
            return "Quota exceeded";
        case CMW_ERROR_INVALID_REQUEST:
            return "Invalid request";

        // Model Errors
        case CMW_ERROR_MODEL_NOT_FOUND:
            return "Model not found";
        case CMW_ERROR_MODEL_LOAD_FAILED:
            return "Failed to load model";
        case CMW_ERROR_MODEL_PARSE_FAILED:
            return "Failed to parse model";
        case CMW_ERROR_MODEL_VALIDATION_FAILED:
            return "Model validation failed";
        case CMW_ERROR_UNSUPPORTED_MODEL_VERSION:
            return "Unsupported model version";
        case CMW_ERROR_UNSUPPORTED_OPERATION:
            return "Model contains unsupported operations";
        case CMW_ERROR_MODEL_CONVERSION_FAILED:
            return "Model conversion failed";
        case CMW_ERROR_MODEL_CORRUPTED:
            return "Model file is corrupted";
        case CMW_ERROR_MODEL_TOO_LARGE:
            return "Model exceeds size limits";
        case CMW_ERROR_INCOMPATIBLE_MODEL:
            return "Model is incompatible with runtime";

        // Provider Errors
        case CMW_ERROR_PROVIDER_NOT_AVAILABLE:
            return "Requested provider is not available";
        case CMW_ERROR_PROVIDER_INIT_FAILED:
            return "Provider initialization failed";
        case CMW_ERROR_PROVIDER_EXECUTION_FAILED:
            return "Provider execution failed";
        case CMW_ERROR_DEVICE_NOT_FOUND:
            return "Compute device not found";
        case CMW_ERROR_DEVICE_LOST:
            return "Compute device was lost";
        case CMW_ERROR_DRIVER_ERROR:
            return "Device driver error";
        case CMW_ERROR_OUT_OF_DEVICE_MEMORY:
            return "Out of device memory";
        case CMW_ERROR_UNSUPPORTED_HARDWARE:
            return "Hardware is not supported";
        case CMW_ERROR_PROVIDER_NOT_COMPATIBLE:
            return "Provider is not compatible with model";
        case CMW_ERROR_PROVIDER_TIMEOUT:
            return "Provider execution timeout";

        // Runtime Errors
        case CMW_ERROR_INTERNAL:
            return "Internal runtime error";
        case CMW_ERROR_NOT_IMPLEMENTED:
            return "Feature not implemented";
        case CMW_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case CMW_ERROR_SERVICE_UNAVAILABLE:
            return "Runtime service is unavailable";
        case CMW_ERROR_SERVICE_INIT_FAILED:
            return "Service initialization failed";
        case CMW_ERROR_IPC_FAILED:
            return "IPC communication failed";
        case CMW_ERROR_CACHE_WRITE_FAILED:
            return "Failed to write to cache";
        case CMW_ERROR_CONFIG_LOAD_FAILED:
            return "Failed to load configuration";
        case CMW_ERROR_REGISTRY_ERROR:
            return "Model registry error";
        case CMW_ERROR_UNKNOWN:
            return "Unknown error";

        // Transient Errors
        case CMW_ERROR_TIMEOUT:
            return "Operation timed out";
        case CMW_ERROR_RESOURCE_BUSY:
            return "Resource is busy";
        case CMW_ERROR_RESOURCE_EXHAUSTED:
            return "Resource exhausted";
        case CMW_ERROR_CONNECTION_LOST:
            return "Connection lost";
        case CMW_ERROR_OPERATION_CANCELLED:
            return "Operation was cancelled";
        case CMW_ERROR_TEMPORARY_FAILURE:
            return "Temporary failure, retry recommended";
        case CMW_ERROR_RATE_LIMITED:
            return "Rate limit exceeded";

        default:
            return "Unknown error code";
    }
}
