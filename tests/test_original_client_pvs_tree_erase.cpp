#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "sin_compat/original_client_pvs_tree_erase.hpp"
#include "sin_compat/original_client_pvs_map_node_growth.hpp"

namespace d=sin_compat::original_client_pvs_tree_erase;
namespace m=d::map;
namespace rb=d::rb;
namespace ix=sin_compat::original_client_pvs_map_search_index;
namespace attach=sin_compat::original_client_pvs_tree_attach;
namespace growth=sin_compat::original_client_pvs_storage_growth;
namespace nodes=sin_compat::original_client_pvs_map_node_growth;

static m::Payload key(std::uint32_t x) {
    return m::Payload{{
        static_cast<std::uint8_t>(x),
        static_cast<std::uint8_t>(x>>8),
        static_cast<std::uint8_t>(x>>16),
        static_cast<std::uint8_t>(x>>24),0,0,0,0}};
}
static std::uint32_t value(const m::Payload& k) {
    return static_cast<std::uint32_t>(k[0])|
       (static_cast<std::uint32_t>(k[1])<<8)|
       (static_cast<std::uint32_t>(k[2])<<16)|
       (static_cast<std::uint32_t>(k[3])<<24);
}
struct Less final:m::Comparator {
    bool less(const m::Payload& a,const m::Payload& b) const override {
        return value(a)<value(b);
    }
};
struct Memory final:growth::IMemoryBackend {
    std::vector<std::size_t> bytes;
    bool request(growth::MemoryAction,std::size_t count) override {
        bytes.push_back(count);
        return true;
    }
};
struct FixupSpy final:d::DeleteBalancingBackend {
    std::vector<std::uint16_t> replacements;
    std::vector<std::uint16_t> roots;
    void balance_after_erase(m::Map& tree,std::uint16_t replacement) override {
        replacements.push_back(replacement);
        roots.push_back(tree.root);
    }
};
static void configure_node(m::Map& tree,std::uint16_t id,
                           std::uint32_t v,std::uint16_t parent,
                           std::uint16_t left,std::uint16_t right,
                           std::uint16_t color) {
    auto& node=m::checked(tree,id);
    node.left=left;
    node.right=right;
    node.payload=key(v);
    node.opaque_header=static_cast<std::uint32_t>(parent)|
        (static_cast<std::uint32_t>(color)<<16);
}

