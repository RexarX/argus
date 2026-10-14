#pragma once

#include <argus/bound.hpp>
#include <argus/error.hpp>
#include <argus/lidar.hpp>
#include <argus/mirror.hpp>
#include <argus/platform.hpp>

#include <cstdint>
#include <span>
#include <string_view>

namespace argus {

/// @brief Scalar written by one `Target`.
/// @details `X`, `Y`, and `Z` are the position components. YAW rotates about
/// world +Y, pitch rises toward +Y, and roll twists around the forward axis.
/// YAW 0 and pitch 0 is forward +Z. ROLL 0 keeps a lidar's up axis at world
/// +Y, and a mirror's width axis at world +X, when the forward axis is +Z.
/// Angles are radians. PITCH near +- π/2 is singular; keep pitch bounds inside
/// that open range. `SCALE` is the lidar pattern scale. `RANGE` is the
/// maximum path length. `WIDTH` and `HEIGHT` are the mirror extents.
enum class Variable : uint8_t {
    X,
    Y,
    Z,
    YAW,
    PITCH,
    ROLL,
    SCALE,
    RANGE,
    WIDTH,
    HEIGHT,
};

/// @brief Name of a `Variable` enumerator.
/// @param variable Enumerator to convert
/// @return Enumerator name, or `"Unknown"`
[[nodiscard]] constexpr std::string_view to_string(Variable variable) noexcept {
    switch (variable) {
        using enum Variable;
        case X:
            return "X";
        case Y:
            return "Y";
        case Z:
            return "Z";
        case YAW:
            return "YAW";
        case PITCH:
            return "PITCH";
        case ROLL:
            return "ROLL";
        case SCALE:
            return "SCALE";
        case RANGE:
            return "RANGE";
        case WIDTH:
            return "WIDTH";
        case HEIGHT:
            return "HEIGHT";
    }
    return "Unknown";
}

/// @brief Which object span a `Target` indexes.
enum class Subject : uint8_t { LIDAR, MIRROR };

/// @brief One dimension of the flat parameter vector.
/// @details `params[i]` belongs to `targets[i]`. A field with no record stays
/// at its current value. `bounded == false` ignores `min` and `max`; `apply`
/// still writes the value. `subject` selects the lidar or mirror span, and
/// `index` is the element in that span. Orientation is yaw, pitch, and roll
/// stored on the object, so the frame is not decomposed on each write.
/// `SCALE` and `RANGE` belong to a lidar. `WIDTH` and `HEIGHT` belong to a
/// mirror.
///
/// @code
/// Target limits[] = {
///     {.min = 0.0F, .max = 4.0F, .index = 0, .subject = Subject::LIDAR,
///      .variable = Variable::X, .bounded = true},
///     {.index = 0, .subject = Subject::MIRROR, .variable = Variable::YAW},
/// };
/// @endcode
struct Target {
    float min = 0.0F;
    float max = 0.0F;
    uint32_t index = 0;
    Subject subject = Subject::LIDAR;
    Variable variable = Variable::X;
    bool bounded = false;

    [[nodiscard]] friend constexpr bool operator==(
        const Target&, const Target&) noexcept = default;
    [[nodiscard]] friend constexpr bool operator!=(
        const Target&, const Target&) noexcept = default;
};

/// @brief Checks that `targets` can drive `lidars` and `mirrors`.
/// @details Rejects an index past the end of the selected span, a variable
/// that does not belong to `subject`, a repeated `(subject, index,
/// variable)`, an unknown variable, and, when `bounded`, `min > max` or a
/// non-positive minimum on `SCALE`, `RANGE`, `WIDTH`, or `HEIGHT`.
/// @param targets Parameterization, one record per dimension
/// @param lidars Lidars the records may index
/// @param mirrors Mirrors the records may index
/// @return Nothing, or `Error::INVALID` when a record is unusable
[[nodiscard]] ARGUS_API auto validate(std::span<const Target> targets,
                                      std::span<const Lidar> lidars,
                                      std::span<const Mirror> mirrors) noexcept
    -> Result<void>;

/// @brief Copies each bounded target range into `bounds`.
/// @param targets Parameterization, one record per dimension
/// @param bounds Output ranges, one per target
/// @warning Asserts in next cases:
/// - If the spans differ in size.
/// - If a target is not bounded.
/// - If a bounded target has `min > max`.
ARGUS_API void copy_bounds(std::span<const Target> targets,
                           std::span<Bound> bounds) noexcept;

/// @brief Reads the current lidar and mirror values selected by `targets`.
/// @param targets Parameterization, one record per dimension
/// @param lidars Lidars the records index
/// @param mirrors Mirrors the records index
/// @param params Output values, one per target
/// @warning Asserts in next cases:
/// - If `params` and `targets` differ in size.
/// - If a record indexes outside its span.
/// - If `variable` does not belong to `subject`.
ARGUS_API void read(std::span<const Target> targets,
                    std::span<const Lidar> lidars,
                    std::span<const Mirror> mirrors,
                    std::span<float> params) noexcept;

/// @brief Writes `params` into the lidars and mirrors selected by `targets`.
/// @param targets Parameterization. `params[i]` maps to `targets[i]`
/// @param lidars Lidars updated in place
/// @param mirrors Mirrors updated in place
/// @param params Values, one per target
/// @warning Asserts in next cases:
/// - If `params` and `targets` differ in size.
/// - If a record indexes outside its span.
/// - If `variable` does not belong to `subject`.
ARGUS_API void apply(std::span<const Target> targets, std::span<Lidar> lidars,
                     std::span<Mirror> mirrors,
                     std::span<const float> params) noexcept;

}  // namespace argus
