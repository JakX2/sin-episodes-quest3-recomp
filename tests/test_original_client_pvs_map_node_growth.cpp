#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include "sin_compat/original_client_pvs_map_node_growth.hpp"

namespace n=sin_compat::original_client_pvs_map_node_growth;
namespace m=n::map;
namespace ix=n::index;
namespace growth=n::growth;

static m::Payload make_key(std::uint32_t x) {
    return m::Payload{{
        static_cast<std::uint8_t>(x),
        static_cast<std::uint8_t>(x>>8),
        static_cast<std::uint8_t>(x>>16),
        static_cast<std::uint8_t>(x>>24),0,0,0,0}};
}
static std::uint32_t as_number(const m::Payload& p) {
    return static_cast<std::uint32_t>(p[0])|
           (static_cast<std::uint32_t>(p[1])<<8)|
           (static_cast<std::uint32_t>(p[2])<<16)|
           (static_cast<std::uint32_t>(p[3])<<24);
}
struct Memory final:growth::IMemoryBackend {
    struct Event {growth::MemoryAction action;std::size_t bytes;};
    std::vector<Event> events;
    bool accept=true;
    bool request(growth::MemoryAction action,std::size_t bytes) override {
        events.push_back({action,bytes});
        return accept;
    }
};
struct Less final:m::Comparator {
    mutable std::size_t calls{};
    bool less(const m::Payload& a,const m::Payload& b) const override {
        ++calls;
        return as_number(a)<as_number(b);
    }
};
struct Attach final:ix::TreeAttachCallback {
    std::size_t calls{};
    void attach(m::Map& tree,std::uint16_t id,m::Placement pos) override {
        ++calls;
        auto& node=m::checked(tree,id);
        node.left=m::kAbsent;
        node.right=m::kAbsent;
        if(pos.parent==m::kAbsent)tree.root=id;
        else if(pos.as_left)m::checked(tree,pos.parent).left=id;
        else m::checked(tree,pos.parent).right=id;
    }
};
struct LeafErase final:m::DeletionBackend {
    void erase_tree_node(m::Map& tree,std::uint16_t id) override {
        const auto& target=m::checked(tree,id);
        if(target.left!=m::kAbsent||target.right!=m::kAbsent)
            throw std::logic_error("synthetic tree erase supports leaves only");
        if(tree.root==id){tree.root=m::kAbsent;return;}
        for(auto& record:tree.nodes) {
            if(record.left==id){record.left=m::kAbsent;return;}
            if(record.right==id){record.right=m::kAbsent;return;}
        }
        throw std::logic_error("synthetic tree leaf missing");
    }
};
static void test_original_native_layout_and_links() {
    static_assert(n::kNativeResolverVA==0x24105F40u);
    static_assert(n::kNativeGrowthVA==0x24106140u);
    static_assert(n::kOriginalReviewedMethods==2u);
    static_assert(n::kOriginalReviewedCodeBytes==191u);
    static_assert(n::kNativeSentinelVA==0x2433FDBCu);
    static_assert(n::kNativeInitFlagVA==0x2433FDC4u);
    static_assert(n::kNativePointerDwordMapOffset==4u);
    static_assert(n::kNativeCapacityDwordMapOffset==8u);
    static_assert(n::kNativeGrowPolicyDwordMapOffset==12u);
    static_assert(n::kNativeRecordStride==16u);
    static_assert(n::kOriginalExistingStorageHelper==0x2416F960u);
    static_assert(n::kOriginalNullStorageHelper==0x2416F8F0u);
}
static void test_resolver_once_and_valid_node_vs_global_sentinel() {
    m::Map map(3u);
    map.nodes[1].left=7u;
    map.nodes[1].right=8u;
    map.nodes[1].opaque_header=0xABCD1234u;
    map.nodes[1].payload=make_key(99u);
    n::ResolverState state;
    const auto real=n::resolve(map,1u,state);
    assert(state.initialized && state.initializations==1u);
    assert(!real.is_sentinel && real.record==&map.nodes[1]);
    assert((real.header==std::array<std::uint16_t,4u>{{7u,8u,0x1234u,0xABCDu}}));
    assert(real.free_next()==8u);
    const auto nil=n::resolve(map,m::kAbsent,state);
    assert(nil.is_sentinel && nil.record==nullptr);
    assert((nil.header==std::array<std::uint16_t,4u>{{
        m::kAbsent,m::kAbsent,m::kAbsent,1u}}));
    assert(state.initializations==1u);
    const auto nil_again=n::resolve(map,m::kAbsent,state);
    assert(nil_again.header==nil.header && state.initializations==1u);
    bool failed=false;
    try{(void)n::resolve(map,3u,state);}
    catch(const std::out_of_range&){failed=true;}
    assert(failed && state.initializations==1u);
}
static void test_native_map_growth_initial_reallocate_and_call_through() {
    m::Map map(0u);
    Memory memory;
    n::ResolverState resolver;
    n::NativeIndexCallbacks backend(map,memory,resolver);
    assert(backend.grow_records(map,1)==growth::MemoryAction::allocate);
    assert(map.nodes.size()==2u);
    assert(backend.storage.capacity==2 && memory.events.size()==1u);
    assert(memory.events[0].action==growth::MemoryAction::allocate);
    assert(memory.events[0].bytes==32u);
    map.nodes[0].payload=make_key(0xBEEFu);
    assert(backend.grow_records(map,0)==growth::MemoryAction::reallocate);
    assert(memory.events.size()==2u && memory.events[1].bytes==32u);
    assert(map.nodes[0].payload==make_key(0xBEEFu));
    assert(backend.grow_records(map,1)==growth::MemoryAction::reallocate);
    assert(map.nodes.size()==4u && backend.storage.capacity==4);
    assert(memory.events.size()==3u);
    assert(memory.events[2].action==growth::MemoryAction::reallocate);
    assert(memory.events[2].bytes==64u);
    assert(map.nodes[0].payload==make_key(0xBEEFu));
}
static void test_original_fixed_increment_and_fixed_capacity() {
    m::Map map(0u);
    Memory memory;
    n::ResolverState resolver;
    n::NativeIndexCallbacks custom(map,memory,resolver,3);
    assert(custom.grow_records(map,4)==growth::MemoryAction::allocate);
    assert(map.nodes.size()==5u && memory.events.back().bytes==80u);
    assert(custom.grow_records(map,1)==growth::MemoryAction::reallocate);
    assert(map.nodes.size()==8u && memory.events.back().bytes==128u);
    m::Map locked(0u);
    Memory unused;
    n::NativeIndexCallbacks disabled(locked,unused,resolver,-1);
    assert(disabled.grow_records(locked,1)==growth::MemoryAction::unchanged);
    assert(locked.nodes.empty() && unused.events.empty());
    ix::IndexState index;
    bool failed=false;
    try{(void)ix::allocate_index(locked,index,disabled);}
    catch(const std::out_of_range&){failed=true;}
    assert(failed && index.next_unused==0u && unused.events.empty());
}
static void test_integrated_insert_erase_recycle_and_growth() {
    m::Map tree(0u);
    Memory memory;
    n::ResolverState resolver;
    n::NativeIndexCallbacks backend(tree,memory,resolver);
    ix::IndexState state;
    Less comparator;
    Attach attach;
    ix::InsertBridge bridge(state,backend,comparator,attach);
    assert(m::insert(tree,make_key(20u),bridge)==0u);
    assert(tree.nodes.size()==2u);
    assert(m::insert(tree,make_key(10u),bridge)==1u);
    assert(m::insert(tree,make_key(30u),bridge)==2u);
    assert(tree.nodes.size()==4u);
    assert(tree.root==0u && tree.nodes[0].left==1u);
    assert(tree.nodes[0].right==2u);
    assert(tree.live_count==3u && attach.calls==3u);
    assert(memory.events.size()==2u &&
           memory.events[0].bytes==32u && memory.events[1].bytes==64u);
    assert(m::lookup(tree,make_key(30u),comparator)==2u);
    LeafErase erase;
    assert(m::release_variant_b(tree,2u,erase));
    assert(tree.free_head==2u && tree.nodes[2].right==m::kAbsent);
    assert(m::insert(tree,make_key(25u),bridge)==2u);
    assert(tree.nodes[0].right==2u);
    assert(tree.free_head==m::kAbsent && tree.live_count==3u);
    assert(m::lookup(tree,make_key(25u),comparator)==2u);
    assert(backend.recycle_resolutions==1u && resolver.initializations==1u);
    assert(memory.events.size()==2u);
    assert(state.next_unused==3u && state.cached_base_refreshes==4u);
}
static void test_host_guard_failed_memory_and_capacity_domain() {
    Memory memory;
    memory.accept=false;
    m::Map tree(0u);
    n::ResolverState resolver;
    n::NativeIndexCallbacks callback(tree,memory,resolver);
    bool failed=false;
    try{(void)callback.grow_records(tree,1);}
    catch(const std::runtime_error&){failed=true;}
    assert(failed && tree.nodes.empty() && callback.storage.capacity==0);
    assert(memory.events.size()==1u && memory.events[0].bytes==32u);
    m::Map large(40000u);
    Memory large_memory;
    n::NativeIndexCallbacks large_callback(large,large_memory,resolver);
    failed=false;
    try{(void)large_callback.grow_records(large,1);}
    catch(const std::out_of_range&){failed=true;}
    assert(failed && large.nodes.size()==40000u &&
           large_memory.events.empty());
    m::Map unusual(0u);
    n::NativeIndexCallbacks bad_policy(unusual,large_memory,resolver,-2);
    failed=false;
    try{(void)bad_policy.grow_records(unusual,1);}
    catch(const std::out_of_range&){failed=true;}
    assert(failed && large_memory.events.empty());
}
int main() {
    test_original_native_layout_and_links();
    test_resolver_once_and_valid_node_vs_global_sentinel();
    test_native_map_growth_initial_reallocate_and_call_through();
    test_original_fixed_increment_and_fixed_capacity();
    test_integrated_insert_erase_recycle_and_growth();
    test_host_guard_failed_memory_and_capacity_domain();
    return 0;
}
