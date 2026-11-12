#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include "sin_compat/original_server_cache_key_comparators.hpp"
#include "sin_compat/original_server_cache_tree_lookup.hpp"
namespace cmp=sin_compat::original_server_cache_key_comparators;
namespace tree=sin_compat::original_server_cache_tree_lookup;

struct ComparatorHost {
    std::vector<std::string> events;
    std::uint32_t compare(std::uint32_t lhs,std::uint32_t rhs) {
        cmp::NativeKeyPrefix32 left{lhs,17u,99u,0u};
        cmp::NativeKeyPrefix32 right{rhs,42u,23u,0u};
        events.emplace_back("cmp");
        return cmp::less_first_dword_unsigned(left,right);
    }
    void initialize_sentinel(std::uint32_t) { events.emplace_back("sentinel"); }
};
int main() {
    static_assert(cmp::kSceneComparatorSlotVA==0x22503EE4u, "original scene object +4");
    static_assert(cmp::kModelSoundsComparatorSlotVA==0x224F038Cu, "original model sounds object +4");
    // The second DWORD and higher must not participate in equality.
    cmp::NativeKeyPrefix32 a{0x80000000u,1u,2u,3u};
    cmp::NativeKeyPrefix32 b{0x7fffffffu,9u,8u,7u};
    assert(cmp::less_first_dword_unsigned(a,b)==0u);
    assert(cmp::less_first_dword_unsigned(b,a)==1u);
    assert(cmp::less_first_dword_unsigned(a,a)==0u);
    assert(cmp::less_first_dword_unsigned({0u,1u},{0u,99u})==0u);
    assert(cmp::less_first_dword_unsigned({0u},{0xffffffffu})==1u);
    assert(cmp::less_first_dword_unsigned({0xffffffffu},{0u})==0u);

    // Invoke the Phase191 tree lookup with the original-comparator predicate.
    // Use a sorted, synthetic tree with unsigned 32-bit values spanning bit31.
    for (auto sentinel_va:{tree::kSceneSentinelVA,tree::kModelSoundsSentinelVA}) {
        tree::Tree t;
        t.nodes.resize(3);
        t.root=0;
        t.nodes[0].key_08=0x80000000u;
        t.nodes[0].left_00=1;
        t.nodes[0].right_02=2;
        t.nodes[1].key_08=0x7fffffffu;
        t.nodes[2].key_08=0xffffffffu;
        tree::SentinelState sentinel;
        ComparatorHost host;
        assert(tree::find(t,0x80000000u,sentinel,sentinel_va,host)==0u);
        assert(!sentinel.ready);
        assert(tree::find(t,0x7fffffffu,sentinel,sentinel_va,host)==1u);
        assert(sentinel.ready);
        assert(tree::find(t,0xffffffffu,sentinel,sentinel_va,host)==2u);
        assert(tree::find(t,0u,sentinel,sentinel_va,host)==tree::kMissing);
        assert(host.events.front()=="cmp");
    }
}
