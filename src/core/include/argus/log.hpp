#pragma once

#include <argus/memory/temporary_storage.hpp>
#include <argus/platform.hpp>
#include <argus/utils/format.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory_resource>
#include <string_view>
#include <utility>

namespace argus::log {

/// @brief Log severity levels.
enum class Level : uint8_t { TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL };

/// @brief Converts a log level to its string representation.
/// @param level The log level to convert
/// @return String representation of the log level
[[nodiscard]] constexpr std::string_view to_string(Level level) noexcept {
    switch (level) {
        using enum Level;
        case TRACE:
            return "trace";
        case DEBUG:
            return "debug";
        case INFO:
            return "info";
        case WARN:
            return "warn";
        case ERROR:
            return "error";
        case CRITICAL:
            return "critical";
    }
    return "unknown";
}

/// @brief Signature of a user-supplied logger callback.
using Callback = void (*)(Level level, std::string_view message) noexcept;

/// @brief Installs a custom logger, overriding the built-in output.
/// @details Passing `nullptr` is equivalent to `reset_custom_logger()`.
/// @param callback Function pointer to receive all future log records
ARGUS_API void set_custom_logger(Callback callback) noexcept;

/// @brief Removes any installed custom logger, restoring built-in output.
ARGUS_API void reset_custom_logger() noexcept;

/// @brief Checks whether a custom logger is currently installed.
/// @return `true` if a custom logger is installed
[[nodiscard]] ARGUS_API bool has_custom_logger() noexcept;

/// @brief Gets the current custom logger.
/// @return The current custom logger, or `nullptr` for built-in output
[[nodiscard]] ARGUS_API Callback get_custom_logger() noexcept;

/// @brief Dispatches a pre-formatted message to the active logger.
/// @param level Severity of the record
/// @param message Pre-formatted text
ARGUS_API void log(Level level, std::string_view message) noexcept;

/// @brief Size of the inline buffer used for formatting log messages.
inline constexpr size_t INLINE_BUFFER_SIZE = 256;

/// @brief Logs a formatted message.
/// @tparam Args Format argument types
/// @param level Severity of the record
/// @param fmt Format string
/// @param args Format arguments
template <typename... Args>
inline void log(Level level, std::format_string<Args...> fmt,
                Args&&... args) noexcept {
    std::array<std::byte, INLINE_BUFFER_SIZE> stack_buffer = {};
    auto temp = mem::get_temporary_resource();
    std::pmr::monotonic_buffer_resource resource{stack_buffer.data(),
                                                 stack_buffer.size(), &temp};
    std::pmr::string message{&resource};
    utils::format_to(message, fmt, std::forward<Args>(args)...);
    log(level, message);
}

#ifndef NDEBUG
/// @brief Logs a trace message.
/// @param message The message to log
inline void trace(std::string_view message) noexcept {
    log(Level::TRACE, message);
}

/// @brief Logs a formatted trace message.
/// @tparam Args Format argument types
/// @param fmt Format string
/// @param args Format arguments
template <typename... Args>
    requires(sizeof...(Args) > 0)
inline void trace(std::format_string<Args...> fmt, Args&&... args) noexcept {
    log(Level::TRACE, fmt, std::forward<Args>(args)...);
}

/// @brief Logs a debug message.
/// @param message The message to log
inline void debug(std::string_view message) noexcept {
    log(Level::DEBUG, message);
}

/// @brief Logs a formatted debug message.
/// @tparam Args Format argument types
/// @param fmt Format string
/// @param args Format arguments
template <typename... Args>
    requires(sizeof...(Args) > 0)
inline void debug(std::format_string<Args...> fmt, Args&&... args) noexcept {
    log(Level::DEBUG, fmt, std::forward<Args>(args)...);
}

#else
inline void trace(std::string_view /*message*/) noexcept {}

template <typename... Args>
inline void trace(Args&&... /*args*/) noexcept {}

inline void debug(std::string_view /*message*/) noexcept {}

template <typename... Args>
inline void debug(Args&&... /*args*/) noexcept {}
#endif

/// @brief Logs an info message.
/// @param message The message to log
inline void info(std::string_view message) noexcept {
    log(Level::INFO, message);
}

/// @brief Logs a formatted info message.
/// @tparam Args Format argument types
/// @param fmt Format string
/// @param args Format arguments
template <typename... Args>
    requires(sizeof...(Args) > 0)
inline void info(std::format_string<Args...> fmt, Args&&... args) noexcept {
    log(Level::INFO, fmt, std::forward<Args>(args)...);
}

/// @brief Logs a warning message.
/// @param message The message to log
inline void warn(std::string_view message) noexcept {
    log(Level::WARN, message);
}

/// @brief Logs a formatted warning message.
/// @tparam Args Format argument types
/// @param fmt Format string
/// @param args Format arguments
template <typename... Args>
    requires(sizeof...(Args) > 0)
inline void warn(std::format_string<Args...> fmt, Args&&... args) noexcept {
    log(Level::WARN, fmt, std::forward<Args>(args)...);
}

/// @brief Logs an error message.
/// @param message The message to log
inline void error(std::string_view message) noexcept {
    log(Level::ERROR, message);
}

/// @brief Logs a formatted error message.
/// @tparam Args Format argument types
/// @param fmt Format string
/// @param args Format arguments
template <typename... Args>
    requires(sizeof...(Args) > 0)
inline void error(std::format_string<Args...> fmt, Args&&... args) noexcept {
    log(Level::ERROR, fmt, std::forward<Args>(args)...);
}

/// @brief Logs a critical message.
/// @param message The message to log
inline void critical(std::string_view message) noexcept {
    log(Level::CRITICAL, message);
}

/// @brief Logs a formatted critical message.
/// @tparam Args Format argument types
/// @param fmt Format string
/// @param args Format arguments
template <typename... Args>
    requires(sizeof...(Args) > 0)
inline void critical(std::format_string<Args...> fmt, Args&&... args) noexcept {
    log(Level::CRITICAL, fmt, std::forward<Args>(args)...);
}

}  // namespace argus::log
