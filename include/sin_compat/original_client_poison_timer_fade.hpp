#pragma once
// Phase 154: clean C++17 reconstruction of CHudPoisonTimer secondary virtual
// slot 93, from original SHA-pinned SiN Episodes (2006) client.dll 0x24236320.
// All +offsets and x87 comparison paths were inspected in the original PE32.
// NOT the original game DLL, x86 layout, VGUI renderer, sound API, or timing.
#include "sin_compat/original_client_poison_timer_hud.hpp"
#include <cstdint>
#include <string_view>

namespace sin_compat::original_client_poison_hud {

enum class FadeCue : std::uint8_t { None, PoisonTimeFadeIn, PoisonTimeFadeOut };

inline constexpr std::string_view original_fade_token(FadeCue cue) noexcept {
    switch (cue) {
        case FadeCue::PoisonTimeFadeIn: return "PoisonTimeFadeIn";
        case FadeCue::PoisonTimeFadeOut: return "PoisonTimeFadeOut";
        default: return {};
    }
}

struct FadeUpdateResult {
    bool updated = false; // original method's early return guard
    FadeCue emitted = FadeCue::None;
};

// Original subobject field +0xA6 is compared with signed byte values 0 and
// 0xFF (i.e. -1) but never written by original 0x24236320. Its source and
// semantic name remain unknown, so the host supplies it explicitly.
//
// Recovered original execution order:
//  1. Missing local player -> return immediately.
//  2. FUCOMPP player+0x11EC vs HUD+0x10C; equal -> return immediately.
//     Unordered (NaN) is NOT considered equal.
//  3. For player value < 1.0 and status 0, emit FadeIn.
//  4. Otherwise check against original 0.25 threshold, zero, 1.0 and
//     status 0xFF; only ordered player >= 1.0 and 0xFF emits FadeOut.
//  5. Sound callbacks run BEFORE overwriting HUD+0x10C with 1.0-value.
//
// A real host must supply the original sound resolver+dispatch API. A host
// callback here receives the exact original sound token, not a sound asset.
template <typename EmitSoundToken>
FadeUpdateResult update_poison_timer_with_fades(State& state,
                                                bool has_local_player,
                                                float player_poison_timer,
                                                std::uint8_t original_status_byte,
                                                EmitSoundToken&& emit_sound) {
    if (!has_local_player || player_poison_timer == state.display_fraction) {
        return {};
    }
    FadeCue cue = FadeCue::None;
    if (player_poison_timer < 1.0f) {
        if (original_status_byte == 0U) {
            cue = FadeCue::PoisonTimeFadeIn;
        }
    } else if (!(player_poison_timer < 0.25f) &&
               player_poison_timer != 0.0f &&
               player_poison_timer >= 1.0f &&
               original_status_byte == 0xFFU) {
        cue = FadeCue::PoisonTimeFadeOut;
    }
    if (cue != FadeCue::None) {
        emit_sound(original_fade_token(cue));
    }
    state.display_fraction = static_cast<float>(
        1.0 - static_cast<double>(player_poison_timer));
    return {true, cue};
}

} // namespace sin_compat::original_client_poison_hud
