#pragma once
#include "native_server_expression.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_vfe_cache_export[]="SiN_GetServerVfeCacheApi";
struct ServerVfeCacheEntry {
    char name[128];
    ServerExpressionBank bank;
};
struct ServerVfeCache {
    ServerVfeCacheEntry* entries;
    unsigned count,capacity;
};
struct ServerVfeCacheBindings {
    void* context;
    // Open and read immutable file view, 1 found, 0 missing, -1 error.
    int (__cdecl *read)(void*,const char*,const unsigned char**,unsigned*) noexcept;
    int (__cdecl *close)(void*,const unsigned char*) noexcept;
    int (__cdecl *allocate)(void*,unsigned,void**) noexcept;
    int (__cdecl *release)(void*,void*) noexcept;
};
struct ServerVfeCacheApi {
    std::uint32_t version,size;
    // Original 220fb9b0 case-insensitive successful-bank cache.
    // 1 found, 0 absent, -1 invalid/error. Output unchanged on error.
    int (__cdecl *load)(ServerVfeCache*,const ServerVfeCacheBindings*,const char*,ServerExpressionBank*) noexcept;
    // Explicit lifetime boundary; invalidate all actor remaps first.
    int (__cdecl *clear)(ServerVfeCache*,const ServerVfeCacheBindings*) noexcept;
};
using GetServerVfeCacheApi=const ServerVfeCacheApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
