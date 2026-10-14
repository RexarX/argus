#ifndef ARGUS_COMPILER_H
#define ARGUS_COMPILER_H

#if defined(__GNUC__) || defined(__clang__)
#define ARGUS_EXPECT_TRUE(x) __builtin_expect(!!(x), 1)
#define ARGUS_EXPECT_FALSE(x) __builtin_expect(!!(x), 0)
#else
#define ARGUS_EXPECT_TRUE(x) (x)
#define ARGUS_EXPECT_FALSE(x) (x)
#endif

#if defined(__GNUC__) || defined(__clang__)
#define ARGUS_C_PRINTF(fmt_index, args_index) \
    __attribute__((format(printf, fmt_index, args_index)))
#else
#define ARGUS_C_PRINTF(fmt_index, args_index)
#endif

#ifdef _MSC_VER
#define ARGUS_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define ARGUS_FORCE_INLINE __attribute__((always_inline)) inline
#else
#define ARGUS_FORCE_INLINE inline
#endif

#ifdef _MSC_VER
#define ARGUS_NO_INLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define ARGUS_NO_INLINE __attribute__((noinline))
#else
#define ARGUS_NO_INLINE
#endif

#endif
