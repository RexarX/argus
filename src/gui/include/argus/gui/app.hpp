#pragma once

#include <argus/assert.hpp>
#include <argus/component_pool.hpp>
#include <argus/entity.hpp>
#include <argus/entity_manager.hpp>
#include <argus/math.hpp>
#include <argus/types.hpp>

#include <memory_resource>

namespace argus::gui {

/// @brief The main structure that holds all state of the application.
struct App {
    // ECS related members
    Entity_Manager entity_manager;
    Component_Pool<float3> position_pool;
    Component_Pool<Orientation> orientation_pool;
    Component_Pool<float> scale_pool;
};

/// @brief Creates an App with provided allocator.
/// @param allocator The memory resource to use for allocations
/// @return An instance of App with initialized members
/// @warning Trigger asserts if allocator is `nullptr`.
[[nodiscard]] inline App create_app_with(
    std::pmr::memory_resource* allocator =
        std::pmr::get_default_resource()) noexcept {
    ARGUS_ASSERT(allocator != nullptr);
    return {
        .entity_manager = create_entity_manager_with(allocator),
        .position_pool = create_component_pool_with<float3>(allocator),
        .orientation_pool = create_component_pool_with<Orientation>(allocator),
        .scale_pool = create_component_pool_with<float>(allocator),
    };
}

}  // namespace argus::gui
