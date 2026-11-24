#pragma once
#include "native_server_flex_weight.h"
#include "native_client_vfe.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_expression_export[]="SiN_GetServerExpressionApi";
struct ServerExpressionBank {unsigned char* data;unsigned size;};
struct ServerExpressionBindings {
    void* context;
    const ServerFlexRange* ranges;unsigned count;
    ServerFlexWeightBindings dirty;
    // Original 220fb9b0: load or reuse a mutable VFE cache entry.
    // 1 found, 0 absent, -1 transport failure.
    int (__cdecl *load)(void*,void*,const char*,ServerExpressionBank*) noexcept;
    // 220fb700 per-actor controller map; -1 missing, -2 transport failure.
    int (__cdecl *controller)(void*,void*,const char*) noexcept;
    int (__cdecl *intensity)(void*,void*,float,float*) noexcept;
    // Inclusive engine integer RNG range [0,max], only on first alias use.
    int (__cdecl *random)(void*,int,int*) noexcept;
    const char* character_stem; // resolved studio stem, null disables override
};
struct ServerExpressionApi {
    std::uint32_t version,size;
    // 220fbb40 / 220fb7d0 ordinary CBaseFlex EXPRESSION event.
    // The caller's actor think marks +c on success, as 220f8450 does.
    int (__cdecl *apply)(void*,unsigned,void*,unsigned,void*,unsigned,float,const ServerExpressionBindings*) noexcept;
};
using GetServerExpressionApi=const ServerExpressionApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
