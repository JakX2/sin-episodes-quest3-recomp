#ifndef SIN_COMPAT_ORIGINAL_RIFLE_C_API_V1_H
#define SIN_COMPAT_ORIGINAL_RIFLE_C_API_V1_H

/* Portable C ABI for the original 2006 SiN assault-rifle profile and weapon
 * session. This is NOT Source's CreateEntityByName or an engine entity factory.
 * The native SourceVR game module must separately register weapon_assault_rifle
 * and provide real host callbacks before the returned session is playable.
 *
 * No pointers to the original x86 DLL or non-public Source engine internals
 * cross this boundary. Returned string pointers are process-lifetime constants
 * owned by this shared library; never free or mutate them.
 */
#include "sin_compat/host_abi_v1.h"
#include <stdint.h>

#if defined(_WIN32)
# if defined(SIN_ORIGINAL_RIFLE_BUILD_SHARED)
#  define SIN_ORIGINAL_RIFLE_API __declspec(dllexport)
# else
#  define SIN_ORIGINAL_RIFLE_API __declspec(dllimport)
# endif
#else
# define SIN_ORIGINAL_RIFLE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum { SIN_ORIGINAL_RIFLE_ABI_VERSION_1 = 1u };

typedef struct sin_original_rifle_info_v1 {
    uint32_t struct_bytes; /* caller sets to sizeof(sin_original_rifle_info_v1) */
    uint32_t abi_version;
    const char *classname;
    const char *viewmodel;
    const char *worldmodel;
    const char *npc_classname;
    const char *npc_model;
    const char *primary_ammo;
    const char *secondary_ammo;
    const char *secondary_projectile;
    const char *secondary_projectile_model;
    const char *shot_sound;
    const char *npc_shot_sound;
    int32_t clip_size;
    int32_t default_clip;
    int32_t clip2_size;
    int32_t default_clip2;
    int32_t zoom_fov; /* flatscreen only; headset HMD FOV remains unchanged */
} sin_original_rifle_info_v1;

/* 0 success; -1 invalid pointer/struct size, -2 wrong version.
 * No callback or engine state is accessed in this metadata function.
 */
SIN_ORIGINAL_RIFLE_API uint32_t sin_original_rifle_abi_version_v1(void);
SIN_ORIGINAL_RIFLE_API int32_t sin_original_rifle_get_info_v1(
    uint32_t requested_abi, sin_original_rifle_info_v1 *out);

/* Creates the engine-NEUTRAL WeaponSession using ORIGINAL SiN weapon metadata,
 * not a new Source entity or pickup. Requires the existing host v1 callback
 * table; rejects every classname except exact "weapon_assault_rifle".
 * Defaults clip to the original 45-round spawn capacity, no reserve ammo.
 * Returns NULL for bad inputs, callback ABI failure or allocation failure.
 * Destroy with sin_session_destroy_v1 from the existing host ABI library.
 */
SIN_ORIGINAL_RIFLE_API sin_session_v1 *sin_original_rifle_create_session_v1(
    const char *classname, const sin_host_callbacks_v1 *callbacks, void *userdata,
    sin_entity_ref_v1 player, sin_entity_ref_v1 weapon, int vr_mode);

#ifdef __cplusplus
}
#endif
#endif
