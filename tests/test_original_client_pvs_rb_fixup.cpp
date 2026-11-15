#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

#include "sin_compat/original_client_pvs_rb_fixup.hpp"
#include "sin_compat/original_client_pvs_map_node_growth.hpp"

namespace rb=sin_compat::original_client_pvs_rb_fixup;
namespace map=rb::map;
namespace attach=rb::attach;
namespace index=sin_compat::original_client_pvs_map_search_index;
namespace nodes=sin_compat::original_client_pvs_map_node_growth;
namespace growth=sin_compat::original_client_pvs_storage_growth;

static map::Payload key(std::uint32_t n) {
    return map::Payload{{
        static_cast<std::uint8_t>(n),
        static_cast<std::uint8_t>(n>>8),
        static_cast<std::uint8_t>(n>>16),
        static_cast<std::uint8_t>(n>>24),0,0,0,0}};
}
static std::uint32_t integer(const map::Payload& k) {
    return static_cast<std::uint32_t>(k[0]) |
        (static_cast<std::uint32_t>(k[1])<<8) |
        (static_cast<std::uint32_t>(k[2])<<16) |
        (static_cast<std::uint32_t>(k[3])<<24);
}
struct Less final:map::Comparator {
    bool less(const map::Payload& a,const map::Payload& b) const override {
        return integer(a)<integer(b);
    }
};
struct Memory final:growth::IMemoryBackend {
    std::vector<std::size_t> requested_bytes;
    bool request(growth::MemoryAction,std::size_t bytes) override {
        requested_bytes.push_back(bytes);
        return true;
    }
};

static void test_original_native_metadata() {
    static_assert(rb::kOriginalRotateLeftVA==0x241067C0u);
    static_assert(rb::kOriginalRotateRightVA==0x24106D40u);
    static_assert(rb::kOriginalInsertFixupVA==0x24106E30u);
    static_assert(rb::kOriginalReviewedMethods==3u);
    static_assert(rb::kOriginalNativeBytes==1947u);
    static_assert(rb::kNodeStride==16u);
    static_assert(rb::kRootOffset==0x10u);
    static_assert(rb::kParentOffset==4u);
    static_assert(rb::kColorOffset==6u);
    static_assert(rb::kNil==0xFFFFu);
    static_assert(rb::kRed==0u);
    static_assert(rb::kBlack==1u);
    static_assert(sizeof(map::Record)==16u);
}

static void test_left_right_rotations_preserve_subtrees_and_payloads() {
    map::Map tree(5u);
    tree.root=0u;
    tree.nodes[0].right=1u;
    tree.nodes[0].payload=key(10u);
    tree.nodes[1].payload=key(20u);
    tree.nodes[1].left=2u;
    tree.nodes[2].payload=key(15u);
    rb::set_parent(tree,0u,rb::kNil);
    rb::set_parent(tree,1u,0u);
    rb::set_parent(tree,2u,1u);
    rb::set_color(tree,0u,rb::kBlack);
    rb::set_color(tree,1u,rb::kRed);
    rb::set_color(tree,2u,rb::kBlack);

    rb::rotate_left(tree,0u);
    assert(tree.root==1u && rb::parent(tree,1u)==rb::kNil);
    assert(tree.nodes[1].left==0u && rb::parent(tree,0u)==1u);
    assert(tree.nodes[0].right==2u && rb::parent(tree,2u)==0u);
    assert(tree.nodes[1].right==rb::kNil);
    assert(rb::color(tree,1u)==rb::kRed);
    assert(rb::color(tree,0u)==rb::kBlack);
    assert(integer(tree.nodes[0].payload)==10u);
    assert(integer(tree.nodes[1].payload)==20u);
    assert(integer(tree.nodes[2].payload)==15u);

    rb::rotate_right(tree,1u);
    assert(tree.root==0u && rb::parent(tree,0u)==rb::kNil);
    assert(tree.nodes[0].right==1u && rb::parent(tree,1u)==0u);
    assert(tree.nodes[1].left==2u && rb::parent(tree,2u)==1u);
    assert(integer(tree.nodes[1].payload)==20u);
}

static void test_rotations_on_nonroot_children() {
    map::Map tree(4u);
    tree.root=0u;
    tree.nodes[0].left=1u;
    tree.nodes[1].right=2u;
    tree.nodes[1].payload=key(5u);
    tree.nodes[2].payload=key(7u);
    rb::set_parent(tree,0u,rb::kNil);
    rb::set_parent(tree,1u,0u);
    rb::set_parent(tree,2u,1u);
    rb::rotate_left(tree,1u);
    assert(tree.root==0u && tree.nodes[0].left==2u);
    assert(rb::parent(tree,2u)==0u);
    assert(tree.nodes[2].left==1u && rb::parent(tree,1u)==2u);
    rb::rotate_right(tree,2u);
    assert(tree.nodes[0].left==1u && tree.nodes[1].right==2u);
    assert(rb::parent(tree,1u)==0u && rb::parent(tree,2u)==1u);
}

