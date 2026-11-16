#pragma once
// Phase244 independent C++17 host models of eight original 2006 SiN client
// PVS list / node primitive methods. Original code SHA256s are verified
// separately against the owner's retail PE32 DLLs. These are not game-owned
// Source objects or a playable client.dll.
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_client_pvs_rb_fixup.hpp"

namespace sin_compat::original_client_pvs_node_fields_list_reset {
namespace map=sin_compat::original_client_pvs_map_frontends;
namespace rb=sin_compat::original_client_pvs_rb_fixup;

inline constexpr std::size_t kNativeMethods=8u;
inline constexpr std::size_t kOriginalCodeBytes=257u;
inline constexpr std::uint16_t kAbsent=0xFFFFu;
inline constexpr std::uint32_t kAuxFreeChainResetVA=0x24105DD0u;
inline constexpr std::uint32_t kList16ActiveVA=0x24105E80u;
inline constexpr std::uint32_t kMapPayloadAddressVA=0x24105D10u;
inline constexpr std::uint32_t kSetParentVA=0x24105EE0u;
inline constexpr std::uint32_t kSetLeftVA=0x24105F00u;
inline constexpr std::uint32_t kSetRightVA=0x24105F20u;
inline constexpr std::uint32_t kReadColorVA=0x24105FA0u;
inline constexpr std::uint32_t kSetColorVA=0x24105FC0u;

// The original native 0x24105DD0 operates on an 8-BYTE indexed linked
// list: backing ptr +0, active head +0xC, tail +0xE, free head +0x10,
// live count +0x12, capacity +0x14. It resets each node's prev WORD+4
// to its own index, and next WORD+6 to the following free index.
struct Aux8Node {
    std::uint32_t payload{};
    std::uint16_t prev{kAbsent};
    std::uint16_t next{kAbsent};
};
static_assert(sizeof(Aux8Node)==8u);
static_assert(offsetof(Aux8Node,prev)==4u);
static_assert(offsetof(Aux8Node,next)==6u);
struct Aux8Manager {
    std::vector<Aux8Node> nodes;
    std::uint16_t head{kAbsent};
    std::uint16_t tail{kAbsent};
    std::uint16_t free_head{kAbsent};
    std::uint16_t live_count{};
    explicit Aux8Manager(std::size_t capacity):nodes(capacity) {
        if(capacity>=kAbsent)
            throw std::out_of_range("host auxiliary list exceeds native WORD indices");
    }
};
inline void reset_aux_8_free_chain(Aux8Manager& list) {
    // Original capacity=0 does not alter any manager field.
    if(list.nodes.empty())return;
    for(std::size_t n=list.nodes.size();n-->0u;) {
        list.nodes[n].prev=static_cast<std::uint16_t>(n);
        list.nodes[n].next=(n+1u==list.nodes.size())?kAbsent:
                           static_cast<std::uint16_t>(n+1u);
    }
    list.free_head=0u;
    list.head=kAbsent;
    list.tail=kAbsent;
    list.live_count=0u;
}

// Original 0x24105E80 operates on a DIFFERENT 16-BYTE list,
// NOT the 8-byte auxiliary chain and NOT the PVS red/black map.
// Its record +0xC/+0xE are previous/next list WORDs; both equal to
// index signals a freed record. Capacity is manager+0x14 WORD.
struct List16Node {
    std::array<std::uint8_t,12u> opaque{};
    std::uint16_t prev{kAbsent};
    std::uint16_t next{kAbsent};
};
static_assert(sizeof(List16Node)==16u);
static_assert(offsetof(List16Node,prev)==12u);
static_assert(offsetof(List16Node,next)==14u);
inline bool list16_is_active(const std::vector<List16Node>& records,
                             std::uint16_t index) {
    if(index>=records.size())return false;
    const auto& node=records[index];
    return node.prev!=index || node.next!=index;
}

// 0x24105D10 returns storage DWORD map+4 plus index*16+8. Host returns
// checked byte OFFSET from backing storage, not a forged 32-bit pointer.
inline std::size_t map_payload_byte_offset(const map::Map& manager,
                                           std::uint16_t index) {
    (void)map::checked(manager,index);
    return std::size_t(index)*16u+8u;
}

// All four original setter bodies take index WORD and value WORD, address
// a 16-byte record through map+4, and RET 8. Unlike the higher-level
// Phase240 balancing API, the native primitives accept arbitrary WORDs,
// so host functions preserve raw 16-bit writes without inferring validity.
inline void set_parent_word(map::Map& manager,std::uint16_t index,
                            std::uint16_t value) {
    auto& record=map::checked(manager,index);
    record.opaque_header=(record.opaque_header&0xFFFF0000u)|value;
}
inline void set_left_word(map::Map& manager,std::uint16_t index,
                          std::uint16_t value) {
    map::checked(manager,index).left=value;
}
inline void set_right_word(map::Map& manager,std::uint16_t index,
                           std::uint16_t value) {
    map::checked(manager,index).right=value;
}
inline void set_color_word(map::Map& manager,std::uint16_t index,
                           std::uint16_t value) {
    auto& record=map::checked(manager,index);
    record.opaque_header=(record.opaque_header&0x0000FFFFu)|
        (static_cast<std::uint32_t>(value)<<16);
}
// Native 0x24105FA0 calls 0x24105F40, then loads returned record WORD+6.
// For index 0xFFFF, the original resolver returns the global nil
// sentinel whose +6 WORD is black(1), previously recovered in Phase238.
inline std::uint16_t read_color_via_resolver(const map::Map& manager,
                                             std::uint16_t index) {
    return rb::color(manager,index);
}
} // namespace sin_compat::original_client_pvs_node_fields_list_reset
