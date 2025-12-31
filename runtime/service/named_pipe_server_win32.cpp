/**
 * Windows Named Pipe IPC Server
 *
 * Implements async named pipe server for Windows with:
 * - Length-prefixed Protocol Buffers messages
 * - Multiple concurrent clients
 * - Overlapped I/O for async operations
 * - Proper error handling and cleanup
 */

#ifdef _WIN32

#include "named_pipe_server_win32.h"
#include "../include/coremlwin_errors.h"
#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>

namespace {
    const size_t MAX_CLIENTS = 32;
    const size_t BUFFER_SIZE = 1024 * 1024;  // 1MB buffer
    const DWORD PIPE_TIMEOUT_MS = 5000;
}

// Client connection state
struct ClientConnection {
    HANDLE pipe_handle;
    OVERLAPPED overlap;
    std::vector<uint8_t> read_buffer;
    std::vector<uint8_t> write_buffer;
    uint32_t message_length;
    bool reading_length;
    bool active;
};

class NamedPipeServerImpl {
public:
    NamedPipeServerImpl(const std::string& pipe_name)
        : pipe_name_(pipe_name)
        , running_(false)
        , completion_port_(NULL)
    {}

    ~NamedPipeServerImpl() {
        Stop();
    }

    CmwErrorCode Start(MessageHandler handler) {
        if (running_) {
            return CMW_ERROR_INTERNAL;
        }

        message_handler_ = handler;

        // Create I/O completion port
        completion_port_ = CreateIoCompletionPort(
            INVALID_HANDLE_VALUE,
            NULL,
            0,
            0
        );

        if (completion_port_ == NULL) {
            return CMW_ERROR_SERVICE_INIT_FAILED;
        }

        running_ = true;

        // Start worker threads
        size_t num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4;

        for (size_t i = 0; i < num_threads; i++) {
            worker_threads_.emplace_back([this]() {
                WorkerThreadProc();
            });
        }

        // Start connection acceptor thread
        acceptor_thread_ = std::thread([this]() {
            AcceptorThreadProc();
        });

        std::cout << "Named pipe server started: " << pipe_name_ << std::endl;
        return CMW_SUCCESS;
    }

