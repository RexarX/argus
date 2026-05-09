#pragma once

#if defined(__GNUC__) || defined(__clang__)
#define ARGUS_EXPECT_TRUE(x) __builtin_expect(!!(x), 1)
#define ARGUS_EXPECT_FALSE(x) __builtin_expect(!!(x), 0)
#else
#define ARGUS_EXPECT_TRUE(x) (x)
#define ARGUS_EXPECT_FALSE(x) (x)
#endif

#if defined(__cpp_lib_move_only_function) && \
    __cpp_lib_move_only_function >= 202110L
#define ARGUS_MOVEONLY_FUNCTION_AVAILABLE
#endif

#if defined(__cpp_lib_containers_ranges) && \
    __cpp_lib_containers_ranges >= 202202L
#define ARGUS_CONTAINERS_RANGES_AVAILABLE
#endif

#if defined(__cpp_lib_flat_map) && __cpp_lib_flat_map >= 202207L
#define ARGUS_STL_FLAT_MAP_AVAILABLE
#endif

#ifdef _MSC_VER
#define ARGUS_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define ARGUS_FORCE_INLINE __attribute__((always_inline)) inline
#else
#define ARGUS_FORCE_INLINE inline
#endif

#if defined(__clang__)
#define ARGUS_ALWAYS_INLINE [[clang::always_inline]]
#elif defined(__GNUC__)
#define ARGUS_ALWAYS_INLINE [[gnu::always_inline]]
#elif defined(_MSC_VER)
#define ARGUS_ALWAYS_INLINE [[msvc::forceinline]]
#else
#define ARGUS_ALWAYS_INLINE
#endif

#ifdef _MSC_VER
#define ARGUS_NO_INLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define ARGUS_NO_INLINE __attribute__((noinline))
#else
#define ARGUS_NO_INLINE
#endif
