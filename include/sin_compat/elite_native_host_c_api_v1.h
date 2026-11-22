#ifndef SIN_COMPAT_ELITE_NATIVE_HOST_C_API_V1_H
#define SIN_COMPAT_ELITE_NATIVE_HOST_C_API_V1_H
/* Phase626: versioned C ABI for an ALREADY ENGINE-OWNED original
 * npc_druglab_grunt_elite. There is no Source vtable, entity allocator,
 * CreateInterface replacement or engine callback interception here.
 *
 * Single-thread/synchronous contract. Callbacks MUST NOT throw, reenter
 * the registry, release userdata, or mutate actor identities. All callbacks
 * return 1=success, 0=negative, -1=host error where stated.
 */
#include <stdint.h>
#if defined(_WIN32)
# if defined(SIN_ELITE_NATIVE_HOST_C_EXPORT)
#  define SIN_ELITE_NATIVE_API __declspec(dllexport)
# else
#  define SIN_ELITE_NATIVE_API __declspec(dllimport)
# endif
#elif defined(__GNUC__) || defined(__clang__)
# define SIN_ELITE_NATIVE_API __attribute__((visibility("default")))
#else
# define SIN_ELITE_NATIVE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum { SIN_ELITE_NATIVE_ABI_V1 = 1u };
enum {
  SIN_ELITE_NATIVE_OK=0,
  SIN_ELITE_NATIVE_EVENT_BLOCKED=1,
  SIN_ELITE_NATIVE_EVENT_FORWARDED=2,
  SIN_ELITE_NATIVE_INVALID_ARGUMENT=-1,
  SIN_ELITE_NATIVE_UNSUPPORTED_CLASS=-2,
  SIN_ELITE_NATIVE_INVALID_IDENTITY=-3,
  SIN_ELITE_NATIVE_DUPLICATE_INDEX=-4,
  SIN_ELITE_NATIVE_STALE_GENERATION=-5,
  SIN_ELITE_NATIVE_NOT_FOUND=-6,
  SIN_ELITE_NATIVE_HOST_REJECTED=-7,
  SIN_ELITE_NATIVE_REENTRANT=-8,
  SIN_ELITE_NATIVE_ALLOCATION_FAILED=-9,
  SIN_ELITE_NATIVE_ALREADY_SPAWNED=-10,
  SIN_ELITE_NATIVE_SPAWN_REQUIRED=-11,
  SIN_ELITE_NATIVE_UNSUPPORTED_X87=-12
};
enum {
  SIN_ELITE_DIRTY_NONE_V1=0u,
  SIN_ELITE_DIRTY_NATIVE_200_V1=1u,
  SIN_ELITE_DIRTY_EDICT_BIT0_V1=2u
};
enum {
  SIN_ELITE_EVENT_43_V1=0x43u,
  SIN_ELITE_HANDLE_INVALID_V1=0xffffffffu
};

typedef struct sin_elite_actor_v1 {
  uint32_t index,generation;
} sin_elite_actor_v1;

typedef struct sin_elite_native_host_v1 {
  uint32_t version,struct_size;
  /* The game host MUST check exact class, entity index+generation,
   * native factory ownership and the opaque owner entity token.
   * This callback runs at attach, Spawn, event43, and snapshot.
   * 1 = currently valid, 0 = stale/mismatch, -1 = host failure.
   */
  int32_t (*confirm_existing_native_elite)(
      void*,const char*,sin_elite_actor_v1,uint64_t);
} sin_elite_native_host_v1;

typedef struct sin_elite_native_attachment_v1 {
  uint32_t version,struct_size;
  const char* original_classname; /* exact npc_druglab_grunt_elite */
  sin_elite_actor_v1 actor;
  uint64_t owner_entity_token;
} sin_elite_native_attachment_v1;

/* Host-authoritative state sampled immediately before original Spawn. */
typedef struct sin_elite_native_spawn_snapshot_v1 {
  uint32_t version,struct_size;
  const char* current_weapon_symbol; /* null means original empty symbol */
  int32_t original_field_9c,original_field_98;
  uint32_t original_flags_b4;
  int32_t original_flag_1f0;
  int32_t original_edict_16c_present;
} sin_elite_native_spawn_snapshot_v1;

