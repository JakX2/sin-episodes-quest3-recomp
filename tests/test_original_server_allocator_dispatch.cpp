#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "sin_compat/original_server_allocator_dispatch.hpp"
#include "sin_compat/original_server_cache_array_growth.hpp"
#include "sin_compat/original_server_cache_slot_allocation.hpp"
namespace m=sin_compat::original_server_allocator_dispatch;
namespace g=sin_compat::original_server_cache_array_growth;
namespace a=sin_compat::original_server_cache_slot_allocation;

struct MockAllocator {
    bool tracked{};
    std::uint32_t context_value{};
    std::uint32_t next=0x1000u;
    std::map<std::uint32_t,std::uint32_t> live;
    std::vector<std::string> calls;
    std::uint32_t make_metadata() {calls.emplace_back("make-metadata");return context_value;}
    bool metadata_enabled() {calls.emplace_back("metadata-enabled");return tracked;}
    std::uint32_t allocate_basic(std::uint32_t bytes) {
        calls.emplace_back("alloc-basic");return reserve(bytes);
    }
    std::uint32_t allocate_tagged(std::uint32_t bytes,std::uint32_t context,
                                  std::uint32_t zero) {
        assert(context==context_value && zero==0u);
        calls.emplace_back("alloc-tagged");return reserve(bytes);
    }
    std::uint32_t reallocate_basic(std::uint32_t previous,std::uint32_t bytes) {
        calls.emplace_back("realloc-basic");return resize(previous,bytes);
    }
    std::uint32_t reallocate_tagged(std::uint32_t previous,std::uint32_t bytes,
                                    std::uint32_t context,std::uint32_t zero) {
        assert(context==context_value && zero==0u);
        calls.emplace_back("realloc-tagged");return resize(previous,bytes);
    }
    void deallocate_direct(std::uint32_t pointer) {
        calls.emplace_back("free");assert(live.erase(pointer)==1u);
    }
    std::uint32_t reserve(std::uint32_t bytes) {
        next+=0x1000u;live[next]=bytes;return next;
    }
    std::uint32_t resize(std::uint32_t previous,std::uint32_t bytes) {
        assert(live.erase(previous)==1u);return reserve(bytes);
    }
};
struct HeapBridge {
    m::NativeLazyContexts32& state;
    MockAllocator& heap;
    std::uint32_t allocate(std::uint32_t bytes) {
        return m::allocate(state,heap,bytes);
    }
    std::uint32_t resize(std::uint32_t pointer,std::uint32_t bytes) {
        return m::reallocate(state,heap,pointer,bytes);
    }
};
struct SlotBridge {
    HeapBridge& heap;
    void grow(a::Pool& pool,std::uint32_t extra) {
        g::NativeVectorHeader32 header{pool.native.slots_04,
            pool.native.capacity_08,pool.native.unknown_0c};
        const auto result=g::grow(header,extra,heap);
        assert(result.action!=g::Allocation::skipped_disabled);
        pool.native.slots_04=header.backing_00;
        pool.native.capacity_08=header.capacity_04;
        pool.slots.resize(header.capacity_04);
    }
    std::uint16_t next_free(const a::Pool& pool,std::uint16_t idx) {
        assert(idx<pool.slots.size());return pool.slots[idx].right_02;
    }
};
int main() {
    static_assert(m::kAllocateVA==0x2226e390u);
    static_assert(m::kReallocateVA==0x2226e400u);
    assert(m::kGlobalFirstFlagVA==m::kGlobalFirstContextVA+4u);
    assert(m::kGlobalSecondContextVA==m::kGlobalFirstFlagVA+4u);
    assert(m::kGlobalSecondFlagVA==m::kGlobalSecondContextVA+4u);
    {
        MockAllocator h;
        m::NativeLazyContexts32 state{};
        auto p=m::allocate(state,h,48u);
        assert(state.alloc_flags_04==1u && state.alloc_context_00==0u);
        assert((h.calls==std::vector<std::string>{"metadata-enabled","alloc-basic"}));
        h.tracked=true;h.context_value=0xA500u;
        auto q=m::allocate(state,h,24u);
        assert(q!=p);
        assert(h.calls.back()=="alloc-basic"); // original failed context remains cached
        auto r=m::reallocate(state,h,p,96u);
        assert(h.calls[h.calls.size()-2]=="make-metadata");
        assert(h.calls.back()=="realloc-tagged");
        assert(state.realloc_context_08==0xA500u);
        assert(state.realloc_flags_0c==1u);
        m::deallocate(h,q);m::deallocate(h,r);
        assert(h.live.empty());
        assert(h.calls[h.calls.size()-1]=="free");
    }
    {
        MockAllocator h;h.tracked=true;h.context_value=0xA600u;
        m::NativeLazyContexts32 state{};
        state.alloc_flags_04=0xA0u;
        state.realloc_flags_0c=0x20u;
        auto p=m::allocate(state,h,48u);
        assert(state.alloc_flags_04==0xA1u);
        assert(state.alloc_context_00==0xA600u);
        assert(h.calls.back()=="alloc-tagged");
        auto r=m::reallocate(state,h,p,72u);
        assert(state.realloc_flags_0c==0x21u);
        assert(state.realloc_context_08==0xA600u);
        assert(h.calls.back()=="realloc-tagged");
        assert(std::count(h.calls.begin(),h.calls.end(),"make-metadata")==2);
        m::deallocate(h,r);assert(h.live.empty());
    }
    for (const bool tracked:{false,true}) {
        MockAllocator heap;heap.tracked=tracked;heap.context_value=0xBE00u;
        m::NativeLazyContexts32 cache{};
        HeapBridge bridge{cache,heap};
        SlotBridge slotbridge{bridge};
        a::Pool pool;
        assert(a::claim(pool,slotbridge)==0u);
        assert(pool.native.capacity_08==2u);
        assert(a::claim(pool,slotbridge)==1u);
        assert(a::claim(pool,slotbridge)==2u);
        assert(pool.native.capacity_08==4u);
        assert(heap.live.size()==1u);
        assert(heap.live.begin()->first==pool.native.slots_04);
        assert(heap.live.begin()->second==96u);
        assert(cache.alloc_flags_04==1u && cache.realloc_flags_0c==1u);
        assert(cache.alloc_context_00==(tracked?0xBE00u:0u));
        assert(cache.realloc_context_08==(tracked?0xBE00u:0u));
        assert(std::count(heap.calls.begin(),heap.calls.end(),tracked?"alloc-tagged":"alloc-basic")==1);
        assert(std::count(heap.calls.begin(),heap.calls.end(),tracked?"realloc-tagged":"realloc-basic")==1);
        m::deallocate(heap,pool.native.slots_04);
        assert(heap.live.empty());
    }
}
