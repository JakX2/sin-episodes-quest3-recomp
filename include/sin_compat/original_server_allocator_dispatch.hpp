#pragma once
// Phase202: reconstructed semantics of four SHA-verified original 2006
// SiN server.dll allocator dispatch methods. NOT actual Source x86 heap ABI.
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace sin_compat::original_server_allocator_dispatch {
inline constexpr std::uint32_t kCreateContextVA = 0x2226e290u;
inline constexpr std::uint32_t kAllocateVA = 0x2226e390u;
inline constexpr std::uint32_t kDeallocateVA = 0x2226e3e0u;
inline constexpr std::uint32_t kReallocateVA = 0x2226e400u;
inline constexpr std::uint32_t kAllocatorGlobalSlotVA = 0x223d2124u;
inline constexpr std::uint32_t kGlobalFirstContextVA = 0x225885dcu;
inline constexpr std::uint32_t kGlobalFirstFlagVA = 0x225885e0u;
inline constexpr std::uint32_t kGlobalSecondContextVA = 0x225885e4u;
inline constexpr std::uint32_t kGlobalSecondFlagVA = 0x225885e8u;
inline constexpr std::uint32_t kMetadataSourceVA = 0x225885d8u;
inline constexpr std::uint32_t kNativeMetadataBufferLimit = 0x104u;
inline constexpr std::uint32_t kNativeMetadataSizeArgument = 0x1cu;

// Four consecutive original 32-bit static fields in server.dll .data.
// The context values are *native pointer tokens* in this model; never
// reinterpret these as host pointers, especially on Android ARM64.
struct NativeLazyContexts32 {
    std::uint32_t alloc_context_00{};
    std::uint32_t alloc_flags_04{};
    std::uint32_t realloc_context_08{};
    std::uint32_t realloc_flags_0c{};
};
static_assert(sizeof(NativeLazyContexts32)==16u);
static_assert(offsetof(NativeLazyContexts32,alloc_flags_04)==4u);
static_assert(offsetof(NativeLazyContexts32,realloc_context_08)==8u);
static_assert(offsetof(NativeLazyContexts32,realloc_flags_0c)==12u);
static_assert(std::is_standard_layout<NativeLazyContexts32>::value);

// The original 0x2226E290 first calls the allocator interface's virtual
// method at byte offset +0x5C. If false, it returns 0. If true it invokes
// four imported helper functions to prepare metadata, returning an opaque
// pointer. The meaning of each import remains UNVERIFIED, so host models
// them through a single explicitly opaque make_metadata() callback.
template<class Host>
std::uint32_t create_context(Host& host) {
    return host.metadata_enabled() ? host.make_metadata() : 0u;
}

// For each routine the original tests bit0 of its own static DWORD, ORs
// bit0 *before* calling create_context, and keeps the returned pointer.
// If context is 0 (including cached failure), the allocator vtable's basic
// method is called. A nonzero context routes to its tagged method with
// explicit trailing DWORD zero. No retry occurs once bit0 is set.
template<class Host>
std::uint32_t allocate(NativeLazyContexts32& state, Host& host,
                       std::uint32_t bytes) {
    if ((state.alloc_flags_04 & 1u)==0u) {
        state.alloc_flags_04 |= 1u;
        state.alloc_context_00 = create_context(host);
    }
    return state.alloc_context_00 == 0u
        ? host.allocate_basic(bytes)           // original vtable +0x04
        : host.allocate_tagged(bytes,state.alloc_context_00,0u); // +0x00
}

template<class Host>
std::uint32_t reallocate(NativeLazyContexts32& state, Host& host,
                         std::uint32_t old_pointer,std::uint32_t bytes) {
    if ((state.realloc_flags_0c & 1u)==0u) {
        state.realloc_flags_0c |= 1u;
        state.realloc_context_08 = create_context(host);
    }
    return state.realloc_context_08 == 0u
        ? host.reallocate_basic(old_pointer,bytes) // original vtable +0x0C
        : host.reallocate_tagged(old_pointer,bytes,state.realloc_context_08,0u); // +0x08
}

template<class Host>
void deallocate(Host& host,std::uint32_t pointer) {
    host.deallocate_direct(pointer); // always original vtable +0x14
}
} // namespace sin_compat::original_server_allocator_dispatch
