"""
Universal ML Runtime Python SDK

Provides a Python interface to the Universal ML Runtime service.
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
from .client import RuntimeClient
from .pipe_client import NamedPipeClient

__version__ = "0.1.0"

__all__ = [
    # Client
    "RuntimeClient",
    "NamedPipeClient",
    # Errors
    "CoreMLWinError",
    "ClientError",
    "ModelError",
    "ProviderError",
    "RuntimeError",
    "TransientError",
    "from_error_code",
]
