/**
 * Runtime State Manager Implementation
 */

#include "runtime_state.h"
#include "../include/logger.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

using namespace coremlwin;

RuntimeState::RuntimeState()
    : initialized_(false)
{}

RuntimeState::~RuntimeState() {
    Shutdown();
}

CmwErrorCode RuntimeState::Initialize(const std::string& cache_dir) {
    if (initialized_) {
        return CMW_SUCCESS;
    }

    cache_dir_ = cache_dir;

    // Create cache directory if it doesn't exist
    try {
        fs::create_directories(cache_dir);
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to create cache directory: " << e.what() << std::endl;
        return CMW_ERROR_CACHE_WRITE_FAILED;
    }

    initialized_ = true;
    LOG_INFO << "Runtime initialized. Cache dir: " << cache_dir_ << std::endl;

    return CMW_SUCCESS;
}

void RuntimeState::Shutdown() {
    if (!initialized_) {
        return;
    }

    // Clear executors
    {
        std::lock_guard<std::mutex> lock(executors_mutex_);
        executors_.clear();
    }

    // Clear registry
    registry_.Clear();

    initialized_ = false;
    std::cout << "Runtime shutdown complete" << std::endl;
}

CmwErrorCode RuntimeState::RegisterModel(
    const RegisterModelRequest& request,
    RegisterModelResponse& response
) {
    if (!initialized_) {
        response.error_code = CMW_ERROR_SERVICE_UNAVAILABLE;
        response.error_message = "Runtime not initialized";
        return response.error_code;
    }

    LOG_INFO << "Registering model: " << request.model_path << std::endl;

    // Check if file exists
    if (!fs::exists(request.model_path)) {
        response.error_code = CMW_ERROR_MODEL_NOT_FOUND;
        response.error_message = "Model file not found: " + request.model_path;
        return response.error_code;
    }

    // Compute model ID (hash of contents)
    std::string model_id;
    if (!request.cache_key.empty()) {
        model_id = request.cache_key;
    } else {
        model_id = ComputeModelHash(request.model_path);
    }

    // Check if already registered
    if (registry_.HasModel(model_id)) {
        LOG_DEBUG << "Model already registered: " << model_id << std::endl;
        response.error_code = CMW_SUCCESS;
        response.model_id = model_id;
        registry_.GetModel(model_id, response.metadata);
        return CMW_SUCCESS;
    }

    // Determine output ONNX path
    std::string onnx_path = cache_dir_ + "/" + model_id + ".onnx";

    // Convert model to ONNX (if not already ONNX)
    ModelMetadata metadata;
    CmwErrorCode result = RunConverterWorker(
        request.model_path,
        onnx_path,
        metadata
    );

    if (result != CMW_SUCCESS) {
        response.error_code = result;
        response.error_message = "Model conversion failed";
        return result;
    }

    // Fill in metadata
    metadata.model_id = model_id;
    metadata.original_path = request.model_path;
    metadata.onnx_path = onnx_path;

    // Register in registry
    result = registry_.RegisterModel(metadata);
    if (result != CMW_SUCCESS) {
        response.error_code = result;
        response.error_message = "Failed to register in registry";
        return result;
    }

    // Success
    response.error_code = CMW_SUCCESS;
    response.model_id = model_id;
    response.metadata = metadata;

    LOG_INFO << "Model registered successfully: " << model_id << std::endl;
    return CMW_SUCCESS;
}

CmwErrorCode RuntimeState::UnregisterModel(const std::string& model_id) {
    if (!initialized_) {
        return CMW_ERROR_SERVICE_UNAVAILABLE;
    }

    // Remove executor from cache
    {
        std::lock_guard<std::mutex> lock(executors_mutex_);
        executors_.erase(model_id);
    }

    // Remove from registry
    return registry_.UnregisterModel(model_id);
}

