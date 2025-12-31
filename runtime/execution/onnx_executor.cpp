/**
 * ONNX Runtime Executor Implementation
 *
 * Supports both real ONNX Runtime (when CMW_HAVE_ONNXRUNTIME is defined)
 * and placeholder implementation for testing without dependencies.
 */

#include "onnx_executor.h"
#include "logger.h"
#include "security_utils.h"
#include <sstream>

#ifdef CMW_HAVE_ONNXRUNTIME
    #include <onnxruntime_cxx_api.h>
    #define USING_REAL_ONNXRUNTIME 1
#else
    #define USING_REAL_ONNXRUNTIME 0
#endif

struct ONNXExecutor::Impl {
    std::string model_path;
    std::string provider_name;
    bool loaded;

    std::vector<std::string> input_names;
    std::vector<std::string> output_names;
    std::map<std::string, std::vector<int64_t>> input_shapes;
    std::map<std::string, std::vector<int64_t>> output_shapes;

#if USING_REAL_ONNXRUNTIME
    std::unique_ptr<Ort::Env> env;
    std::unique_ptr<Ort::Session> session;
    std::unique_ptr<Ort::SessionOptions> session_options;
    Ort::AllocatorWithDefaultOptions allocator;
#endif

    Impl() : loaded(false) {}
};

ONNXExecutor::ONNXExecutor()
    : impl_(std::make_unique<Impl>())
{}

ONNXExecutor::~ONNXExecutor() = default;

CmwErrorCode ONNXExecutor::LoadModel(
    const std::string& model_path,
    const std::string& provider_name
) {
    using namespace coremlwin;
    using namespace coremlwin::security;

    LOG_INFO << "Loading ONNX model: " << SanitizeForLog(model_path)
             << " with provider: " << SanitizeForLog(provider_name);

    // SECURITY: Validate model path to prevent path traversal attacks
    std::filesystem::path canonical_path;
    if (!ValidateModelPath(model_path, canonical_path)) {
        LOG_ERROR << "Invalid or unsafe model path: " << SanitizeForLog(model_path);
        return CMW_ERROR_INVALID_ARGUMENT;
    }

    LOG_DEBUG << "Validated model path: " << canonical_path.string();

    impl_->model_path = canonical_path.string();
    impl_->provider_name = provider_name;

#if USING_REAL_ONNXRUNTIME
    try {
        // Create environment
        impl_->env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "CoreMLWin");
        LOG_DEBUG << "Created ONNX Runtime environment";

        // Create session options
        impl_->session_options = std::make_unique<Ort::SessionOptions>();
        impl_->session_options->SetIntraOpNumThreads(4);
        impl_->session_options->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

        // Set execution provider
        if (provider_name == "DmlExecutionProvider") {
            LOG_INFO << "Configuring DirectML execution provider";
            try {
                Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_DML(*impl_->session_options, 0));
                LOG_INFO << "DirectML provider enabled";
            } catch (const Ort::Exception& e) {
                LOG_WARNING << "Failed to enable DirectML, falling back to CPU: " << e.what();
            }
        } else if (provider_name == "CPUExecutionProvider") {
            LOG_DEBUG << "Using CPU execution provider (default)";
        } else {
            LOG_WARNING << "Unknown provider: " << provider_name << ", using CPU";
        }

        // Create session
        LOG_DEBUG << "Creating ONNX Runtime session";

        // Convert path to wide string on Windows
#ifdef _WIN32
        std::wstring wide_path(model_path.begin(), model_path.end());
        impl_->session = std::make_unique<Ort::Session>(*impl_->env, wide_path.c_str(), *impl_->session_options);
#else
        impl_->session = std::make_unique<Ort::Session>(*impl_->env, model_path.c_str(), *impl_->session_options);