static int validate_recursive(const map::Map& tree,std::uint16_t id,
                              std::uint16_t parent,
                              std::uint32_t lower,std::uint32_t upper,
                              std::vector<bool>& visited,
                              std::vector<std::uint32_t>& inorder) {
    if(id==rb::kNil)return 1;  // nil sentinel is black
    assert(id<tree.nodes.size() && !visited[id]);
    visited[id]=true;
    const auto& record=tree.nodes[id];
    assert(rb::parent(tree,id)==parent);
    const auto value=integer(record.payload);
    assert(value>lower && value<upper);
    assert(rb::color(tree,id)==rb::kRed||rb::color(tree,id)==rb::kBlack);
    if(rb::color(tree,id)==rb::kRed) {
        assert(rb::color(tree,record.left)==rb::kBlack);
        assert(rb::color(tree,record.right)==rb::kBlack);
    }
    const auto left=validate_recursive(
        tree,record.left,id,lower,value,visited,inorder);
    inorder.push_back(value);
    const auto right=validate_recursive(
        tree,record.right,id,value,upper,visited,inorder);
    assert(left==right);
    return left+(rb::color(tree,id)==rb::kBlack?1:0);
}
static void validate_tree(const map::Map& tree,std::size_t expected) {
    assert(tree.live_count==expected);
    if(expected==0u) {
        assert(tree.root==rb::kNil);
        return;
    }
    assert(tree.root!=rb::kNil);
    assert(rb::parent(tree,tree.root)==rb::kNil);
    assert(rb::color(tree,tree.root)==rb::kBlack);
    std::vector<bool> visited(tree.nodes.size(),false);
    std::vector<std::uint32_t> inorder;
    const auto height=validate_recursive(tree,tree.root,rb::kNil,0u,
                          0xFFFFFFFFu,visited,inorder);
    assert(height>=2);
    assert(inorder.size()==expected);
    assert(std::is_sorted(inorder.begin(),inorder.end()));
    assert(std::count(visited.begin(),visited.end(),true)==
           static_cast<std::ptrdiff_t>(expected));
}

struct Fixture {
    map::Map tree{0u};
    Memory memory{};
    nodes::ResolverState resolver{};
    nodes::NativeIndexCallbacks allocator{tree,memory,resolver};
    index::IndexState ids{};
    Less compare{};
    rb::NativeRedBlackFixup fixup{};
    attach::NativeTreeAttach tree_attach{fixup};
    index::InsertBridge bridge{ids,allocator,compare,tree_attach};

    void insert(std::uint32_t k) {
        const auto index=map::insert(tree,key(k),bridge);
        assert(index<tree.nodes.size());
        assert(map::lookup(tree,key(k),compare)!=rb::kNil);
    }
};

static void test_single_and_double_rotation_shapes() {
    for(const auto& sequence:std::vector<std::vector<std::uint32_t>>{
        {10u,20u,30u},
        {30u,20u,10u},
        {30u,10u,20u},
        {10u,30u,20u}
    }) {
        Fixture f;
        for(const auto key_value:sequence)f.insert(key_value);
        validate_tree(f.tree,3u);
        assert(integer(f.tree.nodes[f.tree.root].payload)==20u);
        assert(f.fixup.callbacks==3u);
        if(sequence.front()==10u && sequence[1]==20u)
            assert(f.fixup.left_rotations>=1u);
        if(sequence.front()==30u && sequence[1]==20u)
            assert(f.fixup.right_rotations>=1u);
        if(sequence.front()==30u && sequence[1]==10u)
            assert(f.fixup.left_rotations>=1u&&
                   f.fixup.right_rotations>=1u);
        if(sequence.front()==10u && sequence[1]==30u)
            assert(f.fixup.left_rotations>=1u&&
                   f.fixup.right_rotations>=1u);
    }
}
static void test_recolor_and_randomized_tree_invariants() {
    Fixture ordered;
    for(std::uint32_t i=1;i<=128u;++i) {
        ordered.insert(i);
        validate_tree(ordered.tree,i);
    }
    assert(ordered.fixup.recolor_steps>0u);
    assert(ordered.fixup.left_rotations>0u);
    assert(ordered.fixup.callbacks==128u);
    assert(ordered.tree.nodes.size()==128u);
    assert(ordered.memory.requested_bytes.front()==32u);
    assert(ordered.memory.requested_bytes.back()==2048u);

    std::vector<std::uint32_t> shuffled(192);
    std::iota(shuffled.begin(),shuffled.end(),1u);
    std::mt19937 rng(0x51E2006u);
    std::shuffle(shuffled.begin(),shuffled.end(),rng);
    Fixture random;
    for(std::size_t i=0;i<shuffled.size();++i) {
        random.insert(shuffled[i]);
        validate_tree(random.tree,i+1u);
    }
    assert(random.fixup.recolor_steps>0u);
    assert(random.fixup.left_rotations>0u);
    assert(random.fixup.right_rotations>0u);
    assert(random.fixup.callbacks==shuffled.size());
    assert(random.tree.nodes.size()==256u);
}
static void test_bad_rotations_and_missing_fixup_root_fail_closed() {
    map::Map tree(2u);
    bool failed=false;
    try {rb::rotate_left(tree,rb::kNil);}
    catch(const std::out_of_range&){failed=true;}
    assert(failed);
    failed=false;
    try {rb::rotate_right(tree,0u);}
    catch(const std::out_of_range&){failed=true;}
    assert(failed);
    failed=false;
    try {rb::rotate_left(tree,0u);}
    catch(const std::out_of_range&){failed=true;}
    assert(failed);
    rb::NativeRedBlackFixup balancer;
    failed=false;
    try {balancer.balance_after_attach(tree,0u);}
    catch(const std::logic_error&){failed=true;}
    assert(failed);
    failed=false;
    try {rb::set_color(tree,rb::kNil,rb::kRed);}
    catch(const std::out_of_range&){failed=true;}
    assert(failed);
    tree.root=0u;
    rb::set_parent(tree,0u,rb::kNil);
    rb::set_color(tree,0u,rb::kRed);
    balancer.balance_after_attach(tree,0u);
    assert(rb::color(tree,0u)==rb::kBlack);
}
int main() {
    test_original_native_metadata();
    test_left_right_rotations_preserve_subtrees_and_payloads();
    test_rotations_on_nonroot_children();
    test_single_and_double_rotation_shapes();
    test_recolor_and_randomized_tree_invariants();
    test_bad_rotations_and_missing_fixup_root_fail_closed();
    return 0;
}
