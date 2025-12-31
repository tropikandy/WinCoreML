/**
 * Logger Implementation
 */

#include "../include/logger.h"
#include <iostream>
#include <thread>
#include <iomanip>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

namespace coremlwin {

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    Shutdown();
}

void Logger::Initialize(
    LogLevel min_level,
    bool log_to_file,
    const std::string& log_file_path,
    bool log_to_console
) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        return;  // Already initialized
    }

    min_level_ = min_level;
    log_to_file_ = log_to_file;
    log_to_console_ = log_to_console;

    if (log_to_file_) {
        log_file_.open(log_file_path, std::ios::out | std::ios::app);
        if (!log_file_.is_open()) {
            std::cerr << "Failed to open log file: " << log_file_path << std::endl;
            log_to_file_ = false;
        } else {
            // Write startup message
            log_file_ << "\n========================================\n";
            log_file_ << "Logger initialized at " << GetTimestamp() << "\n";
            log_file_ << "Min level: " << LogLevelToString(min_level_) << "\n";
            log_file_ << "========================================\n";
            log_file_.flush();
        }
    }

    initialized_ = true;
}

void Logger::SetMinLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = level;
}

void Logger::Log(
    LogLevel level,
    const char* file,
    int line,
    const std::string& message
) {
    // Early return if level is below minimum
    if (level < min_level_) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Build log line
    std::ostringstream log_line;
    log_line << GetTimestamp() << " "
             << "[" << GetThreadId() << "] "
             << LogLevelToString(level) << " "
             << file << ":" << line << " - "
             << message;

    std::string log_str = log_line.str();

    // Output to console
    if (log_to_console_) {
        if (level >= LogLevel::ERROR) {
            std::cerr << log_str << std::endl;
        } else {
            std::cout << log_str << std::endl;
        }
    }

    // Output to file
    if (log_to_file_ && log_file_.is_open()) {
        log_file_ << log_str << std::endl;

        // Flush on WARNING or higher
        if (level >= LogLevel::WARNING) {
            log_file_.flush();
        }
    }
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (log_file_.is_open()) {
        log_file_ << "Logger shutdown at " << GetTimestamp() << "\n";
        log_file_.close();
    }

    initialized_ = false;
}

std::string Logger::GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ) % 1000;

    std::tm tm_buf;

#ifdef _WIN32
    localtime_s(&tm_buf, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_buf);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return oss.str();
}

std::string Logger::GetThreadId() {
    std::ostringstream oss;

#ifdef _WIN32
    oss << GetCurrentThreadId();
#else
    oss << std::this_thread::get_id();
#endif

    return oss.str();
}

} // namespace coremlwin
