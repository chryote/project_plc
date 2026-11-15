#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace plc::core {

enum class LogLevel {
    Debug = 0,
    Info,
    Warn,
    Error,
    None
};

class Logger {
public:
    static Logger& instance() {
        static Logger instance_;
        return instance_;
    }

    void set_level(LogLevel level) noexcept {
        current_level_ = level;
    }

    [[nodiscard]] LogLevel level() const noexcept {
        return current_level_;
    }

    void log(LogLevel level, std::string_view msg) {
        if (level < current_level_ || current_level_ == LogLevel::None) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::tm tm_buf{};
#if defined(_WIN32)
        localtime_s(&tm_buf, &now_time_t);
#else
        localtime_r(&now_time_t, &tm_buf);
#endif

        std::ostream& out = (level == LogLevel::Error) ? std::cerr : std::cout;

        out << "[" << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
            << '.' << std::setfill('0') << std::setw(3) << now_ms.count() << "] ["
            << level_prefix(level) << "] "
            << msg << "\n";
    }

    void debug(std::string_view msg) { log(LogLevel::Debug, msg); }
    void info(std::string_view msg) { log(LogLevel::Info, msg); }
    void warn(std::string_view msg) { log(LogLevel::Warn, msg); }
    void error(std::string_view msg) { log(LogLevel::Error, msg); }

private:
    Logger() = default;

    static const char* level_prefix(LogLevel level) noexcept {
        switch (level) {
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info:  return "INFO ";
            case LogLevel::Warn:  return "WARN ";
            case LogLevel::Error: return "ERROR";
            default:              return "LOG  ";
        }
    }

    LogLevel current_level_ = LogLevel::Info;
    std::mutex mutex_;
};

#define LOG_DEBUG(msg) do { \
    std::ostringstream _oss; _oss << msg; \
    ::plc::core::Logger::instance().debug(_oss.str()); \
} while(0)

#define LOG_INFO(msg) do { \
    std::ostringstream _oss; _oss << msg; \
    ::plc::core::Logger::instance().info(_oss.str()); \
} while(0)

#define LOG_WARN(msg) do { \
    std::ostringstream _oss; _oss << msg; \
    ::plc::core::Logger::instance().warn(_oss.str()); \
} while(0)

#define LOG_ERROR(msg) do { \
    std::ostringstream _oss; _oss << msg; \
    ::plc::core::Logger::instance().error(_oss.str()); \
} while(0)

} // namespace plc::core
