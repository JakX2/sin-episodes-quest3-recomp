#ifndef SIN_COMPAT_PHASE101_SOURCE_PROVIDER_C_API_H
#define SIN_COMPAT_PHASE101_SOURCE_PROVIDER_C_API_H
#include <stdint.h>
#if defined(_WIN32)
 #if defined(SIN101_BUILD_SHARED)
  #define SIN101_API __declspec(dllexport)
 #else
  #define SIN101_API __declspec(dllimport)
 #endif
#else
 #define SIN101_API __attribute__((visibility("default")))
#endif
#ifdef __cplusplus
extern "C" {
#endif
#define SIN101_ABI_VERSION 0x00010000u
#define SIN101_PROVIDER_MOCK 1u
#define SIN101_PROVIDER_SOURCEVR_NATIVE 2u
#define SIN101_MAX_NAME_BYTES 256u
#define SIN101_MISSING 0xffffffffu
typedef struct sin101_context sin101_context;
typedef enum sin101_status {
  SIN101_OK = 0,
  SIN101_ALREADY_PRESENT = 1,
  SIN101_INVALID_ARGUMENT = -1,
  SIN101_BAD_VERSION = -2,
  SIN101_LEGACY_LOAD_FAILED = -3,
  SIN101_PROVIDER_NOT_READY = -4,
  SIN101_NATIVE_ABI_NOT_VERIFIED = -5,
  SIN101_PROVIDER_FAILED = -6,
  SIN101_TOKEN_INVALID = -7,
  SIN101_NAME_MISMATCH = -8,
  SIN101_PROVIDER_CHANGED = -9,
  SIN101_MAPPING_CONFLICT = -10,
  SIN101_NOT_FOUND = -11,
  SIN101_BUFFER_TOO_SMALL = -12,
  SIN101_KEYMAP_REJECTED = -13,
  SIN101_INTERNAL_ERROR = -14
} sin101_status;
// Callback table is C ABI, but registration is TEST DOUBLES ONLY until
// the exact real SourceVR 0.5.5 ARM64 VSoundEmitter method ABI is verified.
// Callbacks must remain valid until reset/destroy and must not destroy ctx.
typedef int32_t (*sin101_name_to_token_fn)(void* user,const char* name,
    uint32_t name_bytes,int64_t* candidate);
typedef int32_t (*sin101_validate_token_fn)(void* user,uint32_t token,
    uint32_t* valid);
typedef int32_t (*sin101_token_to_name_fn)(void* user,uint32_t token,
    char* buffer,uint32_t capacity,uint32_t* required);
typedef struct sin101_provider_v1 {
  uint32_t struct_bytes;
  uint32_t version;
  uint32_t kind;
  uint32_t reserved;
  void* userdata;
  sin101_name_to_token_fn name_to_token;
  sin101_validate_token_fn validate_token;
  sin101_token_to_name_fn token_to_name;
} sin101_provider_v1;
SIN101_API uint32_t sin101_abi_version(void);
SIN101_API sin101_status sin101_create(uint32_t requested_abi,
    const char* phase100_shared_library_path,sin101_context** out);
SIN101_API void sin101_destroy(sin101_context* ctx);
SIN101_API sin101_status sin101_register_provider(sin101_context* ctx,
    const sin101_provider_v1* provider);
SIN101_API sin101_status sin101_map_sound(sin101_context* ctx,int32_t legacy_id,
    int32_t legacy_count,const char* legacy_name,uint32_t name_bytes);
SIN101_API sin101_status sin101_old_to_native(sin101_context* ctx,
    int32_t legacy_id,uint32_t* out_native);
SIN101_API sin101_status sin101_native_to_old(sin101_context* ctx,
    uint32_t native_id,uint32_t* out_legacy);
SIN101_API sin101_status sin101_copy_sound_name(sin101_context* ctx,
    int32_t legacy_id,char* buffer,uint32_t capacity,uint32_t* required);
SIN101_API sin101_status sin101_install_keymap_le(sin101_context* ctx,
    const uint8_t* bytes,uint32_t byte_count);
SIN101_API sin101_status sin101_translate_key(sin101_context* ctx,
    int32_t key,uint32_t* translated);
SIN101_API sin101_status sin101_sound_count(sin101_context* ctx,uint32_t* count);
SIN101_API sin101_status sin101_reset(sin101_context* ctx);
#ifdef __cplusplus
}
#endif
#endif
