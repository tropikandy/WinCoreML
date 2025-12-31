/**
 * Runtime State Manager Implementation
 */

#include "runtime_state.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

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
        std::cerr << "Failed to create cache directory: " << e.what() << std::endl;
        return CMW_ERROR_CACHE_WRITE_FAILED;
    }

    initialized_ = true;
    std::cout << "Runtime initialized. Cache dir: " << cache_dir_ << std::endl;

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

    std::cout << "Registering model: " << request.model_path << std::endl;

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
        std::cout << "Model already registered: " << model_id << std::endl;
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

    std::cout << "Model registered successfully: " << model_id << std::endl;
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

    /*
     * FULL IMPLEMENTATION (requires Python subprocess):
     *
     * #ifdef _WIN32
     *     // Windows implementation
     *     STARTUPINFO si = {sizeof(si)};
     *     PROCESS_INFORMATION pi = {0};
     *
     *     // Build command: python -m tools.converter_worker.worker
     *     std::string command = "python -m tools.converter_worker.worker";
     *
     *     // Build JSON request
     *     std::string json_request = R"({
     *         "command": "convert",
     *         "model_path": ")" + model_path + R"(",
     *         "output_path": ")" + output_path + R"(",
     *         "benchmark": true
     *     })";
     *
     *     // Create pipes for stdin/stdout
     *     HANDLE stdin_read, stdin_write;
     *     HANDLE stdout_read, stdout_write;
     *
     *     SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};
     *
     *     CreatePipe(&stdin_read, &stdin_write, &sa, 0);
     *     CreatePipe(&stdout_read, &stdout_write, &sa, 0);
     *
     *     si.hStdInput = stdin_read;
     *     si.hStdOutput = stdout_write;
     *     si.hStdError = stdout_write;
     *     si.dwFlags |= STARTF_USESTDHANDLES;
     *
     *     // Create process
     *     if (!CreateProcessA(NULL, (LPSTR)command.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
     *         return CMW_ERROR_MODEL_CONVERSION_FAILED;
     *     }
     *
     *     // Write JSON request to stdin
     *     DWORD written;
     *     WriteFile(stdin_write, json_request.c_str(), json_request.length(), &written, NULL);
     *     CloseHandle(stdin_write);
     *
     *     // Wait for completion (with timeout)
     *     WaitForSingleObject(pi.hProcess, 60000);
     *
     *     // Read response from stdout
     *     char buffer[4096];
     *     DWORD read;
     *     std::string response;
     *     while (ReadFile(stdout_read, buffer, sizeof(buffer), &read, NULL) && read > 0) {
     *         response.append(buffer, read);
     *     }
     *
     *     CloseHandle(stdout_read);
     *     CloseHandle(pi.hProcess);
     *     CloseHandle(pi.hThread);
     *
     *     // Parse JSON response
     *     // ... (parse metadata and benchmark results)
     *
     *     return CMW_SUCCESS;
     * #endif
     */

    // PLACEHOLDER: Simulate conversion
    std::cout << "PLACEHOLDER: Simulating conversion (no actual converter worker)" << std::endl;

    // For demo purposes, just note that conversion would happen here
    metadata.model_format = "unknown";
    metadata.input_names = {"input"};
    metadata.output_names = {"output"};

    // Create placeholder ONNX file (empty)
    std::ofstream ofs(output_path, std::ios::binary);
    if (!ofs) {
        return CMW_ERROR_MODEL_CONVERSION_FAILED;
    }
    ofs.close();

    std::cout << "PLACEHOLDER: Conversion complete" << std::endl;
    return CMW_SUCCESS;
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
