#pragma once
// Phase178: original 2006 SiN PE32 game-module interface singleton metadata.
// These are ORIGINAL VIRTUAL ADDRESSES, NOT pointers in this host process.
// Do not invoke them without the actual original engine/MSVC x86 ABI.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace sin_compat::original_dll_engine_interface_objects {

enum class Module { Server, Client };

// Original x86 vptr data and the MSVC32 CompleteObjectLocator at vtable[-1].
struct NativeObject32 { std::uint32_t vtable_va; };
struct NativeMSVCCompleteObjectLocator32 {
    std::uint32_t signature, subobject_offset, cd_offset;
    std::uint32_t type_descriptor_va, class_hierarchy_va;
};
static_assert(sizeof(NativeObject32) == 4, "original x86 singleton vptr is one DWORD");
static_assert(sizeof(NativeMSVCCompleteObjectLocator32) == 20,
              "MSVC32 RTTI complete object locator is five DWORDs");
static_assert(std::is_standard_layout<NativeMSVCCompleteObjectLocator32>::value,
              "RTTI records must be plain 32-bit layouts");

struct FactoryPin {
    Module module;
    const char* name;
    std::uint32_t original_factory_va;
    std::uint32_t original_object_va;
    std::uint32_t pe_initialized_vtable_va; // zero means VIRTUAL BSS, unknown
    std::uint16_t verified_executable_prefix_slots; // NOT guaranteed full ABI
};

// Exact owner-original 2006 server.dll and client.dll references. The five
// PE-initialized objects have MSVC RTTI and vtable prefixes pinned by audit.
// The remaining four objects live in uninitialized .data, so their original
// startup constructors must be traced before their vtables can be asserted.
inline constexpr std::array<FactoryPin, 9> kOriginalInterfaceFactories{{
    {Module::Server,"ServerGameDLL003",0x221E7630u,0x22510650u,0x22442EFCu,31},
    {Module::Server,"ServerGameEnts001",0x221E82C0u,0x22510654u,0x22443030u,7},
    {Module::Server,"ServerGameClients003",0x221E8320u,0x22510658u,0x22443054u,15},
    {Module::Server,"GameMovement001",0x222CFFE0u,0x225934B0u,0u,0},
    {Module::Client,"GameClientExports001",0x240FD810u,0x242FA4B0u,0x24296B18u,4},
    {Module::Client,"VClient011",0x240FD840u,0x242FA4B8u,0x24296C6Cu,48},
    {Module::Client,"VClientEntityList003",0x241056B0u,0x2434FDDCu,0u,0},
    {Module::Client,"VClientPrediction001",0x2422EA30u,0x2437FA00u,0u,0},
    {Module::Client,"GameMovement001",0x2420FA40u,0x24379920u,0u,0},
}};

inline const FactoryPin* find_factory(Module module, const char* name) noexcept {
    if (name == nullptr) return nullptr;
    for (const FactoryPin& pin : kOriginalInterfaceFactories) {
        if (pin.module == module && std::strcmp(pin.name, name) == 0) return &pin;
    }
    return nullptr;
}

// Model of each 6-byte original MOV EAX,<singleton VA>; RET factory thunk.
// Returning a virtual address is deliberately NOT returning a host pointer.
inline constexpr std::uint32_t original_factory_return_va(const FactoryPin& pin) noexcept {
    return pin.original_object_va;
}

// The engine-facing vtable has slot-address arithmetic with 4-byte entries.
// An actual x86 engine call additionally requires the exact thiscall ABI and
// correctly initialized game object, neither of which this reference invents.
inline std::uint32_t original_observed_method_va(
    const FactoryPin& pin, const std::vector<std::uint32_t>& host_observed_slots,
    std::size_t index) {
    if (pin.pe_initialized_vtable_va == 0 ||
        index >= pin.verified_executable_prefix_slots ||
        index >= host_observed_slots.size()) {
        throw std::out_of_range("method not established from original PE32 .rdata");
    }
    return host_observed_slots[index];
}

} // namespace sin_compat::original_dll_engine_interface_objects