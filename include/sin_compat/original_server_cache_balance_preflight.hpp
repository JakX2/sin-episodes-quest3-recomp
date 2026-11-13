#pragma once
// Phase197: VERIFIED non-rotation prefixes of original 2006 server.dll
// tree insertion fixup. This is a fail-closed portable reference only.
// Actual native rotations/recoloring remain separate unrecovered methods.
#include <cstdint>
#include "sin_compat/original_server_cache_tree_lookup.hpp"

namespace sin_compat::original_server_cache_balance_preflight {
namespace tree = original_server_cache_tree_lookup;

inline constexpr std::uint32_t kSceneBalanceVA = 0x221A8F20u;
inline constexpr std::uint32_t kModelSoundsBalanceVA = 0x221429D0u;
inline constexpr std::uint32_t kSceneRotationHelperAVA = 0x221A6BB0u;
inline constexpr std::uint32_t kSceneRotationHelperBVA = 0x221A6CA0u;
inline constexpr std::uint32_t kModelSoundsRotationHelperAVA = 0x221409B0u;
inline constexpr std::uint32_t kModelSoundsRotationHelperBVA = 0x22140AA0u;
inline constexpr std::uint16_t kNil = 0xffffu;
inline constexpr std::uint16_t kBlack = 1u;
inline constexpr std::uint16_t kRed = 0u;

enum class Outcome {
    root_blackened,
    parent_already_black,
    requires_unimplemented_rotation_or_recolor,
    invalid_synthetic_tree,
};

// Original node WORD at offset +4 stores parent, WORD at +6 stores the
// color field. Phase191 modeled the two WORDs as one uint32_t named
// unknown_04. The root must finish black, including the fast-return paths.
inline std::uint16_t parent(const tree::NativeSlot32& node) noexcept {
    return static_cast<std::uint16_t>(node.unknown_04);
}
inline std::uint16_t color(const tree::NativeSlot32& node) noexcept {
    return static_cast<std::uint16_t>(node.unknown_04 >> 16);
}
inline void set_color(tree::NativeSlot32& node, std::uint16_t value) noexcept {
    node.unknown_04 = (node.unknown_04 & 0x0000ffffu)
                      | (static_cast<std::uint32_t>(value) << 16);
}

// Native: compare LOW WORD inserted index to root at tree +0x10;
// if equal, blacken root and return. Otherwise obtain new node's parent;
// if parent's WORD at +6 is nonzero, exit and blacken root. If parent
// is red, the original enters color/uncle/rotation logic that this model
// deliberately refuses to simulate. Invalid indexes are safe HOST checks,
// not native behavior. No partial mutation on refusal.
inline Outcome assess_nonrotating_case(tree::Tree& tree,
                                        std::uint32_t native_insert_index) {
    const auto inserted = static_cast<std::uint16_t>(native_insert_index);
    if (tree.root == kNil || tree.root >= tree.nodes.size()
        || inserted >= tree.nodes.size()) {
        return Outcome::invalid_synthetic_tree;
    }
    if (inserted == tree.root) {
        set_color(tree.nodes[tree.root], kBlack);
        return Outcome::root_blackened;
    }
    const std::uint16_t p = parent(tree.nodes[inserted]);
    if (p == kNil || p >= tree.nodes.size() || p == inserted) {
        return Outcome::invalid_synthetic_tree;
    }
    if (color(tree.nodes[p]) == kRed) {
        return Outcome::requires_unimplemented_rotation_or_recolor;
    }
    set_color(tree.nodes[tree.root], kBlack);
    return Outcome::parent_already_black;
}
} // namespace sin_compat::original_server_cache_balance_preflight
