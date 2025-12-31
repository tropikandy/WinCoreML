"""
CoreML-on-Windows Python SDK Error Classes

Provides Python exception classes mapped to the unified error taxonomy.
"""

from typing import Optional


class CoreMLWinError(Exception):
    """Base exception for all CoreMLWin errors."""

    def __init__(self, message: str, code: int, details: Optional[str] = None):
        super().__init__(message)
        self.message = message
        self.code = code
        self.details = details

    def __str__(self) -> str:
        base = f"[{self.code}] {self.message}"
        if self.details:
            base += f"\nDetails: {self.details}"
        return base

    @property
    def is_transient(self) -> bool:
        """Check if this is a transient (retry-able) error."""
        return 5000 <= self.code < 6000

    @property
    def is_client_error(self) -> bool:
        """Check if this is a client error."""
        return 1000 <= self.code < 2000

    @property
    def is_model_error(self) -> bool:
        """Check if this is a model error."""
        return 2000 <= self.code < 3000

    @property
    def is_provider_error(self) -> bool:
        """Check if this is a provider error."""
        return 3000 <= self.code < 4000

    @property
    def is_runtime_error(self) -> bool:
        """Check if this is a runtime error."""
        return 4000 <= self.code < 5000


# ============================================================================
# Client Errors (1000-1999)
# ============================================================================

class ClientError(CoreMLWinError):
    """Base class for client errors (1000-1999)."""
    pass


class InvalidArgumentError(ClientError):
    """Invalid argument provided (1000)."""
    def __init__(self, message: str = "Invalid argument provided", details: Optional[str] = None):
        super().__init__(message, 1000, details)


class InvalidModelIdError(ClientError):
    """Invalid model ID (1001)."""
    def __init__(self, message: str = "Invalid model ID", details: Optional[str] = None):
        super().__init__(message, 1001, details)


class InvalidInputShapeError(ClientError):
    """Invalid input tensor shape (1002)."""
    def __init__(self, message: str = "Invalid input tensor shape", details: Optional[str] = None):
        super().__init__(message, 1002, details)


class InvalidInputTypeError(ClientError):
    """Invalid input tensor type (1003)."""
    def __init__(self, message: str = "Invalid input tensor type", details: Optional[str] = None):
        super().__init__(message, 1003, details)


class InvalidConfigError(ClientError):
    """Invalid configuration (1004)."""
    def __init__(self, message: str = "Invalid configuration", details: Optional[str] = None):
        super().__init__(message, 1004, details)


class MissingRequiredInputError(ClientError):
    """Missing required input (1005)."""
    def __init__(self, message: str = "Missing required input", details: Optional[str] = None):
        super().__init__(message, 1005, details)


class UnsupportedFeatureError(ClientError):
    """Unsupported feature (1006)."""
    def __init__(self, message: str = "Unsupported feature", details: Optional[str] = None):
        super().__init__(message, 1006, details)


class PermissionDeniedError(ClientError):
    """Permission denied (1007)."""
    def __init__(self, message: str = "Permission denied", details: Optional[str] = None):
        super().__init__(message, 1007, details)


class QuotaExceededError(ClientError):
    """Quota exceeded (1008)."""
    def __init__(self, message: str = "Quota exceeded", details: Optional[str] = None):
        super().__init__(message, 1008, details)


# ============================================================================
# Model Errors (2000-2999)
# ============================================================================

class ModelError(CoreMLWinError):
    """Base class for model errors (2000-2999)."""
    pass


class ModelNotFoundError(ModelError):
    """Model not found (2000)."""
    def __init__(self, message: str = "Model not found", details: Optional[str] = None):
        super().__init__(message, 2000, details)


class ModelLoadFailedError(ModelError):
    """Failed to load model (2001)."""
    def __init__(self, message: str = "Failed to load model", details: Optional[str] = None):
        super().__init__(message, 2001, details)


class ModelParseFailedError(ModelError):
    """Failed to parse model (2002)."""
    def __init__(self, message: str = "Failed to parse model", details: Optional[str] = None):
        super().__init__(message, 2002, details)


class ModelValidationFailedError(ModelError):
    """Model validation failed (2003)."""
    def __init__(self, message: str = "Model validation failed", details: Optional[str] = None):
        super().__init__(message, 2003, details)


class UnsupportedModelVersionError(ModelError):
    """Unsupported model version (2004)."""
    def __init__(self, message: str = "Unsupported model version", details: Optional[str] = None):
        super().__init__(message, 2004, details)


class UnsupportedOperationError(ModelError):
    """Model contains unsupported operations (2005)."""
    def __init__(self, message: str = "Model contains unsupported operations", details: Optional[str] = None):
        super().__init__(message, 2005, details)


