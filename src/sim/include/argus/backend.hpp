#pragma once

#include <argus/error.hpp>
#include <argus/platform.hpp>

#include <cstdint>
#include <span>
#include <string_view>
#include <utility>

namespace argus::sim {

/// @brief Ray-tracing implementation selected when an instance is created.
enum class Backend : uint8_t {
    EMBREE,
    CALLBACK,
    COUNT,  ///< Not a valid backend
};

inline constexpr size_t BACKEND_COUNT = std::to_underlying(Backend::COUNT);

/// @brief Name of a `Backend` enumerator.
/// @param backend Enumerator to convert
/// @return Enumerator name, or `"Unknown"`
[[nodiscard]] constexpr std::string_view to_string(Backend backend) noexcept {
    switch (backend) {
        using enum Backend;
        case EMBREE:
            return "EMBREE";
        case CALLBACK:
            return "CALLBACK";
    }
    return "Unknown";
}

/// @brief Backends this binary was linked with.
/// @return Span of compiled backends. Empty if none were linked
[[nodiscard]] ARGUS_API auto compiled_backends() noexcept
    -> std::span<const Backend>;

/// @brief Backends that can run in this process.
/// @return Span of backends that can run. Empty if none can
[[nodiscard]] ARGUS_API auto available_backends() noexcept
    -> std::span<const Backend>;

/// @brief Membership in `available_backends()`.
/// @param backend Backend to test
/// @return `true` if `backend` is in `available_backends()`
[[nodiscard]] ARGUS_API bool available(Backend backend) noexcept;

/// @brief Default backend.
/// @details `CALLBACK` is listed last, so a compiled mesh backend stays
/// ahead of it.
/// @return `available_backends()[0]`, or `Error::UNSUPPORTED` if the span is
/// empty
[[nodiscard]] ARGUS_API auto preferred_backend() noexcept -> Result<Backend>;

}  // namespace argus::sim
