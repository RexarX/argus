#pragma once

#include <argus/memory/arena.hpp>
#include <argus/platform.hpp>
#include <argus/utils/macro.hpp>

#include <cstddef>
#include <memory_resource>
#include <source_location>

namespace argus::mem {

/// @brief Allocates a new arena and binds it to the calling thread.
/// @param options Arena configuration
/// @warning Asserts if the thread already has a bound arena.
ARGUS_API void init_temporary_storage(
    const Arena_Options& options = {}) noexcept;

/// @brief Unbinds and destroys the calling thread's arena.
/// @warning Asserts if the thread has no bound arena. Invalidates every
/// outstanding temporary allocation of this thread.
ARGUS_API void destroy_temporary_storage() noexcept;

/// @brief Resets the calling thread's arena.
/// @param location Recorded as the last reset location
/// @warning Asserts if the thread has no bound arena. Invalidates every
/// outstanding temporary allocation of this thread.
ARGUS_API void reset_temporary_storage(
    std::source_location location = std::source_location::current()) noexcept;

/// @brief Binds an externally owned arena to the calling thread.
/// @details Use this with pooled arenas that outlive a thread.
/// @param arena Initialized arena to bind
/// @warning Asserts in next cases:
/// - If `arena` is null.
/// - If the thread already has a bound arena.
ARGUS_API void bind_temporary_storage(Arena* arena) noexcept;

/// @brief Unbinds the calling thread's arena without destroying it.
/// @return The previously bound arena
/// @warning Asserts if the thread has no bound arena.
ARGUS_API Arena* unbind_temporary_storage() noexcept;

/// @brief Rewinds the calling thread's arena to `mark`.
/// @param mark Result of `get_temporary_mark` on this thread
/// @param location Recorded as the last reset location
/// @warning Asserts if the thread has no bound arena. See `rewind`.
ARGUS_API void rewind_temporary_storage(
    const Arena_Mark& mark,
    std::source_location location = std::source_location::current()) noexcept;

/// @brief Bump-allocates `size` bytes from the calling thread's arena.
/// @param size Byte count. Zero is allowed
/// @param align Alignment in bytes. Power of two
/// @return Pointer to the memory, or null if the system is out of memory
/// @warning Asserts in next cases:
/// - If the thread has no bound arena.
/// - If `align` is not a power of two.
[[nodiscard]] ARGUS_API void* allocate_temporary(
    size_t size, size_t align = ARENA_DEFAULT_ALIGN) noexcept;

/// @brief Bump-allocates uninitialized storage for `count` objects of `T` from
/// the calling thread's arena.
/// @tparam T Element type
/// @param count Element count
/// @return Pointer to the first element, or null on overflow / out of memory
/// @warning Asserts if the thread has no bound arena.
template <typename T>
[[nodiscard]] inline T* allocate_temporary_array(size_t count = 1) noexcept {
    if (count > SIZE_MAX / sizeof(T)) [[unlikely]] {
        return nullptr;
    }
    return static_cast<T*>(allocate_temporary(count * sizeof(T), alignof(T)));
}

/// @brief Checks if the calling thread has a bound arena.
/// @return `true` if bound, `false` otherwise
[[nodiscard]] ARGUS_API bool has_temporary_storage() noexcept;

/// @brief Gets the calling thread's bound arena.
/// @return Bound arena, or null if none is bound
[[nodiscard]] ARGUS_API Arena* get_temporary_storage() noexcept;

/// @brief Captures the calling thread's current position.
/// @return rewind point
/// @warning Asserts if the thread has no bound arena.
[[nodiscard]] ARGUS_API Arena_Mark get_temporary_mark() noexcept;

/// @brief The calling thread's temporary memory resource.
/// @details If the thread has no bound arena this asserts in debug builds and
/// falls back to `std::pmr::new_delete_resource()` in release builds, so a
/// missing init is a slow path instead of a crash.
/// @return Arena memory resource of the thread's bound arena
/// @warning Asserts if the thread has no bound arena.
[[nodiscard]] inline Arena_Resource get_temporary_resource() noexcept {
    Arena* const arena = get_temporary_storage();
    ARGUS_ASSERT(
        arena != nullptr,
        "Thread has no temporary storage, call init_temporary_storage!");
    return get_resource(arena);
}

/// @brief Creates a `polymorphic_allocator<T>` over the calling thread's arena.
/// @tparam T Element type
/// @return Allocator over `get_temporary_resource()`
/// @code
///   std::pmr::vector<int> scratch{argus::mem::get_temporary_allocator<int>()};
/// @endcode
template <typename T = std::byte>
[[nodiscard]] inline auto get_temporary_allocator() noexcept
    -> std::pmr::polymorphic_allocator<T> {
    return std::pmr::polymorphic_allocator<T>{get_temporary_resource()};
}

/// @brief Rewinds the calling thread's arena to its construction-time position
/// on scope exit.
/// @code
///   {
///     argus::mem::Temporary_Scope scope;
///     auto* scratch = argus::mem::allocate_temporary_array<int>(128);
///     ...
///   }  // scratch is gone here
/// @endcode
struct Temporary_Scope {
    /// @brief Marks the calling thread's bound arena.
    /// @param loc Recorded as the last reset location on exit
    /// @warning Asserts if the thread has no bound arena.
    explicit Temporary_Scope(
        std::source_location loc = std::source_location::current()) noexcept
        : scope(get_temporary_storage(), loc) {}

    Arena_Scope scope;
};

/// @brief `init_temporary_storage` on construction,
/// `destroy_temporary_storage` on destruction. For threads the engine does not
/// create (tests, middleware callbacks).
/// @warning Asserts if the calling thread already has a bound arena.
struct Temporary_Thread_Scope {
    explicit Temporary_Thread_Scope(
        const Arena_Options& options = {}) noexcept {
        init_temporary_storage(options);
    }
    Temporary_Thread_Scope(const Temporary_Thread_Scope&) = delete;
    Temporary_Thread_Scope(Temporary_Thread_Scope&&) = delete;
    ~Temporary_Thread_Scope() noexcept { destroy_temporary_storage(); }

    Temporary_Thread_Scope& operator=(const Temporary_Thread_Scope&) = delete;
    Temporary_Thread_Scope& operator=(Temporary_Thread_Scope&&) = delete;
};

}  // namespace argus::mem

/// @brief Creates `Temporary_Scope` local variable with a unique name.
#define ARGUS_TEMPORARY_STORAGE_SCOPE() \
    ::argus::mem::Temporary_Scope ARGUS_ANONYMOUS_VAR(_temporary_scope_)

/// @brief Creates `Temporary_Thread_Scope` local variable with a unique name.
/// @param options `Arena_Options` arena configuration
#define ARGUS_TEMPORARY_STORAGE_THREAD_SCOPE(options)         \
    ::argus::mem::Temporary_Thread_Scope ARGUS_ANONYMOUS_VAR( \
        _temporary_thread_scope_)(options)
