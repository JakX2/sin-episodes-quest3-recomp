#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include "sin_compat/original_server_cache_slot_allocation.hpp"
#include "sin_compat/original_server_cache_tree_insert.hpp"
#include "sin_compat/original_server_cache_key_comparators.hpp"
namespace alloc=sin_compat::original_server_cache_slot_allocation;
namespace ins=sin_compat::original_server_cache_tree_insert;
struct Host {
    std::vector<std::string> calls;
    void grow(alloc::Pool& pool,std::uint32_t extra) {
        calls.emplace_back("grow");assert(extra==1);
        pool.native.capacity_08+=extra;
        pool.slots.resize(pool.native.capacity_08);
        pool.native.slots_04+=0x1000;
    }
    std::uint16_t next_free(const alloc::Pool& pool,std::uint16_t idx) {
        calls.emplace_back("free-next");
        if (idx>=pool.slots.size()) throw std::out_of_range("host-only invalid free list index");
        return pool.slots[idx].right_02;
    }
};
struct InsertHost {
    alloc::Pool& pool;
    Host& h;
    std::vector<std::string> calls;
    void prepare(const ins::Config&,const sin_compat::original_server_cache_key_comparators::NativeKeyPrefix32&,std::uint32_t) {calls.emplace_back("prepare");}
    std::uint32_t claim(const ins::Config&) {calls.emplace_back("claim");return alloc::claim(pool,h);}
    void link(const ins::Config&,std::uint32_t) {calls.emplace_back("link");}
};
int main() {
    static_assert(alloc::kSceneAllocVA==0x221A6890u);
    static_assert(alloc::kModelSoundsAllocVA==0x22140900u);
    for(const auto cfg:{ins::kScene,ins::kModelSounds}) {
        Host h;
        alloc::Pool pool;
        pool.native.slots_04=0x1000;
        pool.native.live_count_12=99;
        assert(alloc::claim(pool,h)==0);
        assert((h.calls==std::vector<std::string>{"grow"}));
        assert(pool.native.capacity_08==1&&pool.native.next_unused_16==1);
        assert(pool.native.live_count_12==99); // separate Phase193 counter
        assert(pool.native.latest_slots_base_18==0x2000);
        assert(pool.slots[0].key_08==0&&pool.slots[0].payload_14==0xffffffffu);
        std::uint32_t dwords[2]{};
        std::memcpy(dwords,pool.slots[0].unknown_0c,8);
        assert(dwords[0]==0u&&dwords[1]==0xfffffffeu);
        h.calls.clear();
        assert(alloc::claim(pool,h)==1);
        assert((h.calls==std::vector<std::string>{"grow"}));
        assert(pool.native.next_unused_16==2&&pool.native.capacity_08==2);
        // Native free-list head reuses slot #0, whose +2 WORD points to #1.
        pool.native.free_head_14=0;
        pool.slots[0].right_02=1;
        pool.slots[0].key_08=55;
        pool.slots[0].payload_14=77;
        h.calls.clear();
        assert(alloc::claim(pool,h)==0);
        assert((h.calls==std::vector<std::string>{"free-next"}));
        assert(pool.native.free_head_14==1&&pool.native.next_unused_16==2);
        assert(pool.slots[0].key_08==0&&pool.slots[0].payload_14==0xffffffffu);
        pool.native.free_head_14=alloc::kMissing;
        pool.native.capacity_08=3;pool.slots.resize(3);
        h.calls.clear();
        assert(alloc::claim(pool,h)==2);
        assert(h.calls.empty());
        assert(pool.native.next_unused_16==3);
        // Allocate with the previous original callback from Phase193 and
        // verify key writes overwrite the allocator's four-word sentinels.
        ins::State insert_state;
        insert_state.slots.nodes.resize(3);
        InsertHost ih{pool,h,{}};
        pool.native.free_head_14=2;
        pool.slots[2].right_02=alloc::kMissing;
        auto index=ins::insert(cfg,insert_state,{0x80000000u,1u,2u,3u},0u,ih);
        assert(index==2);
        assert((ih.calls==std::vector<std::string>{"prepare","claim","link"}));
        assert(insert_state.slots.nodes[2].key_08==0x80000000u);
        assert(insert_state.slots.nodes[2].payload_14==3u);
        assert(pool.native.free_head_14==alloc::kMissing);
        // An impossible native slot index fails safely in the host model.
        pool.native.free_head_14=99;
        bool bad=false;
        try { (void)alloc::claim(pool,h); }
        catch(const std::out_of_range&) {bad=true;}
        assert(bad);
    }
}
