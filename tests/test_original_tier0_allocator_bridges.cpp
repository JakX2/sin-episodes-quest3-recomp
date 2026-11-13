#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstdint>
#include <map>
#include <vector>
#include "sin_compat/original_tier0_allocator_bridges.hpp"
namespace bridge = sin_compat::original_tier0_allocator_bridges;
namespace cross = sin_compat::original_cross_dll_tier0_allocator;

struct Host {
    bool enabled{};
    std::uint32_t owner=0xa55au;
    std::uint32_t next=0x1000u;
    int enabled_calls=0,metadata_calls=0,basic_calls=0,tagged_calls=0;
    std::map<std::uint32_t,std::uint32_t> live{};
    std::vector<std::uint32_t> releases{};
    std::uint32_t virtual_slot=0xffffffffu;
    std::vector<std::uint32_t> virtual_args{};

    bool metadata_enabled() {
        ++enabled_calls;return enabled;
    }
    std::uint32_t make_metadata() {
        ++metadata_calls;return owner;
    }
    std::uint32_t allocate_basic(std::uint32_t bytes) {
        ++basic_calls;
        next+=0x1000u;
        live[next]=bytes;
        return next;
    }
    std::uint32_t allocate_tagged(std::uint32_t bytes,std::uint32_t context,
                                  std::uint32_t line) {
        assert(context==owner && line==0u);
        ++tagged_calls;
        next+=0x1000u;
        live[next]=bytes;
        return next;
    }
    void deallocate_direct(std::uint32_t token) {
        releases.push_back(token);
        if(token!=0u)assert(live.erase(token)==1u);
    }
    std::uint32_t dispatch_opaque_slot00(std::uint32_t a,std::uint32_t b,
                                          std::uint32_t c) {
        virtual_slot=0x00u;
        virtual_args={a,b,c};
        return 0xc0000000u ^ a ^ b ^ c;
    }
    std::uint32_t dispatch_opaque_slot10(std::uint32_t a,std::uint32_t b,
                                          std::uint32_t c) {
        virtual_slot=0x10u;
        virtual_args={a,b,c};
        return 0x10000000u ^ a ^ b ^ c;
    }
    std::uint32_t dispatch_opaque_slot08(std::uint32_t a,std::uint32_t b,
                                          std::uint32_t c,std::uint32_t d) {
        virtual_slot=0x08u;
        virtual_args={a,b,c,d};
        return 0x08000000u ^ a ^ b ^ c ^ d;
    }
};

static void test_virtual_stack_argument_order(const bridge::OriginalModuleEntrypoints m) {
    assert(m.original_tier0_iat_va!=0u);
    Host h;
    const auto a=0x11223344u,b=0x99887766u,c=0xace0f135u,d=0x4f102030u;
    auto result=bridge::dispatch_three_slot00(h,a,b,c);
    assert(h.virtual_slot==0x00u && (h.virtual_args==std::vector<std::uint32_t>{a,b,c}));
    assert(result==(0xc0000000u^a^b^c));
    result=bridge::dispatch_three_slot10(h,a,b,c);
    assert(h.virtual_slot==0x10u && (h.virtual_args==std::vector<std::uint32_t>{a,b,c}));
    assert(result==(0x10000000u^a^b^c));
    result=bridge::dispatch_four_slot08(h,a,b,c,d);
    assert(h.virtual_slot==0x08u && (h.virtual_args==std::vector<std::uint32_t>{a,b,c,d}));
    assert(result==(0x08000000u^a^b^c^d));
    // Observed Phase185 client method has a four-DWORD caller frame but
    // only forwards the first, third and fourth original DWORDs.
    result=bridge::dispatch_skip_second_slot00(h,a,b,c,d);
    assert(h.virtual_slot==0x00u && (h.virtual_args==std::vector<std::uint32_t>{a,c,d}));
    assert(result==(0xc0000000u^a^c^d));
    // Distinct second arg must not leak through to the virtual invocation.
    result=bridge::dispatch_skip_second_slot00(h,a,0x7fffffffu,c,d);
    assert(h.virtual_args[1]==c && result==(0xc0000000u^a^c^d));
}
static void test_intra_module_aliases_and_cached_zero() {
    for(const auto m:{bridge::kServer,bridge::kClient}) {
        (void)m;
        Host h;
        cross::ModuleState state{};
        state.original.alloc_flags_04=0x20u; // other flags preserved
        const auto first=bridge::allocate_cached_alias_a(state,h,24u);
        const auto second=bridge::allocate_cached_alias_b(state,h,48u);
        const auto third=cross::allocate_from_stack(state,h,96u);
        assert(first!=second && second!=third);
        assert(h.enabled_calls==1 && h.metadata_calls==0);
        assert(h.basic_calls==3 && h.tagged_calls==0);
        assert(state.original.alloc_flags_04==0x21u && state.original.alloc_context_00==0u);
        h.enabled=true; // original lazy context has cached FAILURE, not retried
        const auto fourth=bridge::allocate_cached_alias_a(state,h,8u);
        assert(h.enabled_calls==1 && h.metadata_calls==0);
        assert(h.basic_calls==4 && h.tagged_calls==0);
        for(const auto p:{first,second,third,fourth})bridge::dispatch_duplicate_free(h,p);
        bridge::dispatch_duplicate_free(h,0u); // original no-null-guard forwarder
        assert(h.live.empty() && h.releases.size()==5u && h.releases.back()==0u);
    }
}
static void test_contextual_alloc_and_separate_modules() {
    cross::ModuleState server{},client{};
    Host a,b;
    a.enabled=true;b.enabled=false;
    a.owner=0xa100u;b.owner=0xb200u;
    const auto sa=bridge::allocate_cached_alias_b(server,a,13u);
    const auto sb=bridge::allocate_cached_alias_a(server,a,15u);
    const auto ca=bridge::allocate_cached_alias_b(client,b,19u);
    assert(a.enabled_calls==1 && a.metadata_calls==1);
    assert(a.tagged_calls==2 && a.basic_calls==0);
    assert(server.original.alloc_context_00==a.owner);
    assert(b.enabled_calls==1 && b.metadata_calls==0);
    assert(b.basic_calls==1 && b.tagged_calls==0);
    assert(client.original.alloc_context_00==0u);
    assert(server.original.alloc_flags_04==1u && client.original.alloc_flags_04==1u);
    bridge::dispatch_duplicate_free(a,sa);
    bridge::dispatch_duplicate_free(a,sb);
    bridge::dispatch_duplicate_free(b,ca);
    assert(a.live.empty() && b.live.empty());
}
int main() {
    static_assert(bridge::kServer.slot00_three_va==0x2226e720u);
    static_assert(bridge::kClient.slot10_three_va==0x2416fca0u);
    static_assert(bridge::kServer.slot08_four_va==0x2226e760u);
    static_assert(bridge::kClient.slot00_skip_second_va==0x2416fd40u);
    static_assert(bridge::kClient.cached_alloc_alias_b_va==0x2416fd80u);
    static_assert(sizeof(cross::ModuleState)==16u);
    test_virtual_stack_argument_order(bridge::kServer);
    test_virtual_stack_argument_order(bridge::kClient);
    test_intra_module_aliases_and_cached_zero();
    test_contextual_alloc_and_separate_modules();
}
