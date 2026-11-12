#pragma once
// Phase191: synthetic, host-neutral model of two SHA-pinned original 2006
// server.dll lookup routines, NOT a replacement for native Source ABI.
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace sin_compat::original_server_cache_tree_lookup {
inline constexpr std::uint16_t kMissing = 0xffffu;
inline constexpr std::uint32_t kStride = 24u;
inline constexpr std::uint32_t kSceneLookupVA = 0x221A7FB0u;
inline constexpr std::uint32_t kModelSoundsLookupVA = 0x22142670u;
inline constexpr std::uint32_t kSceneSentinelVA = 0x2256E1F0u;
inline constexpr std::uint32_t kModelSoundsSentinelVA = 0x22558D50u;
inline constexpr std::uint32_t kSceneSentinelFlagVA = 0x2256E1F8u;
inline constexpr std::uint32_t kModelSoundsSentinelFlagVA = 0x22558D58u;

// Offsets are from the original indexed 24-byte slot, not the tree header.
struct NativeSlot32 {
    std::uint16_t left_00{kMissing};
    std::uint16_t right_02{kMissing};
    std::uint32_t unknown_04{};
    std::uint32_t key_08{};
    std::uint8_t unknown_0c[8]{};
    std::uint32_t payload_14{};
};
static_assert(sizeof(NativeSlot32) == 24);
static_assert(offsetof(NativeSlot32, key_08) == 8);
static_assert(offsetof(NativeSlot32, payload_14) == 20);
static_assert(std::is_standard_layout<NativeSlot32>::value);

struct Tree {
    // The original root is a WORD at tree object +0x10; this host vector is
    // not the original allocation or pointer layout.
    std::uint16_t root{kMissing};
    std::vector<NativeSlot32> nodes{};
};
struct SentinelState { bool ready{}; };

// Original code calls comparator twice at most per visited node:
//   compare(query, node.key), then compare(node.key, query).
// First true follows node +0, second true follows node +2, and neither
// true returns the current 16-bit node index. Both descent branches lazily
// initialize a different native static 8-byte sentinel (ffff,ffff,ffff,1)
// exactly once; the actual native bytes live outside file-backed .data.
//
// Host: bool compare(uint32_t,uint32_t); void initialize_sentinel(uint32_t)
// Neither key encoding nor the original comparator implementation is known.
// The bounds and cycle checks below are host-only safeguards, absent in x86.
template<class Host>
std::uint16_t find(const Tree& tree, std::uint32_t query, SentinelState& sentinel,
                   std::uint32_t sentinel_va, Host& host) {
    std::uint16_t index = tree.root;
    std::size_t steps = 0;
    while (index != kMissing) {
        if (++steps > tree.nodes.size())
            throw std::length_error("synthetic tree cycle/invalid path");
        if (index >= tree.nodes.size())
            throw std::out_of_range("synthetic tree index not backed by node vector");
        const NativeSlot32& node = tree.nodes[index];
        bool go_left = host.compare(query, node.key_08);
        if (!go_left) {
            if (!host.compare(node.key_08, query)) return index;
        }
        if (!sentinel.ready) {
            host.initialize_sentinel(sentinel_va);
            sentinel.ready = true;
        }
        index = go_left ? node.left_00 : node.right_02;
    }
    return kMissing;
}
} // namespace sin_compat::original_server_cache_tree_lookup
