#include "sin_compat/original_magnum_impact_effects.hpp"

namespace sin_compat::combat {

OriginalMagnumImpactResult dispatch_original_magnum_impact_effect(
    IOriginalMagnumImpactHost& host,
    const OriginalMagnumImpactInfoView& impact) {
    OriginalMagnumImpactResult result;

    host.invoke_base_impact_effect(impact);
    result.base_impact_called = true;

    result.duag_special_bit_set =
        (impact.damage_type & kOriginalDuagSpecialDamageBit) != 0U;
    if (!result.duag_special_bit_set) return result;

    result.trace_gate_blocked =
        (impact.trace_flags_42 & kOriginalDuagImpactTraceGateBit) != 0U;
    if (result.trace_gate_blocked) return result;

    host.dispatch_duag_decal(OriginalMagnumImpactDecal::DuagDecal1, impact);
    result.duag_decal1_dispatched = true;

    host.dispatch_duag_decal(OriginalMagnumImpactDecal::DuagDecal2, impact);
    result.duag_decal2_dispatched = true;
    return result;
}

const char* original_magnum_impact_decal_name(
    OriginalMagnumImpactDecal decal) noexcept {
    switch (decal) {
    case OriginalMagnumImpactDecal::DuagDecal1:
        return "DUAGDecal1";
    case OriginalMagnumImpactDecal::DuagDecal2:
        return "DUAGDecal2";
    }
    return nullptr;
}

} // namespace sin_compat::combat
