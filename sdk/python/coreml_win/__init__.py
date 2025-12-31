"""
CoreML-on-Windows Python SDK

Provides a Python interface to the CoreMLWin runtime service.
"""

from .errors import (
    CoreMLWinError,
    ClientError,
    ModelError,
    ProviderError,
    RuntimeError,
    TransientError,
    from_error_code,
)

__version__ = "0.1.0"

__all__ = [
    "CoreMLWinError",
    "ClientError",
    "ModelError",
    "ProviderError",
    "RuntimeError",
    "TransientError",
    "from_error_code",
]
