#pragma once
// Original SiN 2006 vs SourceVR 0.5.5 native ARM64 FACTORY NAME preflight.
// No interface version aliases. Never invoke unverified native vtables.
#include <array>
#include <string_view>
namespace sin_compat::phase96 {
enum class FactoryGate { MissingRegistration, VersionMismatch, MethodABIUnverified, VerifiedCompatible };
struct InterfaceEvidence {
    std::string_view original_required;
    std::string_view native_registered;
    bool registration_proven;
    bool method_abi_proven;
};
inline constexpr std::array<InterfaceEvidence,3> sourcevr_055_matrix{{
    {"VSoundEmitter001", "VSoundEmitter002", true, false},
    {"VGUI_InputInternal001", "VGUI_InputInternal001", true, false},
    {"VGUI_System010", "VGUI_System010", true, false},
}};
inline constexpr FactoryGate gate_factory(const InterfaceEvidence& e) noexcept {
    if (!e.registration_proven) return FactoryGate::MissingRegistration;
    if (e.original_required != e.native_registered) return FactoryGate::VersionMismatch;
    if (!e.method_abi_proven) return FactoryGate::MethodABIUnverified;
    return FactoryGate::VerifiedCompatible;
}
inline constexpr std::string_view gate_name(FactoryGate status) noexcept {
    switch (status) {
      case FactoryGate::MissingRegistration: return "registration_not_verified";
      case FactoryGate::VersionMismatch: return "factory_version_mismatch";
      case FactoryGate::MethodABIUnverified: return "method_abi_not_verified";
      case FactoryGate::VerifiedCompatible: return "verified_compatible";
    }
    return "unknown";
}
template<std::size_t N>
constexpr bool safe_to_bind(const std::array<InterfaceEvidence,N>& entries) noexcept {
    for (const auto& e: entries)
        if (gate_factory(e) != FactoryGate::VerifiedCompatible) return false;
    return true;
}
} // namespace sin_compat::phase96
