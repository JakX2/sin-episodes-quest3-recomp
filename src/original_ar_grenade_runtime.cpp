#include "sin_compat/original_ar_grenade_runtime.hpp"

#include <algorithm>
#include <cmath>

namespace sin_compat::ar_grenade {

namespace {
bool valid_vec(const Vec3& v) noexcept {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

bool valid_time_and_delay(float time, float delay) noexcept {
    return std::isfinite(time) && std::isfinite(time + delay) &&
           time + delay >= time;
}

bool half_velocity_point(const Vec3& origin, const Vec3& velocity,
                         Vec3& result) noexcept {
    result = {origin.x + velocity.x * 0.5f,
              origin.y + velocity.y * 0.5f,
              origin.z + velocity.z * 0.5f};
    return valid_vec(result);
}
} // namespace

bool Runtime::spawn(const Vec3& origin, const Vec3& velocity,
                    float time) noexcept {
    if (!valid_vec(origin) || !valid_vec(velocity) ||
        !valid_time_and_delay(time, original_initial_think_delay)) return false;
    Vec3 half{};
    if (!half_velocity_point(origin,velocity,half)) return false;
    origin_=origin;
    velocity_=velocity;
    danger_radius_=original_initial_danger_radius;
    next_think_at_=time + original_initial_think_delay;
    damage_=original_initial_damage;
    blast_radius_=original_initial_blast_radius;
    spawned_=true;
    armed_=true;
    return true;
}

bool Runtime::update_kinematics(const Vec3& origin,
                                const Vec3& velocity) noexcept {
    if (!spawned_ || !armed_ ||
        !valid_vec(origin) || !valid_vec(velocity)) return false;
    Vec3 half{};
    if (!half_velocity_point(origin,velocity,half)) return false;
    origin_=origin;
    velocity_=velocity;
    return true;
}

bool Runtime::set_runtime_damage(float damage, float blast_radius) noexcept {
    if (!spawned_ || !armed_ || !std::isfinite(damage) || damage < 0.0f ||
        !std::isfinite(blast_radius) || blast_radius < 0.0f ||
        !std::isfinite(blast_radius * 1.5f)) return false;
    damage_=damage;
    blast_radius_=blast_radius;
    return true;
}

StepResult Runtime::think(float time, bool original_ai_predicate,
                          bool original_increment_predicate, IHost& host) {
    if (!spawned_ || !armed_) return StepResult::Disarmed;
    if (!valid_time_and_delay(time,original_repeat_think_delay))
        return StepResult::InvalidInput;
    if (time < next_think_at_) return StepResult::NotDue;
    Vec3 hearing_position{};
    if (!half_velocity_point(origin_,velocity_,hearing_position))
        return StepResult::InvalidInput;

    // Original checks whether +0x7C4 is below 300 before adding 15.
    // It can temporarily become 310; this is NOT a hard clamp at 300.
    // The conditional itself must eventually come from verified physics/AI.
    if (original_increment_predicate &&
        danger_radius_ < original_danger_radius_comparison) {
        danger_radius_ += original_danger_radius_increment;
    }
    next_think_at_=time+original_repeat_think_delay;

    // The separate AI sound is a server-side hearing stimulus. It has
    // nothing to do with an audible wav or HMD head displacement.
    if (original_ai_predicate) {
        host.on_flight_ai_sound({
            hearing_position,original_ai_sound_danger,
            static_cast<float>(danger_radius_),
            original_danger_sound_lifetime,1
        });
    }
    return StepResult::Applied;
}

StepResult Runtime::detonate(bool approved, Decal selected_decal, IHost& host) {
    if (!spawned_ || !armed_) return StepResult::Disarmed;
    if (!approved) return StepResult::TriggerRejected;
    if (selected_decal!=Decal::None && selected_decal!=Decal::Scorch &&
        selected_decal!=Decal::SmallScorch) return StepResult::InvalidInput;

    // Mirrors original one-shot +0x795 transition; doing this before
    // callbacks also prevents recursive detonation from double-damaging.
    armed_=false;

    // Original trace and Decal selection precede the screen shake. We let
    // the engine determine trace outcome (Scorch / SmallScorch / none).
    host.on_original_trace_effect(selected_decal);

    // The original Source-style utility receives requested amplitude=25,
    // clamps to 16, and uses frequency=150, duration=1, radius=750. The
    // future VR renderer MUST NOT move an OpenXR-tracked headset pose.
    host.on_screen_shake_feedback({
        origin_,25.0f,16.0f,150.0f,1.0f,750.0f,false
    });

    // The original RadiusDamage-like wrapper applies game-rules damage
    // before creating its additional DMG_BLAST combat AI hearing event.
    host.on_radius_damage({
        origin_,damage_,blast_radius_,original_damage_type_blast
    });
    const float sound_radius=std::max(128.0f,blast_radius_*1.5f);
    host.on_explosion_ai_sound({
        origin_,original_ai_sound_explosion,sound_radius,
        original_blast_sound_lifetime,0
    });
    host.on_remove_projectile();
    return StepResult::Applied;
}

} // namespace sin_compat::ar_grenade
