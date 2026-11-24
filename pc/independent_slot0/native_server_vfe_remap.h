#pragma once
#include "native_server_expression.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_vfe_remap_export[]="SiN_GetServerVfeRemapApi";
// One caller-owned cache per actor, reset before controller layout changes
// or bank destruction/reuse. Mirrors original bank-pointer identity policy,
// not the original red-black tree's internal allocation ABI.
struct ServerVfeRemapEntry {
    const unsigned char* bank;
    unsigned keys;
    int local[128];
};
struct ServerVfeRemapCache {
    ServerVfeRemapEntry* entries;
    unsigned count,capacity;
};
struct ServerVfeRemapApi {
    std::uint32_t version,size;
    // 220f5840, first case-insensitive match, zero on miss.
    int (__cdecl *find)(const char* const*,unsigned,const char*,int*) noexcept;
    // 220fb5b0/220fb700, lazily map every VFE key then return cached key.
    int (__cdecl *lookup)(ServerVfeRemapCache*,ServerExpressionBank,const char* const*,unsigned,unsigned,int*) noexcept;
    int (__cdecl *clear)(ServerVfeRemapCache*) noexcept;
};
using GetServerVfeRemapApi=const ServerVfeRemapApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
