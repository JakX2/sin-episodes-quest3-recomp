#pragma once
#include "native_server_facial_think.h"
#include "native_server_npc_random_expression.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_composed_facial_think_export[]="SiN_GetServerComposedFacialThinkApi";
struct ServerComposedFacialThinkBindings {
    ServerFacialThinkBindings base;
    const ServerNpcRandomExpressionBindings* random_expression; // type 16/subtype 6
    bool native_intensity; // use parsed event/scene ramps for all native leaves
};
struct ServerComposedFacialThinkApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerComposedFacialThinkBindings*) noexcept;
};
using GetServerComposedFacialThinkApi=const ServerComposedFacialThinkApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
