#pragma once

#include <argus/backend.hpp>
#include <argus/error.hpp>
#include <argus/platform.hpp>

namespace argus::sim {

/// @brief Pointer-sized owner of one ray-tracing backend instance.
struct Instance {
    void* ptr = nullptr;

    /// @brief True if this handle is non-null.
    /// @return `true` when `ptr != nullptr`
    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return ptr != nullptr;
    }

    [[nodiscard]] constexpr bool operator==(const Instance&) const noexcept =
        default;
    [[nodiscard]] constexpr bool operator!=(const Instance&) const noexcept =
        default;
};

/// @brief Native API of this instance.
/// @param instance Instance to query
/// @return Backend stored in the instance
/// @warning Asserts if `instance` is null.
[[nodiscard]] ARGUS_API Backend backend_of(Instance instance) noexcept;

/// @brief Creates an instance with `preferred_backend()`.
/// @details There is no fallback across backends.
/// @return The instance, or `Error::UNSUPPORTED` if no backend is available
[[nodiscard]] ARGUS_API auto create_instance() noexcept -> Result<Instance>;

/// @brief Creates an instance for an explicit backend. No fallback.
/// @param backend Backend to create. Must be in `available_backends()`
/// @return The instance, or `Error::UNSUPPORTED` if `backend` is unavailable
[[nodiscard]] ARGUS_API auto create_instance(Backend backend) noexcept
    -> Result<Instance>;

/// @brief Releases an instance.
/// @details Null is a no-op. Does not clear the caller's `Instance`. Does
/// not release scenes created on it. destroy those scenes separately. They
/// stay usable after the instance is destroyed.
/// @param instance Instance to destroy
ARGUS_API void destroy(Instance instance) noexcept;

}  // namespace argus::sim
