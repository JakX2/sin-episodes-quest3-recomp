#pragma once
#include "native_server_expression.h"
#include "native_server_flex_animation.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_facial_think_export[]="SiN_GetServerFacialThinkApi";
struct ServerFacialThinkBindings {
    void* context;
    const ServerFlexRange* ranges;unsigned count;
    ServerFlexWeightBindings dirty;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    const ServerExpressionBindings* expression;
    const ServerFlexAnimationBindings* flex;
    // Opt-in to the base class +2ec dispatch for these recovered event types.
    // Derived NPC dispatch can instead be supplied through other.
    unsigned base_expression,base_flex,base_speech;
    int (__cdecl *other)(void*,void*,void*,void*,void*,unsigned*) noexcept;
};
struct ServerFacialThinkApi {
    std::uint32_t version,size;
    // 220f8450. Decay controllers, dispatch live actor event records in order,
    // set record +0c on accepted dispatch, clear actor +600 at end.
    int (__cdecl *run)(void*,unsigned,float,const ServerFacialThinkBindings*) noexcept;
};
using GetServerFacialThinkApi=const ServerFacialThinkApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
