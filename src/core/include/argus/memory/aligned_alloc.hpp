#pragma once

#include <argus/platform.hpp>

#include <cstddef>

namespace argus::mem {

/// @brief Allocates memory with the specified alignment.
/// @details Uses mimalloc when `ARGUS_USE_MIMALLOC` is defined,
/// `_aligned_malloc` on MSVC, and `std::aligned_alloc` on other platforms. On
/// POSIX, `std::aligned_alloc` requires alignment >= `sizeof(void*)`; smaller
/// alignments are satisfied via `malloc`, which provides at least
/// `alignof(std::max_align_t)` alignment.
/// @param alignment Alignment in bytes. Power of two
/// @param size Block size in bytes. Must be non-zero
/// @return Pointer to the allocated block, or null on failure
/// @warning Asserts in next cases:
/// - If `alignment` is zero or not a power of two.
/// - If `size` is zero.
[[nodiscard]] ARGUS_API void* aligned_alloc(size_t alignment,
                                            size_t size) noexcept;

/// @brief Frees memory allocated with `aligned_alloc`.
/// @details Uses the backend paired with `aligned_alloc`. Passing null is a
/// no-op.
/// @param ptr Block to free
ARGUS_API void aligned_free(void* ptr) noexcept;

}  // namespace argus::mem
