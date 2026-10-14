#pragma once

#include <argus/log.hpp>
#include <argus/utils/format.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory_resource>
#include <string_view>
#include <utility>

namespace argus {

/// @brief Recoverable failure of a valid call.
enum class Error : uint8_t {
    UNSUPPORTED,
    OUT_OF_MEMORY,
    INVALID,
};

/// @brief Name of an `Error` enumerator.
/// @param error Enumerator to convert
/// @return Enumerator name, or `"Unknown"`
[[nodiscard]] constexpr std::string_view to_string(Error error) noexcept {
    switch (error) {
        using enum Error;
        case UNSUPPORTED:
            return "UNSUPPORTED";
        case OUT_OF_MEMORY:
            return "OUT_OF_MEMORY";
        case INVALID:
            return "INVALID";
    }
    return "Unknown";
}

/// @brief Value-or-error result of a fallible call.
/// @tparam T Payload type on success
template <typename T>
using Result = std::expected<T, Error>;

/// @brief Logs a recoverable failure and returns `std::unexpected(error)`.
/// @param error Error code to return
/// @param message Context for the log record (prefixed with the error name)
/// @return `std::unexpected(error)`
[[nodiscard]] inline auto fail(Error error, std::string_view message) noexcept
    -> std::unexpected<Error> {
    log::error("{}: {}", to_string(error), message);
    return std::unexpected(error);
}

/// @brief Logs a formatted recoverable failure and returns
/// `std::unexpected(error)`.
/// @tparam Args Format argument types
/// @param error Error code to return
/// @param fmt Format string for the context (prefixed with the error name)
/// @param args Format arguments
/// @return `std::unexpected(error)`
template <typename... Args>
    requires(sizeof...(Args) > 0)
[[nodiscard]] inline auto fail(Error error, std::format_string<Args...> fmt,
                               Args&&... args) noexcept
    -> std::unexpected<Error> {
    std::array<std::byte, log::INLINE_BUFFER_SIZE> stack_buffer = {};
    std::pmr::monotonic_buffer_resource resource{
        stack_buffer.data(), stack_buffer.size(),
        std::pmr::new_delete_resource()};
    std::pmr::string message{&resource};
    utils::format_to(message, fmt, std::forward<Args>(args)...);
    log::error("{}: {}", to_string(error), message);
    return std::unexpected(error);
}

}  // namespace argus
