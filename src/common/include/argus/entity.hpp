#pragma once

#include <cstdint>

namespace argus {

using Entity_Index = uint32_t;
using Entity_Generation = uint32_t;

inline constexpr Entity_Index INVALID_ENTITY_INDEX = UINT32_MAX;
inline constexpr Entity_Generation INVALID_ENTITY_GENERATION = UINT32_MAX;

/// @brief Top bit of an entity generation. Set while the slot holds a live
/// entity, clear while the slot is free.
inline constexpr Entity_Generation ENTITY_GENERATION_ALIVE_BIT = 0x80000000U;

/// @brief Low 31 bits of an entity generation: the slot reuse counter.
inline constexpr Entity_Generation ENTITY_GENERATION_COUNTER_MASK = 0x7FFFFFFFU;

/// @brief Generation given to the first entity ever placed in a slot.
inline constexpr Entity_Generation INITIAL_ALIVE_ENTITY_GENERATION =
    ENTITY_GENERATION_ALIVE_BIT;

/// @brief Checks whether `generation` encodes a live entity.
/// @param generation Stored or handle generation to inspect
/// @return `true` when the alive bit is set and `generation` is not
/// `INVALID_ENTITY_GENERATION`, `false` otherwise
[[nodiscard]] constexpr bool is_alive_generation(
    Entity_Generation generation) noexcept {
    return generation != INVALID_ENTITY_GENERATION &&
           (generation & ENTITY_GENERATION_ALIVE_BIT) != 0;
}

/// @brief Computes the free encoding stored when a live entity is destroyed.
/// @details Advances the reuse counter by one and clears the alive bit, so
/// every entity issued for the previous life of the slot stops matching. The
/// counter value that would collide with `INVALID_ENTITY_GENERATION` is
/// skipped.
/// @param alive_generation Generation of the entity being destroyed
/// @return Free generation to store in the slot
[[nodiscard]] constexpr Entity_Generation get_freed_generation(
    Entity_Generation alive_generation) noexcept {
    Entity_Generation counter =
        ((alive_generation & ENTITY_GENERATION_COUNTER_MASK) + 1) &
        ENTITY_GENERATION_COUNTER_MASK;
    if (counter == ENTITY_GENERATION_COUNTER_MASK) [[unlikely]] {
        counter = 0;
    }
    return counter;
}

/// @brief Computes the alive encoding for a slot that is being reused.
/// @details Keeps the reuse counter chosen by `get_freed_generation` and sets
/// the alive bit.
/// @param freed_generation Free generation currently stored in the slot
/// @return Alive generation for the next entity living in the slot
[[nodiscard]] constexpr Entity_Generation get_revived_generation(
    Entity_Generation freed_generation) noexcept {
    return (freed_generation & ENTITY_GENERATION_COUNTER_MASK) |
           ENTITY_GENERATION_ALIVE_BIT;
}

struct Entity {
    Entity_Index index = INVALID_ENTITY_INDEX;
    /// Needed to avoid dangling references when reusing entity slots
    Entity_Generation generation = INVALID_ENTITY_GENERATION;

    [[nodiscard]] constexpr bool operator==(
        const Entity& other) const noexcept = default;
    [[nodiscard]] constexpr bool operator!=(
        const Entity& other) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(
        const Entity& other) const noexcept = default;
};

inline constexpr Entity INVALID_ENTITY = {
    .index = INVALID_ENTITY_INDEX, .generation = INVALID_ENTITY_GENERATION};

/// @brief Checks if entity is valid.
/// @param handle Entity to check
/// @return `true` when `index` and `generation` are valid, `false` otherwise
[[nodiscard]] constexpr bool is_valid(Entity entity) noexcept {
    return entity.index != INVALID_ENTITY_INDEX &&
           entity.generation != INVALID_ENTITY_GENERATION;
}

}  // namespace argus
