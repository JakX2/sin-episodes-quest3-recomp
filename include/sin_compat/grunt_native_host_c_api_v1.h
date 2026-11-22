#ifndef SIN_COMPAT_GRUNT_NATIVE_HOST_C_API_V1_H
#define SIN_COMPAT_GRUNT_NATIVE_HOST_C_API_V1_H
/* Phase619: independent C attachment API for ALREADY ENGINE-OWNED Grunts.
 * Does not allocate an NPC, hook Source vtables, or implement CreateInterface.
 * All input pointers/callback userdata must stay valid for their documented
 * call lifetime. Use on one host thread; never call destroy from callbacks.
 */
#include "sin_compat/grunt_behavior_c_api_v1.h"
#include <stdint.h>
#if defined(_WIN32)
# if defined(SIN_GRUNT_NATIVE_HOST_C_EXPORT)
#  define SIN_GRUNT_NATIVE_API __declspec(dllexport)
# else
#  define SIN_GRUNT_NATIVE_API __declspec(dllimport)
# endif
#elif defined(__GNUC__) || defined(__clang__)
# define SIN_GRUNT_NATIVE_API __attribute__((visibility("default")))
#else
# define SIN_GRUNT_NATIVE_API
#endif
#ifdef __cplusplus
extern "C" {
#endif

enum { SIN_GRUNT_NATIVE_ABI_V1 = 1u };
enum {
    SIN_GRUNT_NATIVE_OK = 0,
    SIN_GRUNT_NATIVE_UNSUPPORTED_CLASS = -10,
    SIN_GRUNT_NATIVE_INVALID_IDENTITY = -11,
    SIN_GRUNT_NATIVE_HOST_REJECTED = -12,
    SIN_GRUNT_NATIVE_DUPLICATE_INDEX = -13,
    SIN_GRUNT_NATIVE_STALE_GENERATION = -14,
    SIN_GRUNT_NATIVE_NOT_FOUND = -15,
    SIN_GRUNT_NATIVE_REENTRANT = -16,
    SIN_GRUNT_NATIVE_ALLOCATION_FAILED = -17,
    SIN_GRUNT_NATIVE_INVALID_ARGUMENT = -18
};

/* Only the four retail CNPC_MobGrunt factory names are supported: the
 * default npc_druglab_grunt and three explicit weapon variants. The elite
 * classname is a different original CEntityFactory/CNPC_EliteGrunt type.
 * The host must verify the classname, native factory ownership, full
 * (entity index, generation) AND original opaque owner token against its
 * current engine-owned entity. Returns 1 = confirmed, 0 = not current,
 * -1 = host error. The callback must not throw across this C boundary.
 * The registry checks it on attach AND before each step/snapshot.
 */
typedef struct sin_grunt_native_host_v1 {
    uint32_t version, struct_size;
    int32_t (*confirm_existing_native_npc)(
        void *userdata, const char *original_classname,
        sin_grunt_actor_v1 actor, uint64_t owner_entity_token);
} sin_grunt_native_host_v1;

typedef struct sin_grunt_native_attachment_v1 {
    uint32_t version, struct_size;
    const char *original_classname; /* NUL terminated, valid during attach */
    sin_grunt_actor_v1 actor;
    uint64_t owner_entity_token;
    uint32_t registered_condition_3b, registered_condition_18;
    sin_grunt_assault_state_v1 initial_assault;
    sin_grunt_behavior_host_v1 behavior_callbacks;
    void *behavior_userdata; /* valid until detach/destroy */
} sin_grunt_native_attachment_v1;

typedef struct sin_grunt_native_registry_v1 sin_grunt_native_registry_v1;
SIN_GRUNT_NATIVE_API uint32_t sin_grunt_native_abi_version_v1(void);
SIN_GRUNT_NATIVE_API sin_grunt_native_registry_v1 *
sin_grunt_native_create_v1(const sin_grunt_native_host_v1 *host,
                           void *host_userdata);
SIN_GRUNT_NATIVE_API int32_t
sin_grunt_native_destroy_v1(sin_grunt_native_registry_v1 *registry);
SIN_GRUNT_NATIVE_API int32_t
sin_grunt_native_attach_v1(sin_grunt_native_registry_v1 *registry,
                           const sin_grunt_native_attachment_v1 *attachment);
/* On valid output pointers, selected and underlying_behavior_status are
 * always initialized, including rejection. Host callback failures may have
 * partial external effects; they are not silently rolled back.
 */
SIN_GRUNT_NATIVE_API int32_t
sin_grunt_native_step_v1(sin_grunt_native_registry_v1 *registry,
                         sin_grunt_actor_v1 actor, float now,
                         int32_t *out_selected,
                         int32_t *out_underlying_behavior_status);
/* Snapshot output is unmodified on refusal. */
SIN_GRUNT_NATIVE_API int32_t
sin_grunt_native_snapshot_v1(sin_grunt_native_registry_v1 *registry,
                             sin_grunt_actor_v1 actor,
                             sin_grunt_snapshot_v1 *out);
SIN_GRUNT_NATIVE_API int32_t
sin_grunt_native_detach_v1(sin_grunt_native_registry_v1 *registry,
                           sin_grunt_actor_v1 actor);
#ifdef __cplusplus
}
#endif
#endif
