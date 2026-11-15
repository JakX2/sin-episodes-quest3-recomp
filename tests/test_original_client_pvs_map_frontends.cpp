#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include "sin_compat/original_client_pvs_map_frontends.hpp"

namespace n=sin_compat::original_client_pvs_map_frontends;

static n::Payload key(std::uint32_t v) {
    return n::Payload{{static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v>>8),
        static_cast<std::uint8_t>(v>>16),
        static_cast<std::uint8_t>(v>>24),0,0,0,0}};
}
static std::uint32_t number(const n::Payload& p) {
    return static_cast<std::uint32_t>(p[0])|
        (static_cast<std::uint32_t>(p[1])<<8)|
        (static_cast<std::uint32_t>(p[2])<<16)|
        (static_cast<std::uint32_t>(p[3])<<24);
}
struct Less final:n::Comparator {
    mutable std::vector<std::string> comparisons;
    bool less(const n::Payload& a,const n::Payload& b) const override {
        comparisons.push_back(std::to_string(number(a))+":"+
                              std::to_string(number(b)));
        return number(a)<number(b);
    }
};
struct FakeTree final:n::InsertionBackend,n::DeletionBackend {
    std::vector<std::string> events;
    std::uint16_t next_unused{};
    bool force_invalid_index{};
    n::Placement find_placement(n::Map& m,
                                const n::Payload& target) override {
        events.push_back("find");
        n::Placement result{};
        std::uint16_t cursor=m.root;
        while(cursor!=n::kAbsent) {
            result.parent=cursor;
            const auto& rec=n::checked(m,cursor);
            result.as_left=number(target)<number(rec.payload);
            cursor=result.as_left?rec.left:rec.right;
        }
        return result;
    }
    std::uint16_t allocate_index(n::Map& m) override {
        events.push_back("allocate");
        if(force_invalid_index)return n::kAbsent;
        if(m.free_head!=n::kAbsent) {
            auto index=m.free_head;
            m.free_head=n::checked(m,index).right;
            return index;
        }
        if(next_unused>=m.nodes.size())
            throw std::out_of_range("synthetic map allocator exhausted");
        return next_unused++;
    }
    void attach(n::Map& m,std::uint16_t index,
                n::Placement p) override {
        events.push_back("attach");
        auto& rec=n::checked(m,index);
        rec.left=n::kAbsent;
        rec.right=n::kAbsent;
        if(p.parent==n::kAbsent)m.root=index;
        else {
            auto& parent=n::checked(m,p.parent);
            if(p.as_left)parent.left=index;
            else parent.right=index;
        }
    }
    void erase_tree_node(n::Map& m,std::uint16_t index) override {
        events.push_back("erase");
        // Synthetic leaf-only deletion for tests. Original opaque removal
        // at 0x241083C0 is NOT being implemented by this fake callback.
        const auto& target=n::checked(m,index);
        if(target.left!=n::kAbsent||target.right!=n::kAbsent)
            throw std::logic_error("test host only removes leaves");
        if(m.root==index) {m.root=n::kAbsent;return;}
        for(auto& rec:m.nodes) {
            if(rec.left==index){rec.left=n::kAbsent;return;}
            if(rec.right==index){rec.right=n::kAbsent;return;}
        }
        throw std::logic_error("synthetic leaf not attached");
    }
};
static void test_original_layout_and_methods() {
    static_assert(n::kInsertVA==0x24108750u);
    static_assert(n::kLookupVA==0x24106B20u);
    static_assert(n::kReleaseVariantAVA==0x241087C0u);
    static_assert(n::kReleaseVariantBVA==0x24108840u);
    static_assert(n::kNewOriginalMethods==4u);
    static_assert(n::kNewOriginalBytes==473u);
    static_assert(n::kRecordStride==16u);
    static_assert(n::kLeftWordOffset==0u);
    static_assert(n::kRightWordOffset==2u);
    static_assert(n::kPayloadOffset==8u);
    static_assert(n::kManagerStoragePointerDword==4u);
    static_assert(n::kManagerRootWord==0x10u);
    static_assert(n::kManagerCountWord==0x12u);
    static_assert(n::kManagerFreeHeadWord==0x14u);
    static_assert(offsetof(n::Record,opaque_header)==4u);
    static_assert(sizeof(n::Record)==16u);
}
static void test_insert_lookup_and_native_callback_order() {
    n::Map m(5);
    FakeTree hooks;
    Less cmp;
    m.nodes[0].opaque_header=0xDEADBEEFu;
    assert(n::insert(m,key(20u),hooks)==0u);
    assert(m.root==0u && m.live_count==1u);
    assert(m.nodes[0].payload==key(20u));
    assert(m.nodes[0].opaque_header==0xDEADBEEFu);
    assert(n::insert(m,key(10u),hooks)==1u);
    assert(n::insert(m,key(30u),hooks)==2u);
    assert(m.live_count==3u && m.nodes[0].left==1u &&
           m.nodes[0].right==2u);
    const std::vector<std::string> expected={
        "find","allocate","attach","find","allocate","attach",
        "find","allocate","attach"};
    assert(hooks.events==expected);
    assert(n::lookup(m,key(20u),cmp)==0u);
    assert((cmp.comparisons==std::vector<std::string>{"20:20","20:20"}));
    cmp.comparisons.clear();
    assert(n::lookup(m,key(10u),cmp)==1u);
    assert((cmp.comparisons==std::vector<std::string>{
        "10:20","10:10","10:10"}));
    cmp.comparisons.clear();
    assert(n::lookup(m,key(30u),cmp)==2u);
    assert((cmp.comparisons==std::vector<std::string>{
        "30:20","20:30","30:30","30:30"}));
    cmp.comparisons.clear();
    assert(n::lookup(m,key(25u),cmp)==n::kAbsent);
    assert((cmp.comparisons==std::vector<std::string>{
        "25:20","20:25","25:30"}));
}
static void test_both_release_variants_and_free_head_reuse() {
    n::Map m(4);
    FakeTree hooks;
    Less cmp;
    assert(n::insert(m,key(20u),hooks)==0u);
    assert(n::insert(m,key(10u),hooks)==1u);
    assert(n::insert(m,key(30u),hooks)==2u);
    m.nodes[2].opaque_header=0xDEADBEEFu;
    assert(n::release_variant_b(m,2u,hooks));
    assert(hooks.events.back()=="erase");
    assert(m.live_count==2u && m.free_head==2u);
    assert(m.nodes[2].left==2u && m.nodes[2].right==n::kAbsent);
    assert(m.nodes[2].payload==key(30u));
    assert(m.nodes[2].opaque_header==0xDEADBEEFu);
    assert(n::lookup(m,key(30u),cmp)==n::kAbsent);
    assert(n::insert(m,key(40u),hooks)==2u);
    assert(m.free_head==n::kAbsent);
    assert(m.live_count==3u && m.nodes[2].payload==key(40u));
    assert(m.nodes[2].opaque_header==0xDEADBEEFu);
    assert(n::lookup(m,key(40u),cmp)==2u);
    assert(n::release_variant_a(m,2u,hooks));
    assert(m.free_head==2u && m.live_count==2u);
    const auto old_events=hooks.events.size();
    assert(!n::release_variant_a(m,n::kAbsent,hooks));
    assert(hooks.events.size()==old_events);
}
static void test_native_count_wrap_and_sentinel_noop() {
    n::Map m(1);
    FakeTree hooks;
    m.root=0;
    m.nodes[0].opaque_header=0x12345678u;
    m.nodes[0].payload=key(7u);
    assert(n::release_variant_a(m,0u,hooks));
    assert(m.live_count==n::kAbsent); // native WORD decrement wraps
    assert(m.free_head==0u && m.nodes[0].left==0u);
    assert(m.nodes[0].opaque_header==0x12345678u);
    assert(m.nodes[0].payload==key(7u));
}
static void test_host_safety_guards_and_empty_lookup() {
    n::Map empty(0);
    FakeTree hooks;
    Less cmp;
    assert(n::lookup(empty,key(1u),cmp)==n::kAbsent);
    assert(cmp.comparisons.empty());
    assert(!n::release_variant_b(empty,n::kAbsent,hooks));
    bool failed=false;
    try{(void)n::release_variant_b(empty,0u,hooks);}
    catch(const std::out_of_range&){failed=true;}
    assert(failed && hooks.events.empty());
    n::Map invalid(1);
    invalid.root=2u;
    failed=false;
    try{(void)n::lookup(invalid,key(1u),cmp);}
    catch(const std::out_of_range&){failed=true;}
    assert(failed);
    invalid.root=0u;
    invalid.nodes[0].payload=key(20u);
    invalid.nodes[0].left=0u;
    failed=false;
    try{(void)n::lookup(invalid,key(10u),cmp);}
    catch(const std::runtime_error&){failed=true;}
    assert(failed);
    n::Map insertion(1);
    hooks.force_invalid_index=true;
    failed=false;
    try{(void)n::insert(insertion,key(22u),hooks);}
    catch(const std::out_of_range&){failed=true;}
    assert(failed && insertion.live_count==0u && insertion.root==n::kAbsent);
}
int main() {
    test_original_layout_and_methods();
    test_insert_lookup_and_native_callback_order();
    test_both_release_variants_and_free_head_reuse();
    test_native_count_wrap_and_sentinel_noop();
    test_host_safety_guards_and_empty_lookup();
    return 0;
}
