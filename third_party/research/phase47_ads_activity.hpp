#pragma once
// Clean-room behavior model of SiN: Emergence activity selection (Phase 47).
// This is not Ritual's original source code, nor a working engine integration.

#include <cstdint>

namespace sin_re_phase47 {

enum class WeaponKind : std::uint8_t {
    Magnum, AssaultRifle, Scattergun,
};

enum class Activity : int {
    Idle = 0x9d,
    PrimaryAttack = 0xa4,
    SecondaryAttack = 0xa5,
    Recoil1 = 0xbb,
    Recoil2 = 0xbc,
    Recoil3 = 0xbd,
    ZoomedIdle = 0xc0,
    ZoomedPrimaryAttack = 0xc1,
    ZoomedSecondaryAttack = 0xc2,
};

// Binary: weapon+0x578.  Nonzero means zoomed.
constexpr Activity selectPrimaryActivity(WeaponKind weapon, bool zoomed,
                                          int rifleCounter = 0) noexcept {
    // The assault rifle has a unique vtable slot 228 implementation.
    if (weapon == WeaponKind::AssaultRifle) {
        if (zoomed) return Activity::ZoomedPrimaryAttack;
        if (rifleCounter < 2) return Activity::PrimaryAttack;
        if (rifleCounter == 2) return Activity::Recoil1;
        if (rifleCounter == 3) return Activity::Recoil2;
        return Activity::Recoil3;
    }
    // Both the Magnum and Scattergun inherit the base +0x1d mapping.
    return zoomed ? Activity::ZoomedPrimaryAttack : Activity::PrimaryAttack;
}

// All three weapons inherit vtable slot 229 from the same base class.
// This is activity selection only; it does not prove the weapon can actually
// execute a secondary attack.
constexpr Activity selectSecondaryActivity(bool zoomed) noexcept {
    return zoomed ? Activity::ZoomedSecondaryAttack : Activity::SecondaryAttack;
}

constexpr bool isZoomActivity(Activity a) noexcept {
    return a == Activity::ZoomedIdle ||
           a == Activity::ZoomedPrimaryAttack ||
           a == Activity::ZoomedSecondaryAttack;
}

// A narrow model of the *zoomed* branch at server.dll 0x222cb960.
// The original code calls a weapon virtual at +0x320 for its decision.
// We intentionally leave the callee's semantic name unassigned here.
constexpr bool requestsZoomedIdle(bool zoomed, Activity currentActivity,
                                   bool virtual320Result) noexcept {
    if (!zoomed) return false; // other, non-zoom branch not modeled
    if (!isZoomActivity(currentActivity)) return true;
    return virtual320Result;
}

// The entry helper requests the zoomed idle activity.  The exit helper
// only requests the normal idle activity if currentActivity == ZoomedIdle.
constexpr Activity enterZoomActivity() noexcept { return Activity::ZoomedIdle; }
constexpr bool exitRequestsIdle(Activity currentActivity) noexcept {
    return currentActivity == Activity::ZoomedIdle;
}

}  // namespace sin_re_phase47
