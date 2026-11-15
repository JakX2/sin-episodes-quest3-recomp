#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_client_pvs_comparator_dispatch.hpp"
#include "sin_compat/original_client_pvs_map_node_growth.hpp"
#include "sin_compat/original_client_pvs_rb_fixup.hpp"

namespace p=sin_compat::original_client_pvs_comparator_dispatch;
namespace m=p::map;
namespace ix=sin_compat::original_client_pvs_map_search_index;
namespace ng=sin_compat::original_client_pvs_map_node_growth;
namespace growth=sin_compat::original_client_pvs_storage_growth;
namespace rb=sin_compat::original_client_pvs_rb_fixup;
namespace attach=sin_compat::original_client_pvs_tree_attach;

struct Memory final:growth::IMemoryBackend {
    std::vector<std::size_t> requests;
    bool request(growth::MemoryAction,std::size_t bytes) override {
        requests.push_back(bytes);
        return true;
    }
};
struct Setup {
    m::Map manager{0u};
    Memory memory{};
    ng::ResolverState resolver{};
    ng::NativeIndexCallbacks allocation{manager,memory,resolver};
    ix::IndexState indices{};
    p::GlobalComparatorPointer global{};
    p::ComparatorThroughThunk comparator{global};
    rb::NativeRedBlackFixup balance{};
    attach::NativeTreeAttach tree_attach{balance};
    ix::InsertBridge bridge{indices,allocation,comparator,tree_attach};
};

