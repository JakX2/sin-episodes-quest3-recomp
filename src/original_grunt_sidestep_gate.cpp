#include "sin_compat/original_grunt_sidestep_gate.hpp"

namespace sin_compat::original_grunt_sidestep {

Decision EligibilityGate::evaluate(NativeFacts facts) noexcept {
    if (busy_) return {Exit::Reentrant, 0.0f};
    struct BusyGuard {
        bool& value;
        explicit BusyGuard(bool& v) noexcept : value(v) { value = true; }
        ~BusyGuard() noexcept { value = false; }
    } guard(busy_);
    // Original native +0xCA0 and +0x988 tests precede every callback.
    if (facts.byte_ca0_nonzero) return {Exit::Ca0Set, 0.0f};
    if (facts.byte_988_bit2_set) return {Exit::Flag988Set, 0.0f};
    try {
        if (host_.has_original_condition(4))
            return {Exit::Condition4Present, 0.0f};
        if (host_.has_original_condition(3))
            return {Exit::Condition3Present, 0.0f};
        // Original checks +0x8C only *after* both condition queries.
        if (facts.native_field_8c_nonzero)
            return {Exit::Field8cNonzero, 0.0f};
        if (!host_.call_owner_virtual_304_with_original_context())
            return {Exit::OwnerVirtual304Rejected, 0.0f};
        if (!host_.call_manager_virtual_300_with_owner())
            return {Exit::ManagerVirtual300Rejected, 0.0f};
        const float factor =
            host_.has_original_condition(kInPlayerSightsCondition) ? 4.0f : 1.0f;
        if (!host_.original_rules_mobility_decision(factor))
            return {Exit::MobilityRuleRejected, factor};
        return {Exit::Eligible, factor};
    } catch (...) {
        return {Exit::HostException, 0.0f};
    }
}

SelectorState3Result select_original_state_three(
    std::uint32_t field_850, EligibilityGate& gate, NativeFacts facts,
    IState3Fallback& fallback) noexcept {
    SelectorState3Result result{};
    // The original full SelectSchedule checks many higher-priority cases.
    // This isolated helper represents only the verified +0x850 == 3 arm.
    if (field_850 != 3) return result;
    result.gate = gate.evaluate(facts);
    if (result.gate.eligible()) {
        result.handled = true;
        result.original_schedule_id = kOriginalPositiveScheduleId;
        return result;
    }
    if (result.gate.exit == Exit::HostException ||
        result.gate.exit == Exit::Reentrant)
        return result; // independent adapter fail-closed, not original exception ABI
    try {
        result.original_schedule_id = fallback.invoke_original_virtual_844();
        result.handled = result.original_schedule_id != 0;
    } catch (...) {
        result.gate.exit = Exit::HostException;
    }
    return result;
}
} // namespace sin_compat::original_grunt_sidestep