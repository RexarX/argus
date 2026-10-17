#pragma once

#include <argus/assert.hpp>
#include <argus/math.hpp>

#include <cstdint>
#include <memory_resource>
#include <span>
#include <vector>

namespace argus {

/// @brief YAW, pitch, and roll in radians.
/// @details YAW rotates about world +Y, pitch rises toward +Y, and roll
/// twists about forward. YAW 0 and pitch 0 points forward along +Z.
struct Orientation {
    float yaw = 0.0F;
    float pitch = 0.0F;
    float roll = 0.0F;

    [[nodiscard]] constexpr bool operator==(const Orientation&) const noexcept =
        default;
    [[nodiscard]] constexpr bool operator!=(const Orientation&) const noexcept =
        default;
};

/// @brief Owned triangle mesh.
/// @details Vertices stay in the file's coordinate space. Winding is the
/// file's winding after triangulation.
struct Mesh {
    std::pmr::vector<float3> vertices{std::pmr::get_default_resource()};
    std::pmr::vector<uint3> triangles{std::pmr::get_default_resource()};
};

/// @brief Created empty mesh whose containers use `allocator`.
/// @param allocator Resource for `vertices` and `triangles`
/// @return Empty mesh
/// @warning Asserts if `allocator` is null.
[[nodiscard]] inline Mesh create_mesh_with(
    std::pmr::memory_resource* allocator) noexcept {
    ARGUS_ASSERT(allocator != nullptr);
    return {
        .vertices = std::pmr::vector<float3>{allocator},
        .triangles = std::pmr::vector<uint3>{allocator},
    };
}

/// @brief Read-only view of `Mesh`
struct Mesh_View {
    std::span<const float3> vertices;
    std::span<const uint3> triangles;
};

/// @brief Creates a read-only view of `Mesh`.
/// @param mesh Owned mesh to view
/// @return `Mesh` that provides non-owning access to `mesh`'s vertices and
/// triangles
[[nodiscard]] inline Mesh_View create_view(const Mesh& mesh) noexcept {
    return {.vertices = mesh.vertices, .triangles = mesh.triangles};
}

/// @brief One ray to intersect with a scene.
/// @details `direction` is a unit vector. `max_distance` is the farthest
/// hit accepted along that direction.
struct Ray {
    float3 origin = float3::Zero();
    float3 direction = float3::Zero();
    float max_distance = 0.0F;

    [[nodiscard]] bool operator==(const Ray&) const noexcept = default;
    [[nodiscard]] bool operator!=(const Ray&) const noexcept = default;
};

/// @brief Closest intersection of one ray with a scene.
/// @details `hit == false` leaves the other fields unspecified. `distance`
/// is the total path length from the sensor, including earlier bounces.
/// `facing` is the cosine of the incidence angle after the geometric normal
/// is flipped toward the ray. `1` is head-on and `0` is grazing. `primitive`
/// indexes the hit triangle in the committed mesh. `mirror` is the index in
/// the mirror span of the last mirror reflected off, and is meaningful when
/// `bounces > 0`.
struct Hit {
    float3 position = float3::Zero();
    float3 normal = float3::Zero();
    float distance = 0.0F;
    float facing = 0.0F;
    uint32_t primitive = 0;
    uint16_t mirror = 0;
    uint8_t bounces = 0;
    bool hit = false;

    [[nodiscard]] bool operator==(const Hit&) const noexcept = default;
    [[nodiscard]] bool operator!=(const Hit&) const noexcept = default;
};

}  // namespace argus
