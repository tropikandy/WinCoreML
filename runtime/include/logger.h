/**
 * Logger - Structured logging system for Universal ML Runtime
 *
 * Thread-safe logging with multiple severity levels and output targets.
 * Replaces std::cout/std::cerr with structured, filterable logging.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <memory>

namespace coremlwin {

/**
 * Log severity levels
 */
enum class LogLevel {
    TRACE = 0,    // Very detailed debugging
    DEBUG = 1,    // Debug information
    INFO = 2,     // General information
    WARNING = 3,  // Warning messages
    ERROR = 4,    // Error messages
    FATAL = 5     // Fatal errors
};

/**
 * Convert log level to string
 */
inline const char* LogLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE:   return "TRACE";
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO ";
        case LogLevel::WARNING: return "WARN ";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

/**
 * Logger implementation
 */
class Logger {
public:
    /**
     * Get singleton instance
     */
    static Logger& GetInstance();

    /**
     * Initialize logger
     * @param min_level Minimum log level to output
     * @param log_to_file Whether to log to file
     * @param log_file_path Path to log file (if log_to_file is true)
     * @param log_to_console Whether to log to console
     */
    void Initialize(
        LogLevel min_level = LogLevel::INFO,
        bool log_to_file = true,
        const std::string& log_file_path = "coremlwin_runtime.log",
        bool log_to_console = true
    );

    /**
     * Set minimum log level
     */
    void SetMinLevel(LogLevel level);

    /**
     * Log a message
     */
    void Log(
        LogLevel level,
        const char* file,
        int line,
        const std::string& message
    );

    /**
     * Shutdown logger (flush and close files)
     */
    void Shutdown();

private:
    Logger() = default;
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string GetTimestamp();
    std::string GetThreadId();

    std::mutex mutex_;
    LogLevel min_level_ = LogLevel::INFO;
    bool log_to_file_ = false;
    bool log_to_console_ = true;
    std::ofstream log_file_;
    bool initialized_ = false;
};

/**
 * Log stream helper for convenient logging
 */
class LogStream {
public:
    LogStream(LogLevel level, const char* file, int line)
        : level_(level), file_(file), line_(line) {}

    ~LogStream() {
        Logger::GetInstance().Log(level_, file_, line_, stream_.str());
    }

    template<typename T>
    LogStream& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }

private:
    LogLevel level_;
    const char* file_;
    int line_;
    std::ostringstream stream_;
};

} // namespace coremlwin

// Logging macros for convenient use
#define LOG_TRACE    coremlwin::LogStream(coremlwin::LogLevel::TRACE,   __FILE__, __LINE__)
#define LOG_DEBUG    coremlwin::LogStream(coremlwin::LogLevel::DEBUG,   __FILE__, __LINE__)
#define LOG_INFO     coremlwin::LogStream(coremlwin::LogLevel::INFO,    __FILE__, __LINE__)
#define LOG_WARNING  coremlwin::LogStream(coremlwin::LogLevel::WARNING, __FILE__, __LINE__)
#define LOG_ERROR    coremlwin::LogStream(coremlwin::LogLevel::ERROR,   __FILE__, __LINE__)
#define LOG_FATAL    coremlwin::LogStream(coremlwin::LogLevel::FATAL,   __FILE__, __LINE__)

// Conditional logging (only log if condition is true)
#define LOG_IF(level, condition) \
    if (condition) coremlwin::LogStream(level, __FILE__, __LINE__)

// Log with context (adds key-value pairs)
#define LOG_WITH_CONTEXT(level, context) \
    coremlwin::LogStream(level, __FILE__, __LINE__) << "[" << context << "] "

#endif // LOGGER_H
