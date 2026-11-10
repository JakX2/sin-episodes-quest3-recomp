#pragma once
// Phase171: original 2006 SiN Episodes server.dll game entity factory registry
// vtable +0x04 (0x22285550), +0x08 (0x22285580) and +0x0C (0x2228B010).
// Independent C++17 host-neutral control-flow reconstruction, not the
// original MSVC x86 object/vtable or Source game engine allocator ABI.
#include <cstdint>

namespace sin_compat::original_server_entity_registry_lookup {

inline constexpr std::uint32_t kRegistryVtableVA=0x22456324u;
inline constexpr std::uint32_t kCreateResolverVA=0x22285550u;
inline constexpr std::uint32_t kDestroyResolverVA=0x22285580u;
inline constexpr std::uint32_t kFindRecordVA=0x2228B010u;
inline constexpr std::uint32_t kCreateUnknownFormatVA=0x22455F50u;
inline constexpr std::uint32_t kDestroyUnknownFormatVA=0x22455F80u;
inline constexpr std::uint16_t kAbsentSymbolIndex=0xffffu;
inline constexpr std::uint32_t kNativeRecordStride=16u;
inline constexpr std::uint32_t kNativeFactoryWordOffset=12u;

// The host carries an opaque handle for the original registry object.
// Registry member +0x20 stores symbol internals and member +0x04 has the
// indexed 16-byte record table. The model deliberately does not simulate
// the native symbol hashing or allocator.
struct RegistryToken { std::uintptr_t native_instance=0; };

// Original registry virtual +0x0C, 0x2228B010 (94 native bytes).
// The original first calls +0x20 symbol interning (0x2228F7A0) on the
// provided name, then 0x2228A760 on the +0x04 lookup index with a 16-bit
// symbol and an integer zero. 0xffff means not found. On hit, reads one
// 32-bit pointer from native (+0x08) table at index*16 +0x0C. This is
// a *registered provider pointer*, not yet a constructed entity.
template<class Host>
std::uintptr_t original_find_registered_provider(
    RegistryToken registry, const char* classname, Host& host) {
    const std::uint16_t symbol=host.intern_class_symbol(registry,classname);
    const std::uint16_t index=host.lookup_class_symbol_index(registry,symbol,0u);
    if (index==kAbsentSymbolIndex) return 0u;
    return host.read_provider_from_record(registry,index,
                                          kNativeRecordStride,kNativeFactoryWordOffset);
}

// Original registry virtual +0x04, 0x22285550 (46 native bytes).
// Calls registry vtable +0x0C first; missing entry emits native warning
// "Attempted to create unknown entity type %s!\n", returns zero.
// On hit passes the *original classname* to registered provider virtual
// +0x00, returning that invocation's value. No game entity object layout
// or provider +0x00 semantics are guessed here.
template<class Host>
std::uintptr_t original_resolve_create_factory(
    RegistryToken registry, const char* classname, Host& host) {
    const std::uintptr_t provider=original_find_registered_provider(registry,classname,host);
    if(provider==0u) {
        host.report_unknown_create(classname,kCreateUnknownFormatVA);
        return 0u;
    }
    return host.call_registered_provider_create(provider,classname);
}

// Original registry virtual +0x08, 0x22285580 (50 native bytes).
// Same provider lookup, distinct native diagnostic and provider +0x04
// invocation. The second caller-supplied 32-bit argument is passed to
// provider +0x04, *not* the classname. This wrapper does not itself alter
// the Phase170 temporary global edict context.
template<class Host>
std::uintptr_t original_resolve_destroy_factory(
    RegistryToken registry,const char* classname,
    std::uintptr_t opaque_argument,Host& host) {
    const std::uintptr_t provider=original_find_registered_provider(registry,classname,host);
    if(provider==0u) {
        host.report_unknown_destroy(classname,kDestroyUnknownFormatVA);
        return 0u;
    }
    return host.call_registered_provider_destroy(provider,opaque_argument);
}

} // namespace sin_compat::original_server_entity_registry_lookup