    void Stop() {
        if (!running_) {
            return;
        }

        running_ = false;

        // Close completion port to wake up worker threads
        if (completion_port_) {
            CloseHandle(completion_port_);
            completion_port_ = NULL;
        }

        // Join threads
        if (acceptor_thread_.joinable()) {
            acceptor_thread_.join();
        }

        for (auto& thread : worker_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        // Close all client connections
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (auto& client : clients_) {
            if (client && client->pipe_handle != INVALID_HANDLE_VALUE) {
                CloseHandle(client->pipe_handle);
            }
        }
        clients_.clear();

        std::cout << "Named pipe server stopped" << std::endl;
    }

private:
    void AcceptorThreadProc() {
        while (running_) {
            // Create named pipe instance
            HANDLE pipe_handle = CreateNamedPipeA(
                pipe_name_.c_str(),
                PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                MAX_CLIENTS,
                BUFFER_SIZE,
                BUFFER_SIZE,
                PIPE_TIMEOUT_MS,
                NULL
            );

            if (pipe_handle == INVALID_HANDLE_VALUE) {
                std::cerr << "Failed to create named pipe: " << GetLastError() << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            // Wait for client connection
            OVERLAPPED overlap = {0};
            overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

            BOOL connected = ConnectNamedPipe(pipe_handle, &overlap);
            DWORD last_error = GetLastError();

            if (!connected) {
                if (last_error == ERROR_IO_PENDING) {
                    // Wait for connection
                    DWORD wait_result = WaitForSingleObject(overlap.hEvent, INFINITE);
                    if (wait_result != WAIT_OBJECT_0 || !running_) {
                        CloseHandle(overlap.hEvent);
                        CloseHandle(pipe_handle);
                        continue;
                    }
                } else if (last_error != ERROR_PIPE_CONNECTED) {
                    CloseHandle(overlap.hEvent);
                    CloseHandle(pipe_handle);
                    continue;
                }
            }

            CloseHandle(overlap.hEvent);

            if (!running_) {
                CloseHandle(pipe_handle);
                break;
            }

            // Create client connection
            auto client = std::make_shared<ClientConnection>();
            client->pipe_handle = pipe_handle;
            client->read_buffer.resize(BUFFER_SIZE);
            client->write_buffer.reserve(BUFFER_SIZE);
            client->message_length = 0;
            client->reading_length = true;
            client->active = true;
            memset(&client->overlap, 0, sizeof(OVERLAPPED));

            // Associate pipe with completion port
            CreateIoCompletionPort(
                pipe_handle,
                completion_port_,
                (ULONG_PTR)client.get(),
                0
            );

            // Add to client list
            {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                clients_.push_back(client);
            }

            // Start reading
            StartRead(client);

            std::cout << "Client connected" << std::endl;
        }
    }

    void WorkerThreadProc() {
        while (running_) {
            DWORD bytes_transferred = 0;
            ULONG_PTR completion_key = 0;
            OVERLAPPED* overlap = nullptr;

            BOOL result = GetQueuedCompletionStatus(
                completion_port_,
                &bytes_transferred,
                &completion_key,
                &overlap,
                INFINITE
            );

            if (!running_) {
                break;
            }

            if (!result || bytes_transferred == 0) {
                // Client disconnected or error
                if (completion_key) {
                    auto client = (ClientConnection*)completion_key;
                    HandleDisconnect(client);
                }
                continue;
            }

            auto client = (ClientConnection*)completion_key;
            if (!client || !client->active) {
                continue;
            }

            // Handle read completion
            HandleReadCompletion(client, bytes_transferred);
        }
    }

    void StartRead(std::shared_ptr<ClientConnection> client) {
        if (!client->active) {
            return;
        }

        memset(&client->overlap, 0, sizeof(OVERLAPPED));

        size_t bytes_to_read = client->reading_length ? 4 : client->message_length;

        BOOL result = ReadFile(
            client->pipe_handle,
            client->read_buffer.data(),
            static_cast<DWORD>(bytes_to_read),
            NULL,
            &client->overlap
        );

        DWORD last_error = GetLastError();
        if (!result && last_error != ERROR_IO_PENDING) {
            HandleDisconnect(client.get());
        }
    }

    void HandleReadCompletion(ClientConnection* client, DWORD bytes_read) {
        if (client->reading_length) {
            // Read message length (4 bytes, little-endian)
            if (bytes_read < 4) {
                HandleDisconnect(client);
                return;
            }

            client->message_length =
                client->read_buffer[0] |
                (client->read_buffer[1] << 8) |
                (client->read_buffer[2] << 16) |
                (client->read_buffer[3] << 24);

            if (client->message_length > BUFFER_SIZE) {
                std::cerr << "Message too large: " << client->message_length << std::endl;
                HandleDisconnect(client);
                return;
            }

            client->reading_length = false;

            // Start reading message body
            auto client_shared = FindClient(client);
            if (client_shared) {
                StartRead(client_shared);
            }
        } else {
            // Read message body
            if (bytes_read < client->message_length) {
                HandleDisconnect(client);
                return;
            }

            // Process message
            std::vector<uint8_t> request_data(
                client->read_buffer.begin(),
                client->read_buffer.begin() + client->message_length
            );

            // Call message handler
            std::vector<uint8_t> response_data;
            if (message_handler_) {
                response_data = message_handler_(request_data);
            }

            // Send response
            SendResponse(client, response_data);

            // Reset for next message
            client->reading_length = true;
            client->message_length = 0;

            auto client_shared = FindClient(client);
            if (client_shared) {
                StartRead(client_shared);
            }
        }
    }

    void SendResponse(ClientConnection* client, const std::vector<uint8_t>& data) {
        // Build response: 4-byte length + data
        client->write_buffer.clear();
        uint32_t length = static_cast<uint32_t>(data.size());

        client->write_buffer.push_back(length & 0xFF);
        client->write_buffer.push_back((length >> 8) & 0xFF);
        client->write_buffer.push_back((length >> 16) & 0xFF);
        client->write_buffer.push_back((length >> 24) & 0xFF);

        client->write_buffer.insert(
            client->write_buffer.end(),
            data.begin(),
            data.end()
        );

        // Write response
        DWORD bytes_written = 0;
        BOOL result = WriteFile(
            client->pipe_handle,
            client->write_buffer.data(),
            static_cast<DWORD>(client->write_buffer.size()),
            &bytes_written,
            NULL
        );

        if (!result) {
            HandleDisconnect(client);
        }
    }

    void HandleDisconnect(ClientConnection* client) {
        if (!client || !client->active) {
            return;
        }

        client->active = false;

        std::lock_guard<std::mutex> lock(clients_mutex_);

        // Remove from client list
        clients_.erase(
            std::remove_if(
                clients_.begin(),
                clients_.end(),
                [client](const std::shared_ptr<ClientConnection>& c) {
                    return c.get() == client;
                }
            ),
            clients_.end()
        );

        if (client->pipe_handle != INVALID_HANDLE_VALUE) {
            CloseHandle(client->pipe_handle);
            client->pipe_handle = INVALID_HANDLE_VALUE;
        }

        std::cout << "Client disconnected" << std::endl;
    }

    std::shared_ptr<ClientConnection> FindClient(ClientConnection* client) {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (auto& c : clients_) {
            if (c.get() == client) {
                return c;
            }
        }
        return nullptr;
    }

    std::string pipe_name_;
    bool running_;
    HANDLE completion_port_;
    MessageHandler message_handler_;

    std::thread acceptor_thread_;
    std::vector<std::thread> worker_threads_;

    std::mutex clients_mutex_;
    std::vector<std::shared_ptr<ClientConnection>> clients_;
};

// Public API implementation

NamedPipeServer::NamedPipeServer(const std::string& pipe_name)
    : impl_(new NamedPipeServerImpl(pipe_name))
{}

NamedPipeServer::~NamedPipeServer() {
    delete impl_;
}

CmwErrorCode NamedPipeServer::Start(MessageHandler handler) {
    return impl_->Start(handler);
}

void NamedPipeServer::Stop() {
    impl_->Stop();
}

#endif // _WIN32
