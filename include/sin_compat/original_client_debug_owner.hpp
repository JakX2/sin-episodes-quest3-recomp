#pragma once
// Phase186, independently reconstructed 2006 SiN client.dll debug-owner
// resolver and three adjacent tier0 dispatch entrypoints. Not Source ABI.
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace sin_compat::original_client_debug_owner {

inline constexpr std::uint32_t kResolverVA = 0x2416F7F0u;
inline constexpr std::uint32_t kSizeRegisterBridgeVA = 0x2416F850u;
inline constexpr std::uint32_t kPairRegisterBridgeVA = 0x2416F8A0u;
inline constexpr std::uint32_t kStackSizeBridgeVA = 0x2416F8F0u;
inline constexpr std::uint32_t kMemAllocIAT = 0x242761CCu;
inline constexpr std::uint32_t kVirtualQueryAnchor = 0x24372904u;
inline constexpr std::uint32_t kPrimaryCachedFilenameVA = 0x24372908u;
inline constexpr std::uint32_t kPrimaryFlagsVA = 0x2437290Cu;
inline constexpr std::uint32_t kSecondaryCachedFilenameVA = 0x24372910u;
inline constexpr std::uint32_t kSecondaryFlagsVA = 0x24372914u;
inline constexpr std::uint32_t kMemAllocProbeSlotBytes = 0x5Cu;
inline constexpr std::uint32_t kModuleFilenameCapacity = 0x104u;
inline constexpr std::uint32_t kMemoryBasicInfoBytes = 0x1Cu;

// The original x86 VirtualQuery(..., MEMORY_BASIC_INFORMATION*, 28) writes
// AllocationBase to the second DWORD. The resolver passes that value as the
// HMODULE to GetModuleFileNameA; it does not use BaseAddress or the anchor.
struct MemoryBasicInformation32 {
    std::uint32_t base_address = 0;
    std::uint32_t allocation_base = 0;
    std::uint32_t allocation_protect = 0;
    std::uint32_t region_size = 0;
    std::uint32_t state = 0;
    std::uint32_t protect = 0;
    std::uint32_t type = 0;
};
static_assert(sizeof(MemoryBasicInformation32) == 28, "original Win32 MBI is 28 bytes");
static_assert(offsetof(MemoryBasicInformation32, allocation_base) == 4, "AllocationBase offset");
static_assert(std::is_standard_layout<MemoryBasicInformation32>::value, "plain x86 ABI record");

// Host requirements:
// bool allocator_probe_5c();
// uintptr_t get_process_heap();
// uintptr_t heap_alloc(uintptr_t heap, uint32_t flags, uint32_t bytes);
// void virtual_query(uint32_t anchor, MemoryBasicInformation32& output, uint32_t bytes);
// void get_module_filename(uint32_t allocation_base, uintptr_t out, uint32_t bytes);
// Native disregards the return codes of HeapAlloc, VirtualQuery and
// GetModuleFileNameA, and returns HeapAlloc's pointer even if other APIs fail.
// The test host supplies a deterministic MBI; actual Windows VirtualQuery
// failure leaves original stack data undefined and is not modeled as success.
template<class Host>
std::uintptr_t resolve_filename_owner(Host& host) {
    if (!host.allocator_probe_5c()) return 0;
    const std::uintptr_t heap = host.get_process_heap();
    const std::uintptr_t buf = host.heap_alloc(heap, 0, kModuleFilenameCapacity);
    MemoryBasicInformation32 mbi{};
    host.virtual_query(kVirtualQueryAnchor, mbi, kMemoryBasicInfoBytes);
    host.get_module_filename(mbi.allocation_base, buf, kModuleFilenameCapacity);
    return buf;
}

struct CacheState32 {
    std::uint32_t flags = 0;
    std::uint32_t filename = 0;
};
static_assert(sizeof(CacheState32) == 8, "two DWORDs of original client.dll cached state");

// Native wrappers OR bit0 into their original global flags *before* invoking
// the resolver. Even a zero resolver result is cached and not recomputed.
// This single-host model does not duplicate the original process-global
// memory locations: the caller owns each distinct cache record.
template<class Host>
std::uint32_t cached_owner(Host& host, CacheState32& cache) {
    if ((cache.flags & 1u) == 0) {
        cache.flags |= 1u;
        cache.filename = static_cast<std::uint32_t>(host.resolve_filename_owner());
    }
    return cache.filename;
}

// Original 0x2416F850 expects the requested size in ESI. Original 0x2416F8F0
// reads it from the first x86 stack slot. Both share the same flag and
// cached-filename DWORDs, and dispatch to original tier0 +4 or +0.
template<class Host>
std::uintptr_t allocate_single(Host& host, CacheState32& primary,
                               std::uint32_t size) {
    const std::uint32_t owner = cached_owner(host, primary);
    if (!owner) return host.allocate_plain(size);  // IMemAlloc +0x04
    return host.allocate_with_owner(size, owner, 0);  // IMemAlloc +0x00
}

// Original 0x2416F8A0 expects EDI and ESI as the two raw values. It uses a
// SECOND independent cache at 0x24372910/14, and forwards the two values to
// IMemAlloc +0x0C or the (owner,0)-augmented variant at +0x08. We do not
// assign an unverified high-level name/signature (e.g., realloc).
template<class Host>
std::uintptr_t dispatch_pair(Host& host, CacheState32& secondary,
                             std::uint32_t edi_value, std::uint32_t esi_value) {
    const std::uint32_t owner = cached_owner(host, secondary);
    if (!owner) return host.pair_plain(edi_value, esi_value); // +0x0C
    return host.pair_with_owner(edi_value, esi_value, owner, 0); // +0x08
}

} // namespace sin_compat::original_client_debug_owner