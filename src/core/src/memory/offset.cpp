#include <argus/memory/offset.hpp>

#include <argus/assert.hpp>
#include <argus/memory/aligned_alloc.hpp>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace argus::mem {

namespace {

struct Node {
    static constexpr uint32_t UNUSED = OFFSET_NO_SPACE;

    uint32_t data_offset = 0;
    uint32_t size = 0;
    uint32_t bin_prev = UNUSED;
    uint32_t bin_next = UNUSED;
    uint32_t neighbor_prev = UNUSED;
    uint32_t neighbor_next = UNUSED;
};

[[nodiscard]] constexpr uint32_t find_lowest_set_bit_after(
    uint32_t bit_mask, uint32_t start_bit) noexcept {
    if (start_bit >= 32) {
        return OFFSET_NO_SPACE;
    }
    const uint32_t bits_after = bit_mask & (~0U << start_bit);
    if (bits_after == 0) {
        return OFFSET_NO_SPACE;
    }
    return std::countr_zero(bits_after);
}

constexpr uint32_t MANTISSA_BITS = 3;
constexpr uint32_t MANTISSA_VALUE = 1U << MANTISSA_BITS;
constexpr uint32_t MANTISSA_MASK = MANTISSA_VALUE - 1;

[[nodiscard]] constexpr uint32_t uint_to_float_round_up(
    uint32_t size) noexcept {
    if (size < MANTISSA_VALUE) {
        return size;
    }

    const uint32_t leading_zeros = std::countl_zero(size);
    const uint32_t highest_set_bit = 31U - leading_zeros;
    const uint32_t mantissa_start_bit = highest_set_bit - MANTISSA_BITS;
    uint32_t mantissa = (size >> mantissa_start_bit) & MANTISSA_MASK;
    const uint32_t low_bits_mask = (1U << mantissa_start_bit) - 1U;
    if ((size & low_bits_mask) != 0) {
        ++mantissa;
    }
    const uint32_t exp = mantissa_start_bit + 1U;
    return (exp << MANTISSA_BITS) + mantissa;
}

[[nodiscard]] constexpr uint32_t uint_to_float_round_down(
    uint32_t size) noexcept {
    if (size < MANTISSA_VALUE) {
        return size;
    }

    const uint32_t leading_zeros = std::countl_zero(size);
    const uint32_t highest_set_bit = 31U - leading_zeros;
    const uint32_t mantissa_start_bit = highest_set_bit - MANTISSA_BITS;
    const uint32_t mantissa = (size >> mantissa_start_bit) & MANTISSA_MASK;
    const uint32_t exp = mantissa_start_bit + 1U;
    return (exp << MANTISSA_BITS) | mantissa;
}

[[nodiscard]] constexpr uint32_t float_to_uint(uint32_t float_value) noexcept {
    const uint32_t exponent = float_value >> MANTISSA_BITS;
    const uint32_t mantissa = float_value & MANTISSA_MASK;
    if (exponent == 0) {
        return mantissa;
    }
    return (mantissa | MANTISSA_VALUE) << (exponent - 1U);
}

[[nodiscard]] Node* get_nodes(Offset* offset) noexcept {
    return reinterpret_cast<Node*>(offset->storage);
}

[[nodiscard]] const Node* get_nodes(const Offset* offset) noexcept {
    return reinterpret_cast<const Node*>(offset->storage);
}

[[nodiscard]] uint8_t* get_used_bits(Offset* offset) noexcept {
    return reinterpret_cast<uint8_t*>(offset->storage +
                                      (sizeof(Node) * offset->max_allocs));
}

[[nodiscard]] const uint8_t* get_used_bits(const Offset* offset) noexcept {
    return reinterpret_cast<const uint8_t*>(
        offset->storage + (sizeof(Node) * offset->max_allocs));
}

[[nodiscard]] bool is_used(const Offset* offset, uint32_t index) noexcept {
    return (get_used_bits(offset)[index >> 3U] &
            static_cast<uint8_t>(1U << (index & 7U))) != 0;
}

void set_used(Offset* offset, uint32_t index, bool used) noexcept {
    uint8_t& byte = get_used_bits(offset)[index >> 3U];
    const uint8_t mask = static_cast<uint8_t>(1U << (index & 7U));
    const bool was_used = (byte & mask) != 0;
    if (used) {
        byte = static_cast<uint8_t>(byte | mask);
        if (!was_used) {
            ++offset->used_count;
        }
    } else {
        byte = static_cast<uint8_t>(byte & static_cast<uint8_t>(~mask));
        if (was_used) {
            --offset->used_count;
        }
    }
}

[[nodiscard]] constexpr size_t get_storage_bytes(uint32_t max_allocs) noexcept {
    return (sizeof(Node) * max_allocs) +
           ((static_cast<size_t>(max_allocs) + 7U) / 8U);
}

uint32_t insert_node_into_bin(Offset* offset, uint32_t size,
                              uint32_t data_offset) noexcept {
    const uint32_t bin_index = uint_to_float_round_down(size);
    const uint32_t top_bin = bin_index >> OFFSET_TOP_BIN_SHIFT;
    const uint32_t leaf_bin = bin_index & OFFSET_LEAF_BIN_MASK;

    if (offset->bin_indices[bin_index] == Node::UNUSED) {
        offset->used_bins[top_bin] =
            static_cast<uint8_t>(offset->used_bins[top_bin] | (1U << leaf_bin));
        offset->used_bins_top |= 1U << top_bin;
    }

    Node* const nodes = get_nodes(offset);
    const uint32_t top_node_index = offset->bin_indices[bin_index];
    const uint32_t node_index = offset->free_nodes[offset->free_offset--];
    nodes[node_index] = Node{
        .data_offset = data_offset,
        .size = size,
        .bin_prev = Node::UNUSED,
        .bin_next = top_node_index,
    };
    set_used(offset, node_index, false);
    if (top_node_index != Node::UNUSED) {
        nodes[top_node_index].bin_prev = node_index;
    }
    offset->bin_indices[bin_index] = node_index;
    offset->free_bytes += size;
    return node_index;
}

void remove_node_from_bin(Offset* offset, uint32_t node_index) noexcept {
    Node* const nodes = get_nodes(offset);
    Node& node = nodes[node_index];
    if (node.bin_prev != Node::UNUSED) {
        nodes[node.bin_prev].bin_next = node.bin_next;
        if (node.bin_next != Node::UNUSED) {
            nodes[node.bin_next].bin_prev = node.bin_prev;
        }
    } else {
        const uint32_t bin_index = uint_to_float_round_down(node.size);
        const uint32_t top_bin = bin_index >> OFFSET_TOP_BIN_SHIFT;
        const uint32_t leaf_bin = bin_index & OFFSET_LEAF_BIN_MASK;
        offset->bin_indices[bin_index] = node.bin_next;
        if (node.bin_next != Node::UNUSED) {
            nodes[node.bin_next].bin_prev = Node::UNUSED;
        }
        if (offset->bin_indices[bin_index] == Node::UNUSED) {
            offset->used_bins[top_bin] = static_cast<uint8_t>(
                offset->used_bins[top_bin] & ~(1U << leaf_bin));
            if (offset->used_bins[top_bin] == 0) {
                offset->used_bins_top &= ~(1U << top_bin);
            }
        }
    }

    offset->free_nodes[++offset->free_offset] = node_index;
    offset->free_bytes -= node.size;
}

}  // namespace

