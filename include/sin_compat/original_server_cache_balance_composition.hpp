#pragma once
// Phase199: compositional red-black insert fixup built from the SHA-reviewed
// 2006 SiN tree node layout (Phase191), link (196) and rotations (198).
//
// IMPORTANT: This is a conventional red-black FIXUP REFERENCE, not a claim
// that the original 1,387-byte Phase197 x86 function has been completely
// behaviorally recovered. No original binary code or Source engine ABI.
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include "sin_compat/original_server_cache_balance_preflight.hpp"
#include "sin_compat/original_server_cache_tree_rotations.hpp"

namespace sin_compat::original_server_cache_balance_composition {
namespace tree = original_server_cache_tree_lookup;
namespace prefix = original_server_cache_balance_preflight;
namespace rotations = original_server_cache_tree_rotations;
inline constexpr std::uint16_t kNil = 0xffffu;
inline constexpr std::uint16_t kRed = 0u;
inline constexpr std::uint16_t kBlack = 1u;

struct Result {
    std::uint32_t recolor_steps{};
    std::uint32_t left_rotations{};
    std::uint32_t right_rotations{};
    std::uint32_t loop_steps{};
};

// This wraps a working copy so malformed host topology/cycles/failures never
// leave a half-rotated caller tree. Native x86 has no such exception guarantee.
inline Result fixup(tree::Tree& input, std::uint32_t raw_inserted) {
    tree::Tree t = input;
    Result result{};
    const auto index = static_cast<std::uint16_t>(raw_inserted);
    const auto valid = [&t](std::uint16_t n) {
        return n != kNil && n < t.nodes.size();
    };
    const auto getparent = [&t](std::uint16_t n) {
        return rotations::parent(t.nodes[n]);
    };
    const auto getcolor = [&t](std::uint16_t n) {
        return rotations::color(t.nodes[n]);
    };
    const auto setblack = [&t](std::uint16_t n) {
        prefix::set_color(t.nodes[n], kBlack);
    };
    if (!valid(t.root) || !valid(index))
        throw std::out_of_range("cache fixup: invalid root/inserted node");
    std::uint16_t z = index;
    std::size_t fuel = t.nodes.size() * 2u + 2u;
    while (z != t.root) {
        if (fuel-- == 0u)
            throw std::length_error("cache fixup: cyclic parent chain");
        ++result.loop_steps;
        const auto p = getparent(z);
        if (!valid(p) || p == z)
            throw std::out_of_range("cache fixup: invalid parent");
        if (getcolor(p) == kBlack) break;
        if (getcolor(p) != kRed)
            throw std::out_of_range("cache fixup: invalid parent color");
        const auto g = getparent(p);
        if (!valid(g) || g == p || g == z)
            throw std::out_of_range("cache fixup: invalid grandparent");
        const bool parent_left = t.nodes[g].left_00 == p;
        const bool parent_right = t.nodes[g].right_02 == p;
        if (parent_left == parent_right || getparent(p) != g)
            throw std::out_of_range("cache fixup: invalid grandparent child");
        if ((t.nodes[p].left_00 == z) == (t.nodes[p].right_02 == z))
            throw std::out_of_range("cache fixup: inserted node not unique child");
        const auto uncle = parent_left ? t.nodes[g].right_02 : t.nodes[g].left_00;
        if (uncle != kNil && (!valid(uncle) || getparent(uncle) != g))
            throw std::out_of_range("cache fixup: invalid uncle");
        if (uncle != kNil && getcolor(uncle) == kRed) {
            setblack(p);
            setblack(uncle);
            prefix::set_color(t.nodes[g],kRed);
            ++result.recolor_steps;
            z = g;
            continue;
        }
        if (uncle != kNil && getcolor(uncle) != kBlack)
            throw std::out_of_range("cache fixup: invalid uncle color");
        if (parent_left) {
            if (t.nodes[p].right_02 == z) {
                rotations::rotate(t,p,rotations::Direction::left);
                ++result.left_rotations;
                z = p; // CLRS triangle normalization; p is now below former z
            }
            const auto final_parent = getparent(z);
            const auto final_grandparent = getparent(final_parent);
            if (!valid(final_parent) || !valid(final_grandparent))
                throw std::out_of_range("cache fixup: invalid post-triangle links");
            setblack(final_parent);
            prefix::set_color(t.nodes[final_grandparent],kRed);
            rotations::rotate(t,final_grandparent,rotations::Direction::right);
            ++result.right_rotations;
        } else {
            if (t.nodes[p].left_00 == z) {
                rotations::rotate(t,p,rotations::Direction::right);
                ++result.right_rotations;
                z = p;
            }
            const auto final_parent = getparent(z);
            const auto final_grandparent = getparent(final_parent);
            if (!valid(final_parent) || !valid(final_grandparent))
                throw std::out_of_range("cache fixup: invalid post-triangle links");
            setblack(final_parent);
            prefix::set_color(t.nodes[final_grandparent],kRed);
            rotations::rotate(t,final_grandparent,rotations::Direction::left);
            ++result.left_rotations;
        }
        break;
    }
    if (!valid(t.root)) throw std::out_of_range("cache fixup: invalid final root");
    setblack(t.root);
    input = std::move(t);
    return result;
}
} // namespace sin_compat::original_server_cache_balance_composition
