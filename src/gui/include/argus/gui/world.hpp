#pragma once

#include <argus/assert.hpp>
#include <argus/component_pool.hpp>
#include <argus/entity.hpp>
#include <argus/entity_manager.hpp>
#include <argus/math.hpp>
#include <argus/types.hpp>

#include <memory_resource>

namespace argus::gui {

/// @brief Holds all entity and gui components data.
struct World {
    Entity_Manager entity_manager;
};

/// @brief Creates a world with provided allocator.
/// @param allocator The memory resource to use for allocations
/// @return An instance of `World` with initialized members
/// @warning Trigger asserts if allocator is `nullptr`.
[[nodiscard]] inline World create_world_with(
    std::pmr::memory_resource* allocator =
        std::pmr::get_default_resource()) noexcept {
    ARGUS_ASSERT(allocator != nullptr);
    return {
        .entity_manager = create_entity_manager_with(allocator),
    };
}

}  // namespace argus::gui
