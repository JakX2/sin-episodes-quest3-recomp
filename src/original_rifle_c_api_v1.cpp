#define SIN_ORIGINAL_RIFLE_BUILD_SHARED
#include "sin_compat/original_rifle_c_api_v1.h"
#include "sin_compat/original_assault_rifle_profile.hpp"

#include <cstring>

namespace {
using sin_compat::OriginalAssaultRifle;
}

extern "C" {

uint32_t sin_original_rifle_abi_version_v1(void) {
    return SIN_ORIGINAL_RIFLE_ABI_VERSION_1;
}

int32_t sin_original_rifle_get_info_v1(
    uint32_t version, sin_original_rifle_info_v1 *out) {
    if (!out || out->struct_bytes != sizeof(sin_original_rifle_info_v1))
        return -1;
    if (version != SIN_ORIGINAL_RIFLE_ABI_VERSION_1)
        return -2;
    *out = {
        static_cast<uint32_t>(sizeof(sin_original_rifle_info_v1)),
        SIN_ORIGINAL_RIFLE_ABI_VERSION_1,
        OriginalAssaultRifle::classname,
        OriginalAssaultRifle::viewmodel,
        OriginalAssaultRifle::worldmodel,
        OriginalAssaultRifle::npc_classname,
        OriginalAssaultRifle::npc_model,
        OriginalAssaultRifle::primary_ammo,
        OriginalAssaultRifle::secondary_ammo,
        OriginalAssaultRifle::secondary_projectile_classname,
        OriginalAssaultRifle::secondary_projectile_model,
        OriginalAssaultRifle::shot_sound,
        OriginalAssaultRifle::npc_shot_sound,
        OriginalAssaultRifle::clip_size,
        OriginalAssaultRifle::default_clip,
        OriginalAssaultRifle::clip2_size,
        OriginalAssaultRifle::default_clip2,
        OriginalAssaultRifle::zoom_fov
    };
    return 0;
}

sin_session_v1 *sin_original_rifle_create_session_v1(
    const char *classname, const sin_host_callbacks_v1 *callbacks, void *user,
    sin_entity_ref_v1 player, sin_entity_ref_v1 weapon, int vr_mode) {
    if (!classname || std::strcmp(classname, OriginalAssaultRifle::classname) != 0)
        return nullptr;
    const sin_weapon_config_v1 config{
        OriginalAssaultRifle::viewmodel,
        {0.0f, 0.0f, 0.0f}, // no undocumented viewmodel offsets
        OriginalAssaultRifle::zoom_fov,
        OriginalAssaultRifle::clip_size,
        OriginalAssaultRifle::clip2_size
    };
    sin_session_v1 *session = sin_session_create_v1(
        callbacks, user, player, weapon, 1, &config, vr_mode
    );
    if (!session) return nullptr;
    const sin_ammo_v1 original_spawn_ammo{
        OriginalAssaultRifle::default_clip, 0, 0, 0
    };
    if (sin_session_set_ammo_v1(session, original_spawn_ammo) != 0) {
        sin_session_destroy_v1(session);
        return nullptr;
    }
    return session;
}
}
