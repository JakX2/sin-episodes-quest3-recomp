#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "sin_compat/original_client_pvs_aux_helpers.hpp"

namespace n=sin_compat::original_client_pvs_aux_helpers;
struct Allocator final:n::IAuxIndexAllocator {
    std::vector<std::uint16_t> indices;
    std::size_t next{};
    explicit Allocator(std::vector<std::uint16_t> v):indices(std::move(v)){}
    std::uint16_t acquire_index() override {
        if(next>=indices.size())throw std::out_of_range("synthetic allocator exhausted");
        return indices[next++];
    }
};
struct PvsHost final:n::IPvsHost {
    std::vector<std::string> events;
    std::uint32_t notifier{0x12340000u};
    std::uint32_t identity{0xAAAABBBBu};
    std::uint16_t map_index{9u};
    std::uint16_t stored_node_word{7u};
    std::uint16_t previous_list_index{5u};
    std::uint16_t allocation_index{13u};
    std::uint32_t entity_virtual_14(std::uint32_t entity) override {
        assert(entity==0xD00DF00Du);
        events.emplace_back("entity_v14");return notifier;
    }
    std::uint32_t notifier_virtual_48(std::uint32_t entity) override {
        assert(entity==notifier);
        events.emplace_back("notifier_v48");return identity;
    }
    std::uint16_t sixteen_list_acquire_241086F0() override {
        events.emplace_back("alloc_16");return allocation_index;
    }
    void record_sixteen_node(std::uint16_t index,std::uint32_t key,
                             std::uint32_t obj) override {
        assert(index==allocation_index && key==identity && obj==notifier);
        events.emplace_back("write_16_node");
    }
    void original_global_pointer_write(std::uint32_t global,
                                       std::uint32_t value) override {
        assert(global==n::kPvsMapGlobalVA && value==0x11112222u);
        events.emplace_back("global_store");
    }
    void map_link_24108750(std::uint32_t key,std::uint32_t ptr) override {
        assert(key==identity && ptr==notifier);
        events.emplace_back("map_link");
    }
    std::uint16_t map_find_24106B20(std::uint32_t key) override {
        assert(key==identity);
        events.emplace_back("map_find");return map_index;
    }
    std::uint16_t map_node_word_24105F40(std::uint16_t index) override {
        assert(index==map_index);
        events.emplace_back("map_node");return stored_node_word;
    }
    std::uint16_t original_sixteen_node_prev(std::uint16_t index) override {
        assert(index==map_index);
        events.emplace_back("read_prev");return previous_list_index;
    }
    void sixteen_unlink_24106CB0(std::uint16_t index) override {
        assert(index==previous_list_index);
        events.emplace_back("unlink_16");
    }
    void sixteen_repair_links(std::uint16_t prev,
                             std::uint16_t index) override {
        assert(prev==previous_list_index && index==map_index);
        events.emplace_back("repair_16");
    }
    void map_unlink_24108840(std::uint16_t index) override {
        assert(index==map_index);
        events.emplace_back("map_unlink");
    }
    void warning_tier0(std::string_view msg) override {
        assert(msg==n::kNativeWarning);
        events.emplace_back("tier0_Warning");
    }
};
static void test_native_addresses_and_distinct_8_and_16_byte_tables() {
    static_assert(n::kReviewedOriginalMethods==4u);
    static_assert(n::kReviewedOriginalNativeBytes==564u);
    static_assert(n::kNativeAuxNodeStride==8u);
    static_assert(n::kNativeAuxPrevWordOffset==4u);
    static_assert(n::kNativeAuxNextWordOffset==6u);
    static_assert(n::kNativeInvalidIndex==0xFFFFu);
    static_assert(n::kPvsMapGlobalVA==0x2433FDB8u);
    static_assert(n::kPvsParentMapPointerOffset==0x1807Cu);
    static_assert(n::kPvsParentSixteenListOffset==0x18044u);
    static_assert(n::kPvsParentMapOffset==0x18060u);
    std::size_t byte_sum=0;
    for(const auto& m:n::kMethods){
        assert(m.native_ret4_branches==1u || m.native_ret4_branches==2u);
        byte_sum+=m.original_code_bytes;
    }
    assert(byte_sum==564u);
    assert(n::kMethods[0].original_va==0x24108680u);
    assert(n::kMethods[1].original_va==0x24106C20u);
    assert(n::kMethods[2].original_va==0x24108880u);
    assert(n::kMethods[3].original_va==0x24108910u);
}
static void test_auxiliary_list_link_and_unlink_order() {
    n::AuxList list(4);
    assert(list.active_count==0u);
    assert(list.head==n::kNativeInvalidIndex && list.tail==n::kNativeInvalidIndex);
    assert(!n::unlink(list,0u));
    Allocator alloc({1u,2u,3u,2u});
    assert(n::acquire_append(list,11u,alloc)==1u);
    assert(list.head==1u && list.tail==1u && list.active_count==1u);
    assert(n::node(list,1u).prev==n::kNativeInvalidIndex);
    assert(n::node(list,1u).next==n::kNativeInvalidIndex);
    assert(n::node(list,1u).payload==11u);
    n::acquire_append(list,22u,alloc);
    n::acquire_append(list,33u,alloc);
    assert(list.head==1u && list.tail==3u && list.active_count==3u);
    assert(n::node(list,1u).next==2u && n::node(list,2u).prev==1u);
    assert(n::node(list,2u).next==3u && n::node(list,3u).prev==2u);
    assert(n::unlink(list,2u));
    assert(n::node(list,2u).prev==2u && n::node(list,2u).next==2u);
    assert(n::node(list,1u).next==3u && n::node(list,3u).prev==1u);
    assert(list.active_count==2u);
    n::acquire_append(list,44u,alloc);
    assert(list.head==1u && list.tail==2u && list.active_count==3u);
    assert(n::node(list,3u).next==2u && n::node(list,2u).prev==3u);
    assert(n::node(list,2u).payload==44u);
    assert(n::unlink(list,1u));
    assert(list.head==3u && n::node(list,3u).prev==n::kNativeInvalidIndex);
    assert(n::unlink(list,2u));
    assert(list.tail==3u && n::node(list,3u).next==n::kNativeInvalidIndex);
    assert(n::unlink(list,3u));
    assert(list.head==n::kNativeInvalidIndex);
    assert(list.tail==n::kNativeInvalidIndex);
    assert(list.active_count==0u);
    assert(!n::unlink(list,3u));
    assert(!n::unlink(list,0xFFFFu));
}
static void test_auxiliary_reused_active_slot_preserves_count() {
    n::AuxList list(3);
    Allocator alloc({0u,1u,0u});
    n::acquire_append(list,10u,alloc);
    n::acquire_append(list,20u,alloc);
    assert(list.head==0u && list.tail==1u && list.active_count==2u);
    n::acquire_append(list,30u,alloc);
    assert(list.head==1u && list.tail==0u && list.active_count==2u);
    assert(n::node(list,1u).next==0u && n::node(list,0u).prev==1u);
    assert(n::node(list,0u).payload==30u);
    bool refused=false;
    Allocator bad({3u});
    try {n::acquire_append(list,8u,bad);}
    catch(const std::out_of_range&){refused=true;}
    assert(refused && list.active_count==2u);
    refused=false;
    try {n::AuxList too_many(65536u);(void)too_many;}
    catch(const std::out_of_range&){refused=true;}
    assert(refused);
}
static void test_original_pvs_register_events_and_early_exit() {
    PvsHost h;
    n::PvsState state{n::kPvsMapGlobalVA,0x11112222u};
    assert(n::observe_register(0xD00DF00Du,state,h));
    assert((h.events==std::vector<std::string>{
       "entity_v14","notifier_v48","alloc_16","write_16_node",
       "global_store","map_link"}));
    h.events.clear();
    state.original_global_candidate=0u;
    assert(n::observe_register(0xD00DF00Du,state,h));
    assert((h.events==std::vector<std::string>{
       "entity_v14","notifier_v48","alloc_16","write_16_node","map_link"}));
    h.events.clear();
    h.notifier=0u;
    assert(!n::observe_register(0xD00DF00Du,state,h));
    assert((h.events==std::vector<std::string>{"entity_v14"}));
    h.events.clear();h.notifier=0x12340000u;h.identity=0u;
    assert(!n::observe_register(0xD00DF00Du,state,h));
    assert((h.events==std::vector<std::string>{"entity_v14","notifier_v48"}));
}
static void test_original_pvs_unregister_normal_warning_and_missing_cases() {
    PvsHost h;n::PvsState state{n::kPvsMapGlobalVA,0x11112222u};
    assert(n::observe_unregister(0xD00DF00Du,state,h)==n::RemoveResult::Removed);
    assert((h.events==std::vector<std::string>{
        "entity_v14","notifier_v48","global_store","map_find",
        "map_node","read_prev","unlink_16","repair_16","map_unlink"}));
    h.events.clear();
    h.stored_node_word=h.map_index;
    assert(n::observe_unregister(0xD00DF00Du,state,h)==
           n::RemoveResult::WarningNotInMap);
    assert((h.events==std::vector<std::string>{
        "entity_v14","notifier_v48","global_store","map_find",
        "map_node","tier0_Warning"}));
    h.events.clear();h.notifier=0u;
    assert(n::observe_unregister(0xD00DF00Du,state,h)==n::RemoveResult::NoNotifier);
    assert((h.events==std::vector<std::string>{"entity_v14"}));
    h.events.clear();h.notifier=0x12340000u;h.identity=0u;
    assert(n::observe_unregister(0xD00DF00Du,state,h)==n::RemoveResult::NoIdentity);
    assert((h.events==std::vector<std::string>{"entity_v14","notifier_v48"}));
}
static void test_host_null_pointer_guard() {
    PvsHost h;
    bool blocked=false;
    try{n::observe_register(0u,{},h);}
    catch(const std::invalid_argument&){blocked=true;}
    assert(blocked && h.events.empty());
    blocked=false;
    try{n::observe_unregister(0u,{},h);}
    catch(const std::invalid_argument&){blocked=true;}
    assert(blocked && h.events.empty());
}
int main() {
    test_native_addresses_and_distinct_8_and_16_byte_tables();
    test_auxiliary_list_link_and_unlink_order();
    test_auxiliary_reused_active_slot_preserves_count();
    test_original_pvs_register_events_and_early_exit();
    test_original_pvs_unregister_normal_warning_and_missing_cases();
    test_host_null_pointer_guard();
}
