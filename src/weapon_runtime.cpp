#include "sin_compat/weapon_runtime.hpp"
#include "phase53_reload.hpp"
#include "phase52_weapon_deploy.hpp"

#include <utility>

namespace sin_compat {
namespace {
sin_re_phase47::WeaponKind as_phase47(WeaponKind kind) {
    switch (kind) {
        case WeaponKind::AssaultRifle: return sin_re_phase47::WeaponKind::AssaultRifle;
        case WeaponKind::Scattergun: return sin_re_phase47::WeaponKind::Scattergun;
        default: return sin_re_phase47::WeaponKind::Magnum;
    }
}
}

WeaponRuntime::WeaponRuntime(WeaponKind kind, WeaponConfig config,
                             IWeaponHost& host, bool vr_mode)
    : kind_(kind), config_(std::move(config)), host_(host), vr_mode_(vr_mode) {}

bool WeaponRuntime::request_activity(int activity_id) {
    // Phase 51: failure to find a sequence bypasses viewmodel setup.
    const int sequence = host_.resolve_sequence(activity_id);
    if (sequence < 0) return false;
    host_.bind_viewmodel(config_.viewmodel_name, config_.origin_offset);
    host_.play_sequence(sequence);
    active_activity_ = activity_id;
    return true;
}

void WeaponRuntime::enter_zoom() {
    zoom_.zoom_active_0x578 = true;
    (void)request_activity(sin_phase45::kZoomInActivityArgument);
    // VR: do not artificially narrow headset field of view.
    if (!vr_mode_) host_.set_flatscreen_fov(config_.zoom_fov, sin_phase45::kZoomInFovSeconds);
}

void WeaponRuntime::exit_zoom() {
    zoom_.zoom_active_0x578 = false;
    const auto e = sin_phase45::exit_effects(
        active_activity_ == sin_phase45::kZoomInActivityArgument);
    if (e.activity_argument >= 0) (void)request_activity(e.activity_argument);
    if (!vr_mode_) host_.set_flatscreen_fov(0, e.fov_seconds);
}

FrameResult WeaponRuntime::frame(std::uint32_t button_bits, bool player_exists,
                                  bool owner_ready, bool external_zoom_block) {
    buttons_.update(button_bits);
    sin_phase45::Inputs inputs{};
    inputs.has_player = player_exists && !holstered_;
    inputs.player_vfunc_0x120 = owner_ready;
    inputs.weapon_flag_0x4AC = ammo_.reload_active;
    inputs.player_vfunc_0x3C8 = external_zoom_block;
    inputs.player_bits_at_0x93C = buttons_.pressed;
    const auto r = sin_phase45::step(zoom_, inputs);
    if (r.transition == sin_phase45::Transition::Enter) enter_zoom();
    if (r.transition == sin_phase45::Transition::Exit) exit_zoom();
    return {r.transition == sin_phase45::Transition::Enter,
            r.transition == sin_phase45::Transition::Exit,
            r.automatic_restore, r.interrupted};
}

bool WeaponRuntime::begin_reload(int start_activity_id, int end_activity_id) {
    if (holstered_ || ammo_.reload_active) return false;
    // Guarded integration policy: do not begin reload if neither ammo channel can load.
    const auto& cfg = config_.ammo;
    if (sin_phase55::reload_quantity(cfg.clip_size, ammo_.clip, ammo_.reserve) == 0 &&
        sin_phase55::reload_quantity(cfg.clip2_size, ammo_.clip2, ammo_.reserve2) == 0)
        return false;
    sin53::ReloadState stage{};
    stage.stage = ammo_.stage;
    const auto selected = sin53::select_reload_activity(stage, start_activity_id,
                                                         end_activity_id);
    if (!request_activity(selected.requestedId)) return false;
    ammo_.stage = stage.stage;
    ammo_.reload_active = true;
    return true;
}

sin_phase55::Transfer WeaponRuntime::transfer_bulk_at_validated_event(bool owner_valid) {
    // The actual engine must call this at the correct animation/timing event.
    if (!ammo_.reload_active || holstered_) return {};
    return sin_phase55::transfer_bulk(ammo_, config_.ammo, owner_valid);
}

sin_phase55::ShellEvent WeaponRuntime::process_one_round_at_validated_event(
    bool owner_valid, bool deadline_reached, std::uint32_t held_buttons) {
    if (holstered_) return sin_phase55::ShellEvent::NotReady;
    return sin_phase55::process_one_round(ammo_, config_.ammo, owner_valid,
                                         deadline_reached, held_buttons);
}

void WeaponRuntime::finish_reload() {
    ammo_.reload_active = false;
    ammo_.stage = 0;
}

bool WeaponRuntime::cancel_on_detach() {
    const bool was_zoomed = zoom_.zoom_active_0x578;
    zoom_.zoom_active_0x578 = false;
    zoom_.restore_pending_0x579 = false;
    finish_reload();
    active_activity_ = -1;
    if (was_zoomed && !vr_mode_) {
        // Do not touch headset projection. The engine adapter must still
        // handle camera lifetime, death and player transition on its side.
        host_.set_flatscreen_fov(0, sin_phase45::kZoomOutFovSeconds);
    }
    return was_zoomed;
}

bool WeaponRuntime::holster() {
    // Phase 52/54 ordering: zoom-out wrapper precedes reload cleanup and holster.
    exit_zoom();
    zoom_.restore_pending_0x579 = false;
    finish_reload();
    const bool success = request_activity(sin_phase52::ACT_VM_HOLSTER);
    if (success) holstered_ = true; // do not mark successful holster on rejected sequence
    return success;
}

bool WeaponRuntime::primary_attack(int rifle_recoil_counter) {
    if (holstered_ || ammo_.reload_active) return false;
    const auto act = sin_re_phase47::selectPrimaryActivity(
        as_phase47(kind_), zoom_.zoom_active_0x578, rifle_recoil_counter);
    return request_activity(static_cast<int>(act));
}

bool WeaponRuntime::secondary_attack() {
    if (holstered_ || ammo_.reload_active) return false;
    const auto act = sin_re_phase47::selectSecondaryActivity(zoom_.zoom_active_0x578);
    return request_activity(static_cast<int>(act));
}
} // namespace sin_compat
