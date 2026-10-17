#pragma once

#include <argus/assert.hpp>
#include <argus/error.hpp>
#include <argus/platform.hpp>
#include <argus/types.hpp>

#include <memory_resource>

namespace argus {

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
    -> Result<Mesh>;

}  // namespace argus
