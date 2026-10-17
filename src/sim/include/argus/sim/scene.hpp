#pragma once

#include <argus/error.hpp>
#include <argus/platform.hpp>
#include <argus/sim/instance.hpp>
#include <argus/types.hpp>

#include <span>

namespace argus::sim {

/// @brief Pointer-sized owner of one committed scene.
/// @details `create_scene` returns it. The scene owns the acceleration
/// structure built from the mesh. The instance it was created on does not.
/// One instance can have many live scenes. Null until `create_scene` returns
/// it. The pointed-to object starts with that backend's function table.
struct Scene {
    void* ptr = nullptr;

    /// @brief True if this handle is non-null.
    /// @return `true` when `ptr != nullptr`
    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return ptr != nullptr;
    }

    [[nodiscard]] friend constexpr bool operator==(
        const Scene&, const Scene&) noexcept = default;
    [[nodiscard]] friend constexpr bool operator!=(
        const Scene&, const Scene&) noexcept = default;
};

/// @brief User-supplied ray caster.
/// @details `trace` replaces built-in intersection. `user` is passed through
/// unchanged. A simulator can trace with its own line tests and leave the
/// mesh export for an offline `Embree` scene. `trace` writes one hit per ray.
struct User_Trace {
    using Function = Result<void> (*)(void* user, std::span<const Ray> rays,
                                      std::span<Hit> hits) noexcept;

    Function trace = nullptr;
    void* user = nullptr;
};

/// @brief Native API of this scene.
/// @param scene Scene to query
/// @return Backend that created the scene
/// @warning Asserts if `scene` is null.
[[nodiscard]] ARGUS_API Backend backend_of(Scene scene) noexcept;

/// @brief Creates a scene of `mesh` on `instance`.
/// @details Copies `mesh` into the backend. Does not store it on `instance`.
/// Empty geometry is valid. Each call creates another scene, so one instance
/// can have many live scenes. `mesh` spans need to live only for this call.
/// @param instance Instance that creates the scene
/// @param mesh Indexed triangle mesh to commit
/// @return The scene, `Error::UNSUPPORTED` if `instance` cannot create a
/// mesh scene, or another recoverable `Error` from the backend
/// @warning Asserts if `instance` is null.
[[nodiscard]] ARGUS_API auto create_scene(Instance instance,
                                          const Mesh_View& mesh) noexcept
    -> Result<Scene>;

/// @brief Creates a callback scene of `trace` on `instance`.
/// @details Copies the function and user pointer into the scene. Does not
/// store them on `instance`. Each call creates another scene.
/// @param instance Callback instance
/// @param trace Callback and user pointer. `trace.trace` is required
/// @return The scene, or `Error::UNSUPPORTED` if `instance` has no callback
/// caster
/// @warning Asserts in next cases:
/// - If `instance` is null.
/// - If `trace.trace` is null.
[[nodiscard]] ARGUS_API auto create_scene(Instance instance,
                                          const User_Trace& trace) noexcept
    -> Result<Scene>;

/// @brief Releases a scene and the acceleration structure it stores.
/// @details Null is a no-op. Does not clear the caller's `Scene`. Does not
/// release the instance the scene was created on.
/// @param scene Scene to destroy
ARGUS_API void destroy(Scene scene) noexcept;

/// @brief Intersects one ray per element of `rays` with `scene`.
/// @details A scene may be traced from several threads. Do not `destroy` it
/// during those traces. Directions are unit length. On a miss, `hit` is
/// false and the other fields are zero.
/// @param scene Scene to query
/// @param rays Rays to intersect
/// @param hits Output hits, one per ray
/// @return Nothing, or a recoverable `Error` from the backend
/// @warning Asserts in next cases:
/// - If `scene` is null.
/// - If `rays` and `hits` differ in size.
[[nodiscard]] ARGUS_API auto trace(Scene scene, std::span<const Ray> rays,
                                   std::span<Hit> hits) noexcept
    -> Result<void>;

}  // namespace argus::sim
