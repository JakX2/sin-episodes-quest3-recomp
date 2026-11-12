#pragma once
// Phase195: branch-exact synthetic model of the original 2006 SiN server.dll
// cached-repository node/sentinel resolution. No original engine ABI exposed.
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "sin_compat/original_server_cache_tree_lookup.hpp"

namespace sin_compat::original_server_cache_node_resolver {
namespace tree = original_server_cache_tree_lookup;
inline constexpr std::uint16_t kMissing = 0xffffu;
inline constexpr std::uint32_t kStride = 24u;
struct Config { std::uint32_t function_va, sentinel_va, sentinel_flag_va; };
inline constexpr Config kScene{0x221a4d90u,0x2256e1f0u,0x2256e1f8u};
inline constexpr Config kModelSounds{0x2213e990u,0x22558d50u,0x22558d58u};

// Four native WORDs followed by a separate bit-0 initialization flag DWORD.
// The original sentinel belongs to virtual .data; its initial zero values
// need not correspond to file-backed bytes in the original PE image.
struct Sentinel {
    std::uint16_t word_00{}, word_02{}, word_04{}, word_06{};
    std::uint32_t flags_08{};
};
static_assert(sizeof(Sentinel)==12u);
static_assert(offsetof(Sentinel,flags_08)==8u);
static_assert(std::is_standard_layout<Sentinel>::value);
struct Selection {
    bool sentinel;
    std::uint16_t index;
    std::uint16_t word_at_02;
};

// Native THISCALL (ECX=tree), one stack index argument, ret 4:
// - initialize the original static sentinel BEFORE checking the index,
// - 0xffff -> static 8-byte sentinel address,
// - otherwise -> slots_base + uint16(index)*24.
// We model only the WORD at returned pointer+2 consumed by Phase194's
// free-list update. We never treat the 8-byte sentinel as a 24-byte slot.
// Host: void initialized(const Config&) called only when flag bit0 clears.
// Host index bounds checks prevent unsafe native pointer arithmetic.
template<class Host>
Selection select(const Config& cfg, const std::vector<tree::NativeSlot32>& slots,
                 std::uint16_t index, Sentinel& state, Host& host) {
    if ((state.flags_08 & 1u)==0u) {
        state.flags_08 |= 1u; // preserve unrelated bits as native OR does
        state.word_00 = state.word_02 = state.word_04 = kMissing;
        state.word_06 = 1u;
        host.initialized(cfg);
    }
    if (index == kMissing) return {true,index,state.word_02};
    if (index >= slots.size())
        throw std::out_of_range("host-only cache resolver slot bound");
    return {false,index,slots[index].right_02};
}
} // namespace sin_compat::original_server_cache_node_resolver
