#pragma once

// Independently authored portable subset of SiN Episodes (2006) CSE1Rules.
// Values and branch structure are transcribed from the preserved Phase 5
// static-disassembly report; this is not Ritual source or a Source ABI.

#include <cmath>
#include <cstdint>
#include <cstring>

namespace sin_compat::rules {

enum class Cse1ModifierIndex : std::uint8_t {
    DamageTakenScale = 0,
    DamageDoneScale = 1,
    AmmoQuantityScale = 2,
    ForcedHealthKits = 3,
    PercentHealthKitsDrop = 4,
    PercentNpcSecondaryFire = 5,
    PercentNpcMakeDifferent = 6,
    PercentNpcChangeSpawnCount = 7,
    PercentNpcWearsHelmet = 8,
    PercentNpcHelmetShotOff = 9,
    PercentNpcTakesCover = 10,
    RandomNpcWaitBeforeShoot = 11,
    PercentNpcMobile = 13
};

// CSE1Rules vtable slot 63, original server.dll VA 0x222D5420.
// The original code tests the damage-type field at +0x38 against this
// combined mask. If ANY masked bit is present, it skips DAMAGE_TAKEN_SCALE.
// Individual flag names are intentionally not guessed.
constexpr std::uint32_t kDamageTakenBypassMask = 0x80024021U;

// CSE1Rules vtable slot 62, original server.dll VA 0x222D47F0.
// The observed routine multiplies the passed float by DAMAGE_DONE_SCALE.
inline float apply_damage_done_scale(float damage,
                                     float modifier) noexcept {
    return damage * modifier;
}

// CSE1Rules vtable slot 63, original server.dll VA 0x222D5420.
// The input is the already-constructed damage amount and the raw 32-bit
// Source damage-type field. No clamp or per-flag interpretation is applied.
inline float apply_damage_taken_scale(float damage,
                                      std::uint32_t damage_type,
                                      float modifier) noexcept {
    if ((damage_type & kDamageTakenBypassMask) != 0U) return damage;
    return damage * modifier;
}

namespace detail {

// The original x86 routine uses 32-bit ADD/SUB on EAX. Use unsigned
// arithmetic for defined modulo-2^32 behavior, then preserve the resulting
// exact-width signed bit pattern. This targets the original x86 and Quest
// ARM64 two's-complement representations.
inline std::int32_t wrap_add_one(std::int32_t value) noexcept {
    static_assert(sizeof(std::int32_t) == sizeof(std::uint32_t),
                  "SiN spawn-count arithmetic requires 32-bit integers");
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    ++bits;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

inline std::int32_t wrap_sub_one(std::int32_t value) noexcept {
    static_assert(sizeof(std::int32_t) == sizeof(std::uint32_t),
                  "SiN spawn-count arithmetic requires 32-bit integers");
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    --bits;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

} // namespace detail

// CSE1Rules vtable slot 100, original server.dll VA 0x222D4A00.
// The caller supplies the random-float sample obtained from the engine RNG.
// If sample < abs(modifier), the original branch increments for positive
// modifiers or decrements for negative modifiers. Otherwise count is unchanged.
// The report does not establish the RNG provider's exact range or call sites.
inline std::int32_t adjust_npc_spawn_count(std::int32_t count,
                                           float modifier,
                                           float random_sample) noexcept {
    if (modifier != 0.0f && random_sample < std::fabs(modifier)) {
        return modifier > 0.0f ? detail::wrap_add_one(count)
                               : detail::wrap_sub_one(count);
    }
    return count;
}

// CSE1Rules vtable slot 107, original server.dll VA 0x222D4CF0.
// The observed routine adds RANDOM_NPC_WAIT_BEFORE_SHOOT to its float input.
inline float add_npc_wait_before_shoot(float base_wait,
                                       float modifier) noexcept {
    return base_wait + modifier;
}

} // namespace sin_compat::rules
