#pragma once

#include <argus/backend.hpp>
#include <argus/error.hpp>
#include <argus/instance.hpp>
#include <argus/math.hpp>
#include <argus/scene.hpp>
#include <argus/types.hpp>

#include <span>

namespace argus::sim {

/// @brief Operations one backend provides.
/// @details Every native instance and every native scene starts with a
/// `const Backend_VTable*`. Calls read that table from the handle. A null
/// function means that operation is unsupported.
struct Backend_VTable {
    using Create_Instance_Fn = Result<void*> (*)() noexcept;
    using Destroy_Instance_Fn = void (*)(void* instance) noexcept;
    using Create_Scene_Fn = Result<void*> (*)(void* instance,
                                              const Mesh& mesh) noexcept;
    using Create_User_Scene_Fn =
        Result<void*> (*)(void* instance, const User_Trace& trace) noexcept;
    using Destroy_Scene_Fn = void (*)(void* scene) noexcept;
    using Trace_Fn = Result<void> (*)(void* scene, std::span<const Ray> rays,
                                      std::span<Hit> hits) noexcept;

    Backend backend = Backend::EMBREE;
    Create_Instance_Fn create_instance = nullptr;
    Destroy_Instance_Fn destroy_instance = nullptr;
    Create_Scene_Fn create_scene = nullptr;
    Create_User_Scene_Fn create_user_scene = nullptr;
    Destroy_Scene_Fn destroy_scene = nullptr;
    Trace_Fn trace = nullptr;
};

/// @brief Function table stored in `instance`.
/// @param instance Instance to read. Must be non-null
/// @return Table pointer at the start of the native object
[[nodiscard]] inline const Backend_VTable* table_of(
    Instance instance) noexcept {
    return *static_cast<const Backend_VTable* const*>(instance.ptr);
}

/// @brief Function table stored in `scene`.
/// @param scene Scene to read. Must be non-null
/// @return Table pointer at the start of the native object
[[nodiscard]] inline const Backend_VTable* table_of(Scene scene) noexcept {
    return *static_cast<const Backend_VTable* const*>(scene.ptr);
}

/// @brief Table for `backend`, or null when this binary has no such backend.
/// @param backend Backend to look up
/// @return The table, or null if `backend` was not built
[[nodiscard]] const Backend_VTable* backend_table(Backend backend) noexcept;

}  // namespace argus::sim
