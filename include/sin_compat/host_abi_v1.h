#ifndef SIN_COMPAT_HOST_ABI_V1_H
#define SIN_COMPAT_HOST_ABI_V1_H

/* Independently authored, C-callable integration ABI. No Valve/SiN headers.
 * This is NOT a SourceVR/Source engine module loader ABI.
 * Callbacks and session use one calling thread; host must not destroy a
 * session while callbacks execute. Nested calls on the same session return -1;
 * attempts to destroy from a callback are ignored and require explicit retry
 * after the outer operation completes. Host refs are opaque generations. */
#include <stdint.h>
#include <stddef.h>
#if defined(_WIN32)
# if defined(SIN_COMPAT_ABI_EXPORT)
#  define SIN_COMPAT_ABI __declspec(dllexport)
# else
#  define SIN_COMPAT_ABI __declspec(dllimport)
# endif
#elif defined(__GNUC__) || defined(__clang__)
# define SIN_COMPAT_ABI __attribute__((visibility("default")))
#else
# define SIN_COMPAT_ABI
#endif
#ifdef __cplusplus
extern "C" {
#endif

enum { SIN_COMPAT_ABI_VERSION_1 = 1 };
typedef struct sin_entity_ref_v1 { uint32_t index, generation; } sin_entity_ref_v1;
typedef struct sin_weapon_config_v1 {
    const char *viewmodel_name; /* copied during create; caller retains ownership */
    float origin_offset[3];
    int32_t zoom_fov;
    int32_t clip_size;
    int32_t clip2_size;
} sin_weapon_config_v1;

typedef struct sin_host_callbacks_v1 {
    uint32_t version;
    uint32_t struct_size;
    int (*is_live)(void *, sin_entity_ref_v1);
    sin_entity_ref_v1 (*owner_of)(void *, sin_entity_ref_v1);
    sin_entity_ref_v1 (*selected_weapon_of)(void *, sin_entity_ref_v1);
    void (*set_weapon_nodraw)(void *, sin_entity_ref_v1, int);
    int (*is_player_owner)(void *, sin_entity_ref_v1);
    int32_t (*viewmodel_index_of)(void *, sin_entity_ref_v1);
    int (*is_valid_viewmodel_index)(void *, sin_entity_ref_v1, int32_t);
    sin_entity_ref_v1 (*viewmodel_of)(void *, sin_entity_ref_v1, int32_t);
    int (*is_viewmodel_owned_by)(void *, sin_entity_ref_v1, sin_entity_ref_v1);
    void (*set_viewmodel_nodraw)(void *, sin_entity_ref_v1, int);
    int (*is_player)(void *, sin_entity_ref_v1);
    int32_t (*viewmodel_slot_count)(void *, sin_entity_ref_v1);
    sin_entity_ref_v1 (*viewmodel_at)(void *, sin_entity_ref_v1, int32_t);
    sin_entity_ref_v1 (*create_named_entity)(void *, const char *);
    int (*configure_viewmodel)(void *, sin_entity_ref_v1, sin_entity_ref_v1, int32_t);
    int (*store_viewmodel_handle)(void *, sin_entity_ref_v1, int32_t, sin_entity_ref_v1);
    void (*remove_viewmodel)(void *, sin_entity_ref_v1);
    void (*clear_viewmodel_handle)(void *, sin_entity_ref_v1, int32_t);
    int32_t (*resolve_sequence)(void *, int32_t);
    void (*bind_viewmodel)(void *, const char *, const float [3]);
    void (*play_sequence)(void *, int32_t);
    void (*set_flatscreen_fov)(void *, int32_t, float);
} sin_host_callbacks_v1;

typedef struct sin_session_v1 sin_session_v1;
typedef struct sin_ammo_v1 { int32_t clip, clip2, reserve, reserve2; } sin_ammo_v1;
/* 0=Magnum, 1=Assault Rifle, 2=Scattergun. Reject other values. */
SIN_COMPAT_ABI uint32_t sin_abi_version_v1(void);
SIN_COMPAT_ABI sin_session_v1 *sin_session_create_v1(
    const sin_host_callbacks_v1 *callbacks, void *userdata,
    sin_entity_ref_v1 player, sin_entity_ref_v1 weapon,
    int32_t kind, const sin_weapon_config_v1 *config, int vr_mode);
SIN_COMPAT_ABI void sin_session_destroy_v1(sin_session_v1 *session);
/* Returned values correspond to documented C++ enum result order;
 * -1 means null/invalid/host error. */
SIN_COMPAT_ABI int32_t sin_session_ensure_viewmodel_v1(sin_session_v1 *session);
SIN_COMPAT_ABI int32_t sin_session_hide_weapon_v1(sin_session_v1 *session);
/* bit 0 = entered zoom; bit 1 = exited zoom; bit 2 = auto restored;
 * bit 3 = interrupted; -1 = invalid session. */
SIN_COMPAT_ABI int32_t sin_session_usercmd_v1(sin_session_v1 *, uint32_t bits, int block);
SIN_COMPAT_ABI int32_t sin_session_reload_begin_v1(sin_session_v1 *, int32_t start_activity, int32_t end_activity);
SIN_COMPAT_ABI int32_t sin_session_reload_event_v1(sin_session_v1 *, int32_t *primary, int32_t *secondary);
SIN_COMPAT_ABI int32_t sin_session_reload_end_v1(sin_session_v1 *);
SIN_COMPAT_ABI int32_t sin_session_set_ammo_v1(sin_session_v1 *, sin_ammo_v1);
SIN_COMPAT_ABI int32_t sin_session_get_ammo_v1(sin_session_v1 *, sin_ammo_v1 *);
SIN_COMPAT_ABI int32_t sin_session_holster_v1(sin_session_v1 *);
SIN_COMPAT_ABI int32_t sin_session_cleanup_v1(sin_session_v1 *);
#ifdef __cplusplus
}
#endif
#endif
