#pragma once

// Values transcribed from the owner-supplied 2006 SiN Episodes SDK WeaponData
// scripts. Not copied Source SDK code and not an implementation of original
// firing, recoil, weapon-script loading, render alignment or reload timing.
// See docs/PHASE76_SIN_SDK_RESEARCH.md for exact script SHA-256 provenance.

#include "sin_compat/weapon_runtime.hpp"
#include <stdexcept>

namespace sin_compat {

inline WeaponConfig original_sdk_weapon_config(WeaponKind kind) {
    WeaponConfig config{};
    config.origin_offset = {0.0f, 0.0f, 0.0f}; // Unknown here, NOT an original pose.
    config.ammo.clip2_size = -1;               // All three original script files.
    config.ammo.default_clip2 = 0;
    config.ammo.melee_weapon = false;
    switch (kind) {
        case WeaponKind::Magnum:
            config.viewmodel_name = "models/weapons/v_magnum/v_magnum.mdl";
            config.zoom_fov = 50;
            config.ammo.clip_size = 20;
            config.ammo.default_clip = 20;
            break;
        case WeaponKind::AssaultRifle:
            config.viewmodel_name = "models/weapons/v_assault_rifle/v_assault_rifle.mdl";
            config.zoom_fov = 40;
            config.ammo.clip_size = 45;
            config.ammo.default_clip = 45;
            break;
        case WeaponKind::Scattergun:
            config.viewmodel_name = "models/weapons/v_scattergun/v_scattergun.mdl";
            config.zoom_fov = 60;
            config.ammo.clip_size = 6;
            config.ammo.default_clip = 6;
            break;
        default:
            throw std::invalid_argument("unsupported SiN WeaponKind");
    }
    return config;
}

} // namespace sin_compat
