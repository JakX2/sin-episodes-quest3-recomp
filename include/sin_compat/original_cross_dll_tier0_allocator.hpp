#pragma once
// Phase205: one host-neutral C++17 contract for six ORIGINAL server/client
// allocator helper pairs, backed by individually SHA-pinned 2006 PE32 x86.
// Native register arguments and 32-bit pointer tokens are modeled explicitly.
// This is NOT a live Source tier0 IMemAlloc vtable or game DLL entrypoint.
#include <cstdint>
#include <type_traits>
#include "sin_compat/original_client_debug_owner.hpp"
#include "sin_compat/original_server_allocator_dispatch.hpp"

namespace sin_compat::original_cross_dll_tier0_allocator {
namespace client = original_client_debug_owner;
namespace server = original_server_allocator_dispatch;

struct Module {
    std::uint32_t original_context_va;
    std::uint32_t original_register_alloc_va;
    std::uint32_t original_register_realloc_va;
    std::uint32_t original_stack_alloc_va;
    std::uint32_t original_free_va;
    std::uint32_t original_stack_realloc_va;
    std::uint32_t tier0_memalloc_iat_va;
    std::uint32_t primary_context_va;
    std::uint32_t primary_flag_va;
    std::uint32_t secondary_context_va;
    std::uint32_t secondary_flag_va;
};
inline constexpr Module kServer{
    0x2226e290u,0x2226e2f0u,0x2226e340u,0x2226e390u,0x2226e3e0u,
    0x2226e400u,0x223d2124u,0x225885dcu,0x225885e0u,0x225885e4u,0x225885e8u};
inline constexpr Module kClient{
    0x2416f7f0u,0x2416f850u,0x2416f8a0u,0x2416f8f0u,0x2416f940u,
    0x2416f960u,0x242761ccu,0x24372908u,0x2437290cu,0x24372910u,0x24372914u};

static_assert(kClient.original_register_alloc_va==client::kSizeRegisterBridgeVA);
static_assert(kClient.original_register_realloc_va==client::kPairRegisterBridgeVA);
static_assert(kClient.original_context_va==client::kResolverVA);
static_assert(kClient.original_stack_alloc_va==client::kStackSizeBridgeVA);
static_assert(kClient.tier0_memalloc_iat_va==client::kMemAllocIAT);
static_assert(kServer.original_context_va==server::kCreateContextVA);
static_assert(kServer.original_stack_alloc_va==server::kAllocateVA);
static_assert(kServer.original_free_va==server::kDeallocateVA);
static_assert(kServer.original_stack_realloc_va==server::kReallocateVA);
static_assert(kServer.tier0_memalloc_iat_va==server::kAllocatorGlobalSlotVA);

// Each client/server module owns INDEPENDENT process-global static caches
// despite the paired machine-code being byte-identical after normalizing
// image-relative globals. The register wrappers and stack wrappers in one
// DLL share their primary/secondary contexts; never use this host struct to
// merge server and client global memory.
struct ModuleState {
    server::NativeLazyContexts32 original{};
};
static_assert(std::is_standard_layout<ModuleState>::value);
static_assert(sizeof(ModuleState)==16u);
static_assert(sizeof(client::CacheState32)==8u);
static_assert(sizeof(server::NativeLazyContexts32)==16u);

// Phase186's CacheState32 presents flags first, then owner; native layout
// stores owner first, then flags. Adapt by VALUE around each call and write
// back to the SINGLE native 16-byte state. This is essential: original
// register and stack entrypoints address the SAME global flag/owner slots.
template<class Host>
std::uintptr_t allocate_from_esi(ModuleState& state,Host& host,
                                 std::uint32_t esi_size) {
    client::CacheState32 view{state.original.alloc_flags_04,
                              state.original.alloc_context_00};
    const auto result=client::allocate_single(host,view,esi_size);
    state.original.alloc_flags_04=view.flags;
    state.original.alloc_context_00=view.filename;
    return result;
}
template<class Host>
std::uintptr_t dispatch_from_edi_esi(ModuleState& state,Host& host,
                                     std::uint32_t edi_value,
                                     std::uint32_t esi_value) {
    client::CacheState32 view{state.original.realloc_flags_0c,
                              state.original.realloc_context_08};
    const auto result=client::dispatch_pair(host,view,edi_value,esi_value);
    state.original.realloc_flags_0c=view.flags;
    state.original.realloc_context_08=view.filename;
    return result;
}

// Original 79-byte and 90-byte stack wrappers in both DLLs share the
// same verified code; the Phase202 model already captures their context
// caching, allocator method selection and 32-bit argument sequence.
template<class Host>
std::uint32_t allocate_from_stack(ModuleState& state,Host& host,
                                  std::uint32_t bytes) {
    return server::allocate(state.original,host,bytes);
}
template<class Host>
std::uint32_t dispatch_stack_reallocation(ModuleState& state,Host& host,
                                          std::uint32_t old_token,
                                          std::uint32_t requested_bytes) {
    return server::reallocate(state.original,host,old_token,requested_bytes);
}
// The 18-byte free wrapper in client and server uses the imported allocator
// interface vtable +0x14 directly, without a cached context.
template<class Host>
void deallocate_from_stack(Host& host,std::uint32_t token) {
    server::deallocate(host,token);
}
} // namespace sin_compat::original_cross_dll_tier0_allocator
