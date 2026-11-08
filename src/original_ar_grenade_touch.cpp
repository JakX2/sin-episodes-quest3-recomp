#include "sin_compat/original_ar_grenade_touch.hpp"

namespace sin_compat::ar_grenade_touch {

Evaluation evaluate(const OriginalTouchObservation& o) noexcept {
    if (!o.other_entity_present)
        return {Gate::BlockedNoOtherEntity,false,o.grenade_original_field_795};
    if (o.other_field_11c == 0)
        return {Gate::BlockedOtherField11cZero,false,o.grenade_original_field_795};
    if ((o.other_flags_120 & 0x04u) != 0u)
        return {Gate::BlockedOtherFlag120Bit4,false,o.grenade_original_field_795};

    // Original 0x22314310 loads the other entity's virtual function at
    // +0xFC. If its result is non-null and differs from the result of the
    // projectile's helper at 0x222B2730, original code passes byte=1 to
    // the field +0x795 setter at 0x222B2B40. We only report that action.
    const bool request_set =
        o.other_virtual_fc_result_nonnull &&
        !o.other_virtual_fc_matches_local_helper;
    const bool after_contact = o.grenade_original_field_795 || request_set;

    if (!after_contact)
        return {Gate::BlockedOriginalFlag795Zero,false,false};
    return {Gate::CallOriginalDetonateSlot250,request_set,after_contact};
}

DispatchResult dispatch(const OriginalTouchObservation& o,
                        ar_grenade::Runtime& projectile,
                        ar_grenade::Decal decal,
                        ar_grenade::IHost& host) {
    const Evaluation decision=evaluate(o);
    if (!decision.should_dispatch_detonation())
        return {decision,ar_grenade::StepResult::TriggerRejected};
    return {decision,projectile.detonate(true,decal,host)};
}

} // namespace sin_compat::ar_grenade_touch
