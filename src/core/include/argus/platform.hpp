#pragma once

#include <argus/platform.h>

#ifdef ARGUS_PLATFORM_WINDOWS
#define ARGUS_EXPORT __declspec(dllexport)
#elif defined(ARGUS_PLATFORM_LINUX)
#define ARGUS_EXPORT __attribute__((visibility("default")))
#else
#define ARGUS_EXPORT
#endif

/**
 * @brief Inserts a CPU pause hint for busy-wait loops.
 * @details No-op on unknown architectures.
 *
 * Adapted from SDL3 `SDL_CPUPauseInstruction` (`SDL_atomic.h`).
 *
 * Simple DirectMedia Layer
 * Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Altered: renamed to `ARGUS_PAUSE_CPU`
 */
// NOLINTBEGIN(hicpp-no-assembler)
#if (defined(__GNUC__) || defined(__clang__)) && \
    (defined(__i386__) || defined(__x86_64__)) && !defined(__arm64ec__)
#define ARGUS_PAUSE_CPU() __asm__ __volatile__("pause\n")
#elif (defined(__arm__) && defined(__ARM_ARCH) && __ARM_ARCH >= 7) || \
    defined(__aarch64__)
#define ARGUS_PAUSE_CPU() __asm__ __volatile__("yield" ::: "memory")
#elif defined(__powerpc__) || defined(__powerpc64__)
#define ARGUS_PAUSE_CPU() __asm__ __volatile__("or 27,27,27")
#elif defined(__riscv) && __riscv_xlen == 64
#define ARGUS_PAUSE_CPU() __asm__ __volatile__(".insn i 0x0F, 0, x0, x0, 0x010")
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <intrin.h>
#define ARGUS_PAUSE_CPU() _mm_pause()
#elif defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARM64))
#define ARGUS_PAUSE_CPU() __yield()
#else
#define ARGUS_PAUSE_CPU()
#endif
// NOLINTEND(hicpp-no-assembler)
