#ifndef SIN_COMPAT_WEAPON_C_BRIDGE_H
#define SIN_COMPAT_WEAPON_C_BRIDGE_H

#include <stdint.h>

/* ABI for embedding the independently reconstructed SiN weapon runtime into
 * a native host. NOT Source Engine's CreateInterface/vtable ABI. */
#define SIN_WEAPON_BRIDGE_ABI_VERSION 1u

#if defined(_WIN32)
# if defined(SIN_WEAPON_BRIDGE_BUILD)
#  define SIN_WEAPON_API __declspec(dllexport)
# else
#  define SIN_WEAPON_API __declspec(dllimport)
# endif
#elif defined(__GNUC__) || defined(__clang__)
# define SIN_WEAPON_API __attribute__((visibility("default")))
#else
# define SIN_WEAPON_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SinWeaponHandle SinWeaponHandle;

typedef enum SinWeaponKind {
    SIN_WEAPON_MAGNUM = 0,
    SIN_WEAPON_ASSAULT_RIFLE = 1,
    SIN_WEAPON_SCATTERGUN = 2
} SinWeaponKind;

typedef struct SinWeaponSettings {
    const char* viewmodel_name;
    float origin_offset[3];
    int32_t zoom_fov;
    int32_t clip_size;
    int32_t clip2_size;
} SinWeaponSettings;

typedef struct SinWeaponHostCallbacks {
    int32_t (*resolve_sequence)(void* user, int32_t activity_id);
    void (*bind_viewmodel)(void* user, const char* model, const float origin_offset[3]);
    void (*play_sequence)(void* user, int32_t sequence_id);
    void (*set_flatscreen_fov)(void* user, int32_t fov, float seconds);
} SinWeaponHostCallbacks;

typedef struct SinWeaponFrameResult {
    int32_t entered_zoom;
    int32_t exited_zoom;
    int32_t automatically_restored;
    int32_t was_interrupted;
} SinWeaponFrameResult;

typedef struct SinWeaponAmmo {
    int32_t clip;
    int32_t reserve;
    int32_t clip2;
    int32_t reserve2;
    int32_t reload_active;
    int32_t zoom_active;
    int32_t holstered;
} SinWeaponAmmo;

/* All functions are safe to call with a null handle. Invalid input or an
 * internal failure returns 0 (or NULL) without throwing across the C ABI.
 * The caller owns the handle. All callbacks must outlive the handle.
 * No callbacks are invoked by sin_weapon_create(). */
SIN_WEAPON_API uint32_t sin_weapon_abi_version(void);
SIN_WEAPON_API SinWeaponHandle* sin_weapon_create(
    int32_t kind, const SinWeaponSettings* settings,
    const SinWeaponHostCallbacks* callbacks, void* user, int32_t vr_mode);
SIN_WEAPON_API void sin_weapon_destroy(SinWeaponHandle* handle);
SIN_WEAPON_API int32_t sin_weapon_frame(
    SinWeaponHandle* handle, uint32_t buttons, int32_t player_exists,
    int32_t owner_ready, int32_t external_zoom_block, SinWeaponFrameResult* out);
SIN_WEAPON_API int32_t sin_weapon_set_ammo(
    SinWeaponHandle* handle, int32_t clip, int32_t reserve,
    int32_t clip2, int32_t reserve2);
SIN_WEAPON_API int32_t sin_weapon_get_ammo(
    const SinWeaponHandle* handle, SinWeaponAmmo* out);
SIN_WEAPON_API int32_t sin_weapon_begin_reload(
    SinWeaponHandle* handle, int32_t start_activity, int32_t end_activity);
SIN_WEAPON_API int32_t sin_weapon_reload_transfer_event(
    SinWeaponHandle* handle, int32_t owner_valid,
    int32_t* primary_transferred, int32_t* secondary_transferred);
SIN_WEAPON_API int32_t sin_weapon_finish_reload(SinWeaponHandle* handle);
SIN_WEAPON_API int32_t sin_weapon_primary_attack(
    SinWeaponHandle* handle, int32_t rifle_recoil_counter);
SIN_WEAPON_API int32_t sin_weapon_secondary_attack(SinWeaponHandle* handle);
SIN_WEAPON_API int32_t sin_weapon_holster(SinWeaponHandle* handle);

#ifdef __cplusplus
}
#endif
#endif
