#pragma once

#include <argus/assert.hpp>
#include <argus/platform.hpp>

#include <atomic>
#include <cstddef>
#include <memory_resource>

namespace argus::mem {

/// Default alignment for `allocate` on a `Bump`.
inline constexpr size_t BUMP_DEFAULT_ALIGN = 16;

struct Bump;

/// @brief `std::pmr::memory_resource` proxy over one `Bump`.
/// @details Allocation forwards to `allocate`; deallocation is a no-op. It is a
/// member of `Bump`, get it with `get_resource`.
/// @warning Concurrent `allocate` calls are safe. `init` and `reset` are not.
struct Bump_Resource final : public std::pmr::memory_resource {
    Bump* bump = nullptr;

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

/// @brief Wait-free bump allocator over caller-owned storage.
/// @details Does not own `storage`. `allocate` is one relaxed `fetch_add` of
/// `size + align - 1`, then aligns the absolute address inside that
/// reservation. A failed allocation still consumes the reservation. `reset`
/// rewinds the cursor and must not run concurrently with `allocate`.
///
/// Treat fields as internal and use the free functions. Initialize with `init`.
/// @warning Not copyable (`resource` points back at the bump).
struct Bump {
    std::byte* storage = nullptr;
    size_t capacity = 0;
    std::atomic<size_t> offset{0};
};

/// @brief Binds `bump` to `[storage, storage + size)`.
/// @param bump Bump allocator to initialize
/// @param storage Caller-owned buffer. Not freed by the bump
/// @param size Buffer size in bytes. Must be non-zero
/// @warning Asserts in next cases:
/// - If `bump` or `storage` is null.
/// - If `size` is 0.
ARGUS_API void init(Bump* bump, void* storage, size_t size) noexcept;

/// @brief Rewinds the cursor to the start of the buffer.
/// @param bump Bump allocator to reset
/// @warning Asserts if `bump` is null. Must not run concurrently with
/// `allocate`. Live pointers into the buffer become invalid.
ARGUS_API void reset(Bump* bump) noexcept;

/// @brief Bump-allocates `size` bytes from `bump`.
/// @param bump Bump allocator to allocate from
/// @param size Byte count. Zero is allowed
/// @param align Alignment in bytes. Power of two
/// @return Pointer into the caller buffer, or null if the reservation does not
/// fit. A null return still advances the cursor
/// @warning Asserts in next cases:
/// - If `bump` is null or not initialized.
/// - If `align` is not a power of two.
[[nodiscard]] ARGUS_API void* allocate(
    Bump* bump, size_t size, size_t align = BUMP_DEFAULT_ALIGN) noexcept;

/// @brief Gets bytes reserved since the last reset, including failed
/// reservations.
/// @param bump Bump allocator to query
/// @return Cursor offset. May exceed the buffer size after a failed allocation
[[nodiscard]] inline size_t get_used_bytes(const Bump& bump) noexcept {
    return bump.offset.load(std::memory_order_relaxed);
}

/// @brief Gets the PMR proxy over `bump`.
/// @param bump Bump allocator to proxy
/// @return Bump memory resource valid for the bump's lifetime
/// @warning Asserts if `bump` is null.
[[nodiscard]] inline Bump_Resource get_resource(Bump* bump) noexcept {
    ARGUS_ASSERT(bump != nullptr);
    Bump_Resource resource;
    resource.bump = bump;
    return resource;
}

}  // namespace argus::mem
