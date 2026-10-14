#include <argus/memory/aligned_alloc.hpp>

#include <argus/assert.hpp>
#include <argus/memory/common.hpp>

#include <cstddef>
#include <cstdlib>

#ifdef ARGUS_USE_MIMALLOC
#include <mimalloc.h>
#define ARGUS_ALIGNED_ALLOC_USE_MIMALLOC
#elif defined(_MSC_VER)
#include <malloc.h>
#endif

namespace argus::mem {

#ifdef ARGUS_OVERRIDE_GLOBAL_ALLOC
void keep_global_alloc() noexcept;
#endif

void* aligned_alloc(size_t alignment, size_t size) noexcept {
#ifdef ARGUS_OVERRIDE_GLOBAL_ALLOC
    [[maybe_unused]] static const auto global_alloc_anchor = &keep_global_alloc;
#endif
    ARGUS_ASSERT(is_power_of_two(alignment));
    ARGUS_ASSERT(size != 0, );

#ifdef ARGUS_ALIGNED_ALLOC_USE_MIMALLOC
    return mi_malloc_aligned(size, alignment);
#elif defined(_MSC_VER)
    return _aligned_malloc(size, alignment);
#else
    if (alignment < sizeof(void*)) {
        return std::malloc(size);
    }
    return std::aligned_alloc(alignment, align_up(size, alignment));
#endif
}

void aligned_free(void* ptr) noexcept {
    if (ptr == nullptr) [[unlikely]] {
        return;
    }

#ifdef ARGUS_ALIGNED_ALLOC_USE_MIMALLOC
    mi_free(ptr);
#elif defined(_MSC_VER)
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

}  // namespace argus::mem
