#pragma once

#include <argus/error.hpp>
#include <argus/lidar.hpp>
#include <argus/mirror.hpp>
#include <argus/platform.hpp>
#include <argus/sim/scene.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace argus::sim {

/// @brief Number of rays `cast` writes for `lidars`.
/// @details Each lidar contributes `pattern.directions.size()` rays, in
/// order. Mirrors contribute none.
/// @param lidars Sensors to count
/// @return Sum of the lidar pattern sizes
/// @warning Asserts if the sum overflows.
[[nodiscard]] ARGUS_API auto get_ray_count(
    std::span<const Lidar> lidars) noexcept -> size_t;

/// @brief Casts every lidar and reflects off every mirror.
/// @details `hits` is one hit per pattern sample, lidars concatenated in
/// span order. A lidar contributes `pattern.directions.size()` rays.
/// Directions come from the lidar pose (`create_rotation_matrix`) and
/// `scale_angle`, computed once per lidar. Mirrors are analytic quads, not
/// scene triangles.
/// A mirror closer than the scene is kept. Its front face reflects, up to
/// `max_bounces`, and the reflected ray's origin is offset along the bounce
/// so the quad does not hit itself. The back face occludes: the ray stops
/// and `hit` is false. `distance` is the total path length, so the range
/// budget is shared across bounces. `mirror` on a hit is the index in
/// `mirrors` of the last mirror bounced off when `bounces > 0`. `facing` is
/// the cosine of incidence on a scene hit.
/// @param scene Scene to query
/// @param lidars Sensors to cast. Empty skips the cast
/// @param mirrors Analytic quads. Empty skips reflection
/// @param max_bounces Maximum reflections kept for one ray
/// @param hits Output hits, one per lidar ray
/// @return Nothing, or a recoverable `Error` from the backend
/// @warning Asserts in next cases:
/// - If `scene` is null.
/// - If `mirrors` does not fit in `uint16_t`.
/// - If `hits` size is not `ray_count(lidars)`.
/// - If a pattern sample is not unit length.
[[nodiscard]] ARGUS_API auto cast(Scene scene, std::span<const Lidar> lidars,
                                  std::span<const Mirror> mirrors,
                                  uint8_t max_bounces,
                                  std::span<Hit> hits) noexcept -> Result<void>;

}  // namespace argus::sim
