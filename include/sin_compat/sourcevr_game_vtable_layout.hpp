#pragma once
// Concrete SourceVR 0.5.5 AArch64 game-object topology, recovered from
// 11 SHA-pinned libserver/libclient.so game-module ELFs across seven profiles.
// Factory-owning singleton pointer, native RTTI base and ALL slot pointers
// verified by audit_sourcevr_game_module_vtables.py.
// NEVER reinterpret_cast these entries into a SiN 2006 x86 interface.
// This is layout metadata; per-method signatures are NOT verified.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sin_compat::native_game_vtables {

enum class NativeGameObjectRole : std::uint8_t { Server, Client };
enum class MethodAuditLevel : std::uint8_t {
    Unverified,  // no native C++ method signature/semantics independently proven
    FullyVerified
};

struct NativeGameObjectLayout {
    NativeGameObjectRole role;
    std::string_view concrete_type_name;
    std::string_view base_type_name;
    std::string_view native_factory;
    std::uint32_t method_slots;
    std::uint32_t native_pointer_bits;
    bool concrete_rtti_proven;
    bool factory_singleton_vptr_proven;
    MethodAuditLevel method_signatures;
};

// The original SHA256-pinned SiN 2006 Windows x86 vftables were separately
// recovered using MSVC TypeDescriptor / CompleteObjectLocator references:
// server CServerGameDLL: 31 methods, client CHLClient: 48 methods.
// Method count differences are NOT necessarily appended new methods.
inline constexpr std::uint32_t original_sin_server_slots = 31u;
inline constexpr std::uint32_t original_sin_client_slots = 48u;
inline constexpr std::uint32_t native_sourcevr_server_slots = 47u;
inline constexpr std::uint32_t native_sourcevr_client_slots = 78u;
static_assert(native_sourcevr_server_slots - original_sin_server_slots == 16u);
static_assert(native_sourcevr_client_slots - original_sin_client_slots == 30u);

inline constexpr std::array<NativeGameObjectLayout, 2> verified_layouts{{
    {NativeGameObjectRole::Server, "CServerGameDLL", "IServerGameDLL",
     "ServerGameDLL010", 47, 64, true, true, MethodAuditLevel::Unverified},
    {NativeGameObjectRole::Client, "CHLClient", "IBaseClientDLL",
     "VClient017", 78, 64, true, true, MethodAuditLevel::Unverified}
}};

// The table length is a NECESSARY but INSUFFICIENT ABI compatibility test.
// The actual engine dispatch also requires every virtual call's signature,
// object layout, lifecycle, version and observed engine callsite verified.
constexpr bool native_game_object_dispatch_allowed(
    const NativeGameObjectLayout& layout,
    std::size_t implemented_slots) noexcept {
    return layout.concrete_rtti_proven &&
           layout.factory_singleton_vptr_proven &&
           layout.native_pointer_bits == 64 &&
           implemented_slots == layout.method_slots &&
           layout.method_signatures == MethodAuditLevel::FullyVerified;
}

static_assert(verified_layouts[0].method_slots == 47);
static_assert(verified_layouts[1].method_slots == 78);
static_assert(!native_game_object_dispatch_allowed(verified_layouts[0], 47));
static_assert(!native_game_object_dispatch_allowed(verified_layouts[1], 78));

}  // namespace sin_compat::native_game_vtables
