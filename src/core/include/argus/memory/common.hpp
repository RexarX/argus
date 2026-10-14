#pragma once

#include <argus/assert.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

// AddressSanitizer integration: poisoned memory makes a use-after-free-style
// bug report immediately with a stack trace.
#ifdef __SANITIZE_ADDRESS__
#define ARGUS_MEMORY_ASAN 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define ARGUS_MEMORY_ASAN 1
#endif
#endif

#ifdef ARGUS_MEMORY_ASAN
#include <sanitizer/asan_interface.h>
#endif

namespace argus::mem {

/// Minimum alignment used by memory resources.
inline constexpr size_t MIN_ALIGNMENT = alignof(std::max_align_t);

/// Byte pattern written by `fill_poison`.
inline constexpr unsigned char POISON_FILL = 0xDD;

/// @brief Saturating add for `size_t`.
/// @param lhs Left operand
/// @param rhs Right operand
/// @return lhs + rhs, clamped to `size_t` max on overflow
[[nodiscard]] constexpr size_t saturating_add(size_t lhs, size_t rhs) noexcept {
    if (rhs > SIZE_MAX - lhs) {
        return SIZE_MAX;
    }
    return lhs + rhs;
}

/// @brief Saturating multiply for `size_t`.
/// @param lhs Left operand
/// @param rhs Right operand
/// @return lhs * rhs, clamped to `size_t` max on overflow
[[nodiscard]] constexpr size_t saturating_mul(size_t lhs, size_t rhs) noexcept {
    if (lhs == 0 || rhs == 0) {
        return 0;
    }
    if (lhs > SIZE_MAX / rhs) {
        return SIZE_MAX;
    }
    return lhs * rhs;
}

/// @brief Checks if a value is a power of two.
/// @param value Value to check
/// @return `true` if `value` is a power of two, `false` otherwise
[[nodiscard]] constexpr bool is_power_of_two(size_t value) noexcept {
    return value != 0 && (value & (value - 1)) == 0;
}

/// @brief Aligns `value` up to `alignment`.
/// @param value Value to align
/// @param alignment Alignment, must be a power of two
/// @return Aligned value, or `size_t` max on overflow
/// @warning Asserts if `alignment` is not a power of two.
[[nodiscard]] constexpr size_t align_up(size_t value,
                                        size_t alignment) noexcept {
    ARGUS_ASSERT(is_power_of_two(alignment));
    const size_t mask = alignment - 1;
    if (value > SIZE_MAX - mask) {
        return SIZE_MAX;
    }
    return (value + mask) & ~mask;
}

/// @brief Checks if an address is aligned.
/// @param ptr Address to check
/// @param alignment Alignment to check, must be a power of two
/// @return `true` if `ptr` is aligned, `false` otherwise
/// @warning Asserts if `alignment` is not a power of two.
[[nodiscard]] inline bool is_aligned(const void* ptr,
                                     size_t alignment) noexcept {
    ARGUS_ASSERT(is_power_of_two(alignment));
    return (reinterpret_cast<uintptr_t>(ptr) & (alignment - 1)) == 0;
}

/// @brief Computes padding required to align an address.
/// @param ptr Current address
/// @param alignment Requested alignment, must be a power of two
/// @return Padding in bytes
/// @warning Asserts if `alignment` is not a power of two.
[[nodiscard]] inline size_t calculate_padding(const void* ptr,
                                              size_t alignment) noexcept {
    ARGUS_ASSERT(is_power_of_two(alignment));
    const auto address = reinterpret_cast<uintptr_t>(ptr);
    return ((address + alignment - 1) & ~(alignment - 1)) - address;
}

/// @brief Marks a region as inaccessible for AddressSanitizer. No-op without
/// ASan.
/// @param ptr Start of the region
/// @param size Region size in bytes
inline void poison_memory([[maybe_unused]] void* ptr,
                          [[maybe_unused]] size_t size) noexcept {
#ifdef ARGUS_MEMORY_ASAN
    ASAN_POISON_MEMORY_REGION(ptr, size);
#endif
}

/// @brief Marks a region as accessible again for AddressSanitizer. No-op
/// without ASan.
/// @param ptr Start of the region
/// @param size Region size in bytes
inline void unpoison_memory([[maybe_unused]] void* ptr,
                            [[maybe_unused]] size_t size) noexcept {
#ifdef ARGUS_MEMORY_ASAN
    ASAN_UNPOISON_MEMORY_REGION(ptr, size);
#endif
}

/// @brief Fills a region with `POISON_FILL` so stale reads stand out in a
/// debugger.
/// @param ptr Start of the region, must be accessible
/// @param size Region size in bytes
inline void fill_poison(void* ptr, size_t size) noexcept {
    std::memset(ptr, POISON_FILL, size);
}

/// Growth strategy used by growable allocators.
enum class Growth_Mode : uint8_t {
    LINEAR,     ///< Adds a fixed byte step on each growth request.
    GEOMETRIC,  ///< Multiplies capacity by a configured ratio on growth.
};

/// @brief Capacity growth configuration shared by growable allocators.
struct Growth_Policy {
    /// Active growth strategy.
    Growth_Mode mode = Growth_Mode::GEOMETRIC;
    /// Bytes added per growth step when `mode == LINEAR`.
    size_t linear_step = 0;
    /// Capacity multiplier numerator for geometric growth.
    size_t geometric_numerator = 2;
    /// Capacity multiplier denominator for geometric growth.
    size_t geometric_denominator = 1;
    /// Maximum capacity ceiling for any growth operation.
    size_t max_capacity = SIZE_MAX;
};

/// @brief Creates a linear growth policy.
/// @param step Bytes added per growth step. 0 disables growth
/// @param max Maximum capacity ceiling
/// @return Linear growth policy
[[nodiscard]] constexpr Growth_Policy create_linear_growth_policy(
    size_t step, size_t max = SIZE_MAX) noexcept {
    return {.mode = Growth_Mode::LINEAR,
            .linear_step = step,
            .geometric_numerator = 2,
            .geometric_denominator = 1,
            .max_capacity = max};
}

/// @brief Creates a geometric growth policy.
/// @param numerator Capacity multiplier numerator
/// @param denominator Capacity multiplier denominator
/// @param max Maximum capacity ceiling
/// @return Geometric growth policy
[[nodiscard]] constexpr Growth_Policy create_geometric_growth_policy(
    size_t numerator = 2, size_t denominator = 1,
    size_t max = SIZE_MAX) noexcept {
    return {.mode = Growth_Mode::GEOMETRIC,
            .linear_step = 0,
            .geometric_numerator = numerator,
            .geometric_denominator = denominator,
            .max_capacity = max};
}

/// @brief Computes the next capacity for a grow request.
/// @param policy Growth configuration
/// @param current_capacity Current block or region capacity
/// @param min_capacity Minimum bytes required for the pending allocation
/// @return `current_capacity` if it already covers `min_capacity`, otherwise
/// the grown capacity clamped to `max_capacity`. Returns 0 when growth is
/// disabled (zero linear step, or a geometric ratio below 1)
[[nodiscard]] constexpr size_t next_capacity(const Growth_Policy& policy,
                                             size_t current_capacity,
                                             size_t min_capacity) noexcept {
    if (current_capacity >= min_capacity) {
        return current_capacity;
    }

    size_t next = current_capacity == 0 ? MIN_ALIGNMENT : current_capacity;

    if (policy.mode == Growth_Mode::LINEAR) {
        if (policy.linear_step == 0) {
            return 0;
        }
        while (next < min_capacity && next != SIZE_MAX) {
            next = saturating_add(next, policy.linear_step);
        }
    } else {
        if (policy.geometric_denominator == 0 ||
            policy.geometric_numerator < policy.geometric_denominator) {
            return 0;
        }
        while (next < min_capacity && next != SIZE_MAX) {
            const size_t previous = next;
            next = saturating_mul(next, policy.geometric_numerator) /
                   policy.geometric_denominator;
            if (next <= previous) {
                // Ratio too close to 1 to make progress.
                next = SIZE_MAX;
            }
        }
    }

    return std::min(next, policy.max_capacity);
}

}  // namespace argus::mem
