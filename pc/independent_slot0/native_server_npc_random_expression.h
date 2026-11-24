#pragma once
#include "native_server_expression.h"
#include "native_server_event_intensity.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_random_expression_export[]="SiN_GetServerNpcRandomExpressionApi";
inline constexpr unsigned server_npc_random_expression_extent=0xffc;
struct ServerNpcRandomExpressionBindings {
    void* context;
    const ServerFlexRange* ranges;unsigned count;
    ServerFlexWeightBindings dirty;
    int (__cdecl *load)(void*,void*,const char*,ServerExpressionBank*) noexcept;
    int (__cdecl *controller)(void*,void*,const char*) noexcept;
    int (__cdecl *random_int)(void*,int,int,int*) noexcept;
    int (__cdecl *random_float)(void*,float,float,float*) noexcept;
    int (__cdecl *intensity)(void*,void*,float,float*) noexcept;
    ServerIntensityStorage storage; // optional native intensity path
};
struct ServerNpcRandomExpressionApi {
    std::uint32_t version,size;
    // Original 220283e0, dispatched by 2202a070 for type 16/subtype 6.
    // Transport success distinct from original AL (accepted).
    int (__cdecl *apply)(void*,unsigned,void*,unsigned,void*,unsigned,float,float,bool,
        const ServerNpcRandomExpressionBindings*,unsigned*) noexcept;
};
using GetServerNpcRandomExpressionApi=const ServerNpcRandomExpressionApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
