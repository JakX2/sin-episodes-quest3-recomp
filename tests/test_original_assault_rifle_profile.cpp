#include "sin_compat/original_assault_rifle_profile.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace {
struct MockHost final : sin_compat::IWeaponHost {
    std::string model;
    std::vector<int> fov_calls;
    std::vector<int> played;
    int resolve_sequence(int activity) override { return activity + 1000; }
    void bind_viewmodel(const std::string& m, const std::array<float, 3>& offset) override {
        model = m;
        assert((offset == std::array<float, 3>{{0.0f, 0.0f, 0.0f}}));
    }
    void play_sequence(int id) override { played.push_back(id); }
    void set_flatscreen_fov(int fov, float duration) override {
        assert(duration > 0.0f);
        fov_calls.push_back(fov);
    }
};
} // namespace

int main() {
    using namespace sin_compat;
    using P = OriginalAssaultRifle;
    const WeaponConfig data = P::make_runtime_config();

    assert(std::string(P::classname) == "weapon_assault_rifle");
    assert(std::string(P::npc_classname) == "npc_druglab_grunt_assault_rifle");
    assert(std::string(P::npc_model) == "models/characters/MobGrunt/mobgrunt.mdl");
    assert(std::string(P::worldmodel) == "models/weapons/w_assault_rifle/w_assault_rifle.mdl");
    assert(std::string(P::primary_ammo) == "AR");
    assert(std::string(P::secondary_ammo) == "ARNADE");
    assert(std::string(P::secondary_projectile_classname) == "grenade_ar");
    assert(std::string(P::secondary_projectile_model) ==
           "models/weapons/grenades/ar_grenade.mdl");
    assert(std::string(P::npc_shot_sound) == "Weapon_Assault.NPC_Single");
    assert(data.viewmodel_name == "models/weapons/v_assault_rifle/v_assault_rifle.mdl");
    assert(data.zoom_fov == 40);
    assert(data.ammo.clip_size == 45 && data.ammo.default_clip == 45);
    assert(data.ammo.clip2_size == -1 && data.ammo.default_clip2 == 0);
    assert(!data.ammo.melee_weapon);
    assert(P::kick_min[0] == 0.175f && P::kick_ramp_max[2] == 0.275f);

    // The actual engine has not been connected: verify that existing
    // engine-neutral runtime CONSUMES the original configuration.
    MockHost flatscreen;
    WeaponRuntime rifle(WeaponKind::AssaultRifle, data, flatscreen, false);
    assert(rifle.primary_attack());
    assert(flatscreen.model == data.viewmodel_name);
    const auto zoom = rifle.frame(0x00080000u, true, true, false);
    assert(zoom.entered_zoom);
    assert((flatscreen.fov_calls == std::vector<int>{40}));

    // Reload is modeled only at an externally validated animation event;
    // primary ammo AR transfers to the 45-round magazine, not ARNADE.
    rifle.ammo().clip = 20;
    rifle.ammo().reserve = 30;
    assert(rifle.begin_reload());
    assert(rifle.ammo().clip == 20);
    const auto transfer = rifle.transfer_bulk_at_validated_event(true);
    assert(transfer.primary == 25 && transfer.secondary == 0);
    assert(rifle.ammo().clip == 45 && rifle.ammo().reserve == 5);
    rifle.finish_reload();

    MockHost hmd;
    WeaponRuntime vr(WeaponKind::AssaultRifle, data, hmd, true);
    assert(vr.frame(0x00080000u, true, true, false).entered_zoom);
    assert(hmd.fov_calls.empty()); // Never alter physical headset projection.
    assert(hmd.model == data.viewmodel_name);
    return 0;
}
