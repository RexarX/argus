#pragma once

#include <argus/bound.hpp>
#include <argus/context.hpp>
#include <argus/coverage.hpp>
#include <argus/optimize.hpp>
#include <argus/platform.hpp>
#include <argus/target.hpp>
#include <argus/workspace.hpp>

namespace argus {

/// @brief Stable name of the algorithm module.
/// @return `"argus::algo"`
[[nodiscard]] inline const char* library_name() noexcept {
    return "argus::algo";
}

}  // namespace argus
