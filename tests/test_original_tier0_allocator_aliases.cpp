#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <vector>
#include <algorithm>
#include "sin_compat/original_tier0_allocator_aliases.hpp"

namespace aliases=sin_compat::original_tier0_allocator_aliases;
namespace cross=sin_compat::original_cross_dll_tier0_allocator;

struct SyntheticTier0 {
    bool enable_owner{};
    std::uint32_t context=0xa55au;
    std::uint32_t next=0x1000u;
    std::uint32_t enabled_checks{},context_calls{};
    std::uint32_t basic_calls{},tagged_calls{};
    std::uint32_t opaque20_calls{},opaque1c_calls{};
    std::uint32_t last20{},last1c_first{},last1c_second{};
    std::map<std::uint32_t,std::uint32_t> live;
    std::uint32_t reserve(std::uint32_t n) {
        next+=0x1000u;
        live[next]=n;
        return next;
    }
    bool metadata_enabled() {
        ++enabled_checks;
        return enable_owner;
    }
    std::uint32_t make_metadata() {
        ++context_calls;
        return context;
    }
    std::uint32_t allocate_basic(std::uint32_t n) {
        ++basic_calls;return reserve(n);
    }
    std::uint32_t allocate_tagged(std::uint32_t n,std::uint32_t owner,
                                  std::uint32_t zero) {
        assert(owner==context && zero==0u);
        ++tagged_calls;return reserve(n);
    }
    void deallocate_direct(std::uint32_t p) {
        assert(live.erase(p)==1u);
    }
    std::uint32_t dispatch_opaque_vtable_20(std::uint32_t first) {
        ++opaque20_calls;
        last20=first;
        return first^0x55aa55aau;
    }
    std::uint32_t dispatch_opaque_vtable_1c(std::uint32_t first,
                                           std::uint32_t second) {
        ++opaque1c_calls;
        last1c_first=first;
        last1c_second=second;
        return (first<<4u)^second;
    }
};

static void each_original_module(const aliases::OriginalEntrypoints entries) {
    assert(entries.alloc_alias_a_va!=entries.alloc_alias_b_va);
    assert(entries.repeated_vtable20_va!=entries.two_arg_vtable1c_va);
    SyntheticTier0 host;
    cross::ModuleState state{};
    // Set unrelated flag bits; original lazy setup ORs bit0, doesn't reset.
    state.original.alloc_flags_04=0x80u;
    const auto a=aliases::allocate_alias_a(state,host,7u);
    const auto b=aliases::allocate_alias_b(state,host,11u);
    const auto c=cross::allocate_from_stack(state,host,13u);
    assert(a!=b && b!=c && a!=c);
    assert(host.basic_calls==3u && host.tagged_calls==0u);
    assert(host.enabled_checks==1u && host.context_calls==0u);
    assert(state.original.alloc_context_00==0u);
    assert(state.original.alloc_flags_04==0x81u);
    host.enable_owner=true;
    // A null context is CACHED, not recalculated when feature toggles.
    const auto d=aliases::allocate_alias_b(state,host,17u);
    assert(host.basic_calls==4u && host.enabled_checks==1u);
    assert(state.original.alloc_context_00==0u);
    assert(host.live.at(d)==17u);
    for(auto token:{a,b,c,d})host.deallocate_direct(token);
    assert(host.live.empty());

    const auto first=aliases::dispatch_repeated_opaque_vtable20(host,0x12345678u);
    assert(first==(0x12345678u^0x55aa55aau));
    assert(host.last20==0x12345678u && host.opaque20_calls==1u);
    const auto pair=aliases::dispatch_opaque_vtable1c(host,0x10203040u,0xdeadbeefu);
    assert(pair==((0x10203040u<<4u)^0xdeadbeefu));
    assert(host.last1c_first==0x10203040u);
    assert(host.last1c_second==0xdeadbeefu);
    assert(host.opaque1c_calls==1u);
    // Some binaries contain similar wrappers; do not invent extra meaning.
    assert(state.original.realloc_flags_0c==0u);
}

static void tagged_owner_and_cross_entrypoint_cache() {
    for(const auto entries:{aliases::kServer,aliases::kClient}) {
        (void)entries;
        SyntheticTier0 host;
        host.enable_owner=true;
        host.context=0xbeefu;
        cross::ModuleState state{};
        const auto a=aliases::allocate_alias_b(state,host,1u);
        const auto b=aliases::allocate_alias_a(state,host,2u);
        assert(state.original.alloc_context_00==0xbeefu);
        assert(state.original.alloc_flags_04==1u);
        assert(host.enabled_checks==1u);
        assert(host.context_calls==1u);
        assert(host.basic_calls==0u && host.tagged_calls==2u);
        host.deallocate_direct(a);
        host.deallocate_direct(b);
        assert(host.live.empty());
    }
}
static void separate_module_global_states() {
    cross::ModuleState server{},client{};
    SyntheticTier0 a,b;
    a.context=0xa55au;
    b.context=0x7711u;
    a.enable_owner=true;
    b.enable_owner=false;
    const auto sa=aliases::allocate_alias_a(server,a,23u);
    const auto cb=aliases::allocate_alias_b(client,b,29u);
    assert(server.original.alloc_context_00==a.context);
    assert(client.original.alloc_context_00==0u);
    assert(a.context_calls==1u && b.context_calls==0u);
    assert(a.tagged_calls==1u && b.basic_calls==1u);
    a.deallocate_direct(sa);
    b.deallocate_direct(cb);
}
int main() {
    static_assert(aliases::kServer.alloc_alias_a_va==0x2226e600u);
    static_assert(aliases::kServer.alloc_alias_b_va==0x2226e650u);
    static_assert(aliases::kClient.alloc_alias_a_va==0x2416fb60u);
    static_assert(aliases::kClient.alloc_alias_b_va==0x2416fbb0u);
    static_assert(aliases::kServer.repeated_vtable20_va==0x2226e5e0u);
    static_assert(aliases::kClient.repeated_vtable20_va==0x2416fb40u);
    static_assert(aliases::kServer.two_arg_vtable1c_va==0x2226e6a0u);
    static_assert(aliases::kClient.two_arg_vtable1c_va==0x2416fc00u);
    static_assert(sizeof(cross::ModuleState)==16u);
    each_original_module(aliases::kServer);
    each_original_module(aliases::kClient);
    tagged_owner_and_cross_entrypoint_cache();
    separate_module_global_states();
}
