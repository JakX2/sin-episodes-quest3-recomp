#include "sin_compat/weapon_runtime.hpp"
#include <algorithm>
#ifdef NDEBUG
#undef NDEBUG // Keep assertions active in Release CI builds as well.
#endif
#include <cassert>
#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace sin_compat;

struct MockHost final : IWeaponHost {
    std::set<int> missing{};
    std::vector<std::string> log{};
    std::vector<int> activities{};
    std::vector<int> fovs{};
    int bind_count = 0;
    int play_count = 0;
    std::array<float,3> origin{};
    std::string model{};
    int resolve_sequence(int activity_id) override {
        log.push_back("resolve");
        activities.push_back(activity_id);
        return missing.count(activity_id) ? -1 : activity_id + 1000;
    }
    void bind_viewmodel(const std::string& m,const std::array<float,3>& off) override {
        log.push_back("bind"); ++bind_count; model=m; origin=off;
    }
    void play_sequence(int sequence_id) override {
        assert(sequence_id >= 1000);
        log.push_back("play"); ++play_count;
    }
    void set_flatscreen_fov(int fov, float seconds) override {
        assert(seconds == 0.25f || seconds == 0.15f);
        log.push_back("fov"); fovs.push_back(fov);
    }
};

WeaponConfig config() {
    WeaponConfig c;
    c.viewmodel_name = "models/weapons/v_magnum.mdl"; // synthetic test fixture, not verified actual asset path
    c.origin_offset = {1.0f, -2.0f, 3.0f};
    c.zoom_fov = 55; // synthetic test value, not recovered from SiN
    c.ammo.clip_size = 6;
    c.ammo.clip2_size = -1;
    return c;
}

void zoom_press_edges() {
    MockHost h;
    WeaponRuntime w(WeaponKind::Magnum,config(),h,false);
    auto a = w.frame(0x80000u,true,true,false);
    assert(a.entered_zoom && w.is_zoomed());
    assert(h.fovs == std::vector<int>{55});
    assert(h.activities.back() == 0xc0);
    assert(h.origin == (std::array<float,3>{1,-2,3}));
    assert(h.model == config().viewmodel_name);
    a = w.frame(0x80000u,true,true,false);
    assert(!a.entered_zoom && !a.exited_zoom);
    assert(h.fovs.size()==1);
    w.frame(0u,true,true,false);
    a = w.frame(0x80000u,true,true,false);
    assert(a.exited_zoom && !w.is_zoomed());
    assert(h.fovs == (std::vector<int>{55,0}));
    assert(h.activities.back() == 0x9d);
}

void vr_does_not_narrow_headset_fov() {
    MockHost h;
    WeaponRuntime w(WeaponKind::Magnum,config(),h,true);
    w.frame(0x80000u,true,true,false);
    w.frame(0u,true,true,false);
    w.frame(0x80000u,true,true,false);
    assert(h.fovs.empty());
    assert(h.play_count == 2);
}

void interruption_and_restore() {
    MockHost h;
    WeaponRuntime w(WeaponKind::Scattergun,config(),h,false);
    w.frame(0x80000u,true,true,false);
    auto a = w.frame(0u,true,true,true);
    assert(a.exited_zoom && a.was_interrupted && !w.is_zoomed());
    a = w.frame(0u,true,true,false);
    assert(a.entered_zoom && a.automatically_restored && w.is_zoomed());
    assert(h.fovs == (std::vector<int>{55,0,55}));
}

void reload_delayed_transfer_and_restore() {
    MockHost h;
    WeaponRuntime w(WeaponKind::Magnum,config(),h,false);
    w.ammo().clip = 2; w.ammo().reserve=5;
    w.frame(0x80000u,true,true,false);
    assert(w.begin_reload());
    assert(w.ammo().clip == 2 && w.ammo().reserve == 5);
    assert(w.reload_active());
    auto a = w.frame(0u,true,true,false);
    assert(a.was_interrupted && !w.is_zoomed());
    auto t = w.transfer_bulk_at_validated_event(true);
    assert(t.primary == 4 && t.secondary == 0);
    assert(w.ammo().clip == 6 && w.ammo().reserve == 1);
    assert(w.transfer_bulk_at_validated_event(true).primary == 0);
    w.finish_reload();
    a = w.frame(0u,true,true,false);
    assert(a.automatically_restored && w.is_zoomed());
}