#endif

        LOG_INFO << "ONNX Runtime session created successfully";

        // Get input/output metadata
        size_t num_inputs = impl_->session->GetInputCount();
        LOG_DEBUG << "Model has " << num_inputs << " inputs";

        for (size_t i = 0; i < num_inputs; i++) {
            auto name = impl_->session->GetInputNameAllocated(i, impl_->allocator);
            impl_->input_names.push_back(name.get());

            auto type_info = impl_->session->GetInputTypeInfo(i);
            auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
            impl_->input_shapes[impl_->input_names.back()] = tensor_info.GetShape();

            std::ostringstream shape_str;
            for (auto dim : impl_->input_shapes[impl_->input_names.back()]) {
                shape_str << dim << " ";
            }
            LOG_DEBUG << "  Input[" << i << "]: " << impl_->input_names.back()
                      << " shape=[" << shape_str.str() << "]";
        }

        size_t num_outputs = impl_->session->GetOutputCount();
        LOG_DEBUG << "Model has " << num_outputs << " outputs";

        for (size_t i = 0; i < num_outputs; i++) {
            auto name = impl_->session->GetOutputNameAllocated(i, impl_->allocator);
            impl_->output_names.push_back(name.get());

            auto type_info = impl_->session->GetOutputTypeInfo(i);
            auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
            impl_->output_shapes[impl_->output_names.back()] = tensor_info.GetShape();

            std::ostringstream shape_str;
            for (auto dim : impl_->output_shapes[impl_->output_names.back()]) {
                shape_str << dim << " ";
            }
            LOG_DEBUG << "  Output[" << i << "]: " << impl_->output_names.back()
                      << " shape=[" << shape_str.str() << "]";
        }

        impl_->loaded = true;
        LOG_INFO << "Model loaded successfully with " << num_inputs << " inputs and "
                 << num_outputs << " outputs";
        return CMW_SUCCESS;

    } catch (const Ort::Exception& e) {
        LOG_ERROR << "ONNX Runtime error: " << e.what();
        // SECURITY: Clean up resources on error
        impl_->session.reset();
        impl_->session_options.reset();
        impl_->env.reset();
        impl_->loaded = false;
        return CMW_ERROR_MODEL_LOAD_FAILED;
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to load model: " << e.what();
        // SECURITY: Clean up resources on error
        impl_->session.reset();
        impl_->session_options.reset();
        impl_->env.reset();
        impl_->loaded = false;
        return CMW_ERROR_MODEL_LOAD_FAILED;
    }

#else
    // PLACEHOLDER: Simulate successful load when ONNX Runtime is not available
    LOG_WARNING << "ONNX Runtime not available, using placeholder implementation";
    impl_->input_names = {"input"};
    impl_->output_names = {"output"};
    impl_->input_shapes["input"] = {1, 3, 224, 224};
    impl_->output_shapes["output"] = {1, 1000};
    impl_->loaded = true;

    LOG_INFO << "Model loaded (PLACEHOLDER mode)";
    return CMW_SUCCESS;
#endif
}

