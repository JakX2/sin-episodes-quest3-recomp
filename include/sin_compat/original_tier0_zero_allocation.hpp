#pragma once
// Phase206: original SiN 2006 server/client zero-fill allocator and opaque
// tier0 vtable dispatches. Original x86 function bodies are SHA verified;
// this is independent C++17 using synthetic host allocation tokens, not
// native PE32 calling-convention glue or a retail game-engine implementation.
#include <cstdint>
#include <limits>
#include <stdexcept>
#include "sin_compat/original_cross_dll_tier0_allocator.hpp"

namespace sin_compat::original_tier0_zero_allocation {
namespace cross = original_cross_dll_tier0_allocator;

struct Module {
    std::uint32_t original_zero_alloc_va;
    std::uint32_t original_vtable58_tailjump_va;
    std::uint32_t original_vtable20_bridge_va;
    std::uint32_t original_owner_initializer_va;
    std::uint32_t original_tier0_global_iat_va;
};
inline constexpr Module kServer{
    0x2226e460u, 0x2226e5a0u, 0x2226e5c0u, 0x2226e290u,
    0x223d2124u};
inline constexpr Module kClient{
    0x2416f9c0u, 0x2416fb00u, 0x2416fb20u, 0x2416f7f0u,
    0x242761ccu};

static_assert(kServer.original_owner_initializer_va==cross::kServer.original_context_va);
static_assert(kClient.original_owner_initializer_va==cross::kClient.original_context_va);
static_assert(kServer.original_tier0_global_iat_va==cross::kServer.tier0_memalloc_iat_va);
static_assert(kClient.original_tier0_global_iat_va==cross::kClient.tier0_memalloc_iat_va);

// Original code computes low32 of (count * element_size) in ESI (IMUL),
// gets storage through the same cache and +0x04/+0x00 virtual allocator
// methods as the regular stack allocator, and clears exactly ESI bytes
// using REP STOSD (count >> 2) then REP STOSB (count & 3).
// The original routine DOES NOT check allocation failure or 32-bit overflow:
// a null allocation with positive size can access invalid memory.
// Host-only checks below reject overflow / a null positive-size token;
// they do not purport to reproduce native failure/corruption behavior.
// Host must provide the existing Phase205 allocator callbacks PLUS
//   void zero_bytes(uint32_t opaque_pointer_token, uint32_t bytes);
// zero_bytes must only touch bytes owned by that allocation in the HOST.
template<class Host>
std::uint32_t allocate_zeroed(cross::ModuleState& module,
                             Host& host,
                             std::uint32_t element_count,
                             std::uint32_t element_size) {
    const std::uint64_t requested =
        static_cast<std::uint64_t>(element_count) * element_size;
    if (requested > std::numeric_limits<std::uint32_t>::max())
        throw std::overflow_error("host-only original zero allocator size overflow");
    const auto bytes=static_cast<std::uint32_t>(requested);
    const auto token=cross::allocate_from_stack(module,host,bytes);
    if (token==0u && bytes!=0u)
        throw std::runtime_error("host-only original zero allocator null pointer");
    // Native executes two zero-length REP loops with zero bytes; no writes.
    if (bytes!=0u) host.zero_bytes(token,bytes);
    return token;
}

// The original 12-byte function is ONLY an unconditional tail jump into
// the *imported* tier0 allocator object's virtual slot +0x58. It does not
// load an explicit argument. Do not infer a semantic name or reinterpret
// the target vtable as a SourceVR or ARM64 allocator implementation.
template<class Host>
decltype(auto) dispatch_opaque_vtable_58(Host& host) {
    return host.dispatch_opaque_vtable_58();
}

// The original 18-byte function reads its first stack DWORD, pushes it,
// calls the imported tier0 allocator object's virtual slot +0x20 and RETs.
// Its precise externally implemented method meaning remains unknown.
template<class Host>
decltype(auto) dispatch_opaque_vtable_20(Host& host,std::uint32_t arg) {
    return host.dispatch_opaque_vtable_20(arg);
}

} // namespace sin_compat::original_tier0_zero_allocation