void init(Offset* offset, uint32_t size, uint32_t max_allocs) noexcept {
    ARGUS_ASSERT(offset != nullptr);
    ARGUS_ASSERT(offset->storage == nullptr,
                 "Offset allocator is already initialized!");
    ARGUS_ASSERT(size != 0);
    ARGUS_ASSERT(max_allocs != 0);

    const size_t storage_bytes = get_storage_bytes(max_allocs);
    const size_t nodes_bytes =
        sizeof(uint32_t) * static_cast<size_t>(max_allocs);
    void* const storage = aligned_alloc(alignof(Node), storage_bytes);
    void* const free_nodes = aligned_alloc(alignof(uint32_t), nodes_bytes);
    if (storage == nullptr || free_nodes == nullptr) [[unlikely]] {
        aligned_free(storage);
        aligned_free(free_nodes);
        ARGUS_ASSERT(false, "Failed to allocate offset allocator!");
    }

    offset->storage = static_cast<std::byte*>(storage);
    offset->free_nodes = static_cast<uint32_t*>(free_nodes);
    offset->size = size;
    offset->max_allocs = max_allocs;
    reset(offset);
}

void destroy(Offset* offset) noexcept {
    ARGUS_ASSERT(offset != nullptr);
    aligned_free(offset->storage);
    aligned_free(offset->free_nodes);
    offset->storage = nullptr;
    offset->free_nodes = nullptr;
    offset->size = 0;
    offset->max_allocs = 0;
    offset->free_bytes = 0;
    offset->used_count = 0;
}

