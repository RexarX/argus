#pragma once

#include <argus/compiler.hpp>
#include <argus/platform.hpp>
#include <argus/utils/macro.hpp>

#include <format>
#include <memory_resource>
#include <source_location>
#include <string>
#include <string_view>

namespace argus {

/// @brief Function signature for custom assertion handlers.
/// @param condition The failed condition as a string
/// @param message Additional message (may be empty)
/// @param loc Source location of the assertion
using Assertion_Handler = void (*)(std::string_view condition,
                                   std::string_view message,
                                   const std::source_location& loc) noexcept;

/// @brief Default assertion handler (`nullptr` means built-in critical log).
inline constexpr Assertion_Handler DEFAULT_ASSERTION_HANDLER = nullptr;

namespace details {

#if !defined(NDEBUG) || defined(ARGUS_ENABLE_ASSERT)
inline constexpr bool ENABLE_ASSERT = true;
#else
inline constexpr bool ENABLE_ASSERT = false;
#endif

}  // namespace details

/// @brief Formats an assertion failure with source location.
/// @param condition Failed condition text
/// @param message Extra message, or empty
/// @param loc Source location of the assertion
/// @param allocator Resource for the returned string
/// @return Formatted message
/// @warning Asserts if `allocator` is null.
[[nodiscard]] ARGUS_API std::pmr::string format_assertion_message(
    std::string_view condition, std::string_view message,
    const std::source_location& loc = std::source_location::current(),
    std::pmr::memory_resource* allocator =
        std::pmr::get_default_resource()) noexcept;

/// @brief Reports a failed assertion to the active handler.
/// @details Custom handler first, otherwise a critical log. Does not abort;
/// `ARGUS_ASSERT` follows it with `ARGUS_DEBUG_BREAK()`.
/// @param condition Failed condition text
/// @param message Extra message, or empty
/// @param loc Source location of the assertion
ARGUS_API void handle_assertion(
    std::string_view condition, std::string_view message,
    const std::source_location& loc = std::source_location::current()) noexcept;

/// @brief Sets a custom assertion handler.
/// @param handler Custom handler, or `nullptr` for the built-in critical log
ARGUS_API void set_assertion_handler(Assertion_Handler handler) noexcept;

/// @brief Gets the current custom assertion handler.
/// @return The current custom handler, or `nullptr` for the built-in log
[[nodiscard]] ARGUS_API Assertion_Handler get_assertion_handler() noexcept;

}  // namespace argus

// NOLINTBEGIN(cppcoreguidelines-avoid-do-while)
// NOLINTBEGIN(cppcoreguidelines-macro-usage)

/// @brief Aborts when `condition` is false in debug and RelWithDebInfo.
/// @param condition Condition to check
/// @param ... Optional `std::format` message
/// @hideinitializer
#if !defined(NDEBUG) || defined(ARGUS_ENABLE_ASSERT)
#define ARGUS_ASSERT(condition, ...)                              \
    do {                                                          \
        if constexpr (::argus::details::ENABLE_ASSERT) {          \
            if (ARGUS_EXPECT_FALSE(!(condition))) [[unlikely]] {  \
                if constexpr (sizeof(#__VA_ARGS__) > 1) {         \
                    const auto msg = std::format("" __VA_ARGS__); \
                    ::argus::handle_assertion(#condition, msg);   \
                } else {                                          \
                    ::argus::handle_assertion(#condition, "");    \
                }                                                 \
                ARGUS_DEBUG_BREAK();                              \
            }                                                     \
        }                                                         \
    } while (false)
#else
#define ARGUS_ASSERT(condition, ...)                            \
    [[maybe_unused]] static constexpr auto ARGUS_ANONYMOUS_VAR( \
        unused_assert) = 0
#endif

/// @brief Checks `condition` in every build configuration.
/// @param condition Condition to check
/// @param ... Optional `std::format` message
/// @warning debug-breaks when `condition` is false.
/// @hideinitializer
#define ARGUS_VERIFY(condition, ...)                          \
    do {                                                      \
        if (ARGUS_EXPECT_FALSE(!(condition))) [[unlikely]] {  \
            if constexpr (sizeof(#__VA_ARGS__) > 1) {         \
                const auto msg = std::format("" __VA_ARGS__); \
                ::argus::handle_assertion(#condition, msg);   \
            } else {                                          \
                ::argus::handle_assertion(#condition, "");    \
            }                                                 \
            ARGUS_DEBUG_BREAK();                              \
        }                                                     \
    } while (false)

// NOLINTEND(cppcoreguidelines-macro-usage)
// NOLINTEND(cppcoreguidelines-avoid-do-while)
