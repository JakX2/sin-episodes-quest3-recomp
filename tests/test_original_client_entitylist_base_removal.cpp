#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "sin_compat/original_client_entitylist_base_removal.hpp"

namespace native=sin_compat::original_client_entitylist_base_removal;

struct Spy final:native::Callbacks {
    std::vector<std::string> events;
    std::vector<std::uint32_t> old_handles;
    void entity_virtual_slot1(std::uint32_t entity,
                              std::uint32_t& output) override {
        assert(output==0xFFFFFFFFu);
        events.push_back("entity:"+std::to_string(entity));
        output=0u;
    }
    void parent_virtual_slot1(std::uint32_t entity,
                              std::uint32_t handle) override {
        events.push_back("parent:"+std::to_string(entity));
        old_handles.push_back(handle);
    }
};
static void test_source_grounded_native_metadata() {
    static_assert(native::kNativeFreeListAppendVA==0x24127E20u);
    static_assert(native::kNativeRemoveIndexVA==0x24127FC0u);
    static_assert(native::kNativeBaseDestructorVA==0x24128070u);
    static_assert(native::kNativeMaskedHandleVA==0x24128180u);
    static_assert(native::kNativeBaseVtableVA==0x24298EB4u);
    static_assert(native::kSlotCount==4096u);
    static_assert(native::kSlotStride==16u);
    static_assert(native::kIndexMask==0xFFFu);
    static_assert(native::kSerialMask==0x7FFFu);
    static_assert(native::kHighFreeStart==2048u);
    static_assert(native::kLiveHeadPointerOffset==0x10004u);
    static_assert(native::kLiveTailPointerOffset==0x10008u);
    static_assert(native::kHighFreeHeadPointerOffset==0x1000Cu);
    static_assert(native::kHighFreeTailPointerOffset==0x10010u);
    static_assert(native::kNewCompleteOriginalMethods==4u);
    static_assert(native::kNewWholeOriginalCodeBytes==333u);
}
static void test_masked_handle_removal_uses_old_serial_and_wraps() {
    native::State state;
    native::seed_bound(state,7u,101u,0x7FFFu);
    native::seed_bound(state,8u,202u,0x22u);
    Spy spy;
    const bool removed=native::remove_packed_handle(state,0xDEAD0007u,spy);
    assert(removed);
    assert((spy.events==std::vector<std::string>{
        "entity:101","parent:101"}));
    assert(spy.old_handles.size()==1u);
    assert(spy.old_handles[0]==((0x7FFFu<<12)|7u));
    assert(native::checked(state,7u).serial==0u);
    assert(native::checked(state,7u).entity_token==0u);
    assert(state.active.head==8u&&state.active.tail==8u);
    assert(native::checked(state,7u).next==7u);
    assert(native::checked(state,7u).previous==7u);
    assert(state.high_free.head==native::kMissing);
    assert(!native::remove_raw_index(state,7u,spy));
    assert(spy.events.size()==2u);
}
static void test_high_slots_join_separate_free_tail_chain() {
    native::State state;
    native::seed_bound(state,2048u,111u,5u);
    native::seed_bound(state,2049u,222u,6u);
    native::seed_bound(state,2050u,333u,7u);
    Spy spy;
    assert(native::remove_raw_index(state,2048u,spy));
    assert(native::remove_raw_index(state,2049u,spy));
    assert(native::remove_raw_index(state,2050u,spy));
    assert(state.active.head==native::kMissing);
    assert(state.active.tail==native::kMissing);
    assert(state.high_free.head==2048u);
    assert(state.high_free.tail==2050u);
    assert(native::checked(state,2048u).next==2049u);
    assert(native::checked(state,2049u).previous==2048u);
    assert(native::checked(state,2049u).next==2050u);
    assert(native::checked(state,2050u).next==native::kMissing);
    for(const auto id:{2048u,2049u,2050u}) {
        assert(native::checked(state,static_cast<std::uint16_t>(id)).in_high_free);
    }
    // Original 0x24127E20 can move an already-linked node to list tail.
    native::move_to_tail(state.high_free,state,2048u);
    assert(state.high_free.head==2049u);
    assert(state.high_free.tail==2048u);
    assert(native::checked(state,2050u).next==2048u);
    assert(native::checked(state,2048u).previous==2050u);
    assert(native::checked(state,2048u).next==native::kMissing);
}
static void test_reuse_high_free_index_and_separate_chains() {
    native::State state;
    native::seed_bound(state,2051u,77u,0u);
    Spy spy;
    native::remove_raw_index(state,2051u,spy);
    assert(state.high_free.head==2051u);
    native::seed_bound(state,2051u,88u,33u);
    assert(state.high_free.head==native::kMissing);
    assert(state.high_free.tail==native::kMissing);
    assert(state.active.head==2051u && state.active.tail==2051u);
    assert(!native::checked(state,2051u).in_high_free);
    assert(native::checked(state,2051u).serial==33u);
    assert(native::checked(state,2051u).entity_token==88u);
}
static void test_destructor_saves_next_and_unbinds_all() {
    native::State state;
    native::seed_bound(state,3u,103u,1u);
    native::seed_bound(state,2048u,104u,2u);
    native::seed_bound(state,6u,106u,3u);
    native::seed_bound(state,2050u,105u,4u);
    Spy spy;
    native::destroy_base(state,spy);
    assert(state.vtable_token==native::kNativeBaseVtableVA);
    assert(state.active.head==native::kMissing);
    assert(state.active.tail==native::kMissing);
    assert(state.high_free.head==2048u);
    assert(state.high_free.tail==2050u);
    assert(native::checked(state,2048u).next==2050u);
    assert(native::checked(state,2050u).previous==2048u);
    assert((spy.events==std::vector<std::string>{
        "entity:103","parent:103","entity:104","parent:104",
        "entity:106","parent:106","entity:105","parent:105"}));
    assert((spy.old_handles==std::vector<std::uint32_t>{
        (1u<<12)|3u,(2u<<12)|2048u,
        (3u<<12)|6u,(4u<<12)|2050u}));
    assert(native::checked(state,3u).serial==2u);
    assert(native::checked(state,2048u).serial==3u);
    assert(native::checked(state,6u).serial==4u);
    assert(native::checked(state,2050u).serial==5u);
    native::destroy_base(state,spy);
    assert(spy.events.size()==8u); // empty second pass
}
static void test_null_and_out_of_range_fail_closed() {
    native::State state(12u);
    Spy spy;
    assert(!native::remove_raw_index(state,4u,spy));
    assert(state.active.head==native::kMissing);
    bool caught=false;
    try{(void)native::remove_raw_index(state,12u,spy);}
    catch(const std::out_of_range&){caught=true;}
    assert(caught);
    caught=false;
    try{(void)native::remove_raw_index(state,4096u,spy);}
    catch(const std::out_of_range&){caught=true;}
    assert(caught);
    caught=false;
    try{native::seed_bound(state,5u,0u,0u);}
    catch(const std::invalid_argument&){caught=true;}
    assert(caught);
    caught=false;
    try{native::State oversized(4097u);(void)oversized;}
    catch(const std::out_of_range&){caught=true;}
    assert(caught);
    native::destroy_base(state,spy);
    assert(state.vtable_token==native::kNativeBaseVtableVA);
    assert(spy.events.empty());
}
int main() {
    test_source_grounded_native_metadata();
    test_masked_handle_removal_uses_old_serial_and_wraps();
    test_high_slots_join_separate_free_tail_chain();
    test_reuse_high_free_index_and_separate_chains();
    test_destructor_saves_next_and_unbinds_all();
    test_null_and_out_of_range_fail_closed();
    return 0;
}
