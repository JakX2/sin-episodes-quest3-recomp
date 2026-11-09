#include "sin_compat/original_grunt_kick.hpp"
#include <cmath>
namespace sin_compat::ai::grunt {
namespace {
float distance3d(const OriginalGruntKickVector3& a, const OriginalGruntKickVector3& b) noexcept {
    const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}
OriginalGruntKickEventResult dispatch_melee_fallback(
    OriginalGruntKickState& state, IOriginalGruntKickEventHost& host) noexcept {
    host.perform_melee_fallback(state.kick_damage, original_grunt_kick_melee_sound);
    host.record_last_anim_event_handled(original_grunt_kick_animation_event);
    return OriginalGruntKickEventResult::MeleeFallbackDispatched;
}
} // namespace
OriginalGruntKickEligibilityResult evaluate_original_grunt_kick_eligibility(
    const OriginalGruntKickEligibilityInput& input) noexcept {
    OriginalGruntKickEligibilityResult result{};
    if (!input.enemy_resolves) {
        result.failure = OriginalGruntKickEligibilityFailure::MissingEnemy; return result;
    }
    if (!input.tracked_grenade_resolves) {
        result.failure = OriginalGruntKickEligibilityFailure::MissingTrackedGrenade; return result;
    }
    const float vertical = std::fabs(input.grunt_origin.z - input.grenade_origin.z);
    if (!(vertical <= original_grunt_kick_vertical_tolerance)) {
        result.failure = OriginalGruntKickEligibilityFailure::VerticalSeparation; return result;
    }
    result.measured_distance = distance3d(input.grunt_origin, input.grenade_origin);
    result.maximum_distance = input.fraggrenade_radius * original_grunt_kick_radius_scale;
    if (!(result.measured_distance < result.maximum_distance)) {
        result.failure = OriginalGruntKickEligibilityFailure::Distance; return result;
    }
    if (!(input.horizontal_alignment_dot >= original_grunt_kick_alignment_threshold)) {
        result.failure = OriginalGruntKickEligibilityFailure::DirectionAlignment; return result;
    }
    if (!(input.npc_solid_trace_fraction == 1.0F)) {
        result.failure = OriginalGruntKickEligibilityFailure::ObstructedTrace; return result;
    }
    if (!input.candidate_is_base_grenade) {
        result.failure = OriginalGruntKickEligibilityFailure::WrongGrenadeClass; return result;
    }
    if (input.grenade_life_state != 0) {
        result.failure = OriginalGruntKickEligibilityFailure::GrenadeNotAlive; return result;
    }
    result.remaining_fuse = input.grenade_detonate_time - input.current_time;
    if (!(result.remaining_fuse > original_grunt_kick_minimum_fuse_seconds)) {
        result.failure = OriginalGruntKickEligibilityFailure::FuseTooShort; return result;
    }
    result.failure = OriginalGruntKickEligibilityFailure::None;
    return result;
}
std::optional<std::uint32_t> prepare_original_grunt_kick_schedule(
    OriginalGruntKickState& state, const OriginalGruntKickEligibilityInput& input) noexcept {
    if (!state.current_grenade.has_value()) return std::nullopt;
    if (!evaluate_original_grunt_kick_eligibility(input).eligible()) return std::nullopt;
    state.physics_entity = state.current_grenade;
    state.current_grenade.reset();
    return original_grunt_schedule_kick_grenade;
}
void start_original_grunt_kick_task(OriginalGruntKickState& state, float current_time,
    std::uint32_t melee_kick_activity_id, IOriginalGruntKickTaskHost& host) noexcept {
    state.last_kick_task_time = current_time;
    host.set_activity(melee_kick_activity_id);
}
OriginalGruntKickEventResult handle_original_grunt_kick_animation_event(
    OriginalGruntKickState& state, std::uint32_t event_id,
    IOriginalGruntKickEventHost& host) noexcept {
    if (event_id != original_grunt_kick_animation_event) return OriginalGruntKickEventResult::NotHandled;
    if (!state.physics_entity.has_value()) {
        host.log_missing_physics_entity();
        return dispatch_melee_fallback(state, host);
    }
    OriginalGruntPhysicsObjectToken physics_object = 0;
    OriginalGruntKickVector3 grenade_origin{}, related_target_origin{};
    const auto resolution = host.resolve_physics_object(*state.physics_entity, physics_object,
        grenade_origin, related_target_origin);
    if (resolution == OriginalGruntKickPhysicsResolution::MissingEntity) {
        host.log_missing_physics_entity();
        return dispatch_melee_fallback(state, host);
    }
    if (resolution == OriginalGruntKickPhysicsResolution::MissingPhysicsObject) {
        host.log_missing_physics_object();
        return OriginalGruntKickEventResult::MissingPhysicsObjectUnresolved;
    }
    // Preserved evidence supports target-minus-grenade, normalization, 800 speed, and +200 Z.
    // The target's source-level identity is not fully proven.
    const float dx = related_target_origin.x - grenade_origin.x;
    const float dy = related_target_origin.y - grenade_origin.y;
    const float dz = related_target_origin.z - grenade_origin.z;
    const float length = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (!(length > 0.0F)) return OriginalGruntKickEventResult::ZeroLengthDirectionUnresolved;
    const OriginalGruntKickVector3 linear{
        (dx / length) * original_grunt_kick_linear_speed,
        (dy / length) * original_grunt_kick_linear_speed,
        (dz / length) * original_grunt_kick_linear_speed + original_grunt_kick_upward_velocity};
    const OriginalGruntKickVector3 angular{
        host.random_float(original_grunt_kick_spin_x_min, original_grunt_kick_spin_x_max),
        host.random_float(original_grunt_kick_spin_y_min, original_grunt_kick_spin_y_max),
        original_grunt_kick_spin_z};
    host.add_velocity(physics_object, linear, angular);
    state.physics_entity.reset();
    host.clear_physics_entity_handle();
    host.record_last_anim_event_handled(event_id);
    return OriginalGruntKickEventResult::PhysicsVelocityApplied;
}
} // namespace sin_compat::ai::grunt
