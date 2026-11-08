#pragma once

// SourceVR host-agnostic integration prototype.
// Not Ritual source code, not a complete Source engine game DLL.

#include <array>
#include <cstdint>
#include <string>

#include "phase45_zoom_transition.hpp"
#include "phase46_button_edges.hpp"
#include "phase47_ads_activity.hpp"
#include "phase55_reload_clip.hpp"

namespace sin_compat {

enum class WeaponKind { Magnum, AssaultRifle, Scattergun };

struct WeaponConfig {
    std::string viewmodel_name;
    std::array<float, 3> origin_offset{{0.0f, 0.0f, 0.0f}};
    int zoom_fov = 0;  // 0 means default FOV; actual SiN gun values not yet recovered.
    sin_phase55::WeaponInfo ammo{};
};

// Implement with real engine APIs once the SourceVR branch is selected.
class IWeaponHost {
public:
    virtual ~IWeaponHost() = default;
    virtual int resolve_sequence(int activity_id) = 0; // -1 = unavailable
    virtual void bind_viewmodel(const std::string& model,
                                const std::array<float, 3>& origin_offset) = 0;
    virtual void play_sequence(int sequence_id) = 0;
    // For flatscreen compatibility only; physical HMD FOV must not change.
    virtual void set_flatscreen_fov(int fov, float duration_seconds) = 0;
};

struct FrameResult {
    bool entered_zoom = false;
    bool exited_zoom = false;
    bool automatically_restored = false;
    bool was_interrupted = false;
};

class WeaponRuntime {
public:
    WeaponRuntime(WeaponKind kind, WeaponConfig config,
                  IWeaponHost& host, bool vr_mode);
    FrameResult frame(std::uint32_t button_bits, bool player_exists,
                      bool owner_ready, bool external_zoom_block);
    bool begin_reload(int start_activity_id = -1, int end_activity_id = -1);
    sin_phase55::Transfer transfer_bulk_at_validated_event(bool owner_valid);
    sin_phase55::ShellEvent process_one_round_at_validated_event(
        bool owner_valid, bool deadline_reached, std::uint32_t held_buttons);
    void finish_reload();
    // Port safety policy when a weapon is deselected/removed: cancel local
    // ADS/reload without playing an animation on a potentially stale model.
    // Returns whether ADS was active (for session event reporting).
    bool cancel_on_detach();
    bool holster();
    bool primary_attack(int rifle_recoil_counter = 0);
    bool secondary_attack();
    bool is_zoomed() const { return zoom_.zoom_active_0x578; }
    bool is_holstered() const { return holstered_; }
    bool reload_active() const { return ammo_.reload_active; }
    sin_phase55::Weapon& ammo() { return ammo_; }
    const sin_phase55::Weapon& ammo() const { return ammo_; }
    WeaponKind kind() const noexcept { return kind_; }
    const WeaponConfig& config() const { return config_; }

private:
    bool request_activity(int activity_id);
    void enter_zoom();
    void exit_zoom();

    WeaponKind kind_;
    WeaponConfig config_;
    IWeaponHost& host_;
    bool vr_mode_;
    bool holstered_ = false;
    int active_activity_ = -1;
    sin_phase46::ButtonEdges buttons_{};
    sin_phase45::WeaponState zoom_{};
    sin_phase55::Weapon ammo_{};
};
} // namespace sin_compat
