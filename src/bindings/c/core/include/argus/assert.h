#ifndef ARGUS_ASSERT_H
#define ARGUS_ASSERT_H

#include <argus/compiler.h>
#include <argus/platform.h>

#include <stddef.h>

ARGUS_C_BEGIN

/// @brief Function signature for a custom C assertion handler.
/// @param condition Failed condition as a string
/// @param message Additional message; may be empty but not `NULL`
/// @param file Source file of the assertion
/// @param line Source line of the assertion
typedef void (*ArgusAssertionHandler)(const char* condition,
                                      const char* message, const char* file,
                                      int line);

/// @brief Default C assertion handler (`NULL` means built-in critical log).
#define ARGUS_DEFAULT_ASSERTION_HANDLER ((ArgusAssertionHandler)NULL)

/// @brief Dispatches a failed C assertion to the active handler.
/// @param condition Failed condition as a string
/// @param message Additional message, or `NULL` for none
/// @param file Source file of the assertion
/// @param line Source line of the assertion
ARGUS_API void argus_handle_assertion(const char* condition,
                                      const char* message, const char* file,
                                      int line) ARGUS_C_NOEXCEPT;

/// @brief Installs a custom C assertion handler.
/// @details Passing `NULL` restores the built-in critical log.
/// @param handler Custom handler, or `NULL` for default
ARGUS_API void argus_set_assertion_handler(ArgusAssertionHandler handler)
    ARGUS_C_NOEXCEPT;

/// @brief Current custom C assertion handler.
/// @return Custom handler if the C++ slot holds the C thunk, otherwise `NULL`
ARGUS_API ArgusAssertionHandler argus_get_assertion_handler(void)
    ARGUS_C_NOEXCEPT;

#if !defined(NDEBUG) || defined(ARGUS_ENABLE_ASSERT)
/// @brief Assertion that is active in Debug and RelWithDebInfo.
/// @param condition Expression that must be true
/// @warning Fires when `condition` is false.
#define ARGUS_C_ASSERT(condition)                                       \
    do {                                                                \
        if (ARGUS_EXPECT_FALSE(!(condition))) {                         \
            argus_handle_assertion(#condition, "", __FILE__, __LINE__); \
            ARGUS_DEBUG_BREAK();                                        \
        }                                                               \
    } while (0)

/// @brief Assertion with an explicit message string.
/// @param condition Expression that must be true
/// @param message NUL-terminated explanation, or `NULL`
/// @warning Fires when `condition` is false.
#define ARGUS_C_ASSERT_MSG(condition, message)                                 \
    do {                                                                       \
        if (ARGUS_EXPECT_FALSE(!(condition))) {                                \
            argus_handle_assertion(#condition, (message), __FILE__, __LINE__); \
            ARGUS_DEBUG_BREAK();                                               \
        }                                                                      \
    } while (0)
#else
#define ARGUS_C_ASSERT(condition) ((void)0)
#define ARGUS_C_ASSERT_MSG(condition, message) ((void)0)
#endif

ARGUS_C_END

#endif
