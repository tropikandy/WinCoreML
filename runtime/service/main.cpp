/**
 * Universal ML Runtime Service
 *
 * Main entry point for the runtime service.
 * Integrates: Named Pipe Server + Runtime State + Model Registry + ONNX Executor
 */

#include "../include/coremlwin_errors.h"
#include "../include/logger.h"
#include "runtime_state.h"

#ifdef _WIN32
#include "named_pipe_server_win32.h"
#endif

// Protobuf generated headers
#include "coremlwin_runtime.pb.h"

#include <iostream>
#include <cstring>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

using namespace coremlwin;

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
    LOG_INFO << "Shutdown signal received (" << signal << ")";
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
 * Helper: Convert TensorData to protobuf Tensor
 */
void TensorDataToProto(const std::string& name, const TensorData& tensor, coremlwin::Tensor* proto_tensor) {
    proto_tensor->set_name(name);

    // Map dtype to protobuf enum
    switch (tensor.dtype) {
        case DTYPE_FLOAT32: proto_tensor->set_dtype(coremlwin::DTYPE_FLOAT32); break;
        case DTYPE_FLOAT16: proto_tensor->set_dtype(coremlwin::DTYPE_FLOAT16); break;
        case DTYPE_INT32: proto_tensor->set_dtype(coremlwin::DTYPE_INT32); break;
        case DTYPE_INT64: proto_tensor->set_dtype(coremlwin::DTYPE_INT64); break;
        case DTYPE_INT8: proto_tensor->set_dtype(coremlwin::DTYPE_INT8); break;
        case DTYPE_UINT8: proto_tensor->set_dtype(coremlwin::DTYPE_UINT8); break;
        default: proto_tensor->set_dtype(coremlwin::DTYPE_FLOAT32);
    }

    // Copy shape
    for (int64_t dim : tensor.shape) {
        proto_tensor->add_shape(dim);
    }

    // Copy data
    proto_tensor->set_data(tensor.data.data(), tensor.data.size());
}

/**
 * Helper: Convert protobuf Tensor to TensorData
 */
void ProtoToTensorData(const coremlwin::Tensor& proto_tensor, TensorData& tensor) {
    // Map protobuf dtype to internal enum
    switch (proto_tensor.dtype()) {
        case coremlwin::DTYPE_FLOAT32: tensor.dtype = DTYPE_FLOAT32; break;
        case coremlwin::DTYPE_FLOAT16: tensor.dtype = DTYPE_FLOAT16; break;
        case coremlwin::DTYPE_INT32: tensor.dtype = DTYPE_INT32; break;
        case coremlwin::DTYPE_INT64: tensor.dtype = DTYPE_INT64; break;
        case coremlwin::DTYPE_INT8: tensor.dtype = DTYPE_INT8; break;
        case coremlwin::DTYPE_UINT8: tensor.dtype = DTYPE_UINT8; break;
        default: tensor.dtype = DTYPE_FLOAT32;
    }

    // Copy shape
    tensor.shape.clear();
    for (int i = 0; i < proto_tensor.shape_size(); i++) {
        tensor.shape.push_back(proto_tensor.shape(i));
    }

    // Copy data
    const std::string& data_str = proto_tensor.data();
    tensor.data.assign(data_str.begin(), data_str.end());
}

/**
 * Message handler for IPC requests
 * Deserializes protobuf, dispatches to RuntimeState, serializes response
 */
