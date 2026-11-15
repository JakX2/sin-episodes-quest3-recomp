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
#include "sin_compat/original_client_pvs_map_search_index.hpp"

namespace n=sin_compat::original_client_pvs_map_search_index;
namespace m=n::map;

static m::Payload payload(std::uint32_t v) {
    return m::Payload{{
        static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v>>8),
        static_cast<std::uint8_t>(v>>16),
        static_cast<std::uint8_t>(v>>24),0,0,0,0}};
}
static std::uint32_t value(const m::Payload& p) {
    return static_cast<std::uint32_t>(p[0])|
           (static_cast<std::uint32_t>(p[1])<<8)|
           (static_cast<std::uint32_t>(p[2])<<16)|
           (static_cast<std::uint32_t>(p[3])<<24);
}
struct Less final:m::Comparator {
    mutable std::vector<std::string> calls;
    bool less(const m::Payload& a,const m::Payload& b) const override {
        calls.push_back(std::to_string(value(a))+":"+
                        std::to_string(value(b)));
        return value(a)<value(b);
    }
};
struct IndexHooks final:n::IndexCallbacks {
    int grows{};
    int resolves{};
    bool grow_enabled{true};
    bool corrupt_free_link{};
    void grow_one(m::Map& manager) override {
        ++grows;
        if(grow_enabled)manager.nodes.resize(manager.nodes.size()+2u);
    }
    std::uint16_t recycled_next(
        m::Map& manager,std::uint16_t index) override {
        ++resolves;
        return corrupt_free_link?static_cast<std::uint16_t>(999u):
                                 m::checked(manager,index).right;
    }
};
struct Attach final:n::TreeAttachCallback {
    int calls{};
    void attach(m::Map& manager,std::uint16_t index,
                m::Placement p) override {
        ++calls;
        auto& node=m::checked(manager,index);
        node.left=m::kAbsent;
        node.right=m::kAbsent;
        if(p.parent==m::kAbsent)
            manager.root=index;
        else if(p.as_left)
            m::checked(manager,p.parent).left=index;
        else
            m::checked(manager,p.parent).right=index;
    }
};
struct LeafDelete final:m::DeletionBackend {
    int calls{};
    void erase_tree_node(m::Map& manager,std::uint16_t index) override {
        ++calls;
        const auto& node=m::checked(manager,index);
        if(node.left!=m::kAbsent||node.right!=m::kAbsent)
            throw std::logic_error("synthetic leaf-only deletion");
        if(manager.root==index){manager.root=m::kAbsent;return;}
        for(auto& rec:manager.nodes) {
            if(rec.left==index){rec.left=m::kAbsent;return;}
            if(rec.right==index){rec.right=m::kAbsent;return;}
        }
        throw std::logic_error("synthetic node not linked");
    }
};

