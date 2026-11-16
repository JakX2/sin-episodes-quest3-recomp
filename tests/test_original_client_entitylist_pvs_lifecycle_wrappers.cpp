#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include "sin_compat/original_client_entitylist_pvs_lifecycle_wrappers.hpp"
#include "sin_compat/original_client_pvs_map_node_growth.hpp"
#include "sin_compat/original_client_pvs_tree_attach.hpp"
#include "sin_compat/original_client_pvs_rb_fixup.hpp"

namespace life=sin_compat::original_client_entitylist_pvs_lifecycle_wrappers;
namespace map=life::map;
namespace list=life::list;
namespace index=life::index;
namespace growth=sin_compat::original_client_pvs_storage_growth;
namespace node=sin_compat::original_client_pvs_map_node_growth;
namespace attach=sin_compat::original_client_pvs_tree_attach;
namespace rb=sin_compat::original_client_pvs_rb_fixup;

static void test_original_entries_and_object_layout() {
    static_assert(life::kNewOriginalMethods==7u);
    static_assert(life::kNewOriginalNativeBytes==572u);
    static_assert(life::kDrainVA==0x241061E0u);
    static_assert(life::kDestructorVA==0x24107430u);
    static_assert(life::kRecycle8VA==0x241082C0u);
    static_assert(life::kRecycle16VA==0x241082F0u);
    static_assert(life::kAcquire8VA==0x24108320u);
    static_assert(life::kAcquire16VA==0x24108360u);
    static_assert(life::kAcquireMapVA==0x24108390u);
    static_assert(life::kParentVtableVA==0x242973B8u);
    static_assert(life::kEmbeddedVtableVA==0x24297390u);
    static_assert(life::kBaseDestructorVA==0x24128070u);
    static_assert(life::kOriginalFreeVA==0x2416F940u);
    static_assert(life::kOriginalRemoveEntityHandleVA==0x24128180u);
    static_assert(life::kOwnerPrimaryPointerOffset==0x10004u);
    static_assert(life::kOwnerLiveCountOffset==0x10018u);
    static_assert(life::kOwnerSecondCountOffset==0x10020u);
    static_assert(life::kOwnerHighestSlotOffset==0x10024u);
    static_assert(life::kAux8ManagerOwnerOffset==0x18028u);
    static_assert(life::kAux16ManagerOwnerOffset==0x18044u);
    static_assert(life::kMapManagerOwnerOffset==0x18060u);
}
struct DrainSpy final:life::EntityDrainCallbacks {
    std::vector<std::uint32_t> handles;
    std::vector<std::string> events;
    explicit DrainSpy(std::vector<std::uint32_t> h):handles(std::move(h)){}
    std::uint32_t current_primary_handle() override {
        return handles.empty()?0xFFFFFFFFu:handles.front();
    }
    void dispatch_native_entity_virtuals(std::uint32_t h) override {
        assert(!handles.empty()&&handles.front()==h);
        events.push_back("virtual:"+std::to_string(h));
    }
    void remove_entity_handle_24128180(std::uint32_t h) override {
        assert(!handles.empty()&&handles.front()==h);
        events.push_back("remove:"+std::to_string(h));
        handles.erase(handles.begin());
    }
};
struct Stalled final:life::EntityDrainCallbacks {
    std::uint32_t current_primary_handle() override{return 123u;}
    void dispatch_native_entity_virtuals(std::uint32_t) override{}
    void remove_entity_handle_24128180(std::uint32_t) override{}
};
static void test_drain_dispatch_observes_virtual_then_remove_order() {
    life::EntityCounters counters{4u,5u,6u,7u};
    DrainSpy spy({0x1001u,0x2002u,0x3003u});
    life::drain_live_entities(counters,spy,3u);
    assert(spy.handles.empty());
    assert((spy.events==std::vector<std::string>{
        "virtual:4097","remove:4097","virtual:8194","remove:8194",
        "virtual:12291","remove:12291"}));
    assert(counters.live_count==0u&&counters.state_slot_count==0u);
    assert(counters.second_count==0u&&counters.highest_slot==0xFFFFFFFFu);
    life::EntityCounters empty{1u,2u,3u,4u};
    DrainSpy empty_spy({});
    life::drain_live_entities(empty,empty_spy,0u);
    assert(empty_spy.events.empty()&&empty.highest_slot==0xFFFFFFFFu);
    life::EntityCounters stuck{11u,22u,33u,44u};
    Stalled malicious;
    bool caught=false;
    try {life::drain_live_entities(stuck,malicious,2u);}
    catch(const std::runtime_error&){caught=true;}
    assert(caught&&stuck.live_count==11u&&stuck.highest_slot==44u);
}
struct DestructSpy final:life::ParentDestructionCallbacks {
    std::vector<std::string> events;
    void drain_241061E0() override{events.push_back("drain");}
    void list16_reset_24106360() override{events.push_back("reset16");}
    void aux8_reset_24105DD0() override{events.push_back("reset8");}
    void free_storage_2416F940(std::uint32_t token) override{
        events.push_back("free:"+std::to_string(token));
    }
    void tail_base_destructor_24128070() override{
        events.push_back("base");
    }
};
static void test_parent_destructor_actual_order_and_policies() {
    life::ParentState p{};
    p.map16={11u,5u,0u};
    p.list16={22u,7u,0u};
    p.aux8={33u,9u,0u};
    DestructSpy cb;
    life::destruct_parent(p,cb);
    assert(p.parent_vptr==life::kParentVtableVA);
    assert(p.embedded_vptr==life::kEmbeddedVtableVA);
    assert((cb.events==std::vector<std::string>{
        "drain","free:11","reset16","free:22",
        "reset8","free:33","base"}));
    assert(p.map16.pointer_token==0u&&p.map16.capacity==0u);
    assert(p.list16.pointer_token==0u&&p.list16.capacity==0u);
    assert(p.aux8.pointer_token==0u&&p.aux8.capacity==0u);

    life::ParentState fixed{};
    fixed.map16={55u,10u,life::kFixedGrowthPolicy};
    fixed.list16={0u,12u,0u};
    fixed.aux8={99u,15u,life::kFixedGrowthPolicy};
    DestructSpy no_free;
    life::destruct_parent(fixed,no_free);
    assert((no_free.events==std::vector<std::string>{
        "drain","reset16","reset8","base"}));
    assert(fixed.map16.pointer_token==55u&&fixed.map16.capacity==10u);
    assert(fixed.aux8.pointer_token==99u&&fixed.aux8.capacity==15u);
    assert(fixed.list16.pointer_token==0u&&fixed.list16.capacity==0u);
}
struct ListSpy final:life::RecycleCallbacks,life::ListAllocateCallbacks {
    std::vector<std::string> events;
    std::uint16_t next_aux{};
    std::uint16_t next_sixteen{};
    std::uint32_t payload_before_link{};