void reset(Offset* offset) noexcept {
    ARGUS_ASSERT(offset != nullptr);
    ARGUS_ASSERT(offset->storage != nullptr,
                 "Offset allocator is not initialized!");
    ARGUS_ASSERT(offset->size != 0);
    ARGUS_ASSERT(offset->max_allocs != 0);

    offset->free_bytes = 0;
    offset->used_bins_top = 0;
    offset->free_offset = offset->max_allocs - 1;
    offset->used_count = 0;
    offset->used_bins.fill(0);
    offset->bin_indices.fill(Node::UNUSED);
    std::memset(get_used_bits(offset), 0,
                (static_cast<size_t>(offset->max_allocs) + 7U) / 8U);

    for (uint32_t index = 0; index < offset->max_allocs; ++index) {
        offset->free_nodes[index] = offset->max_allocs - index - 1;
    }

    insert_node_into_bin(offset, offset->size, 0);
}

Offset_Allocation allocate(Offset* offset, uint32_t size) noexcept {
    ARGUS_ASSERT(offset != nullptr);
    ARGUS_ASSERT(offset->storage != nullptr,
                 "Offset allocator is not initialized!");
    ARGUS_ASSERT(size != 0);
    if (offset->free_offset == 0) {
        return {};
    }

    const uint32_t min_bin_index = uint_to_float_round_up(size);
    const uint32_t min_top_bin = min_bin_index >> OFFSET_TOP_BIN_SHIFT;
    const uint32_t min_leaf_bin = min_bin_index & OFFSET_LEAF_BIN_MASK;

    uint32_t top_bin = min_top_bin;
    uint32_t leaf_bin = OFFSET_NO_SPACE;

    if ((offset->used_bins_top & (1U << top_bin)) != 0) {
        leaf_bin =
            find_lowest_set_bit_after(offset->used_bins[top_bin], min_leaf_bin);
    }

    if (leaf_bin == OFFSET_NO_SPACE) {
        top_bin =
            find_lowest_set_bit_after(offset->used_bins_top, min_top_bin + 1);
        if (top_bin == OFFSET_NO_SPACE) {
            return {};
        }
        leaf_bin = std::countr_zero(offset->used_bins[top_bin]);
    }

    Node* const nodes = get_nodes(offset);
    const uint32_t bin_index = (top_bin << OFFSET_TOP_BIN_SHIFT) | leaf_bin;
    const uint32_t node_index = offset->bin_indices[bin_index];
    Node& node = nodes[node_index];
    const uint32_t node_total_size = node.size;
    node.size = size;
    set_used(offset, node_index, true);
    offset->bin_indices[bin_index] = node.bin_next;
    if (node.bin_next != Node::UNUSED) {
        nodes[node.bin_next].bin_prev = Node::UNUSED;
    }
    offset->free_bytes -= node_total_size;

    if (offset->bin_indices[bin_index] == Node::UNUSED) {
        offset->used_bins[top_bin] = static_cast<uint8_t>(
            offset->used_bins[top_bin] & ~(1U << leaf_bin));
        if (offset->used_bins[top_bin] == 0) {
            offset->used_bins_top &= ~(1U << top_bin);
        }
    }

    const uint32_t remainder = node_total_size - size;
    if (remainder > 0) {
        const uint32_t new_index =
            insert_node_into_bin(offset, remainder, node.data_offset + size);
        if (node.neighbor_next != Node::UNUSED) {
            nodes[node.neighbor_next].neighbor_prev = new_index;
        }
        nodes[new_index].neighbor_prev = node_index;
        nodes[new_index].neighbor_next = node.neighbor_next;
        node.neighbor_next = new_index;
    }

    return {.offset = node.data_offset, .metadata = node_index};
}

