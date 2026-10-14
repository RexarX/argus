#pragma once

#include <memory_resource>

namespace argus {

/// @brief Settings for algorithm allocations that outlive the call.
/// @details Scratch that dies before the function returns comes from temporary
/// storage, not from `allocator`.
struct Context {
    /// Resource for returned containers. Must be non-null.
    std::pmr::memory_resource* allocator = std::pmr::get_default_resource();
};

}  // namespace argus