static void test_original_native_layout_and_opaque_boundary() {
    static_assert(d::kNativeEraseVA==0x241083C0u);
    static_assert(d::kOpaqueNativeDeleteFixupVA==0x24107750u);
    static_assert(d::kOriginalNodeResolverVA==0x24105F40u);
    static_assert(d::kOriginalEntireMethodBytes==693u);
    static_assert(d::kNodeStride==16u);
    static_assert(d::kLeftWORD==0u);
    static_assert(d::kRightWORD==2u);
    static_assert(d::kParentWORD==4u);
    static_assert(d::kColorWORD==6u);
    static_assert(d::kRootWORD==0x10u);
}
static void test_nil_noop_and_single_black_root_removal() {
    m::Map tree(1u);
    tree.root=0u;
    tree.live_count=1u;
    configure_node(tree,0u,42u,rb::kNil,rb::kNil,rb::kNil,rb::kBlack);
    FixupSpy spy;
    d::NativeTreeErase deletion(spy);
    assert(!m::release_variant_a(tree,rb::kNil,deletion));
    assert(deletion.erase_calls==0u && tree.live_count==1u);
    assert(m::release_variant_a(tree,0u,deletion));
    assert(tree.root==rb::kNil && tree.live_count==0u);
    assert(tree.free_head==0u && deletion.erase_calls==1u);
    assert(deletion.rebalance_calls==0u && spy.replacements.empty());
    assert(tree.nodes[0].left==0u && tree.nodes[0].right==rb::kNil);
    assert(tree.nodes[0].payload==key(42u));
}
static void test_black_single_child_calls_opaque_rebalance() {
    m::Map tree(2u);
    tree.root=0u;
    tree.live_count=2u;
    configure_node(tree,0u,20u,rb::kNil,1u,rb::kNil,rb::kBlack);
    configure_node(tree,1u,10u,0u,rb::kNil,rb::kNil,rb::kRed);
    FixupSpy spy;
    d::NativeTreeErase deletion(spy);
    assert(m::release_variant_b(tree,0u,deletion));
    assert(tree.root==1u && rb::parent(tree,1u)==rb::kNil);
    assert(tree.live_count==1u && tree.free_head==0u);
    assert(deletion.erase_calls==1u && deletion.rebalance_calls==1u);
    assert((spy.replacements==std::vector<std::uint16_t>{1u}));
    assert((spy.roots==std::vector<std::uint16_t>{1u}));
    // Host balancing is intentionally injected, so color may remain red
    // until the still-opaque original 0x24107750 is reconstructed.
    assert(rb::color(tree,1u)==rb::kRed);
}
static void test_direct_right_successor_transplants_without_self_cycle() {
    m::Map tree(3u);
    tree.root=0u;
    tree.live_count=3u;
    configure_node(tree,0u,20u,rb::kNil,1u,2u,rb::kBlack);
    configure_node(tree,1u,10u,0u,rb::kNil,rb::kNil,rb::kBlack);
    configure_node(tree,2u,30u,0u,rb::kNil,rb::kNil,rb::kBlack);
    FixupSpy spy;
    d::NativeTreeErase deletion(spy);
    assert(m::release_variant_b(tree,0u,deletion));
    assert(tree.root==2u && rb::parent(tree,2u)==rb::kNil);
    assert(tree.nodes[2].left==1u && tree.nodes[2].right==rb::kNil);
    assert(rb::parent(tree,1u)==2u && rb::color(tree,2u)==rb::kBlack);
    assert(tree.nodes[2].payload==key(30u));
    assert(tree.nodes[0].payload==key(20u));
    assert(tree.nodes[0].left==0u && tree.free_head==0u);
    assert(tree.live_count==2u && spy.replacements.empty());
    Less cmp;
    assert(m::lookup(tree,key(10u),cmp)==1u);
    assert(m::lookup(tree,key(30u),cmp)==2u);
    assert(m::lookup(tree,key(20u),cmp)==rb::kNil);
}
static void test_deep_successor_with_child_and_black_fixup_gate() {
    for(const auto removed_color:std::vector<std::uint16_t>{
        rb::kRed,rb::kBlack
    }) {
        m::Map tree(5u);
        tree.root=0u;
        tree.live_count=5u;
        configure_node(tree,0u,40u,rb::kNil,1u,2u,rb::kBlack);
        configure_node(tree,1u,20u,0u,rb::kNil,rb::kNil,rb::kBlack);
        configure_node(tree,2u,70u,0u,3u,rb::kNil,rb::kBlack);
        configure_node(tree,3u,50u,2u,rb::kNil,4u,removed_color);
        configure_node(tree,4u,60u,3u,rb::kNil,rb::kNil,rb::kRed);
        FixupSpy spy;
        d::NativeTreeErase deletion(spy);
        assert(m::release_variant_a(tree,0u,deletion));
        assert(tree.root==3u && rb::parent(tree,3u)==rb::kNil);
        assert(tree.nodes[3].left==1u && tree.nodes[3].right==2u);
        assert(rb::parent(tree,1u)==3u && rb::parent(tree,2u)==3u);
        assert(tree.nodes[2].left==4u && rb::parent(tree,4u)==2u);
        assert(rb::color(tree,3u)==rb::kBlack);
        assert(tree.nodes[3].payload==key(50u));
        assert(tree.nodes[0].payload==key(40u));
        assert(tree.live_count==4u && tree.free_head==0u);
        assert(deletion.rebalance_calls==(
            removed_color==rb::kBlack?1u:0u));
        if(removed_color==rb::kBlack)
            assert((spy.replacements==std::vector<std::uint16_t>{4u}));
        else assert(spy.replacements.empty());
        Less cmp;
        for(const auto v:std::vector<std::uint32_t>{20u,50u,60u,70u})
            assert(m::lookup(tree,key(v),cmp)!=rb::kNil);
        assert(m::lookup(tree,key(40u),cmp)==rb::kNil);
    }
}
static void test_integrated_red_leaf_release_recycle_and_balanced_insert() {
    m::Map tree(0u);
    Memory memory;
    nodes::ResolverState resolver;
    nodes::NativeIndexCallbacks allocator(tree,memory,resolver);
    ix::IndexState indices;
    Less cmp;
    rb::NativeRedBlackFixup native_insert_fixup;
    attach::NativeTreeAttach native_attachment(native_insert_fixup);
    ix::InsertBridge bridge(indices,allocator,cmp,native_attachment);
    assert(m::insert(tree,key(20u),bridge)==0u);
    assert(m::insert(tree,key(10u),bridge)==1u);
    assert(m::insert(tree,key(30u),bridge)==2u);
    assert(tree.root==0u && rb::color(tree,0u)==rb::kBlack);
    assert(rb::color(tree,1u)==rb::kRed);
    assert(rb::color(tree,2u)==rb::kRed);
    FixupSpy spy;
    d::NativeTreeErase native_delete(spy);
    assert(m::release_variant_b(tree,2u,native_delete));
    assert(tree.live_count==2u && tree.free_head==2u);
    assert(native_delete.rebalance_calls==0u);
    assert(m::lookup(tree,key(30u),cmp)==rb::kNil);
    assert(m::insert(tree,key(25u),bridge)==2u);
    assert(tree.root==0u && tree.nodes[0].right==2u);
    assert(tree.free_head==rb::kNil && tree.live_count==3u);
    assert(m::lookup(tree,key(25u),cmp)==2u);
    assert(native_insert_fixup.callbacks==4u);
    assert(allocator.recycle_resolutions==1u);
    assert(indices.next_unused==3u);
    assert((memory.bytes==std::vector<std::size_t>{32u,64u}));
}
static void test_missing_node_and_corrupt_parent_fail_closed() {
    m::Map empty(2u);
    FixupSpy spy;
    d::NativeTreeErase deletion(spy);
    bool rejected=false;
    try{deletion.erase_tree_node(empty,0u);}
    catch(const std::logic_error&){rejected=true;}
    assert(rejected);
    rejected=false;
    try{deletion.erase_tree_node(empty,3u);}
    catch(const std::out_of_range&){rejected=true;}
    assert(rejected);
    m::Map bad(2u);
    bad.root=0u;
    configure_node(bad,0u,10u,rb::kNil,1u,rb::kNil,rb::kBlack);
    configure_node(bad,1u,5u,rb::kNil,rb::kNil,rb::kNil,rb::kRed);
    rejected=false;
    try{deletion.erase_tree_node(bad,1u);}
    catch(const std::logic_error&){rejected=true;}
    assert(rejected && bad.root==0u);
    assert(spy.replacements.empty());
}
int main() {
    test_original_native_layout_and_opaque_boundary();
    test_nil_noop_and_single_black_root_removal();
    test_black_single_child_calls_opaque_rebalance();
    test_direct_right_successor_transplants_without_self_cycle();
    test_deep_successor_with_child_and_black_fixup_gate();
    test_integrated_red_leaf_release_recycle_and_balanced_insert();
    test_missing_node_and_corrupt_parent_fail_closed();
    return 0;
}
