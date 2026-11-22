#ifndef SIN_COMPAT_GRUNT_BEHAVIOR_C_API_V1_H
#define SIN_COMPAT_GRUNT_BEHAVIOR_C_API_V1_H
/* Phase613: independent, versioned Grunt behavior-host ABI.
 * Only one caller thread per session. Callbacks must not throw, reenter,
 * or release live userdata. This is NOT the Source CreateInterface ABI.
 */
#include <stdint.h>
#if defined(_WIN32)
# if defined(SIN_GRUNT_BEHAVIOR_C_EXPORT)
#  define SIN_GRUNT_API __declspec(dllexport)
# else
#  define SIN_GRUNT_API __declspec(dllimport)
# endif
#elif defined(__GNUC__) || defined(__clang__)
# define SIN_GRUNT_API __attribute__((visibility("default")))
#else
# define SIN_GRUNT_API
#endif
#ifdef __cplusplus
extern "C" {
#endif

enum { SIN_GRUNT_BEHAVIOR_ABI_V1 = 1u, SIN_GRUNT_BEHAVIOR_COUNT_V1 = 8u,
       SIN_GRUNT_ASSAULT_INDEX_V1 = 5u };
enum { SIN_GRUNT_BEHAVIOR_OK = 0, SIN_GRUNT_BEHAVIOR_NO_ELIGIBLE = 1,
       SIN_GRUNT_BEHAVIOR_INVALID = -1, SIN_GRUNT_BEHAVIOR_HOST_FAILED = -2,
       SIN_GRUNT_BEHAVIOR_ACTOR_EXPIRED = -3,
       SIN_GRUNT_BEHAVIOR_REENTRANT = -4 };

typedef struct sin_grunt_actor_v1 {uint32_t index, generation;} sin_grunt_actor_v1;
typedef struct sin_grunt_assault_state_v1 {
    uint32_t handle_14, handle_18, raw_1c;
    uint8_t byte_24, byte_25, byte_26, reserved;
    float time_28, deadline_2c;
} sin_grunt_assault_state_v1;
typedef struct sin_grunt_snapshot_v1 {
    int32_t current_index; /* -1 means none */
    sin_grunt_assault_state_v1 assault;
} sin_grunt_snapshot_v1;

/* Every callback returns 1 for success/yes, 0 for no/not found,
 * and -1 for error. Out-parameters are valid only when returning 1.
 * The host maps raw SiN condition 0x3B and 0x18 to its OWN registered IDs
 * supplied during create. Do not reuse 2006 condition numbers blindly.
 * Other behavior eligibility is delegated because the remaining seven
 * source-verified methods have not all been ported to this C interface.
 * Entity tokens are host-owned opaque identities, NOT 2006 C++ pointers.
 */
typedef struct sin_grunt_behavior_host_v1 {
    uint32_t version, struct_size;
    int32_t (*actor_live)(void *, sin_grunt_actor_v1);
    int32_t (*other_eligible)(void *, sin_grunt_actor_v1, uint32_t index);
    int32_t (*owner_gate)(void *, sin_grunt_actor_v1);
    int32_t (*has_condition)(void *, sin_grunt_actor_v1, uint32_t host_condition);
    int32_t (*owner_target_nonnull)(void *, sin_grunt_actor_v1);
    int32_t (*serial_entity_present)(void *, uint32_t index, uint32_t serial);
    int32_t (*primary_target_byte)(void *, uint32_t encoded, uint8_t *out_byte_3b9);
    int32_t (*set_condition)(void *, sin_grunt_actor_v1, uint32_t host_condition);
    int32_t (*resolve_entity)(void *, uint32_t encoded, uint64_t *out_token);
    int32_t (*read_entity_link)(void *, uint64_t entity_token, uint32_t *out_encoded);
    int32_t (*write_entity_link)(void *, uint64_t entity_token, uint32_t encoded);
    /* stage 0: new behavior v+0x1C; stage 1: new v+0x24;
     * stage 2: old behavior v+0x20, ONLY for non-Assault classes.
     * Assault stage-2 callback is implemented by the Phase611 model.
     */
    int32_t (*behavior_lifecycle)(void *, sin_grunt_actor_v1, uint32_t index,
                                  uint32_t stage);
    int32_t (*owner_transition)(void *, sin_grunt_actor_v1,
                                int32_t old_index, int32_t new_index);
} sin_grunt_behavior_host_v1;

typedef struct sin_grunt_behavior_session_v1 sin_grunt_behavior_session_v1;
SIN_GRUNT_API uint32_t sin_grunt_behavior_abi_version_v1(void);
SIN_GRUNT_API sin_grunt_behavior_session_v1 *sin_grunt_behavior_create_v1(
    const sin_grunt_behavior_host_v1 *, void *userdata, sin_grunt_actor_v1 actor,
    uint64_t owner_entity_token, uint32_t registered_condition_3b,
    uint32_t registered_condition_18,
    const sin_grunt_assault_state_v1 *initial_assault);
SIN_GRUNT_API int32_t sin_grunt_behavior_destroy_v1(sin_grunt_behavior_session_v1 *);
/* A failed host callback can leave partial externally visible side effects.
 * The host must reconcile them before retrying. There is no invented rollback.
 */
SIN_GRUNT_API int32_t sin_grunt_behavior_step_v1(
    sin_grunt_behavior_session_v1 *, float now, int32_t *out_current_index);
SIN_GRUNT_API int32_t sin_grunt_behavior_snapshot_v1(
    sin_grunt_behavior_session_v1 *, sin_grunt_snapshot_v1 *out);
#ifdef __cplusplus
}
#endif
#endif
