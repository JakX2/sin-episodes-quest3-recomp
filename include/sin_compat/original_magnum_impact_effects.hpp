#pragma once

// Independently authored portable reconstruction of the original SiN
// CWeaponMagnum impact-effect override. The host owns native trace objects,
// decal dispatch and the engine's inherited impact-effect implementation.

#include <cstdint>

namespace sin_compat::combat {

// Original 2006 server.dll Phase 39 evidence. This is a bit test, not equality.
inline constexpr std::uint32_t kOriginalDuagSpecialDamageBit = 0x00400000U;

// Raw byte at the original trace-like object +0x42. Its symbolic field name
// remains unresolved; bit 0x04 suppresses the two extra DUAG decals.
inline constexpr std::uint8_t kOriginalDuagImpactTraceGateBit = 0x04U;

enum class OriginalMagnumImpactDecal : std::uint8_t {
    DuagDecal1,
    DuagDecal2
};

// Only the two original fields used by this override are decoded. host_payload
// is borrowed and opaque; this library never dereferences or retains it.
struct OriginalMagnumImpactInfoView {
    std::uint32_t damage_type = 0;
    std::uint8_t trace_flags_42 = 0;
    const void* host_payload = nullptr;
};

struct OriginalMagnumImpactResult {
    bool base_impact_called = false;
    bool duag_special_bit_set = false;
    bool trace_gate_blocked = false;
    bool duag_decal1_dispatched = false;
    bool duag_decal2_dispatched = false;
};

class IOriginalMagnumImpactHost {
public:
    virtual ~IOriginalMagnumImpactHost() = default;

    // The preserved Phase 39 contract records the inherited impact handler
    // before the SiN-specific DUAG branch. The host maps this to its own
    // compatible engine method; no original x86 slot is invoked here.
    virtual void invoke_base_impact_effect(
        const OriginalMagnumImpactInfoView& impact) = 0;

    // Dispatches one of the two original DUAG decal resources. The host owns
    // trace/material resolution and the real engine effect API.
    virtual void dispatch_duag_decal(
        OriginalMagnumImpactDecal decal,
        const OriginalMagnumImpactInfoView& impact) = 0;
};

// Reconstructs the CWeaponMagnum impact-effect override's base-first order,
// then its DUAG damage-bit and raw trace-byte gates. It does not model bullet
// penetration, material selection, or the engine's generic impact effect.
OriginalMagnumImpactResult dispatch_original_magnum_impact_effect(
    IOriginalMagnumImpactHost& host,
    const OriginalMagnumImpactInfoView& impact);

// Exact original resource names recovered by the preserved Phase 39 verifier.
const char* original_magnum_impact_decal_name(
    OriginalMagnumImpactDecal decal) noexcept;

} // namespace sin_compat::combat
