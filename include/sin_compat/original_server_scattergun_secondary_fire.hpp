#pragma once
// Phase167: host-neutral branch/callback model reconstructed from exact original
// 2006 SiN Episodes server.dll at 0x2231C810 and 0x222CBC20.
// This is NOT a native MSVC x86 weapon object, nor a projectile/trace port.
#include "sin_compat/original_server_scattergun_ammo.hpp"
#include "sin_compat/original_server_scattergun_deferred.hpp"
#include <cstdint>

namespace sin_compat::original_server_scattergun_secondary_fire {
namespace ammo = original_server_scattergun_ammo;
namespace timer = original_server_scattergun_deferred;

// Vectors at 0x222CBC20 are copied as three *raw* DWORDs. The layout
// preserves bit representations rather than performing float arithmetic.
struct NativeVector3 { std::uint32_t words[3]{}; };
inline constexpr std::uint32_t kOriginalVectorArgumentBits = 0x3DB27EB6u;
inline constexpr std::uint32_t kSpreadPatternAddress = 0x2252E888u;
inline constexpr float kPelletDistance = 1500.0f;
inline constexpr std::uint32_t kPelletCount = 8u;

// Original shared vector acquisition helper 0x222CBC20 (225 native bytes).
// Host handles the engine's owner/candidate object lookups and virtual
// +0x120/+0x358/+0x514/+0x734 calls. The original output bytes are copied
// only on success; failure must leave both output vectors untouched.
template<class Host>
bool original_obtain_fire_vectors(NativeVector3& first,
                                  NativeVector3& second, Host& host) {
    const auto owner = host.find_owner_for_vectors(); // 0x222AD370
    if (!owner) return false;
    if (host.owner_uses_direct_vector(owner)) { // owner vtable+0x120
        first = host.owner_origin_vector(owner); // owner vtable+0x358
        second = host.owner_offset_vector(owner, kOriginalVectorArgumentBits);
        return true; // owner vtable+0x514
    }
    const auto candidate = host.find_alternative_vector_source(); // 0x22139F60
    if (!candidate) return false;
    first = host.owner_origin_vector(owner); // owner vtable+0x358
    second = host.candidate_vector(candidate, first); // vtable+0x734
    return true;
}

struct SecondaryState {
    float gate_timer_49c = 0.0f;
    float other_timer_4a0 = 0.0f;
    std::uint32_t timing_arg_3ec = 0;
    std::uint32_t alternate_ammo_type_4e8 = 0;
    std::uint32_t observed_secondary_584 = 0;
    timer::ChangeNotification notify{};
    ammo::SecondaryAmmoConsumption clip{};
};

enum class Outcome : std::uint8_t {
    MissingOwner,
    OwnerModeCooldown,
    NoFireVectors,
    EmptyTrackedClip,
    Fired,
};

// Original ScatterGun vtable secondary-fire function 0x2231C810 (612 bytes).
// Engine callbacks and 8x pellet geometry/trace pipeline are intentionally
// opaque host operations. Only verified control flow, call ordering, constant
// arguments, signed clip gate and tracked timer effects are represented.
template<class Host>
Outcome original_scattergun_secondary_fire(SecondaryState& state, Host& host) {
    const auto owner = host.find_owner(); // 0x222AD370
    if (!owner) return Outcome::MissingOwner;
    const bool owner_can_animate = host.owner_action_capable(owner); // v+0x120
    if (host.owner_mode_298(owner) == 3u) {
        host.dispatch_state_event(0u, 0u); // weapon v+0x3C4
        const float now = host.read_game_time();
        // Native x87 FADD uses the 64-bit 0.2 literal at 0x223D8FD0.
        const float when = static_cast<float>(static_cast<double>(now) + 0.2);
        timer::original_tracked_float_assign(state.other_timer_4a0,
                                              when, state.notify);
        return Outcome::OwnerModeCooldown;
    }
    if (!host.acquire_fire_vectors()) // shared 0x222CBC20
        return Outcome::NoFireVectors;
    // The first +0x43C virtual check occurs before firing; its return
    // can differ from a second check after animation and owner callbacks.
    if (host.weapon_uses_secondary_clip() &&
        (state.clip.secondary_clip_4f0 == 0u ||
         (state.clip.secondary_clip_4f0 & 0x80000000u) != 0u)) {
        host.weapon_empty_callback(); // weapon v+0x380
        return Outcome::EmptyTrackedClip;
    }
    host.dispatch_state_event(3u, 0u); // v+0x3C4
    const std::uint32_t selected = host.weapon_fire_selection(); // v+0x394
    host.dispatch_selection(selected); // v+0x314
    const float interval = host.timing_interval(state.timing_arg_3ec);
    const float first_time = host.read_game_time();
    timer::original_tracked_float_assign(state.gate_timer_49c,
                                          interval + first_time, state.notify);
    const float second_time = host.read_game_time();
    timer::original_tracked_float_assign(state.other_timer_4a0,
                                          interval + second_time, state.notify);
    host.owner_fire_callback(owner); // owner v+0x3D4, unconditional
    if (owner_can_animate) {
        host.owner_animation(owner, 5u); // owner v+0x484
        if (host.weapon_uses_secondary_clip()) {
            ammo::original_scattergun_consume_secondary_ammo(state.clip);
        } else {
            host.owner_consume_ammo(owner, 1u,
                                    state.alternate_ammo_type_4e8); // 0x222AA670
        }
        const auto ammo_type = host.weapon_secondary_ammo_type(); // v+0x450
        state.observed_secondary_584 =
            host.owner_ammo_count(owner, ammo_type); // 0x222AA770
        host.weapon_post_fire(); // v+0x3A0
        host.weapon_feedback(6u); // 0x222AAFE0
    }
    host.prepare_pellet_frame(); // 0x22033250
    for (std::uint32_t i=0; i<kPelletCount; ++i) {
        // Encapsulates original 0x22033280, 0x22269E50, 0x222AD370,
        // 0x2231B9F0, 3x x87 multiply by 1500 and 0x22141A00.
        // No geometry or hit semantics are asserted for this callback.
        host.process_original_pellet(i, kSpreadPatternAddress,
                                     1.0f, kPelletDistance);
    }
    return Outcome::Fired;
}
} // namespace sin_compat::original_server_scattergun_secondary_fire
