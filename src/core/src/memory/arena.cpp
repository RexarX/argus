#include <argus/memory/arena.hpp>

#include <argus/memory/aligned_alloc.hpp>
#include <argus/memory/common.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <source_location>

namespace argus::mem {

namespace {

constexpr size_t ROUND_GRANULARITY = 4096;
constexpr size_t SHRINK_FACTOR = 4;

[[nodiscard]] std::byte* data(Arena_Block* block) noexcept {
    return reinterpret_cast<std::byte*>(block) + sizeof(Arena_Block);
}

[[nodiscard]] const std::byte* data(const Arena_Block& block) noexcept {
    return reinterpret_cast<const std::byte*>(&block) + sizeof(Arena_Block);
}

/// Makes `[begin, end)` unreadable: optional fill, then ASan poison.
void scrub(const Arena& arena, std::byte* begin, std::byte* end) noexcept {
    if (begin >= end) {
        return;
    }

    const auto size = static_cast<size_t>(end - begin);
    unpoison_memory(begin, size);
    if (arena.options.poison_on_reset) {
        fill_poison(begin, size);
    }
    poison_memory(begin, size);
}

void set_cursor_to_head(Arena* arena) noexcept {
    arena->ptr = data(arena->head);
    arena->end = arena->ptr + arena->head->capacity;
}

[[nodiscard]] Arena_Block* create_block(Arena* arena,
                                        size_t capacity) noexcept {
    if (capacity > SIZE_MAX - sizeof(Arena_Block)) [[unlikely]] {
        return nullptr;
    }

    void* const memory =
        aligned_alloc(ARENA_DEFAULT_ALIGN, sizeof(Arena_Block) + capacity);
    if (memory == nullptr) [[unlikely]] {
        return nullptr;
    }

    auto* const block =
        ::new (memory) Arena_Block{.next = nullptr, .capacity = capacity};
    ++arena->block_count;
    arena->capacity_total += capacity;
    scrub(*arena, data(block), data(block) + capacity);
    return block;
}

void destroy_block(Arena* arena, Arena_Block* block) noexcept {
    unpoison_memory(data(block), block->capacity);
    --arena->block_count;
    arena->capacity_total -= block->capacity;
    aligned_free(block);
}

void free_chain(Arena* arena, Arena_Block* head) noexcept {
    while (head != nullptr) {
        Arena_Block* const next = head->next;
        destroy_block(arena, head);
        head = next;
    }
}

[[nodiscard]] Arena_Block* take_spare(Arena* arena, size_t needed) noexcept {
    // The top spare is the smallest. If it is too small, drop it and look at
    // the next one.
    while (arena->spare != nullptr && arena->spare->capacity < needed) {
        Arena_Block* const block = arena->spare;
        arena->spare = block->next;
        destroy_block(arena, block);
    }

    Arena_Block* const block = arena->spare;
    if (block != nullptr) {
        arena->spare = block->next;
        block->next = nullptr;
    }
    return block;
}

/// Capacity for a new block that must hold at least `needed` bytes, or 0 if
/// the growth policy forbids growing.
[[nodiscard]] size_t next_block_capacity(const Arena& arena,
                                         size_t needed) noexcept {
    const size_t current = arena.head->capacity;
    const size_t grown = next_capacity(arena.options.growth, current,
                                       saturating_add(current, 1));
    if (grown == 0) {
        return 0;
    }
    return std::max(grown, needed);
}

void consolidate_to(Arena* arena, size_t capacity) noexcept {
    const bool single = arena->head->next == nullptr && arena->spare == nullptr;
    if (single && arena->head->capacity == capacity) {
        return;
    }

    // allocate first: on failure keep the current head and drop the rest.
    Arena_Block* const fresh = create_block(arena, capacity);
    if (fresh == nullptr) [[unlikely]] {
        free_chain(arena, arena->head->next);
        arena->head->next = nullptr;
        free_chain(arena, arena->spare);
        arena->spare = nullptr;
        return;
    }

    free_chain(arena, arena->head);
    free_chain(arena, arena->spare);
    arena->spare = nullptr;
    arena->head = fresh;
}

/// Bumps inside the current block, or returns null if it does not fit.
[[nodiscard]] void* try_bump(Arena* arena, size_t size, size_t align) noexcept {
    const auto address = reinterpret_cast<uintptr_t>(arena->ptr);
    const size_t padding = ((address + align - 1) & ~(align - 1)) - address;
    const auto remaining = static_cast<size_t>(arena->end - arena->ptr);
    if (size > remaining || padding > remaining - size) {
        return nullptr;
    }

    std::byte* const result = arena->ptr + padding;
    arena->ptr = result + size;
    unpoison_memory(result, size);
    return result;
}

/// Adds or reuses a block, then bumps in it.
[[nodiscard]] void* allocate_slow(Arena* arena, size_t size,
                                  size_t align) noexcept {
    // Worst-case padding is align - 1, and a fresh block start is already
    // ARENA_DEFAULT_ALIGN-aligned, so `size + align` always fits.
    const size_t needed = saturating_add(size, align);
    if (needed == SIZE_MAX) [[unlikely]] {
        return nullptr;
    }

    Arena_Block* block = take_spare(arena, needed);
    if (block == nullptr) {
        const size_t capacity = next_block_capacity(*arena, needed);
        if (capacity == 0) {
            return nullptr;
        }
        block = create_block(arena, capacity);
        if (block == nullptr) [[unlikely]] {
            return nullptr;
        }
    }

    arena->retired_bytes += static_cast<size_t>(arena->ptr - data(arena->head));
    block->next = arena->head;
    arena->head = block;
    set_cursor_to_head(arena);
    return try_bump(arena, size, align);
}

}  // namespace

void* Arena_Resource::do_allocate(size_t bytes, size_t alignment) {
    void* const ptr = argus::mem::allocate(arena, bytes, alignment);
    ARGUS_ASSERT(ptr != nullptr, "Arena is out of memory!");
    return ptr;
}

void init(Arena* arena, const Arena_Options& options) noexcept {
    ARGUS_ASSERT(arena != nullptr);
    ARGUS_ASSERT(arena->head == nullptr, "Arena is already initialized!");
    ARGUS_ASSERT(options.initial_capacity != 0);
    ARGUS_ASSERT(options.growth.max_capacity >= options.initial_capacity);

    arena->options = options;
    arena->head = create_block(arena, options.initial_capacity);
    ARGUS_ASSERT(arena->head != nullptr, "Failed to allocate arena block!");
    set_cursor_to_head(arena);
}

void destroy(Arena* arena) noexcept {
    ARGUS_ASSERT(arena != nullptr);
    free_chain(arena, arena->head);
    free_chain(arena, arena->spare);
    arena->head = nullptr;
    arena->spare = nullptr;
    arena->ptr = nullptr;
    arena->end = nullptr;
    arena->retired_bytes = 0;
}

void reset(Arena* arena, std::source_location location) noexcept {
    ARGUS_ASSERT(arena != nullptr, "Reset needs an arena");

    arena->cycle_peak = std::max(arena->cycle_peak, get_used_bytes(*arena));
    const size_t peak = arena->cycle_peak;
    arena->high_water = std::max(arena->high_water, peak);
    arena->cycle_peak = 0;
    arena->last_reset_location = location;

    if (arena->head->next != nullptr || arena->spare != nullptr) {
        // Overflowed: replace the chain with one block that fits the whole
        // cycle (12.5% slack covers padding differences between block layouts).
        consolidate_to(arena, std::max(arena->options.initial_capacity,
                                       align_up(saturating_add(peak, peak / 8),
                                                ROUND_GRANULARITY)));
        arena->underused_resets = 0;
    } else if (arena->head->capacity > arena->options.initial_capacity &&
               peak * SHRINK_FACTOR < arena->head->capacity) {
        if (++arena->underused_resets >= ARENA_SHRINK_AFTER_RESETS) {
            consolidate_to(arena, std::max(arena->options.initial_capacity,
                                           align_up(saturating_mul(peak, 2),
                                                    ROUND_GRANULARITY)));
            arena->underused_resets = 0;
        }
    } else {
        arena->underused_resets = 0;
    }

    arena->retired_bytes = 0;
    set_cursor_to_head(arena);
    scrub(*arena, arena->ptr, arena->end);
}

void rewind(Arena* arena, const Arena_Mark& mark,
            std::source_location location) noexcept {
    ARGUS_ASSERT(arena != nullptr);
    ARGUS_ASSERT(mark.block != nullptr);

    if (arena->head == mark.block) {
        ARGUS_ASSERT(mark.ptr <= arena->ptr,
                     "Mark is newer than the current position!");
    }

    arena->cycle_peak = std::max(arena->cycle_peak, get_used_bytes(*arena));

    // Drop blocks added after the mark into the spare list. Newest (largest)
    // goes first, so the smallest ends up on top and is reused first.
    while (arena->head != mark.block) {
        ARGUS_ASSERT(arena->head != nullptr,
                     "Mark does not belong to this arena or was rewound past!");
        Arena_Block* const block = arena->head;
        arena->head = block->next;
        scrub(*arena, data(block), data(block) + block->capacity);
        block->next = arena->spare;
        arena->spare = block;
    }

    arena->ptr = mark.ptr;
    arena->end = data(arena->head) + arena->head->capacity;
    arena->retired_bytes = mark.retired_bytes;
    scrub(*arena, arena->ptr, arena->end);
    arena->last_reset_location = location;
}

[[nodiscard]] void* allocate(Arena* arena, size_t size, size_t align) noexcept {
    ARGUS_ASSERT(arena != nullptr);
    ARGUS_ASSERT(is_power_of_two(align));

    void* const result = try_bump(arena, size, align);
    if (result != nullptr) [[likely]] {
        return result;
    }
    return allocate_slow(arena, size, align);
}

size_t get_used_bytes(const Arena& arena) noexcept {
    return arena.retired_bytes +
           static_cast<size_t>(arena.ptr - data(arena.head));
}

Arena_Stats get_stats(const Arena& arena) noexcept {
    const size_t used = get_used_bytes(arena);
    return {.used = used,
            .high_water = std::max({arena.high_water, arena.cycle_peak, used}),
            .capacity = arena.capacity_total,
            .block_count = arena.block_count};
}

}  // namespace argus::mem
