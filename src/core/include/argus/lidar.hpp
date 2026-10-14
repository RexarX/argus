#pragma once

#include <argus/assert.hpp>
#include <argus/math.hpp>
#include <argus/platform.hpp>
#include <argus/types.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

namespace argus {

/// @brief Unit directions in the sensor frame.
/// @details The span is a view. The samples live with the caller. Sensor +Z
/// is forward, +Y is up, and +X is right.
struct Pattern {
    std::span<const float3> directions;
};

/// @brief One lidar.
/// @details `pattern` holds unit directions in the sensor frame. `scale`
/// multiplies the angle between a pattern direction and sensor +Z; `1`
/// leaves the pattern unchanged. `distance` is the maximum accepted path
/// length, in meters. Angles in `orientation` are radians.
struct Lidar {
    Pattern pattern;
    float3 position = float3::Zero();
    Orientation orientation;
    float scale = 1.0F;
    float distance = 0.0F;
};

/// @brief Scales the angle between `local` and sensor +Z.
/// @param local Unit direction in the sensor frame
/// @param scale Factor applied to that angle. `1` leaves `local` unchanged
/// @return Unit direction in the sensor frame
[[nodiscard]] inline float3 scale_angle(float3 local, float scale) noexcept {
    if (scale == 1.0F) {
        return local;
    }
    const float off = std::sqrt(local[0] * local[0] + local[1] * local[1]);
    const float scaled = std::atan2(off, local[2]) * scale;
    const float along = std::cos(scaled);
    const float across = std::sin(scaled);
    if (off <= 1.0e-8F) {
        return {0.0F, 0.0F, local[2] < 0.0F ? -along : along};
    }
    const float factor = across / off;
    return {local[0] * factor, local[1] * factor, along};
}

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
/// - If `columns` or `rows` is 0.
/// - If `vertical_fov == 0` and `rows != 1`.
/// - If `columns * rows` overflows.
/// - If `pattern` size is not `columns * rows`.
ARGUS_API void fill_uniform_pattern(float fov, float vertical_fov,
                                    uint32_t columns, uint32_t rows,
                                    std::span<float3> pattern) noexcept;

/// @brief World direction of one pattern sample.
/// @details Applies `lidar.scale`, then `rotation` of the lidar pose.
/// @param lidar Sensor whose pattern and pose to read
/// @param index Sample index in `lidar.pattern`
/// @return Unit world direction
/// @warning Asserts in next cases:
/// - If `index` is outside `lidar.pattern`.
/// - If the pattern sample is not unit length.
[[nodiscard]] inline float3 ray_direction(const Lidar& lidar,
                                          size_t index) noexcept {
    ARGUS_ASSERT(index < lidar.pattern.directions.size());
    const float3& sample = lidar.pattern.directions[index];
    ARGUS_ASSERT(std::abs(sample.squaredNorm() - 1.0F) <= 1.0e-4F);

    const float3x3 frame = rotation(
        lidar.orientation.yaw, lidar.orientation.pitch, lidar.orientation.roll);
    return frame * scale_angle(sample, lidar.scale);
}

}  // namespace argus
