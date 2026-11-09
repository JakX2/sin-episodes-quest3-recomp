#pragma once

// Independently authored reconstruction of the verified SiN Episodes (2006)
// FireBullets target-filter and TraceAttack dispatch boundary. The host owns
// all native Source objects and forwards its own damage-info payload.

#include "sin_compat/weapon_entity_io.hpp"

#include <cstdint>

namespace sin_compat::combat {

inline constexpr std::uint32_t kBaseDamageFilterRejectExactTypeA = 0x00400000U;
inline constexpr std::uint32_t kBaseDamageFilterRejectExactTypeB = 0x00010000U;

// These are concrete classes whose original RTTI/vtables were checked by the
// preserved Phase 42 report. Unknown classes are deliberately not guessed.
enum class BulletTargetClass : std::uint8_t {
    BaseEntity,
    BaseAnimating,
    BaseCombatCharacter,
    BasePlayer,
    Hl2Player,
    PhysicsProp,
    PropContainer,
    Unknown
};

enum class DamageFilterRoute : std::uint8_t {
    BaseEntity,
    Hl2PlayerOverride
};

enum class TraceAttackRoute : std::uint8_t {
    BaseEntity,
    BasePlayer,
    PhysicsPropOrContainerCopyThenBase
};

// The raw damage-type field is the only part read by this reconstruction.
// host_payload is borrowed and opaque: it may point at the host's native
// CTakeDamageInfo-like object, but this library never dereferences or retains it.
struct DamageInfoView {
    std::uint32_t damage_type = 0;
    const void* host_payload = nullptr;
};

enum class DamageFilterLookup : std::uint8_t {
    NoFilter,
    LiveFilter,
    ReadFailed
};

enum class BulletTargetDispatchStatus : std::uint8_t {
    TraceAttackDispatched,
    StaleTarget,
    UnsupportedTargetClass,
    RejectedByExactDamageType,
    RejectedByDamageFilter,
    FilterLookupFailed,
    FilterInvocationFailed,
    TraceAttackFailed
};

enum class DamageFilterRejection : std::uint8_t {
    None,
    Hl2PlayerExactTypeA,
    BaseEntityExactTypeA,
    BaseEntityExactTypeB,
    GameFilter
};

struct BulletTargetDispatchResult {
    BulletTargetDispatchStatus status =
        BulletTargetDispatchStatus::UnsupportedTargetClass;
    DamageFilterRoute filter_route = DamageFilterRoute::BaseEntity;
    TraceAttackRoute trace_route = TraceAttackRoute::BaseEntity;
    DamageFilterRejection rejection = DamageFilterRejection::None;
    EntityRef filter_entity{};
    bool filter_invoked = false;
    bool trace_attack_invoked = false;
};

class IOriginalBulletTargetHost {
public:
    virtual ~IOriginalBulletTargetHost() = default;
    virtual bool is_live(EntityRef entity) const = 0;

    // Return NoFilter for an empty or stale native handle. Return LiveFilter
    // only with a generation-valid filter entity. ReadFailed is a host error.
    virtual DamageFilterLookup resolve_damage_filter(
        EntityRef target, EntityRef& filter) const = 0;

    // Mirrors the resolved filter entity's PassesDamageFilter virtual call.
    virtual bool invoke_damage_filter(EntityRef filter,
                                      EntityRef target,
                                      const DamageInfoView& damage,
                                      bool& passes) = 0;

    // The host maps this route to its own native engine method. In particular,
    // PhysicsProp/PropContainer copy the damage-info value before forwarding
    // to the common base TraceAttack implementation.
    virtual bool trace_attack(EntityRef target,
                              TraceAttackRoute route,
                              const DamageInfoView& damage) = 0;
};

BulletTargetDispatchResult dispatch_original_bullet_target_hit(
    IOriginalBulletTargetHost& host,
    EntityRef target,
    BulletTargetClass target_class,
    const DamageInfoView& damage);

} // namespace sin_compat::combat
