#pragma once

#include <argus/backend.hpp>
#include <argus/error.hpp>
#include <argus/platform.hpp>

struct RTCDeviceTy;

namespace argus::sim {

struct Backend_VTable;

namespace embree {

/// @brief Embree instance.
/// @details First field is the function table the portable `sim::Instance`
/// reads. Owns the Embree context in `rtc`. Does not store geometry.
struct Instance {
    const Backend_VTable* table = nullptr;
    RTCDeviceTy* rtc = nullptr;
};

/// @brief Creates an Embree instance. No scene exists yet.
/// @return Owning pointer, or `Error::OUT_OF_MEMORY`. Release it with
/// `destroy`.
[[nodiscard]] ARGUS_API auto create_instance() noexcept -> Result<Instance*>;

/// @brief Releases the instance.
/// @details Null is a no-op. Scenes already created on this instance stay
/// valid until they are destroyed.
/// @param instance Instance to destroy
ARGUS_API void destroy(Instance* instance) noexcept;

}  // namespace embree

}  // namespace argus::sim
