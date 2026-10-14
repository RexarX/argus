#ifndef ARGUS_VERSION_H
#define ARGUS_VERSION_H

#include <argus/platform.h>
#include <argus/version_macros.h>

#include <stdbool.h>
#include <stdint.h>

ARGUS_C_BEGIN

/// @brief Semantic library / header version.
typedef struct ArgusVersion {
    uint8_t major;
    uint8_t minor;
    uint16_t patch;
} ArgusVersion;

/// @brief Gets the version of the headers visible to this translation unit.
/// @return `ArgusVersion` with `major`, `minor`, and `patch`
static inline ArgusVersion argus_header_version(void) {
    const ArgusVersion version = {
        .major = ARGUS_VERSION_MAJOR,
        .minor = ARGUS_VERSION_MINOR,
        .patch = ARGUS_VERSION_PATCH,
    };
    return version;
}

/// @brief Gets the version baked into the linked argus binary.
/// @return Linked binary `major`, `minor`, and `patch`
ARGUS_API ArgusVersion argus_linked_version(void) ARGUS_C_NOEXCEPT;

/// @brief Gets `"major.minor.patch"` for `argus_linked_version()`.
/// @return Null-terminated version string
ARGUS_API const char* argus_version_string(void) ARGUS_C_NOEXCEPT;

/// @brief Whether a binary at `linked` can satisfy headers at `header`.
/// @param header Version of the headers the caller compiled against
/// @param linked Version of the argus binary being used
/// @return `true` if majors match and `linked.minor >= header.minor`
static inline bool argus_versions_compatible(ArgusVersion header,
                                             ArgusVersion linked) {
    return linked.major == header.major && linked.minor >= header.minor;
}

/// @brief Checks header compatibility against the linked binary.
/// @return `true` if the linked binary matches these headers
static inline bool argus_compatible_with_headers(void) {
    return argus_versions_compatible(argus_header_version(),
                                     argus_linked_version());
}

ARGUS_C_END

#endif
