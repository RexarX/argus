#include <argus/memory/bump.hpp>

#include <argus/assert.hpp>
#include <argus/memory/common.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace argus::mem {

void* Bump_Resource::do_allocate(size_t bytes, size_t alignment) {
    void* const ptr = argus::mem::allocate(bump, bytes, alignment);
    ARGUS_ASSERT(ptr != nullptr, "Bump allocator is out of memory!");
    return ptr;
}

void init(Bump* bump, void* storage, size_t size) noexcept {
    ARGUS_ASSERT(bump != nullptr);
    ARGUS_ASSERT(storage != nullptr);
    ARGUS_ASSERT(size != 0);

    bump->storage = static_cast<std::byte*>(storage);
    bump->capacity = size;
    bump->offset.store(0, std::memory_order_relaxed);
}

void reset(Bump* bump) noexcept {
    ARGUS_ASSERT(bump != nullptr);
    bump->offset.store(0, std::memory_order_relaxed);
}

void* allocate(Bump* bump, size_t size, size_t align) noexcept {
    ARGUS_ASSERT(bump != nullptr);
    ARGUS_ASSERT(bump->storage != nullptr,
                 "Bump allocator is not initialized!");
    ARGUS_ASSERT(is_power_of_two(align));

    if (size > SIZE_MAX - (align - 1)) [[unlikely]] {
        return nullptr;
    }
    const size_t reservation = size + (align - 1);
    const size_t begin =
        bump->offset.fetch_add(reservation, std::memory_order_relaxed);
    if (reservation > bump->capacity || begin > bump->capacity - reservation) {
        return nullptr;
    }

    const auto base = reinterpret_cast<uintptr_t>(bump->storage);
    const uintptr_t raw = base + begin;
    const uintptr_t aligned = (raw + static_cast<uintptr_t>(align) - 1) &
                              ~(static_cast<uintptr_t>(align) - 1);
    return reinterpret_cast<void*>(aligned);
}

}  // namespace argus::mem