class ModelConversionFailedError(ModelError):
    """Model conversion failed (2006)."""
    def __init__(self, message: str = "Model conversion failed", details: Optional[str] = None):
        super().__init__(message, 2006, details)


class ModelCorruptedError(ModelError):
    """Model file is corrupted (2007)."""
    def __init__(self, message: str = "Model file is corrupted", details: Optional[str] = None):
        super().__init__(message, 2007, details)


class ModelTooLargeError(ModelError):
    """Model exceeds size limits (2008)."""
    def __init__(self, message: str = "Model exceeds size limits", details: Optional[str] = None):
        super().__init__(message, 2008, details)


# ============================================================================
# Provider Errors (3000-3999)
# ============================================================================

class ProviderError(CoreMLWinError):
    """Base class for provider errors (3000-3999)."""
    pass


class ProviderNotAvailableError(ProviderError):
    """Requested provider is not available (3000)."""
    def __init__(self, message: str = "Requested provider is not available", details: Optional[str] = None):
        super().__init__(message, 3000, details)


class ProviderInitFailedError(ProviderError):
    """Provider initialization failed (3001)."""
    def __init__(self, message: str = "Provider initialization failed", details: Optional[str] = None):
        super().__init__(message, 3001, details)


class ProviderExecutionFailedError(ProviderError):
    """Provider execution failed (3002)."""
    def __init__(self, message: str = "Provider execution failed", details: Optional[str] = None):
        super().__init__(message, 3002, details)


class DeviceNotFoundError(ProviderError):
    """Compute device not found (3003)."""
    def __init__(self, message: str = "Compute device not found", details: Optional[str] = None):
        super().__init__(message, 3003, details)


class DeviceLostError(ProviderError):
    """Compute device was lost (3004)."""
    def __init__(self, message: str = "Compute device was lost", details: Optional[str] = None):
        super().__init__(message, 3004, details)


class DriverError(ProviderError):
    """Device driver error (3005)."""
    def __init__(self, message: str = "Device driver error", details: Optional[str] = None):
        super().__init__(message, 3005, details)


class OutOfDeviceMemoryError(ProviderError):
    """Out of device memory (3006)."""
    def __init__(self, message: str = "Out of device memory", details: Optional[str] = None):
        super().__init__(message, 3006, details)


class UnsupportedHardwareError(ProviderError):
    """Hardware is not supported (3007)."""
    def __init__(self, message: str = "Hardware is not supported", details: Optional[str] = None):
        super().__init__(message, 3007, details)


class ProviderNotCompatibleError(ProviderError):
    """Provider is not compatible with model (3008)."""
    def __init__(self, message: str = "Provider is not compatible with model", details: Optional[str] = None):
        super().__init__(message, 3008, details)


class ProviderTimeoutError(ProviderError):
    """Provider execution timeout (3009)."""
    def __init__(self, message: str = "Provider execution timeout", details: Optional[str] = None):
        super().__init__(message, 3009, details)


# ============================================================================
# Runtime Errors (4000-4999)
# ============================================================================

class RuntimeError(CoreMLWinError):
    """Base class for runtime errors (4000-4999)."""
    pass


class InternalError(RuntimeError):
    """Internal runtime error (4000)."""
    def __init__(self, message: str = "Internal runtime error", details: Optional[str] = None):
        super().__init__(message, 4000, details)


class NotImplementedError(RuntimeError):
    """Feature not implemented (4001)."""
    def __init__(self, message: str = "Feature not implemented", details: Optional[str] = None):
        super().__init__(message, 4001, details)


class OutOfMemoryError(RuntimeError):
    """Out of memory (4002)."""
    def __init__(self, message: str = "Out of memory", details: Optional[str] = None):
        super().__init__(message, 4002, details)


class ServiceUnavailableError(RuntimeError):
    """Runtime service is unavailable (4003)."""
    def __init__(self, message: str = "Runtime service is unavailable", details: Optional[str] = None):
        super().__init__(message, 4003, details)


class ServiceInitFailedError(RuntimeError):
    """Service initialization failed (4004)."""
    def __init__(self, message: str = "Service initialization failed", details: Optional[str] = None):
        super().__init__(message, 4004, details)


class IPCFailedError(RuntimeError):
    """IPC communication failed (4005)."""
    def __init__(self, message: str = "IPC communication failed", details: Optional[str] = None):
        super().__init__(message, 4005, details)


class CacheWriteFailedError(RuntimeError):
    """Failed to write to cache (4006)."""
    def __init__(self, message: str = "Failed to write to cache", details: Optional[str] = None):
        super().__init__(message, 4006, details)


class ConfigLoadFailedError(RuntimeError):
    """Failed to load configuration (4007)."""
    def __init__(self, message: str = "Failed to load configuration", details: Optional[str] = None):
        super().__init__(message, 4007, details)


