#include "sin_compat/legacy_bridge_c_api.h"
#include "sin_compat/legacy_sound_vgui_prebridge.hpp"
#include "sin_compat/phase100_sha256.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>
#include <new>
#include <string_view>

struct sin100_context {
    std::mutex lock;
    sin_compat::phase99::LegacyKeyTranslator keyboard;
    sin_compat::phase99::VerifiedSoundTokenCrosswalk sound;
};
namespace {
using namespace sin_compat;
using sin_compat::phase99::CrosswalkResult;
sin100_status translate_binding_result(CrosswalkResult v) noexcept {
    switch(v) {
      case CrosswalkResult::Added:return SIN100_OK;
      case CrosswalkResult::AlreadyPresent:return SIN100_ALREADY_PRESENT;
      case CrosswalkResult::LegacyIndexInvalid:return SIN100_LEGACY_INDEX_INVALID;
      case CrosswalkResult::NativeIndexInvalid:return SIN100_NATIVE_INDEX_INVALID;
      case CrosswalkResult::NativeValidationFailed:return SIN100_VALIDATION_REQUIRED;
      case CrosswalkResult::NameMismatch:return SIN100_NAME_MISMATCH;
      case CrosswalkResult::MappingConflict:return SIN100_MAPPING_CONFLICT;
    }
    return SIN100_INTERNAL_ERROR;
}
constexpr std::uint32_t kMaxNameBytes=256;
} // namespace
extern "C" {
SIN100_API std::uint32_t sin100_abi_version(void) {return SIN100_ABI_VERSION;}
SIN100_API sin100_status sin100_create(std::uint32_t requested_abi,sin100_context** out) {
    if(!out)return SIN100_INVALID_ARGUMENT;
    *out=nullptr;
    if(requested_abi!=SIN100_ABI_VERSION)return SIN100_BAD_VERSION;
    try {
      *out=new (std::nothrow) sin100_context();
      return *out?SIN100_OK:SIN100_INTERNAL_ERROR;
    } catch(...) {return SIN100_INTERNAL_ERROR;}
}
SIN100_API void sin100_destroy(sin100_context* ctx) {delete ctx;}
SIN100_API sin100_status sin100_install_keymap_le(sin100_context* ctx,const std::uint8_t* bytes,std::uint32_t count) {
    if(!ctx)return SIN100_INVALID_ARGUMENT;
    try {
      std::lock_guard<std::mutex> guard(ctx->lock);
      ctx->keyboard.reset();
      if(!bytes || count!=SIN100_KEYMAP_BYTE_COUNT)return SIN100_INVALID_ARGUMENT;
      if(phase100::sha256(bytes,count)!=phase100::original_keymap_sha)return SIN100_HASH_MISMATCH;
      std::array<std::uint32_t,256> map{};
      for(std::size_t i=0;i<map.size();++i) {
        const auto* b=bytes+i*4;
        map[i]=std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|
               (std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);
      }
      return ctx->keyboard.install(map,true)?SIN100_OK:SIN100_MAP_INVALID;
    } catch(...) {return SIN100_INTERNAL_ERROR;}
}
SIN100_API sin100_status sin100_translate_key(sin100_context* ctx,std::int32_t legacy_key,std::uint32_t* out) {
    if(!ctx || !out)return SIN100_INVALID_ARGUMENT;
    *out=0;
    try {
      std::lock_guard<std::mutex> guard(ctx->lock);
      if(!ctx->keyboard.ready())return SIN100_NOT_READY;
      *out=ctx->keyboard.translate(legacy_key);
      return SIN100_OK;
    } catch(...) {return SIN100_INTERNAL_ERROR;}
}
SIN100_API sin100_status sin100_bind_sound(sin100_context* ctx,std::int32_t legacy_id,std::int32_t legacy_count,
  std::int64_t native_candidate,std::uint32_t native_validated,const char* legacy_name,
  std::uint32_t legacy_name_len,const char* native_name,std::uint32_t native_name_len) {
    if(!ctx || (native_validated!=0 && native_validated!=1) ||
       (legacy_name_len && !legacy_name) || (native_name_len && !native_name) ||
       legacy_name_len>kMaxNameBytes || native_name_len>kMaxNameBytes)
       return SIN100_INVALID_ARGUMENT;
    if((legacy_name_len && std::memchr(legacy_name,0,legacy_name_len)) ||
       (native_name_len && std::memchr(native_name,0,native_name_len)))
       return SIN100_INVALID_ARGUMENT;
    try {
      std::lock_guard<std::mutex> guard(ctx->lock);
      const std::string_view old(legacy_name?legacy_name:"",legacy_name_len);
      const std::string_view native(native_name?native_name:"",native_name_len);
      return translate_binding_result(ctx->sound.bind(legacy_id,legacy_count,native_candidate,
          native_validated==1,old,native));
    } catch(...) {return SIN100_INTERNAL_ERROR;}
}
SIN100_API sin100_status sin100_old_to_native(sin100_context* ctx,std::int32_t legacy_id,std::uint32_t* out) {
    if(!ctx || !out)return SIN100_INVALID_ARGUMENT;
    *out=SIN100_NOT_FOUND_TOKEN;
    try {
      std::lock_guard<std::mutex> guard(ctx->lock);
      *out=ctx->sound.native_from_old_token(legacy_id);
      return *out==SIN100_NOT_FOUND_TOKEN?SIN100_NOT_FOUND:SIN100_OK;
    } catch(...) {return SIN100_INTERNAL_ERROR;}
}
SIN100_API sin100_status sin100_native_to_old(sin100_context* ctx,std::uint32_t native_id,std::uint32_t* out) {
    if(!ctx || !out)return SIN100_INVALID_ARGUMENT;
    *out=SIN100_NOT_FOUND_TOKEN;
    if(native_id>phase99::max_native_token)return SIN100_NATIVE_INDEX_INVALID;
    try {
      std::lock_guard<std::mutex> guard(ctx->lock);
      *out=ctx->sound.old_from_native_token(static_cast<std::uint16_t>(native_id));
      return *out==SIN100_NOT_FOUND_TOKEN?SIN100_NOT_FOUND:SIN100_OK;
    } catch(...) {return SIN100_INTERNAL_ERROR;}
}
SIN100_API sin100_status sin100_copy_sound_name(sin100_context* ctx,std::int32_t id,
  char* destination,std::uint32_t capacity,std::uint32_t* required) {
    if(!ctx || !required || (capacity && !destination))return SIN100_INVALID_ARGUMENT;
    *required=0;
    try {
      std::lock_guard<std::mutex> guard(ctx->lock);
      const char* text=ctx->sound.name_from_old_token(id);
      if(!*text)return SIN100_NOT_FOUND;
      const auto bytes=std::strlen(text)+1;
      if(bytes>std::numeric_limits<std::uint32_t>::max())return SIN100_INTERNAL_ERROR;
      *required=static_cast<std::uint32_t>(bytes);
      if(!destination || capacity<bytes)return SIN100_BUFFER_TOO_SMALL;
      std::memcpy(destination,text,bytes);
      return SIN100_OK;
    } catch(...) {return SIN100_INTERNAL_ERROR;}
}
SIN100_API sin100_status sin100_sound_count(sin100_context* ctx,std::uint32_t* count) {
    if(!ctx || !count)return SIN100_INVALID_ARGUMENT;
    *count=0;
    try {
      std::lock_guard<std::mutex> guard(ctx->lock);
      auto size=ctx->sound.size();
      if(size>std::numeric_limits<std::uint32_t>::max())return SIN100_INTERNAL_ERROR;
      *count=static_cast<std::uint32_t>(size);
      return SIN100_OK;
    } catch(...) {return SIN100_INTERNAL_ERROR;}
}
SIN100_API sin100_status sin100_reset(sin100_context* ctx) {
    if(!ctx)return SIN100_INVALID_ARGUMENT;
    try {
      std::lock_guard<std::mutex> guard(ctx->lock);
      ctx->keyboard.reset();ctx->sound.reset();
      return SIN100_OK;
    } catch(...) {return SIN100_INTERNAL_ERROR;}
}
} // extern "C"