CmwErrorCode RuntimeState::Predict(
    const PredictRequest& request,
    PredictResponse& response
) {
    if (!initialized_) {
        response.error_code = CMW_ERROR_SERVICE_UNAVAILABLE;
        response.error_message = "Runtime not initialized";
        return response.error_code;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // Get or create executor
    std::shared_ptr<ONNXExecutor> executor;
    CmwErrorCode result = GetOrCreateExecutor(request.model_id, executor);

    if (result != CMW_SUCCESS) {
        response.error_code = result;
        response.error_message = "Failed to get executor";
        return result;
    }

    // Run inference
    result = executor->RunInference(request.inputs, response.outputs);

    if (result != CMW_SUCCESS) {
        response.error_code = result;
        response.error_message = "Inference failed";
        return result;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time
    );

    response.error_code = CMW_SUCCESS;
    response.provider_used = "CPUExecutionProvider";  // TODO: Get from executor
    response.inference_time_us = duration.count();

    std::cout << "Inference complete: " << duration.count() << " µs" << std::endl;
    return CMW_SUCCESS;
}

CmwErrorCode RuntimeState::GetModelMetadata(
    const std::string& model_id,
    ModelMetadata& metadata
) {
    return registry_.GetModel(model_id, metadata);
}

std::vector<ModelMetadata> RuntimeState::ListModels() {
    return registry_.GetAllModels();
}

CmwErrorCode RuntimeState::RunConverterWorker(
    const std::string& model_path,
    const std::string& output_path,
    ModelMetadata& metadata
) {
    std::cout << "Converting model: " << model_path << " → " << output_path << std::endl;

    // Check if model is already ONNX
    fs::path path(model_path);
    if (path.extension() == ".onnx") {
        std::cout << "Model is already ONNX, copying..." << std::endl;
        try {
            fs::copy_file(model_path, output_path, fs::copy_options::overwrite_existing);
            metadata.model_format = "onnx";
            metadata.input_names = {"input"};  // Will be extracted by executor
            metadata.output_names = {"output"};
            return CMW_SUCCESS;
        } catch (const std::exception& e) {
            std::cerr << "Failed to copy ONNX file: " << e.what() << std::endl;
            return CMW_ERROR_MODEL_LOAD_FAILED;
        }
    }

#ifdef _WIN32
    // Windows implementation using CreateProcess
    std::cout << "Launching converter worker subprocess..." << std::endl;

    // Build command: python -m tools.converter_worker.worker
    std::string python_cmd = "python -m tools.converter_worker.worker";

    // Build JSON request with minimal escaping
    std::ostringstream json_stream;
    json_stream << "{\n";
    json_stream << "  \"command\": \"convert\",\n";
    json_stream << "  \"model_path\": \"" << model_path << "\",\n";
    json_stream << "  \"output_path\": \"" << output_path << "\",\n";
    json_stream << "  \"benchmark\": true,\n";
    json_stream << "  \"opset_version\": 14\n";
    json_stream << "}\n";
    std::string json_request = json_stream.str();

    std::cout << "Converter request: " << json_request << std::endl;

    // Create pipes for stdin/stdout/stderr
    HANDLE stdin_read = NULL, stdin_write = NULL;
    HANDLE stdout_read = NULL, stdout_write = NULL;
    HANDLE stderr_read = NULL, stderr_write = NULL;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&stdin_read, &stdin_write, &sa, 0)) {
        std::cerr << "Failed to create stdin pipe" << std::endl;
        return CMW_ERROR_MODEL_CONVERSION_FAILED;
    }
    if (!CreatePipe(&stdout_read, &stdout_write, &sa, 0)) {
        std::cerr << "Failed to create stdout pipe" << std::endl;
        CloseHandle(stdin_read);
        CloseHandle(stdin_write);
        return CMW_ERROR_MODEL_CONVERSION_FAILED;
    }
    if (!CreatePipe(&stderr_read, &stderr_write, &sa, 0)) {
        std::cerr << "Failed to create stderr pipe" << std::endl;
        CloseHandle(stdin_read);
        CloseHandle(stdin_write);
        CloseHandle(stdout_read);
        CloseHandle(stdout_write);
        return CMW_ERROR_MODEL_CONVERSION_FAILED;
    }

    // Ensure child doesn't inherit write end of stdout/stderr
    SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stderr_read, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stdin_write, HANDLE_FLAG_INHERIT, 0);

    // Setup process startup info
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdInput = stdin_read;
    si.hStdOutput = stdout_write;
    si.hStdError = stderr_write;
    si.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    // Create modifiable command string
    std::vector<char> cmd_buffer(python_cmd.begin(), python_cmd.end());
    cmd_buffer.push_back('\0');

    // Create process
    if (!CreateProcessA(
        NULL,                   // Application name
        cmd_buffer.data(),      // Command line
        NULL,                   // Process security attributes
        NULL,                   // Thread security attributes
        TRUE,                   // Inherit handles
        CREATE_NO_WINDOW,       // Creation flags
        NULL,                   // Environment
        NULL,                   // Current directory
        &si,                    // Startup info
        &pi                     // Process information
    )) {
        DWORD error = GetLastError();
        std::cerr << "Failed to create converter process. Error code: " << error << std::endl;
        std::cerr << "Make sure Python is in PATH and converter_worker module is accessible" << std::endl;

        CloseHandle(stdin_read);
        CloseHandle(stdin_write);
        CloseHandle(stdout_read);
        CloseHandle(stdout_write);
        CloseHandle(stderr_read);
        CloseHandle(stderr_write);

        return CMW_ERROR_MODEL_CONVERSION_FAILED;
    }

    std::cout << "Converter process started (PID: " << pi.dwProcessId << ")" << std::endl;

    // Close unused pipe ends in parent
    CloseHandle(stdin_read);
    CloseHandle(stdout_write);
    CloseHandle(stderr_write);

    // Write JSON request to stdin
    DWORD bytes_written;
    if (!WriteFile(stdin_write, json_request.c_str(), json_request.length(), &bytes_written, NULL)) {
        std::cerr << "Failed to write to converter stdin" << std::endl;
        CloseHandle(stdin_write);
        CloseHandle(stdout_read);
        CloseHandle(stderr_read);
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return CMW_ERROR_MODEL_CONVERSION_FAILED;
    }

    CloseHandle(stdin_write);  // Signal EOF

    // Wait for process completion (60 second timeout for conversion)
    DWORD wait_result = WaitForSingleObject(pi.hProcess, 60000);

    if (wait_result == WAIT_TIMEOUT) {
        std::cerr << "Converter process timed out" << std::endl;
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(stdout_read);
        CloseHandle(stderr_read);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return CMW_ERROR_TIMEOUT;
    }

    // Check exit code
    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    // Read stdout
    char buffer[4096];
    DWORD bytes_read;
    std::string stdout_output;

    while (ReadFile(stdout_read, buffer, sizeof(buffer) - 1, &bytes_read, NULL) && bytes_read > 0) {
        buffer[bytes_read] = '\0';
        stdout_output.append(buffer, bytes_read);
    }

    // Read stderr
    std::string stderr_output;
    while (ReadFile(stderr_read, buffer, sizeof(buffer) - 1, &bytes_read, NULL) && bytes_read > 0) {
        buffer[bytes_read] = '\0';
        stderr_output.append(buffer, bytes_read);
    }

    CloseHandle(stdout_read);
    CloseHandle(stderr_read);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (!stderr_output.empty()) {
        std::cout << "Converter stderr: " << stderr_output << std::endl;
    }

    if (exit_code != 0) {
        std::cerr << "Converter process failed with exit code: " << exit_code << std::endl;
        std::cerr << "Output: " << stdout_output << std::endl;
        return CMW_ERROR_MODEL_CONVERSION_FAILED;
    }

    std::cout << "Converter output: " << stdout_output << std::endl;

    // Parse JSON response
    // NOTE: Using basic string parsing. For production, use nlohmann/json library.
    // To integrate: Download https://github.com/nlohmann/json/single_include/nlohmann/json.hpp
    //               and include in project.

    if (!fs::exists(output_path)) {
        std::cerr << "Converter did not produce output file" << std::endl;
        return CMW_ERROR_MODEL_CONVERSION_FAILED;
    }

    // Check for success
    if (stdout_output.find("\"success\": true") == std::string::npos &&
        stdout_output.find("\"success\":true") == std::string::npos) {
        std::cerr << "Converter reported failure" << std::endl;
        return CMW_ERROR_MODEL_CONVERSION_FAILED;
    }

    // Extract model format
    size_t format_pos = stdout_output.find("\"format\":");
    if (format_pos != std::string::npos) {
        size_t quote1 = stdout_output.find("\"", format_pos + 10);
        size_t quote2 = stdout_output.find("\"", quote1 + 1);
        if (quote1 != std::string::npos && quote2 != std::string::npos) {
            metadata.model_format = stdout_output.substr(quote1 + 1, quote2 - quote1 - 1);
        }
    }

    // Extract input names
    size_t input_names_pos = stdout_output.find("\"input_names\":");
    if (input_names_pos != std::string::npos) {
        size_t array_start = stdout_output.find("[", input_names_pos);
        size_t array_end = stdout_output.find("]", array_start);
        if (array_start != std::string::npos && array_end != std::string::npos) {
            std::string array_content = stdout_output.substr(array_start + 1, array_end - array_start - 1);
            // Simple parsing: split by quotes
            metadata.input_names.clear();
            size_t pos = 0;
            while ((pos = array_content.find("\"", pos)) != std::string::npos) {
                size_t end = array_content.find("\"", pos + 1);
                if (end != std::string::npos) {
                    std::string name = array_content.substr(pos + 1, end - pos - 1);
                    if (!name.empty()) {
                        metadata.input_names.push_back(name);
                    }
                    pos = end + 1;
                } else {
                    break;
                }
            }
        }
    }

    // Extract output names (same logic as input names)
    size_t output_names_pos = stdout_output.find("\"output_names\":");
    if (output_names_pos != std::string::npos) {
        size_t array_start = stdout_output.find("[", output_names_pos);
        size_t array_end = stdout_output.find("]", array_start);
        if (array_start != std::string::npos && array_end != std::string::npos) {
            std::string array_content = stdout_output.substr(array_start + 1, array_end - array_start - 1);
            metadata.output_names.clear();
            size_t pos = 0;
            while ((pos = array_content.find("\"", pos)) != std::string::npos) {
                size_t end = array_content.find("\"", pos + 1);
                if (end != std::string::npos) {
                    std::string name = array_content.substr(pos + 1, end - pos - 1);
                    if (!name.empty()) {
                        metadata.output_names.push_back(name);
                    }
                    pos = end + 1;
                } else {
                    break;
                }
            }
        }
    }

    // Extract benchmark results
    size_t benchmark_pos = stdout_output.find("\"benchmark\":");
    if (benchmark_pos != std::string::npos) {
        // Extract fastest_provider
        size_t fastest_pos = stdout_output.find("\"fastest_provider\":", benchmark_pos);
        if (fastest_pos != std::string::npos) {
            size_t quote1 = stdout_output.find("\"", fastest_pos + 19);
            size_t quote2 = stdout_output.find("\"", quote1 + 1);
            if (quote1 != std::string::npos && quote2 != std::string::npos) {
                metadata.fastest_provider = stdout_output.substr(quote1 + 1, quote2 - quote1 - 1);
            }
        }

        // Extract best_latency_ms
        size_t best_latency_pos = stdout_output.find("\"best_latency_ms\":", benchmark_pos);
        if (best_latency_pos != std::string::npos) {
            size_t num_start = best_latency_pos + 18;
            while (num_start < stdout_output.length() &&
                   (stdout_output[num_start] == ' ' || stdout_output[num_start] == ':')) {
                num_start++;
            }
            size_t num_end = num_start;
            while (num_end < stdout_output.length() &&
                   (isdigit(stdout_output[num_end]) || stdout_output[num_end] == '.')) {
                num_end++;
            }
            if (num_end > num_start) {
                std::string latency_str = stdout_output.substr(num_start, num_end - num_start);
                try {
                    metadata.best_latency_ms = std::stof(latency_str);
                } catch (...) {}
            }
        }

        // Extract speedup_vs_cpu
        float speedup_vs_cpu = 1.0f;
        size_t speedup_pos = stdout_output.find("\"speedup_vs_cpu\":", benchmark_pos);
        if (speedup_pos != std::string::npos) {
            size_t num_start = speedup_pos + 17;
            while (num_start < stdout_output.length() &&
                   (stdout_output[num_start] == ' ' || stdout_output[num_start] == ':')) {
                num_start++;
            }
            size_t num_end = num_start;
            while (num_end < stdout_output.length() &&
                   (isdigit(stdout_output[num_end]) || stdout_output[num_end] == '.')) {
                num_end++;
            }
            if (num_end > num_start) {
                std::string speedup_str = stdout_output.substr(num_start, num_end - num_start);
                try {
                    speedup_vs_cpu = std::stof(speedup_str);
                } catch (...) {}
            }
        }

        // Extract results array (simplified - just get provider names and latencies)
        size_t results_pos = stdout_output.find("\"results\":", benchmark_pos);
        if (results_pos != std::string::npos) {
            size_t array_start = stdout_output.find("[", results_pos);
            size_t array_end = stdout_output.find("]", array_start);

            if (array_start != std::string::npos && array_end != std::string::npos) {
                std::string results_content = stdout_output.substr(array_start + 1, array_end - array_start - 1);

                // Parse each result object (very basic parsing)
                size_t obj_pos = 0;
                while ((obj_pos = results_content.find("{", obj_pos)) != std::string::npos) {
                    size_t obj_end = results_content.find("}", obj_pos);
                    if (obj_end == std::string::npos) break;

                    std::string obj_content = results_content.substr(obj_pos, obj_end - obj_pos + 1);

                    ProviderBenchmark bench;
                    bench.success = true;
                    bench.mean_latency_ms = 0.0f;
                    bench.std_latency_ms = 0.0f;
                    bench.speedup_vs_cpu = 1.0f;

                    // Extract provider name
                    size_t prov_pos = obj_content.find("\"provider\":");
                    if (prov_pos != std::string::npos) {
                        size_t q1 = obj_content.find("\"", prov_pos + 11);
                        size_t q2 = obj_content.find("\"", q1 + 1);
                        if (q1 != std::string::npos && q2 != std::string::npos) {
                            bench.provider_name = obj_content.substr(q1 + 1, q2 - q1 - 1);
                        }
                    }

                    // Extract mean_latency_ms
                    size_t lat_pos = obj_content.find("\"mean_latency_ms\":");
                    if (lat_pos != std::string::npos) {
                        size_t n_start = lat_pos + 18;
                        while (n_start < obj_content.length() &&
                               (obj_content[n_start] == ' ' || obj_content[n_start] == ':')) {
                            n_start++;
                        }
                        size_t n_end = n_start;
                        while (n_end < obj_content.length() &&
                               (isdigit(obj_content[n_end]) || obj_content[n_end] == '.')) {
                            n_end++;
                        }
                        if (n_end > n_start) {
                            try {
                                bench.mean_latency_ms = std::stof(obj_content.substr(n_start, n_end - n_start));
                            } catch (...) {}
                        }
                    }

                    // Extract success flag
                    size_t succ_pos = obj_content.find("\"success\":");
                    if (succ_pos != std::string::npos) {
                        bench.success = obj_content.find("true", succ_pos) != std::string::npos;
                    }

                    if (!bench.provider_name.empty()) {
                        metadata.benchmarks.push_back(bench);
                    }

                    obj_pos = obj_end + 1;
                }
            }
        }

        std::cout << "Benchmark data extracted: fastest=" << metadata.fastest_provider
                  << ", latency=" << metadata.best_latency_ms << "ms, "
                  << "speedup=" << speedup_vs_cpu << "x" << std::endl;
    }

    // Set defaults if not extracted
    if (metadata.model_format.empty()) {
        metadata.model_format = "unknown";
    }
    if (metadata.input_names.empty()) {
        metadata.input_names = {"input"};
    }
    if (metadata.output_names.empty()) {
        metadata.output_names = {"output"};
    }

    std::cout << "Conversion successful: " << metadata.model_format << " → ONNX" << std::endl;
    return CMW_SUCCESS;
