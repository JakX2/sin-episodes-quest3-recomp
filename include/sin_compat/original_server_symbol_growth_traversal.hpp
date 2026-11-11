#pragma once
// Phase176: host-neutral reconstruction of four original SiN Episodes 2006
// server.dll x86 functions: 0x2228E200, 0x2228E280, 0x2228E470,
// 0x2228E5B0. This is NOT the original MSVC32/Source engine object ABI.
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace sin_compat::original_server_symbol_growth_traversal {
inline constexpr std::uint16_t kEmptyIndex=0xffffu;
inline constexpr std::uint32_t kNoAllocator=0xffffffffu;

// Avoid implementation-defined unsigned-to-signed 32-bit casts when modeling
// signed native JGE/JL after modular 32-bit additions.
inline constexpr std::int64_t signed_word(std::uint32_t x) noexcept {
    return x < 0x80000000u ? static_cast<std::int64_t>(x)
      : static_cast<std::int64_t>(x) - 0x100000000ll;
}

struct ArrayGrowthState {
    std::uintptr_t buffer_0=0;
    std::uint32_t capacity_4=0;
    std::uint32_t growth_8=0;
};

// Functions 0x2228E200 (115 bytes, 12-byte records, initial capacity 3)
// and 0x2228E280 (112 bytes, 4-byte records, initial capacity 8).
// The original ALWAYS invokes allocate/reallocate if growth_8 != -1, even
// when request is 0 or the old capacity already meets the request.
// Native arithmetic is unsigned modulo 2^32; comparisons are signed.
// On pathological signed-overflow cycles the native code could loop; this
// portable model throws instead, a documented safety difference.
template <class Host>
void original_grow_array(ArrayGrowthState& a, std::uint32_t additional,
                         std::uint32_t stride, std::uint32_t initial,
                         Host& host) {
    if (a.growth_8==kNoAllocator) return;
    const std::uint32_t target=a.capacity_4+additional;
    std::uint32_t iterations=0;
    while (signed_word(a.capacity_4) < signed_word(target)) {
        if (++iterations>1000000u)
            throw std::overflow_error("original native growth would not converge");
        if (a.capacity_4==0u) a.capacity_4=initial;
        else if (a.growth_8!=0u) a.capacity_4+=a.growth_8;
        else a.capacity_4+=a.capacity_4;
    }
    const std::uint32_t raw_bytes=a.capacity_4*stride;
    if (a.buffer_0!=0u)
        a.buffer_0=host.reallocate_2226E400(a.buffer_0,raw_bytes);
    else
        a.buffer_0=host.allocate_2226E390(raw_bytes);
}
template<class Host>
void original_grow_symbol_nodes_2228E200(ArrayGrowthState& a,
                                           std::uint32_t additional,Host& host) {
    original_grow_array(a,additional,12u,3u,host);
}
template<class Host>
void original_grow_dword_slots_2228E280(ArrayGrowthState& a,
                                          std::uint32_t additional,Host& host) {
    original_grow_array(a,additional,4u,8u,host);
}

struct SymbolNode12 {
    std::uint16_t link_0=kEmptyIndex;
    std::uint16_t link_2=kEmptyIndex;
    std::uint16_t meta_4=kEmptyIndex;
    std::uint16_t meta_6=0u;
    std::uint32_t key_8=0u;
};
static_assert(sizeof(SymbolNode12)==12u,"Original symbol tree records are 12 bytes");
struct NativeSentinel {
    bool guard_initialized=false;
    SymbolNode12 node{};
};
struct InsertionSearchState {
    std::uint16_t root_10=kEmptyIndex;
    std::vector<SymbolNode12> records{};
};
struct InsertionSearchResult {
    std::uint16_t last_index=kEmptyIndex;
    std::uint8_t comparator_nonzero=0u;
};

// Original 0x2228E470 (261 bytes). Native comparator at owner+0 is invoked
// as compare(search_key, &record+8, opaque_context). On nonzero AL follow
// WORD record+0; otherwise follow WORD record+2. The original initializes
// global sentinel 0x2259012C / guard 0x22590134 lazily on the *first*
// comparator outcome, not on an empty root. The native function has no
// bounds/cycle protections: our vector range guard is host safety only.
template<class Host>
InsertionSearchResult original_prepare_symbol_tree_position_2228E470(
    const InsertionSearchState& state, std::uint32_t search_key,
    std::uint32_t opaque_context, NativeSentinel& sentinel,Host& host) {
    InsertionSearchResult result{};
    std::uint16_t current=state.root_10;
    std::size_t steps=0;
    while (current!=kEmptyIndex) {
        if (current>=state.records.size() || ++steps>state.records.size()+1u)
            throw std::out_of_range("original native symbol index invalid/cyclic");
        result.last_index=current;
        const SymbolNode12& node=state.records[current];
        result.comparator_nonzero=static_cast<std::uint8_t>(
            host.compare_key_to_node(search_key,node.key_8,opaque_context)!=0);
        if (!sentinel.guard_initialized) {
            sentinel.guard_initialized=true; // original OR bit0 before stores
            sentinel.node.link_0=kEmptyIndex;
            sentinel.node.link_2=kEmptyIndex;
            sentinel.node.meta_4=kEmptyIndex;
            sentinel.node.meta_6=1u;
            host.on_sentinel_initialized();
        }
        current=result.comparator_nonzero!=0 ? node.link_0 : node.link_2;
    }
    return result;
}

struct SymbolCleanupState {
    std::uintptr_t allocation_0=0;
    std::uint32_t capacity_4=0;
    std::uint32_t guard_8=kNoAllocator;
    std::uint32_t pending_0c=0;
    std::uintptr_t cached_pointer_10=0;
};
// Original 0x2228E5B0 (58 bytes): counter +0x0C becomes zero even on
// allocator sentinel -1; in the enabled branch free allocation if nonnull,
// set buffer to null, clear capacity, and copy current buffer into cache+0x10.
// When guard is -1, capacity/buffer are preserved and cache becomes buffer.
template<class Host>
void original_reset_symbol_storage_2228E5B0(SymbolCleanupState& s,Host& host) {
    s.pending_0c=0u;
    if (s.guard_8!=kNoAllocator) {
        if (s.allocation_0!=0u) {
            host.free_2226E3E0(s.allocation_0);
            s.allocation_0=0;
        }
        s.capacity_4=0u;
    }
    s.cached_pointer_10=s.allocation_0;
}
} // namespace sin_compat::original_server_symbol_growth_traversal