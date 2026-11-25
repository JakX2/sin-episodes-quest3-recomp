#pragma once
#include "native_server_npc_condition.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_schedule_state_export[]="SiN_GetServerNpcScheduleStateApi";
struct ServerNpcScheduleBindings {
    ServerNpcConditionMapBindings conditions;
    void* context;
    // Resolved PE32 schedule or task-array storage. No parser required.
    int (__cdecl *storage)(void*,std::uint32_t,const void**,unsigned*) noexcept;
    // Original 22077e60, invoked by schedule install before mask reset.
    int (__cdecl *reset_movement)(void*,void* actor,std::uint32_t component) noexcept;
};
struct ServerNpcScheduleTask {int id;float parameter;};
struct ServerNpcScheduleStateResult {
    std::uint32_t schedule;
    int global,index,count,status;
    unsigned complete,has_task,preserved_condition;
    ServerNpcScheduleTask task;
};
struct ServerNpcScheduleStateApi {
    std::uint32_t version,size;
    // 2204f2d0, does not clear current or snapshot condition banks.
    int (__cdecl *clear)(void*,unsigned) noexcept;
    // 2204f330, installs an already resolved schedule token.
    int (__cdecl *install)(void*,unsigned,std::uint32_t,float,const ServerNpcScheduleBindings*,ServerNpcScheduleStateResult*) noexcept;
    // 2204e160 / 2204e5b0.
    int (__cdecl *inspect)(void*,unsigned,const ServerNpcScheduleBindings*,ServerNpcScheduleStateResult*) noexcept;
    // 2204e180, increments cursor and resets status/timing; final task sets condition 0x22.
    int (__cdecl *advance)(void*,unsigned,const ServerNpcScheduleBindings*,ServerNpcScheduleStateResult*) noexcept;
    // 22039a80 / 22039b10: declared schedule interrupt with optional current-condition gate.
    int (__cdecl *interrupt)(void*,unsigned,int,bool,const ServerNpcScheduleBindings*,unsigned*) noexcept;
};
using GetServerNpcScheduleStateApi=const ServerNpcScheduleStateApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
