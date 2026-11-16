#pragma once
// Phase248: independent C++17 reference for ORIGINAL 2006 SiN client.dll
// CBaseEntityList constructor and explicit/non-networked registrations.
// Original entry VAs: 0x24127E80, 0x24127F50, 0x241280A0,
// 0x241280C0, 0x241A8B90. Not a retail engine-owned game object.
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include "sin_compat/original_client_entitylist_base_removal.hpp"

namespace sin_compat::original_client_entitylist_base_register {
namespace base=sin_compat::original_client_entitylist_base_removal;

inline constexpr std::uint32_t kBaseCtorVA=0x24127E80u;
inline constexpr std::uint32_t kRegisterExplicitVA=0x24127F50u;
inline constexpr std::uint32_t kRegisterForwardVA=0x241280A0u;
inline constexpr std::uint32_t kRegisterNonNetworkedVA=0x241280C0u;
inline constexpr std::uint32_t kRandomSerialWrapperVA=0x241A8B90u;
inline constexpr std::uint32_t kOriginalVstdlibRandomIntIAT=0x24276278u;
inline constexpr std::uint32_t kOriginalTier0ErrorIAT=0x242761FCu;
inline constexpr std::uint16_t kFirstInitialHighFreeSlot=2049u;
inline constexpr std::uint16_t kLastInitialHighFreeSlot=4095u;
inline constexpr std::size_t kInitialHighFreeSlotCount=2047u;
inline constexpr std::uint32_t kKeepOriginalSerial=0xFFFFFFFFu;
inline constexpr std::size_t kNewWholeOriginalMethods=5u;
inline constexpr std::size_t kNewWholeNativeCodeBytes=536u;

// Original helper 0x241A8B90 pushes 0 and 32767, calls vstdlib.dll's
// imported RandomInt(min,max), and returns. Constructor additionally
// ANDs its return with 0x7FFF before storing slot serial. Use an injected
// source, NOT C++ rand() or an invented deterministic engine PRNG.
struct SerialSource {
    virtual ~SerialSource()=default;
    virtual std::uint32_t random_int(std::uint32_t min_inclusive,
                                      std::uint32_t max_inclusive)=0;
};
inline std::uint32_t original_initial_serial(SerialSource& source) {
    return source.random_int(0u,0x7FFFu)&base::kSerialMask;
}

// Original 0x24127E80 installs base vtable, clears active/free list
// head/tail fields, then initializes exactly 4096 16-byte slot records:
// entity=NULL, serial=RandomInt(0,32767)&7FFF, prev=next=self.
// Its SECOND loop appends precisely indexes 2049..4095 to high free.
// 2048 is intentionally excluded from the initial high-free chain,
// though Phase247 raw removal requeues a released slot 2048.
inline void construct_base(base::State& state,SerialSource& random) {
    if(state.slots.size()!=base::kSlotCount)
        throw std::logic_error("retail base constructor needs exactly 4096 slots");
    state.vtable_token=base::kNativeBaseVtableVA;
    state.active={};
    state.high_free={};
    for(std::size_t i=0;i<state.slots.size();++i) {
        auto& node=state.slots[i];
        node.entity_token=0u;
        node.serial=original_initial_serial(random);
        node.previous=static_cast<std::uint16_t>(i);
        node.next=static_cast<std::uint16_t>(i);
        node.in_high_free=false;
    }
    for(std::uint16_t id=kFirstInitialHighFreeSlot;
         id<=kLastInitialHighFreeSlot;++id) {
        base::move_to_tail(state.high_free,state,id);
        base::checked(state,id).in_high_free=true;
    }
}

// The two original registration methods call entity vtable slot1 with
// a POINTER to a stack-local packed handle, and parent/base vtable
// slot0 with entity pointer and current handle. Both effects are
// engine-dependent: the host forwards them through explicit callbacks.
struct BindCallbacks {
    virtual ~BindCallbacks()=default;
    virtual void entity_virtual_slot1(std::uint32_t entity_token,
                                      std::uint32_t& mutable_handle)=0;
    virtual void parent_virtual_slot0(std::uint32_t entity_token,
                                      std::uint32_t handle)=0;
};
inline void publish_binding(base::State& state,std::uint16_t index,
                            std::uint32_t entity,BindCallbacks& callback,
                            std::uint32_t& output_handle) {
    const auto serial=base::checked(state,index).serial;
    std::uint32_t handle=(serial<<12)|index;
    callback.entity_virtual_slot1(entity,handle);
    callback.parent_virtual_slot0(entity,handle);
    output_handle=handle;
}

// Original 0x24127F50 args: output DWORD*, entity pointer, raw slot
// index, serial override. It stores entity at record+0; an override
// other than 0xFFFFFFFF is written RAW to record+4 (no serial mask).
// Then appends record to the active list using 0x24127E20, publishes
// vtable callbacks and writes output handle. Host refuses conflicting
// live slots and nonsensical native pointers rather than corrupting RAM.
inline void register_explicit(base::State& state,
                              std::uint32_t& output_handle,
                              std::uint32_t entity,
                              std::uint16_t raw_index,
                              std::uint32_t serial_override,
                              BindCallbacks& callback) {
    if(entity==0u)
        throw std::invalid_argument("original host registration needs nonnull entity");
    auto& slot=base::checked(state,raw_index);
    if(slot.entity_token!=0u)
        throw std::logic_error("entity-list registration overwrites occupied slot");
    if(slot.in_high_free) {
        base::unlink(state.high_free,state,raw_index);
        slot.in_high_free=false;
    }
    slot.entity_token=entity;
    if(serial_override!=kKeepOriginalSerial)
        slot.serial=serial_override;
    base::move_to_tail(state.active,state,raw_index);
    publish_binding(state,raw_index,entity,callback,output_handle);
}

// Native 0x241280A0 is a four-argument forwarding wrapper. It pushes
// arguments to 0x24127F50 and returns its original output pointer in EAX,
// with RET 16. Host returns a pointer into a CALLER-owned output token.
inline std::uint32_t* register_explicit_forward(
    base::State& state,std::uint32_t* output_handle,
    std::uint32_t entity,std::uint16_t index,
    std::uint32_t serial_override,BindCallbacks& callback) {
    if(output_handle==nullptr)
        throw std::invalid_argument("null output token pointer");
    register_explicit(state,*output_handle,entity,index,
                      serial_override,callback);
    return output_handle;
}

// Original 0x241280C0 is the non-networked entity registration path.
// Pop the high-free LIST HEAD, unlink it, copy entity pointer, append
// to active list, dispatch two original virtual callbacks, and output
// the packed serial/index handle. Retail invokes tier0.dll!Error with
// "CBaseEntityList::AddNonNetworkableEntity: no free slots!" when empty.
// This host throws rather than executing a retail 2006 fatal error path.
inline void register_non_networked(base::State& state,
                                   std::uint32_t& output_handle,
                                   std::uint32_t entity,
                                   BindCallbacks& callback) {
    if(entity==0u)
        throw std::invalid_argument("null non-networked entity host token");
    const auto index=state.high_free.head;
    if(index==base::kMissing)
        throw std::runtime_error(
            "CBaseEntityList::AddNonNetworkableEntity: no free slots!");
    auto& node=base::checked(state,index);
    if(!node.in_high_free)
        throw std::logic_error("invalid high-free list head");
    base::unlink(state.high_free,state,index);
    node.in_high_free=false;
    if(node.entity_token!=0u)
        throw std::logic_error("high-free entity is occupied");
    node.entity_token=entity;
    base::move_to_tail(state.active,state,index);
    publish_binding(state,index,entity,callback,output_handle);
}

} // namespace sin_compat::original_client_entitylist_base_register
