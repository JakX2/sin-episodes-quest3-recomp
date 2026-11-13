#pragma once
// Phase199: host-neutral *candidate* for 2006 SiN cached-tree insertion fixup.
// Verified earlier: 16-bit parent/color offsets, red=0, black=1, root black,
// uncle tests, original left/right rotation topology. The exact 1,387-byte
// red-parent original code path has NOT yet been fully behaviorally verified.
// This reference uses conventional red-black cases and FAILS CLOSED on bad
// synthetic topology. No original x86 game engine ABI is implemented.
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>
#include "sin_compat/original_server_cache_balance_preflight.hpp"
#include "sin_compat/original_server_cache_tree_rotations.hpp"

namespace sin_compat::original_server_cache_insert_fixup {
namespace tree = original_server_cache_tree_lookup;
namespace balance = original_server_cache_balance_preflight;
namespace rotation = original_server_cache_tree_rotations;
inline constexpr std::uint16_t kNil = 0xffffu;

struct Config {
    std::uint32_t original_balance_va;
    std::uint32_t original_left_rotation_va;
    std::uint32_t original_right_rotation_va;
};
inline constexpr Config kScene{balance::kSceneBalanceVA,
    rotation::kSceneLeft.native_entry_va, rotation::kSceneRight.native_entry_va};
inline constexpr Config kModelSounds{balance::kModelSoundsBalanceVA,
    rotation::kModelSoundsLeft.native_entry_va,
    rotation::kModelSoundsRight.native_entry_va};

enum class Status { completed, malformed_host_tree, nonconvergent_host_tree };
struct Stats {
    std::uint32_t left_rotations{};
    std::uint32_t right_rotations{};
    std::uint32_t red_uncle_recolor_rounds{};
    std::uint32_t loop_iterations{};
};
struct Result { Status status; Stats stats; };

inline std::uint16_t parent(const tree::NativeSlot32& node) noexcept {
    return rotation::parent(node);
}
inline std::uint16_t color(const tree::NativeSlot32& node) noexcept {
    return rotation::color(node);
}
inline std::uint16_t child(const tree::NativeSlot32& node, bool first) noexcept {
    return first ? node.left_00 : node.right_02;
}
inline bool valid_index(const tree::Tree& t, std::uint16_t idx) noexcept {
    return idx != kNil && idx < t.nodes.size();
}
inline bool black_or_nil(const tree::Tree& t, std::uint16_t idx) noexcept {
    return idx == kNil || color(t.nodes[idx]) != balance::kRed;
}

// Host-only reachable-node validation. It protects the reference against
// invalid pointers/cycles, verifies parent linkage, and ignores free slots.
inline bool host_topology_valid(const tree::Tree& t, std::uint16_t inserted) {
    if (!valid_index(t,t.root) || !valid_index(t,inserted) ||
        parent(t.nodes[t.root]) != kNil) return false;
    struct Pending { std::uint16_t idx, expected_parent; };
    std::vector<Pending> pending{{t.root,kNil}};
    std::vector<std::uint8_t> visited(t.nodes.size(),0u);
    bool found = false;
    while (!pending.empty()) {
        const auto item = pending.back();
        pending.pop_back();
        if (!valid_index(t,item.idx) || visited[item.idx] != 0u) return false;
        visited[item.idx] = 1u;
        const auto& node = t.nodes[item.idx];
        if (parent(node) != item.expected_parent || color(node) > balance::kBlack)
            return false;
        if (item.idx == inserted) found = true;
        if (node.left_00 != kNil) pending.push_back({node.left_00,item.idx});
        if (node.right_02 != kNil) pending.push_back({node.right_02,item.idx});
    }
    return found;
}

// Conventional red-black insert fixup. This is an interoperability/reference
// hypothesis, NOT a claim that every original x86 branch has been recovered.
// Work on a copy so malformed synthetic input never partially mutates tree.
inline Result apply(tree::Tree& source, std::uint32_t raw_insert_index,
                    const Config&) {
    const auto inserted = static_cast<std::uint16_t>(raw_insert_index);
    if (!host_topology_valid(source,inserted))
        return {Status::malformed_host_tree,{}};
    tree::Tree t = source;
    Stats stats{};
    std::uint16_t z = inserted;
    bool stopped = false;
    try {
        for (std::size_t fuel = 0; fuel <= t.nodes.size(); ++fuel) {
            if (z == t.root) { stopped = true; break; }
            const auto p = parent(t.nodes[z]);
            if (!valid_index(t,p)) return {Status::malformed_host_tree,{}};
            if (color(t.nodes[p]) != balance::kRed) { stopped = true; break; }
            const auto g = parent(t.nodes[p]);
            if (!valid_index(t,g)) return {Status::malformed_host_tree,{}};
            const bool p_left = t.nodes[g].left_00 == p;
            if (p_left == (t.nodes[g].right_02 == p))
                return {Status::malformed_host_tree,{}};
            const auto uncle = child(t.nodes[g],!p_left);
            if (uncle != kNil && !valid_index(t,uncle))
                return {Status::malformed_host_tree,{}};
            ++stats.loop_iterations;
            if (!black_or_nil(t,uncle)) {
                balance::set_color(t.nodes[p],balance::kBlack);
                balance::set_color(t.nodes[uncle],balance::kBlack);
                balance::set_color(t.nodes[g],balance::kRed);
                ++stats.red_uncle_recolor_rounds;
                z = g;
                continue;
            }
            if (p_left) {
                if (z == t.nodes[p].right_02) {
                    z = p;
                    rotation::rotate(t,z,rotation::Direction::left);
                    ++stats.left_rotations;
                }
                const auto cp = parent(t.nodes[z]);
                if (!valid_index(t,cp)) return {Status::malformed_host_tree,{}};
                const auto cg = parent(t.nodes[cp]);
                if (!valid_index(t,cg)) return {Status::malformed_host_tree,{}};
                balance::set_color(t.nodes[cp],balance::kBlack);
                balance::set_color(t.nodes[cg],balance::kRed);
                rotation::rotate(t,cg,rotation::Direction::right);
                ++stats.right_rotations;
            } else {
                if (z == t.nodes[p].left_00) {
                    z = p;
                    rotation::rotate(t,z,rotation::Direction::right);
                    ++stats.right_rotations;
                }
                const auto cp = parent(t.nodes[z]);
                if (!valid_index(t,cp)) return {Status::malformed_host_tree,{}};
                const auto cg = parent(t.nodes[cp]);
                if (!valid_index(t,cg)) return {Status::malformed_host_tree,{}};
                balance::set_color(t.nodes[cp],balance::kBlack);
                balance::set_color(t.nodes[cg],balance::kRed);
                rotation::rotate(t,cg,rotation::Direction::left);
                ++stats.left_rotations;
            }
            stopped = true;
            break;
        }
    } catch (const std::out_of_range&) {
        return {Status::malformed_host_tree,{}};
    }
    if (!stopped) return {Status::nonconvergent_host_tree,{}};
    balance::set_color(t.nodes[t.root],balance::kBlack);
    if (!host_topology_valid(t,inserted))
        return {Status::malformed_host_tree,{}};
    source = std::move(t);
    return {Status::completed,stats};
}
} // namespace sin_compat::original_server_cache_insert_fixup