class RegistryError(RuntimeError):
    """Model registry error (4008)."""
    def __init__(self, message: str = "Model registry error", details: Optional[str] = None):
        super().__init__(message, 4008, details)


class UnsupportedPlatformError(RuntimeError):
    """Platform not supported (4009)."""
    def __init__(self, message: str = "Platform not supported", details: Optional[str] = None):
        super().__init__(message, 4009, details)


# ============================================================================
# Transient Errors (5000-5999)
# ============================================================================

class TransientError(CoreMLWinError):
    """Base class for transient (retry-able) errors (5000-5999)."""
    pass


class TimeoutError(TransientError):
    """Operation timed out (5000)."""
    def __init__(self, message: str = "Operation timed out", details: Optional[str] = None):
        super().__init__(message, 5000, details)


class ResourceBusyError(TransientError):
    """Resource is busy (5001)."""
    def __init__(self, message: str = "Resource is busy", details: Optional[str] = None):
        super().__init__(message, 5001, details)


class ResourceExhaustedError(TransientError):
    """Resource exhausted (5002)."""
    def __init__(self, message: str = "Resource exhausted", details: Optional[str] = None):
        super().__init__(message, 5002, details)


class ConnectionLostError(TransientError):
    """Connection lost (5003)."""
    def __init__(self, message: str = "Connection lost", details: Optional[str] = None):
        super().__init__(message, 5003, details)


class OperationCancelledError(TransientError):
    """Operation was cancelled (5004)."""
    def __init__(self, message: str = "Operation was cancelled", details: Optional[str] = None):
        super().__init__(message, 5004, details)


class TemporaryFailureError(TransientError):
    """Temporary failure, retry recommended (5005)."""
    def __init__(self, message: str = "Temporary failure, retry recommended", details: Optional[str] = None):
        super().__init__(message, 5005, details)


class RateLimitedError(TransientError):
    """Rate limit exceeded (5006)."""
    def __init__(self, message: str = "Rate limit exceeded", details: Optional[str] = None):
        super().__init__(message, 5006, details)


# ============================================================================
# Error Factory
# ============================================================================

# Map error codes to exception classes
_ERROR_CODE_MAP = {
    # Client errors
    1000: InvalidArgumentError,
    1001: InvalidModelIdError,
    1002: InvalidInputShapeError,
    1003: InvalidInputTypeError,
    1004: InvalidConfigError,
    1005: MissingRequiredInputError,
    1006: UnsupportedFeatureError,
    1007: PermissionDeniedError,
    1008: QuotaExceededError,

    # Model errors
    2000: ModelNotFoundError,
    2001: ModelLoadFailedError,
    2002: ModelParseFailedError,
    2003: ModelValidationFailedError,
    2004: UnsupportedModelVersionError,
    2005: UnsupportedOperationError,
    2006: ModelConversionFailedError,
    2007: ModelCorruptedError,
    2008: ModelTooLargeError,

    # Provider errors
    3000: ProviderNotAvailableError,
    3001: ProviderInitFailedError,
    3002: ProviderExecutionFailedError,
    3003: DeviceNotFoundError,
    3004: DeviceLostError,
    3005: DriverError,
    3006: OutOfDeviceMemoryError,
    3007: UnsupportedHardwareError,
    3008: ProviderNotCompatibleError,
    3009: ProviderTimeoutError,

    # Runtime errors
    4000: InternalError,
    4001: NotImplementedError,
    4002: OutOfMemoryError,
    4003: ServiceUnavailableError,
    4004: ServiceInitFailedError,
    4005: IPCFailedError,
    4006: CacheWriteFailedError,
    4007: ConfigLoadFailedError,
    4008: RegistryError,
    4009: UnsupportedPlatformError,

    # Transient errors
    5000: TimeoutError,
    5001: ResourceBusyError,
    5002: ResourceExhaustedError,
    5003: ConnectionLostError,
    5004: OperationCancelledError,
    5005: TemporaryFailureError,
    5006: RateLimitedError,
}


def from_error_code(code: int, message: str, details: Optional[str] = None) -> CoreMLWinError:
    """
    Create appropriate exception instance from error code.

    Args:
        code: Error code from CmwErrorCode enum
        message: Error message
        details: Optional additional details

    Returns:
        Specific exception instance for the error code
    """
    error_class = _ERROR_CODE_MAP.get(code)

    if error_class:
        return error_class(message, details)

    # Fallback to generic error with category detection
    if 1000 <= code < 2000:
        return ClientError(message, code, details)
    elif 2000 <= code < 3000:
        return ModelError(message, code, details)
    elif 3000 <= code < 4000:
        return ProviderError(message, code, details)
    elif 4000 <= code < 5000:
        return RuntimeError(message, code, details)
    elif 5000 <= code < 6000:
        return TransientError(message, code, details)
    else:
        return CoreMLWinError(message, code, details)
