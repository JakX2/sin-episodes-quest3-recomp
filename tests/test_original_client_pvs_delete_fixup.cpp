#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_client_pvs_delete_fixup.hpp"
#include "sin_compat/original_client_pvs_map_node_growth.hpp"

namespace fix=sin_compat::original_client_pvs_delete_fixup;
namespace m=fix::map;
namespace rb=fix::rb;
namespace er=fix::erase;
namespace ix=sin_compat::original_client_pvs_map_search_index;
namespace at=sin_compat::original_client_pvs_tree_attach;
namespace growth=sin_compat::original_client_pvs_storage_growth;
namespace node=sin_compat::original_client_pvs_map_node_growth;

static m::Payload key(std::uint32_t n) {
    return m::Payload{{static_cast<std::uint8_t>(n),
      static_cast<std::uint8_t>(n>>8),
      static_cast<std::uint8_t>(n>>16),
      static_cast<std::uint8_t>(n>>24),0,0,0,0}};
}
static std::uint32_t value(const m::Payload& p) {
    return static_cast<std::uint32_t>(p[0])|
      (static_cast<std::uint32_t>(p[1])<<8)|
      (static_cast<std::uint32_t>(p[2])<<16)|
      (static_cast<std::uint32_t>(p[3])<<24);
}
static void setup(m::Map& tree,std::uint16_t id,std::uint32_t v,
                  std::uint16_t p,std::uint16_t l,std::uint16_t r,
                  std::uint16_t color) {
    auto& n=m::checked(tree,id);
    n.left=l; n.right=r; n.payload=key(v);
    n.opaque_header=static_cast<std::uint32_t>(p)|
        (static_cast<std::uint32_t>(color)<<16);
}
struct Less final:m::Comparator {
    bool less(const m::Payload& a,const m::Payload& b) const override {
        return value(a)<value(b);
    }
};
struct Memory final:growth::IMemoryBackend {
    std::vector<std::size_t> requests{};
    bool request(growth::MemoryAction,std::size_t bytes) override {
        requests.push_back(bytes); return true;
    }
};
static void test_original_evidence_constants() {
    static_assert(fix::kOriginalFixupVA==0x24107750u);
    static_assert(fix::kOriginalWholeNativeCodeBytes==2609u);
    static_assert(fix::kNativeRotateLeftVA==0x241067C0u);
    static_assert(fix::kNativeRotateRightVA==0x24106D40u);
    static_assert(fix::kNodeStride==16u);
    static_assert(fix::kRootWORD==0x10u);
    static_assert(fix::kParentWORD==4u);
    static_assert(fix::kColorWORD==6u);
    static_assert(fix::kRed==0u && fix::kBlack==1u);
    static_assert(fix::kNil==0xFFFFu);
    static_assert(sizeof(m::Record)==16u);
}

static void test_root_and_red_replacement_are_blackened() {
    m::Map tree(2u);
    tree.root=0u;
    setup(tree,0u,20u,fix::kNil,1u,fix::kNil,fix::kBlack);
    setup(tree,1u,10u,0u,fix::kNil,fix::kNil,fix::kRed);
    fix::NativeDeleteFixup balance;
    balance.balance_after_erase(tree,1u);
    assert(rb::color(tree,1u)==fix::kBlack);
    assert(balance.callbacks==1u && balance.left_rotations==0u);
    // Native method exits immediately if replacement is the root.
    tree.root=1u;
    rb::set_parent(tree,1u,fix::kNil);
    rb::set_color(tree,1u,fix::kRed);
    balance.balance_after_erase(tree,1u);
    assert(rb::color(tree,1u)==fix::kBlack);
    assert(balance.callbacks==2u);
}
static void test_black_sibling_black_children_moves_up() {
    m::Map tree(3u);
    tree.root=0u;
    setup(tree,0u,20u,fix::kNil,1u,2u,fix::kBlack);
    setup(tree,1u,10u,0u,fix::kNil,fix::kNil,fix::kBlack);
    setup(tree,2u,30u,0u,fix::kNil,fix::kNil,fix::kBlack);
    fix::NativeDeleteFixup b;
    b.balance_after_erase(tree,1u);
    assert(tree.root==0u);
    assert(rb::color(tree,2u)==fix::kRed);
    assert(rb::color(tree,0u)==fix::kBlack);
    assert(b.black_nephews_cases==1u);
    assert(b.red_sibling_cases==0u);
    assert(b.left_rotations==0u && b.right_rotations==0u);
}

