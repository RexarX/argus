#ifndef ARGUS_LOG_H
#define ARGUS_LOG_H

#include <argus/compiler.h>
#include <argus/platform.h>

#include <stdbool.h>
#include <stdint.h>

ARGUS_C_BEGIN

/// @brief Log severity levels.
typedef uint8_t ArgusLogLevel;

enum {
    ARGUS_LOG_LEVEL_TRACE = 0U,
    ARGUS_LOG_LEVEL_DEBUG,
    ARGUS_LOG_LEVEL_INFO,
    ARGUS_LOG_LEVEL_WARN,
    ARGUS_LOG_LEVEL_ERROR,
    ARGUS_LOG_LEVEL_CRITICAL,
};

/// @brief Converts a log level to its string representation.
/// @param level Enumerator to convert
/// @return Enumerator name, or `"unknown"`
static inline const char* argus_log_level_to_string(ArgusLogLevel level) {
    switch (level) {
        case ARGUS_LOG_LEVEL_TRACE:
            return "trace";
        case ARGUS_LOG_LEVEL_DEBUG:
            return "debug";
        case ARGUS_LOG_LEVEL_INFO:
            return "info";
        case ARGUS_LOG_LEVEL_WARN:
            return "warn";
        case ARGUS_LOG_LEVEL_ERROR:
            return "error";
        case ARGUS_LOG_LEVEL_CRITICAL:
            return "critical";
        default:
            return "unknown";
    }
}

/// @brief Signature of a user-supplied logger callback.
/// @param level Severity of the record
/// @param message NUL-terminated text
/// @param user Pointer passed to `argus_log_set_custom`
typedef void (*ArgusLogCallback)(ArgusLogLevel level, const char* message,
                                 void* user);

/// @brief Installs a custom C logger, overriding the built-in output.
/// @param callback Function pointer, or `NULL` for the default logger
/// @param user Forwarded to every callback invocation. May be `NULL`
ARGUS_API void argus_log_set_custom(ArgusLogCallback callback,
                                    void* user) ARGUS_C_NOEXCEPT;

/// @brief Removes any installed custom logger, restoring built-in output.
ARGUS_API void argus_log_reset_custom(void) ARGUS_C_NOEXCEPT;

/// @brief Checks whether a custom C logger is currently installed.
/// @return `true` if the C++ slot holds the C thunk
ARGUS_API bool argus_log_has_custom(void) ARGUS_C_NOEXCEPT;

/// @brief Dispatches a pre-formatted NUL-terminated message.
/// @param level Severity of the record
/// @param message NUL-terminated text
/// @warning Asserts if `message` is null.
ARGUS_API void argus_log(ArgusLogLevel level,
                         const char* message) ARGUS_C_NOEXCEPT;

/// @brief Logs a NUL-terminated trace message. No-op in release.
/// @param message NUL-terminated text
ARGUS_API void argus_log_trace(const char* message) ARGUS_C_NOEXCEPT;

/// @brief Logs a NUL-terminated debug message. No-op in release.
/// @param message NUL-terminated text
ARGUS_API void argus_log_debug(const char* message) ARGUS_C_NOEXCEPT;

/// @brief Logs a NUL-terminated info message.
/// @param message NUL-terminated text
ARGUS_API void argus_log_info(const char* message) ARGUS_C_NOEXCEPT;

/// @brief Logs a NUL-terminated warning message.
/// @param message NUL-terminated text
ARGUS_API void argus_log_warn(const char* message) ARGUS_C_NOEXCEPT;

/// @brief Logs a NUL-terminated error message.
/// @param message NUL-terminated text
ARGUS_API void argus_log_error(const char* message) ARGUS_C_NOEXCEPT;

/// @brief Logs a NUL-terminated critical message.
/// @param message NUL-terminated text
ARGUS_API void argus_log_critical(const char* message) ARGUS_C_NOEXCEPT;

/// @brief Logs a `printf`-formatted message at `level`.
/// @param level Severity of the record
/// @param fmt `printf` format string
ARGUS_API void argus_logf(ArgusLogLevel level, const char* fmt,
                          ...) ARGUS_C_NOEXCEPT ARGUS_C_PRINTF(2, 3);

/// @brief Logs a `printf`-formatted trace message. No-op in release.
/// @param fmt `printf` format string
ARGUS_API void argus_log_tracef(const char* fmt, ...) ARGUS_C_NOEXCEPT
    ARGUS_C_PRINTF(1, 2);

/// @brief Logs a `printf`-formatted debug message. No-op in release.
/// @param fmt `printf` format string
ARGUS_API void argus_log_debugf(const char* fmt, ...) ARGUS_C_NOEXCEPT
    ARGUS_C_PRINTF(1, 2);

/// @brief Logs a `printf`-formatted info message.
/// @param fmt `printf` format string
ARGUS_API void argus_log_infof(const char* fmt, ...) ARGUS_C_NOEXCEPT
    ARGUS_C_PRINTF(1, 2);

/// @brief Logs a `printf`-formatted warning message.
/// @param fmt `printf` format string
ARGUS_API void argus_log_warnf(const char* fmt, ...) ARGUS_C_NOEXCEPT
    ARGUS_C_PRINTF(1, 2);

/// @brief Logs a `printf`-formatted error message.
/// @param fmt `printf` format string
ARGUS_API void argus_log_errorf(const char* fmt, ...) ARGUS_C_NOEXCEPT
    ARGUS_C_PRINTF(1, 2);

/// @brief Logs a `printf`-formatted critical message.
/// @param fmt `printf` format string
ARGUS_API void argus_log_criticalf(const char* fmt, ...) ARGUS_C_NOEXCEPT
    ARGUS_C_PRINTF(1, 2);

ARGUS_C_END

#endif
