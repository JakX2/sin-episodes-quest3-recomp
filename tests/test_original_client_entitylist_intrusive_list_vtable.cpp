#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_client_entitylist_intrusive_list_vtable.hpp"

namespace native=sin_compat::original_client_entitylist_intrusive_list_vtable;
namespace base=native::base;

static void verify_chain(const base::State& state,const base::Chain& chain,
                         const std::vector<std::uint16_t>& expected) {
    if(expected.empty()) {
        assert(chain.head==base::kMissing);
        assert(chain.tail==base::kMissing);
        return;
    }
    assert(chain.head==expected.front());
    assert(chain.tail==expected.back());
    std::uint16_t pos=chain.head;
    for(std::size_t i=0;i<expected.size();++i) {
        assert(pos==expected[i]);
        const auto& node=base::checked(state,pos);
        assert(node.previous==(i==0u?base::kMissing:expected[i-1u]));
        assert(node.next==(i+1u==expected.size()?base::kMissing:expected[i+1u]));
        assert(native::slot_is_linked_by_previous(state,pos));
        pos=node.next;
    }
    assert(pos==base::kMissing);
}
static void test_original_function_addresses_and_base_virtual_overrides() {
    static_assert(native::kSlotSelfLinkVA==0x24127C80u);
    static_assert(native::kListClearHeadsVA==0x24127C90u);
    static_assert(native::kSlotLinkedPredicateVA==0x24127CA0u);
    static_assert(native::kBaseVirtualSlot0VA==0x24127CC0u);
    static_assert(native::kBaseVirtualSlot1VA==0x24127CD0u);
    static_assert(native::kGetEntityPointerVA==0x24127CE0u);
    static_assert(native::kUnlinkVA==0x24127CF0u);
    static_assert(native::kInsertBeforeVA==0x24127D40u);
    static_assert(native::kInsertAfterVA==0x24127DB0u);
    static_assert(native::kOriginalBaseVtableVA==0x24298EB4u);
    static_assert(native::kOriginalDerivedParentVtableVA==0x242973B8u);
    static_assert(native::kOriginalEmbeddedEntityListVtableVA==0x24297390u);
    static_assert(native::kDerivedRegisterVirtualVA==0x241089D0u);
    static_assert(native::kDerivedRemoveVirtualVA==0x24108A70u);
    static_assert(native::kEmbeddedGetEntityVA==0x241056F0u);
    static_assert(native::kEmbeddedGetEntityViaHandleVA==0x24105BB0u);
    static_assert(native::kOriginalMethodCount==9u);
    static_assert(native::kWholeOriginalCodeBytes==327u);
    static_assert(native::kPrevDwordOffset==8u);
    static_assert(native::kNextDwordOffset==12u);
    static_assert(native::kHeadDwordOffset==0u);
    static_assert(native::kTailDwordOffset==4u);
    std::uint32_t output=0xABCD1000u;
    native::base_virtual_slot0_noop(999u,output);
    assert(output==0xABCD1000u);
    native::base_virtual_slot1_noop(123u,output);
    assert(output==0xABCD1000u);
    assert(native::kDerivedRegisterVirtualVA!=native::kBaseVirtualSlot0VA);
    assert(native::kDerivedRemoveVirtualVA!=native::kBaseVirtualSlot1VA);
}
static void test_original_selflink_clear_heads_linked_predicate_and_getter() {
    base::State state(8u);
    base::Chain chain{};
    auto& slot=base::checked(state,4u);
    slot.entity_token=0x1234ABCDu;
    slot.serial=0x1122u;
    slot.previous=5u;
    slot.next=6u;
    assert(native::slot_is_linked_by_previous(state,4u));
    assert(native::entity_token(state,4u)==0x1234ABCDu);
    native::self_link_slot(state,4u);
    assert(!native::slot_is_linked_by_previous(state,4u));
    assert(slot.previous==4u && slot.next==4u);
    assert(slot.entity_token==0x1234ABCDu&&slot.serial==0x1122u);
    // Original 0x24127CA0 only inspects PREVIOUS, not NEXT.
    slot.next=2u;
    assert(!native::slot_is_linked_by_previous(state,4u));
    slot.previous=base::kMissing;
    slot.next=4u;
    assert(native::slot_is_linked_by_previous(state,4u));
    native::self_link_slot(state,4u);
    chain.head=2u;chain.tail=5u;
    native::clear_list_heads(chain);
    assert(chain.head==base::kMissing&&chain.tail==base::kMissing);
    assert(native::entity_token(state,4u)==0x1234ABCDu);
}
static void test_intrusive_insert_before_after_and_move_existing_members() {
    base::State state(16u);
    base::Chain chain{};
    native::insert_before(chain,state,3u,base::kMissing);
    verify_chain(state,chain,{3u});
    native::insert_before(chain,state,1u,3u);
    verify_chain(state,chain,{1u,3u});
    native::insert_after(chain,state,5u,3u);
    verify_chain(state,chain,{1u,3u,5u});
    native::insert_after(chain,state,0u,base::kMissing);
    verify_chain(state,chain,{0u,1u,3u,5u});
    native::insert_before(chain,state,5u,1u);
    verify_chain(state,chain,{0u,5u,1u,3u});
    native::insert_after(chain,state,0u,3u);
    verify_chain(state,chain,{5u,1u,3u,0u});
    assert(native::unlink_if_linked(chain,state,1u));
    verify_chain(state,chain,{5u,3u,0u});
    assert(base::checked(state,1u).previous==1u);
    assert(base::checked(state,1u).next==1u);
    assert(!native::unlink_if_linked(chain,state,1u));
    verify_chain(state,chain,{5u,3u,0u});
    assert(native::unlink_if_linked(chain,state,5u));
    verify_chain(state,chain,{3u,0u});
    assert(native::unlink_if_linked(chain,state,0u));
    verify_chain(state,chain,{3u});
    assert(native::unlink_if_linked(chain,state,3u));
    verify_chain(state,chain,{});
}
static void test_one_element_prepend_append_and_direct_repositions() {
    base::State state(8u);base::Chain chain{};
    native::insert_after(chain,state,1u,base::kMissing);
    verify_chain(state,chain,{1u});
    native::insert_after(chain,state,2u,1u);
    verify_chain(state,chain,{1u,2u});
    native::insert_after(chain,state,3u,base::kMissing);
    verify_chain(state,chain,{3u,1u,2u});
    native::insert_before(chain,state,4u,base::kMissing);
    verify_chain(state,chain,{3u,1u,2u,4u});
    native::insert_before(chain,state,2u,3u);
    verify_chain(state,chain,{2u,3u,1u,4u});
    native::insert_after(chain,state,4u,2u);
    verify_chain(state,chain,{2u,4u,3u,1u});
}
static void test_host_refuses_malformed_graph_and_invalid_native_positions() {
    base::State state(8u);base::Chain chain{};
    native::insert_before(chain,state,1u,base::kMissing);
    native::insert_before(chain,state,2u,base::kMissing);
    const auto original=chain;
    bool refused=false;
    try{native::insert_before(chain,state,3u,3u);}
    catch(const std::logic_error&){refused=true;}
    assert(refused);
    refused=false;
    try{native::insert_before(chain,state,3u,4u);}
    catch(const std::logic_error&){refused=true;}
    assert(refused);
    refused=false;
    try{native::insert_after(chain,state,3u,4u);}
    catch(const std::logic_error&){refused=true;}
    assert(refused);
    volatile std::uint16_t invalid_index=8u;
    refused=false;
    try{native::insert_after(chain,state,invalid_index,base::kMissing);}
    catch(const std::out_of_range&){refused=true;}
    assert(refused);
    refused=false;
    try{(void)native::entity_token(state,invalid_index);}
    catch(const std::out_of_range&){refused=true;}
    assert(refused);
    verify_chain(state,chain,{1u,2u});
    assert(chain.head==original.head&&chain.tail==original.tail);
    // Host graph guards are deliberate additions, not present in retail x86.
    base::checked(state,2u).previous=6u;
    refused=false;
    try{(void)native::unlink_if_linked(chain,state,2u);}
    catch(const std::logic_error&){refused=true;}
    assert(refused);
    assert(chain.head==1u&&chain.tail==2u);
}
struct RemovalSpy final:base::Callbacks {
    std::vector<std::uint32_t> handles;
    std::vector<std::uint32_t> entities;
    void entity_virtual_slot1(std::uint32_t e,std::uint32_t& out) override {
        assert(out==0xFFFFFFFFu);
        entities.push_back(e);
    }
    void parent_virtual_slot1(std::uint32_t,std::uint32_t handle) override {
        handles.push_back(handle);
    }
};
static void test_cross_phase_native_entitylist_removal_and_destructor() {
    base::State state;
    base::Chain& active=state.active;
    for(const auto id:std::vector<std::uint16_t>{3u,5u,7u}) {
        auto& slot=base::checked(state,id);
        slot.entity_token=static_cast<std::uint32_t>(100u+id);
        slot.serial=id;
        native::insert_before(active,state,id,base::kMissing);
    }
    verify_chain(state,active,{3u,5u,7u});
    RemovalSpy spy;
    assert(base::remove_packed_handle(state,(5u<<12)|5u,spy));
    verify_chain(state,active,{3u,7u});
    assert(base::checked(state,5u).entity_token==0u);
    assert(!native::slot_is_linked_by_previous(state,5u));
    assert((spy.handles==std::vector<std::uint32_t>{(5u<<12)|5u}));
    native::insert_after(active,state,5u,3u);
    base::checked(state,5u).entity_token=105u;
    verify_chain(state,active,{3u,5u,7u});
    base::destroy_base(state,spy);
    verify_chain(state,active,{});
    assert(state.vtable_token==base::kNativeBaseVtableVA);
    assert((spy.handles==std::vector<std::uint32_t>{
        (5u<<12)|5u,(3u<<12)|3u,(6u<<12)|5u,(7u<<12)|7u}));
}
int main(){
    test_original_function_addresses_and_base_virtual_overrides();
    test_original_selflink_clear_heads_linked_predicate_and_getter();
    test_intrusive_insert_before_after_and_move_existing_members();
    test_one_element_prepend_append_and_direct_repositions();
    test_host_refuses_malformed_graph_and_invalid_native_positions();
    test_cross_phase_native_entitylist_removal_and_destructor();
    return 0;
}
