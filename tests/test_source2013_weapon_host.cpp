// This test compiles the SAME adapter source against wholly independent,
// SDK-shaped minimal stubs. It is not a real Valve/SourceVR engine test.
#include "source2013_weapon_host.hpp"
#include "player.h"
#include "basecombatweapon.h"
#include "baseviewmodel.h"
#include "sin_compat/original_sdk_weapon_config.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
using namespace sin_compat;

int main() {
    CBasePlayer player;
    CBaseCombatWeapon weapon;
    CBaseViewModel vm;
    weapon.owner=&player;
    player.active_weapon=&weapon;
    player.viewmodel=&vm;

    // Real original Script metadata is referenced only through our independent
    // SDK-derived helper; proprietary model files are absent.
    WeaponConfig config=original_sdk_weapon_config(WeaponKind::Magnum);
    assert(config.zoom_fov==50 && config.ammo.clip_size==20);
    Source2013WeaponBridge flat(&player,&weapon,WeaponKind::Magnum,config,false);
    assert(flat.host().selected());

    // Source SDK VM sequence lookup -> model+weapon bind -> sequence playback.
    assert(flat.on_primary_attack());
    assert(vm.select_calls==1);
    assert(vm.model_calls==1);
    assert(vm.play_calls==1);
    assert(vm.last_sequence>=0);
    assert(vm.bound_model=="models/weapons/v_magnum/v_magnum.mdl");
    assert(vm.GetWeapon()==&weapon);
    assert(player.requested_vm_index==0 && !player.last_observer_flag);
    assert(flat.host().last_config_offset()[0]==0);

    // Stable model should not rebind when successive animation activities run.
    const int prior_binds=vm.model_calls;
    assert(flat.on_secondary_attack());
    assert(vm.model_calls==prior_binds);
    assert(vm.play_calls==2);

    // Unknown sequence prevents both model bind and play.
    vm.sequence_available=false;
    const int played=vm.play_calls;
    assert(!flat.on_primary_attack());
    assert(vm.play_calls==played);
    vm.sequence_available=true;

    // Test the actual host's Source2013->SetFOV callback; VR never calls it.
    flat.host().set_flatscreen_fov(50,0.2f);
    assert(player.set_fov_calls==1 && player.last_fov==50);
    assert(player.last_fov_requester==&player);
    flat.host().set_flatscreen_fov(200,0.2f); // invalid
    flat.host().set_flatscreen_fov(50,-1.0f);
    assert(player.set_fov_calls==1);

    // The standalone WeaponRuntime calls this on zoom button rising edge.
    auto r=flat.on_usercmd(0x00080000u,false);
    assert(r.entered_zoom && flat.runtime().is_zoomed());
    assert(player.set_fov_calls==2 && player.last_fov==50);
    flat.on_usercmd(0,false);
    r=flat.on_usercmd(0x00080000u,false);
    assert(r.exited_zoom && !flat.runtime().is_zoomed());
    assert(player.set_fov_calls==3 && player.last_fov==0);

    // Reload transfer happens ONLY at explicit engine-authorized event, not
    // on beginning an animation; reserve and clip are host supplied here.
    auto& ammo=flat.runtime().ammo();
    ammo.clip=3;
    ammo.reserve=12;
    const int original_clip=ammo.clip, original_reserve=ammo.reserve;
    assert(flat.on_reload_begin(5,6));
    assert(ammo.clip==original_clip && ammo.reserve==original_reserve);
    (void)flat.on_validated_reload_event();
    assert(ammo.clip>=original_clip && ammo.reserve<=original_reserve);
    flat.on_reload_end();

    // Stale owner/selection fails without mutating the viewmodel.
    CBaseCombatWeapon other;
    player.active_weapon=&other;
    const int before=vm.play_calls;
    assert(!flat.host().selected());
    assert(!flat.on_primary_attack());
    assert(!flat.on_secondary_attack());
    assert(!flat.on_reload_begin(5,6));
    assert(vm.play_calls==before);
    flat.on_detach();

    player.active_weapon=&weapon;
    // Physical HMD mode: no artificial change to HMD camera FOV.
    Source2013WeaponBridge vr(&player,&weapon,WeaponKind::Magnum,config,true);
    const int before_fov=player.set_fov_calls;
    vr.host().set_flatscreen_fov(50,0.2f);
    assert(player.set_fov_calls==before_fov);
    r=vr.on_usercmd(0x00080000u,false);
    assert(r.entered_zoom && vr.runtime().is_zoomed());
    assert(player.set_fov_calls==before_fov);
    vr.on_detach();
    assert(player.set_fov_calls==before_fov);

    // Missing player/viewmodel causes safely rejected output.
    Source2013WeaponHost null_host(nullptr,&weapon,false);
    assert(!null_host.selected());
    assert(null_host.resolve_sequence(4)==-1);
    null_host.bind_viewmodel(config.viewmodel_name,{0,0,0});
    null_host.play_sequence(1);
    null_host.set_flatscreen_fov(60,0);
    player.viewmodel=nullptr;
    Source2013WeaponHost missing_vm(&player,&weapon,false);
    assert(missing_vm.resolve_sequence(4)==-1);
    return 0;
}
