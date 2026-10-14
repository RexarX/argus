#pragma once

#include <argus/assert.hpp>
#include <argus/entity.hpp>

#include <cstddef>
#include <memory_resource>
#include <utility>
#include <vector>

namespace argus::gui {

/// Sparse set storing one component of type `T` per entity index.
/// `sparse[entity_index]` is the position in `dense`, or
/// `INVALID_ENTITY_INDEX` when the entity has no component.
template <typename T>
struct Component_Pool {
    std::pmr::vector<T> dense{std::pmr::get_default_resource()};
    /// Dense index -> Entity_Index mapping
    std::pmr::vector<Entity_Index> dense_to_entity{
        std::pmr::get_default_resource()};
    /// Entity_Index -> dense index mapping
    std::pmr::vector<Entity_Index> sparse{std::pmr::get_default_resource()};
};

/// Result of `try_assign_component`.
template <typename T>
struct Try_Assign_Result {
    /// Component stored for the index after the call, or `nullptr` when the
    /// `sparse` slot does not exist.
    T* component = nullptr;
    /// `true` when the provided component was stored, `false` when the
    /// existing component was retained or no slot exists.
    bool inserted = false;
};

namespace detail {

/// `true` when `sparse` covers `index` (the slot exists, occupied or not).
template <typename T>
[[nodiscard]] inline bool has_slot(const Component_Pool<T>& component_pool,
                                   Entity_Index index) noexcept {
    return static_cast<size_t>(index) < component_pool.sparse.size();
}

/// Appends `component` to `dense` and links it to `index`.
/// Requires an existing, empty slot.
template <typename T>
inline T* append_component_impl(Component_Pool<T>* component_pool,
                                Entity_Index index, T component) noexcept {
    ARGUS_ASSERT(has_slot(*component_pool, index));
    ARGUS_ASSERT(component_pool->sparse[index] == INVALID_ENTITY_INDEX);

    component_pool->sparse[index] =
        static_cast<Entity_Index>(component_pool->dense.size());
    component_pool->dense_to_entity.push_back(index);
    return &component_pool->dense.emplace_back(std::move(component));
}

template <typename T>
inline void remove_component_impl(Component_Pool<T>* component_pool,
                                  Entity_Index index) noexcept {
    const Entity_Index slot = component_pool->sparse[index];
    const auto last =
        static_cast<Entity_Index>(component_pool->dense.size() - 1);

    if (slot != last) {
        component_pool->dense[slot] = std::move(component_pool->dense[last]);
        const Entity_Index moved_entity = component_pool->dense_to_entity[last];
        component_pool->dense_to_entity[slot] = moved_entity;
        component_pool->sparse[moved_entity] = slot;
    }

    component_pool->dense.pop_back();
    component_pool->dense_to_entity.pop_back();
    component_pool->sparse[index] = INVALID_ENTITY_INDEX;
}

}  // namespace detail

/// @brief Creates an empty component pool whose containers use `allocator`.
/// @param allocator Resource for `dense`, `dense_to_entity` and `sparse`
/// @return Empty component pool
/// @warning Asserts if `allocator` is null.
template <typename T>
[[nodiscard]] inline auto create_component_pool_with(
    std::pmr::memory_resource* allocator) noexcept -> Component_Pool<T> {
    ARGUS_ASSERT(allocator != nullptr);
    return {
        .dense = std::pmr::vector<T>{allocator},
        .dense_to_entity = std::pmr::vector<Entity_Index>{allocator},
        .sparse = std::pmr::vector<Entity_Index>{allocator},
    };
}

/// @brief Removes every component.
/// @param component_pool Pool to clear
/// @warning Asserts if `component_pool` is `nullptr`.
template <typename T>
inline void clear(Component_Pool<T>* component_pool) noexcept {
    ARGUS_ASSERT(component_pool != nullptr);

    component_pool->dense.clear();
    component_pool->dense_to_entity.clear();
    component_pool->sparse.clear();
}

/// @brief Reserves storage to minimize allocations.
/// @details Never shrinks. New `sparse` slots are marked empty.
/// @param component_pool Pool to modify
/// @param components Number of components to reserve space for
/// @param entity_slots Number of entity indices `sparse` should cover
/// (typically `Entity_Manager::generations.size()`)
/// @warning Asserts if `component_pool` is `nullptr`.
template <typename T>
inline void reserve(Component_Pool<T>* component_pool, size_t components,
                    size_t entity_slots = 0) noexcept {
    ARGUS_ASSERT(component_pool != nullptr);

    component_pool->dense.reserve(components);
    component_pool->dense_to_entity.reserve(components);
    if (entity_slots > component_pool->sparse.size()) {
        component_pool->sparse.resize(entity_slots, INVALID_ENTITY_INDEX);
    }
}

/// @brief Creates the `sparse` slot for `index` so a component can be
/// assigned to it.
/// @details Grows `sparse` to cover `index`, new slots are empty. Does nothing
/// if the slot already exists. Never shrinks.
/// @param component_pool Pool to modify
/// @param index Entity index
/// @warning Asserts in next cases:
/// - If `component_pool` is `nullptr`.
/// - If `index` is `INVALID_ENTITY_INDEX`.
/// @warning Not thread-safe.
template <typename T>
inline void create_component_slot(Component_Pool<T>* component_pool,
                                  Entity_Index index) noexcept {
    ARGUS_ASSERT(component_pool != nullptr);
    ARGUS_ASSERT(index != INVALID_ENTITY_INDEX);

    if (!detail::has_slot(*component_pool, index)) {
        component_pool->sparse.resize(static_cast<size_t>(index) + 1,
                                      INVALID_ENTITY_INDEX);
    }
}

/// @brief Stores `component` for `index`, overwriting an existing one.
/// @details Requires the slot for `index` to exist, see `create_component_slot`
/// and `reserve`.
/// @param component_pool Pool to modify
/// @param index Entity index
/// @param component Component to store
/// @return Pointer to the stored component
/// @warning Asserts in next cases:
/// - If `component_pool` is `nullptr`.
/// - If `index` is `INVALID_ENTITY_INDEX`.
/// - If the slot for `index` does not exist.
/// @warning Not thread-safe. References to components are invalidated.
template <typename T>
inline T* assign_component(Component_Pool<T>* component_pool,
                           Entity_Index index, T component) noexcept {
    ARGUS_ASSERT(component_pool != nullptr);
    ARGUS_ASSERT(index != INVALID_ENTITY_INDEX);
    ARGUS_ASSERT(detail::has_slot(*component_pool, index));

    const Entity_Index existing = component_pool->sparse[index];
    if (existing != INVALID_ENTITY_INDEX) {
        component_pool->dense[existing] = std::move(component);
        return &component_pool->dense[existing];
    }
    return detail::append_component_impl(component_pool, index,
                                         std::move(component));
}

/// @brief Stores `component` for `index` only if its slot exists and is empty.
/// @details Never overwrites and never creates slots. Outcomes:
/// - Slot empty: component stored, `inserted == true`.
/// - Slot occupied: old component retained, `inserted == false`, `component`
///   points to the old one.
/// - Slot missing: nothing happens, `component == nullptr`,
///   `inserted == false`.
/// @param component_pool Pool to modify
/// @param index Entity index
/// @param component Component to store
/// @return Pointer to the component now stored for `index` and whether the
/// provided component was the one stored
/// @warning Asserts in next cases:
/// - If `component_pool` is `nullptr`.
/// - If `index` is `INVALID_ENTITY_INDEX`.
/// @warning Not thread-safe. References to components are invalidated when
/// `inserted` is `true`.
template <typename T>
inline auto try_assign_component(Component_Pool<T>* component_pool,
                                 Entity_Index index, T component) noexcept
    -> Try_Assign_Result<T> {
    ARGUS_ASSERT(component_pool != nullptr);
    ARGUS_ASSERT(index != INVALID_ENTITY_INDEX);

    if (!detail::has_slot(*component_pool, index)) {
        return {.component = nullptr, .inserted = false};
    }

    const Entity_Index existing = component_pool->sparse[index];
    if (existing != INVALID_ENTITY_INDEX) {
        return {.component = &component_pool->dense[existing],
                .inserted = false};
    }
    return {.component = detail::append_component_impl(component_pool, index,
                                                       std::move(component)),
            .inserted = true};
}

/// @brief Removes the component of `index` (swap-and-pop).
/// @param component_pool Pool to modify
/// @param index Entity index
/// @warning Asserts in next cases:
/// - If `component_pool` is `nullptr`.
/// - If `index` is `INVALID_ENTITY_INDEX` or out of bounds for
/// `component_pool->sparse`.
/// - If `index` has no component.
/// @warning Not thread-safe. Moves the last component into the freed slot.
template <typename T>
inline void remove_component(Component_Pool<T>* component_pool,
                             Entity_Index index) noexcept {
    ARGUS_ASSERT(component_pool != nullptr);
    ARGUS_ASSERT(index != INVALID_ENTITY_INDEX);
    ARGUS_ASSERT(has_component(*component_pool, index));

    detail::remove_component_impl(component_pool, index);
}

/// @brief Removes the component of `index` (swap-and-pop).
/// @param component_pool Pool to modify
/// @param index Entity index
/// @return `true` when a component was removed, `false` when none existed
/// @warning Asserts in next cases:
/// - If `component_pool` is `nullptr`.
/// - If `index` is `INVALID_ENTITY_INDEX`
/// @warning Not thread-safe. Moves the last component into the freed slot.
template <typename T>
inline bool try_remove_component(Component_Pool<T>* component_pool,
                                 Entity_Index index) noexcept {
    ARGUS_ASSERT(component_pool != nullptr);
    ARGUS_ASSERT(index != INVALID_ENTITY_INDEX);

    if (!has_component(*component_pool, index)) {
        return false;
    }
    detail::remove_component_impl(component_pool, index);
    return true;
}

/// @brief Checks whether the entity index has a component in the pool.
/// @param component_pool Pool to query
/// @param index Entity index
/// @return `true` when a component is stored for `index`
template <typename T>
[[nodiscard]] inline bool has_component(const Component_Pool<T>& component_pool,
                                        Entity_Index index) noexcept {
    return index < component_pool.sparse.size() &&
           component_pool.sparse[index] != INVALID_ENTITY_INDEX;
}

/// @brief Returns the component of `index`, or `nullptr` if there is none.
/// @param component_pool Pool to query
/// @param index Entity index
/// @return Pointer to the component, invalidated by `assign_component` and
/// `remove_component`
/// @warning Asserts if `component_pool` is `nullptr`.
template <typename T>
[[nodiscard]] inline T* get_component(Component_Pool<T>* component_pool,
                                      Entity_Index index) noexcept {
    ARGUS_ASSERT(component_pool != nullptr);
    if (!has_component(*component_pool, index)) {
        return nullptr;
    }
    return &component_pool->dense[component_pool->sparse[index]];
}

/// @brief Returns the component of `index`, or `nullptr` if there is none.
/// @param component_pool Pool to query
/// @param index Entity index
/// @return Const pointer to the component, invalidated by
/// `assign_component` and `remove_component`
template <typename T>
[[nodiscard]] inline const T* get_component(
    const Component_Pool<T>& component_pool, Entity_Index index) noexcept {
    if (!has_component(component_pool, index)) {
        return nullptr;
    }
    return &component_pool.dense[component_pool.sparse[index]];
}

}  // namespace argus::gui
