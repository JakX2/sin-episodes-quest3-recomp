#include "sin_compat/original_server_symbol_tree_storage.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
namespace s = sin_compat::original_server_symbol_tree_storage;
namespace {
struct SlotHost {
    std::vector<std::string> log;
    std::vector<s::Node12> nodes;
    void grow_slot_storage(s::SymbolSlots& slots,unsigned additional) {
        assert(additional==1u);
        log.emplace_back("grow");
        slots.capacity+=additional;
        slots.allocation_token=0x55667788u;
    }
    std::uint16_t read_recycled_right_link(s::SymbolSlots& slots,
                                           std::uint16_t old_head) {
        log.emplace_back("read-next");
        assert(slots.free_head==old_head);
        return nodes.at(old_head).right;
    }
};
struct LinkHost {
    std::vector<s::Node12>* nodes=nullptr;
    std::uint16_t* root=nullptr;
    std::vector<std::uint16_t> calls;
    void rebalance_after_link(std::uint16_t child) {
        assert(nodes!=nullptr && root!=nullptr);
        const s::Node12& node=nodes->at(child);
        assert(node.left==s::kInvalidSlot);
        assert(node.right==s::kInvalidSlot);
        assert(node.metadata==0u);
        if(node.parent==s::kInvalidSlot)assert(*root==child);
        else assert(nodes->at(node.parent).left==child ||
                    nodes->at(node.parent).right==child);
        calls.push_back(child);
    }
};
struct AppendHost {
    std::vector<std::uint32_t> calls;
    void reserve_extra(s::AppendStorage& st,std::uint32_t count) {
        calls.push_back(count);
        // Explicit original 0x2228E280 host callback, not an invented
        // native reserve algorithm. The test updates observable fields.
        st.capacity+=count;
        st.allocation_token=0x99u;
    }
};
void test_original_12_byte_record_math_and_right_word() {
    static_assert(sizeof(s::Node12)==12);
    assert(s::original_node_byte_offset(0u)==0u);
    assert(s::original_node_byte_offset(1u)==12u);
    assert(s::original_node_byte_offset(0xffffu)==786420u);
    std::vector<s::Node12> nodes(3);
    nodes[1].payload=0xabcdeff0u;
    nodes[1].left=3u;
    s::original_set_right_link(nodes,1u,0u);
    assert(nodes[1].right==0u && nodes[1].left==3u);
    assert(nodes[1].payload==0xabcdeff0u);
    s::original_set_right_link(nodes,1u,0xffffu);
    assert(nodes[1].right==0xffffu);
    bool failed=false;
    volatile std::uint16_t out_of_range_index=4u;
    try{s::original_set_right_link(nodes,out_of_range_index,1u);}
    catch(const std::out_of_range&){failed=true;}
    assert(failed); // deliberate safe reference difference; native unchecked
}
void test_fresh_slot_grow_and_cache_order() {
    s::SymbolSlots slots{};
    slots.allocation_token=7u;
    SlotHost host{};
    assert(s::original_take_symbol_slot(slots,host)==0u);
    assert((host.log==std::vector<std::string>{"grow"}));
    assert(slots.used_count==1u && slots.capacity==1u);
    assert(slots.cached_allocation==0x55667788u);
    host.log.clear();slots.capacity=3;
    slots.allocation_token=0x1111u;
    assert(s::original_take_symbol_slot(slots,host)==1u);
    assert(host.log.empty() && slots.used_count==2u);
    assert(slots.cached_allocation==0x1111u);
    // Original counter is WORD-sized, so it wraps; native code has no
    // overflow check. This is not a claim that such a state is safe.
    slots.used_count=0xffffu;slots.capacity=0xffffffffu;
    assert(s::original_take_symbol_slot(slots,host)==0xffffu);
    assert(slots.used_count==0u);
}
void test_recycle_slot_preserves_used_counter() {
    s::SymbolSlots slots{};
    slots.free_head=2u;slots.used_count=5u;
    slots.capacity=7u;slots.allocation_token=0xa5a5u;
    SlotHost host{};host.nodes.resize(5);
    host.nodes[2].right=4u;
    assert(s::original_take_symbol_slot(slots,host)==2u);
    assert(slots.free_head==4u && slots.used_count==5u);
    assert(slots.cached_allocation==0xa5a5u);
    assert((host.log==std::vector<std::string>{"read-next"}));
    host.log.clear();host.nodes[4].right=s::kInvalidSlot;
    assert(s::original_take_symbol_slot(slots,host)==4u);
    assert(slots.free_head==s::kInvalidSlot);
    assert(slots.used_count==5u && host.log.size()==1u);
}
void test_link_root_left_right_and_payload_preservation() {
    std::vector<s::Node12> nodes(4);
    std::uint16_t root=s::kInvalidSlot;
    LinkHost host{&nodes,&root,{}};
    nodes[0].payload=0x88776655u;
    nodes[0].left=2u;nodes[0].right=3u;
    nodes[0].metadata=0xffffu;
    s::original_link_symbol_node(nodes,root,0u,s::kInvalidSlot,1u,host);
    assert(root==0u && nodes[0].parent==s::kInvalidSlot);
    assert(nodes[0].payload==0x88776655u);
    nodes[1].payload=42u;
    s::original_link_symbol_node(nodes,root,1u,0u,2u,host);
    assert(root==0u && nodes[0].left==1u);
    assert(nodes[1].parent==0u && nodes[1].payload==42u);
    nodes[2].payload=13u;
    s::original_link_symbol_node(nodes,root,2u,0u,0u,host);
    assert(nodes[0].right==2u && nodes[2].parent==0u);
    assert((host.calls==std::vector<std::uint16_t>{0u,1u,2u}));
    // zero link writer directly affects the same +2 WORD as the native link.
    s::original_set_right_link(nodes,0u,s::kInvalidSlot);
    assert(nodes[0].right==s::kInvalidSlot);
}
void test_signed_32bit_append_reserve_and_cache() {
    AppendHost host{};
    s::AppendStorage b{};b.capacity=20u;b.used=8u;
    b.allocation_token=0x111u;
    s::original_append_reserve(b,9u,host);
    assert(host.calls.empty());
    assert(b.used==17u && b.cached_allocation==0x111u);
    s::original_append_reserve(b,10u,host);
    assert((host.calls==std::vector<std::uint32_t>{7u}));
    assert(b.used==27u && b.capacity==27u);
    assert(b.cached_allocation==0x99u);
    // Bitwise x86 signed comparisons, not unsigned capacity arithmetic.
    host.calls.clear();b.used=0x80000000u;b.capacity=0x7fffffffu;
    s::original_append_reserve(b,0u,host);
    assert(host.calls.empty() && b.used==0x80000000u);
    b.used=0xffffffffu;b.capacity=3u;
    s::original_append_reserve(b,1u,host);
    assert(host.calls.empty() && b.used==0u);
    assert(s::native_signed32(0xffffffffu)==-1);
    assert(s::native_signed32(0x80000000u)==std::numeric_limits<std::int32_t>::min());
}
void test_native_allocator_then_tree_links_integration() {
    s::SymbolSlots slots{};SlotHost slots_host{};
    slots_host.nodes.resize(3);
    const auto root=s::original_take_symbol_slot(slots,slots_host);
    const auto left=s::original_take_symbol_slot(slots,slots_host);
    const auto right=s::original_take_symbol_slot(slots,slots_host);
    assert(root==0u && left==1u && right==2u);
    std::uint16_t root_idx=s::kInvalidSlot;
    LinkHost link_host{&slots_host.nodes,&root_idx,{}};
    s::original_link_symbol_node(slots_host.nodes,root_idx,root,s::kInvalidSlot,0u,link_host);
    s::original_link_symbol_node(slots_host.nodes,root_idx,left,root,1u,link_host);
    s::original_link_symbol_node(slots_host.nodes,root_idx,right,root,0u,link_host);
    assert(root_idx==0u && slots_host.nodes[0].left==1u &&
           slots_host.nodes[0].right==2u);
    assert(slots.used_count==3u && slots.capacity==3u);
    assert(link_host.calls.size()==3u);
}
} // namespace
int main(){
    test_original_12_byte_record_math_and_right_word();
    test_fresh_slot_grow_and_cache_order();
    test_recycle_slot_preserves_used_counter();
    test_link_root_left_right_and_payload_preservation();
    test_signed_32bit_append_reserve_and_cache();
    test_native_allocator_then_tree_links_integration();
}