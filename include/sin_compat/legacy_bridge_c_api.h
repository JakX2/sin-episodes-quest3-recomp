#ifndef SIN_COMPAT_PHASE100_LEGACY_BRIDGE_C_API_H
#define SIN_COMPAT_PHASE100_LEGACY_BRIDGE_C_API_H
#include <stdint.h>
#if defined(_WIN32)
 #if defined(SIN100_BUILD_SHARED)
  #define SIN100_API __declspec(dllexport)
 #else
  #define SIN100_API __declspec(dllimport)
 #endif
#else
 #define SIN100_API __attribute__((visibility("default")))
#endif
#ifdef __cplusplus
extern "C" {
#endif
#define SIN100_ABI_VERSION 0x00010000u
#define SIN100_KEYMAP_BYTE_COUNT 1024u
#define SIN100_NOT_FOUND_TOKEN 0xffffffffu
typedef enum sin100_status {
  SIN100_OK = 0,
  SIN100_ALREADY_PRESENT = 1,
  SIN100_INVALID_ARGUMENT = -1,
  SIN100_BAD_VERSION = -2,
  SIN100_NOT_READY = -3,
  SIN100_HASH_MISMATCH = -4,
  SIN100_MAP_INVALID = -5,
  SIN100_LEGACY_INDEX_INVALID = -6,
  SIN100_VALIDATION_REQUIRED = -7,
  SIN100_NAME_MISMATCH = -8,
  SIN100_MAPPING_CONFLICT = -9,
  SIN100_NOT_FOUND = -10,
  SIN100_BUFFER_TOO_SMALL = -11,
  SIN100_INTERNAL_ERROR = -12,
  SIN100_NATIVE_INDEX_INVALID = -13
} sin100_status;
typedef struct sin100_context sin100_context;
SIN100_API uint32_t sin100_abi_version(void);
SIN100_API sin100_status sin100_create(uint32_t requested_abi, sin100_context** out);
SIN100_API void sin100_destroy(sin100_context* ctx);
// Exact 1024-byte little-endian original key table, privately derived from
// owner-held 2006 vgui2.dll. Independently verified against embedded SHA256.
// Any failed install resets translator to fail-closed.
SIN100_API sin100_status sin100_install_keymap_le(sin100_context* ctx,const uint8_t* bytes,uint32_t count);
SIN100_API sin100_status sin100_translate_key(sin100_context* ctx,int32_t legacy_key,uint32_t* translated);
// native_validated is asserted by a TRUSTED caller, NOT independently checked
// against SourceVR vtable ABI here. No engine method is invoked by this module.
// Names are bounded length-delimited inputs, embedded NUL rejected.
SIN100_API sin100_status sin100_bind_sound(sin100_context* ctx,int32_t legacy_id,int32_t legacy_count,
  int64_t native_candidate,uint32_t native_validated,const char* legacy_name,uint32_t legacy_name_len,
  const char* native_roundtrip_name,uint32_t native_name_len);
SIN100_API sin100_status sin100_old_to_native(sin100_context* ctx,int32_t legacy_id,uint32_t* native_id);
SIN100_API sin100_status sin100_native_to_old(sin100_context* ctx,uint32_t native_id,uint32_t* legacy_id);
// Copies NUL-terminated name to CALLER-owned buffer. Required includes NUL.
// To query length first, call with destination=NULL and capacity=0.
SIN100_API sin100_status sin100_copy_sound_name(sin100_context* ctx,int32_t legacy_id,
  char* destination,uint32_t capacity,uint32_t* required);
SIN100_API sin100_status sin100_sound_count(sin100_context* ctx,uint32_t* count);
SIN100_API sin100_status sin100_reset(sin100_context* ctx);
#ifdef __cplusplus
}
#endif
#endif
