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
    std::array<std::uint8_t, 8> unknown_08{};
    std::uint16_t token = 0;                  // original +0x10
    std::array<std::uint8_t, 2> unknown_12{};
    std::uint32_t converter_native32 = 0;    // original +0x14: DO NOT dereference on new hosts
};
static_assert(sizeof(NodeRecord) == 24, "2006 client converter record stride");
static_assert(offsetof(NodeRecord, smaller_child) == 0x00);
static_assert(offsetof(NodeRecord, larger_child) == 0x04);
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
} // namespace sin_compat::original_client_converter