static void test_whole_original_metadata_and_owner_layout() {
    static_assert(p::kUnsignedComparatorVA==0x241056C0u);
    static_assert(p::kGlobalThunkVA==0x24105CF0u);
    static_assert(p::kGlobalGetSetVA==0x24105760u);
    static_assert(p::kKeyInsertionAdapterVA==0x24108800u);
    static_assert(p::kNativeGlobalPointerVA==0x2433FDB8u);
    static_assert(p::kNewOriginalMethods==4u);
    static_assert(p::kWholeOriginalCodeBytes==97u);
    static_assert(p::kOwnerEmbeddedMapOffset==0x18060u);
    static_assert(p::kMapThunkDword==0u);
    static_assert(p::kMapCandidateDword==0x1Cu);
    static_assert(p::kKeyStoredBytes==8u);
    static_assert(p::kNativeKeyInitializedBytes==6u);
    static_assert(p::kNativeKeyUninitializedBytes==2u);
}
static void test_unsigned_first_dword_only() {
    p::UnsignedFirstDwordComparator cmp;
    const auto small=p::key_from_native_adapter(1u,0xFFFFu,{{0xFFu,0xFFu}});
    const auto larger=p::key_from_native_adapter(0x80000000u,0u,{{0u,0u}});
    const auto max=p::key_from_native_adapter(0xFFFFFFFFu,0u,{{0u,0u}});
    assert(cmp.less(small,larger));
    assert(cmp.less(larger,max));
    assert(!cmp.less(max,small));
    assert(p::first_dword(larger)==0x80000000u);
    assert(p::first_dword(max)==0xFFFFFFFFu);
    const auto same_first=p::key_from_native_adapter(1u,0u,{{0u,0u}});
    assert(!cmp.less(small,same_first));
    assert(!cmp.less(same_first,small));
    assert(small!=same_first);
}
static void test_native_undefined_adapter_bytes_are_supplied_not_zeroed() {
    const auto key=p::key_from_native_adapter(0x89ABCDEFu,0x1234u,{{0xD3u,0x7Au}});
    assert((key==m::Payload{{0xEFu,0xCDu,0xABu,0x89u,0x34u,0x12u,0xD3u,0x7Au}}));
    const auto alternative=p::key_from_native_adapter(
        0x89ABCDEFu,0x1234u,{{0x01u,0x02u}});
    assert(p::UnsignedFirstDwordComparator{}.less(key,alternative)==false);
    assert(p::UnsignedFirstDwordComparator{}.less(alternative,key)==false);
    assert(key[6]!=alternative[6] && key[7]!=alternative[7]);
}
static void test_native_global_set_or_get_and_thunk_refusal() {
    p::GlobalComparatorPointer global;
    p::ComparatorThroughThunk thunk(global);
    const auto a=p::key_from_native_adapter(2u,0u,{{0u,0u}});
    const auto b=p::key_from_native_adapter(4u,0u,{{0u,0u}});
    assert(global.native_set_or_get(0u)==0u);
    assert(global.writes==0u);
    bool refused=false;
    try{(void)thunk.less(a,b);}
    catch(const std::runtime_error&){refused=true;}
    assert(refused);
    assert(global.native_set_or_get(p::kUnsignedComparatorVA)==p::kUnsignedComparatorVA);
    assert(global.writes==1u);
    assert(thunk.less(a,b));
    assert(!thunk.less(b,a));
    assert(global.native_set_or_get(0u)==p::kUnsignedComparatorVA);
    assert(global.writes==1u);
    assert(global.native_set_or_get(0xDEAD1000u)==0xDEAD1000u);
    assert(global.writes==2u);
    refused=false;
    try{(void)thunk.less(a,b);}
    catch(const std::runtime_error&){refused=true;}
    assert(refused);
    (void)global.native_set_or_get(p::kUnsignedComparatorVA);
    assert(global.writes==3u && thunk.less(a,b));
}
static void test_map_adapter_with_cross_phase_insert_lookup_and_recolor() {
    Setup host;
    const auto idx_a=p::insert_with_native_adapter(
        host.manager,0x80000000u,0x0A0Bu,{{0x11u,0x22u}},
        p::kUnsignedComparatorVA,host.global,host.bridge);
    assert(idx_a==0u && host.global.writes==1u);
    assert(host.manager.nodes[0].payload==p::key_from_native_adapter(
        0x80000000u,0x0A0Bu,{{0x11u,0x22u}}));
    // A zero candidate must preserve the previous global pointer.
    const auto idx_b=p::insert_with_native_adapter(
        host.manager,1u,0xAA55u,{{0x7Fu,0x80u}},
        0u,host.global,host.bridge);
    const auto idx_c=p::insert_with_native_adapter(
        host.manager,0xFFFFFFFFu,0x1234u,{{0x34u,0x12u}},
        0u,host.global,host.bridge);
    assert(idx_b==1u && idx_c==2u);
    assert(host.global.target==p::kUnsignedComparatorVA);
    assert(host.global.writes==1u);
    assert(host.manager.root==0u);
    assert(host.manager.nodes[0].left==1u);
    assert(host.manager.nodes[0].right==2u);
    assert(host.manager.live_count==3u);
    assert(host.balance.callbacks==3u);
    assert((host.memory.requests==std::vector<std::size_t>{32u,64u}));
    assert(m::lookup(host.manager,
        p::key_from_native_adapter(1u,0u,{{0u,0u}}),
        host.comparator)==idx_b);
    // Original map lookup compares FIRST DWORD only, even when other
    // four stored payload bytes are different.
    assert(m::lookup(host.manager,
        p::key_from_native_adapter(0x80000000u,0u,{{0u,0u}}),
        host.comparator)==idx_a);
    assert(m::lookup(host.manager,
        p::key_from_native_adapter(0xFFFFFFFFu,0xFFFFu,{{0xAAu,0xBBu}}),
        host.comparator)==idx_c);
    assert(m::lookup(host.manager,
        p::key_from_native_adapter(99u,0u,{{0u,0u}}),
        host.comparator)==m::kAbsent);
}
static void test_equal_first_dword_has_equal_order_despite_other_bytes() {
    Setup host;
    const auto first=p::insert_with_native_adapter(
        host.manager,77u,0xAAAAu,{{0x11u,0x22u}},
        p::kUnsignedComparatorVA,host.global,host.bridge);
    const auto second=p::insert_with_native_adapter(
        host.manager,77u,0xBBBBu,{{0x33u,0x44u}},
        0u,host.global,host.bridge);
    assert(first==0u && second==1u);
    assert(host.manager.live_count==2u);
    // Phase237 placement uses one less-than comparison and follows
    // the right branch when equal. The original comparator defines
    // equal ordering even for two distinct stored eight-byte payloads.
    assert(host.manager.nodes[first].right==second);
    assert(m::lookup(host.manager,
       p::key_from_native_adapter(77u,0u,{{0u,0u}}),
       host.comparator)==first);
    assert(host.manager.nodes[second].payload==
       p::key_from_native_adapter(77u,0xBBBBu,{{0x33u,0x44u}}));
}
static void test_unknown_native_function_target_fails_closed() {
    Setup host;
    const auto inserted=p::insert_with_native_adapter(
        host.manager,10u,0u,{{0u,0u}},
        p::kUnsignedComparatorVA,host.global,host.bridge);
    assert(inserted==0u);
    const auto size=host.manager.live_count;
    bool refused=false;
    try{
        (void)p::insert_with_native_adapter(
            host.manager,20u,0u,{{0u,0u}},
            0xDEADBEEFu,host.global,host.bridge);
    }catch(const std::runtime_error&){refused=true;}
    assert(refused);
    assert(host.global.target==0xDEADBEEFu);
    assert(host.manager.live_count==size);
    assert(host.indices.next_unused==1u);
    assert(host.balance.callbacks==1u);
}
int main() {
    test_whole_original_metadata_and_owner_layout();
    test_unsigned_first_dword_only();
    test_native_undefined_adapter_bytes_are_supplied_not_zeroed();
    test_native_global_set_or_get_and_thunk_refusal();
    test_map_adapter_with_cross_phase_insert_lookup_and_recolor();
    test_equal_first_dword_has_equal_order_despite_other_bytes();
    test_unknown_native_function_target_fails_closed();
    return 0;
}
