#include <cstddef>
#include <cstdlib>
#include <exception>
#include <new>

// mimalloc global hooks might bypass MSVC ASan-instrumented allocation. In this
// case route through the CRT/debug heap when AddressSanitizer is enabled (MSVC
// /fsanitize=address or Clang -fsanitize=address).
#ifdef ARGUS_USE_MIMALLOC
#include <mimalloc.h>
#define ARGUS_GLOBAL_ALLOC_USE_MIMALLOC_BACKEND
#endif

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

// These replaceable operator new/delete overloads form a single atomic set:
// once any of them is provided, every allocation must be paired with a
// deallocation from this same file. LTO and linker dead-stripping can each
// independently decide that an individual overload has no directly-visible
// caller (calls to it may only be emitted implicitly, e.g. by scalar
// deleting destructors) and discard it, while keeping others - silently
// falling back to the default library implementation for the discarded
// overloads. Mixing this file's operator new with the default operator
// delete (or vice versa) corrupts the heap, so every overload below is
// force-kept regardless of apparent reachability.
#ifdef __has_attribute
#if __has_attribute(retain)
#define ARGUS_GLOBAL_ALLOC_KEEP __attribute__((used, retain))
#else
#define ARGUS_GLOBAL_ALLOC_KEEP __attribute__((used))
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define ARGUS_GLOBAL_ALLOC_KEEP __attribute__((used))
#else
#define ARGUS_GLOBAL_ALLOC_KEEP
#endif

namespace {

[[nodiscard]] void* raw_alloc(size_t size) noexcept {
#ifdef ARGUS_GLOBAL_ALLOC_USE_MIMALLOC_BACKEND
    return mi_malloc(size);
#elif defined(_MSC_VER) && defined(_DEBUG)
    // MSVC debug builds route new/delete through the debug heap. Using malloc
    // here mixes allocators and can deadlock during static initialization.
    return _malloc_dbg(size, _NORMAL_BLOCK, nullptr, 0);
#else
    return std::malloc(size);
#endif
}

[[nodiscard]] void* raw_aligned_alloc(size_t size,
                                      std::align_val_t alignment) noexcept {
#ifdef ARGUS_GLOBAL_ALLOC_USE_MIMALLOC_BACKEND
    return mi_malloc_aligned(size, static_cast<size_t>(alignment));
#elif defined(_MSC_VER) && defined(_DEBUG)
    return _aligned_malloc_dbg(size, static_cast<size_t>(alignment), nullptr,
                               0);
#elifdef _MSC_VER
    return _aligned_malloc(size, static_cast<size_t>(alignment));
#else
    const auto align = static_cast<size_t>(alignment);
    const size_t aligned = (size + align - 1) & ~(align - 1);
    return std::aligned_alloc(align, aligned);
#endif
}

void raw_free(void* ptr) noexcept {
#ifdef ARGUS_GLOBAL_ALLOC_USE_MIMALLOC_BACKEND
    mi_free(ptr);
#elif defined(_MSC_VER) && defined(_DEBUG)
    _free_dbg(ptr, _NORMAL_BLOCK);
#else
    std::free(ptr);
#endif
}

void raw_aligned_free(void* ptr) noexcept {
#ifdef ARGUS_GLOBAL_ALLOC_USE_MIMALLOC_BACKEND
    mi_free(ptr);
#elif defined(_MSC_VER) && defined(_DEBUG)
    _aligned_free_dbg(ptr);
#elifdef _MSC_VER
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

[[noreturn]] void on_allocation_failure() {
#if defined(__cpp_exceptions) && __cpp_exceptions
    throw std::bad_alloc{};
#else
    std::terminate();
#endif
}

}  // namespace

ARGUS_GLOBAL_ALLOC_KEEP void* operator new(size_t size) {
    void* const ptr = raw_alloc(size);
    if (ptr == nullptr) [[unlikely]] {
        on_allocation_failure();
    }

    return ptr;
}

ARGUS_GLOBAL_ALLOC_KEEP void* operator new[](size_t size) {
    return ::operator new(size);
}

ARGUS_GLOBAL_ALLOC_KEEP void* operator new(
    size_t size, const std::nothrow_t& /*nothrow*/) noexcept {
    return raw_alloc(size);
}

ARGUS_GLOBAL_ALLOC_KEEP void* operator new[](
    size_t size, const std::nothrow_t& /*nothrow*/) noexcept {
    return ::operator new(size, std::nothrow);
}

ARGUS_GLOBAL_ALLOC_KEEP void operator delete(void* ptr) noexcept {
    if (ptr == nullptr) [[unlikely]] {
        return;
    }

    raw_free(ptr);
}

ARGUS_GLOBAL_ALLOC_KEEP void operator delete[](void* ptr) noexcept {
    ::operator delete(ptr);
}

ARGUS_GLOBAL_ALLOC_KEEP void operator delete(void* ptr, size_t) noexcept {
    ::operator delete(ptr);
}

ARGUS_GLOBAL_ALLOC_KEEP void operator delete[](void* ptr, size_t) noexcept {
    ::operator delete(ptr);
}

ARGUS_GLOBAL_ALLOC_KEEP void operator delete(
    void* ptr, const std::nothrow_t& /*nothrow*/) noexcept {
    ::operator delete(ptr);
}

ARGUS_GLOBAL_ALLOC_KEEP void operator delete[](
    void* ptr, const std::nothrow_t& /*nothrow*/) noexcept {
    ::operator delete(ptr);
}

ARGUS_GLOBAL_ALLOC_KEEP void* operator new(size_t size,
                                           std::align_val_t alignment) {
    void* const ptr = raw_aligned_alloc(size, alignment);
    if (ptr == nullptr) [[unlikely]] {
        on_allocation_failure();
    }

    return ptr;
}

ARGUS_GLOBAL_ALLOC_KEEP void* operator new[](size_t size,
                                             std::align_val_t alignment) {
    return ::operator new(size, alignment);
}

ARGUS_GLOBAL_ALLOC_KEEP void* operator new(
    size_t size, std::align_val_t alignment,
    const std::nothrow_t& /*nothrow*/) noexcept {
    return raw_aligned_alloc(size, alignment);
}

ARGUS_GLOBAL_ALLOC_KEEP void* operator new[](
    size_t size, std::align_val_t alignment,
    const std::nothrow_t& /*nothrow*/) noexcept {
    return ::operator new(size, alignment, std::nothrow);
}

ARGUS_GLOBAL_ALLOC_KEEP void operator delete(
    void* ptr, std::align_val_t /*alignment*/) noexcept {
    if (ptr == nullptr) [[unlikely]] {
        return;
    }

    raw_aligned_free(ptr);
}

ARGUS_GLOBAL_ALLOC_KEEP void operator delete[](
    void* ptr, std::align_val_t alignment) noexcept {
    ::operator delete(ptr, alignment);
}

ARGUS_GLOBAL_ALLOC_KEEP void operator delete(
    void* ptr, size_t /*size*/, std::align_val_t alignment) noexcept {
    ::operator delete(ptr, alignment);
}

ARGUS_GLOBAL_ALLOC_KEEP void operator delete[](
    void* ptr, size_t size, std::align_val_t alignment) noexcept {
    ::operator delete(ptr, size, alignment);
}

namespace argus::mem {

void keep_global_alloc() noexcept {}

}  // namespace argus::mem
