// Must compile as part of an actual Source SDK 2013 SERVER game-code target.
// The independent CI fixture is a structural regression test, not a build of
// Valve's engine or verification against a Quest SourceVR runtime.
#include "cbase.h"
#include "player.h"
#include "basecombatweapon.h"
#include "baseviewmodel.h"
#include "source2013_weapon_host.hpp"

#include <cmath>
#include <utility>

namespace sin_compat {
Source2013WeaponHost::Source2013WeaponHost(
    CBasePlayer* player,CBaseCombatWeapon* weapon,bool vr,int viewmodel_index) noexcept
    : player_(player),weapon_(weapon),vr_mode_(vr),viewmodel_index_(viewmodel_index) {}

bool Source2013WeaponHost::selected() const noexcept {
    return player_ && weapon_ && viewmodel_index_>=0 &&
           player_->GetActiveWeapon()==weapon_ &&
           weapon_->GetOwner()==player_;
}
CBaseViewModel* Source2013WeaponHost::viewmodel() const noexcept {
    return selected() ? player_->GetViewModel(viewmodel_index_, false) : nullptr;
}
int Source2013WeaponHost::resolve_sequence(int activity_id) {
    if (activity_id<0) return -1;
    CBaseViewModel* vm=viewmodel();
    if (!vm) return -1;
    // Verified public SDK signatures: CBaseAnimating::SelectWeightedSequence
    // takes Activity, and CBasePlayer::GetViewModel returns CBaseViewModel*.
    return vm->SelectWeightedSequence(static_cast<Activity>(activity_id));
}
void Source2013WeaponHost::bind_viewmodel(
    const std::string& model,const std::array<float,3>& origin_offset) {
    CBaseViewModel* vm=viewmodel();
    if (!vm || model.empty()) return;
    // Source's SetWeaponModel updates viewmodel model AND weapon association.
    // Rebinding every animation would reset animation unnecessarily.
    if (vm->GetWeapon()!=weapon_ || last_bound_model_!=model) {
        vm->SetWeaponModel(model.c_str(),weapon_);
        last_bound_model_=model;
    }
    // Store but do NOT apply an unverified SiN sight pose to the Source viewmodel.
    last_offset_=origin_offset;
}
void Source2013WeaponHost::play_sequence(int sequence_id) {
    CBaseViewModel* vm=viewmodel();
    if (!vm || sequence_id<0 || vm->GetWeapon()!=weapon_) return;
    vm->SendViewModelMatchingSequence(sequence_id);
}
void Source2013WeaponHost::set_flatscreen_fov(int fov,float duration_seconds) {
    // OpenXR/HMD projection is never modified in VR mode.
    if (vr_mode_ || !selected() || fov<0 || fov>=180 ||
        !std::isfinite(duration_seconds) || duration_seconds<0) return;
    (void)player_->SetFOV(player_,fov,duration_seconds);
}
Source2013WeaponBridge::Source2013WeaponBridge(
    CBasePlayer* player,CBaseCombatWeapon* weapon,WeaponKind kind,
    WeaponConfig configuration,bool vr,int vm_index)
    : host_(player,weapon,vr,vm_index),
      runtime_(kind,std::move(configuration),host_,vr) {}
FrameResult Source2013WeaponBridge::on_usercmd(
    std::uint32_t buttons,bool blocked) {
    const bool ok=host_.selected();
    return runtime_.frame(buttons,ok,ok,blocked);
}
bool Source2013WeaponBridge::on_primary_attack(int recoil) {
    return host_.selected() && runtime_.primary_attack(recoil);
}
bool Source2013WeaponBridge::on_secondary_attack() {
    return host_.selected() && runtime_.secondary_attack();
}
bool Source2013WeaponBridge::on_reload_begin(int start,int end) {
    return host_.selected() && runtime_.begin_reload(start,end);
}
sin_phase55::Transfer Source2013WeaponBridge::on_validated_reload_event() {
    return host_.selected() ? runtime_.transfer_bulk_at_validated_event(true)
                            : sin_phase55::Transfer{};
}
void Source2013WeaponBridge::on_reload_end() {runtime_.finish_reload();}
bool Source2013WeaponBridge::on_holster() {
    return host_.selected() && runtime_.holster();
}
void Source2013WeaponBridge::on_detach() { (void)runtime_.cancel_on_detach(); }
} // namespace sin_compat
