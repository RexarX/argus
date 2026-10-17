#include <pch.hpp>

#include <argus/mesh_io.hpp>

#include <argus/assert.hpp>
#include <argus/error.hpp>

#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include <cstdint>
#include <memory_resource>

namespace argus {

namespace {

constexpr unsigned int IMPORT_FLAGS =
    aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
    aiProcess_SortByPType | aiProcess_PreTransformVertices;

[[nodiscard]] bool has_triangles(const aiMesh* mesh) noexcept {
    return mesh != nullptr &&
           mesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE &&
           mesh->mVertices != nullptr && mesh->mFaces != nullptr;
}

}  // namespace

auto load_mesh(const char* path, std::pmr::memory_resource* allocator) noexcept
    -> Result<Mesh> {
    ARGUS_ASSERT(allocator != nullptr);
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, IMPORT_FLAGS);
    if (scene == nullptr) [[unlikely]] {
        return fail(Error::INVALID, "Mesh import failed for '{}': {}!", path,
                    importer.GetErrorString());
    }

    Mesh loaded = create_mesh_with(allocator);
    for (unsigned int index = 0; index < scene->mNumMeshes; ++index) {
        const aiMesh* mesh = scene->mMeshes[index];
        if (!has_triangles(mesh)) {
            continue;
        }

        if (loaded.vertices.size() > UINT32_MAX - mesh->mNumVertices)
            [[unlikely]] {
            return fail(Error::INVALID, "Mesh '{}' has too many vertices!",
                        path);
        }

        const auto base = static_cast<uint32_t>(loaded.vertices.size());
        loaded.vertices.reserve(loaded.vertices.size() + mesh->mNumVertices);
        for (unsigned int vertex = 0; vertex < mesh->mNumVertices; ++vertex) {
            const aiVector3D& point = mesh->mVertices[vertex];
            loaded.vertices.emplace_back(point.x, point.y, point.z);
        }

        loaded.triangles.reserve(loaded.triangles.size() + mesh->mNumFaces);
        for (unsigned int face = 0; face < mesh->mNumFaces; ++face) {
            const aiFace& polygon = mesh->mFaces[face];
            if (polygon.mNumIndices != 3 || polygon.mIndices == nullptr) {
                continue;
            }
            loaded.triangles.emplace_back(base + polygon.mIndices[0],
                                          base + polygon.mIndices[1],
                                          base + polygon.mIndices[2]);
        }
    }
    return loaded;
}

}  // namespace argus
