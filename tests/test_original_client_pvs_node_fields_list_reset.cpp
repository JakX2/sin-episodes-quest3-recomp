#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_client_pvs_node_fields_list_reset.hpp"

namespace n=sin_compat::original_client_pvs_node_fields_list_reset;
namespace m=n::map;
namespace rb=n::rb;

static void test_native_layout_and_method_inventory() {
    static_assert(n::kNativeMethods==8u);
    static_assert(n::kOriginalCodeBytes==257u);
    static_assert(n::kNativeMethods==8u);
    static_assert(n::kAuxFreeChainResetVA==0x24105DD0u);
    static_assert(n::kList16ActiveVA==0x24105E80u);
    static_assert(n::kMapPayloadAddressVA==0x24105D10u);
    static_assert(n::kSetParentVA==0x24105EE0u);
    static_assert(n::kSetLeftVA==0x24105F00u);
    static_assert(n::kSetRightVA==0x24105F20u);
    static_assert(n::kReadColorVA==0x24105FA0u);
    static_assert(n::kSetColorVA==0x24105FC0u);
    static_assert(sizeof(n::Aux8Node)==8u);
    static_assert(sizeof(n::List16Node)==16u);
    static_assert(sizeof(m::Record)==16u);
}
static void test_aux_8_chain_reconstruction() {
    for(const std::size_t count:std::vector<std::size_t>{1u,2u,5u,32u}) {
        n::Aux8Manager manager(count);
        manager.head=3u;
        manager.tail=4u;
        manager.free_head=2u;
        manager.live_count=7u;
        for(std::size_t i=0;i<count;++i) {
            manager.nodes[i].payload=static_cast<std::uint32_t>(100u+i);
            manager.nodes[i].prev=0x1111u;
            manager.nodes[i].next=0x2222u;
        }
        n::reset_aux_8_free_chain(manager);
        assert(manager.head==n::kAbsent);
        assert(manager.tail==n::kAbsent);
        assert(manager.free_head==0u);
        assert(manager.live_count==0u);
        std::uint16_t free=manager.free_head;
        for(std::size_t i=0;i<count;++i) {
            assert(free==i);
            const auto& record=manager.nodes[free];
            assert(record.prev==i);
            assert(record.payload==100u+i); // native does not wipe payload
            free=record.next;
        }
        assert(free==n::kAbsent);
        n::reset_aux_8_free_chain(manager);
        assert(manager.free_head==0u&&manager.nodes[0].prev==0u);
    }
}
static void test_zero_capacity_original_fast_return() {
    n::Aux8Manager empty(0u);
    empty.head=3u;empty.tail=5u;empty.free_head=6u;
    empty.live_count=8u;
    n::reset_aux_8_free_chain(empty);
    // Unlike a blanket memset, original code returns without changes.
    assert(empty.head==3u&&empty.tail==5u);
    assert(empty.free_head==6u&&empty.live_count==8u);
    bool rejected=false;
    try {n::Aux8Manager huge(65535u);(void)huge;}
    catch(const std::out_of_range&){rejected=true;}
    assert(rejected);
}
static void test_separate_list16_active_predicate() {
    std::vector<n::List16Node> list(3u);
    // Even if both links are nil, native treats an in-bounds record
    // as active unless BOTH links are the node's own index.
    assert(n::list16_is_active(list,0u));
    list[0].prev=0u;list[0].next=0u;
    assert(!n::list16_is_active(list,0u));
    list[1].prev=1u;list[1].next=2u;
    assert(n::list16_is_active(list,1u));
    list[1].next=1u;
    assert(!n::list16_is_active(list,1u));
    list[2].prev=0u;list[2].next=2u;
    assert(n::list16_is_active(list,2u));
    // Keep invalid indices runtime-valued: GCC may otherwise emit a
    // misleading -Warray-bounds diagnostic for proven rejected indices.
    volatile std::uint16_t outside_capacity=3u;
    volatile std::uint16_t nil_index=n::kAbsent;
    assert(!n::list16_is_active(list,outside_capacity));
    assert(!n::list16_is_active(list,nil_index));
}
static void test_all_four_map_node_word_writes_and_color_read() {
    m::Map map(4u);
    map.nodes[2].payload={{1u,2u,3u,4u,5u,6u,7u,8u}};
    n::set_left_word(map,2u,1u);
    n::set_right_word(map,2u,3u);
    n::set_parent_word(map,2u,0xABCDu);
    n::set_color_word(map,2u,1u);
    assert(map.nodes[2].left==1u);
    assert(map.nodes[2].right==3u);
    assert(map.nodes[2].opaque_header==0x0001ABCDu);
    assert(n::read_color_via_resolver(map,2u)==1u);
    assert(n::read_color_via_resolver(map,n::kAbsent)==rb::kBlack);
    assert(n::map_payload_byte_offset(map,2u)==40u);
    const m::Payload expected_payload{{1u,2u,3u,4u,5u,6u,7u,8u}};
    assert(map.nodes[2].payload==expected_payload);
    n::set_color_word(map,2u,0x1234u);
    assert(map.nodes[2].opaque_header==0x1234ABCDu);
    n::set_parent_word(map,2u,0xFEDCu);
    assert(map.nodes[2].opaque_header==0x1234FEDCu);
    assert(n::read_color_via_resolver(map,2u)==0x1234u);
    // Native setter allows arbitrary WORD colors, unlike high-level
    // insertion balancing's red/black precondition checks.
    assert(n::map_payload_byte_offset(map,0u)==8u);
}
static void test_missing_host_indexes_fail_closed() {
    m::Map manager(1u);
    bool caught=false;
    try {n::set_left_word(manager,1u,0u);}
    catch(const std::out_of_range&){caught=true;}
    assert(caught);
    caught=false;
    try {n::set_color_word(manager,n::kAbsent,0u);}
    catch(const std::out_of_range&){caught=true;}
    assert(caught);
    caught=false;
    try {(void)n::map_payload_byte_offset(manager,5u);}
    catch(const std::out_of_range&){caught=true;}
    assert(caught);
    caught=false;
    try {(void)n::read_color_via_resolver(manager,5u);}
    catch(const std::out_of_range&){caught=true;}
    assert(caught);
}
static void test_map_field_write_works_with_existing_rb_rotation() {
    m::Map tree(3u);tree.root=0u;
    n::set_parent_word(tree,0u,n::kAbsent);
    n::set_color_word(tree,0u,rb::kBlack);
    n::set_right_word(tree,0u,1u);
    n::set_left_word(tree,0u,n::kAbsent);
    n::set_parent_word(tree,1u,0u);
    n::set_color_word(tree,1u,rb::kRed);
    n::set_left_word(tree,1u,2u);
    n::set_right_word(tree,1u,n::kAbsent);
    n::set_parent_word(tree,2u,1u);
    n::set_color_word(tree,2u,rb::kBlack);
    n::set_left_word(tree,2u,n::kAbsent);
    n::set_right_word(tree,2u,n::kAbsent);
    rb::rotate_left(tree,0u);
    assert(tree.root==1u);
    assert(rb::parent(tree,1u)==n::kAbsent);
    assert(tree.nodes[1].left==0u);
    assert(tree.nodes[0].right==2u);
    assert(rb::parent(tree,2u)==0u);
    assert(n::read_color_via_resolver(tree,1u)==rb::kRed);
    assert(n::read_color_via_resolver(tree,n::kAbsent)==rb::kBlack);
}
int main() {
    test_native_layout_and_method_inventory();
    test_aux_8_chain_reconstruction();
    test_zero_capacity_original_fast_return();
    test_separate_list16_active_predicate();
    test_all_four_map_node_word_writes_and_color_read();
    test_missing_host_indexes_fail_closed();
    test_map_field_write_works_with_existing_rb_rotation();
    return 0;
}
