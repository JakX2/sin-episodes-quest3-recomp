#pragma once
// Phase245 independent C++17 host reference: 2006 original SiN client
// entity-list drain / destructor and 8-byte, 16-byte, map index wrappers.
// Native entries 0x241061E0, 0x24107430, 0x241082C0..0x24108390.
// This does NOT instantiate original engine-owned objects or run a
// standalone gameplay DLL. Original virtual/allocator methods are opaque.
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_client_pvs_node_fields_list_reset.hpp"
#include "sin_compat/original_client_pvs_map_search_index.hpp"

namespace sin_compat::original_client_entitylist_pvs_lifecycle_wrappers {
namespace list = sin_compat::original_client_pvs_node_fields_list_reset;
namespace map = sin_compat::original_client_pvs_map_frontends;
namespace index = sin_compat::original_client_pvs_map_search_index;

inline constexpr std::size_t kNewOriginalMethods=7u;
inline constexpr std::size_t kNewOriginalNativeBytes=572u;
inline constexpr std::uint32_t kDrainVA=0x241061E0u;
inline constexpr std::uint32_t kDestructorVA=0x24107430u;
inline constexpr std::uint32_t kRecycle8VA=0x241082C0u;
inline constexpr std::uint32_t kRecycle16VA=0x241082F0u;
inline constexpr std::uint32_t kAcquire8VA=0x24108320u;
inline constexpr std::uint32_t kAcquire16VA=0x24108360u;
inline constexpr std::uint32_t kAcquireMapVA=0x24108390u;
inline constexpr std::uint32_t kParentVtableVA=0x242973B8u;
inline constexpr std::uint32_t kEmbeddedVtableVA=0x24297390u;
inline constexpr std::uint32_t kBaseDestructorVA=0x24128070u;
inline constexpr std::uint32_t kOriginalFreeVA=0x2416F940u;
inline constexpr std::uint32_t kOriginalRemoveEntityHandleVA=0x24128180u;
inline constexpr std::uint32_t kAux8ManagerOwnerOffset=0x18028u;
inline constexpr std::uint32_t kAux16ManagerOwnerOffset=0x18044u;
inline constexpr std::uint32_t kMapManagerOwnerOffset=0x18060u;
inline constexpr std::uint32_t kOwnerPrimaryPointerOffset=0x10004u;
inline constexpr std::uint32_t kOwnerLiveCountOffset=0x10018u;
inline constexpr std::uint32_t kOwnerSecondCountOffset=0x10020u;
inline constexpr std::uint32_t kOwnerHighestSlotOffset=0x10024u;
inline constexpr std::uint32_t kFixedGrowthPolicy=0xFFFFFFFFu;
inline constexpr std::uint16_t kAbsent=0xFFFFu;

// The original 0x241061E0 loops while parent+0x10004 names a
// currently bound entity. Each iteration constructs handle=(serial<<12)|
// index, invokes one of several engine virtual callback paths and finally
// calls native 0x24128180 to unbind that handle. It then zeros three
// counters and sets the highest slot to -1. Host callbacks own the real
// entity handles and the still-opaque game virtual method behavior.
struct EntityDrainCallbacks {
    virtual ~EntityDrainCallbacks()=default;
    virtual std::uint32_t current_primary_handle()=0;
    virtual void dispatch_native_entity_virtuals(std::uint32_t handle)=0;
    virtual void remove_entity_handle_24128180(std::uint32_t handle)=0;
};
struct EntityCounters {
    std::uint32_t live_count{};
    std::uint32_t state_slot_count{};
    std::uint32_t second_count{};
    std::uint32_t highest_slot{0xFFFFFFFFu};
};
inline void drain_live_entities(EntityCounters& state,
                               EntityDrainCallbacks& callbacks,
                               std::size_t max_iterations) {
    // Original code has no iteration bound. Host rejects an opaque
    // virtual callback that leaves the same entity bound forever.
    std::size_t iterations=0u;
    for(auto handle=callbacks.current_primary_handle();handle!=0xFFFFFFFFu;
             handle=callbacks.current_primary_handle()) {
        if(++iterations>max_iterations)
            throw std::runtime_error("host entity-list drain failed to advance");
        callbacks.dispatch_native_entity_virtuals(handle);
        callbacks.remove_entity_handle_24128180(handle);
    }
    state.live_count=0u;
    state.state_slot_count=0u;
    state.second_count=0u;
    state.highest_slot=0xFFFFFFFFu;
}

// Original parent destructor 0x24107430 restores the two vptrs,
// runs 0x241061E0, releases the map storage when growth policy != -1,
// resets and conditionally frees the separate 16/8-byte list containers,
// then tail-JMPs to base destructor 0x24128070.
// Pointer fields here are inert 32-bit tokens, NEVER dereferenced.
struct Storage {
    std::uint32_t pointer_token{};
    std::uint32_t capacity{};
    std::uint32_t growth_policy{};
};
struct ParentState {
    std::uint32_t parent_vptr{};
    std::uint32_t embedded_vptr{};
    Storage map16{};
    Storage list16{};
    Storage aux8{};
};
struct ParentDestructionCallbacks {
    virtual ~ParentDestructionCallbacks()=default;
    virtual void drain_241061E0()=0;
    virtual void list16_reset_24106360()=0;
    virtual void aux8_reset_24105DD0()=0;
    virtual void free_storage_2416F940(std::uint32_t pointer_token)=0;
    virtual void tail_base_destructor_24128070()=0;
};
inline void maybe_release_original_storage(Storage& slot,
                                           ParentDestructionCallbacks& callbacks) {
    if(slot.growth_policy==kFixedGrowthPolicy)return;
    if(slot.pointer_token!=0u) {
        callbacks.free_storage_2416F940(slot.pointer_token);
        slot.pointer_token=0u;
    }
    slot.capacity=0u;
}
inline void destruct_parent(ParentState& state,ParentDestructionCallbacks& cb) {
    state.parent_vptr=kParentVtableVA;
    state.embedded_vptr=kEmbeddedVtableVA;
    cb.drain_241061E0();
    maybe_release_original_storage(state.map16,cb);
    cb.list16_reset_24106360();
    maybe_release_original_storage(state.list16,cb);
    cb.aux8_reset_24105DD0();
    maybe_release_original_storage(state.aux8,cb);
    cb.tail_base_destructor_24128070();
}

struct Aux8State {
    list::Aux8Manager list;
    explicit Aux8State(std::size_t capacity):list(capacity){}
};
struct List16State {
    std::vector<list::List16Node> nodes;
    std::uint16_t free_head{kAbsent};
    std::uint16_t live_count{};
    explicit List16State(std::size_t capacity):nodes(capacity) {
        if(capacity>=kAbsent)
            throw std::out_of_range("synthetic 16-byte list capacity invalid");
    }
};

// 0x241082C0 and 0x241082F0 always call their corresponding native
// active-node unlinker before pushing the index onto the free chain.
// The native wrappers do not test whether the callee unlinked anything.
// The original unlink bodies live in Phases232/233, not these functions.
struct RecycleCallbacks {
    virtual ~RecycleCallbacks()=default;
    virtual void unlink_aux8_24106C20(Aux8State& list,std::uint16_t index)=0;
    virtual void unlink_list16_24106CB0(List16State& list,
                                         std::uint16_t index)=0;
};
inline void recycle_aux8(Aux8State& state,std::uint16_t idx,
                         RecycleCallbacks& cb) {
    if(idx>=state.list.nodes.size())
        throw std::out_of_range("aux8 recycle index outside host storage");
    cb.unlink_aux8_24106C20(state,idx);
    auto& node=state.list.nodes[idx];
    node.next=state.list.free_head;
    state.list.free_head=idx;
}
inline void recycle_list16(List16State& state,std::uint16_t idx,
                           RecycleCallbacks& cb) {
    if(idx>=state.nodes.size())
        throw std::out_of_range("list16 recycle index outside host storage");
    cb.unlink_list16_24106CB0(state,idx);
    auto& node=state.nodes[idx];
    node.next=state.free_head;
    state.free_head=idx;
}

// Original allocation frontends call allocator with PUSH 0 then insert
// the newly allocated index into an existing list via 0x241075E0 or
// 0x24107660. Only the 8-byte wrapper copies an input DWORD payload.
// Underlying native growth and link implementations are injected.
struct ListAllocateCallbacks {
    virtual ~ListAllocateCallbacks()=default;
    virtual std::uint16_t acquire_aux8_zero_24106570(Aux8State& state)=0;
    virtual void link_aux8_241075E0(Aux8State& state,
            std::uint16_t before,std::uint16_t acquired)=0;
    virtual std::uint16_t acquire_list16_zero_24106610(List16State& state)=0;
    virtual void link_list16_24107660(List16State& state,
            std::uint16_t before,std::uint16_t acquired)=0;
};
inline std::uint16_t acquire_link_aux8(Aux8State& state,
                                      std::uint32_t payload,
                                      std::uint16_t before,
                                      ListAllocateCallbacks& cb) {
    const auto id=cb.acquire_aux8_zero_24106570(state);
    if(id>=state.list.nodes.size())
        throw std::out_of_range("aux8 allocated index invalid");
    cb.link_aux8_241075E0(state,before,id);
    state.list.nodes[id].payload=payload;
    return id;
}
inline std::uint16_t acquire_link_list16(List16State& state,
                                         std::uint16_t before,
                                         ListAllocateCallbacks& cb) {
    const auto id=cb.acquire_list16_zero_24106610(state);
    if(id>=state.nodes.size())
        throw std::out_of_range("list16 allocated index invalid");
    cb.link_list16_24107660(state,before,id);
    return id;
}

// 0x24108390 delegates map index allocation to SHA-reviewed 0x24106700
// and map tree attachment to SHA-reviewed 0x241076E0, increments the
// native map live_count WORD+0x12, returns AX. No payload copy here:
// this is distinct from the Phase236 0x24108750 insertion frontend.
inline std::uint16_t acquire_attach_map(map::Map& manager,
     index::IndexState& indices,index::IndexCallbacks& alloc,
     index::TreeAttachCallback& attach,
     std::uint16_t parent,bool side_left) {
    const auto new_index=index::allocate_index(manager,indices,alloc);
    (void)map::checked(manager,new_index);
    attach.attach(manager,new_index,{parent,side_left});
    manager.live_count=static_cast<std::uint16_t>(manager.live_count+1u);
    return new_index;
}

} // namespace sin_compat::original_client_entitylist_pvs_lifecycle_wrappers
