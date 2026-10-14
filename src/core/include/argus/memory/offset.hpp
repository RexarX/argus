#pragma once

#include <argus/platform.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

namespace argus::mem {

/// Top-level bins in the two-level size map.
inline constexpr uint32_t OFFSET_TOP_BIN_COUNT = 32;
/// Leaf bins under each top-level bin.
inline constexpr uint32_t OFFSET_BINS_PER_LEAF = 8;
/// Shift from a leaf bin index to its top-level bin.
inline constexpr uint32_t OFFSET_TOP_BIN_SHIFT = 3;
/// Mask of the leaf index inside one top-level bin.
inline constexpr uint32_t OFFSET_LEAF_BIN_MASK = 0x7;
/// Total leaf bins.
inline constexpr uint32_t OFFSET_LEAF_BIN_COUNT =
    OFFSET_TOP_BIN_COUNT * OFFSET_BINS_PER_LEAF;
/// Default node pool size for `init`.
inline constexpr uint32_t OFFSET_DEFAULT_MAX_ALLOCS = 128U * 1024U;
/// Sentinel offset and metadata for a failed allocation.
inline constexpr uint32_t OFFSET_NO_SPACE = 0xFFFFFFFFU;

/// @brief One contiguous range inside the managed storage.
struct Offset_Allocation {
    uint32_t offset = OFFSET_NO_SPACE;
    /// Internal node index. Needed to free.
    uint32_t metadata = OFFSET_NO_SPACE;

    [[nodiscard]] friend constexpr bool operator==(
        const Offset_Allocation&, const Offset_Allocation&) noexcept = default;
    [[nodiscard]] friend constexpr bool operator!=(
        const Offset_Allocation&, const Offset_Allocation&) noexcept = default;

    /// @brief Checks if this is a live allocation.
    /// @return `true` if this is a live allocation, `false` otherwise
    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return offset != OFFSET_NO_SPACE;
    }
};

/// @brief Coarse free-space query.
struct Offset_Report {
    uint32_t free_bytes = 0;
    uint32_t largest_free_region = 0;
};

/// @brief Per-bin free-region counts.
struct Offset_Report_Full {
    struct Region {
        uint32_t size = 0;
        uint32_t count = 0;
    };

    std::array<Region, OFFSET_LEAF_BIN_COUNT> free_regions = {};
};

/// @brief O(1) offset allocator over a `[0, size)` range.
/// @details TLSF-style 256 bins (3-bit mantissa, 5-bit exponent). Metadata
/// lives outside the data range, so the allocator returns offsets, not
/// pointers. Based on Sebastian Aaltonen's OffsetAllocator (MIT, 2023).
///
/// Treat fields as internal and use the free functions. Initialize with
/// `init`, release with `destroy`.
/// @warning Not thread-safe.
struct Offset {
    std::byte* storage = nullptr;
    uint32_t* free_nodes = nullptr;
    std::array<uint32_t, OFFSET_LEAF_BIN_COUNT> bin_indices = {};
    uint32_t size = 0;
    uint32_t max_allocs = 0;
    uint32_t free_bytes = 0;
    uint32_t used_bins_top = 0;
    uint32_t free_offset = 0;
    uint32_t used_count = 0;
    std::array<uint8_t, OFFSET_TOP_BIN_COUNT> used_bins = {};
};

/// @brief Allocates the node pool and installs one free region of `size`.
/// @param offset Allocator to initialize
/// @param size Byte count of the managed range. Must be non-zero
/// @param max_allocs Node pool size (live allocations plus free fragments)
/// @warning Asserts in next cases:
/// - If `offset` is null.
/// - If `offset` is already initialized.
/// - If `size` or `max_allocs` is 0.
/// - If the node pool cannot be allocated.
ARGUS_API void init(Offset* offset, uint32_t size,
                    uint32_t max_allocs = OFFSET_DEFAULT_MAX_ALLOCS) noexcept;

/// @brief Frees the node pool. The allocator can be initialized again.
/// @param offset Allocator to destroy
/// @warning Asserts if `offset` is null.
ARGUS_API void destroy(Offset* offset) noexcept;

/// @brief Drops every allocation and restores a single free region.
/// @param offset Allocator to reset
/// @warning Asserts if `offset` is null or not initialized.
ARGUS_API void reset(Offset* offset) noexcept;

/// @brief Allocates `size` contiguous bytes.
/// @param offset Allocator to allocate from
/// @param size Byte count. Must be non-zero
/// @return Offset and metadata, or a false `Offset_Allocation` if out of space
/// @warning Asserts in next cases:
/// - If `offset` is null or not initialized.
/// - If `size` is 0.
[[nodiscard]] ARGUS_API Offset_Allocation allocate(Offset* offset,
                                                   uint32_t size) noexcept;

/// @brief Returns `allocation` to the pool and coalesces neighbors.
/// @param offset Allocator that created `allocation`
/// @param allocation Result of `allocate`
/// @warning Asserts in next cases:
/// - If `offset` is null or not initialized.
/// - If `allocation` is not a live allocation from this allocator.
ARGUS_API void free(Offset* offset, Offset_Allocation allocation) noexcept;

/// @brief Byte count of `allocation`.
/// @param offset Allocator that created `allocation`
/// @param allocation Result of `allocate`
/// @return Size passed to `allocate`, or 0 if `allocation` is empty
[[nodiscard]] ARGUS_API uint32_t get_allocation_size(
    const Offset& offset, Offset_Allocation allocation) noexcept;

/// @brief Checks for live allocations.
/// @param offset Allocator to query
/// @return `true` if every byte is free, `false` otherwise
[[nodiscard]] inline bool is_empty(const Offset& offset) noexcept {
    return offset.used_count == 0;
}

/// @brief Gets free bytes and a lower bound on the largest free region.
/// @param offset Allocator to query
/// @return Storage report
[[nodiscard]] ARGUS_API Offset_Report get_report(const Offset& offset) noexcept;

/// @brief Gets per-bin free-region size and count.
/// @param offset Allocator to query
/// @return Full storage report
[[nodiscard]] ARGUS_API auto get_full_report(const Offset& offset) noexcept
    -> Offset_Report_Full;

/// @brief Gets the managed range size in bytes.
/// @param offset Allocator to query
/// @return Managed range size in bytes
[[nodiscard]] inline uint32_t get_size(const Offset& offset) noexcept {
    return offset.size;
}

/// @brief Gets currently free bytes. The free space may be fragmented.
/// @param offset Allocator to query
/// @return Currently free bytes
[[nodiscard]] inline uint32_t get_free_bytes(const Offset& offset) noexcept {
    return offset.free_bytes;
}

}  // namespace argus::mem
