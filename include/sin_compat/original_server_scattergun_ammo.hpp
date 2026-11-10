#pragma once
// Phase165: 2006 original SiN Episodes server.dll CWeaponScatterGun
// independently reconstructed ammunition-state control flow.
// Host C++17 reference, NOT the original MSVC32 object layout or ABI.
#include <cstdint>

namespace sin_compat::original_server_scattergun_ammo {

// Original scattergun vtable slot216 (offset +0x360) -> 0x2231C3E0.
// Numeric labels refer to real instance offsets in server.dll.
struct ObservedAmmoState {
    std::uint32_t clip_4ec=0;
    std::uint8_t secondary_tracking_578=0;
    std::uint32_t prior_clip_57c=0xffffffffu;
    std::uint32_t prior_secondary_580=0xffffffffu;
    std::uint32_t secondary_584=0;
    std::uint8_t change_pending_589=0;
};

// Original first calls base 0x222CD050, compares AFTER that callback,
// updates both snapshots (secondary only if tracking enabled), and
// latches pending on changes with an old value other than -1.
// Pending is NOT cleared here. When nonzero, it tail-jumps to the
// separate routine 0x2231C2F0. Its effects are not reconstructed.
template<class BaseUpdate,class DeferredAction>
void original_scattergun_update_ammo(
    ObservedAmmoState& state,BaseUpdate&& base_update,
    DeferredAction&& deferred_action) {
    base_update();
    if(state.prior_clip_57c!=state.clip_4ec) {
        if(state.prior_clip_57c!=0xffffffffu)
            state.change_pending_589=1;
        state.prior_clip_57c=state.clip_4ec;
    }
    if(state.prior_secondary_580!=state.secondary_584 &&
       state.secondary_tracking_578!=0) {
        if(state.prior_secondary_580!=0xffffffffu)
            state.change_pending_589=1;
        state.prior_secondary_580=state.secondary_584;
    }
    if(state.change_pending_589!=0)deferred_action();
}

// The original 0x2231C810 scattergun virtual calls helper 0x2231C740
// at 0x2231C964, passing ECX=original this+0x4F0. The small helper
// decrements its input DWORD unconditionally, without clamping.
// If this+0x1F0 is nonzero it signals byte+0x200; otherwise it ORs bit0
// into the DWORD pointed to by original this+0x16C, if that pointer exists.
// The helper returns RET 4; its original stack argument is not read.
struct SecondaryAmmoConsumption {
    std::uint32_t secondary_clip_4f0=0;
    std::uint8_t mode_byte_1f0=0;
    std::uint8_t signal_byte_200=0;
    std::uint32_t* event_flags_at_pointer_16c=nullptr;
};
inline void original_scattergun_consume_secondary_ammo(
    SecondaryAmmoConsumption& state) noexcept {
    if(state.mode_byte_1f0!=0)state.signal_byte_200=1;
    else if(state.event_flags_at_pointer_16c!=nullptr)
        *state.event_flags_at_pointer_16c |= 1u;
    state.secondary_clip_4f0-=1u; // native DWORD wrap, including zero -> -1
}
} // namespace sin_compat::original_server_scattergun_ammo
