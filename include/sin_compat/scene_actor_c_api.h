#ifndef SIN_COMPAT_SCENE_ACTOR_C_API_H
#define SIN_COMPAT_SCENE_ACTOR_C_API_H
/* Independent C ABI for the experimental JC/Jessica scene-actor host bridge.
 * THIS IS NOT a Source engine CreateInterface/ServerGameDLL module.
 * It contains no engine or game binaries and does not register map classnames.
 * A real game module must implement callbacks with verified Source APIs.
 * One caller thread per session; no reentry or concurrent destruction.
 */
#include <stddef.h>
#include <stdint.h>
#if defined(_WIN32)
# if defined(SIN_SCENE_C_API_EXPORT)
#  define SIN_SCENE_EXPORT __declspec(dllexport)
# else
#  define SIN_SCENE_EXPORT __declspec(dllimport)
# endif
#elif defined(__GNUC__) || defined(__clang__)
# define SIN_SCENE_EXPORT __attribute__((visibility("default")))
#else
# define SIN_SCENE_EXPORT
#endif
#ifdef __cplusplus
extern "C" {
#endif

enum { SIN_SCENE_ABI_VERSION_1 = 1 };
enum {
  SIN_SCENE_ACTOR_JC = 0,
  SIN_SCENE_ACTOR_JESSICA = 1,
  SIN_SCENE_ACTOR_GENERIC = 2
};
enum {
  SIN_SCENE_INPUT_VOID = 0,
  SIN_SCENE_INPUT_STRING = 1,
  SIN_SCENE_INPUT_INTEGER = 2
};
/* Mirrors sin_compat::SceneActorResult exactly for return codes >= 0.
 * -1 means a bad session, unavailable ABI callback or exception boundary. */
enum {
  SIN_SCENE_OK = 0,
  SIN_SCENE_INVALID_STATE = 1,
  SIN_SCENE_INVALID_ARGUMENT = 2,
  SIN_SCENE_HOST_FAILED = 3,
  SIN_SCENE_UNSUPPORTED_INPUT = 4,
  SIN_SCENE_WRONG_INPUT_TYPE = 5,
  SIN_SCENE_NO_PENDING_THROW = 6
};
typedef struct sin_scene_actor_ref_v1 {
  uint32_t index;
  uint32_t generation;
} sin_scene_actor_ref_v1;
/* Callbacks receive opaque userdata. Returned refs are valid if both words
 * are nonzero. All strings are valid, NUL-terminated during the callback.
 * The caller keeps callback function code and userdata live until destroy.
 * Do not throw exceptions across the C callback boundary. */
typedef struct sin_scene_callbacks_v1 {
  uint32_t version;
  uint32_t struct_size;
  sin_scene_actor_ref_v1 (*create_from_existing_base)(void *, const char *, const char *);
  int (*set_model)(void *, sin_scene_actor_ref_v1, const char *);
  int (*set_health)(void *, sin_scene_actor_ref_v1, int32_t);
  int (*set_targetname)(void *, sin_scene_actor_ref_v1, const char *);
  int (*choreography_capable)(void *, sin_scene_actor_ref_v1);
  int (*play_scene)(void *, sin_scene_actor_ref_v1, const char *, const char *);
  int (*holster_player_weapon)(void *);
  int (*throw_item_at_player)(void *, sin_scene_actor_ref_v1, const char *, int32_t, int32_t);
  void (*remove_actor)(void *, sin_scene_actor_ref_v1);
} sin_scene_callbacks_v1;

typedef struct sin_scene_session_v1 sin_scene_session_v1;
SIN_SCENE_EXPORT uint32_t sin_scene_abi_version_v1(void);
SIN_SCENE_EXPORT sin_scene_session_v1 *sin_scene_create_v1(
    const sin_scene_callbacks_v1 *callbacks, void *userdata, int32_t actor_kind);
/* Will refuse to destroy during a nested callback; host must retry afterward. */
SIN_SCENE_EXPORT void sin_scene_destroy_v1(sin_scene_session_v1 *session);
/* targetname and model_override may be NULL to use empty/no override. */
SIN_SCENE_EXPORT int32_t sin_scene_spawn_v1(sin_scene_session_v1 *session,
    const char *targetname, const char *model_override);
/* Requires original VCD file path and exact actor token, including !Target1. */
SIN_SCENE_EXPORT int32_t sin_scene_begin_v1(sin_scene_session_v1 *session,
    const char *scene_file, const char *actor_token);
/* For string input pass string_value; for integer input pass integer_value. */
SIN_SCENE_EXPORT int32_t sin_scene_input_v1(sin_scene_session_v1 *session,
    const char *name, int32_t type, const char *string_value, int32_t integer_value);
SIN_SCENE_EXPORT int32_t sin_scene_animation_event_v1(sin_scene_session_v1 *session,
    const char *symbol);
/* Returns 0 only after successful spawn, otherwise -1. */
SIN_SCENE_EXPORT int32_t sin_scene_get_actor_v1(
    sin_scene_session_v1 *session, sin_scene_actor_ref_v1 *out);
#ifdef __cplusplus
}
#endif
#endif
