#pragma once
// Phase170: original 2006 PE32 server.dll entity factory entry points,
// registry singleton and registry construction. This is a host-neutral
// decision/callback model, NOT a native 32-bit engine registry or DLL ABI.
#include <cstdint>

namespace sin_compat::original_server_entity_factory {
inline constexpr std::uint32_t kRegistrySingletonVA = 0x2258FFB0u;
inline constexpr std::uint32_t kRegistryGuardVA = 0x22590004u;
inline constexpr std::uint32_t kRegistryVirtualTableVA = 0x22456324u;
inline constexpr std::uint32_t kRegistryCallbackVA = 0x222868C0u;
inline constexpr std::uint32_t kRegistryDestructorVA = 0x223CEA10u;
inline constexpr std::uint32_t kEngineInterfaceGlobalVA = 0x22572198u;
inline constexpr std::uint32_t kEdictContextGlobalVA = 0x22558764u;
inline constexpr std::uint32_t kEdictFailureFormatVA = 0x224542A4u;
inline constexpr std::uint32_t kFactoryLookupVirtualOffset = 0x04u;
inline constexpr std::uint32_t kOrdinaryFactoryVirtualOffset = 0x24u;
inline constexpr std::uint32_t kAlternateFactoryVirtualOffset = 0x20u;
inline constexpr std::uint32_t kEngineCreateEdictVirtualOffset = 0x54u;
inline constexpr std::int32_t kNoExplicitEdict = -1;

// Original factory-list constructor 0x2228AFB0, 96 bytes.
// This is a semantic representation of the original 32-bit +0x00..+0x20
// fields, NOT a memcpy-compatible MSVC object layout.
struct RegistryStorage {
    std::uint32_t original_vtable_va = 0;
    std::uint32_t original_callback_va = 0;
    std::uintptr_t allocation_08 = 0;
    std::uint32_t capacity_step_0c = 0;
    std::uint32_t reserved_10 = 0;
    std::uint16_t marker_14 = 0;
    std::uint16_t marker_16 = 0;
    std::uint16_t marker_18 = 0;
    std::uint16_t marker_1a = 0;
    std::uintptr_t allocation_alias_1c = 0;
};

template<class Host>
void original_registry_construct(RegistryStorage& state, Host& host) {
    state.original_vtable_va = kRegistryVirtualTableVA;
    state.original_callback_va = kRegistryCallbackVA;
    state.allocation_08 = 0;
    state.capacity_step_0c = 0x80u;
    state.reserved_10 = 0;
    // Native allocation size: 0x800 bytes, independent of subsequent +0x20
    // member initialization (whose exact private engine implementation is
    // represented by a callback rather than guessed).
    const std::uintptr_t allocated = host.allocate_registry_storage(0x800u);
    state.allocation_08 = allocated;
    state.marker_14 = 0xffffu;
    state.marker_18 = 0xffffu;
    state.marker_16 = 0;
    state.marker_1a = 0;
    state.allocation_alias_1c = allocated;
    host.initialize_registry_table(state, 0u, 0x80u, 1u); // 0x2228F360
}

struct RegistrySingleton {
    std::uint32_t initialization_guard = 0;
    RegistryStorage storage{};
};

// Original 0x2228B110 (48 bytes). The original tests guard BYTE bit0,
// sets it *before* constructor call, then registers a destructor.
// Reentrant get during construction sees the guarded, partial instance.
template<class Host>
RegistryStorage* original_registry_singleton(RegistrySingleton& singleton,
                                             Host& host) {
    if ((singleton.initialization_guard & 1u) == 0u) {
        singleton.initialization_guard |= 1u;
        original_registry_construct(singleton.storage, host);
        host.register_registry_destructor(kRegistryDestructorVA);
    }
    return &singleton.storage;
}

struct FactoryGlobals {
    RegistrySingleton registry{};
    std::uintptr_t current_edict_context = 0;
};

// Original CreateEntityByName-style function 0x22266B00 (89 bytes).
// The native signed -1 index skips edict allocation, but EVERY path clears
// original global edict context +0x22558764 after registry lookup, even when
// no matching factory exists. Native factory callback +0x24 is a TAIL JMP.
// The engine virtual +0x54 may return null: original prints a warning but
// still performs the classname lookup and potential construction.
template<class Host>
std::uintptr_t original_create_entity_by_name(
    FactoryGlobals& globals, const char* classname,
    std::int32_t requested_edict, Host& host) {
    if (requested_edict != kNoExplicitEdict) {
        const std::uintptr_t edict = host.engine_create_edict(requested_edict);
        globals.current_edict_context = edict;
        if (edict == 0)
            host.report_edict_failure(classname, requested_edict,
                                      kEdictFailureFormatVA);
    }
    RegistryStorage* registry = original_registry_singleton(globals.registry,
                                                            host);
    // Phase171: registry vtable +0x04 first resolves a 16-byte provider
    // record via registry +0x0C, then calls provider virtual +0x00.
    // The return value, not the provider record pointer, is the object
    // dispatched below via factory virtual +0x24 (or +0x20).
    const std::uintptr_t factory = host.lookup_class_factory(
        *registry, classname, kFactoryLookupVirtualOffset);
    globals.current_edict_context = 0;
    if (factory == 0) return 0;
    return host.invoke_factory(factory, kOrdinaryFactoryVirtualOffset);
}

// Original 0x22266B60 (29 bytes): same registry lookup but +0x20 vtable
// dispatch, not +0x24, with no edict context handling whatsoever.
// No proven semantic name is assigned to its distinct callback output.
template<class Host>
std::uintptr_t original_secondary_factory_dispatch(
    FactoryGlobals& globals, const char* classname, Host& host) {
    RegistryStorage* registry = original_registry_singleton(globals.registry,
                                                            host);
    const std::uintptr_t factory = host.lookup_class_factory(
        *registry, classname, kFactoryLookupVirtualOffset);
    if (factory == 0) return 0;
    return host.invoke_factory(factory, kAlternateFactoryVirtualOffset);
}
} // namespace sin_compat::original_server_entity_factory