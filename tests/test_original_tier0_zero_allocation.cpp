#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_tier0_zero_allocation.hpp"

namespace zero=sin_compat::original_tier0_zero_allocation;
namespace cross=sin_compat::original_cross_dll_tier0_allocator;

struct FakeTier0 {
    bool allow_owner{};
    bool fail_allocation{};
    cross::ModuleState* shared{};
    std::uint32_t owner=0xa501u;
    std::uint32_t next=0x1000u;
    std::uint32_t metadata_checks{};
    std::uint32_t metadata_creations{};
    std::uint32_t basic_count{};
    std::uint32_t tagged_count{};
    std::uint32_t clear_count{};
    std::uint32_t opaque58_count{};
    std::uint32_t opaque20_count{};
    std::uint32_t last_opaque20_arg{};
    std::map<std::uint32_t,std::vector<std::uint8_t>> blocks{};

    bool metadata_enabled() {
        ++metadata_checks;
        // The original code ORs bit0 in module-global flags FIRST.
        assert(shared && (shared->original.alloc_flags_04&1u)!=0u);
        return allow_owner;
    }
    std::uint32_t make_metadata() {
        ++metadata_creations;
        return owner;
    }
    std::uint32_t allocate_basic(std::uint32_t bytes) {
        ++basic_count;
        return make_block(bytes);
    }
    std::uint32_t allocate_tagged(std::uint32_t bytes,
                                  std::uint32_t cached_owner,
                                  std::uint32_t zero_line) {
        ++tagged_count;
        assert(cached_owner==owner && zero_line==0u);
        return make_block(bytes);
    }
    std::uint32_t make_block(std::uint32_t bytes) {
        if(fail_allocation)return 0u;
        next+=0x1000u;
        blocks.emplace(next,std::vector<std::uint8_t>(bytes,0xa5u));
        return next;
    }
    void zero_bytes(std::uint32_t token,std::uint32_t bytes) {
        auto it=blocks.find(token);
        assert(it!=blocks.end() && it->second.size()==bytes);
        ++clear_count;
        std::fill(it->second.begin(),it->second.end(),std::uint8_t{0});
    }
    std::uint32_t dispatch_opaque_vtable_58() {
        ++opaque58_count;
        return 0x58005800u;
    }
    std::uint32_t dispatch_opaque_vtable_20(std::uint32_t argument) {
        ++opaque20_count;
        last_opaque20_arg=argument;
        return argument^0x20202020u;
    }
};
static void check_actual_native_module_pair(const zero::Module config) {
    assert(config.original_zero_alloc_va!=0u);
    assert(config.original_tier0_global_iat_va!=0u);
    cross::ModuleState state{};
    FakeTier0 host;
    host.shared=&state;
    // Original first request: 3*7 = 21B, 5 DWORD clears + 1 tail byte.
    auto a=zero::allocate_zeroed(state,host,3u,7u);
    assert(a!=0u && host.blocks[a].size()==21u);
    assert(std::all_of(host.blocks[a].begin(),host.blocks[a].end(),
                       [](std::uint8_t b){return b==0u;}));
    assert(host.metadata_checks==1u && host.metadata_creations==0u);
    assert(host.basic_count==1u && host.clear_count==1u);
    auto b=zero::allocate_zeroed(state,host,2u,8u);
    assert(b!=a && host.blocks[b].size()==16u);
    assert(host.metadata_checks==1u && host.basic_count==2u);
    assert(host.clear_count==2u);
    auto z=zero::allocate_zeroed(state,host,0u,42u);
    assert(z!=0u && host.blocks[z].empty());
    assert(host.basic_count==3u && host.clear_count==2u);
    // A later owner feature toggle does NOT recalculate a cached zero.
    host.allow_owner=true;
    (void)zero::allocate_zeroed(state,host,1u,1u);
    assert(host.metadata_checks==1u && host.metadata_creations==0u);
    assert(host.tagged_count==0u);
    assert(zero::dispatch_opaque_vtable_58(host)==0x58005800u);
    assert(host.opaque58_count==1u);
    assert(zero::dispatch_opaque_vtable_20(host,0x12345678u)==
           (0x12345678u^0x20202020u));
    assert(host.last_opaque20_arg==0x12345678u && host.opaque20_count==1u);

    bool rejected=false;
    const auto prior_allocations=host.basic_count+host.tagged_count;
    try {(void)zero::allocate_zeroed(state,host,0xffffffffu,2u);}
    catch(const std::overflow_error&){rejected=true;}
    assert(rejected && host.basic_count+host.tagged_count==prior_allocations);
}
static void check_contextual_and_failed_allocations() {
    for(const auto cfg:{zero::kServer,zero::kClient}) {
        (void)cfg;
        cross::ModuleState state{};
        FakeTier0 host;
        host.shared=&state;
        host.allow_owner=true;
        auto pointer=zero::allocate_zeroed(state,host,5u,5u);
        assert(pointer!=0u && host.blocks[pointer].size()==25u);
        assert(host.metadata_checks==1u && host.metadata_creations==1u);
        assert(host.tagged_count==1u && host.basic_count==0u);
        // Deliberately reject native unchecked null-pointer writes on host.
        FakeTier0 bad;
        cross::ModuleState failure_state{};
        bad.shared=&failure_state;
        bad.fail_allocation=true;
        bool rejected=false;
        try {(void)zero::allocate_zeroed(failure_state,bad,1u,8u);}
        catch(const std::runtime_error&){rejected=true;}
        assert(rejected && bad.clear_count==0u && bad.blocks.empty());
    }
}
static void check_separate_dll_globals() {
    cross::ModuleState server{},client{};
    FakeTier0 hs,hc;
    hs.shared=&server;hc.shared=&client;
    hs.allow_owner=true;hc.allow_owner=false;
    auto s=zero::allocate_zeroed(server,hs,2u,3u);
    auto c=zero::allocate_zeroed(client,hc,2u,3u);
    assert(s!=0u && c!=0u && hs.tagged_count==1u && hc.basic_count==1u);
    assert(server.original.alloc_context_00==hs.owner);
    assert(client.original.alloc_context_00==0u);
    assert(hs.metadata_checks==1u && hc.metadata_checks==1u);
}
int main() {
    static_assert(zero::kServer.original_zero_alloc_va==0x2226e460u);
    static_assert(zero::kClient.original_zero_alloc_va==0x2416f9c0u);
    static_assert(zero::kServer.original_vtable58_tailjump_va==0x2226e5a0u);
    static_assert(zero::kClient.original_vtable20_bridge_va==0x2416fb20u);
    check_actual_native_module_pair(zero::kServer);
    check_actual_native_module_pair(zero::kClient);
    check_contextual_and_failed_allocations();
    check_separate_dll_globals();
}
