#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_server_cache_array_growth.hpp"
#include "sin_compat/original_server_cache_slot_allocation.hpp"

namespace growth=sin_compat::original_server_cache_array_growth;
namespace slot=sin_compat::original_server_cache_slot_allocation;

struct Heap {
    std::vector<std::uint32_t> bytes{};
    std::vector<std::uint32_t> previous{};
    std::uint32_t next=0x1000u;
    std::uint32_t allocate(std::uint32_t length) {
        bytes.push_back(length); previous.push_back(0u);
        return next+=0x1000u;
    }
    std::uint32_t resize(std::uint32_t old,std::uint32_t length) {
        bytes.push_back(length); previous.push_back(old);
        return next+=0x1000u;
    }
};
struct SlotHost {
    Heap& heap;
    void grow(slot::Pool& pool,std::uint32_t extra) {
        // Proven original Phase194 call uses ECX=&tree.+4, so +0/+4/+8
        // in growth view are the native tree +4,+8,+0C WORDs/DWORDs.
        growth::NativeVectorHeader32 v{
            pool.native.slots_04,pool.native.capacity_08,pool.native.unknown_0c
        };
        (void)growth::grow(v,extra,heap);
        pool.native.slots_04=v.backing_00;
        pool.native.capacity_08=v.capacity_04;
        pool.slots.resize(v.capacity_04);
    }
    std::uint16_t next_free(const slot::Pool& pool,std::uint16_t index) {
        assert(index<pool.slots.size());
        return pool.slots[index].right_02;
    }
};
int main() {
    static_assert(growth::kSceneGrowVA==slot::kSceneGrowVA);
    static_assert(growth::kModelSoundsGrowVA==slot::kModelSoundsGrowVA);
    static_assert(offsetof(slot::NativeTreeHeader32,slots_04)+offsetof(growth::NativeVectorHeader32,capacity_04)==offsetof(slot::NativeTreeHeader32,capacity_08));
    static_assert(offsetof(slot::NativeTreeHeader32,slots_04)+offsetof(growth::NativeVectorHeader32,growth_step_08)==offsetof(slot::NativeTreeHeader32,unknown_0c));

    for(const auto va:{growth::kSceneGrowVA,growth::kModelSoundsGrowVA}) {
        (void)va;
        Heap h;
        growth::NativeVectorHeader32 v{};
        auto r=growth::grow(v,1u,h);
        assert(r.action==growth::Allocation::allocated && r.capacity==2u && r.bytes==48u);
        assert(v.backing_00==0x2000u && v.capacity_04==2u);
        r=growth::grow(v,1u,h);
        assert(r.action==growth::Allocation::reallocated && r.bytes==96u && r.capacity==4u);
        assert(v.backing_00==0x3000u && h.previous.back()==0x2000u);
        r=growth::grow(v,0u,h); // native code reallocates even without capacity change
        assert(r.action==growth::Allocation::reallocated && r.bytes==96u);
        assert(h.bytes.size()==3u);
        v.growth_step_08=3u;
        r=growth::grow(v,4u,h);
        assert(r.capacity==10u && r.bytes==240u); // cap 4 -> 7 -> 10
        v.growth_step_08=growth::kDisabledPolicy;
        const auto saved=v;
        r=growth::grow(v,55u,h);
        assert(r.action==growth::Allocation::skipped_disabled);
        assert(h.bytes.size()==4u && v.backing_00==saved.backing_00 && v.capacity_04==saved.capacity_04);

        growth::NativeVectorHeader32 zero{0u,0u,0u};
        r=growth::grow(zero,0u,h);
        assert(r.action==growth::Allocation::allocated && r.bytes==0u && r.capacity==0u);
        growth::NativeVectorHeader32 big{0u,0x7fffffffu,0u};
        bool rejected=false;
        try {(void)growth::grow(big,1u,h);}catch(const std::overflow_error&){rejected=true;}
        assert(rejected);
        growth::NativeVectorHeader32 step{0u,0u,7u};
        r=growth::grow(step,9u,h);
        assert(r.capacity==9u && r.bytes==216u);

        growth::NativeKeyInitializer32 key{12u,34u,56u,78u};
        growth::initialize_key(&key);
        assert(key.word_00==0u && key.word_04==0u);
        assert(key.word_08==0xfffffffeu && key.word_0c==0xffffffffu);
        growth::initialize_key(nullptr);

        // Real cross-phase wiring: Phase194 claim calls the exact Phase201
        // capacity algorithm, not the old synthetic +1 growth callback.
        Heap heap;
        slot::Pool pool;
        SlotHost host{heap};
        assert(slot::claim(pool,host)==0u);
        assert(pool.native.capacity_08==2u && pool.native.next_unused_16==1u);
        assert(pool.native.latest_slots_base_18==pool.native.slots_04);
        assert(heap.bytes.size()==1u && heap.bytes.back()==48u);
        assert(slot::claim(pool,host)==1u);
        assert(pool.native.next_unused_16==2u && heap.bytes.size()==1u);
        assert(slot::claim(pool,host)==2u);
        assert(pool.native.capacity_08==4u && pool.native.next_unused_16==3u);
        assert(heap.bytes.size()==2u && heap.bytes.back()==96u);
        assert(heap.previous.back()!=0u);
        pool.native.free_head_14=0u;
        pool.slots[0].right_02=slot::kMissing;
        assert(slot::claim(pool,host)==0u);
        assert(pool.native.free_head_14==slot::kMissing);
        assert(heap.bytes.size()==2u); // free-list reuse never grows
    }
}
