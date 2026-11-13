#pragma once
// Phase196: original 2006 server.dll scene/model-sounds tree link wrapper.
// The exact native 109-byte methods were SHA256-verified in original PE32.
// Host-neutral C++17 model; NOT the game DLL's x86 thiscall ABI or balancing.
#include <cstdint>
#include <stdexcept>
#include "sin_compat/original_server_cache_tree_lookup.hpp"

namespace sin_compat::original_server_cache_tree_link {
namespace tree = original_server_cache_tree_lookup;

inline constexpr std::uint16_t kMissingIndex = 0xffffu;
inline constexpr std::uint32_t kSceneLinkVA = 0x221AA890u;
inline constexpr std::uint32_t kModelSoundsLinkVA = 0x221441B0u;
inline constexpr std::uint32_t kSceneBalanceVA = 0x221A8F20u;
inline constexpr std::uint32_t kModelSoundsBalanceVA = 0x221429D0u;

struct Config {
    std::uint32_t original_entry_va;
    std::uint32_t original_balance_va;
};
inline constexpr Config kScene{kSceneLinkVA, kSceneBalanceVA};
inline constexpr Config kModelSounds{kModelSoundsLinkVA, kModelSoundsBalanceVA};

// Native thiscall ECX=tree, three arguments and 'ret 0x0c':
//   (raw new index, WORD parent index, BYTE choose-first-child).
// 1. New slot at (uint16_t)newIndex * 24 gets parent at +4,
//    both child indices 0xffff at +0/+2 and a zero WORD at +6.
// 2. If parent == 0xffff, root at tree +0x10 becomes new low WORD.
//    Else nonzero side byte sets parent's left (+0); zero sets right (+2).
// 3. Original calls a repository-specific balancing helper with the
//    FULL 32-bit newIndex, even though slot arithmetic truncates to 16 bits.
// This reference does not infer the balancing algorithm. Host must supply:
//   void balance(const Config&, uint32_t raw_new_index).
// The vector bounds checks are host-only safeguards not found in x86.
template <class Host>
void link(const Config& cfg, tree::Tree& target, std::uint32_t new_index,
          std::uint16_t parent, std::uint8_t choose_first_child, Host& host) {
    const std::uint16_t narrow = static_cast<std::uint16_t>(new_index);
    if (narrow >= target.nodes.size() ||
        (parent != kMissingIndex && parent >= target.nodes.size())) {
        throw std::out_of_range("synthetic cache tree link index out of range");
    }
    tree::NativeSlot32& node = target.nodes[narrow];
    node.left_00 = kMissingIndex;
    node.right_02 = kMissingIndex;
    // Original two WORD writes at node +4 and +6. The current Phase191
    // 24-byte reference represents both using one DWORD at offset +4.
    node.unknown_04 = static_cast<std::uint32_t>(parent);
    if (parent == kMissingIndex) {
        target.root = narrow;
    } else if (choose_first_child != 0u) {
        target.nodes[parent].left_00 = narrow;
    } else {
        target.nodes[parent].right_02 = narrow;
    }
    host.balance(cfg, new_index);
}
} // namespace sin_compat::original_server_cache_tree_link
