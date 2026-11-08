#include "sin_compat/original_jessica_throw.hpp"
#include <utility>

namespace sin_compat::jessica {
InputValue InputValue::string_value(std::string text) {
    return {2, std::move(text), 0};
}
InputValue InputValue::integer_value(std::int32_t number) noexcept {
    return {5, {}, number};
}

void ThrowRuntime::input_throw_item_at_player(const InputValue& value) {
    // 0x223622D0: compares variant type with 2. A mismatch writes an
    // empty string handle to Jessica+0x144C, but STILL sets condition 0x47.
    const std::string next = value.original_type == 2 ? value.text : std::string{};
    throw_name_ = next;
    host_.set_condition(kThrowCondition);
}

void ThrowRuntime::input_set_primary_ammo_count_to_throw(
    const InputValue& value) noexcept {
    // 0x22362320: writes +0x1450 from integer type=5 or zero otherwise.
    primary_ammo_ = value.original_type == 5 ? value.integer : 0;
}
void ThrowRuntime::input_set_secondary_ammo_count_to_throw(
    const InputValue& value) noexcept {
    // 0x22362350: writes +0x1454 from integer type=5 or zero otherwise.
    secondary_ammo_ = value.original_type == 5 ? value.integer : 0;
}
bool ThrowRuntime::input_holster_weapon() {
    // 0x22362300: resolves local player and dispatches its virtual method.
    // The host, not this portable code, must resolve that player / ABI.
    return host_.holster_local_player_weapon();
}
int ThrowRuntime::select_schedule() {
    // 0x22362380: if condition 0x47 is set, return 0x59. Only call the
    // companion selector when absent (it may have side effects).
    if (host_.has_condition(kThrowCondition)) return kThrowSchedule;
    return host_.select_companion_schedule();
}
AnimationResult ThrowRuntime::on_animation_event(int event_id) {
    if (!registered_event_id_) {
        host_.on_companion_animation_event(event_id);
        return AnimationResult::NoRegisteredEvent;
    }
    if (event_id != *registered_event_id_) {
        host_.on_companion_animation_event(event_id);
        return AnimationResult::ForwardedToCompanion;
    }
    // 0x22362420: clear condition via 0x220398A0 BEFORE the spawn.
    host_.clear_condition(kThrowCondition);
    const auto entity = host_.spawn_at_attachment(throw_name_, kRightHandAttachment);
    if (!entity) return AnimationResult::SpawnRejected;
    if (host_.is_combat_weapon(*entity)) {
        // Only nonnegative values are applied. The original writes to
        // weapon+0x4D4 and +0x4D8 and consumes each applied value (-1).
        if (primary_ammo_ >= 0) {
            host_.set_primary_ammo(*entity, primary_ammo_);
            primary_ammo_ = -1;
        }
        if (secondary_ammo_ >= 0) {
            host_.set_secondary_ammo(*entity, secondary_ammo_);
            secondary_ammo_ = -1;
        }
    }
    if (toss_provider_) {
        const auto geometry=toss_provider_->observe_toss(
            *entity,kRightHandAttachment);
        if (!geometry) return AnimationResult::LaunchRejected;
        const auto solution=solve_toss(*geometry);
        // Invalid gravity/NaNs are rejected before any velocity impulse.
        // This guard was not established in the original x87.
        if (!solution.valid()) return AnimationResult::LaunchRejected;
        const auto result=apply_original_velocity_impulse(
            *toss_provider_,*entity,solution.velocity);
        // The original animation dispatch does not require a new entity
        // velocity when the impulse is zero. The solver's normal throw
        // path produces a nonzero up component, but retain the contract.
        return result==ImpulseCode::AppliedPhysics ||
               result==ImpulseCode::AppliedNonPhysics ||
               result==ImpulseCode::IgnoredZero
            ? AnimationResult::Launched : AnimationResult::LaunchRejected;
    }
    // The abstract callback only remains for the Phase-103 mock host.
    return host_.launch_toward_player(*entity)
        ? AnimationResult::Launched : AnimationResult::LaunchRejected;
}
} // namespace sin_compat::jessica
