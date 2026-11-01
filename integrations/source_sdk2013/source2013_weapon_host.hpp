#pragma once
// Independent adapter compiled *within* a Source SDK 2013 server game target.
// No Valve headers are redistributed. The integrator supplies the real SDK.
// This is a first engine-facing weapon seam, NOT an Android SourceVR game DLL.
#include "sin_compat/weapon_runtime.hpp"
#include <array>
#include <cstdint>
#include <string>

class CBasePlayer;
class CBaseCombatWeapon;
class CBaseViewModel;

namespace sin_compat {
class Source2013WeaponHost final : public IWeaponHost {
public:
    Source2013WeaponHost(CBasePlayer* player, CBaseCombatWeapon* weapon,
                         bool vr_mode, int viewmodel_index=0,
                         std::string initial_model = {}) noexcept;
    bool selected() const noexcept;
    int resolve_sequence(int activity_id) override;
    void bind_viewmodel(const std::string& model,
                        const std::array<float,3>& origin_offset) override;
    void play_sequence(int sequence_id) override;
    void set_flatscreen_fov(int fov, float duration_seconds) override;
    // Origin/aim pose is not recovered from original game. The adapter does
    // not reorient a headset or assume a fabricated weapon pose.
    const std::array<float,3>& last_config_offset() const noexcept {
        return last_offset_;
    }
private:
    CBaseViewModel* viewmodel() const noexcept;
    CBaseViewModel* ensure_model_bound();
    CBasePlayer* player_;
    CBaseCombatWeapon* weapon_;
    bool vr_mode_;
    int viewmodel_index_;
    std::string expected_model_;
    std::string last_bound_model_;
    std::array<float,3> last_offset_{{0,0,0}};
};

// Source SDK game-module code must create this during its own lifetime and
// feed UserCmd buttons and *validated* animation reload events. Nothing in
// this wrapper installs a Source entity, vtable or game-loop hook.
class Source2013WeaponBridge final {
public:
    Source2013WeaponBridge(CBasePlayer* player, CBaseCombatWeapon* weapon,
                           WeaponKind kind, WeaponConfig configuration,
                           bool vr_mode, int viewmodel_index=0);
    FrameResult on_usercmd(std::uint32_t buttons, bool external_zoom_block);
    bool on_primary_attack(int rifle_recoil_counter=0);
    bool on_secondary_attack();
    bool on_reload_begin(int start_activity=-1, int end_activity=-1);
    sin_phase55::Transfer on_validated_reload_event();
    void on_reload_end();
    bool on_holster();
    void on_detach();
    Source2013WeaponHost& host() noexcept {return host_;}
    WeaponRuntime& runtime() noexcept {return runtime_;}
private:
    Source2013WeaponHost host_;
    WeaponRuntime runtime_;
};
} // namespace sin_compat