#else
    // Non-Windows platforms - not supported yet
    std::cerr << "Converter worker only supported on Windows" << std::endl;
    return CMW_ERROR_UNSUPPORTED_PLATFORM;
#endif
}

std::string RuntimeState::ComputeModelHash(const std::string& model_path) {
    // Simple hash based on path and timestamp for now
    // In production, would hash file contents
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::hash<std::string> hasher;
    size_t hash = hasher(model_path) ^ hasher(std::to_string(now));

    std::stringstream ss;
    ss << std::hex << hash;
    return ss.str();
}

CmwErrorCode RuntimeState::GetOrCreateExecutor(
    const std::string& model_id,
    std::shared_ptr<ONNXExecutor>& executor
) {
    // Check cache first
    {
        std::lock_guard<std::mutex> lock(executors_mutex_);
        auto it = executors_.find(model_id);
        if (it != executors_.end()) {
            executor = it->second;
            return CMW_SUCCESS;
        }
    }

    // Get model metadata
    ModelMetadata metadata;
    CmwErrorCode result = registry_.GetModel(model_id, metadata);
    if (result != CMW_SUCCESS) {
        return result;
    }

    // Create new executor
    auto new_executor = std::make_shared<ONNXExecutor>();
    result = new_executor->LoadModel(metadata.onnx_path, "CPUExecutionProvider");

    if (result != CMW_SUCCESS) {
        return result;
    }

    // Cache executor
    {
        std::lock_guard<std::mutex> lock(executors_mutex_);
        executors_[model_id] = new_executor;
    }

    executor = new_executor;
    return CMW_SUCCESS;
}
