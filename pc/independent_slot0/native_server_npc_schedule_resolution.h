#pragma once
#include "native_server_grunt_schedule_selector.h"
#include "native_server_npc_state_reconsider.h"
#include "native_server_jetpack_schedule_selector.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_schedule_resolution_export[]="SiN_GetServerNpcScheduleResolutionApi";
struct ServerNpcScheduleResolutionBindings {
    ServerGruntScheduleSelectorBindings grunt;
    ServerNpcStateBindings state;
    void* context;
    std::uint32_t head; // original manager head, linked descriptor +30
    bool jetpack;
    int (__cdecl *translate)(void*,void*,int,int*) noexcept; // class +43c
    int (__cdecl *component_schedule)(void*,std::uint32_t,int,std::uint32_t*) noexcept;
    int (__cdecl *special_select)(void*,void*,int,int*) noexcept; // Jetpack / state4,7
    int (__cdecl *fail_select)(void*,void*,int,int,int,int*) noexcept; // +438
    int (__cdecl *deactivate)(void*,void*,std::uint32_t) noexcept; // 2216e510(0)
};
struct ServerNpcScheduleResolutionResult {
    int requested,translated,global,ideal;
    std::uint32_t token;
    unsigned visited,fallback,component_hit,component_deactivated,forced,failed;
    ServerGruntScheduleSelectorResult grunt;
    ServerNpcStateResult state;
};
struct ServerNpcScheduleResolutionApi {
    std::uint32_t version,size;
    int (__cdecl *find)(void*,unsigned,int,const ServerNpcScheduleResolutionBindings*,ServerNpcScheduleResolutionResult*) noexcept;
    int (__cdecl *select)(void*,unsigned,float,bool,const ServerNpcScheduleResolutionBindings*,ServerNpcScheduleResolutionResult*) noexcept;
};
using GetServerNpcScheduleResolutionApi=const ServerNpcScheduleResolutionApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_jetpack_schedule_resolution_export[]="SiN_GetServerJetpackScheduleResolutionApi";
struct ServerJetpackScheduleResolutionApi {
    std::uint32_t version,size;
    int (__cdecl *select)(void*,unsigned,float,bool,const ServerNpcScheduleResolutionBindings*,const ServerJetpackSelectorBindings*,ServerNpcScheduleResolutionResult*,ServerJetpackSelectorResult*) noexcept;
};
using GetServerJetpackScheduleResolutionApi=const ServerJetpackScheduleResolutionApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_native_slot_resolution_export[]="SiN_GetServerNpcNativeSlotResolutionApi";
struct ServerNpcNativeSlotResolutionApi {
    std::uint32_t version,size;
    int (__cdecl *select)(void*,unsigned,float,bool,const ServerNpcScheduleResolutionBindings*,const ServerNpcGruntSquadSlotsBindings*,ServerNpcScheduleResolutionResult*,ServerGruntNativeSlotSelectorResult*) noexcept;
};
using GetServerNpcNativeSlotResolutionApi=const ServerNpcNativeSlotResolutionApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
