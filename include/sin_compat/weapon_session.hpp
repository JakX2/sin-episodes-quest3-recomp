#pragma once

#include "sin_compat/weapon_runtime.hpp"
#include "sin_compat/weapon_viewmodel_io.hpp"
#include "sin_compat/weapon_viewmodel_lifecycle.hpp"

namespace sin_compat {

// R1 seam: implement this interface WITHIN the chosen game/engine module.
// It intentionally contains no 2006 x86 offsets or Source SDK declarations.
// A genuine SourceVR adapter must bind these methods to actual engine APIs.
class IWeaponSessionHost : public IWeaponHost,
                           public IWeaponViewModelHost,
                           public IWeaponViewModelLifecycleHost {
public:
    ~IWeaponSessionHost() override = default;
    // Coalesce the same pure virtual member declared on both host bases.
    virtual bool is_live(EntityRef entity) const override = 0;
};

// One instance per selected weapon. The host is responsible for mapping live
// handles, dispatching original engine events and preserving state on save/load.
// A synthetic CTest host is NOT evidence of a loaded SourceVR module.
class WeaponSession {
public:
    WeaponSession(IWeaponSessionHost& host, EntityRef owner, EntityRef weapon,
                  WeaponKind kind, WeaponConfig config, bool vr_mode);

    // All external gameplay events must enter through these guarded methods.
    // A frame with lost selection still reaches WeaponRuntime so ADS can reset.
    FrameResult on_usercmd(std::uint32_t button_bits, bool external_zoom_block);
    EnsureViewModelResult ensure_viewmodel();
    HideWeaponInputResult on_hide_weapon_input();
    bool on_primary_attack(int rifle_recoil_counter = 0);
    bool on_secondary_attack();
    bool on_reload_begin(int start_activity_id = -1, int end_activity_id = -1);
    // The real engine must invoke this exactly when it validates its reload
    // animation/timing event. A repeated event cannot refill reserve ammo.
    sin_phase55::Transfer on_reload_transfer_event();
    void on_reload_end();
    bool on_holster();
    void on_player_cleanup();

    WeaponRuntime& runtime() { return runtime_; }
    const WeaponRuntime& runtime() const { return runtime_; }

private:
    bool selected() const;

    IWeaponSessionHost& host_;
    EntityRef owner_;
    EntityRef weapon_;
    WeaponRuntime runtime_;
    bool reload_transfer_consumed_ = false;
};

} // namespace sin_compat