    void unlink_aux8_24106C20(life::Aux8State& state,
                              std::uint16_t idx) override {
        events.push_back("unlink8:"+std::to_string(idx));
        state.list.nodes[idx].prev=idx;
        state.list.nodes[idx].next=idx;
    }
    void unlink_list16_24106CB0(life::List16State& state,
                                 std::uint16_t idx) override {
        events.push_back("unlink16:"+std::to_string(idx));
        state.nodes[idx].prev=idx;
        state.nodes[idx].next=idx;
    }
    std::uint16_t acquire_aux8_zero_24106570(life::Aux8State&) override {
        events.push_back("alloc8zero");
        return next_aux++;
    }
    void link_aux8_241075E0(life::Aux8State& state,std::uint16_t before,
                             std::uint16_t acquired) override {
        events.push_back("link8:"+std::to_string(before));
        payload_before_link=state.list.nodes[acquired].payload;
    }
    std::uint16_t acquire_list16_zero_24106610(life::List16State&) override {
        events.push_back("alloc16zero");
        return next_sixteen++;
    }
    void link_list16_24107660(life::List16State&,std::uint16_t before,
                               std::uint16_t) override {
        events.push_back("link16:"+std::to_string(before));
    }
};
static void test_aux8_and_list16_recycle_frontends() {
    life::Aux8State aux(4u);
    life::List16State sixteen(3u);
    ListSpy host;
    life::recycle_aux8(aux,1u,host);
    life::recycle_aux8(aux,2u,host);
    assert(aux.list.free_head==2u);
    assert(aux.list.nodes[2].next==1u);
    assert(aux.list.nodes[1].next==life::kAbsent);
    assert(aux.list.nodes[1].prev==1u&&aux.list.nodes[2].prev==2u);
    life::recycle_list16(sixteen,1u,host);
    life::recycle_list16(sixteen,2u,host);
    assert(sixteen.free_head==2u);
    assert(sixteen.nodes[2].next==1u&&sixteen.nodes[1].next==life::kAbsent);
    assert((host.events==std::vector<std::string>{
        "unlink8:1","unlink8:2","unlink16:1","unlink16:2"}));
    bool rejected=false;
    try {life::recycle_aux8(aux,4u,host);}
    catch(const std::out_of_range&){rejected=true;}
    assert(rejected&&host.events.size()==4u);
    rejected=false;
    try {life::recycle_list16(sixteen,life::kAbsent,host);}
    catch(const std::out_of_range&){rejected=true;}
    assert(rejected&&host.events.size()==4u);
}
static void test_separate_aux8_and_list16_index_allocate_frontends() {
    life::Aux8State aux(3u);
    life::List16State sixteen(3u);
    ListSpy host;
    const auto a=life::acquire_link_aux8(aux,0xAABBCCDDu,life::kAbsent,host);
    assert(a==0u&&aux.list.nodes[0].payload==0xAABBCCDDu);
    assert(host.payload_before_link==0u);
    const auto b=life::acquire_link_aux8(aux,99u,0u,host);
    assert(b==1u&&aux.list.nodes[1].payload==99u);
    assert((host.events==std::vector<std::string>{
        "alloc8zero","link8:65535","alloc8zero","link8:0"}));
    host.events.clear();
    const auto c=life::acquire_link_list16(sixteen,life::kAbsent,host);
    const auto d=life::acquire_link_list16(sixteen,0u,host);
    assert(c==0u&&d==1u);
    assert((host.events==std::vector<std::string>{
        "alloc16zero","link16:65535","alloc16zero","link16:0"}));
    host.next_aux=life::kAbsent;
    bool rejected=false;
    try{(void)life::acquire_link_aux8(aux,22u,life::kAbsent,host);}
    catch(const std::out_of_range&){rejected=true;}
    assert(rejected);
}
struct Memory final:growth::IMemoryBackend {
    std::vector<std::size_t> bytes;
    bool request(growth::MemoryAction,std::size_t length) override {
        bytes.push_back(length);return true;
    }
};
static void test_map_allocate_attach_without_payload_copy() {
    map::Map manager(0u);
    Memory backend;
    node::ResolverState sentinel;
    node::NativeIndexCallbacks alloc(manager,backend,sentinel);
    index::IndexState ids;
    rb::NativeRedBlackFixup fixup;
    attach::NativeTreeAttach tree_attach(fixup);
    const auto root=life::acquire_attach_map(
        manager,ids,alloc,tree_attach,life::kAbsent,false);
    assert(root==0u&&manager.root==root);
    assert(manager.live_count==1u&&fixup.callbacks==1u);
    assert(manager.nodes[root].payload==map::Payload{});
    // Payload belongs to a DIFFERENT frontend, not native 0x24108390.
    manager.nodes[root].payload={{20u,0u,0u,0u,0u,0u,0u,0u}};
    const auto left=life::acquire_attach_map(manager,ids,alloc,tree_attach,root,true);
    manager.nodes[left].payload={{10u,0u,0u,0u,0u,0u,0u,0u}};
    const auto right=life::acquire_attach_map(manager,ids,alloc,tree_attach,root,false);
    assert(left==1u&&right==2u);
    assert(manager.nodes[root].left==left&&manager.nodes[root].right==right);
    assert(manager.live_count==3u&&fixup.callbacks==3u);
    assert(manager.nodes[right].payload==map::Payload{});
    assert(manager.root==root&&rb::color(manager,root)==rb::kBlack);
    assert(ids.next_unused==3u&&ids.cached_base_refreshes==3u);
    assert((backend.bytes==std::vector<std::size_t>{32u,64u}));
}
int main() {
    test_original_entries_and_object_layout();
    test_drain_dispatch_observes_virtual_then_remove_order();
    test_parent_destructor_actual_order_and_policies();
    test_aux8_and_list16_recycle_frontends();
    test_separate_aux8_and_list16_index_allocate_frontends();
    test_map_allocate_attach_without_payload_copy();
    return 0;
}
