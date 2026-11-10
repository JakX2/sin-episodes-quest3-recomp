#pragma once
// Independent original SiN Episodes: Emergence (2006) client.dll reconstruction.
// SHA-pinned CHudPoisonTimer original methods: 0x242360B0, 0x242360C0;
// verified selection/update portions of 0x24236120 and 0x24236320.
// Deliberately NOT the original 32-bit C++ object layout or VGUI ABI.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sin_compat::original_client_poison_hud {

// The offsets below document observations in the original PE32 class;
// this portable state intentionally DOES NOT use the original class layout.
struct State {
    float display_fraction = 0.0f;    // original this+0x10C
    std::uint32_t paint_enabled = 0;  // original this+0x110
    std::uintptr_t positive_resource = 0; // original this+0x114
    std::uintptr_t nonpositive_resource = 0; // original this+0x118
    std::uint32_t reset_word = 0;     // original this+0x120
    std::array<std::uintptr_t, 3> resolved_resources{}; // +0x124/128/12C
};

// Original 0x242360B0: exactly one 32-bit zero-store at +0x120.
inline void reset_slot(State& state) noexcept {
    state.reset_word = 0;
}

// Original 0x242360C0 calls helper 0x241442C0 three times in order,
// with char arrays at +0xBC, +0xDD and +0xFE. Host supplies the names
// and resolver; 2006 VGUI resource identity/ownership is not presumed.
template <typename Resolver>
void bind_three_resources(State& state,
                          const std::array<std::string_view, 3>& names,
                          Resolver&& resolve) {
    for (std::size_t i = 0; i < names.size(); ++i) {
        state.resolved_resources[i] = resolve(names[i]);
    }
}

struct PaintSelection {
    bool should_draw = false;
    std::uintptr_t selected_resource = 0;
};

// Original 0x24236120 (secondary CHudPoisonTimer vtable slot 109):
// FLD +0x10C / FCOMP 0.0; positive ordered values use +0x114,
// equal, negative and unordered values use +0x118.
// Subsequent original code rejects a zero +0x110 and a null resource.
// The original inherited paint, VGUI geometry, text formatting and
// actual font draw at 0x24154D50 are NOT reconstructed by this selector.
inline PaintSelection choose_paint_resource(const State& state) noexcept {
    const auto choice = state.display_fraction > 0.0f
        ? state.positive_resource
        : state.nonpositive_resource;
    return {state.paint_enabled != 0 && choice != 0, choice};
}

// Original 0x24236320 (secondary vtable slot 93) reads a local-player
// float at player+0x11EC and, if player exists, eventually stores
// double 1.0 minus that float at HUD+0x10C. The original routine also
// branches to PoisonTimeFadeIn/PoisonTimeFadeOut event callbacks; those
// engine-specific audio side effects are modeled separately in original_client_poison_timer_fade.hpp. This helper omits audio but preserves the verified early-exit equality guard.
inline bool update_display_from_player(State& state, bool has_player,
                                       float original_player_value) noexcept {
    if (!has_player || original_player_value == state.display_fraction) return false;
    state.display_fraction = static_cast<float>(
        1.0 - static_cast<double>(original_player_value));
    return true;
}

} // namespace sin_compat::original_client_poison_hud
