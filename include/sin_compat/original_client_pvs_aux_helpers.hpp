#pragma once
// Phase232 original 2006 SiN client.dll PVS notifier/auxiliary helpers.
// Original whole-SHA method references: 0x24108680, 0x24106C20,
// 0x24108880, 0x24108910. Safe independent host reference only.
// Does NOT construct engine-owned classes, invoke native original
// methods, dereference original 32-bit address tokens, or implement
// the unreviewed original 16-byte map/free-list helpers.
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace sin_compat::original_client_pvs_aux_helpers {
inline constexpr std::size_t kReviewedOriginalMethods=4u;
inline constexpr std::size_t kReviewedOriginalNativeBytes=564u;
inline constexpr std::size_t kNativeAuxNodeStride=8u;
inline constexpr std::size_t kNativeAuxPrevWordOffset=4u;
inline constexpr std::size_t kNativeAuxNextWordOffset=6u;
inline constexpr std::uint16_t kNativeInvalidIndex=0xFFFFu;
inline constexpr std::uint32_t kPvsMapGlobalVA=0x2433FDB8u;
inline constexpr std::uint32_t kPvsParentMapPointerOffset=0x1807Cu;
inline constexpr std::uint32_t kPvsParentSixteenListOffset=0x18044u;
inline constexpr std::uint32_t kPvsParentMapOffset=0x18060u;
inline constexpr std::string_view kNativeWarning="PVS notifier not in m_PVSNotifierMap\n";

struct ReviewedMethod {
    std::uint32_t original_va{};
    std::uint32_t original_code_bytes{};
    std::uint32_t native_ret4_branches{};
};
inline constexpr ReviewedMethod kMethods[]{{
    0x24108680u,105u,1u
},{
    0x24106C20u,137u,2u
},{
    0x24108880u,132u,1u
},{
    0x24108910u,190u,2u
}};

struct AuxNode {
    std::uint32_t payload{};
    std::uint16_t prev{kNativeInvalidIndex};
    std::uint16_t next{kNativeInvalidIndex};
};
struct AuxList {
    std::vector<AuxNode> nodes;
    std::uint16_t head{kNativeInvalidIndex};
    std::uint16_t tail{kNativeInvalidIndex};
    std::uint16_t active_count{};
    explicit AuxList(std::size_t capacity) : nodes(capacity) {
        if(capacity>kNativeInvalidIndex)
            throw std::out_of_range("original auxiliary 16-bit index capacity exceeded");
        for(std::size_t i=0;i<nodes.size();i++) {
            // Original 0x24106C20 treats prev==self as a free node.
            nodes[i].prev=static_cast<std::uint16_t>(i);
            nodes[i].next=static_cast<std::uint16_t>(i);
        }
    }
};
inline AuxNode& node(AuxList& list,std::uint16_t index) {
    if(index>=list.nodes.size())
        throw std::out_of_range("host refused unbacked original auxiliary entry");
    return list.nodes[index];
}
inline const AuxNode& node(const AuxList& list,std::uint16_t index) {
    if(index>=list.nodes.size())
        throw std::out_of_range("host refused unbacked original auxiliary entry");
    return list.nodes[index];
}
// Native 0x24106C20, thiscall+RET 4:
// reject index>=capacity or node.prev==index (free marker); otherwise
// repair previous/next list links, mark freed node prev=next=index and
// decrement original uint16 active count modulo 65536.
inline bool unlink(AuxList& list,std::uint16_t index) {
    if(index>=list.nodes.size())return false;
    const auto original=node(list,index);
    if(original.prev==index)return false;
    if(original.prev!=kNativeInvalidIndex)
        (void)node(list,original.prev);
    if(original.next!=kNativeInvalidIndex)
        (void)node(list,original.next);
    if(original.prev!=kNativeInvalidIndex)
        node(list,original.prev).next=original.next;
    else list.head=original.next;
    if(original.next!=kNativeInvalidIndex)
        node(list,original.next).prev=original.prev;
    else list.tail=original.prev;
    node(list,index).prev=index;
    node(list,index).next=index;
    list.active_count=static_cast<std::uint16_t>(list.active_count-1u);
    return true;
}
struct IAuxIndexAllocator {
    virtual ~IAuxIndexAllocator()=default;
    // Calls into original native 0x24106570; its real allocation and
    // capacity growth behavior remain outside this phase.
    virtual std::uint16_t acquire_index()=0;
};
// Native 0x24108680, thiscall+RET 4:
// reserve index via unreviewed 0x24106570; call 0x24106C20
// unconditionally, then append index at tail, update count, copy
// one input DWORD from pointer arg to node payload, return index.
// Host prechecks references and refuses invalid indices.
inline std::uint16_t acquire_append(AuxList& list,std::uint32_t payload,
                                    IAuxIndexAllocator& allocator) {
    const auto index=allocator.acquire_index();
    (void)node(list,index);
    (void)unlink(list,index);
    const auto old_tail=list.tail;
    if(old_tail!=kNativeInvalidIndex)
        (void)node(list,old_tail);
    AuxNode& n=node(list,index);
    n.next=kNativeInvalidIndex;
    n.prev=old_tail;
    list.tail=index;
    if(old_tail==kNativeInvalidIndex)list.head=index;
    else node(list,old_tail).next=index;
    list.active_count=static_cast<std::uint16_t>(list.active_count+1u);
    n.payload=payload;
    return index;
}

// Original 0x24108880 and 0x24108910 are PVS-notifier map helpers.
// The host translates original virtual calls and nested helper edges
// into explicit injected events. No original map, entity, vtable,
// allocator or C++ ownership graph is fabricated.
struct IPvsHost {
    virtual ~IPvsHost()=default;
    virtual std::uint32_t entity_virtual_14(std::uint32_t entity)=0;
    virtual std::uint32_t notifier_virtual_48(std::uint32_t notifier)=0;
    virtual std::uint16_t sixteen_list_acquire_241086F0()=0;
    virtual void record_sixteen_node(std::uint16_t index,
                                    std::uint32_t identity,
                                    std::uint32_t notifier)=0;
    virtual void original_global_pointer_write(std::uint32_t global_va,
                                               std::uint32_t pointer_token)=0;
    virtual void map_link_24108750(std::uint32_t identity,
                                   std::uint32_t notifier)=0;
    virtual std::uint16_t map_find_24106B20(std::uint32_t identity)=0;
    virtual std::uint16_t map_node_word_24105F40(std::uint16_t index)=0;
    virtual std::uint16_t original_sixteen_node_prev(std::uint16_t index)=0;
    virtual void sixteen_unlink_24106CB0(std::uint16_t prev_index)=0;
    virtual void sixteen_repair_links(std::uint16_t prev_index,
                                     std::uint16_t map_index)=0;
    virtual void map_unlink_24108840(std::uint16_t index)=0;
    virtual void warning_tier0(std::string_view message)=0;
};
struct PvsState {
    std::uint32_t original_parent_pointer_token{};
    // Native parent+0x1807C value: when nonzero original x86 copies
    // it into absolute original data global 0x2433FDB8.
    std::uint32_t original_global_candidate{};
};
// Emits only observed calls and writes, *not* the internals of the
// opaque original methods. If notifier or identity is null, native
// routine exits without touching the rest of the map.
inline bool observe_register(std::uint32_t entity,PvsState state,IPvsHost& host) {
    if(entity==0u)throw std::invalid_argument("host-only null entity identity");
    const auto notifier=host.entity_virtual_14(entity);
    if(notifier==0u)return false;
    const auto identity=host.notifier_virtual_48(notifier);
    if(identity==0u)return false;
    const auto index=host.sixteen_list_acquire_241086F0();
    host.record_sixteen_node(index,identity,notifier);
    if(state.original_global_candidate!=0u)
        host.original_global_pointer_write(kPvsMapGlobalVA,state.original_global_candidate);
    host.map_link_24108750(identity,notifier);
    return true;
}
enum class RemoveResult { NoNotifier, NoIdentity, WarningNotInMap, Removed };
// The native unregistration warning is original tier0.dll!Warning,
// not Msg. Native code checks the first WORD of the returned map
// node against the lookup index and emits the literal warning when
// they match. The normal branch then adjusts the separate 16-byte
// list via 0x24106CB0 and removes map index via 0x24108840.
inline RemoveResult observe_unregister(std::uint32_t entity,
                                       PvsState state,IPvsHost& host) {
    if(entity==0u)throw std::invalid_argument("host-only null entity identity");
    const auto notifier=host.entity_virtual_14(entity);
    if(notifier==0u)return RemoveResult::NoNotifier;
    const auto identity=host.notifier_virtual_48(notifier);
    if(identity==0u)return RemoveResult::NoIdentity;
    if(state.original_global_candidate!=0u)
        host.original_global_pointer_write(kPvsMapGlobalVA,state.original_global_candidate);
    const auto index=host.map_find_24106B20(identity);
    if(host.map_node_word_24105F40(index)==index) {
        host.warning_tier0(kNativeWarning);
        return RemoveResult::WarningNotInMap;
    }
    const auto prev=host.original_sixteen_node_prev(index);
    host.sixteen_unlink_24106CB0(prev);
    host.sixteen_repair_links(prev,index);
    host.map_unlink_24108840(index);
    return RemoveResult::Removed;
}
} // namespace sin_compat::original_client_pvs_aux_helpers
