/**
 * Unit Tests for Security Utilities
 *
 * Tests validation functions to ensure protection against:
 * - Integer overflow
 * - Buffer overflow
 * - Path traversal
 * - Log injection
 *
 * Compile with:
 *   g++ -std=c++17 test_security_utils.cpp -I../runtime/include -o test_security_utils
 *   ./test_security_utils
 */

#include "security_utils.h"
#include <iostream>
#include <cassert>
#include <fstream>

using namespace coremlwin::security;

// Test counter
int tests_run = 0;
int tests_passed = 0;

#define TEST(name) \
    void test_##name(); \
    struct TestRunner_##name { \
        TestRunner_##name() { \
            tests_run++; \
            std::cout << "Running test: " << #name << "..."; \
            try { \
                test_##name(); \
                tests_passed++; \
                std::cout << " PASSED\n"; \
            } catch (const std::exception& e) { \
                std::cout << " FAILED: " << e.what() << "\n"; \
            } catch (...) { \
                std::cout << " FAILED: Unknown exception\n"; \
            } \
        } \
    } test_runner_##name; \
    void test_##name()

#define ASSERT(condition) \
    if (!(condition)) { \
        throw std::runtime_error("Assertion failed: " #condition); \
    }

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        throw std::runtime_error("Assertion failed: " #a " == " #b); \
    }

// ============================================================================
// ValidateTensorShape Tests
// ============================================================================

TEST(valid_shape_simple) {
    std::vector<int64_t> shape = {1, 3, 224, 224};
    size_t num_elements;

    ASSERT(ValidateTensorShape(shape, num_elements));
    ASSERT_EQ(num_elements, 1 * 3 * 224 * 224);
}

TEST(valid_shape_1d) {
    std::vector<int64_t> shape = {1000};
    size_t num_elements;

    ASSERT(ValidateTensorShape(shape, num_elements));
    ASSERT_EQ(num_elements, 1000);
}

TEST(valid_shape_5d) {
    std::vector<int64_t> shape = {2, 3, 4, 5, 6};
    size_t num_elements;

    ASSERT(ValidateTensorShape(shape, num_elements));
    ASSERT_EQ(num_elements, 2 * 3 * 4 * 5 * 6);
}

TEST(invalid_shape_empty) {
    std::vector<int64_t> shape = {};
    size_t num_elements;

    ASSERT(!ValidateTensorShape(shape, num_elements));
}

TEST(invalid_shape_negative) {
    std::vector<int64_t> shape = {1, -3, 224, 224};
    size_t num_elements;

    ASSERT(!ValidateTensorShape(shape, num_elements));
}

TEST(invalid_shape_zero) {
    std::vector<int64_t> shape = {1, 0, 224, 224};
    size_t num_elements;

    ASSERT(!ValidateTensorShape(shape, num_elements));
}

TEST(invalid_shape_too_many_dims) {
    std::vector<int64_t> shape = {2, 2, 2, 2, 2, 2, 2, 2, 2};  // 9 dimensions
    size_t num_elements;

    ASSERT(!ValidateTensorShape(shape, num_elements));  // MAX is 8
}

TEST(invalid_shape_overflow) {
    // Shape that overflows size_t
    // 2^20 * 2^20 * 2^20 = 2^60 (overflows on 64-bit)
    std::vector<int64_t> shape = {1ULL << 20, 1ULL << 20, 1ULL << 20};
    size_t num_elements;

    ASSERT(!ValidateTensorShape(shape, num_elements));
}

TEST(invalid_shape_too_large) {
    // Exceeds MAX_TENSOR_ELEMENTS (1B)
    std::vector<int64_t> shape = {10000, 10000, 11};  // > 1B elements
    size_t num_elements;

    ASSERT(!ValidateTensorShape(shape, num_elements));
}

// ============================================================================
// ValidateTensorDataSize Tests
// ============================================================================

TEST(valid_data_size_match) {
    std::vector<int64_t> shape = {1, 3, 224, 224};
    size_t data_size = 1 * 3 * 224 * 224 * sizeof(float);

    ASSERT(ValidateTensorDataSize(shape, data_size, sizeof(float)));
}

TEST(invalid_data_size_too_small) {
    std::vector<int64_t> shape = {1, 3, 224, 224};
    size_t data_size = 1000;  // Way too small

    ASSERT(!ValidateTensorDataSize(shape, data_size, sizeof(float)));
}

TEST(invalid_data_size_too_large) {
    std::vector<int64_t> shape = {1, 3, 224, 224};
    size_t data_size = 1 * 3 * 224 * 224 * sizeof(float) * 2;  // 2x too large

    ASSERT(!ValidateTensorDataSize(shape, data_size, sizeof(float)));
}

TEST(invalid_data_size_overflow_in_bytes) {
    // Shape valid, but byte size would overflow
    std::vector<int64_t> shape = {1000000000};  // 1B elements
    size_t data_size = SIZE_MAX;  // Will overflow when checked

    ASSERT(!ValidateTensorDataSize(shape, data_size, sizeof(float)));
}

TEST(valid_data_size_int64) {
    std::vector<int64_t> shape = {100, 100};
    size_t data_size = 100 * 100 * sizeof(int64_t);

    ASSERT(ValidateTensorDataSize(shape, data_size, sizeof(int64_t)));
}

// ============================================================================
// ValidateModelPath Tests
// ============================================================================

TEST(valid_path_current_dir) {
    // Create temporary test file
    std::ofstream test_file("test_model.onnx");
    test_file << "dummy";
    test_file.close();

    std::filesystem::path canonical;
    ASSERT(ValidateModelPath("test_model.onnx", canonical));
    ASSERT(canonical.extension() == ".onnx");

    // Cleanup
    std::filesystem::remove("test_model.onnx");
}

TEST(invalid_path_does_not_exist) {
    std::filesystem::path canonical;
    ASSERT(!ValidateModelPath("nonexistent_model.onnx", canonical));
}

TEST(invalid_path_wrong_extension) {
    // Create file with wrong extension
    std::ofstream test_file("test.txt");
    test_file << "dummy";
    test_file.close();

    std::filesystem::path canonical;
    ASSERT(!ValidateModelPath("test.txt", canonical));

    // Cleanup
    std::filesystem::remove("test.txt");
}

TEST(invalid_path_directory) {
    // Try to use directory as model path
    std::filesystem::create_directory("test_dir");

    std::filesystem::path canonical;
    ASSERT(!ValidateModelPath("test_dir", canonical));

    // Cleanup
    std::filesystem::remove("test_dir");
}

TEST(invalid_path_null_byte) {
    std::string path_with_null = std::string("model.onnx") + '\0' + "/etc/passwd";
    std::filesystem::path canonical;

    ASSERT(!ValidateModelPath(path_with_null, canonical));
}

TEST(invalid_path_too_long) {
    std::string long_path(5000, 'a');
    long_path += ".onnx";
    std::filesystem::path canonical;

    ASSERT(!ValidateModelPath(long_path, canonical));
}

// ============================================================================
// SanitizeForLog Tests
// ============================================================================

TEST(sanitize_normal_string) {
    std::string input = "normal_model_path.onnx";
    std::string sanitized = SanitizeForLog(input);

    ASSERT_EQ(sanitized, input);
}

TEST(sanitize_ansi_escapes) {
    std::string input = "\x1b[2J\x1b[Hmalicious.onnx";
    std::string sanitized = SanitizeForLog(input);

    // ANSI escapes should be replaced with '?'
    ASSERT(sanitized.find('\x1b') == std::string::npos);
    ASSERT(sanitized.find('?') != std::string::npos);
}

TEST(sanitize_control_characters) {
    std::string input = "model\x01\x02\x03.onnx";
    std::string sanitized = SanitizeForLog(input);

    // Control chars should be removed
    ASSERT(sanitized.find('\x01') == std::string::npos);
    ASSERT(sanitized.find('?') != std::string::npos);
}

TEST(sanitize_newlines_tabs) {
    std::string input = "model\n\r\t.onnx";
    std::string sanitized = SanitizeForLog(input);

    ASSERT(sanitized.find('\n') == std::string::npos);
    ASSERT(sanitized.find('\r') == std::string::npos);
    ASSERT(sanitized.find('\t') == std::string::npos);
}

TEST(sanitize_very_long_string) {
    std::string input(1000, 'a');
    std::string sanitized = SanitizeForLog(input);

    // Should be truncated to 256 chars
    ASSERT(sanitized.size() <= 256);
    ASSERT(sanitized.find("...") != std::string::npos);
}

TEST(sanitize_empty_string) {
    std::string input = "";
    std::string sanitized = SanitizeForLog(input);

    ASSERT_EQ(sanitized, "");
}

// ============================================================================
// ValidateMessageSize Tests
// ============================================================================

TEST(valid_message_size_small) {
    ASSERT(ValidateMessageSize(1024));
}

TEST(valid_message_size_large) {
    ASSERT(ValidateMessageSize(50 * 1024 * 1024));  // 50 MB
}

TEST(valid_message_size_at_limit) {
    ASSERT(ValidateMessageSize(MAX_MESSAGE_SIZE));
}

TEST(invalid_message_size_zero) {
    ASSERT(!ValidateMessageSize(0));
}

TEST(invalid_message_size_too_large) {
    ASSERT(!ValidateMessageSize(MAX_MESSAGE_SIZE + 1));
}

TEST(invalid_message_size_way_too_large) {
    ASSERT(!ValidateMessageSize(1ULL * 1024 * 1024 * 1024));  // 1 GB
}

// ============================================================================
// GenerateSecureModelID Tests
// ============================================================================

TEST(generate_model_id_length) {
    std::string id = GenerateSecureModelID();
    ASSERT_EQ(id.size(), 32);  // 16 bytes = 32 hex chars
}

TEST(generate_model_id_hex) {
    std::string id = GenerateSecureModelID();

    // Should only contain hex chars
    for (char c : id) {
        ASSERT((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
    }
}

TEST(generate_model_id_unique) {
    std::string id1 = GenerateSecureModelID();
    std::string id2 = GenerateSecureModelID();

    // Should be different (extremely high probability)
    ASSERT(id1 != id2);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "======================================\n";
    std::cout << "Security Utils Test Suite\n";
    std::cout << "======================================\n\n";

    // Tests run automatically via static initialization

    std::cout << "\n======================================\n";
    std::cout << "Results: " << tests_passed << "/" << tests_run << " passed\n";

    if (tests_passed == tests_run) {
        std::cout << "✓ ALL TESTS PASSED\n";
        std::cout << "======================================\n\n";
        return 0;
    } else {
        std::cout << "✗ SOME TESTS FAILED\n";
        std::cout << "======================================\n\n";
        return 1;
    }
}