std::vector<uint8_t> handle_message(const std::vector<uint8_t>& request_data) {
    coremlwin::PipeResponseEnvelope response_envelope;

    try {
        // Parse incoming request
        coremlwin::PipeRequestEnvelope request_envelope;
        if (!request_envelope.ParseFromArray(request_data.data(), request_data.size())) {
            LOG_ERROR << "Failed to parse protobuf request (" << request_data.size() << " bytes)";

            // Return error response
            response_envelope.mutable_status()->set_code(CMW_ERROR_INVALID_ARGUMENT);
            response_envelope.mutable_status()->set_message("Failed to parse request");

            std::string serialized;
            response_envelope.SerializeToString(&serialized);
            return std::vector<uint8_t>(serialized.begin(), serialized.end());
        }

        // Echo request ID
        response_envelope.set_request_id(request_envelope.request_id());

        // Dispatch based on request type
        if (request_envelope.has_health_check()) {
            LOG_DEBUG << "Handling HealthCheck request";

            auto* health_response = response_envelope.mutable_health_check();
            health_response->set_version("0.1.0");
            health_response->set_ready(g_runtime_state && g_runtime_state->IsInitialized());

            response_envelope.mutable_status()->set_code(CMW_SUCCESS);

        } else if (request_envelope.has_register_model()) {
            LOG_INFO << "Handling RegisterModel request";

            const auto& req = request_envelope.register_model();

            RegisterModelRequest internal_req;
            internal_req.model_path = req.model_path();
            internal_req.cache_key = req.cache_key();

            RegisterModelResponse internal_resp;
            CmwErrorCode result = g_runtime_state->RegisterModel(internal_req, internal_resp);

            if (result == CMW_SUCCESS) {
                auto* reg_response = response_envelope.mutable_register_model();
                reg_response->set_model_id(internal_resp.model_id);

                auto* metadata = reg_response->mutable_metadata();
                metadata->set_model_id(internal_resp.metadata.model_id);
                metadata->set_model_format(internal_resp.metadata.model_format);

                for (const auto& name : internal_resp.metadata.input_names) {
                    metadata->add_input_names(name);
                }
                for (const auto& name : internal_resp.metadata.output_names) {
                    metadata->add_output_names(name);
                }

                // Add benchmark results
                for (const auto& bench : internal_resp.metadata.benchmarks) {
                    auto* bench_result = metadata->add_benchmarks();
                    bench_result->set_provider_name(bench.provider_name);
                    bench_result->set_success(bench.success);
                    bench_result->set_mean_latency_ms(bench.mean_latency_ms);
                    bench_result->set_std_latency_ms(bench.std_latency_ms);
                    bench_result->set_speedup_vs_cpu(bench.speedup_vs_cpu);
                    if (!bench.error_message.empty()) {
                        bench_result->set_error_message(bench.error_message);
                    }
                }

                if (!internal_resp.metadata.fastest_provider.empty()) {
                    metadata->set_fastest_provider(internal_resp.metadata.fastest_provider);
                }
                if (internal_resp.metadata.best_latency_ms > 0) {
                    metadata->set_best_latency_ms(internal_resp.metadata.best_latency_ms);
                }

                response_envelope.mutable_status()->set_code(CMW_SUCCESS);
            } else {
                response_envelope.mutable_status()->set_code(result);
                response_envelope.mutable_status()->set_message(internal_resp.error_message);
            }

        } else if (request_envelope.has_predict()) {
            LOG_DEBUG << "Handling Predict request";

            const auto& req = request_envelope.predict();

            PredictRequest internal_req;
            internal_req.model_id = req.model_id();

            // Convert input tensors
            for (int i = 0; i < req.inputs_size(); i++) {
                TensorData tensor_data;
                ProtoToTensorData(req.inputs(i), tensor_data);
                internal_req.inputs[req.inputs(i).name()] = tensor_data;
            }

            // Config
            if (req.has_config()) {
                internal_req.compute_units = "ALL";  // TODO: Map enum
                internal_req.timeout_ms = req.config().timeout_ms();
            }

            PredictResponse internal_resp;
            CmwErrorCode result = g_runtime_state->Predict(internal_req, internal_resp);

            if (result == CMW_SUCCESS) {
                auto* pred_response = response_envelope.mutable_predict();

                // Convert output tensors
                for (const auto& [name, tensor] : internal_resp.outputs) {
                    auto* proto_tensor = pred_response->add_outputs();
                    TensorDataToProto(name, tensor, proto_tensor);
                }

                // Debug info
                auto* debug_info = pred_response->mutable_debug_info();
                debug_info->set_provider_used(internal_resp.provider_used);
                debug_info->set_inference_time_us(internal_resp.inference_time_us);

                response_envelope.mutable_status()->set_code(CMW_SUCCESS);
            } else {
                response_envelope.mutable_status()->set_code(result);
                response_envelope.mutable_status()->set_message(internal_resp.error_message);
            }

        } else if (request_envelope.has_list_models()) {
            std::cout << "Handling ListModels request" << std::endl;

            auto models = g_runtime_state->ListModels();

            auto* list_response = response_envelope.mutable_list_models();
            for (const auto& model : models) {
                auto* metadata = list_response->add_models();
                metadata->set_model_id(model.model_id);
                metadata->set_model_format(model.model_format);

                for (const auto& name : model.input_names) {
                    metadata->add_input_names(name);
                }
                for (const auto& name : model.output_names) {
                    metadata->add_output_names(name);
                }
            }

            response_envelope.mutable_status()->set_code(CMW_SUCCESS);

        } else if (request_envelope.has_unregister_model()) {
            std::cout << "Handling UnregisterModel request" << std::endl;

            const auto& req = request_envelope.unregister_model();
            CmwErrorCode result = g_runtime_state->UnregisterModel(req.model_id());

            auto* unreg_response = response_envelope.mutable_unregister_model();
            unreg_response->set_success(result == CMW_SUCCESS);

            response_envelope.mutable_status()->set_code(result);

        } else {
            std::cerr << "Unknown request type" << std::endl;
            response_envelope.mutable_status()->set_code(CMW_ERROR_INVALID_ARGUMENT);
            response_envelope.mutable_status()->set_message("Unknown request type");
        }

    } catch (const std::exception& e) {
        std::cerr << "Exception handling request: " << e.what() << std::endl;
        response_envelope.mutable_status()->set_code(CMW_ERROR_INTERNAL);
        response_envelope.mutable_status()->set_message(std::string("Internal error: ") + e.what());
    }

    // Serialize response
    std::string serialized;
    if (!response_envelope.SerializeToString(&serialized)) {
        std::cerr << "Failed to serialize response" << std::endl;

        // Create minimal error response
        coremlwin::PipeResponseEnvelope error_envelope;
        error_envelope.mutable_status()->set_code(CMW_ERROR_INTERNAL);
        error_envelope.mutable_status()->set_message("Failed to serialize response");
        error_envelope.SerializeToString(&serialized);
    }

    return std::vector<uint8_t>(serialized.begin(), serialized.end());
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

    // Initialize logger
    LogLevel log_level = dev_mode ? LogLevel::DEBUG : LogLevel::INFO;
    Logger::GetInstance().Initialize(
        log_level,
        true,   // log to file
        cache_dir + "/coremlwin_runtime.log",
        true    // log to console
    );

    // Print banner
    std::cout << "========================================" << std::endl;
    print_version();
    std::cout << "========================================" << std::endl;

    if (dev_mode) {
        std::cout << "Running in DEVELOPMENT mode (DEBUG logging)" << std::endl;
    }

    LOG_INFO << "Universal ML Runtime Service starting";
    LOG_INFO << "Cache directory: " << cache_dir;
    LOG_INFO << "Pipe name: " << pipe_name;

    // Install signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Initialize runtime state
    LOG_INFO << "Initializing runtime state...";
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
