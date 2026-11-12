#pragma once
// Phase194: portable reference for two original 2006 server.dll tree slot
// allocation methods. Original thiscall/heap ABI is NOT implemented.
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "sin_compat/original_server_cache_tree_lookup.hpp"

namespace sin_compat::original_server_cache_slot_allocation {
namespace lookup = original_server_cache_tree_lookup;
inline constexpr std::uint32_t kSceneAllocVA = 0x221A6890u;
inline constexpr std::uint32_t kModelSoundsAllocVA = 0x22140900u;
inline constexpr std::uint32_t kSceneGrowVA = 0x221A3A10u;
inline constexpr std::uint32_t kModelSoundsGrowVA = 0x2213AAC0u;
inline constexpr std::uint32_t kSceneFreeNodeVA = 0x221A4D90u;
inline constexpr std::uint32_t kModelSoundsFreeNodeVA = 0x2213E990u;
inline constexpr std::uint16_t kMissing = 0xffffu;

// Fixed-width original 32-bit tree fields. 64-bit host pointers must NEVER
// be stored into these fields as though they were live native pointers.
struct NativeTreeHeader32 {
    std::uint32_t comparator_00{};
    std::uint32_t slots_04{};
    std::uint32_t capacity_08{};
    std::uint32_t unknown_0c{};
    std::uint16_t root_10{kMissing};
    std::uint16_t live_count_12{};
    std::uint16_t free_head_14{kMissing};
    std::uint16_t next_unused_16{};
    std::uint32_t latest_slots_base_18{};
};
static_assert(sizeof(NativeTreeHeader32) == 0x1cu);
static_assert(offsetof(NativeTreeHeader32, slots_04) == 0x04u);
static_assert(offsetof(NativeTreeHeader32, capacity_08) == 0x08u);
static_assert(offsetof(NativeTreeHeader32, root_10) == 0x10u);
static_assert(offsetof(NativeTreeHeader32, live_count_12) == 0x12u);
static_assert(offsetof(NativeTreeHeader32, free_head_14) == 0x14u);
static_assert(offsetof(NativeTreeHeader32, next_unused_16) == 0x16u);
static_assert(offsetof(NativeTreeHeader32, latest_slots_base_18) == 0x18u);
static_assert(std::is_standard_layout<NativeTreeHeader32>::value);

struct Pool {
    NativeTreeHeader32 native{};
    // Synthetic safe storage; no original game heap layout is reproduced.
    std::vector<lookup::NativeSlot32> slots{};
};

// Original native method first uses free_head (+0x14). When 0xffff, it
// grows the array at +0x04 by one slot only if capacity_08 == next_unused_16,
// claims the previous 16-bit next_unused value and increments that WORD.
// Otherwise, it selects free_head and sets free_head from the free node's
// next pointer (+2) returned through the repository-specific helper.
// Both paths clear slot +8/+12, write 0xfffffffe to +16 and 0xffffffff
// to +20, update +0x18 with the current pointer stored at native +0x04,
// and return index in AX. This does not alter the live count at +0x12.
// Host: void grow(Pool&,uint32_t count), uint16_t next_free(const Pool&,uint16_t).
// Host-vector bounds guards are deliberately stricter than native x86.
template<class Host>
std::uint16_t claim(Pool& pool,Host& host) {
    const std::uint16_t head=pool.native.free_head_14;
    std::uint16_t index=head;
    if (head==kMissing) {
        if (pool.native.capacity_08 == pool.native.next_unused_16)
            host.grow(pool,1u);
        index=pool.native.next_unused_16;
        pool.native.next_unused_16=static_cast<std::uint16_t>(index+1u);
    } else {
        pool.native.free_head_14=host.next_free(pool,head);
    }
    if (index>=pool.slots.size())
        throw std::out_of_range("synthetic original cache-tree allocator slot guard");
    lookup::NativeSlot32& slot=pool.slots[index];
    slot.key_08=0u;
    slot.unknown_0c[0]=slot.unknown_0c[1]=slot.unknown_0c[2]=slot.unknown_0c[3]=0u;
    slot.unknown_0c[4]=0xfeu;
    slot.unknown_0c[5]=slot.unknown_0c[6]=slot.unknown_0c[7]=0xffu;
    slot.payload_14=0xffffffffu;
    pool.native.latest_slots_base_18=pool.native.slots_04;
    return index;
}
} // namespace sin_compat::original_server_cache_slot_allocation
