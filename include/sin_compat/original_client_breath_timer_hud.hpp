#pragma once
// Phase155: evidence-backed independent C++17 model of CHudBreathTimer
// from original SHA-pinned SiN Episodes: Emergence 2006 client.dll.
// This is NOT a binary-compatible 2006 MSVC x86 VGUI or game DLL class.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sin_compat::original_client_breath_hud {

struct State {
    float previous_breath_value = 0.0f; // original HUD +0xEC
    std::uint8_t shared_status_byte = 0; // original HUD +0xA6, meaning not recovered
    std::uintptr_t first_paint_resource = 0; // original HUD +0xF0
    std::uintptr_t second_paint_resource = 0; // original HUD +0xF4
    float reset_value = 0.0f; // original HUD +0x100
    std::array<std::uintptr_t, 2> resolved_resources{}; // +0x104/+0x108
};

// Original primary virtual slot 2 at 0x24230A50 sets +0x100 = 1.0f.
inline void reset_slot(State& state) noexcept { state.reset_value = 1.0f; }

// Original primary virtual slot 3 at 0x24230A60 invokes 0x241442C0
// twice, resolving stored +0xBC and +0xDD names in that order.
template <typename Resolve>
void bind_two_resources(State& state,
                        const std::array<std::string_view, 2>& names,
                        Resolve&& resolve) {
    for (std::size_t i = 0; i < names.size(); ++i)
        state.resolved_resources[i] = resolve(names[i]);
}

// Original panel slot 109 at 0x24230C90 calls inherited paint first,
// then tests +0xA6 != 0, +0xF0 != 0 and +0xF4 != 0 in that order.
// Actual VGUI geometry, numerical conversion and rendering are NOT modeled.
inline bool should_paint_after_base(const State& state) noexcept {
    return state.shared_status_byte != 0 &&
           state.first_paint_resource != 0 &&
           state.second_paint_resource != 0;
}

enum class BreathSoundCue : std::uint8_t {
    None, FadeIn, Low, Empty, FadeOut
};
inline constexpr std::string_view original_sound_token(
    BreathSoundCue cue) noexcept {
    switch (cue) {
        case BreathSoundCue::FadeIn: return "BreathTimeFadeIn";
        case BreathSoundCue::Low: return "BreathTimeLow";
        case BreathSoundCue::Empty: return "BreathTimeEmpty";
        case BreathSoundCue::FadeOut: return "BreathTimeFadeOut";
        default: return {};
    }
}
struct UpdateResult {
    bool updated = false;
    BreathSoundCue event = BreathSoundCue::None;
};

// Original panel vtable slot93, function 0x24230BB0 (224 bytes).
// Input is the m_HL2Local.m_breathTimer networked float at
// client C_HL2MP_Player+0x11F0. Server's corresponding original field
// resides at +0x13A4. The class names and offsets are NOT portable ABI.
//
// Original control-flow order (x87 FCOMP/FUCOMPP/FNSTSW+TEST AH):
//  - No player or incoming == previous -> early return, no write.
//  - Incoming < 1.0 and status byte == 0 -> FadeIn.
//  - Otherwise incoming < 0.25 -> Low (regardless of status).
//  - Otherwise incoming == 0.0 -> Empty (a presently unreachable
//    branch for ordinary ordered floats, because 0.0 < 0.25).
//  - Otherwise incoming == 1.0 and status byte > 0 -> FadeOut.
//  - Emit token through original sound singleton / helper BEFORE
//    writing the unmodified incoming float to HUD+0xEC.
// Unordered comparisons (NaN) skip all sound cases but still update.
// This model deliberately avoids imaginary sound engine callbacks.
template <typename EmitSoundToken>
UpdateResult update_from_original_player(State& state,
                                         bool has_local_player,
                                         float incoming_breath_value,
                                         EmitSoundToken&& emit_sound) {
    if (!has_local_player ||
        incoming_breath_value == state.previous_breath_value)
        return {};

    BreathSoundCue cue = BreathSoundCue::None;
    if (incoming_breath_value < 1.0f && state.shared_status_byte == 0U) {
        cue = BreathSoundCue::FadeIn;
    } else if (incoming_breath_value < 0.25f) {
        cue = BreathSoundCue::Low;
    } else if (incoming_breath_value == 0.0f) {
        cue = BreathSoundCue::Empty;
    } else if (incoming_breath_value == 1.0f &&
               state.shared_status_byte > 0U) {
        cue = BreathSoundCue::FadeOut;
    }
    if (cue != BreathSoundCue::None)
        emit_sound(original_sound_token(cue));
    state.previous_breath_value = incoming_breath_value;
    return {true, cue};
}

} // namespace sin_compat::original_client_breath_hud
