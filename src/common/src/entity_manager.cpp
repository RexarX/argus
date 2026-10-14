#include <pch.hpp>

#include <argus/assert.hpp>
#include <argus/entity.hpp>
#include <argus/entity_manager.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>

namespace argus {

namespace {

/// Grows slot storage so that at least `required` slots exist. New slots hold
/// `INVALID_ENTITY_GENERATION`.
void grow_slots(Entity_Manager* entity_manager, size_t required) noexcept {
    ARGUS_ASSERT(entity_manager != nullptr);
    if (required <= entity_manager->generations.size()) {
        return;
    }
    entity_manager->generations.resize(required, INVALID_ENTITY_GENERATION);
}

/// Publishes `generation` for the existing slot `index` and counts it as
/// living.
Entity activate_slot(Entity_Manager* entity_manager, Entity_Index index,
                     Entity_Generation generation) noexcept {
    ARGUS_ASSERT(entity_manager != nullptr);
    ARGUS_ASSERT(index < entity_manager->generations.size());

    entity_manager->generations[index] = generation;
    ++entity_manager->count;

    return {.index = index, .generation = generation};
}

/// Pops the most recently freed index and revives its slot.
[[nodiscard]] Entity create_from_free_list(
    Entity_Manager* entity_manager) noexcept {
    ARGUS_ASSERT(entity_manager != nullptr);
    ARGUS_ASSERT(!entity_manager->free_indices.empty());

    const Entity_Index index = entity_manager->free_indices.back();
    entity_manager->free_indices.pop_back();
    entity_manager->free_cursor.store(
        static_cast<int64_t>(entity_manager->free_indices.size()),
        std::memory_order_relaxed);

    return activate_slot(
        entity_manager, index,
        get_revived_generation(entity_manager->generations[index]));
}

}  // namespace

void clear(Entity_Manager* entity_manager) noexcept {
    ARGUS_ASSERT(entity_manager != nullptr);

    entity_manager->generations.clear();
    entity_manager->free_indices.clear();
    entity_manager->count = 0;
    entity_manager->next_index.store(0, std::memory_order_relaxed);
    entity_manager->free_cursor.store(0, std::memory_order_relaxed);
}

void reserve(Entity_Manager* entity_manager, size_t count) noexcept {
    ARGUS_ASSERT(entity_manager != nullptr);
    grow_slots(entity_manager, count);
    entity_manager->free_indices.reserve(count);
}

void flush(Entity_Manager* entity_manager) noexcept {
    ARGUS_ASSERT(entity_manager != nullptr);

    const int64_t cursor =
        entity_manager->free_cursor.load(std::memory_order_relaxed);

    size_t new_cursor = 0;
    size_t flushed = 0;

    if (cursor >= 0) {
        new_cursor = static_cast<size_t>(cursor);
    } else {
        // Negative cursor: brand-new indices were reserved beyond next_index.
        const Entity_Index old_next =
            entity_manager->next_index.load(std::memory_order_relaxed);
        const auto fresh = static_cast<Entity_Index>(-cursor);
        const Entity_Index new_next = old_next + fresh;

        grow_slots(entity_manager, new_next);
        for (Entity_Index index = old_next; index < new_next; ++index) {
            entity_manager->generations[index] =
                INITIAL_ALIVE_ENTITY_GENERATION;
        }

        flushed += fresh;
        entity_manager->next_index.store(new_next, std::memory_order_relaxed);
    }

    // Reserved free-list entries live in free_indices[new_cursor .. end).
    // Their slots still hold the free generation written by destroy_entity;
    // publish the alive encoding now.
    ARGUS_ASSERT(new_cursor <= entity_manager->free_indices.size());
    const auto recycled =
        std::span<const Entity_Index>{entity_manager->free_indices}.subspan(
            new_cursor);
    for (const Entity_Index index : recycled) {
        entity_manager->generations[index] =
            get_revived_generation(entity_manager->generations[index]);
    }
    flushed += recycled.size();

    entity_manager->free_indices.resize(new_cursor);
    entity_manager->free_cursor.store(static_cast<int64_t>(new_cursor),
                                      std::memory_order_relaxed);
    entity_manager->count += flushed;
}

Entity create_entity(Entity_Manager* entity_manager) noexcept {
    ARGUS_ASSERT(entity_manager != nullptr);
    ARGUS_ASSERT(!needs_flush(*entity_manager));

    if (!entity_manager->free_indices.empty()) {
        return create_from_free_list(entity_manager);
    }

    const Entity_Index index =
        entity_manager->next_index.fetch_add(1, std::memory_order_relaxed);
    grow_slots(entity_manager, static_cast<size_t>(index) + 1);
    return activate_slot(entity_manager, index,
                         INITIAL_ALIVE_ENTITY_GENERATION);
}

void reserve_entities(Entity_Manager* entity_manager,
                      std::span<Entity> out) noexcept {
    ARGUS_ASSERT(entity_manager != nullptr);

    if (out.empty()) {
        return;
    }

    const int64_t cursor_before = entity_manager->free_cursor.fetch_sub(
        static_cast<int64_t>(out.size()), std::memory_order_relaxed);
    const auto next = static_cast<int64_t>(
        entity_manager->next_index.load(std::memory_order_relaxed));

    for (size_t i = 0; i < out.size(); ++i) {
        const int64_t slot = cursor_before - static_cast<int64_t>(i);

        if (slot > 0) {
            const Entity_Index index =
                entity_manager->free_indices[static_cast<size_t>(slot - 1)];
            out[i] = {.index = index,
                      .generation = get_revived_generation(
                          entity_manager->generations[index])};
        } else {
            out[i] = {.index = static_cast<Entity_Index>(next - slot),
                      .generation = INITIAL_ALIVE_ENTITY_GENERATION};
        }
    }
}

void create_entities(Entity_Manager* entity_manager,
                     std::span<Entity> out) noexcept {
    ARGUS_ASSERT(entity_manager != nullptr);
    ARGUS_ASSERT(!needs_flush(*entity_manager));

    size_t written = 0;

    // Satisfy as many as possible from the free list first.
    while (written < out.size() && !entity_manager->free_indices.empty()) {
        out[written++] = create_from_free_list(entity_manager);
    }

    // Allocate brand-new slots for the rest, growing storage only once.
    const size_t remaining = out.size() - written;
    if (remaining > 0) {
        const Entity_Index start = entity_manager->next_index.fetch_add(
            static_cast<Entity_Index>(remaining), std::memory_order_relaxed);
        const Entity_Index end = start + static_cast<Entity_Index>(remaining);

        grow_slots(entity_manager, end);
        for (Entity_Index index = start; index < end; ++index) {
            out[written++] = activate_slot(entity_manager, index,
                                           INITIAL_ALIVE_ENTITY_GENERATION);
        }
    }
}

Entity reserve_entity(Entity_Manager* entity_manager) noexcept {
    ARGUS_ASSERT(entity_manager != nullptr);

    // Atomically claim one slot: free list first, then brand-new indices.
    // Only `free_cursor` is mutated here; slot state is materialized in
    // `flush`.
    const int64_t n =
        entity_manager->free_cursor.fetch_sub(1, std::memory_order_relaxed);
    if (n > 0) {
        const Entity_Index index =
            entity_manager->free_indices[static_cast<size_t>(n - 1)];
        // The slot still stores the free generation. Hand out the future alive
        // generation, which deliberately does not match storage until `flush`.
        return {.index = index,
                .generation =
                    get_revived_generation(entity_manager->generations[index])};
    }

    // Cursor was 0 or negative: the more negative, the farther past
    // `next_index` the new index lies.
    const auto index = static_cast<Entity_Index>(
        static_cast<int64_t>(
            entity_manager->next_index.load(std::memory_order_relaxed)) -
        n);
    return {.index = index, .generation = INITIAL_ALIVE_ENTITY_GENERATION};
}

bool destroy_entity(Entity_Manager* entity_manager, Entity entity) noexcept {
    ARGUS_ASSERT(entity_manager != nullptr);
    ARGUS_ASSERT(is_valid(entity));
    ARGUS_ASSERT(!needs_flush(*entity_manager));

    if (!has_entity(*entity_manager, entity)) [[unlikely]] {
        return false;
    }

    entity_manager->generations[entity.index] =
        get_freed_generation(entity.generation);
    entity_manager->free_indices.push_back(entity.index);
    entity_manager->free_cursor.store(
        static_cast<int64_t>(entity_manager->free_indices.size()),
        std::memory_order_relaxed);
    --entity_manager->count;
    return true;
}

size_t destroy_entities(Entity_Manager* entity_manager,
                        std::span<const Entity> entities) noexcept {
    ARGUS_ASSERT(entity_manager != nullptr);
    ARGUS_ASSERT(!needs_flush(*entity_manager));

    entity_manager->free_indices.reserve(entity_manager->free_indices.size() +
                                         entities.size());

    size_t destroyed = 0;
    for (const Entity entity : entities) {
        destroyed +=
            static_cast<size_t>(destroy_entity(entity_manager, entity));
    }
    return destroyed;
}

}  // namespace argus
