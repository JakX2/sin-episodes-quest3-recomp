#pragma once
// Phase185: native 2006 SiN client.dll tier0 IMemAlloc forwarding boundary.
// Portable instruction-level dispatch model. This is NOT tier0's actual
// allocator implementation, a Source engine vtable or a drop-in DLL.
#include <cstdint>
#include <cstddef>
#include <type_traits>

namespace sin_compat::original_client_memalloc_bridge {
inline constexpr std::uint32_t kTier0MemAllocIAT = 0x242761CCu;
inline constexpr std::uint32_t kVstdlibKeyValuesSystemIAT = 0x24276230u;
inline constexpr std::uint32_t kDebugStateFlags = 0x2437290Cu;
inline constexpr std::uint32_t kDebugOwnerSlot = 0x24372908u;
inline constexpr std::uint32_t kDebugOwnerResolverVA = 0x2416F7F0u;
inline constexpr std::uint32_t kAllocBridgeA = 0x2416FCF0u;
inline constexpr std::uint32_t kDirectDebugAllocA = 0x2416FD40u;
inline constexpr std::uint32_t kFreeBridgeA = 0x2416FD60u;
inline constexpr std::uint32_t kAllocBridgeB = 0x2416FD80u;
inline constexpr std::uint32_t kDirectDebugAllocB = 0x2416FDD0u;
inline constexpr std::uint32_t kFreeBridgeB = 0x2416FDF0u;
inline constexpr std::uint32_t kAllocatorPresentVA = 0x2416FE10u;

// Observed MSVC x86 virtual offsets in imported tier0.dll g_pMemAlloc.
// +0 = debug allocation (size, opaque owner pointer, line), +4 = ordinary
// allocation (size), +0x14 = Free(void*). No engine-owned object is allocated
// by the model itself. Source typedefs/method labels remain inferred.
struct NativeDispatchOffsets32 {
    std::uint32_t debug_alloc_vslot = 0x00;
    std::uint32_t alloc_vslot = 0x04;
    std::uint32_t free_vslot = 0x14;
};
static_assert(sizeof(NativeDispatchOffsets32) == 12, "Three native virtual byte offsets");
static_assert(std::is_standard_layout<NativeDispatchOffsets32>::value, "Plain metadata");

struct NativeCachedDebugState32 {
    std::uint32_t bit0_flags = 0;
    std::uint32_t opaque_debug_owner = 0;
};
static_assert(sizeof(NativeCachedDebugState32) == 8, "two observed DWORD values");

// Host contract:
//   std::uintptr_t resolve_debug_owner(); (the still-unported 0x2416F7F0)
//   std::uintptr_t ordinary_alloc(uint32_t);
//   std::uintptr_t debug_alloc(uint32_t,uintptr_t,uint32_t);
//   void free_block(uintptr_t);
// Native invokes a virtual function with no pointer-validity guard; the
// host must supply a valid allocator. In the original, if the debug owner
// resolver returns zero, normal allocation is chosen. A zero result is cached.
class AllocationBridge {
public:
    explicit AllocationBridge(NativeCachedDebugState32 state = {}) noexcept : state_(state) {}

    template<class Host>
    std::uintptr_t allocate(Host& host, std::uint32_t bytes) {
        if (!(state_.bit0_flags & 1u)) {
            state_.bit0_flags |= 1u;  // native OR DWORD at 0x2437290C
            state_.opaque_debug_owner = static_cast<std::uint32_t>(host.resolve_debug_owner());
        }
        if (!state_.opaque_debug_owner) return host.ordinary_alloc(bytes);
        return host.debug_alloc(bytes, state_.opaque_debug_owner, 0u);
    }

    // Original function versions at 0x2416FD40 and 0x2416FDD0 take three
    // raw x86 stack arguments and dispatch directly to virtual slot +0.
    template<class Host>
    static std::uintptr_t allocate_explicit_debug(Host& host,
                                                  std::uint32_t bytes,
                                                  std::uintptr_t owner,
                                                  std::uint32_t line) {
        return host.debug_alloc(bytes, owner, line);
    }

    // Both native Free bridges unconditionally forward (including null).
    template<class Host>
    static void free(Host& host, std::uintptr_t pointer) {
        host.free_block(pointer);
    }

    const NativeCachedDebugState32& cached_state() const noexcept { return state_; }
private:
    NativeCachedDebugState32 state_;
};

// Original 0x2416FE10 reads the pointer stored in the imported tier0 global
// g_pMemAlloc. The native routine assumes its import-cell address is valid.
inline bool allocator_present(std::uintptr_t native_allocator_pointer) noexcept {
    return native_allocator_pointer != 0;
}
}  // namespace sin_compat::original_client_memalloc_bridge