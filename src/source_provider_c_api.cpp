#include "sin_compat/source_provider_c_api.h"
#include "sin_compat/legacy_bridge_c_api.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <dlfcn.h>
#include <memory>
#include <mutex>
#include <new>
#include <string>
struct sin101_context {
    void* module=nullptr;
    sin100_context* legacy=nullptr;
    uint32_t (*old_version)(void)=nullptr;
    sin100_status (*old_create)(uint32_t,sin100_context**)=nullptr;
    void (*old_destroy)(sin100_context*)=nullptr;
    sin100_status (*old_bind)(sin100_context*,int32_t,int32_t,int64_t,uint32_t,
                             const char*,uint32_t,const char*,uint32_t)=nullptr;
    sin100_status (*old_to_native)(sin100_context*,int32_t,uint32_t*)=nullptr;
    sin100_status (*native_to_old)(sin100_context*,uint32_t,uint32_t*)=nullptr;
    sin100_status (*copy_name)(sin100_context*,int32_t,char*,uint32_t,uint32_t*)=nullptr;
    sin100_status (*install_map)(sin100_context*,const uint8_t*,uint32_t)=nullptr;
    sin100_status (*translate_key)(sin100_context*,int32_t,uint32_t*)=nullptr;
    sin100_status (*count)(sin100_context*,uint32_t*)=nullptr;
    sin100_status (*reset_old)(sin100_context*)=nullptr;
    std::mutex mutex;
    sin101_provider_v1 provider{};
    uint64_t epoch=0;
    bool active=false;
};
namespace {
template<typename Fn> bool get_fn(void* handle,const char* name,Fn& target) noexcept {
    static_assert(sizeof(Fn)==sizeof(void*),"this ELF C ABI requires equal pointer sizes");
    void* entry=dlsym(handle,name);
    if(!entry)return false;
    std::memcpy(&target,&entry,sizeof entry);
    return true;
}
sin101_status adapt(sin100_status old) noexcept {
    switch(old) {
      case SIN100_OK:return SIN101_OK;
      case SIN100_ALREADY_PRESENT:return SIN101_ALREADY_PRESENT;
      case SIN100_INVALID_ARGUMENT:return SIN101_INVALID_ARGUMENT;
      case SIN100_NOT_READY:return SIN101_PROVIDER_NOT_READY;
      case SIN100_HASH_MISMATCH:case SIN100_MAP_INVALID:return SIN101_KEYMAP_REJECTED;
      case SIN100_LEGACY_INDEX_INVALID:case SIN100_NATIVE_INDEX_INVALID:return SIN101_TOKEN_INVALID;
      case SIN100_VALIDATION_REQUIRED:return SIN101_PROVIDER_FAILED;
      case SIN100_NAME_MISMATCH:return SIN101_NAME_MISMATCH;
      case SIN100_MAPPING_CONFLICT:return SIN101_MAPPING_CONFLICT;
      case SIN100_NOT_FOUND:return SIN101_NOT_FOUND;
      case SIN100_BUFFER_TOO_SMALL:return SIN101_BUFFER_TOO_SMALL;
      default:return SIN101_INTERNAL_ERROR;
    }
}
}
extern "C" {
SIN101_API uint32_t sin101_abi_version(void){return SIN101_ABI_VERSION;}
SIN101_API sin101_status sin101_create(uint32_t version,const char* path,sin101_context** out) {
    if(!out)return SIN101_INVALID_ARGUMENT;
    *out=nullptr;
    if(version!=SIN101_ABI_VERSION)return SIN101_BAD_VERSION;
    if(!path || !*path || std::strlen(path)>4096)return SIN101_INVALID_ARGUMENT;
    try {
        std::unique_ptr<sin101_context> p(new (std::nothrow) sin101_context());
        if(!p)return SIN101_INTERNAL_ERROR;
        p->module=dlopen(path,RTLD_NOW|RTLD_LOCAL);
        if(!p->module)return SIN101_LEGACY_LOAD_FAILED;
        bool loaded = get_fn(p->module,"sin100_abi_version",p->old_version) &&
            get_fn(p->module,"sin100_create",p->old_create) &&
            get_fn(p->module,"sin100_destroy",p->old_destroy) &&
            get_fn(p->module,"sin100_bind_sound",p->old_bind) &&
            get_fn(p->module,"sin100_old_to_native",p->old_to_native) &&
            get_fn(p->module,"sin100_native_to_old",p->native_to_old) &&
            get_fn(p->module,"sin100_copy_sound_name",p->copy_name) &&
            get_fn(p->module,"sin100_install_keymap_le",p->install_map) &&
            get_fn(p->module,"sin100_translate_key",p->translate_key) &&
            get_fn(p->module,"sin100_sound_count",p->count) &&
            get_fn(p->module,"sin100_reset",p->reset_old);
        if(!loaded || p->old_version()!=SIN100_ABI_VERSION) {
            dlclose(p->module);return SIN101_LEGACY_LOAD_FAILED;
        }
        if(p->old_create(SIN100_ABI_VERSION,&p->legacy)!=SIN100_OK || !p->legacy) {
            dlclose(p->module);return SIN101_LEGACY_LOAD_FAILED;
        }
        *out=p.release();return SIN101_OK;
    } catch(...) {return SIN101_INTERNAL_ERROR;}
}
SIN101_API void sin101_destroy(sin101_context* ctx) {
    if(!ctx)return;
    if(ctx->legacy)ctx->old_destroy(ctx->legacy);
    if(ctx->module)dlclose(ctx->module);
    delete ctx;
}
SIN101_API sin101_status sin101_register_provider(sin101_context* ctx,const sin101_provider_v1* in) {
    if(!ctx || !in)return SIN101_INVALID_ARGUMENT;
    if(in->struct_bytes!=sizeof(sin101_provider_v1) || in->version!=SIN101_ABI_VERSION ||
       in->reserved || !in->name_to_token || !in->validate_token || !in->token_to_name)
        return SIN101_INVALID_ARGUMENT;
    // NEVER call original SiN 001 or SourceVR 002 vtables before their ABI
    // is verified. Only explicitly configured TEST PROVIDERS are permitted.
    if(in->kind==SIN101_PROVIDER_SOURCEVR_NATIVE)return SIN101_NATIVE_ABI_NOT_VERIFIED;
    if(in->kind!=SIN101_PROVIDER_MOCK)return SIN101_INVALID_ARGUMENT;
    try {
        std::lock_guard<std::mutex> guard(ctx->mutex);
        ctx->provider=*in;ctx->active=true;++ctx->epoch;return SIN101_OK;
    } catch(...) {return SIN101_INTERNAL_ERROR;}
}
SIN101_API sin101_status sin101_map_sound(sin101_context* ctx,int32_t old_id,
      int32_t old_count,const char* name,uint32_t length) {
    if(!ctx || !name || !length || length>SIN101_MAX_NAME_BYTES ||
       std::memchr(name,0,length) || old_id<0 || old_count<=old_id || old_count<0)
        return SIN101_INVALID_ARGUMENT;
    try {
        const std::string owned(name,length);
        sin101_provider_v1 cb{};uint64_t ticket=0;
        {std::lock_guard<std::mutex> guard(ctx->mutex);
         if(!ctx->active)return SIN101_PROVIDER_NOT_READY;
         cb=ctx->provider;ticket=ctx->epoch;}
        // External callbacks are invoked WITHOUT holding the mutex. If a
        // callback resets/replaces the provider, the generation check below
        // discards stale results before inserting anything.
        int64_t token=-1;
        if(cb.name_to_token(cb.userdata,owned.data(),length,&token)!=0)
            return SIN101_PROVIDER_FAILED;
        if(token<0 || token>65534)return SIN101_TOKEN_INVALID;
        uint32_t valid=0;
        if(cb.validate_token(cb.userdata,static_cast<uint32_t>(token),&valid)!=0 || valid!=1)
            return SIN101_PROVIDER_FAILED;
        std::array<char,SIN101_MAX_NAME_BYTES+1> reversed{};
        uint32_t required=0;
        if(cb.token_to_name(cb.userdata,static_cast<uint32_t>(token),reversed.data(),
                            static_cast<uint32_t>(reversed.size()),&required)!=0)
            return SIN101_PROVIDER_FAILED;
        if(required!=length+1 || reversed[length]!='\0' ||
           std::memchr(reversed.data(),0,length) ||
           std::memcmp(reversed.data(),owned.data(),length)!=0)
            return SIN101_NAME_MISMATCH;
        std::lock_guard<std::mutex> guard(ctx->mutex);
        if(!ctx->active || ticket!=ctx->epoch)return SIN101_PROVIDER_CHANGED;
        // "validated" below is true ONLY for the verified MOCK callback
        // roundtrip, never a claim of real SourceVR method compatibility.
        return adapt(ctx->old_bind(ctx->legacy,old_id,old_count,token,1,
                 owned.data(),length,reversed.data(),length));
    } catch(...) {return SIN101_INTERNAL_ERROR;}
}
SIN101_API sin101_status sin101_old_to_native(sin101_context* ctx,int32_t id,uint32_t* out) {
    if(!ctx || !out)return SIN101_INVALID_ARGUMENT;
    try {std::lock_guard<std::mutex> guard(ctx->mutex);
      return adapt(ctx->old_to_native(ctx->legacy,id,out));}
    catch(...) {return SIN101_INTERNAL_ERROR;}
}
SIN101_API sin101_status sin101_native_to_old(sin101_context* ctx,uint32_t id,uint32_t* out) {
    if(!ctx || !out)return SIN101_INVALID_ARGUMENT;
    try {std::lock_guard<std::mutex> guard(ctx->mutex);
      return adapt(ctx->native_to_old(ctx->legacy,id,out));}
    catch(...) {return SIN101_INTERNAL_ERROR;}
}
SIN101_API sin101_status sin101_copy_sound_name(sin101_context* ctx,int32_t id,
    char* out,uint32_t capacity,uint32_t* needed) {
    if(!ctx || !needed)return SIN101_INVALID_ARGUMENT;
    try {std::lock_guard<std::mutex> guard(ctx->mutex);
      return adapt(ctx->copy_name(ctx->legacy,id,out,capacity,needed));}
    catch(...) {return SIN101_INTERNAL_ERROR;}
}
SIN101_API sin101_status sin101_install_keymap_le(sin101_context* ctx,const uint8_t* input,uint32_t bytes) {
    if(!ctx)return SIN101_INVALID_ARGUMENT;
    try {std::lock_guard<std::mutex> guard(ctx->mutex);
      return adapt(ctx->install_map(ctx->legacy,input,bytes));}
    catch(...) {return SIN101_INTERNAL_ERROR;}
}
SIN101_API sin101_status sin101_translate_key(sin101_context* ctx,int32_t key,uint32_t* value) {
    if(!ctx || !value)return SIN101_INVALID_ARGUMENT;
    try {std::lock_guard<std::mutex> guard(ctx->mutex);
      return adapt(ctx->translate_key(ctx->legacy,key,value));}
    catch(...) {return SIN101_INTERNAL_ERROR;}
}
SIN101_API sin101_status sin101_sound_count(sin101_context* ctx,uint32_t* count) {
    if(!ctx || !count)return SIN101_INVALID_ARGUMENT;
    try {std::lock_guard<std::mutex> guard(ctx->mutex);
      return adapt(ctx->count(ctx->legacy,count));}
    catch(...) {return SIN101_INTERNAL_ERROR;}
}
SIN101_API sin101_status sin101_reset(sin101_context* ctx) {
    if(!ctx)return SIN101_INVALID_ARGUMENT;
    try {std::lock_guard<std::mutex> guard(ctx->mutex);
      ctx->active=false;ctx->provider={};++ctx->epoch;
      return adapt(ctx->reset_old(ctx->legacy));}
    catch(...) {return SIN101_INTERNAL_ERROR;}
}
} // extern "C"
