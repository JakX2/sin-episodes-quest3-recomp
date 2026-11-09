#include "sin_compat/original_grunt_beserk_behavior.hpp"

namespace sin_compat::ai::grunt {

OriginalGruntBeserkBehavior::OriginalGruntBeserkBehavior(
    float current_time) noexcept {
    initialize(current_time);
}

void OriginalGruntBeserkBehavior::initialize(float current_time) noexcept {
    active_ = false;
    deadline_ = current_time;
}

void OriginalGruntBeserkBehavior::activate(
    float current_time,
    IOriginalGruntBeserkHost& host) noexcept {
    deadline_ = current_time +
        original_grunt_beserk_activation_seconds;

    if (!active_) {
        host.set_ai_condition(OriginalSharedAiCondition::ScheduleDone);
        host.invoke_unresolved_npc_operation(3);
        host.apply_unresolved_npc_state_change_if_required();
    }

    active_ = true;
}

void OriginalGruntBeserkBehavior::update_expiration(
    float current_time) noexcept {
    if (active_ && current_time > deadline_) {
        active_ = false;
    }
}

bool OriginalGruntBeserkBehavior::active() const noexcept {
    return active_;
}

float OriginalGruntBeserkBehavior::deadline() const noexcept {
    return deadline_;
}

std::optional<std::uint32_t> original_grunt_beserk_schedule(
    bool separate_schedule_request_flag) noexcept {
    if (!separate_schedule_request_flag) return std::nullopt;
    return original_grunt_schedule_beserk;
}

} // namespace sin_compat::ai::grunt
