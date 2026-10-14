#pragma once

#include <argus/math.hpp>
#include <argus/types.hpp>

namespace argus {

/// @brief WIDTH and height of a rectangular mirror, in meters.
struct Extent {
    float width = 0.0F;
    float height = 0.0F;
};

/// @brief One rectangular mirror.
/// @details `position` is the center. `orientation` builds the frame
/// (`rotation`): column 0 is the in-plane width axis, column 1 is the height
/// axis, and column 2 is the normal. Angles are radians.
struct Mirror {
    float3 position = float3::Zero();
    Orientation orientation;
    Extent extent;
};

}  // namespace argus
