#ifndef SIN_COMPAT_EVENT_BRIDGE_V1_H
#define SIN_COMPAT_EVENT_BRIDGE_V1_H

/* Independently authored engine-facing C ABI. This is not a SourceVR plugin,
 * Source CreateInterface implementation, nor the original SiN game DLL ABI.
 *
 * Host callback userdata and referenced entities must remain valid until
 * detaching/destroying the bridge. Exactly one calling thread is supported.
 * Calls from a callback on the same bridge return -1; destroy from a callback
 * is ignored and must be explicitly retried after the outer call.
 *
 * The bridge library depends on sin_compat_host_abi at runtime. Both .so
 * libraries must be deployed together on Linux/Android.
 */
#include "sin_compat/host_abi_v1.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sin_event_bridge_v1 sin_event_bridge_v1;
enum { SIN_EVENT_BRIDGE_ABI_VERSION_1 = 1 };

typedef enum sin_bridge_event_v1 {
    SIN_BRIDGE_EQUIP_V1 = 1,
    SIN_BRIDGE_HIDE_WEAPON_V1 = 2,
    SIN_BRIDGE_USERCMD_V1 = 3,
    SIN_BRIDGE_RELOAD_BEGIN_V1 = 4,
    SIN_BRIDGE_RELOAD_TRANSFER_V1 = 5,
    SIN_BRIDGE_RELOAD_END_V1 = 6,
    SIN_BRIDGE_SET_AMMO_V1 = 7,
    SIN_BRIDGE_GET_AMMO_V1 = 8,
    SIN_BRIDGE_HOLSTER_V1 = 9,
    SIN_BRIDGE_MAP_UNLOAD_V1 = 10,
    SIN_BRIDGE_WEAPON_REMOVED_V1 = 11
} sin_bridge_event_v1;

typedef struct sin_bridge_event_input_v1 {
    uint32_t buttons;       /* USER_CMD: Source style button mask */
    int32_t blocked;        /* USER_CMD: movement/ADS gate */
    int32_t start_activity; /* RELOAD_BEGIN */
    int32_t end_activity;   /* RELOAD_BEGIN */
    sin_ammo_v1 ammo;       /* SET_AMMO */
} sin_bridge_event_input_v1;

typedef struct sin_bridge_event_output_v1 {
    int32_t primary_transfer;    /* RELOAD_TRANSFER */
    int32_t secondary_transfer;  /* RELOAD_TRANSFER */
    sin_ammo_v1 ammo;            /* GET_AMMO */
} sin_bridge_event_output_v1;

/* Exported ABI surface, all resolved with dlsym/GetProcAddress.
 * - create returns null for invalid callbacks/version/alloc failure
 * - attach: 1=attached, 0=rejected, -1=invalid/reentrant call
 * - dispatch: result of underlying v1 session method; -1 for
 *   invalid event, detached state, stale player/weapon or reentrancy.
 * - MAP_UNLOAD and WEAPON_REMOVED return 0 and detach, even if idle.
 * - for output-producing events, output is only written on success.
 * - is_active: 1/0 or -1 if null/reentrant.
 */
SIN_COMPAT_ABI uint32_t sin_event_bridge_abi_version_v1(void);
SIN_COMPAT_ABI sin_event_bridge_v1 *sin_event_bridge_create_v1(
    const sin_host_callbacks_v1 *callbacks, void *userdata);
SIN_COMPAT_ABI int32_t sin_event_bridge_attach_v1(
    sin_event_bridge_v1 *bridge, sin_entity_ref_v1 player, sin_entity_ref_v1 weapon,
    int32_t kind, const sin_weapon_config_v1 *config, int32_t vr_mode);
SIN_COMPAT_ABI int32_t sin_event_bridge_dispatch_v1(
    sin_event_bridge_v1 *bridge, int32_t event,
    const sin_bridge_event_input_v1 *input, sin_bridge_event_output_v1 *output);
SIN_COMPAT_ABI int32_t sin_event_bridge_is_active_v1(const sin_event_bridge_v1 *bridge);
SIN_COMPAT_ABI void sin_event_bridge_destroy_v1(sin_event_bridge_v1 *bridge);

#ifdef __cplusplus
}
#endif
#endif
