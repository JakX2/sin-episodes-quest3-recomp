#include "sin_compat/original_sdk_weapon_config.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace sin_compat;

struct ZoomHost : IWeaponHost {
    std::vector<int> seen_fov;
    int resolve_sequence(int activity) override { return activity; }
    void bind_viewmodel(const std::string&, const std::array<float, 3>&) override {}
    void play_sequence(int) override {}
    void set_flatscreen_fov(int fov, float seconds) override {
        assert(seconds == 0.25f || seconds == 0.15f);
        seen_fov.push_back(fov);
    }
};

void check(WeaponKind kind, int fov, int clip, const char* model) {
    const auto cfg = original_sdk_weapon_config(kind);
    assert(cfg.zoom_fov == fov);
    assert(cfg.ammo.clip_size == clip);
    assert(cfg.ammo.default_clip == clip);
    assert(cfg.ammo.clip2_size == -1);
    assert(cfg.ammo.default_clip2 == 0);
    assert(cfg.viewmodel_name == model);
    assert(cfg.origin_offset == (std::array<float, 3>{0, 0, 0}));
    assert(!cfg.ammo.melee_weapon);
    ZoomHost screen;
    WeaponRuntime desktop(kind, cfg, screen, false);
    auto step = desktop.frame(0x00080000u, true, true, false);
    assert(step.entered_zoom);
    assert(screen.seen_fov == std::vector<int>{fov});
    desktop.frame(0u, true, true, false);
    step = desktop.frame(0x00080000u, true, true, false);
    assert(step.exited_zoom);
    assert((screen.seen_fov == std::vector<int>{fov, 0}));
    ZoomHost vr;
    WeaponRuntime headset(kind, cfg, vr, true);
    assert(headset.frame(0x00080000u, true, true, false).entered_zoom);
    assert(vr.seen_fov.empty()); // Must never change VR projection FOV.
}

int main() {
    check(WeaponKind::Magnum, 50, 20,
          "models/weapons/v_magnum/v_magnum.mdl");
    check(WeaponKind::AssaultRifle, 40, 45,
          "models/weapons/v_assault_rifle/v_assault_rifle.mdl");
    check(WeaponKind::Scattergun, 60, 6,
          "models/weapons/v_scattergun/v_scattergun.mdl");
    bool rejected = false;
    try { (void)original_sdk_weapon_config(static_cast<WeaponKind>(99)); }
    catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    std::cout << "original SDK script settings and VR projection guard passed\n";
}
