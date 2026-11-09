#pragma once
// Independently authored, evidence-bounded reconstruction of the original
// SiN CNPC_GruntBase grenade-kick eligibility, task start and animation event 3.
// Original PE32 offsets are evidence only; the host supplies all engine objects.
#include "sin_compat/original_grunt_relationship.hpp"
#include "sin_compat/original_grunt_schedule_selector.hpp"
#include <cstdint>
#include <optional>
namespace sin_compat::ai::grunt {
inline constexpr std::uint32_t original_grunt_task_kick_attack = 0x9AU;
inline constexpr std::uint32_t original_grunt_schedule_kick_grenade = 0x73U;
inline constexpr std::uint32_t original_grunt_kick_animation_event = 3U;
inline constexpr std::uint32_t original_grunt_kick_trace_mask_npc_solid = 0x0202400BU;
inline constexpr std::uint32_t original_grunt_kick_x86_add_velocity_slot = 44U;
inline constexpr std::uint32_t original_grunt_kick_x86_add_velocity_byte_offset = 0xB0U;
inline constexpr float original_grunt_kick_vertical_tolerance = 12.0F;
inline constexpr float original_grunt_kick_radius_scale = 0.5F;
inline constexpr float original_grunt_kick_alignment_threshold = 0.8F;
inline constexpr float original_grunt_kick_minimum_fuse_seconds = 0.5F;
inline constexpr float original_grunt_kick_default_fraggrenade_radius = 250.0F;
inline constexpr std::int32_t original_grunt_kick_default_damage = 10;
inline constexpr float original_grunt_kick_linear_speed = 800.0F;
inline constexpr float original_grunt_kick_upward_velocity = 200.0F;
inline constexpr float original_grunt_kick_spin_x_min = -360.0F;
inline constexpr float original_grunt_kick_spin_x_max = 360.0F;
inline constexpr float original_grunt_kick_spin_y_min = -180.0F;
inline constexpr float original_grunt_kick_spin_y_max = 180.0F;
inline constexpr float original_grunt_kick_spin_z = 20.0F;
inline constexpr char original_grunt_kick_melee_sound[] = "NPC_Grunt.WeaponBash";
struct OriginalGruntKickVector3 { float x = 0.0F; float y = 0.0F; float z = 0.0F; };
enum class OriginalGruntKickEligibilityFailure : std::uint8_t {
    None, MissingEnemy, MissingTrackedGrenade, VerticalSeparation, Distance,
    DirectionAlignment, ObstructedTrace, WrongGrenadeClass, GrenadeNotAlive, FuseTooShort,
};
struct OriginalGruntKickEligibilityInput {
    bool enemy_resolves = false;
    bool tracked_grenade_resolves = false;
    bool candidate_is_base_grenade = false;
    std::int32_t grenade_life_state = 0;
    OriginalGruntKickVector3 grunt_origin{};
    OriginalGruntKickVector3 grenade_origin{};
    float fraggrenade_radius = original_grunt_kick_default_fraggrenade_radius;
    // Original horizontal dot-product operands remain unresolved; host supplies the computed value.
    float horizontal_alignment_dot = 1.0F;
    // Host supplies UTIL_TraceLine's fraction using MASK_NPCSOLID.
    float npc_solid_trace_fraction = 1.0F;
    float grenade_detonate_time = 0.0F;
    float current_time = 0.0F;
};
struct OriginalGruntKickEligibilityResult {
    OriginalGruntKickEligibilityFailure failure = OriginalGruntKickEligibilityFailure::None;
    float measured_distance = 0.0F;
    float maximum_distance = 0.0F;
    float remaining_fuse = 0.0F;
    bool eligible() const noexcept { return failure == OriginalGruntKickEligibilityFailure::None; }
};
OriginalGruntKickEligibilityResult evaluate_original_grunt_kick_eligibility(
    const OriginalGruntKickEligibilityInput& input) noexcept;
struct OriginalGruntKickState {
    std::optional<OriginalGruntEntityToken> current_grenade{};
    std::optional<OriginalGruntEntityToken> physics_entity{};
    float last_kick_task_time = 0.0F;
    // Host supplies the original sk_grunt_kick ConVar after reproducing its float-to-int conversion.
    std::int32_t kick_damage = original_grunt_kick_default_damage;
};
std::optional<std::uint32_t> prepare_original_grunt_kick_schedule(
    OriginalGruntKickState& state, const OriginalGruntKickEligibilityInput& input) noexcept;
class IOriginalGruntKickTaskHost {
public:
    virtual ~IOriginalGruntKickTaskHost() = default;
    virtual void set_activity(std::uint32_t activity_id) noexcept = 0;
};
void start_original_grunt_kick_task(
    OriginalGruntKickState& state, float current_time, std::uint32_t melee_kick_activity_id,
    IOriginalGruntKickTaskHost& host) noexcept;
enum class OriginalGruntKickPhysicsResolution : std::uint8_t { MissingEntity, MissingPhysicsObject, Ready };
using OriginalGruntPhysicsObjectToken = std::uint64_t;
enum class OriginalGruntKickEventResult : std::uint8_t {
    NotHandled, PhysicsVelocityApplied, MeleeFallbackDispatched,
    MissingPhysicsObjectUnresolved, ZeroLengthDirectionUnresolved,
};
class IOriginalGruntKickEventHost {
public:
    virtual ~IOriginalGruntKickEventHost() = default;
    // Resolves the EHANDLE, entity physics pointer (+0x218), and the two original event-3 position operands.
    // The second position is likely the current enemy, but that identity is not treated as proven here.
    virtual OriginalGruntKickPhysicsResolution resolve_physics_object(
        OriginalGruntEntityToken entity, OriginalGruntPhysicsObjectToken& physics_object,
        OriginalGruntKickVector3& grenade_origin,
        OriginalGruntKickVector3& related_target_origin) noexcept = 0;
    virtual float random_float(float minimum, float maximum) noexcept = 0;
    // Host maps this to its actual physics ABI. Never use the old x86 0xB0 offset on ARM64.
    virtual void add_velocity(OriginalGruntPhysicsObjectToken physics_object,
        const OriginalGruntKickVector3& linear_velocity,
        const OriginalGruntKickVector3& angular_velocity) noexcept = 0;
    virtual void log_missing_physics_entity() noexcept = 0;
    virtual void log_missing_physics_object() noexcept = 0;
    // Trace parameters and damage-info helper remain engine-owned/unresolved.
    virtual void perform_melee_fallback(std::int32_t damage, const char* sound_name) noexcept = 0;
    virtual void clear_physics_entity_handle() noexcept = 0;
    virtual void record_last_anim_event_handled(std::uint32_t event_id) noexcept = 0;
};
OriginalGruntKickEventResult handle_original_grunt_kick_animation_event(
    OriginalGruntKickState& state, std::uint32_t event_id,
    IOriginalGruntKickEventHost& host) noexcept;

// Couples the recovered KickableGrenade selector branch to eligibility and
// handle transfer. A rejected candidate lets the selector continue priorities.
std::optional<OriginalGruntScheduleId> select_verified_grunt_schedule_with_kick_gate(
    GruntSelectorState& selector_state,
    IGruntSelectorExtension& extension,
    OriginalGruntKickState& kick_state,
    const OriginalGruntKickEligibilityInput& eligibility_input);
} // namespace sin_compat::ai::grunt
