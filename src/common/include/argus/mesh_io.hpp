#pragma once

#include <argus/assert.hpp>
#include <argus/error.hpp>
#include <argus/platform.hpp>
#include <argus/types.hpp>

#include <memory_resource>
#include <vector>

namespace argus {

/// @brief Owned triangle mesh loaded from a file.
/// @details Vertices stay in the file's coordinate space. Winding is the
/// file's winding after triangulation. `get_view` is the non-owning mesh sim
/// accepts.
struct Owned_Mesh {
    std::pmr::vector<float3> vertices{std::pmr::get_default_resource()};
    std::pmr::vector<uint3> triangles{std::pmr::get_default_resource()};
};

/// @brief Created empty owned mesh whose containers use `allocator`.
/// @param allocator Resource for `vertices` and `triangles`
/// @return Empty owned mesh
/// @warning Asserts if `allocator` is null.
[[nodiscard]] inline Owned_Mesh create_loaded_mesh_with(
    std::pmr::memory_resource* allocator) noexcept {
    ARGUS_ASSERT(allocator != nullptr);
    return {
        .vertices = std::pmr::vector<float3>{allocator},
        .triangles = std::pmr::vector<uint3>{allocator},
    };
}

/// @brief Non-owning view of `mesh.vertices` and `mesh.triangles`.
/// @param mesh Owned mesh to view
/// @return `Mesh` that provides non-owning access to `mesh`'s vertices and
/// triangles
[[nodiscard]] inline Mesh get_view(const Owned_Mesh& mesh) noexcept {
    return {.vertices = mesh.vertices, .triangles = mesh.triangles};
}

/// @brief Loads a triangle mesh from `path`.
/// @details Triangulates faces, joins identical vertices, and bakes node
/// transforms into one mesh. A file with no triangles succeeds with empty
/// spans.
/// @param path Filesystem path of the asset
/// @param allocator Resource for the returned vertex and triangle vectors
/// @return The mesh, or `Error::INVALID` when the file cannot be imported
/// @warning Asserts if `allocator` is null.
[[nodiscard]] ARGUS_API auto load_mesh(
    const char* path, std::pmr::memory_resource* allocator =
                          std::pmr::get_default_resource()) noexcept
    -> Result<Owned_Mesh>;

}  // namespace argus
