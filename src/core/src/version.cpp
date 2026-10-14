#include <pch.hpp>

#include <argus/version.hpp>

namespace argus {

Version linked_version() noexcept {
    return header_version();
}

const char* version_string() noexcept {
    return ARGUS_VERSION_STRING;
}

}  // namespace argus
