#pragma once
// Evidence-bounded model of 0x222CCD40 up to 0x222CCDE6, not Ritual source.
// Original x86 DLLs are not shipped; binary findings were verified against the 2006 DLLs in Phase 46.
#include <cstdint>

namespace sin_phase45 {
constexpr std::uint32_t kZoomMask = 0x00080000u;
constexpr float kZoomInFovSeconds = 0.25f;
constexpr float kZoomOutFovSeconds = 0.15f;
constexpr int kZoomInActivityArgument = 0xC0;
constexpr int kConditionalZoomOutActivityArgument = 0x9D;
constexpr unsigned kAnimationVirtualSlot = 0x314 / 4;  // 197 (x86)
constexpr unsigned kFovGetterVirtualSlot = 0x440 / 4;   // 272 (x86)

struct WeaponState {
    bool zoom_active_0x578 = false;
    bool restore_pending_0x579 = false;
};

// The names identify observed checks, not verified original member names.
struct Inputs {
    bool has_player = true;               // result of 0x222AD370
    bool player_vfunc_0x120 = true;       // a prerequisite, not yet named
    bool weapon_flag_0x4AC = false;
    bool player_vfunc_0x3C8 = false;
    std::uint32_t player_bits_at_0x93C = 0;
};

enum class Transition { None, Enter, Exit };
struct Result {
    Transition transition = Transition::None;
    bool eligible = false;
    bool automatic_restore = false;
    bool interrupted = false;
};

inline Result step(WeaponState& state, const Inputs& in) {
    Result out{};
    // 0x222CCD51 -> 0x222CCD65: early exits if these checks fail.
    if (!in.has_player || !in.player_vfunc_0x120) return out;
    out.eligible = !in.weapon_flag_0x4AC && !in.player_vfunc_0x3C8;

    // 0x222CCD8E through 0x222CCDA4: active + disallowed -> disable, remember.
    if (state.zoom_active_0x578 && !out.eligible) {
        state.zoom_active_0x578 = false;
        state.restore_pending_0x579 = true;
        out.transition = Transition::Exit;
        out.interrupted = true;
        return out;
    }

    // 0x222CCDAA: if disallowed and already inactive, do nothing.
    if (!out.eligible) return out;

    // 0x222CCDAE through 0x222CCDC5: automatically restore, no zoom bit test.
    if (!state.zoom_active_0x578 && state.restore_pending_0x579) {
        state.zoom_active_0x578 = true;
        state.restore_pending_0x579 = false;
        out.transition = Transition::Enter;
        out.automatic_restore = true;
        return out;
    }

    // 0x222CCDCB: a bit test in a player-owned field at +0x93E.
    // Phase 46 confirmed that +0x93C contains newly pressed button edges.
    if ((in.player_bits_at_0x93C & kZoomMask) != 0) {
        out.transition = state.zoom_active_0x578 ? Transition::Exit : Transition::Enter;
        state.zoom_active_0x578 = !state.zoom_active_0x578;
    }
    return out;
}

// Side effects of helpers at 0x222CBB60/0x222CBBC0 that the excerpt supports.
// The actual player FOV call is additionally guarded inside each helper.
struct HelperEffects {
    int activity_argument = -1; // -1 means no observed activity request
    int requested_fov = -1;     // -1 = weapon-specific ZoomFOV from virtual slot 272
    float fov_seconds = 0.f;
};
inline HelperEffects enter_effects() {
    return {kZoomInActivityArgument, -1, kZoomInFovSeconds};
}
inline HelperEffects exit_effects(bool current_activity_is_0xC0) {
    return {current_activity_is_0xC0 ? kConditionalZoomOutActivityArgument : -1,
            0, kZoomOutFovSeconds};
}
} // namespace sin_phase45
