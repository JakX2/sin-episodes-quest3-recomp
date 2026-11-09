#pragma once

// Independently authored portable reconstruction of the verified common
// CBaseCombatCharacter alive-health reduction stage in SiN Episodes (2006).
// This is not a Source ABI, damage filter, armor model, or complete TakeDamage.

#include "sin_compat/weapon_entity_io.hpp" // Shared generation-aware host handle

#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_compat::combat {

enum class DamageClass : std::uint8_t {
    Unknown,
    CombatCharacter,
    BasePlayer,
    Hl2Player,
    AiBaseNpc,
    RecombinantMajor,
    RecombinantMinor
};

enum class DamageStage : std::uint8_t {
    Hl2PlayerTakeDamage,
    BasePlayerTakeDamage,
    CombatCharacterTakeDamage,
    BasePlayerAliveDamage,
    AiBaseNpcAliveDamage,
    RecombinantMajorAliveDamage,
    RecombinantMinorAliveDamage,
    MutantBaseAliveDamage,
    CommonHealthReduction
};

struct DamageRoute {
    std::array<DamageStage, 5> stages{};
    std::size_t count = 0;
};

// Returns only call-chain stages established by the SHA-pinned original
// server.dll evidence. Unknown subclasses are not guessed.
DamageRoute verified_damage_route(DamageClass damage_class) noexcept;

enum class DamageStatus : std::uint8_t {
    Applied,
    StaleVictim,
    UnsupportedClass,
    InvalidDamage,
    ReadFailed,
    AlreadyDead,
    HealthOverflow,
    WriteFailed,
    KilledDispatchFailed
};

struct DamageResult {
    DamageStatus status = DamageStatus::InvalidDamage;
    std::int32_t old_health = 0;
    std::int32_t integer_damage = 0;
    std::int32_t new_health = 0;
    DamageRoute route{};
    bool killed_dispatch_attempted = false;
    bool killed_dispatch_succeeded = false;
};

class IOriginalCombatDamageHost {
public:
    virtual ~IOriginalCombatDamageHost() = default;
    virtual bool is_live(EntityRef victim) const = 0;
    // Maps to host-owned typed health/life-state APIs, never original x86 offsets.
    // The runtime treats nonzero life_state as already dead; this is a host
    // safety gate, not a claim that every original caller repeats this check.
    virtual bool read_health_and_life_state(EntityRef victim,
                                            std::int32_t& health,
                                            std::uint8_t& life_state) const = 0;
    virtual bool write_health(EntityRef victim, std::int32_t health) = 0;
    // Dispatch the recipient's actual death virtual (+0xE8 in the original
    // x86 class layout). The host chooses the real subclass implementation.
    virtual bool dispatch_killed(EntityRef victim, DamageClass damage_class) = 0;
};

// Applies only the common alive-health reduction stage at original VA
// 0x222A7B20. damage must already have passed the caller's damage filters,
// game rules, armor, physics replay and any class-specific modifiers.
// The original x87 control word requests truncation toward zero before the
// signed integer is subtracted from the entity's integer health field.
DamageResult apply_common_alive_damage(IOriginalCombatDamageHost& host,
                                       EntityRef victim,
                                       float damage,
                                       DamageClass damage_class);

} // namespace sin_compat::combat
