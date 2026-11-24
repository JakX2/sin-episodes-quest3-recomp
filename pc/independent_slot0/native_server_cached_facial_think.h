#pragma once
#include "native_server_named_facial_think.h"
#include "native_server_vfe_cache.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_cached_facial_think_export[]="SiN_GetServerCachedFacialThinkApi";
struct ServerCachedFacialThinkBindings {
    ServerNamedFacialThinkBindings named;
    ServerVfeCache* banks;
    const ServerVfeCacheBindings* files;
};
struct ServerCachedFacialThinkApi {
    std::uint32_t version,size;
    // Native filename cache, eager per-actor bank remap, raw scene intensity
    // and track spline. Caller owns cache lifetimes and filesystem services.
    int (__cdecl *run)(void*,unsigned,float,const ServerCachedFacialThinkBindings*) noexcept;
};
using GetServerCachedFacialThinkApi=const ServerCachedFacialThinkApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
