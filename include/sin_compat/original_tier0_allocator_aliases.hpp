#pragma once
// Phase207: original 2006 SiN server.dll/client.dll allocator entrypoint
// aliases and a two-argument opaque tier0 vtable (+0x1C) dispatch.
// This host-neutral reference shares established Phase205/206 state.
// It is NOT a drop-in tier0.dll allocator ABI or game engine replacement.
#include <cstdint>
#include "sin_compat/original_cross_dll_tier0_allocator.hpp"
#include "sin_compat/original_tier0_zero_allocation.hpp"

namespace sin_compat::original_tier0_allocator_aliases {
namespace cross = original_cross_dll_tier0_allocator;
namespace zero = original_tier0_zero_allocation;

struct OriginalEntrypoints {
    std::uint32_t repeated_vtable20_va;
    std::uint32_t alloc_alias_a_va;
    std::uint32_t alloc_alias_b_va;
    std::uint32_t two_arg_vtable1c_va;
    std::uint32_t original_owner_resolver_va;
    std::uint32_t tier0_allocator_iat_va;
};
inline constexpr OriginalEntrypoints kServer{
    0x2226e5e0u,0x2226e600u,0x2226e650u,0x2226e6a0u,
    0x2226e290u,0x223d2124u};
inline constexpr OriginalEntrypoints kClient{
    0x2416fb40u,0x2416fb60u,0x2416fbb0u,0x2416fc00u,
    0x2416f7f0u,0x242761ccu};

static_assert(kServer.original_owner_resolver_va==cross::kServer.original_context_va);
static_assert(kClient.original_owner_resolver_va==cross::kClient.original_context_va);
static_assert(kServer.tier0_allocator_iat_va==cross::kServer.tier0_memalloc_iat_va);
static_assert(kClient.tier0_allocator_iat_va==cross::kClient.tier0_memalloc_iat_va);

// Both 79-byte alias entrypoints reproduce the same original function
// behavior as the previously recovered Phase202/205 stack-size allocator:
// check bit0 of the module's ORIGINAL primary lazy state, OR bit0 before
// resolver call, and dispatch through imported allocator vtable +0x04
// (plain) or +0x00 (contextual). Alias A and B must not create extra state.
template<class Host>
std::uint32_t allocate_alias_a(cross::ModuleState& state,
                               Host& host,std::uint32_t bytes) {
    return cross::allocate_from_stack(state,host,bytes);
}
template<class Host>
std::uint32_t allocate_alias_b(cross::ModuleState& state,
                               Host& host,std::uint32_t bytes) {
    return cross::allocate_from_stack(state,host,bytes);
}

// Both 18-byte native duplicate wrappers load tier0.dll!g_pMemAlloc,
// push their first stack argument and CALL the imported allocator object's
// vtable method at byte offset +0x20. The class method name is unknown.
template<class Host>
decltype(auto) dispatch_repeated_opaque_vtable20(Host& host,
                                                 std::uint32_t arg) {
    return zero::dispatch_opaque_vtable_20(host,arg);
}

// The paired 23-byte x86 wrapper loads [original ESP+8] (second argument),
// pushes it; then reads [modified ESP+8] (original first argument) and
// pushes it. The virtual method at byte offset +0x1C therefore receives
// two original 32-bit arguments IN ORIGINAL SOURCE ORDER. The vtable
// method identity/meaning and calling convention remain unverified.
template<class Host>
decltype(auto) dispatch_opaque_vtable1c(Host& host,
                                       std::uint32_t first,
                                       std::uint32_t second) {
    return host.dispatch_opaque_vtable_1c(first,second);
}
} // namespace sin_compat::original_tier0_allocator_aliases
