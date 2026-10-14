#pragma once

#include <argus/embree/instance.hpp>
#include <argus/error.hpp>
#include <argus/platform.hpp>
#include <argus/types.hpp>

#include <span>

struct RTCSceneTy;

namespace argus::sim::embree {

/// @brief Embree scene.
/// @details First field is the function table the portable `sim::Scene`
/// reads. Owns `rtc_scene`. The Embree context stays on the instance. One
/// instance can create many scenes. Triangle data lives in the Embree
/// buffers, not beside them.
struct Scene {
    const Backend_VTable* table = nullptr;
    RTCSceneTy* rtc_scene = nullptr;
};

/// @brief Creates a scene of `mesh` on `instance`.
/// @details Copies `mesh` into Embree buffers. Empty geometry is valid.
/// Each call creates another scene. The instance does not store them.
/// @param instance Instance to create the scene on
/// @param mesh Indexed triangle mesh to commit
/// @return Owning scene, or `Error::OUT_OF_MEMORY` / `Error::INVALID`.
/// Release it with `destroy`.
/// @warning Asserts in next cases:
/// - If `instance` is null.
/// - If `instance->rtc` is null.
[[nodiscard]] ARGUS_API auto create_scene(Instance* instance,
                                          const Mesh& mesh) noexcept
    -> Result<Scene*>;

/// @brief Releases `scene` and the acceleration structure it stores.
/// @param scene Scene to destroy. Null is a no-op
ARGUS_API void destroy(Scene* scene) noexcept;

/// @brief Intersects rays with `scene`.
/// @param scene Scene to query
/// @param rays Rays to intersect. Directions are unit length
/// @param hits Output hits, one per ray
/// @warning Asserts in next cases:
/// - If `scene` is null.
/// - If `scene->rtc_scene` is null.
/// - If `rays` and `hits` differ in size.
[[nodiscard]] ARGUS_API auto trace(Scene* scene, std::span<const Ray> rays,
                                   std::span<Hit> hits) noexcept
    -> Result<void>;

}  // namespace argus::sim::embree
