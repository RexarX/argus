#include <pch.hpp>

#include <argus/sim/scene.hpp>

#include <argus/assert.hpp>
#include <argus/error.hpp>
#include <argus/sim/backend.hpp>
#include <argus/sim/instance.hpp>
#include <dispatch.hpp>

#include <expected>
#include <span>

namespace argus::sim {

namespace {

[[nodiscard]] auto as_scene(Result<void*> created) noexcept -> Result<Scene> {
    if (!created) [[unlikely]] {
        return std::unexpected(created.error());
    }
    ARGUS_ASSERT(*created != nullptr);
    return Scene{.ptr = *created};
}

}  // namespace

Backend backend_of(Scene scene) noexcept {
    ARGUS_ASSERT(scene.ptr != nullptr);
    const Backend_VTable* table = table_of(scene);
    ARGUS_ASSERT(table != nullptr);
    return table->backend;
}

auto create_scene(Instance instance, const Mesh_View& mesh) noexcept
    -> Result<Scene> {
    ARGUS_ASSERT(instance.ptr != nullptr);

    const Backend_VTable* table = table_of(instance);
    ARGUS_ASSERT(table != nullptr, "Unknown instance backend!");
    if (table->create_scene == nullptr) [[unlikely]] {
        return fail(Error::UNSUPPORTED,
                    "Backend '{}' does not support mesh scenes!",
                    to_string(table->backend));
    }
    return as_scene(table->create_scene(instance.ptr, mesh));
}

auto create_scene(Instance instance, const User_Trace& trace) noexcept
    -> Result<Scene> {
    ARGUS_ASSERT(instance.ptr != nullptr);
    ARGUS_ASSERT(trace.trace != nullptr);

    const Backend_VTable* table = table_of(instance);
    ARGUS_ASSERT(table != nullptr, "Unknown instance backend!");
    if (table->create_user_scene == nullptr) [[unlikely]] {
        return fail(Error::UNSUPPORTED,
                    "Backend '{}' does not support user trace scenes!",
                    to_string(table->backend));
    }
    return as_scene(table->create_user_scene(instance.ptr, trace));
}

void destroy(Scene scene) noexcept {
    if (!scene) [[unlikely]] {
        return;
    }

    const Backend_VTable* table = table_of(scene);
    ARGUS_ASSERT(table != nullptr && table->destroy_scene != nullptr,
                 "Unknown scene backend: {}!", to_string(table->backend));
    table->destroy_scene(scene.ptr);
}

auto trace(Scene scene, std::span<const Ray> rays, std::span<Hit> hits) noexcept
    -> Result<void> {
    ARGUS_ASSERT(scene.ptr != nullptr);
    ARGUS_ASSERT(rays.size() == hits.size());

    const Backend_VTable* table = table_of(scene);
    ARGUS_ASSERT(table != nullptr && table->trace != nullptr,
                 "Unknown scene backend: {}!", to_string(table->backend));
    return table->trace(scene.ptr, rays, hits);
}

}  // namespace argus::sim
