#ifndef SIN_COMPAT_ASSAULT_NPC_C_API_H
#define SIN_COMPAT_ASSAULT_NPC_C_API_H
/* Engine-neutral assault NPC callback ABI. No SourceVR game module or
 * 2006 Windows EHANDLE layout is implied. One caller thread per session;
 * callbacks must not throw/re-enter or destroy a live session.
 */
#include <stdint.h>
#if defined(_WIN32)
# if defined(SIN_ASSAULT_NPC_C_EXPORT)
#  define SIN_ASSAULT_API __declspec(dllexport)
# else
#  define SIN_ASSAULT_API __declspec(dllimport)
# endif
#elif defined(__GNUC__) || defined(__clang__)
# define SIN_ASSAULT_API __attribute__((visibility("default")))
#else
# define SIN_ASSAULT_API
#endif
#ifdef __cplusplus
extern "C" {
#endif

enum { SIN_ASSAULT_ABI_VERSION_1 = 1u, SIN_ASSAULT_MAX_CANDIDATES = 4096u };
enum {
    SIN_ASSAULT_OK = 0,
    SIN_ASSAULT_NO_CANDIDATE = 1,
    SIN_ASSAULT_LOCK_REJECTED = 2,
    SIN_ASSAULT_UNRESOLVED_RALLY = 3,
    SIN_ASSAULT_INVALID_ARGUMENT = -1,
    SIN_ASSAULT_HOST_FAILED = -2,
    SIN_ASSAULT_REENTRANT = -3,
    SIN_ASSAULT_NOT_BOUND = -4,
    SIN_ASSAULT_ALREADY_BOUND = -5
};
typedef struct sin_assault_ref_v1 {
    uint32_t index;
    uint32_t generation;
} sin_assault_ref_v1;

typedef struct sin_assault_candidate_v1 {
    uint32_t rally_id;             /* Engine-assigned stable per-level token. */
    int32_t priority;
    float origin[3];
    sin_assault_ref_v1 snapshot_owner; /* zero/zero means unowned. */
    uint32_t eligible;            /* Host's classname/group filter. */
} sin_assault_candidate_v1;

/* All callbacks return 1=yes/success, 0=no/failure, -1=error.
 * The host owns its real NPC, rally locks, registration and state fields.
 * For resolve_rally, 0 means selected handle cannot be resolved.
 * initialize_behavior must implement real additional reset effects (not ABI
 * offsets); it runs after lock+resolution and before committing cue fields.
 * Host MUST provide race-safe exclusive try_lock and matching unlock.
 * Caller retains userdata and callback code until destroy; no C++ throws.
 */
typedef struct sin_assault_host_v1 {
    uint32_t version;
    uint32_t struct_size;
    int32_t (*is_actor_live)(void *, sin_assault_ref_v1);
    int32_t (*try_lock_rally)(void *, uint32_t, sin_assault_ref_v1);
    int32_t (*resolve_rally)(void *, uint32_t);
    int32_t (*unlock_rally)(void *, uint32_t, sin_assault_ref_v1);
    int32_t (*initialize_behavior)(void *, sin_assault_ref_v1, uint32_t);
    int32_t (*reset_begin_assault_fields)(void *, sin_assault_ref_v1);
    int32_t (*set_npc_condition)(void *, sin_assault_ref_v1, uint32_t);
    int32_t (*has_npc_condition)(void *, sin_assault_ref_v1, uint32_t);
} sin_assault_host_v1;

typedef struct sin_assault_session_v1 sin_assault_session_v1;
typedef struct sin_assault_state_v1 {
    int32_t configured_mode;
    int32_t received_cue;
    uint32_t rally_id; /* zero when not bound */
} sin_assault_state_v1;

SIN_ASSAULT_API uint32_t sin_assault_abi_version_v1(void);
/* The host passes its registered condition IDs, NOT legacy 0x18 or 0x33
 * unless it independently and genuinely registered those values. */
SIN_ASSAULT_API sin_assault_session_v1 *sin_assault_create_v1(
    const sin_assault_host_v1 *, void *, sin_assault_ref_v1 actor,
    uint32_t registered_provoked, uint32_t registered_hear_combat);
/* Nested destruction is refused. The host must release a bound rally first. */
SIN_ASSAULT_API int32_t sin_assault_destroy_v1(sin_assault_session_v1 *);
/* The snapshot is read synchronously. If no candidate: cue state unchanged.
 * On invalid selected point: configured mode=0, received cue unchanged;
 * acquired lock is rolled back. If host unlock fails, the session retains
 * the rally token and must be released by finish_v1 before destruction.
 * Rollback and failure retention are NEW port safety policies.
 * A successful binding resets received cue=0, notifies host reset callback.
 */
SIN_ASSAULT_API int32_t sin_assault_bind_v1(sin_assault_session_v1 *,
    const float npc_origin[3], const sin_assault_candidate_v1 *candidates,
    uint32_t count, int32_t requested_mode, uint32_t *out_rally_id);
/* Resets two NPC fields via host callback, records received cue=1, then sets
 * host-registered COND_PROVOKED. Failed host condition writes restore cue.
 */
SIN_ASSAULT_API int32_t sin_assault_begin_v1(sin_assault_session_v1 *);
/* Polls host-registered COND_HEAR_COMBAT ONLY for configured mode 2,
 * unless received cue=4 override; mode 3 stores received cue=3. */
SIN_ASSAULT_API int32_t sin_assault_check_cue_v1(
    sin_assault_session_v1 *, uint32_t *out_ready);
/* Releases acquired rally and clears configured mode; received cue remains. */
SIN_ASSAULT_API int32_t sin_assault_finish_v1(sin_assault_session_v1 *);
SIN_ASSAULT_API int32_t sin_assault_get_state_v1(
    sin_assault_session_v1 *, sin_assault_state_v1 *out_state);
#ifdef __cplusplus
}
#endif
#endif
