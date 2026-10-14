#ifndef ARGUS_PLATFORM_H
#define ARGUS_PLATFORM_H

#ifdef ARGUS_BUILD_SHARED
#ifdef _WIN32
#ifdef ARGUS_EXPORTS
#define ARGUS_API __declspec(dllexport)
#else
#define ARGUS_API __declspec(dllimport)
#endif
#else
#define ARGUS_API __attribute__((visibility("default")))
#endif
#else
#define ARGUS_API
#endif

#ifdef __cplusplus
#define ARGUS_C_NOEXCEPT noexcept
#define ARGUS_C_BEGIN extern "C" {
#define ARGUS_C_END }
#define ARGUS_C_ALIGNAS(n) alignas(n)
#else
#define ARGUS_C_NOEXCEPT
#define ARGUS_C_BEGIN
#define ARGUS_C_END
#define ARGUS_C_ALIGNAS(n) _Alignas(n)
#endif

// TODO: Replace with std::breakpoint() from <debugging> in C++26
// https://en.cppreference.com/w/cpp/utility/breakpoint.html

#if defined(_MSC_VER) && (_MSC_VER >= 1300)
#define ARGUS_DEBUG_BREAK() __debugbreak()

#elif defined(__arm64__) && defined(__APPLE__)
#define ARGUS_DEBUG_BREAK() __builtin_debugtrap()

#elif defined(__arm64__) && (defined(__GNUC__) || defined(__clang__))
#define ARGUS_DEBUG_BREAK() __asm__("brk 0")

#elif defined(__arm__) && defined(__APPLE__)
#define ARGUS_DEBUG_BREAK() __asm__("trap")

#elif defined(__arm__) && (defined(__GNUC__) || defined(__clang__))
#define ARGUS_DEBUG_BREAK() __asm__("bkpt 0")

#elif defined(__arm__) && defined(__ARMCC_VERSION)
#define ARGUS_DEBUG_BREAK() __breakpoint(0)

#elif (defined(__x86_64__) || defined(__i386__)) && \
    (defined(__GNUC__) || defined(__clang__))
#define ARGUS_DEBUG_BREAK() __asm__("int3")

#elif (defined(_M_X64) || defined(_M_IX86)) && defined(_MSC_VER)
#define ARGUS_DEBUG_BREAK() __asm int 3

#elif defined(__powerpc__) || defined(__ppc__) || defined(_ARCH_PPC)
#define ARGUS_DEBUG_BREAK() __asm__(".long 0")

#elif defined(__riscv) || defined(__riscv__)
#define ARGUS_DEBUG_BREAK() __asm__("ebreak")

#elif defined(__wasm__)
#define ARGUS_DEBUG_BREAK() __asm__("unreachable")

#elif defined(__has_builtin) && __has_builtin(__builtin_trap)
#define ARGUS_DEBUG_BREAK() __builtin_trap()

#else
#include <signal.h>
#define ARGUS_DEBUG_BREAK() raise(SIGTRAP)
#endif

#endif
