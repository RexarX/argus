#pragma once

#include <argus/assert.hpp>
#include <argus/gui/world.hpp>
#include <argus/sim/world.hpp>

#include <memory_resource>

namespace argus::gui {

/// @brief Holds all state of the application.
struct App {
    sim::World sim_world;
    World world;
};

/// @brief Creates an app with provided allocator.
/// @param allocator The memory resource to use for allocations
/// @return An instance of `App` with initialized members
/// @warning Trigger asserts if allocator is `nullptr`.
[[nodiscard]] inline App create_app_with(
    std::pmr::memory_resource* allocator =
        std::pmr::get_default_resource()) noexcept {
    ARGUS_ASSERT(allocator != nullptr);
    return {
        .sim_world = sim::create_world_with(allocator),
        .world = create_world_with(allocator),
    };
}

}  // namespace argus::gui