static void test_left_branch_outer_red_nephew() {
    m::Map t(4u);t.root=0u;
    setup(t,0u,20u,fix::kNil,1u,2u,fix::kBlack);
    setup(t,1u,10u,0u,fix::kNil,fix::kNil,fix::kBlack);
    setup(t,2u,30u,0u,fix::kNil,3u,fix::kBlack);
    setup(t,3u,40u,2u,fix::kNil,fix::kNil,fix::kRed);
    fix::NativeDeleteFixup b;
    b.balance_after_erase(t,1u);
    assert(t.root==2u && rb::parent(t,2u)==fix::kNil);
    assert(t.nodes[2].left==0u && rb::parent(t,0u)==2u);
    assert(t.nodes[2].right==3u && rb::parent(t,3u)==2u);
    assert(rb::color(t,3u)==fix::kBlack);
    assert(b.left_rotations==1u && b.right_rotations==0u);
    assert(b.outer_nephew_cases==1u && b.inner_nephew_cases==0u);
}
static void test_left_branch_inner_red_nephew() {
    m::Map t(4u);t.root=0u;
    setup(t,0u,20u,fix::kNil,1u,2u,fix::kBlack);
    setup(t,1u,10u,0u,fix::kNil,fix::kNil,fix::kBlack);
    setup(t,2u,30u,0u,3u,fix::kNil,fix::kBlack);
    setup(t,3u,25u,2u,fix::kNil,fix::kNil,fix::kRed);
    fix::NativeDeleteFixup b;
    b.balance_after_erase(t,1u);
    assert(t.root==3u && rb::parent(t,3u)==fix::kNil);
    assert(t.nodes[3].left==0u && t.nodes[3].right==2u);
    assert(rb::parent(t,0u)==3u && rb::parent(t,2u)==3u);
    assert(rb::color(t,3u)==fix::kBlack);
    assert(b.inner_nephew_cases==1u && b.outer_nephew_cases==1u);
    assert(b.left_rotations==1u && b.right_rotations==1u);
}
static void test_left_red_sibling_rotation_then_recolor() {
    m::Map t(5u);t.root=0u;
    setup(t,0u,20u,fix::kNil,1u,2u,fix::kBlack);
    setup(t,1u,10u,0u,fix::kNil,fix::kNil,fix::kBlack);
    setup(t,2u,30u,0u,3u,4u,fix::kRed);
    setup(t,3u,25u,2u,fix::kNil,fix::kNil,fix::kBlack);
    setup(t,4u,40u,2u,fix::kNil,fix::kNil,fix::kBlack);
    fix::NativeDeleteFixup b;
    b.balance_after_erase(t,1u);
    assert(t.root==2u && rb::parent(t,2u)==fix::kNil);
    assert(t.nodes[2].left==0u && rb::parent(t,0u)==2u);
    assert(t.nodes[0].right==3u && rb::parent(t,3u)==0u);
    assert(rb::color(t,2u)==fix::kBlack);
    assert(rb::color(t,3u)==fix::kRed);
    assert(b.red_sibling_cases==1u && b.black_nephews_cases==1u);
    assert(b.left_rotations==1u);
}
static void test_mirrored_right_branch_rotations() {
    { // outer LEFT nephew of the left sibling
        m::Map t(4u);t.root=0u;
        setup(t,0u,20u,fix::kNil,2u,1u,fix::kBlack);
        setup(t,1u,30u,0u,fix::kNil,fix::kNil,fix::kBlack);
        setup(t,2u,10u,0u,3u,fix::kNil,fix::kBlack);
        setup(t,3u,5u,2u,fix::kNil,fix::kNil,fix::kRed);
        fix::NativeDeleteFixup b;
        b.balance_after_erase(t,1u);
        assert(t.root==2u && t.nodes[2].right==0u);
        assert(rb::parent(t,0u)==2u && rb::color(t,3u)==fix::kBlack);
        assert(b.right_rotations==1u && b.left_rotations==0u);
        assert(b.outer_nephew_cases==1u);
    }
    { // inner RIGHT nephew of the left sibling
        m::Map t(4u);t.root=0u;
        setup(t,0u,20u,fix::kNil,2u,1u,fix::kBlack);
        setup(t,1u,30u,0u,fix::kNil,fix::kNil,fix::kBlack);
        setup(t,2u,10u,0u,fix::kNil,3u,fix::kBlack);
        setup(t,3u,15u,2u,fix::kNil,fix::kNil,fix::kRed);
        fix::NativeDeleteFixup b;
        b.balance_after_erase(t,1u);
        assert(t.root==3u && t.nodes[3].left==2u && t.nodes[3].right==0u);
        assert(rb::parent(t,2u)==3u && rb::parent(t,0u)==3u);
        assert(b.left_rotations==1u && b.right_rotations==1u);
        assert(b.inner_nephew_cases==1u && b.outer_nephew_cases==1u);
    }
    { // red LEFT sibling, black children, symmetric initial rotation
        m::Map t(5u);t.root=0u;
        setup(t,0u,20u,fix::kNil,2u,1u,fix::kBlack);
        setup(t,1u,30u,0u,fix::kNil,fix::kNil,fix::kBlack);
        setup(t,2u,10u,0u,3u,4u,fix::kRed);
        setup(t,3u,5u,2u,fix::kNil,fix::kNil,fix::kBlack);
        setup(t,4u,15u,2u,fix::kNil,fix::kNil,fix::kBlack);
        fix::NativeDeleteFixup b;
        b.balance_after_erase(t,1u);
        assert(t.root==2u && t.nodes[2].right==0u);
        assert(rb::parent(t,0u)==2u && rb::color(t,2u)==fix::kBlack);
        assert(rb::color(t,4u)==fix::kRed);
        assert(b.right_rotations==1u && b.red_sibling_cases==1u);
        assert(b.black_nephews_cases==1u);
    }
}
static void test_phase241_black_removed_with_red_child() {
    m::Map t(2u);t.root=0u;t.live_count=2u;
    setup(t,0u,20u,fix::kNil,1u,fix::kNil,fix::kBlack);
    setup(t,1u,10u,0u,fix::kNil,fix::kNil,fix::kRed);
    fix::NativeDeleteFixup balance;
    er::NativeTreeErase eraser(balance);
    assert(m::release_variant_b(t,0u,eraser));
    assert(t.root==1u && rb::parent(t,1u)==fix::kNil);
    assert(rb::color(t,1u)==fix::kBlack);
    assert(t.free_head==0u && t.live_count==1u);
    assert(balance.callbacks==1u && eraser.rebalance_calls==1u);
    Less cmp;
    assert(m::lookup(t,key(10u),cmp)==1u);
    assert(m::lookup(t,key(20u),cmp)==fix::kNil);
}
static void test_cross_phase_grow_insert_balance_erase_recycle() {
    m::Map t(0u);Memory memory;node::ResolverState resolver;
    node::NativeIndexCallbacks allocator(t,memory,resolver);
    ix::IndexState state;Less cmp;rb::NativeRedBlackFixup insert_balance;
    at::NativeTreeAttach att(insert_balance);
    ix::InsertBridge bridge(state,allocator,cmp,att);
    assert(m::insert(t,key(20u),bridge)==0u);
    assert(m::insert(t,key(10u),bridge)==1u);
    assert(m::insert(t,key(30u),bridge)==2u);
    assert(t.root==0u && rb::color(t,0u)==fix::kBlack);
    fix::NativeDeleteFixup delete_balance;
    er::NativeTreeErase erase(delete_balance);
    assert(m::release_variant_b(t,2u,erase)); // red leaf, no fixup callback
    assert(delete_balance.callbacks==0u && erase.rebalance_calls==0u);
    assert(m::insert(t,key(25u),bridge)==2u);
    assert(t.free_head==fix::kNil && t.live_count==3u);
    assert(m::lookup(t,key(25u),cmp)==2u);
    assert(insert_balance.callbacks==4u);
    assert(allocator.recycle_resolutions==1u);
    assert((memory.requests==std::vector<std::size_t>{32u,64u}));
}
static void test_invalid_input_safeguards() {
    m::Map tree(2u);fix::NativeDeleteFixup b;
    bool failed=false;
    try{b.balance_after_erase(tree,fix::kNil);}
    catch(const std::out_of_range&){failed=true;}
    assert(failed);
    failed=false;
    try{b.balance_after_erase(tree,5u);}
    catch(const std::out_of_range&){failed=true;}
    assert(failed);
    failed=false;
    try{b.balance_after_erase(tree,0u);}
    catch(const std::logic_error&){failed=true;}
    assert(failed);
    tree.root=0u;
    setup(tree,0u,20u,fix::kNil,fix::kNil,fix::kNil,fix::kBlack);
    setup(tree,1u,10u,0u,fix::kNil,fix::kNil,fix::kBlack);
    failed=false;
    try{b.balance_after_erase(tree,1u);}
    catch(const std::logic_error&){failed=true;}
    assert(failed); // parent does not link child
}
int main() {
    test_original_evidence_constants();
    test_root_and_red_replacement_are_blackened();
    test_black_sibling_black_children_moves_up();
    test_left_branch_outer_red_nephew();
    test_left_branch_inner_red_nephew();
    test_left_red_sibling_rotation_then_recolor();
    test_mirrored_right_branch_rotations();
    test_phase241_black_removed_with_red_child();
    test_cross_phase_grow_insert_balance_erase_recycle();
    test_invalid_input_safeguards();
    return 0;
}
