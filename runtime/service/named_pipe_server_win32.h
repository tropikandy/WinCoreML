/**
 * Windows Named Pipe IPC Server Header
 */

#ifndef NAMED_PIPE_SERVER_WIN32_H
#define NAMED_PIPE_SERVER_WIN32_H

#ifdef _WIN32

#include "../include/coremlwin_errors.h"
#include <string>
#include <vector>
#include <functional>

class NamedPipeServerImpl;

/**
 * Message handler callback
 * Takes request data, returns response data
 */
using MessageHandler = std::function<std::vector<uint8_t>(const std::vector<uint8_t>&)>;

/**
 * Named Pipe Server for Windows
 *
 * Implements async named pipe server with:
 * - Length-prefixed messages (4-byte little-endian + data)
 * - Multiple concurrent clients
 * - Overlapped I/O
 * - Thread pool for handling requests
 */
class NamedPipeServer {
public:
    /**
     * Create named pipe server
     * @param pipe_name Pipe name (e.g., "\\\\.\\pipe\\coremlwin_runtime")
     */
    explicit NamedPipeServer(const std::string& pipe_name);

    ~NamedPipeServer();

    /**
     * Start server and begin accepting connections
     * @param handler Message handler callback
     * @return Error code
     */
    CmwErrorCode Start(MessageHandler handler);

    /**
     * Stop server and disconnect all clients
     */
    void Stop();

private:
    NamedPipeServerImpl* impl_;

    // Non-copyable
    NamedPipeServer(const NamedPipeServer&) = delete;
    NamedPipeServer& operator=(const NamedPipeServer&) = delete;
};

#endif // _WIN32

#endif // NAMED_PIPE_SERVER_WIN32_H
