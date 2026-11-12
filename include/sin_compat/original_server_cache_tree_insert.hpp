#pragma once
// Phase193, owner-original 2006 server.dll: two 112-byte index insertion
// wrappers. Host-neutral C++17 reference, not original engine ABI or DLL.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include "sin_compat/original_server_cache_tree_lookup.hpp"
#include "sin_compat/original_server_cache_key_comparators.hpp"

namespace sin_compat::original_server_cache_tree_insert {
namespace lookup = original_server_cache_tree_lookup;
namespace key = original_server_cache_key_comparators;
inline constexpr std::uint32_t kSceneInsertVA = 0x221ACD00u;
inline constexpr std::uint32_t kModelSoundsInsertVA = 0x22146EF0u;
inline constexpr std::uint32_t kNodeKeyOffset = 8u;
inline constexpr std::uint32_t kNodePayloadOffset = 20u;
struct Config {
    std::uint32_t native_entry_va;
    std::uint32_t prepare_va;
    std::uint32_t claim_va;
    std::uint32_t link_va;
};
inline constexpr Config kScene{0x221ACD00u,0x221A6510u,0x221A6890u,0x221AA890u};
inline constexpr Config kModelSounds{0x22146EF0u,0x22140700u,0x22140900u,0x221441B0u};

struct State {
    lookup::Tree slots{};
    std::uint16_t live_count{}; // native object offset +0x12
};
// An original function invokes (1) an opaque four-argument position/setup
// helper, (2) a slot allocator, (3) an opaque three-argument tree linker,
// then increments its native 16-bit count at +0x12 and copies four DWORDs
// from the incoming 16-byte key to slot[index].+0x08..+0x17.
// Native uses the *full* allocator index for link, but only DI (low 16 bits)
// for copy and its AX return. The native code does not validate allocations.
// Host callbacks: void prepare(const Config&, const NativeKeyPrefix32&,uint32_t);
// uint32_t claim(const Config&); void link(const Config&, uint32_t);
// This model uses checked vector storage to prevent memory corruption.
template<class Host>
std::uint16_t insert(const Config& cfg, State& state,
                     const key::NativeKeyPrefix32& item,
                     std::uint32_t auxiliary, Host& host) {
    host.prepare(cfg, item, auxiliary);
    const std::uint32_t raw_index = host.claim(cfg);
    host.link(cfg, raw_index);
    state.live_count = static_cast<std::uint16_t>(state.live_count + 1u);
    const auto index = static_cast<std::uint16_t>(raw_index);
    if (index >= state.slots.nodes.size())
        throw std::out_of_range("host-only tree slot index guard");
    auto& node = state.slots.nodes[index];
    node.key_08 = item.first_word_00;
    std::memcpy(&node.unknown_0c[0], &item.opaque_04, sizeof(item.opaque_04));
    std::memcpy(&node.unknown_0c[4], &item.opaque_08, sizeof(item.opaque_08));
    node.payload_14 = item.opaque_0c;
    return index;
}
} // namespace sin_compat::original_server_cache_tree_insert
