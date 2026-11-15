#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <utility>
#include "sin_compat/original_client_pvs_sixteen_list.hpp"

namespace n=sin_compat::original_client_pvs_sixteen_list;

struct SyntheticAllocator final:n::IIndexAllocator {
    std::vector<std::uint16_t> indices;
    std::size_t cursor{};
    std::size_t calls{};
    explicit SyntheticAllocator(std::vector<std::uint16_t> values):
        indices(std::move(values)){}
    std::uint16_t reserve_zero() override {
        ++calls;
        if(cursor>=indices.size())
            throw std::out_of_range("host index allocator exhausted");
        return indices[cursor++];
    }
};

static void test_original_method_and_layout_evidence() {
    static_assert(n::kOriginalAppendVA==0x241086F0u);
    static_assert(n::kOriginalUnlinkVA==0x24106CB0u);
    static_assert(n::kOriginalReserveVA==0x24106610u);
    static_assert(n::kOriginalReviewedMethods==2u);
    static_assert(n::kOriginalReviewedBytes==236u);
    static_assert(n::kNativeEntryStride==16u);
    static_assert(sizeof(n::Node)==16u);
    static_assert(n::kNativePrevWord==0x0Cu);
    static_assert(n::kNativeNextWord==0x0Eu);
    static_assert(n::kManagerHeadWord==0x0Cu);
    static_assert(n::kManagerTailWord==0x0Eu);
    static_assert(n::kManagerCountWord==0x12u);
    static_assert(n::kManagerCapacityWord==0x14u);
    static_assert(n::kAbsent==0xFFFFu);
}
static void test_append_head_tail_and_payload_is_untouched() {
    n::List list(5u);
    list.nodes[1u].opaque_payload[0]=0xA1u;
    list.nodes[3u].opaque_payload[11]=0xB2u;
    SyntheticAllocator a({1u,3u,2u,3u});
    assert(n::append(list,a)==1u);
    assert(list.count==1u && list.head==1u && list.tail==1u);
    assert(n::checked_node(list,1u).prev==n::kAbsent);
    assert(n::checked_node(list,1u).next==n::kAbsent);
    assert(n::append(list,a)==3u);
    assert(list.head==1u && list.tail==3u && list.count==2u);
    assert(list.nodes[1u].next==3u && list.nodes[3u].prev==1u);
    assert(n::append(list,a)==2u);
    assert(list.head==1u && list.tail==2u && list.count==3u);
    assert(list.nodes[3u].next==2u && list.nodes[2u].prev==3u);
    // 0x241086F0 first calls 0x24106CB0, so re-append of an
    // already-active slot moves it to the tail without changing count.
    assert(n::append(list,a)==3u);
    assert(a.calls==4u);
    assert(list.head==1u && list.tail==3u && list.count==3u);
    assert(list.nodes[1u].next==2u);
    assert(list.nodes[2u].prev==1u && list.nodes[2u].next==3u);
    assert(list.nodes[3u].prev==2u && list.nodes[3u].next==n::kAbsent);
    assert(list.nodes[1u].opaque_payload[0]==0xA1u);
    assert(list.nodes[3u].opaque_payload[11]==0xB2u);
}
static void test_unlink_head_middle_tail_and_free_marker() {
    n::List list(6u);
    SyntheticAllocator a({1u,2u,3u,4u});
    for(std::size_t i=0;i<4u;i++)n::append(list,a);
    assert(list.count==4u && list.head==1u && list.tail==4u);
    assert(n::unlink(list,2u));
    assert(list.nodes[1u].next==3u && list.nodes[3u].prev==1u);
    assert(list.nodes[2u].prev==2u && list.nodes[2u].next==2u);
    assert(list.count==3u);
    assert(n::unlink(list,1u));
    assert(list.head==3u && list.nodes[3u].prev==n::kAbsent);
    assert(list.count==2u);
    assert(n::unlink(list,4u));
    assert(list.tail==3u && list.nodes[3u].next==n::kAbsent);
    assert(list.count==1u);
    assert(n::unlink(list,3u));
    assert(list.head==n::kAbsent && list.tail==n::kAbsent);
    assert(list.count==0u);
    assert(!n::unlink(list,3u));
    assert(!n::unlink(list,5u));
    assert(!n::unlink(list,n::kAbsent));
    assert(list.count==0u);
}
static void test_native_word_count_wrap_and_host_safety() {
    n::List list(3u);
    SyntheticAllocator a({1u,3u,0u});
    assert(n::append(list,a)==1u);
    assert(list.count==1u);
    // Original x86 'add WORD [ECX+0x12],0xffff' wraps modulo 2^16.
    list.count=0u; // deliberately malformed state, not actual normal list
    assert(n::unlink(list,1u));
    assert(list.count==0xFFFFu);
    const auto before=list;
    bool rejected=false;
    try{n::append(list,a);}catch(const std::out_of_range&){rejected=true;}
    assert(rejected && list.head==before.head && list.tail==before.tail);
    assert(list.count==before.count);
    rejected=false;
    try{n::List oversize(65536u);(void)oversize;}
    catch(const std::out_of_range&){rejected=true;}
    assert(rejected);
    rejected=false;
    try{(void)n::checked_node(list,65535u);}
    catch(const std::out_of_range&){rejected=true;}
    assert(rejected);
    // Unlinking an index beyond original capacity fails without mutation.
    assert(!n::unlink(list,100u));
}
int main() {
    test_original_method_and_layout_evidence();
    test_append_head_tail_and_payload_is_untouched();
    test_unlink_head_middle_tail_and_free_marker();
    test_native_word_count_wrap_and_host_safety();
}
