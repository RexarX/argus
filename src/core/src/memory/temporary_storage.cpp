#include <argus/memory/temporary_storage.hpp>

#include <argus/assert.hpp>
#include <argus/memory/arena.hpp>

#include <cstddef>
#include <new>
#include <source_location>

namespace argus::mem {

namespace {

/// The only global state: the calling thread's bound arena.
thread_local Arena* g_temporary_storage = nullptr;

}  // namespace

void init_temporary_storage(const Arena_Options& options) noexcept {
    auto* const arena = new (std::nothrow) Arena;
    ARGUS_ASSERT(arena != nullptr, "Failed to allocate temporary storage!");
    init(arena, options);
    bind_temporary_storage(arena);
}

void destroy_temporary_storage() noexcept {
    Arena* const arena = unbind_temporary_storage();
    destroy(arena);
    delete arena;
}

void reset_temporary_storage(std::source_location location) noexcept {
    ARGUS_ASSERT(g_temporary_storage != nullptr,
                 "Thread has no temporary storage!");
    reset(g_temporary_storage, location);
}

void rewind_temporary_storage(const Arena_Mark& mark,
                              std::source_location location) noexcept {
    ARGUS_ASSERT(g_temporary_storage != nullptr,
                 "Thread has no temporary storage!");
    rewind(g_temporary_storage, mark, location);
}

void bind_temporary_storage(Arena* arena) noexcept {
    ARGUS_ASSERT(arena != nullptr);
    ARGUS_ASSERT(g_temporary_storage == nullptr,
                 "Thread already has temporary storage!");
    g_temporary_storage = arena;
}

Arena* unbind_temporary_storage() noexcept {
    Arena* const arena = g_temporary_storage;
    ARGUS_ASSERT(arena != nullptr);
    g_temporary_storage = nullptr;
    return arena;
}

void* allocate_temporary(size_t size, size_t align) noexcept {
    ARGUS_ASSERT(g_temporary_storage != nullptr,
                 "Thread has no temporary storage!");
    return allocate(g_temporary_storage, size, align);
}

bool has_temporary_storage() noexcept {
    return g_temporary_storage != nullptr;
}

Arena* get_temporary_storage() noexcept {
    return g_temporary_storage;
}

Arena_Mark get_temporary_mark() noexcept {
    ARGUS_ASSERT(g_temporary_storage != nullptr,
                 "Thread has no temporary storage!");
    return get_mark(*g_temporary_storage);
}

}  // namespace argus::mem
