#include "sin_compat/weapon_session.hpp"

#include <utility>

namespace sin_compat {

WeaponSession::WeaponSession(IWeaponSessionHost& host, EntityRef owner,
                             EntityRef weapon, WeaponKind kind,
                             WeaponConfig config, bool vr_mode)
    : host_(host), owner_(owner), weapon_(weapon),
      runtime_(kind, std::move(config), host, vr_mode) {}

bool WeaponSession::selected() const {
    if (!host_.is_live(owner_) || !host_.is_live(weapon_)) return false;
    if (host_.owner_of(weapon_) != owner_) return false;
    const EntityRef selected_weapon = host_.selected_weapon_of(owner_);
    return host_.is_live(selected_weapon) && selected_weapon == weapon_;
}

FrameResult WeaponSession::on_usercmd(std::uint32_t bits, bool block) {
    const bool active = selected();
    if (!active) {
        // Still record usercmd edges, but reset detached gameplay state.
        // The original state machine early-outs on a missing player, so
        // leaving local zoom active here would persist ADS across switches.
        (void)runtime_.frame(bits, false, false, true);
        const bool was_zoomed = runtime_.cancel_on_detach();
        reload_transfer_consumed_ = false;
        return {false, was_zoomed, false, was_zoomed};
    }
    return runtime_.frame(bits, true, true, block);
}

EnsureViewModelResult WeaponSession::ensure_viewmodel() {
    if (!selected() || !host_.is_player(owner_) ||
        !host_.is_player_owner(owner_)) {
        return EnsureViewModelResult::InvalidPlayer;
    }
    const int slot = host_.viewmodel_index_of(weapon_);
    if (!host_.is_valid_viewmodel_index(owner_, slot)) {
        return EnsureViewModelResult::InvalidSlot;
    }
    return ensure_viewmodel_slot(host_, owner_, slot);
}

HideWeaponInputResult WeaponSession::on_hide_weapon_input() {
    // The original input validates owner & selected weapon before dispatch.
    // Phase 63/64/65: a live associated viewmodel is hidden too.
    if (!selected()) return host_.is_live(weapon_)
        ? (host_.is_live(owner_) ? HideWeaponInputResult::IgnoredNotSelected
                                 : HideWeaponInputResult::IgnoredInvalidOwner)
        : HideWeaponInputResult::IgnoredInvalidWeapon;
    return process_hide_weapon_input_with_viewmodel(host_, weapon_);
}

bool WeaponSession::on_primary_attack(int counter) {
    return selected() && runtime_.primary_attack(counter);
}

bool WeaponSession::on_secondary_attack() {
    return selected() && runtime_.secondary_attack();
}

bool WeaponSession::on_reload_begin(int start, int end) {
    if (!selected()) return false;
    if (!runtime_.begin_reload(start, end)) return false;
    reload_transfer_consumed_ = false;
    return true;
}

sin_phase55::Transfer WeaponSession::on_reload_transfer_event() {
    if (!selected() || !runtime_.reload_active() ||
        reload_transfer_consumed_) return {};
    // Consumed even if reserve/clip cannot transfer, to make the host event
    // exactly-once. This guards against repeated animation-event delivery.
    reload_transfer_consumed_ = true;
    return runtime_.transfer_bulk_at_validated_event(true);
}

void WeaponSession::on_reload_end() {
    runtime_.finish_reload();
    reload_transfer_consumed_ = false;
}

bool WeaponSession::on_holster() {
    if (!selected()) return false;
    const bool succeeded = runtime_.holster();
    if (succeeded) reload_transfer_consumed_ = false;
    return succeeded;
}

void WeaponSession::on_player_cleanup() {
    // Cleanup is player-level and should not be triggered simply because a
    // weapon is switched or removed. The caller owns that lifecycle decision.
    if (host_.is_live(owner_) && host_.is_player(owner_)) {
        clear_viewmodel_slots(host_, owner_);
    }
}

} // namespace sin_compat
