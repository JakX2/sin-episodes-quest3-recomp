#pragma once
// Phase247, independent C++17 host reference for owner-original SiN
// Episodes: Emergence (2006) client.dll entity-list base operations.
// Native VAs: 0x24127E20, 0x24127FC0, 0x24128070, 0x24128180.
// Do not call host references original C++ engine-owned objects; they
// do not execute original entity virtual methods or Source heap code.
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace sin_compat::original_client_entitylist_base_removal {

inline constexpr std::uint32_t kNativeFreeListAppendVA=0x24127E20u;
inline constexpr std::uint32_t kNativeRemoveIndexVA=0x24127FC0u;
inline constexpr std::uint32_t kNativeBaseDestructorVA=0x24128070u;
inline constexpr std::uint32_t kNativeMaskedHandleVA=0x24128180u;
inline constexpr std::uint32_t kNativeBaseVtableVA=0x24298EB4u;
inline constexpr std::uint16_t kMissing=0xFFFFu;
inline constexpr std::uint16_t kIndexMask=0x0FFFu;
inline constexpr std::uint16_t kHighFreeStart=2048u;
inline constexpr std::uint32_t kSerialMask=0x7FFFu;
inline constexpr std::size_t kSlotCount=4096u;
inline constexpr std::size_t kSlotStride=16u;
inline constexpr std::uint32_t kLiveHeadPointerOffset=0x10004u;
inline constexpr std::uint32_t kLiveTailPointerOffset=0x10008u;
inline constexpr std::uint32_t kHighFreeHeadPointerOffset=0x1000Cu;
inline constexpr std::uint32_t kHighFreeTailPointerOffset=0x10010u;
inline constexpr std::size_t kNewCompleteOriginalMethods=4u;
inline constexpr std::size_t kNewWholeOriginalCodeBytes=333u;

// The original native 16-byte slot stores entity pointer DWORD+0, serial
// DWORD+4, and doubly linked list pointers DWORD+8/+C. Host uses indices
// as *tokens* for those pointers; these are NOT original 32-bit pointers.
struct Slot {
    std::uint32_t entity_token{};
    std::uint32_t serial{};
    std::uint16_t previous{kMissing};
    std::uint16_t next{kMissing};
    bool in_high_free{};
};
struct Chain {
    std::uint16_t head{kMissing};
    std::uint16_t tail{kMissing};
};
struct State {
    std::vector<Slot> slots;
    Chain active{};
    Chain high_free{};
    std::uint32_t vtable_token{};
    explicit State(std::size_t count=kSlotCount):slots(count) {
        if(count>kSlotCount)throw std::out_of_range("native entity slot maximum 4096");
        for(std::size_t i=0;i<count;++i) {
            slots[i].previous=static_cast<std::uint16_t>(i);
            slots[i].next=static_cast<std::uint16_t>(i);
        }
    }
};
inline Slot& checked(State& state,std::uint16_t id) {
    if(id>=state.slots.size())
        throw std::out_of_range("unbacked original entity-list slot");
    return state.slots.at(id);
}
inline const Slot& checked(const State& state,std::uint16_t id) {
    if(id>=state.slots.size())
        throw std::out_of_range("unbacked original entity-list slot");
    return state.slots.at(id);
}

// A self-linked slot is not present in a linked list. Native pointer
// arithmetic has no bounds checks; these guards are host-only safety.
inline void unlink(Chain& chain,State& state,std::uint16_t id) {
    auto& slot=checked(state,id);
    if(slot.next==id)return;
    const auto prev=slot.previous,next=slot.next;
    if(prev!=kMissing) {
        if(checked(state,prev).next!=id)
            throw std::logic_error("entity-list corrupt previous pointer");
    } else if(chain.head!=id) {
        throw std::logic_error("entity-list slot is not chain head");
    }
    if(next!=kMissing) {
        if(checked(state,next).previous!=id)
            throw std::logic_error("entity-list corrupt next pointer");
    } else if(chain.tail!=id) {
        throw std::logic_error("entity-list slot is not chain tail");
    }
    if(prev==kMissing)chain.head=next;
    else checked(state,prev).next=next;
    if(next==kMissing)chain.tail=prev;
    else checked(state,next).previous=prev;
    slot.previous=id;
    slot.next=id;
}

// SHA-reviewed native 0x24127E20: unlink a node if currently active in
// this list, then append to the same list's tail. Both native returns pop
// one DWORD. This helper receives the list manager; it is not global.
inline void move_to_tail(Chain& chain,State& state,std::uint16_t id) {
    auto& node=checked(state,id);
    if(node.next!=id)unlink(chain,state,id);
    const auto previous_tail=chain.tail;
    if(previous_tail!=kMissing)
        (void)checked(state,previous_tail);
    node.previous=previous_tail;
    node.next=kMissing;
    chain.tail=id;
    if(previous_tail==kMissing)chain.head=id;
    else checked(state,previous_tail).next=id;
}

// Synthetic helper used only by host tests to prepare valid linked
// entities. Native insertion is a different original method.
inline void seed_bound(State& state,std::uint16_t id,
                       std::uint32_t entity_token,std::uint32_t serial) {
    if(entity_token==0u)
        throw std::invalid_argument("host cannot seed null entity");
    auto& node=checked(state,id);
    if(node.entity_token!=0u)
        throw std::logic_error("duplicate entity slot");
    if(node.in_high_free) {
        unlink(state.high_free,state,id);
        node.in_high_free=false;
    }
    node.entity_token=entity_token;
    node.serial=serial&kSerialMask;
    move_to_tail(state.active,state,id);
}

// The retail method invokes entity vtable slot1 with a caller-owned
// output initialized to -1, then parent vtable slot1 with the entity
// and packed old serial/index. The callbacks here do not model the
// opaque game entity/engine virtual method bodies.
struct Callbacks {
    virtual ~Callbacks()=default;
    virtual void entity_virtual_slot1(std::uint32_t entity_token,
                                      std::uint32_t& output_token)=0;
    virtual void parent_virtual_slot1(std::uint32_t entity_token,
                                      std::uint32_t packed_handle)=0;
};

// SHA-reviewed original 0x24127FC0 directly consumes an index; no
// 12-bit mask occurs inside it. Null entity fast-exits without effects.
inline bool remove_raw_index(State& state,std::uint16_t id,Callbacks& callbacks) {
    if(id>kIndexMask)
        throw std::out_of_range("native raw remove index outside 12 bits");
    auto& slot=checked(state,id);
    if(slot.entity_token==0u)return false;
    const auto entity=slot.entity_token;
    const auto handle=static_cast<std::uint32_t>(
        (slot.serial<<12)|id);
    std::uint32_t entity_output=0xFFFFFFFFu;
    callbacks.entity_virtual_slot1(entity,entity_output);
    callbacks.parent_virtual_slot1(entity,handle);

    slot.serial=(slot.serial+1u)&kSerialMask;
    slot.entity_token=0u;
    unlink(state.active,state,id);
    if(id>=kHighFreeStart) {
        move_to_tail(state.high_free,state,id);
        slot.in_high_free=true;
    }
    return true;
}

// Whole-original 18-byte native thunk 0x24128180 masks its stack input
// to 0xFFF and TAIL JUMPS into 0x24127FC0; its caller does not push
// a new return address. The host simply delegates to remove_raw_index.
inline bool remove_packed_handle(State& state,
                                 std::uint32_t packed_handle,
                                 Callbacks& callbacks) {
    return remove_raw_index(state,
        static_cast<std::uint16_t>(packed_handle&kIndexMask),callbacks);
}

// SHA-reviewed base destructor 0x24128070 first installs vtable
// 0x24298EB4, then saves each active slot's NEXT pointer before calling
// 0x24127FC0 on it. The native code does not free the object here; its
// own containing-object destructor is 0x24107430 (Phase245).
inline void destroy_base(State& state,Callbacks& callbacks) {
    state.vtable_token=kNativeBaseVtableVA;
    auto index=state.active.head;
    std::size_t visited=0u;
    while(index!=kMissing) {
        if(++visited>state.slots.size())
            throw std::runtime_error("cyclic synthetic entity-list destructor");
        const auto next=checked(state,index).next;
        if(!remove_raw_index(state,index,callbacks))
            throw std::logic_error("entity-list active slot has null entity");
        // In a correctly linked original list, the next saved before
        // removal is precisely the new active head.
        if(state.active.head!=next)
            throw std::logic_error("entity-list destructor head drift");
        index=next;
    }
}

} // namespace sin_compat::original_client_entitylist_base_removal
