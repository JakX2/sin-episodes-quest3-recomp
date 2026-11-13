#pragma once
// Phase198: host-neutral reconstruction of SHA-verified native server.dll
// cache-tree left/right rotation pointer updates (2006 i386 PE32).
// This does not implement the original x86 calling convention, original
// node-resolver invocation counts, or full red-black insertion fixup.
#include <cstdint>
#include <stdexcept>
#include "sin_compat/original_server_cache_tree_lookup.hpp"

namespace sin_compat::original_server_cache_tree_rotations {
namespace tree = original_server_cache_tree_lookup;
inline constexpr std::uint16_t kNil = 0xffffu;
enum class Direction { left, right };
struct Config {
    std::uint32_t native_entry_va;
    std::uint32_t native_resolver_va;
    Direction direction;
};
inline constexpr Config kSceneLeft{0x221A6BB0u,0x221A4D90u,Direction::left};
inline constexpr Config kSceneRight{0x221A6CA0u,0x221A4D90u,Direction::right};
inline constexpr Config kModelSoundsLeft{0x221409B0u,0x2213E990u,Direction::left};
inline constexpr Config kModelSoundsRight{0x22140AA0u,0x2213E990u,Direction::right};

inline std::uint16_t parent(const tree::NativeSlot32& n) noexcept {
    return static_cast<std::uint16_t>(n.unknown_04);
}
inline std::uint16_t color(const tree::NativeSlot32& n) noexcept {
    return static_cast<std::uint16_t>(n.unknown_04 >> 16u);
}
inline void set_parent(tree::NativeSlot32& n, std::uint16_t p) noexcept {
    n.unknown_04 = (n.unknown_04 & 0xffff0000u)
                   | static_cast<std::uint32_t>(p);
}

// Original input: ECX=tree, one stack argument (raw node index), RET 4.
// Left rotation: y=x.right; beta=y.left; x.right=beta; beta.parent=x;
// y.parent=x.parent; old parent/root points to y; y.left=x; x.parent=y.
// Right rotation mirrors left/right. Parent and children are 16-bit indices
// in a 24-byte slot. Original code does NOT alter color/key/payload here.
// The original code performs up to nine calls to the Phase195 resolver,
// whose lazy static-sentinel initialization is handled separately.
// These pre-validation checks are HOST-ONLY and absent in native x86.
// If invalid, reject BEFORE any mutation rather than corrupt host memory.
inline void rotate(tree::Tree& t, std::uint32_t raw_x, Direction d) {
    const auto x = static_cast<std::uint16_t>(raw_x);
    if (x == kNil || x >= t.nodes.size())
        throw std::out_of_range("original cache rotation: missing pivot");
    const auto y = d == Direction::left
                  ? t.nodes[x].right_02 : t.nodes[x].left_00;
    if (y == kNil || y >= t.nodes.size() || y == x)
        throw std::out_of_range("original cache rotation: missing promotion child");
    const auto beta = d == Direction::left
                     ? t.nodes[y].left_00 : t.nodes[y].right_02;
    const auto p = parent(t.nodes[x]);
    if ((beta != kNil && (beta >= t.nodes.size() || beta == x || beta == y))
        || (p != kNil && (p >= t.nodes.size() || p == x || p == y))
        || parent(t.nodes[y]) != x
        || (beta != kNil && parent(t.nodes[beta]) != y)
        || ((p == kNil) != (t.root == x))) {
        throw std::out_of_range("original cache rotation: inconsistent synthetic topology");
    }
    if (p != kNil) {
        const auto& gp = t.nodes[p];
        const bool on_left = gp.left_00 == x;
        const bool on_right = gp.right_02 == x;
        if (on_left == on_right)
            throw std::out_of_range("original cache rotation: pivot not unique child");
    }

    if (d == Direction::left) t.nodes[x].right_02 = beta;
    else t.nodes[x].left_00 = beta;
    if (beta != kNil) set_parent(t.nodes[beta], x);
    set_parent(t.nodes[y], p);
    if (p == kNil) {
        t.root = y;
    } else if (t.nodes[p].left_00 == x) {
        t.nodes[p].left_00 = y;
    } else {
        t.nodes[p].right_02 = y;
    }
    if (d == Direction::left) t.nodes[y].left_00 = x;
    else t.nodes[y].right_02 = x;
    set_parent(t.nodes[x], y);
}

inline void rotate(tree::Tree& t, std::uint32_t raw_x, const Config& cfg) {
    rotate(t, raw_x, cfg.direction);
}
} // namespace sin_compat::original_server_cache_tree_rotations
