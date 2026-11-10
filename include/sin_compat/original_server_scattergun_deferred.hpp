#pragma once
// Phase166: original 2006 SiN Episodes server.dll ScatterGun deferred
// ammo event and its three engine shared tracked-float assignments.
// Complete valid-state decision order reconstructed from x86, but
// native MSVC object layout, animation, audio, player acquisition,
// time source, callbacks and Source networking remain host-provided.
#include <cstdint>

namespace sin_compat::original_server_scattergun_deferred {

// Original shared value setters at 0x2203D190, 0x22173260 and 0x221732A0.
// Their FUCOMPP / FNSTSW / TEST AH,0x44 / JNP sequence performs *no*
// store for numerically equal float32 values (+0 and -0 compare equal).
// NaN compares unordered and ALWAYS takes the changed-value path.
// When changed, original mode byte +0x1F0 routes notification either
// to the dirty byte at +0x200 or bit 0 in the external pointer at +0x16C.
struct ChangeNotification {
    std::uint8_t mode_byte_1f0 = 0;
    std::uint8_t dirty_byte_200 = 0;
    std::uint32_t* external_flags_16c = nullptr;
};
inline bool original_tracked_float_assign(
    float& stored, float proposed, ChangeNotification& notify) noexcept {
    if (stored == proposed) return false;
    if (notify.mode_byte_1f0 != 0) {
        notify.dirty_byte_200 = 1;
    } else if (notify.external_flags_16c != nullptr) {
        *notify.external_flags_16c |= 1u;
    }
    stored = proposed;
    return true;
}

// The original owner/weapon fields are NOT consecutive standalone floats
// in an ABI-safe host structure. These are observed values at their
// exact original 32-bit object offsets, represented as portable state.
struct OwnerState {
    float float_timer_60c = 0.0f;
    ChangeNotification notify{};
};
struct ScatterGunState {
    std::uint8_t pending_byte_589 = 0;
    float gate_timer_49c = 0.0f;
    float mirrored_timer_4a0 = 0.0f;
    std::uint32_t primary_clip_4ec = 0;
    std::uint8_t blocking_byte_4ac = 0;
    std::uint8_t secondary_track_578 = 0;
    std::uint32_t timing_arg_3ec = 0;
    ChangeNotification notify{};
};

inline constexpr std::uint32_t kDefaultUntrackedSelection = 0x126u;
inline constexpr std::uint32_t kTrackedSelectorDataVA = 0x2247AAF0u;
inline constexpr std::uint32_t kDispatchStateVirtualOffset = 0x3C4u;
inline constexpr std::uint32_t kDispatchSelectionVirtualOffset = 0x314u;

enum class DeferredOutcome : std::uint8_t {
    NoPending,
    ScheduledForLater,
    NonpositivePrimaryClip,
    Blocked,
    MissingOwner,
    Dispatched,
};

// Original 0x2231C2F0 (240 bytes). The host provides:
// read_game_time() -> float, find_owner() -> OwnerState* or nullptr,
// dispatch_state_event(13,0), tracked_selection(0x2247AAF0) -> uint32,
// dispatch_selection(id), and timing_interval(field_3ec) -> float.
//
// The gate's x87 FCOMP / TEST AH,0x41 / JE suppresses dispatch iff
// gate_timer_49c > clock. Unordered comparison (NaN) is *not* suppressed.
// Native CMP [clip_4ec],0 / JLE is signed int32, so a clip with
// bit31 set is treated as negative; do not use an unsigned >0 check.
// The original clears pending at +0x589 *before* the first virtual call,
// then reads tracking byte and timestamp later, after external callbacks.
template<class Host>
DeferredOutcome original_scattergun_process_deferred(
    ScatterGunState& state, Host& host) {
    if (state.pending_byte_589 == 0) return DeferredOutcome::NoPending;
    const float first_clock = host.read_game_time();
    if (state.gate_timer_49c > first_clock)
        return DeferredOutcome::ScheduledForLater;
    if (state.primary_clip_4ec == 0 ||
        (state.primary_clip_4ec & 0x80000000u) != 0u)
        return DeferredOutcome::NonpositivePrimaryClip;
    if (state.blocking_byte_4ac != 0)
        return DeferredOutcome::Blocked;
    OwnerState* owner = host.find_owner();
    if (owner == nullptr) return DeferredOutcome::MissingOwner;

    state.pending_byte_589 = 0;
    host.dispatch_state_event(13u, 0u); // original vtable +0x3C4
    const std::uint32_t selection =
        state.secondary_track_578 != 0
        ? host.tracked_selection(kTrackedSelectorDataVA)
        : kDefaultUntrackedSelection;
    host.dispatch_selection(selection); // original vtable +0x314
    // The argument and second clock read happen AFTER those callbacks.
    const float interval = host.timing_interval(state.timing_arg_3ec);
    const float deadline = interval + host.read_game_time();
    original_tracked_float_assign(owner->float_timer_60c, deadline,
                                  owner->notify); // 0x2203D190
    // The native code loads the potentially unchanged owner timer back,
    // then copies its resulting value to both weapon timers. Important
    // when values compare equal, including -0 vs +0.
    const float stored_owner_time = owner->float_timer_60c;
    original_tracked_float_assign(state.gate_timer_49c, stored_owner_time,
                                  state.notify); // 0x22173260
    original_tracked_float_assign(state.mirrored_timer_4a0, stored_owner_time,
                                  state.notify); // 0x221732A0
    return DeferredOutcome::Dispatched;
}
} // namespace sin_compat::original_server_scattergun_deferred
