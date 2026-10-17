#include <pch.hpp>

#include <argus/lidar.hpp>

#include <argus/assert.hpp>
#include <argus/math.hpp>

#include <cstddef>
#include <cstdint>

namespace argus {

void fill_uniform_pattern(float fov, float vertical_fov, uint32_t columns,
                          uint32_t rows, Pattern* pattern) noexcept {
    ARGUS_ASSERT(pattern != nullptr);
    ARGUS_ASSERT(columns > 0);
    ARGUS_ASSERT(rows > 0);
    if (vertical_fov == 0.0F) {
        ARGUS_ASSERT(rows == 1);
    }

    const auto column_count = static_cast<size_t>(columns);
    const auto row_count = static_cast<size_t>(rows);
    ARGUS_ASSERT(column_count <= SIZE_MAX / row_count);
    ARGUS_ASSERT(pattern->directions.size() == column_count * row_count);

    const float yaw_den = columns == 1 ? 1.0F : static_cast<float>(columns - 1);
    const float pitch_den = rows == 1 ? 1.0F : static_cast<float>(rows - 1);

    for (uint32_t row = 0; row < rows; ++row) {
        const float pitch =
            rows == 1
                ? 0.0F
                : (-vertical_fov * 0.5F) +
                      (vertical_fov * static_cast<float>(row) / pitch_den);
        for (uint32_t column = 0; column < columns; ++column) {
            const float yaw =
                columns == 1 ? 0.0F
                             : (-fov * 0.5F) +
                                   (fov * static_cast<float>(column) / yaw_den);
            float3& direction =
                pattern->directions[(static_cast<size_t>(row) * column_count) +
                                    column];
            direction = get_direction(yaw, pitch);
        }
    }
}

}  // namespace argus
