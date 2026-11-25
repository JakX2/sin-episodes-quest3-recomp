#pragma once
#include "native_server_npc_schedule_state.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_behavior_selection_export[]="SiN_GetServerNpcBehaviorSelectionApi";
struct ServerNpcBehaviorBindings {
    void* context;
    int (__cdecl *storage)(void*,std::uint32_t,const void**,unsigned*) noexcept;
    int (__cdecl *eligible)(void*,std::uint32_t,bool*) noexcept; // component+18
    int (__cdecl *accept)(void*,void*,std::uint32_t,bool*) noexcept; // actor+77c
    int (__cdecl *enter)(void*,std::uint32_t) noexcept; // component+1c
    int (__cdecl *activate)(void*,std::uint32_t) noexcept; // component+24, guard set by host
    int (__cdecl *leave)(void*,std::uint32_t) noexcept; // component+20
    int (__cdecl *reset)(void*,void*) noexcept; // 22054ca0
    int (__cdecl *changed)(void*,void*,std::uint32_t old_token,std::uint32_t new_token) noexcept; // actor+778
};
struct ServerNpcBehaviorResult {std::uint32_t prior,selected;unsigned visited,eligible,accepted,changed;};
struct ServerNpcBehaviorSelectionApi {
    std::uint32_t version,size;
    // 2234e590, 2216e510: first eligible and accepted component wins.
    int (__cdecl *select)(void*,unsigned,const ServerNpcBehaviorBindings*,ServerNpcBehaviorResult*) noexcept;
    // 2205eec0: schedule local/global mapping (+4 root) and compare.
    int (__cdecl *schedule_is)(void*,unsigned,int,bool,const ServerNpcScheduleBindings*,unsigned*) noexcept;
};
using GetServerNpcBehaviorSelectionApi=const ServerNpcBehaviorSelectionApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_schedule_namespace_export[]="SiN_GetServerNpcScheduleNamespaceApi";
struct ServerNpcScheduleNamespaceApi {
    std::uint32_t version,size;
    int (__cdecl *to_global)(void*,int,const ServerNpcConditionMapBindings*,int*) noexcept;
    int (__cdecl *to_local)(void*,int,const ServerNpcConditionMapBindings*,int*) noexcept;
};
using GetServerNpcScheduleNamespaceApi=const ServerNpcScheduleNamespaceApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