void free(Offset* offset, Offset_Allocation allocation) noexcept {
    ARGUS_ASSERT(offset != nullptr, "Free needs an offset allocator");
    if (!allocation) [[unlikely]] {
        return;
    }
    ARGUS_ASSERT(offset->storage != nullptr,
                 "Offset allocator is not initialized!");
    ARGUS_ASSERT(allocation.metadata != OFFSET_NO_SPACE,
                 "Allocation was not taken from an offset allocator!");

    Node* const nodes = get_nodes(offset);
    const uint32_t node_index = allocation.metadata;
    Node& node = nodes[node_index];
    ARGUS_ASSERT(is_used(offset, node_index),
                 "Allocation is not live in this offset allocator!");

    uint32_t data_offset = node.data_offset;
    uint32_t size = node.size;

    if (node.neighbor_prev != Node::UNUSED &&
        !is_used(offset, node.neighbor_prev)) {
        Node& prev = nodes[node.neighbor_prev];
        data_offset = prev.data_offset;
        size += prev.size;
        remove_node_from_bin(offset, node.neighbor_prev);
        ARGUS_ASSERT(prev.neighbor_next == node_index,
                     "Offset neighbor list is corrupt!");
        node.neighbor_prev = prev.neighbor_prev;
    }

    if (node.neighbor_next != Node::UNUSED &&
        !is_used(offset, node.neighbor_next)) {
        Node& next = nodes[node.neighbor_next];
        size += next.size;
        remove_node_from_bin(offset, node.neighbor_next);
        ARGUS_ASSERT(next.neighbor_prev == node_index,
                     "Offset neighbor list is corrupt!");
        node.neighbor_next = next.neighbor_next;
    }

    const uint32_t neighbor_next = node.neighbor_next;
    const uint32_t neighbor_prev = node.neighbor_prev;
    set_used(offset, node_index, false);
    offset->free_nodes[++offset->free_offset] = node_index;

    const uint32_t combined = insert_node_into_bin(offset, size, data_offset);
    if (neighbor_next != Node::UNUSED) {
        nodes[combined].neighbor_next = neighbor_next;
        nodes[neighbor_next].neighbor_prev = combined;
    }
    if (neighbor_prev != Node::UNUSED) {
        nodes[combined].neighbor_prev = neighbor_prev;
        nodes[neighbor_prev].neighbor_next = combined;
    }
}

uint32_t get_allocation_size(const Offset& offset,
                             Offset_Allocation allocation) noexcept {
    if (!allocation || offset.storage == nullptr) [[unlikely]] {
        return 0;
    }
    return get_nodes(&offset)[allocation.metadata].size;
}

Offset_Report get_report(const Offset& offset) noexcept {
    Offset_Report report{};
    if (offset.free_offset == 0) {
        return report;
    }
    report.free_bytes = offset.free_bytes;
    if (offset.used_bins_top != 0) {
        const uint32_t top_bin = 31U - std::countl_zero(offset.used_bins_top);
        const uint32_t leaf_bin =
            31U -
            std::countl_zero(static_cast<uint32_t>(offset.used_bins[top_bin]));
        report.largest_free_region =
            float_to_uint((top_bin << OFFSET_TOP_BIN_SHIFT) | leaf_bin);
    }
    return report;
}

Offset_Report_Full get_full_report(const Offset& offset) noexcept {
    Offset_Report_Full report{};
    const Node* const nodes = get_nodes(&offset);
    for (uint32_t index = 0; index < OFFSET_LEAF_BIN_COUNT; ++index) {
        uint32_t count = 0;
        uint32_t node_index = offset.bin_indices[index];
        while (node_index != Node::UNUSED) {
            node_index = nodes[node_index].bin_next;
            ++count;
        }
        report.free_regions[index] = {
            .size = float_to_uint(index),
            .count = count,
        };
    }
    return report;
}

}  // namespace argus::mem
