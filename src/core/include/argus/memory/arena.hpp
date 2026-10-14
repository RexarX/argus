#pragma once

#include <argus/assert.hpp>
#include <argus/memory/common.hpp>
#include <argus/platform.hpp>

#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <source_location>

namespace argus::mem {

/// Alignment of every block start and the default allocation alignment.
/// Allocations up to this alignment never pad at a block start.
inline constexpr size_t ARENA_DEFAULT_ALIGN = 16;

/// Underused `reset` cycles in a row before an oversized block is shrunk.
inline constexpr size_t ARENA_SHRINK_AFTER_RESETS = 64;

/// @brief Configuration for an `Arena`.
struct Arena_Options {
    /// Capacity of the first block in bytes. Must be non-zero.
    size_t initial_capacity = 64U * 1024U;
    /// How new blocks grow when the current one is full. A single oversized
    /// request still gets a block that fits it, even past `max_capacity`. A
    /// policy that cannot grow (see `next_capacity`) makes the arena
    /// fixed-capacity: allocation fails once the first block is full.
    Growth_Policy growth =
        create_geometric_growth_policy(2, 1, 64U * 1024U * 1024U);
    /// Fill rewound memory with a pattern on `reset` / `rewind`. On in debug
    /// builds only by default.
#ifndef NDEBUG
    bool poison_on_reset = true;
#else
    bool poison_on_reset = false;
#endif
};

/// Arena block.
struct alignas(ARENA_DEFAULT_ALIGN) Arena_Block {
    Arena_Block* next = nullptr;
    size_t capacity = 0;
};

/// @brief rewind point captured by `get_mark`.
struct Arena_Mark {
    Arena_Block* block = nullptr;
    std::byte* ptr = nullptr;
    size_t retired_bytes = 0;
};

/// @brief Usage and capacity snapshot.
struct Arena_Stats {
    /// Bytes handed out since the last reset, including alignment padding.
    size_t used = 0;
    /// Largest `used` seen at the end of any cycle or rewind, lifetime-wide.
    size_t high_water = 0;
    /// Total bytes of all owned blocks, spares included.
    size_t capacity = 0;
    /// Number of owned blocks, spares included.
    size_t block_count = 0;
};

struct Arena;

/// @brief `std::pmr::memory_resource` proxy over one `Arena`.
/// @details Allocation forwards to `allocate`; deallocation is a no-op. It is a
/// member of `Arena`, get it with `get_resource`.
/// @warning Not thread-safe. Only the thread that owns the arena may use it.
struct Arena_Resource final : public std::pmr::memory_resource {
    Arena* arena = nullptr;

private:
    [[nodiscard]] ARGUS_API void* do_allocate(size_t bytes,
                                              size_t alignment) override;
    void do_deallocate(void* /*ptr*/, size_t /*bytes*/,
                       size_t /*alignment*/) override {}
    [[nodiscard]] bool do_is_equal(
        const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

/// @brief Single-threaded bump arena over a chain of blocks.
/// @details Allocations are never freed individually and no destructors run on
/// reset, so use it for trivially destructible or scope-bound data. `reset`
/// retains memory: if the previous cycle overflowed the first block, all blocks
/// are replaced by one block sized to the cycle's peak, so steady state is zero
/// mallocs per cycle. Persistent over-capacity is shrunk back after
/// `ARENA_SHRINK_AFTER_RESETS` underused cycles. `get_mark` / `rewind` give
/// nested scoped rewind, and blocks dropped by a rewind are kept as spares and
/// reused before malloc.
///
/// Treat fields as internal and use the free functions. Initialize with `init`,
/// release with `destroy`.
/// @warning Not thread-safe, not copyable or movable (`resource` points back at
/// the arena).
struct Arena {
    Arena_Options options;

    Arena_Block* head = nullptr;
    Arena_Block* spare = nullptr;
    std::byte* ptr = nullptr;
    std::byte* end = nullptr;
    /// Bytes used in blocks older than `head`.
    size_t retired_bytes = 0;
    size_t cycle_peak = 0;
    size_t high_water = 0;
    size_t capacity_total = 0;
    size_t block_count = 0;
    size_t underused_resets = 0;
    std::source_location last_reset_location = std::source_location::current();
};

/// @brief Initializes `arena` and allocates its first block.
/// @param arena Arena to initialize
/// @param options Arena configuration
/// @warning Asserts in next cases:
/// - If `arena` is null.
/// - If `arena` is already initialized.
/// - If `options.initial_capacity` is 0.
/// - If `options.growth.max_capacity` is below `options.initial_capacity`.
/// - If the first block cannot be allocated.
ARGUS_API void init(Arena* arena, const Arena_Options& options = {}) noexcept;

/// @brief Frees every block of `arena`. The arena can be initialized again.
/// @param arena Arena to destroy
/// @warning Asserts if `arena` is null. Invalidates all its allocations.
ARGUS_API void destroy(Arena* arena) noexcept;

/// @brief Rewinds everything and applies the retention policy.
/// @details If more than one block was needed, replaces them with one block
/// sized to this cycle's peak. If one block stayed far larger than needed for
/// `ARENA_SHRINK_AFTER_RESETS` cycles, shrinks it.
/// @param arena Arena to reset
/// @param location Recorded as the last reset location
/// @warning Asserts if `arena` is null.
ARGUS_API void reset(
    Arena* arena,
    std::source_location location = std::source_location::current()) noexcept;

/// @brief Rewinds to `mark`, invalidating everything allocated after it.
/// @details Blocks added after the mark are kept as spares for reuse.
/// @param arena Arena to rewind
/// @param mark Result of `get_mark` on this arena
/// @param location Recorded as the last reset location
/// @warning Asserts in next cases:
/// - If `arena` is null.
/// - If `mark` was not taken from this arena.
/// - If `mark` is already rewound past (marks must be used LIFO).
ARGUS_API void rewind(
    Arena* arena, const Arena_Mark& mark,
    std::source_location location = std::source_location::current()) noexcept;

/// @brief Bump-allocates `size` bytes from `arena`.
/// @param arena Arena to allocate from
/// @param size Byte count. Zero is allowed
/// @param align Alignment in bytes. Power of two
/// @return Pointer to the memory, or null if the system is out of memory or the
/// growth policy forbids a new block
/// @warning Asserts in next cases:
/// - If `arena` is null.
/// - If `align` is not a power of two.
[[nodiscard]] ARGUS_API void* allocate(
    Arena* arena, size_t size, size_t align = ARENA_DEFAULT_ALIGN) noexcept;

/// @brief Bump-allocates uninitialized storage for `count` objects of `T`.
/// @tparam T Element type
/// @param arena Arena to allocate from
/// @param count Element count
/// @return Pointer to the first element, or null on overflow / out of memory
/// @warning Asserts if `arena` is null.
template <typename T>
[[nodiscard]] inline T* allocate_array(Arena* arena,
                                       size_t count = 1) noexcept {
    if (count > SIZE_MAX / sizeof(T)) [[unlikely]] {
        return nullptr;
    }
    return static_cast<T*>(allocate(arena, count * sizeof(T), alignof(T)));
}

/// @brief Captures the current position for a later `rewind`.
/// @param arena Arena to mark
/// @return rewind point
[[nodiscard]] inline Arena_Mark get_mark(const Arena& arena) noexcept {
    return {.block = arena.head,
            .ptr = arena.ptr,
            .retired_bytes = arena.retired_bytes};
}

/// @brief Gets bytes handed out since the last reset.
/// @param arena Arena to query
/// @return Used bytes, alignment padding included
[[nodiscard]] ARGUS_API size_t get_used_bytes(const Arena& arena) noexcept;

/// @brief Gets a usage and capacity snapshot.
/// @param arena Arena to query
/// @return Stats
[[nodiscard]] ARGUS_API Arena_Stats get_stats(const Arena& arena) noexcept;

/// @brief Gets the PMR proxy over `arena`.
/// @param arena Arena to proxy
/// @return Arena memory resource valid for the arena's lifetime
/// @warning Asserts if `arena` is null.
[[nodiscard]] inline Arena_Resource get_resource(Arena* arena) noexcept {
    ARGUS_ASSERT(arena != nullptr);
    Arena_Resource resource;
    resource.arena = arena;
    return resource;
}

/// @brief Rewinds an arena to its construction-time position on scope exit.
/// @code
///   {
///     argus::mem::Arena_Scope scope{&arena};
///     void* scratch = argus::mem::allocate(&arena, 128);
///     ...
///   }  // scratch is gone here
/// @endcode
struct Arena_Scope {
    /// @brief Marks `target`.
    /// @param target Arena to rewind on exit
    /// @param loc Recorded as the last reset location on exit
    /// @warning Asserts if `target` is null.
    explicit Arena_Scope(
        Arena* target,
        std::source_location loc = std::source_location::current()) noexcept
        : arena(target), location(loc) {
        ARGUS_ASSERT(target != nullptr);
        mark = get_mark(*target);
    }
    Arena_Scope(const Arena_Scope&) = delete;
    Arena_Scope(Arena_Scope&&) = delete;
    ~Arena_Scope() noexcept { rewind(arena, mark, location); }

    Arena_Scope& operator=(const Arena_Scope&) = delete;
    Arena_Scope& operator=(Arena_Scope&&) = delete;

    Arena* const arena = nullptr;
    Arena_Mark mark;
    const std::source_location location;
};

}  // namespace argus::mem
