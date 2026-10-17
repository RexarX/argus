#pragma once

#include <argus/assert.hpp>
#include <argus/component_pool.hpp>
#include <argus/entity.hpp>
#include <argus/entity_manager.hpp>
#include <argus/lidar.hpp>
#include <argus/math.hpp>
#include <argus/mirror.hpp>
#include <argus/types.hpp>

#include <memory_resource>

namespace argus::sim {

/// @brief Holds all entity and sim components data.
struct World {
    Entity_Manager entity_manager;
    Component_Pool<float3> position_pool;
    Component_Pool<Orientation> orientation_pool;
    Component_Pool<float> scale_pool;
    Component_Pool<float> distance_pool;
    Component_Pool<Pattern> pattern_pool;
    Component_Pool<Extent> extent_pool;
    Component_Pool<Mesh> mesh_pool;
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
        .position_pool = create_component_pool_with<float3>(allocator),
        .orientation_pool = create_component_pool_with<Orientation>(allocator),
        .scale_pool = create_component_pool_with<float>(allocator),
        .distance_pool = create_component_pool_with<float>(allocator),
        .pattern_pool = create_component_pool_with<Pattern>(allocator),
        .extent_pool = create_component_pool_with<Extent>(allocator),
        .mesh_pool = create_component_pool_with<Mesh>(allocator),
    };
}

}  // namespace argus::sim
