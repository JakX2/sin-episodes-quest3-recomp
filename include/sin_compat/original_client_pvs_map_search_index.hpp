#pragma once
// Phase237 clean-room C++17 host reference for original 2006 SiN client
// PVS map insertion-location search and free-or-unused index allocation.
// Original native entries: 0x24106440 and 0x24106700.
// The map record layout and insert frontend come from Phase236.
// This is NOT an engine-owned original Source C++ object or gameplay DLL.
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include "sin_compat/original_client_pvs_map_frontends.hpp"

namespace sin_compat::original_client_pvs_map_search_index {

namespace map = sin_compat::original_client_pvs_map_frontends;

inline constexpr std::uint32_t kOriginalSearchVA=0x24106440u;
inline constexpr std::uint32_t kOriginalIndexAllocatorVA=0x24106700u;
inline constexpr std::size_t kOriginalReviewedMethods=2u;
inline constexpr std::size_t kOriginalReviewedNativeBytes=341u;
inline constexpr std::size_t kMapCapacityDword=0x08u;
inline constexpr std::size_t kMapRootWord=0x10u;
inline constexpr std::size_t kMapFreeHeadWord=0x14u;
inline constexpr std::size_t kMapNextUnusedWord=0x16u;
inline constexpr std::size_t kMapCachedPointerDword=0x18u;
inline constexpr std::size_t kNativeRecordStride=16u;
inline constexpr std::size_t kNativeFreeNextWord=2u;

// Native 0x24106440 calls the manager comparator EXACTLY once per visited
// node with (key, node.payload), writes parent WORD and side BYTE, and walks
// left on true, right on false. It does not reject equal keys. The extra
// fourth native stack argument is not read by this original code body.
// Original lazy global nil-sentinel registration is represented by kAbsent.
inline map::Placement find_insertion_placement(
    const map::Map& manager,
    const map::Payload& key,
    const map::Comparator& comparator) {
    map::Placement placement{};
    std::uint16_t current=manager.root;
    std::size_t traversed=0u;
    while(current!=map::kAbsent) {
        // Safety-only: original x86 assumes a valid acyclic tree.
        if(++traversed>manager.nodes.size())
            throw std::runtime_error("cyclic synthetic PVS placement tree");
        const auto& node=map::checked(manager,current);
        placement.parent=current;
        placement.as_left=comparator.less(key,node.payload);
        current=placement.as_left?node.left:node.right;
    }
    return placement;
}

// Original allocator uses manager+0x14 free-list head; when absent, reads
// virgin WORD at +0x16, compares to DWORD capacity at +0x08, and invokes
// growth method 0x24106140 with this+0x04 and PUSH 1 on equality.
// The recycled path resolves its node via opaque 0x24105F40 and reads +2.
// Both paths refresh cached base pointer at manager+0x18 and return AX via C3.
struct IndexState {
    std::uint16_t next_unused{};
    std::size_t cached_base_refreshes{}; // host trace; not an x86 pointer
};
struct IndexCallbacks {
    virtual ~IndexCallbacks()=default;
    // Original 0x24106140 is an unreviewed memory growth callee.
    virtual void grow_one(map::Map& manager)=0;
    // Original 0x24105F40 is an unreviewed node-address resolver;
    // the caller loads the resolved node's +2 WORD free-chain next index.
    virtual std::uint16_t recycled_next(
        map::Map& manager,std::uint16_t index)=0;
};

inline std::uint16_t allocate_index(
    map::Map& manager, IndexState& state, IndexCallbacks& callbacks) {
    if(manager.free_head!=map::kAbsent) {
        const auto index=manager.free_head;
        (void)map::checked(manager,index);
        const auto next=callbacks.recycled_next(manager,index);
        if(next!=map::kAbsent && next>=manager.nodes.size())
            throw std::out_of_range("invalid synthetic map free-chain successor");
        manager.free_head=next;
        ++state.cached_base_refreshes;
        return index;
    }
    // Original checks equality, not greater-than; host also rejects an
    // inconsistent state to avoid OOB access or 16-bit sentinel wrap.
    if(state.next_unused==manager.nodes.size())
        callbacks.grow_one(manager);
    if(state.next_unused==map::kAbsent ||
       state.next_unused>=manager.nodes.size())
        throw std::out_of_range("synthetic map index allocator exhausted");
    const auto index=state.next_unused;
    state.next_unused=static_cast<std::uint16_t>(index+1u);
    ++state.cached_base_refreshes;
    return index;
}

struct TreeAttachCallback {
    virtual ~TreeAttachCallback()=default;
    // Original 0x241076E0 remains unresolved; host owns actual tree links.
    virtual void attach(
        map::Map& manager,std::uint16_t index,map::Placement location)=0;
};

// Integrates these two recovered methods with Phase236's INSERT frontend,
// without incorrectly claiming to reconstruct original tree balancing.
// Call sequence: placement search -> index allocation -> tree attach ->
// Phase236 increments WORD count and copies two DWORD payload words.
struct InsertBridge final : map::InsertionBackend {
    IndexState& index_state;
    IndexCallbacks& index_callbacks;
    const map::Comparator& comparator;
    TreeAttachCallback& attach_callback;
    InsertBridge(IndexState& state,IndexCallbacks& callbacks,
                 const map::Comparator& cmp,TreeAttachCallback& attach):
        index_state(state),index_callbacks(callbacks),
        comparator(cmp),attach_callback(attach){}
    map::Placement find_placement(
        map::Map& manager,const map::Payload& key) override {
        return find_insertion_placement(manager,key,comparator);
    }
    std::uint16_t allocate_index(map::Map& manager) override {
        return original_client_pvs_map_search_index::allocate_index(
            manager,index_state,index_callbacks);
    }
    void attach(map::Map& manager,std::uint16_t index,
                map::Placement placement) override {
        attach_callback.attach(manager,index,placement);
    }
};

} // namespace sin_compat::original_client_pvs_map_search_index
