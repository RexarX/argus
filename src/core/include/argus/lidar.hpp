#pragma once

#include <argus/assert.hpp>
#include <argus/math.hpp>
#include <argus/platform.hpp>
#include <argus/types.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <span>
#include <vector>

namespace argus {

/// @brief Unit directions in the sensor frame.
/// @details Sensor +Z is forward, +Y is up, and +X is right.
struct Pattern {
    std::pmr::vector<float3> directions{std::pmr::get_default_resource()};
};

/// @brief Creates a pattern with provided allocator and optional initial size.
/// @param resource Memory resource to use for allocations
/// @param count Optional initial size of the pattern
/// @return Pattern with provided allocator and optional initial size
[[nodiscard]] inline Pattern create_pattern_with(
    std::pmr::memory_resource* resource, size_t count = 0) noexcept {
    ARGUS_ASSERT(resource != nullptr);
    return {.directions = std::pmr::vector<float3>{count, resource}};
}

/// @brief Read-only view of a pattern.
struct Pattern_View {
    std::span<const float3> directions;
};

/// @brief Creates a read-only view of a pattern.
/// @param pattern `Pattern` to view
/// @return Read-only view of `pattern`
[[nodiscard]] inline Pattern_View create_view(const Pattern& pattern) noexcept {
    return {.directions = pattern.directions};
}

/// @brief One lidar.
/// @details `pattern` holds unit directions in the sensor frame. `scale`
/// multiplies the angle between a pattern direction and sensor +Z; `1`
/// leaves the pattern unchanged. `distance` is the maximum accepted path
/// length, in meters. Angles in `orientation` are radians.
struct Lidar {
    Pattern_View pattern;
    float3 position = float3::Zero();
    Orientation orientation;
    float scale = 1.0F;
    float distance = 0.0F;
};

/// @brief Writes a uniform angular grid in the sensor frame.
/// @details Column 0 is the left edge (`-fov / 2`) and row 0 is the bottom
/// edge. `fov` and `vertical_fov` are full angles in radians. A zero
/// `vertical_fov` is one scan plane and requires `rows == 1`.
/// @param fov Full horizontal angle
/// @param vertical_fov Full vertical angle
/// @param columns Azimuth samples. At least 1
/// @param rows Elevation samples. At least 1
/// @param pattern Output directions, `columns * rows` long
/// @warning Asserts in next cases:
/// - If `pattern` is null.
/// - If `columns` or `rows` is 0.
/// - If `vertical_fov == 0` and `rows != 1`.
/// - If `columns * rows` overflows.
/// - If `pattern` size is not `columns * rows`.
ARGUS_API void fill_uniform_pattern(float fov, float vertical_fov,
                                    uint32_t columns, uint32_t rows,
                                    Pattern* pattern) noexcept;

/// @brief World direction of one pattern sample.
/// @details Applies `lidar.scale`, then `create_rotation_matrix` of the lidar
/// pose.
/// @param lidar Sensor whose pattern and pose to read
/// @param index Sample index in `lidar.pattern`
/// @return Unit world direction
/// @warning Asserts in next cases:
/// - If `index` is outside `lidar.pattern`.
/// - If the pattern sample is not unit length.
[[nodiscard]] inline float3 get_ray_direction(const Lidar& lidar,
                                              size_t index) noexcept {
    ARGUS_ASSERT(index < lidar.pattern.directions.size());
    const float3& sample = lidar.pattern.directions[index];
    ARGUS_ASSERT(std::abs(sample.squaredNorm() - 1.0F) <= 1.0e-4F);

    const float3x3 frame = create_rotation_matrix(
        lidar.orientation.yaw, lidar.orientation.pitch, lidar.orientation.roll);
    return frame * scale_angle(sample, lidar.scale);
}

}  // namespace argus
