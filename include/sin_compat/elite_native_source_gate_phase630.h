#ifndef SIN_ELITE_NATIVE_SOURCE_GATE_PHASE630_H
#define SIN_ELITE_NATIVE_SOURCE_GATE_PHASE630_H
/* Gameplay Phase630: independent C bridge from the existing Phase626
 * engine-owned Elite registry to Phase629's original event43 gate/update.
 * The original original 2006 DLL is never loaded/called by this library.
 * ALL callbacks are synchronous and must be called on the game thread.
 * Native game entity ownership, class/serial validation and memory writes
 * are exclusively responsibilities of the actual engine host.
 *
 * Links *against*, but does NOT alter, Phase626's exact eight-export v1 DLL.
 */
#include "sin_compat/elite_native_host_c_api_v1.h"
#include <stdint.h>
#if defined(_WIN32)
# if defined(SIN_ELITE_SOURCE_GATE_C_EXPORT)
#  define SIN_ELITE_SOURCE_GATE_API __declspec(dllexport)
# else
#  define SIN_ELITE_SOURCE_GATE_API __declspec(dllimport)
# endif
#elif defined(__GNUC__) || defined(__clang__)
# define SIN_ELITE_SOURCE_GATE_API __attribute__((visibility("default")))
#else
# define SIN_ELITE_SOURCE_GATE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum { SIN_ELITE_SOURCE_GATE_ABI_V1=1u };
enum {
  SIN_ELITE_SOURCE_NOT_INVOKED_V1=0,
  SIN_ELITE_SOURCE_GATE_BLOCKED_V1=1,
  SIN_ELITE_SOURCE_GATE_ALLOWED_V1=2,
  SIN_ELITE_SOURCE_UPDATE_ADVANCED_V1=3,
  SIN_ELITE_SOURCE_UPDATE_UNCHANGED_V1=4,
  SIN_ELITE_SOURCE_TARGET_MISSING_V1=-20,
  SIN_ELITE_SOURCE_NATIVE_CALLBACK_FAILED_V1=-21
};
typedef struct sin_elite_source_gate_callbacks_v1 {
  uint32_t version,struct_size;
  /* Read actual Elite+0xFDC on every event/tick; cannot cache Spawn handle. */
  int32_t (*read_current_serial_handle_fdc)(void*,uint32_t*);
  /* Check serial AND current resolved owner; return 1 with nonzero token,
   * 0 if stale or absent, -1 if source lookup failed. */
  int32_t (*resolve_serial_handle)(void*,uint32_t,uint64_t*);
  /* Host-verified resolved target, exact original fields: DWORD +0x594
   * and f32 +0x590/+0x58C. No assumption about target class name. */
  int32_t (*read_resolved_state)(void*,uint64_t,uint32_t*,float*,float*);
  /* These callbacks must commit to the *actual* native target at the
   * source instruction's order, not merely mutate a shadow struct. */
  int32_t (*write_phase_594)(void*,uint64_t,uint32_t);
  int32_t (*write_time_590)(void*,uint64_t,float);
  int32_t (*write_time_58c)(void*,uint64_t,float);
  /* Exact original resolved-object virtual offsets, host supplied only.
   * Never dispatch to guessed Source SDK vtables. */
  int32_t (*virtual_3c4)(void*,uint64_t,uint32_t,uint32_t);
  int32_t (*virtual_3c8)(void*,uint64_t,uint32_t);
  /* Original global 0x22572190->float+0x0C: independent reads required. */
  int32_t (*read_original_engine_time)(void*,float*);
  /* Only event43 uses these callbacks; tick never publishes FD9 or
   * directly invokes the inherited animation event. */
  int32_t (*publish_elite_fd9)(void*,uint8_t);
  int32_t (*forward_inherited_event43)(void*,float);
} sin_elite_source_gate_callbacks_v1;

typedef struct sin_elite_source_gate_result_v1 {
  uint32_t version,struct_size;
  int32_t source_status; /* one of SIN_ELITE_SOURCE_*_V1 */
  int32_t failed_stage; /* Phase629 Stage enum, zero=None */
  uint32_t phase_before,phase_after;
  uint32_t current_fdc;
  int32_t target_valid;
} sin_elite_source_gate_result_v1;

SIN_ELITE_SOURCE_GATE_API uint32_t
sin_elite_source_gate_abi_version_v1(void);
/* Reuses Phase626's own actor-generation and Spawn checks, live handle
 * read, serial resolution and FD9/parent sequencing. With a null or stale
 * handle, rejects safely (Phase629 native ECX dereferences without guard).
 * Returns Phase626 BLOCKED(1), FORWARDED(2), or a negative error code.
 */
SIN_ELITE_SOURCE_GATE_API int32_t
sin_elite_source_gate_event43_v1(
    sin_elite_native_registry_v1*,sin_elite_actor_v1,float,
    const sin_elite_source_gate_callbacks_v1*,void*,
    sin_elite_source_gate_result_v1*);
/* Advance the original gate target's 1-second state machine.
 * Enforces Phase626 actor generation and completed Spawn first.
 * Returns SOURCE_UPDATE_ADVANCED(3)/UNCHANGED(4), or negative status.
 */
SIN_ELITE_SOURCE_GATE_API int32_t
sin_elite_source_gate_update_v1(
    sin_elite_native_registry_v1*,sin_elite_actor_v1,
    const sin_elite_source_gate_callbacks_v1*,void*,
    sin_elite_source_gate_result_v1*);
#ifdef __cplusplus
}
#endif
#endif
