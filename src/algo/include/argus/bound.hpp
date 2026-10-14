#pragma once

namespace argus {

/// @brief Inclusive range of one search dimension.
struct Bound {
    float min = 0.0F;
    float max = 0.0F;

    [[nodiscard]] friend constexpr bool operator==(
        const Bound&, const Bound&) noexcept = default;
    [[nodiscard]] friend constexpr bool operator!=(
        const Bound&, const Bound&) noexcept = default;
};

}  // namespace argus
