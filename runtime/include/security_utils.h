/**
 * Security Utilities for CoreMLWin Runtime
 *
 * Provides security-critical validation functions to prevent:
 * - Path traversal attacks
 * - Buffer overflows
 * - Integer overflows
 * - Log injection
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <random>

namespace coremlwin {
namespace security {

// Maximum limits to prevent DoS
constexpr size_t MAX_MESSAGE_SIZE = 100 * 1024 * 1024;      // 100 MB
constexpr size_t MAX_TENSOR_ELEMENTS = 1024 * 1024 * 1024;  // 1B elements
constexpr size_t MAX_TENSOR_SIZE_BYTES = 4ULL * 1024 * 1024 * 1024;  // 4 GB
constexpr size_t MAX_MODEL_SIZE = 2ULL * 1024 * 1024 * 1024;  // 2 GB
constexpr int MAX_SHAPE_DIMENSIONS = 8;  // Typical max is 4-5

/**
 * Validates tensor shape and calculates total elements with overflow protection.
 *
 * @param shape Tensor dimensions
 * @param num_elements Output: total number of elements
 * @return true if valid, false if overflow or exceeds limits
 */
inline bool ValidateTensorShape(
    const std::vector<int64_t>& shape,
    size_t& num_elements
) {
    if (shape.empty()) {
        return false;
    }

    if (shape.size() > MAX_SHAPE_DIMENSIONS) {
        return false;
    }

    num_elements = 1;

    for (auto dim : shape) {
        // Check for negative or zero dimensions
        if (dim <= 0) {
            return false;
        }

        // Check for overflow before multiplication
        if (num_elements > SIZE_MAX / static_cast<size_t>(dim)) {
            return false;  // Would overflow
        }

        num_elements *= static_cast<size_t>(dim);

        // Check against element limit
        if (num_elements > MAX_TENSOR_ELEMENTS) {
            return false;
        }
    }

    return true;
}

/**
 * Validates tensor data size matches shape with dtype.
 *
 * @param shape Tensor dimensions
 * @param data_size Size of data buffer in bytes
 * @param dtype_size Size of data type in bytes (e.g., sizeof(float))
 * @return true if sizes match exactly
 */
inline bool ValidateTensorDataSize(
    const std::vector<int64_t>& shape,
    size_t data_size,
    size_t dtype_size
) {
    size_t num_elements;
    if (!ValidateTensorShape(shape, num_elements)) {
        return false;
    }

    // Check for overflow in byte size calculation
    if (num_elements > SIZE_MAX / dtype_size) {
        return false;
    }

    size_t expected_size = num_elements * dtype_size;

    // Check against byte limit
    if (expected_size > MAX_TENSOR_SIZE_BYTES) {
        return false;
    }

    return data_size == expected_size;
}

/**
 * Validates and canonicalizes a model file path.
 *
 * Prevents path traversal attacks by:
 * - Resolving to canonical absolute path
 * - Checking file exists and is regular file
 * - Validating .onnx extension
 * - Optionally checking it's within allowed directory
 *
 * @param model_path Input path (may be relative)
 * @param canonical_path Output canonical path
 * @param allowed_base Optional base directory to restrict to
 * @return true if valid and safe
 */
inline bool ValidateModelPath(
    const std::string& model_path,
    std::filesystem::path& canonical_path,
    const std::string& allowed_base = ""
) {
    try {
        // Check for null bytes (can truncate string in C APIs)
        if (model_path.find('\0') != std::string::npos) {
            return false;
        }

        // Check for excessively long paths
        if (model_path.size() > 4096) {
            return false;
        }

        std::filesystem::path path(model_path);

        // Check if file exists
        if (!std::filesystem::exists(path)) {
            return false;
        }

        // Resolve to canonical path (resolves .., symlinks, etc.)
        canonical_path = std::filesystem::canonical(path);

        // Must be a regular file
        if (!std::filesystem::is_regular_file(canonical_path)) {
            return false;
        }

        // Check file size
        auto file_size = std::filesystem::file_size(canonical_path);
        if (file_size > MAX_MODEL_SIZE) {
            return false;
        }

        // Validate extension is .onnx
        if (canonical_path.extension() != ".onnx") {
            return false;
        }

        // If base directory specified, ensure file is within it
        if (!allowed_base.empty()) {
            std::filesystem::path base_canonical = std::filesystem::canonical(allowed_base);
            auto relative = canonical_path.lexically_relative(base_canonical);

            // If relative path starts with "..", it's outside base
            std::string rel_str = relative.string();
            if (relative.empty() || rel_str.rfind("..", 0) == 0) {  // C++17 compatible
                return false;
            }
        }

        return true;

    } catch (const std::filesystem::filesystem_error&) {
        return false;
    } catch (...) {
        return false;
    }
}

/**
 * Sanitizes a string for safe logging.
 *
 * Removes:
 * - Control characters (ANSI escape codes)
 * - Non-printable ASCII
 * - Null bytes
 *
 * @param input Untrusted user input
 * @return Sanitized string safe for logging
 */
inline std::string SanitizeForLog(const std::string& input) {
    std::string sanitized;
    sanitized.reserve(input.size());

    for (char c : input) {
        // Only allow printable ASCII (space to ~)
        if (c >= 32 && c <= 126) {
            sanitized += c;
        } else {
            sanitized += '?';
        }
    }

    // Truncate very long strings
    if (sanitized.size() > 256) {
        sanitized = sanitized.substr(0, 253) + "...";
    }

    return sanitized;
}

/**
 * Validates protobuf message size.
 *
 * @param size Message size in bytes
 * @return true if within acceptable limits
 */
inline bool ValidateMessageSize(size_t size) {
    return size > 0 && size <= MAX_MESSAGE_SIZE;
}

/**
 * Generates a cryptographically secure random model ID.
 *
 * @return 32-character hex string (128 bits of entropy)
 */
inline std::string GenerateSecureModelID() {
    // Use random_device for cryptographic randomness
    std::random_device rd;

    // Generate 16 random bytes (128 bits)
    uint8_t bytes[16];
    for (int i = 0; i < 16; i++) {
        bytes[i] = static_cast<uint8_t>(rd());
    }

    // Convert to hex string
    const char* hex_chars = "0123456789abcdef";
    std::string id;
    id.reserve(32);

    for (int i = 0; i < 16; i++) {
        id += hex_chars[(bytes[i] >> 4) & 0xf];
        id += hex_chars[bytes[i] & 0xf];
    }

    return id;
}

} // namespace security
} // namespace coremlwin