CmwErrorCode ONNXExecutor::RunInference(
    const std::map<std::string, TensorData>& inputs,
    std::map<std::string, TensorData>& outputs
) {
    using namespace coremlwin;

    if (!impl_->loaded) {
        LOG_ERROR << "Cannot run inference: model not loaded";
        return CMW_ERROR_MODEL_NOT_FOUND;
    }

    LOG_DEBUG << "Running inference with " << inputs.size() << " inputs";

#if USING_REAL_ONNXRUNTIME
    try {
        Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        // Prepare inputs
        std::vector<Ort::Value> input_tensors;
        std::vector<const char*> input_names_cstr;

        for (const auto& input_name : impl_->input_names) {
            auto it = inputs.find(input_name);
            if (it == inputs.end()) {
                LOG_ERROR << "Missing required input: " << input_name;
                return CMW_ERROR_MISSING_REQUIRED_INPUT;
            }

            const auto& tensor = it->second;
            LOG_DEBUG << "  Input '" << input_name << "': " << tensor.data.size() << " bytes";

            // SECURITY: Validate tensor shape to prevent integer overflow
            size_t num_elements;
            if (!ValidateTensorShape(tensor.shape, num_elements)) {
                LOG_ERROR << "Invalid tensor shape for input: " << input_name;
                return CMW_ERROR_INVALID_ARGUMENT;
            }

            // SECURITY: Validate tensor data size matches shape (prevent buffer overflow)
            // TODO: Support multiple dtypes, currently assuming float32
            if (!ValidateTensorDataSize(tensor.shape, tensor.data.size(), sizeof(float))) {
                LOG_ERROR << "Tensor data size mismatch for input: " << input_name
                         << " (expected " << (num_elements * sizeof(float))
                         << " bytes, got " << tensor.data.size() << " bytes)";
                return CMW_ERROR_INVALID_ARGUMENT;
            }

            auto tensor_value = Ort::Value::CreateTensor<float>(
                memory_info,
                reinterpret_cast<float*>(const_cast<uint8_t*>(tensor.data.data())),
                num_elements,  // Use validated count
                tensor.shape.data(),
                tensor.shape.size()
            );

            input_tensors.push_back(std::move(tensor_value));
            input_names_cstr.push_back(input_name.c_str());
        }

        // Prepare output names
        std::vector<const char*> output_names_cstr;
        for (const auto& name : impl_->output_names) {
            output_names_cstr.push_back(name.c_str());
        }

        // Run inference
        LOG_DEBUG << "Executing ONNX Runtime session";
        auto output_tensors = impl_->session->Run(
            Ort::RunOptions{nullptr},
            input_names_cstr.data(),
            input_tensors.data(),
            input_tensors.size(),
            output_names_cstr.data(),
            output_names_cstr.size()
        );

        LOG_DEBUG << "Inference complete, processing " << output_tensors.size() << " outputs";

        // Extract outputs
        for (size_t i = 0; i < output_tensors.size(); i++) {
            TensorData output_tensor;
            output_tensor.name = impl_->output_names[i];
            output_tensor.dtype = CMW_DTYPE_FLOAT32; // TODO: Get actual dtype

            auto& ort_tensor = output_tensors[i];
            auto tensor_info = ort_tensor.GetTensorTypeAndShapeInfo();
            output_tensor.shape = tensor_info.GetShape();

            float* data_ptr = ort_tensor.GetTensorMutableData<float>();
            size_t data_size = tensor_info.GetElementCount() * sizeof(float);

            output_tensor.data.resize(data_size);
            std::memcpy(output_tensor.data.data(), data_ptr, data_size);

            LOG_DEBUG << "  Output '" << output_tensor.name << "': " << data_size << " bytes";
            outputs[output_tensor.name] = std::move(output_tensor);
        }

        LOG_DEBUG << "Inference successful";
        return CMW_SUCCESS;

    } catch (const Ort::Exception& e) {
        LOG_ERROR << "ONNX Runtime inference error: " << e.what();
        return CMW_ERROR_PROVIDER_EXECUTION_FAILED;
    } catch (const std::exception& e) {
        LOG_ERROR << "Inference error: " << e.what();
        return CMW_ERROR_PROVIDER_EXECUTION_FAILED;
    }

#else
    // PLACEHOLDER: Return dummy output when ONNX Runtime is not available
    LOG_DEBUG << "Running inference in PLACEHOLDER mode";

    for (const auto& output_name : impl_->output_names) {
        TensorData output_tensor;
        output_tensor.name = output_name;
        output_tensor.dtype = CMW_DTYPE_FLOAT32;
        output_tensor.shape = impl_->output_shapes[output_name];

        // SECURITY: Calculate size with overflow protection (even in placeholder mode)
        size_t num_elements;
        if (!ValidateTensorShape(output_tensor.shape, num_elements)) {
            LOG_ERROR << "Invalid output shape in placeholder mode: " << output_name;
            return CMW_ERROR_PROVIDER_EXECUTION_FAILED;
        }

        // Check size before allocation
        if (num_elements > SIZE_MAX / sizeof(float)) {
            LOG_ERROR << "Output tensor too large in placeholder mode";
            return CMW_ERROR_PROVIDER_EXECUTION_FAILED;
        }

        output_tensor.data.resize(num_elements * sizeof(float));

        // Fill with dummy data
        float* data_ptr = reinterpret_cast<float*>(output_tensor.data.data());
        for (size_t i = 0; i < num_elements; i++) {
            data_ptr[i] = 0.001f * (i % 1000);
        }

        outputs[output_name] = std::move(output_tensor);
    }

    LOG_DEBUG << "Inference complete (PLACEHOLDER mode)";
    return CMW_SUCCESS;
#endif
}

std::vector<std::string> ONNXExecutor::GetInputNames() const {
    return impl_->input_names;
}

std::vector<std::string> ONNXExecutor::GetOutputNames() const {
    return impl_->output_names;
}

std::map<std::string, std::vector<int64_t>> ONNXExecutor::GetInputShapes() const {
    return impl_->input_shapes;
}

std::map<std::string, std::vector<int64_t>> ONNXExecutor::GetOutputShapes() const {
    return impl_->output_shapes;
}

bool ONNXExecutor::IsLoaded() const {
    return impl_->loaded;
}
