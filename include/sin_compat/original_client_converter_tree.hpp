#pragma once
// Independently authored bounded reference to original SiN 2006 client.dll
// converter binary-search tree, derived from verified original 0x2400A4D0
// and comparator 0x2400A3C0. NOT an engine-facing Source container.
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace sin_compat::original_client_converter {

// Original binary compares the 16-bit token at node+0x10, follows 32-bit
// child indices at +0/+4, and uses 24-byte (0x18) array record stride.
// Other 2006 native members are intentionally opaque and not mislabelled.
struct NodeRecord final {
    std::int32_t smaller_child = -1;          // original +0x00
    std::int32_t larger_child = -1;           // original +0x04
    std::int32_t parent_index = -1;          // original +0x08, Phase544
    std::uint32_t color = 0;                // original +0x0C; 0=red, 1=black
    std::uint16_t token = 0;                  // original +0x10
    std::array<std::uint8_t, 2> unknown_12{};
    std::uint32_t converter_native32 = 0;    // original +0x14: DO NOT dereference on new hosts
};
static_assert(sizeof(NodeRecord) == 24, "2006 client converter record stride");
static_assert(offsetof(NodeRecord, smaller_child) == 0x00);
static_assert(offsetof(NodeRecord, larger_child) == 0x04);
static_assert(offsetof(NodeRecord, parent_index) == 0x08);
static_assert(offsetof(NodeRecord, color) == 0x0C);
static_assert(offsetof(NodeRecord, token) == 0x10);
static_assert(offsetof(NodeRecord, converter_native32) == 0x14);

enum class LookupStatus : std::uint8_t { Found, NotFound, InvalidHostTree };
struct LookupResult final {
    LookupStatus status = LookupStatus::InvalidHostTree;
    std::int32_t node_index = -1;
};

// Original comparison callback 0x2400A3C0 implements uint16_t(a)<uint16_t(b).
constexpr bool less_than_16(std::uint16_t a, std::uint16_t b) noexcept {
    return a < b;
}

// Valid-tree branch order matches the original 0x2400A4D0. Added finite
// bounds/cycle protection is deliberately PORT-ONLY; x86 did not establish
// a safe error-return on malformed record indexes.
inline LookupResult find_original_token_tree(
    const std::vector<NodeRecord>& nodes,
    std::int32_t root_index,
    std::uint16_t query) noexcept {
    std::int32_t index = root_index;
    for (std::size_t step=0; step<=nodes.size(); ++step) {
        if (index == -1) return {LookupStatus::NotFound, -1};
        if (index < 0 || static_cast<std::size_t>(index) >= nodes.size())
            return {LookupStatus::InvalidHostTree, -1};
        const auto& node = nodes[static_cast<std::size_t>(index)];
        if (less_than_16(query, node.token)) index = node.smaller_child;
        else if (less_than_16(node.token, query)) index = node.larger_child;
        else return {LookupStatus::Found, index};
    }
    return {LookupStatus::InvalidHostTree, -1};
}

// The original 0x2400ABA0 insertion-position search differs from exact
// lookup 0x2400A4D0: it calls comparator(query,node) only, so equal
// tokens follow the larger-child branch. The original caller normally
// performed a duplicate lookup first. No C++ type names are inferred.
// InvalidHostTree/cycle refusal is strictly independent port safety.
enum class InsertSlotStatus : std::uint8_t { Position, InvalidHostTree };
struct InsertSlotResult final {
    InsertSlotStatus status = InsertSlotStatus::InvalidHostTree;
    std::int32_t parent_index = -1; // -1 means caller must set the root
    bool smaller_child = false;     // false on the empty-tree path
};
inline InsertSlotResult locate_original_converter_insert_slot(
    const std::vector<NodeRecord>& nodes,
    std::int32_t root_index,
    std::uint16_t token) noexcept {
    std::int32_t parent=-1;
    std::int32_t index=root_index;
    bool choose_smaller=false;
    for (std::size_t step=0;step<=nodes.size();++step) {
        if (index==-1) return {InsertSlotStatus::Position,parent,choose_smaller};
        if (index<0 || static_cast<std::size_t>(index)>=nodes.size())
            return {InsertSlotStatus::InvalidHostTree,-1,false};
        parent=index;
        const auto& node=nodes[static_cast<std::size_t>(index)];
        choose_smaller=less_than_16(token,node.token);
        index=choose_smaller?node.smaller_child:node.larger_child;
    }
    return {InsertSlotStatus::InvalidHostTree,-1,false};
}
} // namespace sin_compat::original_client_converter