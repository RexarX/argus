#pragma once

#include <argus/version_macros.h>
#include <argus/platform.hpp>

#include <compare>
#include <cstdint>

namespace argus {

/// @brief Semantic library / header version.
/// @details `major` and `minor` are the compatibility key. `patch` is ignored
/// by `compatible`.
struct Version {
    uint8_t major = 0;
    uint8_t minor = 0;
    uint16_t patch = 0;

    [[nodiscard]] friend constexpr bool operator==(
        const Version&, const Version&) noexcept = default;
    [[nodiscard]] friend constexpr bool operator!=(
        const Version&, const Version&) noexcept = default;
    [[nodiscard]] friend constexpr auto operator<=>(
        const Version&, const Version&) noexcept = default;
};

/// @brief Gets the version of the headers visible to this translation unit.
/// @return `Version` with `major`, `minor`, and `patch`
[[nodiscard]] constexpr Version header_version() noexcept {
    return {
        .major = ARGUS_VERSION_MAJOR,
        .minor = ARGUS_VERSION_MINOR,
        .patch = ARGUS_VERSION_PATCH,
    };
}

/// @brief Gets the version baked into the linked argus binary.
/// @return `Version` of the linked binary
[[nodiscard]] ARGUS_API Version linked_version() noexcept;

/// @brief Gets `"major.minor.patch"` for `linked_version()`.
/// @return Null-terminated version string
[[nodiscard]] ARGUS_API const char* version_string() noexcept;

/// @brief Whether a binary at `linked` can satisfy headers at `header`.
/// @param header Version of the headers the caller compiled against
/// @param linked Version of the argus binary being used
/// @return `true` if majors match and `linked.minor >= header.minor`
[[nodiscard]] constexpr bool compatible(Version header,
                                        Version linked) noexcept {
    return linked.major == header.major && linked.minor >= header.minor;
}

/// @brief Checks `compatible(header_version(), linked_version())`.
/// @return `true` if the linked binary matches these headers
[[nodiscard]] inline bool compatible_with_headers() noexcept {
    return compatible(header_version(), linked_version());
}

}  // namespace argus
