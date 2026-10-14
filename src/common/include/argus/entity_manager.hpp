#pragma once

#include <argus/assert.hpp>
#include <argus/entity.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <span>
#include <vector>

namespace argus {

/// Pure index-based entity allocator. The slot index of an entity is its
/// position in `generations`; per-entity data (components) lives elsewhere,
/// keyed by `Entity::index`.
struct Entity_Manager {
    /// Encoded generation per slot (alive bit + reuse counter).
    std::pmr::vector<Entity_Generation> generations{
        std::pmr::get_default_resource()};
    /// Recycled slot indices. Entries at `[free_cursor, size)` are reserved
    /// but not yet flushed
    std::pmr::vector<Entity_Index> free_indices{
        std::pmr::get_default_resource()};

    /// Number of living entities
    size_t count = 0;
    /// Next never-used index (thread-safe)
    std::atomic<Entity_Index> next_index{0};
    /// Cursor for free list (negative means reserved brand-new indices)
    std::atomic<int64_t> free_cursor{0};
};

/// @brief Creates empty entity manager whose containers use `allocator`.
/// @param allocator Memory resource to use
/// @return `Entity_Manager` instance
/// @warning Asserts if `allocator` is null.
[[nodiscard]] inline Entity_Manager create_entity_manager_with(
    std::pmr::memory_resource* allocator) noexcept {
    ARGUS_ASSERT(allocator != nullptr);
    return {
        .generations = std::pmr::vector<Entity_Generation>{allocator},
        .free_indices = std::pmr::vector<Entity_Index>{allocator},
    };
}

/// @brief Destroys every entity and resets the manager to its initial state.
/// @details Reserved but unflushed indices are dropped as well.
/// @param entity_manager Entity manager to clear
/// @warning Asserts if `entity_manager` is `nullptr`.
/// @warning Not thread-safe.
void clear(Entity_Manager* entity_manager) noexcept;

/// @brief Reserves storage for `count` entity slots to minimize allocations.
/// @details Grows slot storage up to `count` without creating entities.
/// Never shrinks.
/// @param entity_manager Entity manager to modify
/// @param count Number of slots to reserve space for
/// @warning Asserts if `entity_manager` is `nullptr`.
/// @warning Not thread-safe.
void reserve(Entity_Manager* entity_manager, size_t count) noexcept;

/// @brief Materializes every entity handed out by `reserve_entity`.
/// @details Publishes the alive generation of each reserved slot. Afterwards
/// all reserved entities pass `has_entity`.
/// @param entity_manager Entity manager to modify
/// @warning Asserts in next cases:
/// - If `entity_manager` is `nullptr`.
/// - If the free cursor is out of sync with the free list.
/// @warning Not thread-safe.
void flush(Entity_Manager* entity_manager) noexcept;

/// @brief Checks whether reserved indices are awaiting `flush`.
/// @param entity_manager Entity manager to query
/// @return `true` when `reserve_entity` claimed slots that were not yet
/// materialized by `flush`
/// @warning Thread-safe during a stable reservation phase (no concurrent
/// structural operation).
[[nodiscard]] inline bool needs_flush(
    const Entity_Manager& entity_manager) noexcept {
    return entity_manager.free_cursor.load(std::memory_order_relaxed) !=
           static_cast<int64_t>(entity_manager.free_indices.size());
}

/// @brief Creates a new entity immediately.
/// @details Reuses a destroyed slot when available (performing the
/// free->alive transition synchronously), otherwise allocates a new slot.
/// @param entity_manager Entity manager to modify
/// @return New entity
/// @warning Asserts in next cases:
/// - If `entity_manager` is `nullptr`.
/// - If reserved indices have not been flushed (`needs_flush`).
/// @warning Not thread-safe. Slot storage may grow, so any external storage
/// indexed by entity index must be grown accordingly.
[[nodiscard]] Entity create_entity(Entity_Manager* entity_manager) noexcept;

/// @brief Creates `out.size()` entities at once.
/// @details Cheaper than repeated `create_entity` calls: slot storage grows at
/// most once. Entities are written to `out` in creation order.
/// @param entity_manager Entity manager to modify
/// @param out Destination for the created entities
/// @warning Asserts in next cases:
/// - If `entity_manager` is `nullptr`.
/// - If reserved indices have not been flushed (`needs_flush`).
/// @warning Not thread-safe.
///
/// @code
/// std::array<Entity, 16> data = {};
/// create_entities(&entity_manager, data);
/// @endcode
void create_entities(Entity_Manager* entity_manager,
                     std::span<Entity> out) noexcept;

/// @brief Reserves an entity that can be used immediately.
/// @details Prefers recycled slots, returning the future alive generation
/// (which deliberately does not match storage until `flush`). Otherwise hands
/// out a brand-new index with `INITIAL_ALIVE_ENTITY_GENERATION`. Slot state is
/// materialized by `flush`, so `has_entity` rejects the entity until then.
/// @param entity_manager Entity manager to reserve from
/// @return Reserved entity with valid index and (future) alive generation
/// @warning Asserts if `entity_manager` is `nullptr`.
/// @warning Thread-safe during a stable reservation phase (no concurrent
/// structural operation).
[[nodiscard]] Entity reserve_entity(Entity_Manager* entity_manager) noexcept;

/// @brief Reserves `out.size()` entities that can be used immediately.
/// @details Same semantics as repeated `reserve_entity` calls, but claims all
/// slots with a single atomic update to the free cursor.
/// @param entity_manager Entity manager to reserve from
/// @param out Destination for the reserved entities
/// @warning Asserts if `entity_manager` is `nullptr`.
/// @warning Thread-safe during a stable reservation phase (no concurrent
/// structural operation).
void reserve_entities(Entity_Manager* entity_manager,
                      std::span<Entity> out) noexcept;

/// @brief Destroys an entity.
/// @details Stores the free generation in its slot and adds the index to the
/// free list. Entities that do not refer to a live entity are ignored.
/// Components must be removed by the caller.
/// @param entity_manager Entity manager to modify
/// @param entity Entity to destroy
/// @return `true` when an entity was destroyed, `false` when `entity` was
/// stale or did not exist
/// @warning Asserts in next cases:
/// - If `entity_manager` is `nullptr`.
/// - If `entity` is invalid.
/// - If reserved indices have not been flushed (`needs_flush`).
/// @warning Not thread-safe.
bool destroy_entity(Entity_Manager* entity_manager, Entity entity) noexcept;

/// @brief Destroys multiple entities.
/// @details Behaves like repeated `destroy_entity` calls (a duplicate entity
/// is destroyed once and skipped the second time) but reserves free-list
/// storage up front.
/// @param entity_manager Entity manager to modify
/// @param entities Entities to destroy
/// @return Number of entities actually destroyed
/// @warning Asserts in next cases:
/// - If `entity_manager` is `nullptr`.
/// - If any entity is invalid.
/// - If reserved indices have not been flushed (`needs_flush`).
/// @warning Not thread-safe.
size_t destroy_entities(Entity_Manager* entity_manager,
                        std::span<const Entity> entities) noexcept;

/// @brief Checks whether `entity` refers to a live entity.
/// @details Requires a structurally valid entity, an in-range index, an exact
/// generation match and an alive-encoded stored generation. Reserved entities
/// that were not flushed yet are rejected.
/// @param entity_manager Entity manager to query
/// @param entity Entity to check
/// @return `true` when the entity exists and is alive, `false` otherwise
/// @warning Asserts if `entity` is invalid.
/// @warning Thread-safe during a stable reservation phase (no concurrent
/// structural operation).
[[nodiscard]] inline bool has_entity(const Entity_Manager& entity_manager,
                                     Entity entity) noexcept {
    ARGUS_ASSERT(is_valid(entity));
    if (!is_valid(entity) || entity.index >= entity_manager.generations.size())
        [[unlikely]] {
        return false;
    }
    const Entity_Generation stored = entity_manager.generations[entity.index];
    return stored == entity.generation && is_alive_generation(stored);
}

/// @brief Returns the encoded generation stored for `index`.
/// @details The value includes the alive/free bit tagging; it is not a raw
/// reuse counter.
/// @param entity_manager Entity manager to query
/// @param index Slot index
/// @return Stored generation
/// @warning Asserts if `index` is out of range.
/// @warning Thread-safe during a stable reservation phase (no concurrent
/// structural operation).
[[nodiscard]] inline Entity_Generation get_entity_generation(
    const Entity_Manager& entity_manager, Entity_Index index) noexcept {
    ARGUS_ASSERT(index < entity_manager.generations.size());
    return entity_manager.generations[index];
}

/// @brief Returns the encoded generation stored for `index`, if any.
/// @param entity_manager Entity manager to query
/// @param index Slot index
/// @return Stored generation, or `INVALID_ENTITY_GENERATION` when `index` is
/// out of range
/// @warning Thread-safe during a stable reservation phase (no concurrent
/// structural operation).
[[nodiscard]] inline Entity_Generation try_get_entity_generation(
    const Entity_Manager& entity_manager, Entity_Index index) noexcept {
    if (index >= entity_manager.generations.size()) {
        return INVALID_ENTITY_GENERATION;
    }
    return entity_manager.generations[index];
}

}  // namespace argus
