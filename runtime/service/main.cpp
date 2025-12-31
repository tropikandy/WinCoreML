/**
 * Universal ML Runtime Service
 *
 * Main entry point for the runtime service.
 * Integrates: Named Pipe Server + Runtime State + Model Registry + ONNX Executor
 */

#include "../include/coremlwin_errors.h"
#include "runtime_state.h"

#ifdef _WIN32
#include "named_pipe_server_win32.h"
#endif

#include <iostream>
#include <cstring>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

// Version information
#define UMLRT_VERSION_MAJOR 0
#define UMLRT_VERSION_MINOR 1
#define UMLRT_VERSION_PATCH 0

// Global state for signal handling
std::atomic<bool> g_running(true);

#ifdef _WIN32
NamedPipeServer* g_server = nullptr;
#endif

RuntimeState* g_runtime_state = nullptr;

void signal_handler(int signal) {
    std::cout << "\nShutdown signal received (" << signal << ")" << std::endl;
    g_running = false;

#ifdef _WIN32
    if (g_server) {
        g_server->Stop();
    }
#endif
}

void print_version() {
    std::cout << "Universal ML Runtime Service v"
              << UMLRT_VERSION_MAJOR << "."
              << UMLRT_VERSION_MINOR << "."
              << UMLRT_VERSION_PATCH << std::endl;
    std::cout << "Supports: PyTorch, TensorFlow, CoreML, ONNX" << std::endl;
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]\n"
              << "\n"
              << "Options:\n"
              << "  --version        Print version and exit\n"
              << "  --dev            Run in development mode\n"
              << "  --cache PATH     Cache directory (default: ./cache)\n"
              << "  --pipe NAME      Pipe name (default: \\\\.\\pipe\\coremlwin_runtime)\n"
              << "  --help           Show this help message\n"
              << std::endl;
}

/**
 * Message handler for IPC requests
 * This is a simplified handler - in production would use protobuf
 */
std::vector<uint8_t> handle_message(const std::vector<uint8_t>& request_data) {
    // PLACEHOLDER: Simple echo for testing
    // In production, would deserialize protobuf, dispatch to RuntimeState, serialize response

    std::cout << "Received request: " << request_data.size() << " bytes" << std::endl;

    // For now, just echo back with a status byte prepended
    std::vector<uint8_t> response;
    response.push_back(0);  // Success
    response.insert(response.end(), request_data.begin(), request_data.end());

    return response;
}

int main(int argc, char* argv[]) {
    bool dev_mode = false;
    std::string cache_dir = "./cache";
    std::string pipe_name = "\\\\.\\pipe\\coremlwin_runtime";

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else if (std::strcmp(argv[i], "--dev") == 0) {
            dev_mode = true;
        } else if (std::strcmp(argv[i], "--cache") == 0) {
            if (i + 1 < argc) {
                cache_dir = argv[++i];
            } else {
                std::cerr << "Error: --cache requires a path argument" << std::endl;
                return 1;
            }
        } else if (std::strcmp(argv[i], "--pipe") == 0) {
            if (i + 1 < argc) {
                pipe_name = argv[++i];
            } else {
                std::cerr << "Error: --pipe requires a name argument" << std::endl;
                return 1;
            }
        } else if (std::strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown option: " << argv[i] << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    // Print banner
    std::cout << "========================================" << std::endl;
    print_version();
    std::cout << "========================================" << std::endl;

    if (dev_mode) {
        std::cout << "Running in DEVELOPMENT mode" << std::endl;
    }

    // Install signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Initialize runtime state
    std::cout << "\n[1/3] Initializing runtime state..." << std::endl;
    RuntimeState runtime_state;
    g_runtime_state = &runtime_state;

    CmwErrorCode result = runtime_state.Initialize(cache_dir);
    if (result != CMW_SUCCESS) {
        std::cerr << "Failed to initialize runtime: " << cmw_error_string(result) << std::endl;
        return 1;
    }

    std::cout << "✓ Runtime state initialized" << std::endl;
    std::cout << "  Cache directory: " << cache_dir << std::endl;

#ifdef _WIN32
    // Start named pipe server
    std::cout << "\n[2/3] Starting named pipe server..." << std::endl;
    std::cout << "  Pipe name: " << pipe_name << std::endl;

    NamedPipeServer server(pipe_name);
    g_server = &server;

    result = server.Start(handle_message);
    if (result != CMW_SUCCESS) {
        std::cerr << "Failed to start named pipe server: " << cmw_error_string(result) << std::endl;
        return 1;
    }

    std::cout << "✓ Named pipe server started" << std::endl;
    std::cout << "  Waiting for client connections..." << std::endl;
#else
    std::cerr << "ERROR: Named pipes only supported on Windows" << std::endl;
    return 1;
#endif

    // Enter service loop
    std::cout << "\n[3/3] Service ready!" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nUniversal ML Runtime is running" << std::endl;
    std::cout << "Press Ctrl+C to stop\n" << std::endl;

    std::cout << "Supported Model Formats:" << std::endl;
    std::cout << "  • PyTorch (.pt, .pth)" << std::endl;
    std::cout << "  • TensorFlow (SavedModel, .h5)" << std::endl;
    std::cout << "  • CoreML (.mlmodel, .mlpackage)" << std::endl;
    std::cout << "  • ONNX (.onnx)" << std::endl;
    std::cout << "\nFeatures:" << std::endl;
    std::cout << "  • Automatic conversion to ONNX" << std::endl;
    std::cout << "  • Multi-provider benchmarking" << std::endl;
    std::cout << "  • Intelligent provider selection" << std::endl;
    std::cout << "  • Hardware acceleration (DirectML, OpenVINO)" << std::endl;

    // Main loop - just wait for shutdown signal
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Cleanup
    std::cout << "\n========================================" << std::endl;
    std::cout << "Shutting down..." << std::endl;

#ifdef _WIN32
    server.Stop();
    g_server = nullptr;
#endif

    runtime_state.Shutdown();
    g_runtime_state = nullptr;

    std::cout << "✓ Shutdown complete" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
