#pragma once

#include <argus/backend.hpp>
#include <argus/instance.hpp>
#include <argus/scan.hpp>
#include <argus/scene.hpp>

namespace argus::sim {

/// @brief Stable name of the simulation module.
/// @return `"argus::sim"`
[[nodiscard]] inline const char* library_name() noexcept {
    return "argus::sim";
}

}  // namespace argus::sim
