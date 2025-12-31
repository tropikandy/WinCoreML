"""
Windows Named Pipe Client

Communicates with the Universal ML Runtime service via named pipes.
"""

import struct
import logging
from typing import Optional
from pathlib import Path

# Windows-specific imports
try:
    import win32file
    import win32pipe
    import pywintypes
    WINDOWS_AVAILABLE = True
except ImportError:
    WINDOWS_AVAILABLE = False
    # Fallback for development on non-Windows platforms
    win32file = None
    win32pipe = None
    pywintypes = None

from .errors import from_error_code, ServiceUnavailableError, IPCFailedError


logger = logging.getLogger(__name__)


class NamedPipeClient:
    """
    Named Pipe IPC client for Windows.

    Handles connection to runtime service and message exchange with
    length-prefixed Protocol Buffers.
    """

    def __init__(
        self,
        pipe_name: str = r"\\.\pipe\coremlwin_runtime",
        timeout_ms: int = 30000
    ):
        """
        Initialize named pipe client.

        Args:
            pipe_name: Named pipe path
            timeout_ms: Connection timeout in milliseconds
        """
        self.pipe_name = pipe_name
        self.timeout_ms = timeout_ms
        self.pipe_handle = None

    def connect(self) -> None:
        """
        Connect to named pipe server.

        Raises:
            ServiceUnavailableError: If server is not running
            IPCFailedError: If connection fails
        """
        if not WINDOWS_AVAILABLE:
            raise ServiceUnavailableError(
                "Windows named pipes not available (pywin32 not installed)"
            )

        try:
            # Wait for pipe to be available
            win32pipe.WaitNamedPipe(self.pipe_name, self.timeout_ms)

            # Open pipe
            self.pipe_handle = win32file.CreateFile(
                self.pipe_name,
                win32file.GENERIC_READ | win32file.GENERIC_WRITE,
                0,
                None,
                win32file.OPEN_EXISTING,
                0,
                None
            )

            # Set pipe mode to message mode
            win32pipe.SetNamedPipeHandleState(
                self.pipe_handle,
                win32pipe.PIPE_READMODE_BYTE,
                None,
                None
            )

            logger.info(f"Connected to pipe: {self.pipe_name}")

        except pywintypes.error as e:
            error_code, func_name, error_msg = e.args
            if error_code == 2:  # ERROR_FILE_NOT_FOUND
                raise ServiceUnavailableError(
                    f"Runtime service not running: {error_msg}"
                )
            else:
                raise IPCFailedError(
                    f"Failed to connect to pipe: {error_msg}"
                )

    def disconnect(self) -> None:
        """Disconnect from named pipe."""
        if self.pipe_handle:
            try:
                win32file.CloseHandle(self.pipe_handle)
            except:
                pass
            self.pipe_handle = None
            logger.info("Disconnected from pipe")

    def send_message(self, data: bytes) -> bytes:
        """
        Send message and receive response.

        Args:
            data: Message data (protobuf bytes)

        Returns:
            Response data (protobuf bytes)

        Raises:
            IPCFailedError: If communication fails
        """
        if not self.pipe_handle:
            raise IPCFailedError("Not connected to pipe")

        try:
            # Send message with length prefix (4 bytes little-endian)
            length = len(data)
            length_bytes = struct.pack('<I', length)
            message = length_bytes + data

            # Write to pipe
            win32file.WriteFile(self.pipe_handle, message)
            logger.debug(f"Sent {len(message)} bytes")

            # Read response length (4 bytes)
            result, length_bytes = win32file.ReadFile(self.pipe_handle, 4)
            if result != 0:
                raise IPCFailedError(f"Pipe read failed: {result}")

            response_length = struct.unpack('<I', length_bytes)[0]
            logger.debug(f"Response length: {response_length}")

            if response_length > 10 * 1024 * 1024:  # 10MB sanity check
                raise IPCFailedError(f"Response too large: {response_length}")

            # Read response data
            result, response_data = win32file.ReadFile(
                self.pipe_handle,
                response_length
            )

            if result != 0:
                raise IPCFailedError(f"Pipe read failed: {result}")

            logger.debug(f"Received {len(response_data)} bytes")
            return bytes(response_data)

        except pywintypes.error as e:
            error_code, func_name, error_msg = e.args
            raise IPCFailedError(f"Pipe communication error: {error_msg}")

    def __enter__(self):
        """Context manager entry."""
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit."""
        self.disconnect()
        return False
