#pragma once

// Source-derived data profile for the original SiN Episodes assault rifle.
// Configuration facts came from the owner's original 2006 SDK and shipped
// scripts/weapon_assault_rifle.txt, checked against hash-pinned client/server
// DLL identifiers. This does not register a Source/SourceVR weapon entity,
// load assets, reproduce firing, or implement original SinTEK NPC behavior.

#include "sin_compat/weapon_runtime.hpp"

#include <array>

namespace sin_compat {

struct OriginalAssaultRifle {
    static constexpr const char* classname = "weapon_assault_rifle";
    static constexpr const char* npc_classname = "npc_druglab_grunt_assault_rifle";
    static constexpr const char* npc_model = "models/characters/MobGrunt/mobgrunt.mdl";
    static constexpr const char* viewmodel = "models/weapons/v_assault_rifle/v_assault_rifle.mdl";
    static constexpr const char* worldmodel = "models/weapons/w_assault_rifle/w_assault_rifle.mdl";
    static constexpr const char* anim_prefix = "smg2";
    static constexpr const char* primary_ammo = "AR";
    // Separate secondary ammo type; clip2_size=-1, do not treat this as a magazine.
    static constexpr const char* secondary_ammo = "ARNADE";
    // 2006 server.dll: x86 weapon routine selects the grenade_ar classname;
    // this is evidence metadata, not a recreated projectile implementation.
    static constexpr const char* secondary_projectile_classname = "grenade_ar";
    static constexpr const char* secondary_projectile_model =
        "models/weapons/grenades/ar_grenade.mdl";

    static constexpr int clip_size = 45;
    static constexpr int default_clip = 45;
    static constexpr int clip2_size = -1;
    static constexpr int default_clip2 = 0;
    static constexpr int zoom_fov = 40;
    static constexpr int weight = 7;

    // Original weapon script recoil keys. Values are not yet consumed by the
    // prototype's weapon firing code: no claim of authentic recoil physics.
    static constexpr std::array<float, 3> kick_min{{0.175f, 0.0f, 0.0f}};
    static constexpr std::array<float, 3> kick_ramp_min{{0.0f, -1.0f, -0.275f}};
    static constexpr std::array<float, 3> kick_ramp_max{{1.5f, 1.0f, 0.275f}};
    static constexpr float kick_ramp_seconds = 1.5f;
    static constexpr float kick_ramp_start_scale = 0.2f;
    static constexpr float kick_ramp_end_scale = 0.7f;

    // Script references. Presence of each identifier was cross-checked against
    // shipped scripts/game_sounds_weapons.txt, not exercised in an engine.
    static constexpr const char* shot_sound = "Weapon_Assault.Single";
    static constexpr const char* npc_shot_sound = "Weapon_Assault.NPC_Single";
    static constexpr const char* secondary_sound = "Weapon_Assault.Double";
    static constexpr const char* reload_sound = "Weapon_Assault.ReloadOne";
    static constexpr const char* npc_reload_sound = "Weapon_Assault.NPC_Reload";

    static WeaponConfig make_runtime_config() {
        WeaponConfig config;
        config.viewmodel_name = viewmodel;
        config.zoom_fov = zoom_fov; // flatscreen only; WeaponRuntime leaves VR HMD FOV alone.
        config.ammo.clip_size = clip_size;
        config.ammo.default_clip = default_clip;
        config.ammo.clip2_size = clip2_size;
        config.ammo.default_clip2 = default_clip2;
        config.ammo.melee_weapon = false;
        // No fabricated viewmodel offset: needs a mounted model/engine measurement.
        return config;
    }
};

} // namespace sin_compat
