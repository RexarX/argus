#include <pch.hpp>

#include <argus/version.h>

#include <argus/version.hpp>

extern "C" {

ArgusVersion argus_linked_version(void) noexcept {
    const argus::Version version = argus::linked_version();
    const ArgusVersion result = {
        .major = version.major,
        .minor = version.minor,
        .patch = version.patch,
    };
    return result;
}

const char* argus_version_string(void) noexcept {
    return argus::version_string();
}
}
