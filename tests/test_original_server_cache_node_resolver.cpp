#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_server_cache_node_resolver.hpp"
#include "sin_compat/original_server_cache_slot_allocation.hpp"

namespace r=sin_compat::original_server_cache_node_resolver;
namespace a=sin_compat::original_server_cache_slot_allocation;
namespace t=sin_compat::original_server_cache_tree_lookup;

struct Host {
    r::Config cfg;
    r::Sentinel state{};
    std::vector<std::uint32_t> initialization_addresses;
    std::uint32_t grows{};
    explicit Host(r::Config c):cfg(c){}
    void initialized(const r::Config& c){ initialization_addresses.push_back(c.sentinel_va); }
    void grow(a::Pool& pool,std::uint32_t count) {
        assert(count==1u);
        ++grows;
        pool.native.capacity_08+=count;
        pool.slots.resize(pool.native.capacity_08);
    }
    std::uint16_t next_free(const a::Pool& pool,std::uint16_t index) {
        return r::select(cfg,pool.slots,index,state,*this).word_at_02;
    }
};
int main() {
    static_assert(r::kScene.function_va==0x221a4d90u);
    static_assert(r::kModelSounds.function_va==0x2213e990u);
    static_assert(r::kScene.sentinel_flag_va==r::kScene.sentinel_va+8u);
    for (const auto cfg : {r::kScene,r::kModelSounds}) {
        std::vector<t::NativeSlot32> slots(2);
        slots[0].right_02=1;
        slots[1].right_02=r::kMissing;
        Host h(cfg);
        h.state.flags_08=2u;
        auto node = r::select(cfg,slots,0,h.state,h);
        assert(!node.sentinel && node.index==0 && node.word_at_02==1);
        assert(h.state.flags_08==3u && h.state.word_00==r::kMissing);
        assert(h.state.word_02==r::kMissing && h.state.word_04==r::kMissing && h.state.word_06==1u);
        assert(h.initialization_addresses.size()==1 && h.initialization_addresses.front()==cfg.sentinel_va);
        auto missing=r::select(cfg,slots,r::kMissing,h.state,h);
        assert(missing.sentinel && missing.word_at_02==r::kMissing);
        assert(h.initialization_addresses.size()==1);
        auto second=r::select(cfg,slots,1,h.state,h);
        assert(!second.sentinel && second.word_at_02==r::kMissing);
        bool bounds=false;
        try { (void)r::select(cfg,slots,2,h.state,h); }
        catch(const std::out_of_range&) { bounds=true; }
        assert(bounds);
        // Phase194 uses original [node+2] WORD to advance free head.
        a::Pool pool;
        pool.slots=slots;
        pool.native.capacity_08=2u;
        pool.native.next_unused_16=2u;
        pool.native.free_head_14=0u;
        h.state={};h.initialization_addresses.clear();
        assert(a::claim(pool,h)==0u);
        assert(pool.native.free_head_14==1u);
        assert(h.initialization_addresses.size()==1u);
        assert(a::claim(pool,h)==1u);
        assert(pool.native.free_head_14==r::kMissing);
        assert(h.initialization_addresses.size()==1u);
        assert(h.grows==0u);
        // Fresh allocation skips resolver; grows when capacity is reached.
        assert(a::claim(pool,h)==2u);
        assert(h.grows==1u);
        h.state.flags_08=0u;h.state.word_00=0u;
        missing=r::select(cfg,slots,r::kMissing,h.state,h);
        assert(missing.sentinel && h.state.word_00==r::kMissing);
        assert(h.initialization_addresses.size()==2u);
    }
    return 0;
}
