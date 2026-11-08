#pragma once

// Independently authored, host-driven reconstruction of verified SiN Episodes
// (2006) CGrenadeAR behavior, from the owner's SHA-pinned original server.dll.
// Original PE32 virtual addresses and vtables are NEVER called on ARM64.
//
// Source-backed facts:
//  * grenade_ar factory; CGrenadeAR inherits CBaseGrenade, Detonate = slot250
//  * spawn: model, damage 100, blast radius 250, danger radius 100,
//    initial think +0.1 seconds; later think intervals +0.05 seconds
//  * active flight danger stimulus: origin + velocity*0.5, type 0x8,
//    lifetime 0.2 seconds, channel 1, radius may increase by 15 when <300
//  * Detonate: guarded by +0x795 and clears it, uses blast damage type 0x40,
//    screen shake (25 input clamped to 16, 150Hz, 1 second, 750 radius),
//    then radius damage and AI explosion stimulus before entity removal
//  * explosion AI stimulus: type 0x02000001, radius max(128, blast*1.5),
//    lifetime 1 second, channel 0
//
// Original source-grounded machine-code reports:
// research/phases/sin-original-ar-grenade-lifecycle-and-ammo.md
// research/phases/sin-original-ar-grenade-detonation-effects.md
// research/phases/sin-original-grenade-ai-danger-combat-sounds.md
//
// UNPROVEN: original projectile owner/velocity application, collision masks,
// precise think/detonation predicates, trace shape and decal selection, damage
// game-rules and network semantics, actual engine function signatures.
// Those details are HOST inputs, never invented by this adapter.
#include <cstdint>

namespace sin_compat::ar_grenade {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

inline constexpr const char* entity_classname = "grenade_ar";
inline constexpr const char* original_model = "models/weapons/grenades/ar_grenade.mdl";
inline constexpr float original_initial_damage = 100.0f;
inline constexpr float original_initial_blast_radius = 250.0f;
inline constexpr int original_initial_danger_radius = 100;
inline constexpr int original_danger_radius_increment = 15;
inline constexpr int original_danger_radius_comparison = 300;
inline constexpr float original_initial_think_delay = 0.1f;
inline constexpr float original_repeat_think_delay = 0.05f;
inline constexpr std::uint32_t original_damage_type_blast = 0x40u;
inline constexpr std::uint32_t original_ai_sound_danger = 0x08u;
inline constexpr std::uint32_t original_ai_sound_explosion = 0x02000001u;
inline constexpr float original_danger_sound_lifetime = 0.2f;
inline constexpr float original_blast_sound_lifetime = 1.0f;

struct AiSoundEvent {
    Vec3 origin{};
    std::uint32_t type = 0;
    float radius = 0.0f;   // Source-unit AI hearing radius, NOT audio gain
    float duration_seconds = 0.0f;
    int channel = 0;
};

struct BlastDamageEvent {
    Vec3 origin{};
    float damage = 0.0f;
    float radius = 0.0f;
    std::uint32_t damage_type = 0;
};

struct ScreenShakeEvent {
    Vec3 origin{};
    float requested_amplitude = 25.0f;
    float engine_clamped_amplitude = 16.0f;
    float frequency = 150.0f;
    float duration_seconds = 1.0f;
    float radius = 750.0f;
    // The original flat-screen shake MUST NOT be applied as tracked-HMD
    // displacement. An eventual VR host chooses comfort-safe feedback.
    bool move_tracked_hmd = false;
};

enum class Decal : std::uint8_t {
    None, Scorch, SmallScorch
};

// The actual original routine performs trace/effects before shake/damage and
// calls engine game-rules for RadiusDamage. These methods are CONTRACT hooks,
// not implementations of SourceVR/Quest callbacks or physics.
class IHost {
public:
    virtual ~IHost() = default;
    virtual void on_flight_ai_sound(const AiSoundEvent&) = 0;
    // The real trace decides Scorch vs SmallScorch; caller supplies decal.
    virtual void on_original_trace_effect(Decal) = 0;
    virtual void on_screen_shake_feedback(const ScreenShakeEvent&) = 0;
    virtual void on_radius_damage(const BlastDamageEvent&) = 0;
    virtual void on_explosion_ai_sound(const AiSoundEvent&) = 0;
    virtual void on_remove_projectile() = 0;
};

enum class StepResult : std::uint8_t {
    Applied, NotDue, Disarmed, TriggerRejected, InvalidInput
};

class Runtime {
public:
    // Creation leaves the projectile armed. The host owns physics, identity,
    // collision/trace decisions, game time and eventual engine entity removal.
    bool spawn(const Vec3& origin, const Vec3& velocity,
               float game_time_seconds) noexcept;
    bool update_kinematics(const Vec3& origin, const Vec3& velocity) noexcept;

    // "original_ai_predicate" and "original_increment_predicate" are supplied
    // by a future verified engine callback. We deliberately do NOT infer the
    // original think branches from the known numerical constants.
    StepResult think(float game_time_seconds, bool original_ai_predicate,
                     bool original_increment_predicate, IHost& host);

    // Only the engine can establish the original touch/think/physics trigger.
    // "approved" is external precisely because that predicate is unresolved.
    // One-shot guard is cleared BEFORE host callbacks for reentrancy safety.
    StepResult detonate(bool approved, Decal trace_selected_decal, IHost& host);

    // Original spawn values; a real skill/game-rules path may adjust these
    // ONLY after its original behavior is reconstructed separately.
    bool set_runtime_damage(float damage, float blast_radius) noexcept;

    bool armed() const noexcept { return armed_; }
    bool spawned() const noexcept { return spawned_; }
    int danger_radius() const noexcept { return danger_radius_; }
    float next_think_seconds() const noexcept { return next_think_at_; }
    float damage() const noexcept { return damage_; }
    float blast_radius() const noexcept { return blast_radius_; }
    Vec3 origin() const noexcept { return origin_; }
    Vec3 velocity() const noexcept { return velocity_; }

private:
    bool spawned_ = false;
    bool armed_ = false;
    Vec3 origin_{};
    Vec3 velocity_{};
    int danger_radius_ = original_initial_danger_radius;
    float next_think_at_ = 0.0f;
    float damage_ = original_initial_damage;
    float blast_radius_ = original_initial_blast_radius;
};

} // namespace sin_compat::ar_grenade
