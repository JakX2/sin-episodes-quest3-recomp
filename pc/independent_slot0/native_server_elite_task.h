#pragma once
#include "native_server_grunt_task_dispatch.h"
#include "native_server_npc_reload_services.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_elite_task_export[]="SiN_GetServerEliteTaskApi";
struct ServerEliteTaskBindings {
    ServerGruntTaskBindings grunt;
    ServerNpcReloadServicesBindings reload;
    void* context;
    int (__cdecl *is_player)(void*,void*,bool*) noexcept;
    // Original +4dc request1; native composed runtime can supply this leaf.
    int (__cdecl *idle)(void*,void*) noexcept;
    int (__cdecl *stop_sound)(void*,void*,int) noexcept;
};
struct ServerEliteTaskResult {
    unsigned delegated,spin_ready,weapon_live,timer_written,cue_called,idle_called,spin_changed;
    ServerGruntTaskResult grunt;
    ServerNpcTaskLifecycleResult lifecycle;
    ServerNpcParentWeaponEventResult reload;
    ServerWeaponEventSoundResult sound;
};
struct ServerEliteTaskApi {
    std::uint32_t version,size;
    int (__cdecl *execute)(void*,unsigned,const ServerNpcScheduleTask*,bool,float,const ServerEliteTaskBindings*,ServerEliteTaskResult*) noexcept;
    // 22315390: sound/state update; not the physical weapon simulation.
    int (__cdecl *spin_update)(void*,unsigned,float,const ServerEliteTaskBindings*,ServerEliteTaskResult*) noexcept;
};
using GetServerEliteTaskApi=const ServerEliteTaskApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
