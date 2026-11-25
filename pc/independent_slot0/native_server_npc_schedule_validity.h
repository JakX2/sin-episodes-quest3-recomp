#pragma once
#include "native_server_npc_behavior_selection.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_schedule_validity_export[]="SiN_GetServerNpcScheduleValidityApi";
enum ServerNpcScheduleValidityReason : unsigned {
    ScheduleValid,ScheduleMissing,ScheduleEmpty,ScheduleInterrupted,ScheduleCompleted,ScheduleFailed
};
struct ServerNpcScheduleValidityBindings {
    ServerNpcScheduleBindings schedule;
    void* context;
    bool jetpack,record_interrupt;
    std::uint32_t unknown_name;
    // 2202d790 inherited interrupt builder, invoked before Grunt changes.
    int (__cdecl *base_build)(void*,void*) noexcept;
    int (__cdecl *mobility)(void*,float,bool*) noexcept;
    int (__cdecl *name_token)(void*,void*,int,std::uint32_t*) noexcept;
};
struct ServerNpcScheduleValidityResult {
    unsigned reason,valid,builder_called,burning,jetpack_clear,matched,recorded;
    int first_bit,first_global;
};
struct ServerNpcScheduleValidityApi {
    std::uint32_t version,size;
    // 22052f50, with ground or jetpack +490.
    int (__cdecl *validate)(void*,unsigned,float,const ServerNpcScheduleValidityBindings*,ServerNpcScheduleValidityResult*) noexcept;
};
using GetServerNpcScheduleValidityApi=const ServerNpcScheduleValidityApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
