#pragma once
// Phase249: independent C++17 host reference for nine authentic original
// SiN Episodes: Emergence (2006) client.dll intrusive-list / base vtable
// methods, authenticated separately by whole-native-body SHA256.
// No real retail x86 entity pointers, original game DLL or engine startup.
#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "sin_compat/original_client_entitylist_base_removal.hpp"

namespace sin_compat::original_client_entitylist_intrusive_list_vtable {
namespace base=sin_compat::original_client_entitylist_base_removal;
inline constexpr std::uint32_t kSlotSelfLinkVA=0x24127C80u;
inline constexpr std::uint32_t kListClearHeadsVA=0x24127C90u;
inline constexpr std::uint32_t kSlotLinkedPredicateVA=0x24127CA0u;
inline constexpr std::uint32_t kBaseVirtualSlot0VA=0x24127CC0u;
inline constexpr std::uint32_t kBaseVirtualSlot1VA=0x24127CD0u;
inline constexpr std::uint32_t kGetEntityPointerVA=0x24127CE0u;
inline constexpr std::uint32_t kUnlinkVA=0x24127CF0u;
inline constexpr std::uint32_t kInsertBeforeVA=0x24127D40u;
inline constexpr std::uint32_t kInsertAfterVA=0x24127DB0u;
inline constexpr std::uint32_t kOriginalBaseVtableVA=0x24298EB4u;
inline constexpr std::uint32_t kOriginalDerivedParentVtableVA=0x242973B8u;
inline constexpr std::uint32_t kOriginalEmbeddedEntityListVtableVA=0x24297390u;
inline constexpr std::uint32_t kDerivedRegisterVirtualVA=0x241089D0u;
inline constexpr std::uint32_t kDerivedRemoveVirtualVA=0x24108A70u;
inline constexpr std::uint32_t kEmbeddedGetEntityVA=0x241056F0u;
inline constexpr std::uint32_t kEmbeddedGetEntityViaHandleVA=0x24105BB0u;
inline constexpr std::size_t kOriginalMethodCount=9u;
inline constexpr std::size_t kWholeOriginalCodeBytes=327u;
inline constexpr std::size_t kPrevDwordOffset=8u;
inline constexpr std::size_t kNextDwordOffset=12u;
inline constexpr std::size_t kHeadDwordOffset=0u;
inline constexpr std::size_t kTailDwordOffset=4u;

// Original 0x24127C80 only writes node+8 = node and node+0xC = node.
// Host uses a node INDEX token to represent those native pointer fields.
// Does not unlink the node from an existing chain: callers are responsible.
inline void self_link_slot(base::State& s,std::uint16_t id) {
    auto& slot=base::checked(s,id);
    slot.previous=id;
    slot.next=id;
}

// Original 0x24127C90 zeros only the manager head and tail DWORDs.
// It does NOT reset the slot records.
inline void clear_list_heads(base::Chain& list) noexcept {
    list.head=base::kMissing;
    list.tail=base::kMissing;
}

// Original 0x24127CA0 compares slot's DWORD+8 (prev) to its own
// address, returns a Boolean byte, and does NOT read DWORD+0xC.
inline bool slot_is_linked_by_previous(const base::State& s,
                                       std::uint16_t id) {
    return base::checked(s,id).previous!=id;
}

// Original base vtable at 0x24298EB4 has TWO no-op methods, each RET8.
// Derived parent vtable 0x242973B8 overrides these with previously
// SHA-reviewed 0x241089D0 registration and 0x24108A70 removal callbacks.
// Host does not substitute the derived methods for the original no-ops.
inline void base_virtual_slot0_noop(std::uint32_t /*entity*/,
                                    std::uint32_t& /*handle*/) noexcept {}
inline void base_virtual_slot1_noop(std::uint32_t /*entity*/,
                                    std::uint32_t& /*handle*/) noexcept {}

// Original 0x24127CE0 reads DWORD+0 entity pointer. This returns the
// host-owned entity identifier token, not a genuine retail process address.
inline std::uint32_t entity_token(const base::State& s,std::uint16_t id) {
    return base::checked(s,id).entity_token;
}

// Original 0x24127CF0 unlinks only if the slot's PREV DWORD+8 does
// not equal its own pointer. It rewrites neighbors / manager head/tail
// and then sets both slot links back to its own pointer.
// The original has no validation. Host rejects corrupt associations.
inline bool unlink_if_linked(base::Chain& list,base::State& state,
                             std::uint16_t id) {
    auto& slot=base::checked(state,id);
    if(slot.previous==id)
        return false;
    const auto previous=slot.previous;
    const auto next=slot.next;
    if(previous==base::kMissing) {
        if(list.head!=id)
            throw std::logic_error("original host unlink: node not head");
    } else if(base::checked(state,previous).next!=id) {
        throw std::logic_error("original host unlink: previous mismatch");
    }
    if(next==base::kMissing) {
        if(list.tail!=id)
            throw std::logic_error("original host unlink: node not tail");
    } else if(base::checked(state,next).previous!=id) {
        throw std::logic_error("original host unlink: next mismatch");
    }
    if(previous==base::kMissing)list.head=next;
    else base::checked(state,previous).next=next;
    if(next==base::kMissing)list.tail=previous;
    else base::checked(state,next).previous=previous;
    slot.previous=id;
    slot.next=id;
    return true;
}

inline void require_member(const base::Chain& list,
                           const base::State& state,std::uint16_t id) {
    if(id==base::kMissing)return;
    const auto& position=base::checked(state,id);
    if(position.previous==id)
        throw std::logic_error("insertion position is a detached slot");
    if(position.previous==base::kMissing) {
        if(list.head!=id)throw std::logic_error("position not list head");
    } else if(base::checked(state,position.previous).next!=id) {
        throw std::logic_error("invalid position previous link");
    }
    if(position.next==base::kMissing) {
        if(list.tail!=id)throw std::logic_error("position not list tail");
    } else if(base::checked(state,position.next).previous!=id) {
        throw std::logic_error("invalid position next link");
    }
}

// Native 0x24127D40 inserts node BEFORE its second argument; nullptr
// means append at TAIL. Existing linked nodes are unlinked first.
// This is not the same as native 0x24127DB0 (after).
inline void insert_before(base::Chain& list,base::State& state,
                          std::uint16_t id,std::uint16_t before) {
    if(id==before)
        throw std::logic_error("cannot use entity slot as own before position");
    (void)base::checked(state,id);
    if(before!=base::kMissing)require_member(list,state,before);
    (void)unlink_if_linked(list,state,id);
    const auto previous=before==base::kMissing
        ? list.tail : base::checked(state,before).previous;
    auto& node=base::checked(state,id);
    node.next=before;
    node.previous=previous;
    if(before==base::kMissing)list.tail=id;
    else base::checked(state,before).previous=id;
    if(previous==base::kMissing)list.head=id;
    else base::checked(state,previous).next=id;
}

// Native 0x24127DB0 inserts node AFTER its second argument; nullptr
// means prepend at HEAD. Existing linked nodes are unlinked first.
inline void insert_after(base::Chain& list,base::State& state,
                         std::uint16_t id,std::uint16_t after) {
    if(id==after)
        throw std::logic_error("cannot use entity slot as own after position");
    (void)base::checked(state,id);
    if(after!=base::kMissing)require_member(list,state,after);
    (void)unlink_if_linked(list,state,id);
    const auto next=after==base::kMissing
        ? list.head : base::checked(state,after).next;
    auto& node=base::checked(state,id);
    node.previous=after;
    node.next=next;
    if(after==base::kMissing)list.head=id;
    else base::checked(state,after).next=id;
    if(next==base::kMissing)list.tail=id;
    else base::checked(state,next).previous=id;
}

} // namespace sin_compat::original_client_entitylist_intrusive_list_vtable