void no_transfer_without_owner_or_activity() {
    MockHost h;
    WeaponRuntime w(WeaponKind::Magnum,config(),h,false);
    w.ammo().clip=1; w.ammo().reserve=5;
    assert(!w.transfer_bulk_at_validated_event(true).primary);
    assert(w.begin_reload());
    auto t = w.transfer_bulk_at_validated_event(false);
    assert(t.primary==0 && w.ammo().clip==1);
    h.missing.insert(0x9c);
    const int binds=h.bind_count;
    assert(!w.holster());
    assert(!w.is_holstered());
    assert(h.bind_count == binds); // missing sequence cannot bind viewmodel
    assert(!w.reload_active()); // cleanup precedes activity request
}

void no_one_round_by_default() {
    MockHost h;
    WeaponRuntime w(WeaponKind::Scattergun,config(),h,false);
    w.ammo().clip=1; w.ammo().reserve=5;
    assert(w.begin_reload());
    assert(w.process_one_round_at_validated_event(true,true,0) == sin_phase55::ShellEvent::NotReady);
    w.ammo().reloads_singly=true; // externally controlled until verified against original game
    assert(w.process_one_round_at_validated_event(true,true,0) == sin_phase55::ShellEvent::Inserted);
    assert(w.ammo().clip==2 && w.ammo().reserve==4);
    assert(w.process_one_round_at_validated_event(true,true,0x1) == sin_phase55::ShellEvent::Interrupted);
    assert(!w.reload_active());
}

void missing_animation_is_safe() {
    MockHost h;
    h.missing.insert(0xc0);
    WeaponRuntime w(WeaponKind::Magnum,config(),h,true);
    w.frame(0x80000u,true,true,false);
    assert(w.is_zoomed()); // zoom gameplay decision is independent of missing sequence
    assert(h.bind_count==0 && h.play_count==0);
}

void rifle_recoil_and_holster() {
    MockHost h;
    WeaponRuntime w(WeaponKind::AssaultRifle,config(),h,true);
    assert(w.primary_attack(1)); assert(h.activities.back() == 0xa4);
    assert(w.primary_attack(2)); assert(h.activities.back() == 0xbb);
    assert(w.primary_attack(3)); assert(h.activities.back() == 0xbc);
    assert(w.primary_attack(4)); assert(h.activities.back() == 0xbd);
    w.frame(0x80000u,true,true,false);
    assert(w.primary_attack(10)); assert(h.activities.back() == 0xc1);
    assert(w.secondary_attack()); assert(h.activities.back() == 0xc2);
    assert(w.holster());
    assert(w.is_holstered());
    assert(!w.is_zoomed());
    assert(h.activities.back() == 0x9c);
    assert(!w.primary_attack());
}

void ammo_conservation_fuzz() {
    MockHost h;
    WeaponRuntime w(WeaponKind::Magnum,config(),h,false);
    for(int clip=0;clip<=6;++clip) for(int reserve=0;reserve<=15;++reserve) {
        w.finish_reload();
        w.ammo().clip=clip;w.ammo().reserve=reserve;
        if (clip == 6 || reserve == 0) {assert(!w.begin_reload());continue;}
        assert(w.begin_reload());
        const auto t=w.transfer_bulk_at_validated_event(true);
        assert(t.primary==std::min(6-clip,reserve)); // fixture weapon capacity is 6
        assert(w.ammo().clip+w.ammo().reserve==clip+reserve);
        assert(w.ammo().clip<=6 && w.ammo().reserve>=0);
      }
}

int main() {
    zoom_press_edges();
    vr_does_not_narrow_headset_fov();
    interruption_and_restore();
    reload_delayed_transfer_and_restore();
    no_transfer_without_owner_or_activity();
    no_one_round_by_default();
    missing_animation_is_safe();
    rifle_recoil_and_holster();
    ammo_conservation_fuzz();
    std::cout << "weapon runtime integration tests passed\n";
}