typedef struct sin_elite_native_spawn_callbacks_v1 {
  uint32_t version,struct_size;
  int32_t (*assign_original_weapon_symbol)(void*,const char*);
  int32_t (*native_virtual_68)(void*);
  int32_t (*native_virtual_1c_then_6c)(void*);
  int32_t (*inherited_npc_spawn)(void*);
  int32_t (*clear_byte_fd9)(void*);
  int32_t (*clear_byte_cb2_zero)(void*);
  /* Read twice, in the order +0x9C then +0x98; 1=valid f32. */
  int32_t (*read_original_convar_float_28)(void*,float*);
  int32_t (*virtual_1b4_then_write_field_9c)(void*,int32_t);
  int32_t (*virtual_1b0_then_write_field_98)(void*,int32_t);
  int32_t (*commit_flags_b4)(void*,uint32_t,uint32_t dirty_path);
  /* 1=typed CWeaponChaingun encoded native handle, 0=absent or wrong
   * type, -1=host error; out handle is used only when return is 1. */
  int32_t (*resolve_chaingun_encoded_handle)(void*,uint32_t*);
  int32_t (*write_field_fdc)(void*,uint32_t);
} sin_elite_native_spawn_callbacks_v1;

typedef struct sin_elite_native_spawn_result_v1 {
  uint32_t version,struct_size;
  int32_t failed_stage; /* corresponds to original SpawnReference::Stage */
  uint32_t flags_after;
  uint32_t handle_fdc;
  int32_t wrote_handle;
} sin_elite_native_spawn_result_v1;

typedef struct sin_elite_native_event43_callbacks_v1 {
  uint32_t version,struct_size;
  /* Called only if original Spawn saved a non-INVALID +0xFDC handle.
   * Return 1=resolved native entity token, 0=not found, -1=error. */
  int32_t (*resolve_serial_handle)(void*,uint32_t,uint64_t*);
  /* Called EVEN with zero token, exactly as original event 0x43.
   * 1=forward parent event, 0=block, -1=host failure. */
  int32_t (*original_gate43)(void*,uint64_t);
  /* Writes the original NPC's byte +0xFD9 BEFORE inherited dispatch. */
  int32_t (*publish_byte_fd9)(void*,uint8_t);
  int32_t (*forward_inherited_event43)(void*,float);
} sin_elite_native_event43_callbacks_v1;

typedef struct sin_elite_native_state_v1 {
  uint32_t version,struct_size;
  int32_t spawn_attempted,spawn_completed;
  uint32_t serial_handle_fdc;
  uint8_t predicate_fd9;
  uint8_t reserved[3];
} sin_elite_native_state_v1;

typedef struct sin_elite_native_registry_v1 sin_elite_native_registry_v1;
SIN_ELITE_NATIVE_API uint32_t sin_elite_native_abi_version_v1(void);
SIN_ELITE_NATIVE_API sin_elite_native_registry_v1*
sin_elite_native_create_v1(const sin_elite_native_host_v1*,void* userdata);
SIN_ELITE_NATIVE_API int32_t
sin_elite_native_destroy_v1(sin_elite_native_registry_v1*);
SIN_ELITE_NATIVE_API int32_t
sin_elite_native_attach_v1(sin_elite_native_registry_v1*,
                            const sin_elite_native_attachment_v1*);
/* A Spawn attempt is ONE-SHOT per attached generation even if a native
 * callback fails after partial original-engine effects. Detach/re-attach
 * only after actual engine removal/new generation. Output initialized on
 * valid pointers for both accepted and rejected calls.
 */
SIN_ELITE_NATIVE_API int32_t
sin_elite_native_spawn_v1(sin_elite_native_registry_v1*,
                           sin_elite_actor_v1,
                           const sin_elite_native_spawn_snapshot_v1*,
                           const sin_elite_native_spawn_callbacks_v1*,
                           void* spawn_userdata,
                           sin_elite_native_spawn_result_v1*);
/* Only authenticated 0x43 event path is supported, NOT events 0x03/0x47.
 * Requires completed original Spawn on the exact live generation.
 * Return 1=blocked, 2=forwarded, negative=status error.
 * If publishing succeeded but forwarding fails, native byte +0xFD9 may
 * already have changed; no unverified rollback is attempted.
 */
SIN_ELITE_NATIVE_API int32_t
sin_elite_native_event43_v1(sin_elite_native_registry_v1*,
                             sin_elite_actor_v1,
                             float native_parameter,
                             const sin_elite_native_event43_callbacks_v1*,
                             void* event_userdata);
SIN_ELITE_NATIVE_API int32_t
sin_elite_native_snapshot_v1(sin_elite_native_registry_v1*,
                              sin_elite_actor_v1,
                              sin_elite_native_state_v1*);
/* Exact-generation detach remains valid after engine object death. */
SIN_ELITE_NATIVE_API int32_t
sin_elite_native_detach_v1(sin_elite_native_registry_v1*,
                            sin_elite_actor_v1);
#ifdef __cplusplus
}
#endif
#endif