static void test_native_metadata() {
    static_assert(n::kOriginalSearchVA==0x24106440u);
    static_assert(n::kOriginalIndexAllocatorVA==0x24106700u);
    static_assert(n::kOriginalReviewedMethods==2u);
    static_assert(n::kOriginalReviewedNativeBytes==341u);
    static_assert(n::kMapCapacityDword==0x08u);
    static_assert(n::kMapRootWord==0x10u);
    static_assert(n::kMapFreeHeadWord==0x14u);
    static_assert(n::kMapNextUnusedWord==0x16u);
    static_assert(n::kMapCachedPointerDword==0x18u);
    static_assert(n::kNativeRecordStride==16u);
    static_assert(n::kNativeFreeNextWord==2u);
}
static void test_find_placement_empty_left_right_and_equal() {
    m::Map tree(4u);
    Less cmp;
    auto pos=n::find_insertion_placement(tree,payload(20u),cmp);
    assert(pos.parent==m::kAbsent && !pos.as_left && cmp.calls.empty());
    tree.root=0u;
    tree.nodes[0].payload=payload(20u);
    tree.nodes[1].payload=payload(10u);
    tree.nodes[2].payload=payload(30u);
    tree.nodes[0].left=1u;
    tree.nodes[0].right=2u;
    cmp.calls.clear();
    pos=n::find_insertion_placement(tree,payload(5u),cmp);
    assert(pos.parent==1u && pos.as_left);
    assert((cmp.calls==std::vector<std::string>{"5:20","5:10"}));
    cmp.calls.clear();
    pos=n::find_insertion_placement(tree,payload(40u),cmp);
    assert(pos.parent==2u && !pos.as_left);
    assert((cmp.calls==std::vector<std::string>{"40:20","40:30"}));
    cmp.calls.clear();
    pos=n::find_insertion_placement(tree,payload(20u),cmp);
    // Original placement compares once, so equal keys travel right.
    assert(pos.parent==2u && pos.as_left);
    assert((cmp.calls==std::vector<std::string>{"20:20","20:30"}));
}
static void test_allocator_free_head_virgin_and_injected_growth() {
    m::Map tree(2u);
    n::IndexState state;
    IndexHooks hooks;
    assert(n::allocate_index(tree,state,hooks)==0u);
    assert(n::allocate_index(tree,state,hooks)==1u);
    assert(state.next_unused==2u && state.cached_base_refreshes==2u);
    assert(hooks.grows==0 && hooks.resolves==0);
    assert(n::allocate_index(tree,state,hooks)==2u);
    assert(tree.nodes.size()==4u && hooks.grows==1);
    assert(state.next_unused==3u && state.cached_base_refreshes==3u);
    tree.free_head=1u;
    tree.nodes[1].right=m::kAbsent;
    assert(n::allocate_index(tree,state,hooks)==1u);
    assert(tree.free_head==m::kAbsent && hooks.resolves==1);
    assert(state.next_unused==3u && state.cached_base_refreshes==4u);
}
static void test_phase236_insert_bridge_and_reuse() {
    m::Map tree(3u);
    n::IndexState state;
    IndexHooks hooks;
    Less cmp;
    Attach attach;
    n::InsertBridge bridge(state,hooks,cmp,attach);
    assert(m::insert(tree,payload(20u),bridge)==0u);
    assert(m::insert(tree,payload(10u),bridge)==1u);
    assert(m::insert(tree,payload(30u),bridge)==2u);
    assert(tree.root==0u && tree.nodes[0].left==1u);
    assert(tree.nodes[0].right==2u);
    assert(tree.live_count==3u && attach.calls==3);
    assert(state.next_unused==3u && state.cached_base_refreshes==3u);
    assert(hooks.grows==0);
    assert(m::lookup(tree,payload(30u),cmp)==2u);
    LeafDelete del;
    assert(m::release_variant_b(tree,2u,del));
    assert(tree.live_count==2u && tree.free_head==2u);
    assert(tree.nodes[2].left==2u && tree.nodes[2].right==m::kAbsent);
    assert(m::insert(tree,payload(25u),bridge)==2u);
    assert(tree.nodes[0].right==2u);
    assert(tree.nodes[2].payload==payload(25u));
    assert(m::lookup(tree,payload(25u),cmp)==2u);
    assert(tree.live_count==3u && tree.free_head==m::kAbsent);
    assert(state.next_unused==3u && state.cached_base_refreshes==4u);
    assert(hooks.resolves==1 && hooks.grows==0);
}
static void test_host_safety_nonprogressing_growth_and_corrupt_links() {
    m::Map empty(0u);
    n::IndexState state;
    IndexHooks hooks;
    hooks.grow_enabled=false;
    bool failed=false;
    try{(void)n::allocate_index(empty,state,hooks);}
    catch(const std::out_of_range&){failed=true;}
    assert(failed && hooks.grows==1);
    assert(state.next_unused==0u && state.cached_base_refreshes==0u);
    m::Map tree(1u);
    tree.free_head=0u;
    hooks.corrupt_free_link=true;
    failed=false;
    try{(void)n::allocate_index(tree,state,hooks);}
    catch(const std::out_of_range&){failed=true;}
    assert(failed && tree.free_head==0u &&
           state.cached_base_refreshes==0u);
    hooks.corrupt_free_link=false;
    tree.free_head=m::kAbsent;
    state.next_unused=m::kAbsent;
    failed=false;
    try{(void)n::allocate_index(tree,state,hooks);}
    catch(const std::out_of_range&){failed=true;}
    assert(failed);
    Less cmp;
    tree.root=0u;
    tree.nodes[0].payload=payload(20u);
    tree.nodes[0].left=0u;
    failed=false;
    try{(void)n::find_insertion_placement(tree,payload(10u),cmp);}
    catch(const std::runtime_error&){failed=true;}
    assert(failed);
    tree.nodes[0].left=99u;
    failed=false;
    try{(void)n::find_insertion_placement(tree,payload(10u),cmp);}
    catch(const std::out_of_range&){failed=true;}
    assert(failed);
}
int main() {
    test_native_metadata();
    test_find_placement_empty_left_right_and_equal();
    test_allocator_free_head_virgin_and_injected_growth();
    test_phase236_insert_bridge_and_reuse();
    test_host_safety_nonprogressing_growth_and_corrupt_links();
    return 0;
}
