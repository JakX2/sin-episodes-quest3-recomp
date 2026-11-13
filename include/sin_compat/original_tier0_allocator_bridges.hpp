#pragma once
// Phase208: original 2006 SiN PE32 tier0 wrapper contracts.
// Each entrypoint is complete-SHA reviewed in the two original game DLLs.
// No original game code or tier0 implementation is shipped here.
// Host callbacks represent UNVERIFIED external virtual method implementations.
#include <cstdint>
#include "sin_compat/original_tier0_allocator_aliases.hpp"
#include "sin_compat/original_client_memalloc_bridge.hpp"

namespace sin_compat::original_tier0_allocator_bridges {
namespace aliases = original_tier0_allocator_aliases;
namespace cross = original_cross_dll_tier0_allocator;
namespace client_old = original_client_memalloc_bridge;

struct OriginalModuleEntrypoints {
    std::uint32_t slot00_three_va;
    std::uint32_t slot10_three_va;
    std::uint32_t slot08_four_va;
    std::uint32_t cached_alloc_alias_a_va;
    std::uint32_t slot00_skip_second_va;
    std::uint32_t duplicate_free_va;
    std::uint32_t cached_alloc_alias_b_va;
    std::uint32_t original_tier0_iat_va;
};

inline constexpr OriginalModuleEntrypoints kServer{
    0x2226e720u,0x2226e740u,0x2226e760u,0x2226e790u,
    0x2226e7e0u,0x2226e800u,0x2226e820u,0x223d2124u};
inline constexpr OriginalModuleEntrypoints kClient{
    0x2416fc80u,0x2416fca0u,0x2416fcc0u,0x2416fcf0u,
    0x2416fd40u,0x2416fd60u,0x2416fd80u,0x242761ccu};

static_assert(kServer.original_tier0_iat_va==cross::kServer.tier0_memalloc_iat_va);
static_assert(kClient.original_tier0_iat_va==cross::kClient.tier0_memalloc_iat_va);
static_assert(kClient.cached_alloc_alias_a_va==client_old::kAllocBridgeA);
static_assert(kClient.cached_alloc_alias_b_va==client_old::kAllocBridgeB);
static_assert(kClient.slot00_skip_second_va==client_old::kDirectDebugAllocA);
static_assert(kClient.duplicate_free_va==client_old::kFreeBridgeA);

// Native stack order: load the original final stack argument, PUSH; then
// load/push the prior one(s). Arguments reach imported allocator vtable in
// source-order. +0x00 is known to act as debug allocation in Phase185;
// +0x10 method meaning is NOT established by these PE32 wrappers.
template<class Host>
decltype(auto) dispatch_three_slot00(Host& host,
                                     std::uint32_t first,
                                     std::uint32_t second,
                                     std::uint32_t third) {
    return host.dispatch_opaque_slot00(first,second,third);
}
template<class Host>
decltype(auto) dispatch_three_slot10(Host& host,
                                     std::uint32_t first,
                                     std::uint32_t second,
                                     std::uint32_t third) {
    return host.dispatch_opaque_slot10(first,second,third);
}
template<class Host>
decltype(auto) dispatch_four_slot08(Host& host,
                                    std::uint32_t first,
                                    std::uint32_t second,
                                    std::uint32_t third,
                                    std::uint32_t fourth) {
    return host.dispatch_opaque_slot08(first,second,third,fourth);
}

// Observed native stack reads differ from the straight three-arg case:
// first push accesses [ESP+0x10] (original FOURTH),
// second [ESP+0x10] after one push (original THIRD),
// third [ESP+0x0c] after two pushes (original FIRST).
// The original SECOND stack argument is deliberately not forwarded.
// This matches Phase185 client 0x2416FD40's prior function evidence.
template<class Host>
decltype(auto) dispatch_skip_second_slot00(Host& host,
                                            std::uint32_t first,
                                            std::uint32_t /*unforwarded_second*/,
                                            std::uint32_t third,
                                            std::uint32_t fourth) {
    return host.dispatch_opaque_slot00(first,third,fourth);
}

// Two identical per-module lazy primary owner/context allocation wrapper
// entrypoints. They do not own separate cache structures. Client versions
// were already independently verified in Phase185; server mirrors are new.
template<class Host>
std::uint32_t allocate_cached_alias_a(cross::ModuleState& state, Host& host,
                                      std::uint32_t bytes) {
    return cross::allocate_from_stack(state,host,bytes);
}
template<class Host>
std::uint32_t allocate_cached_alias_b(cross::ModuleState& state, Host& host,
                                      std::uint32_t bytes) {
    return cross::allocate_from_stack(state,host,bytes);
}

// An exact duplicate of the original Phase202/205 +0x14 tier0 free wrapper,
// with a NEW server VA and a previously reviewed Phase185 client VA.
// There is no null guard or owner-context access in original native x86.
template<class Host>
void dispatch_duplicate_free(Host& host,std::uint32_t token) {
    cross::deallocate_from_stack(host,token);
}
} // namespace sin_compat::original_tier0_allocator_bridges
