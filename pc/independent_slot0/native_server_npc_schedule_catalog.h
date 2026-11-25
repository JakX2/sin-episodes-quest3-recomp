#pragma once
#include "native_server_npc_schedule_maintenance.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_schedule_catalog_export[]="SiN_GetServerNpcScheduleCatalogApi";
enum ServerScheduleArgumentKind : unsigned {ScheduleArgumentScalar,ScheduleArgumentSchedule,ScheduleArgumentActivity,ScheduleArgumentPrivateActivity,ScheduleArgumentMemory,ScheduleArgumentGoal,ScheduleArgumentDamageToken};
struct ServerScheduleCatalogTask {int local_task;unsigned argument_kind;float argument;};
struct ServerScheduleCatalogEntry {
    unsigned family;int local_schedule;const char* name;
    std::uint32_t source_string,vector_insert_call;
    const ServerScheduleCatalogTask* tasks;unsigned task_count;
    const int* conditions;unsigned condition_count;
};
struct ServerNpcScheduleCatalogApi {
    std::uint32_t version,size;
    int (__cdecl *all)(const ServerScheduleCatalogEntry**,unsigned*) noexcept;
    // Search class first, then ground base. Distinct subclass local IDs.
    int (__cdecl *find)(unsigned,int,const ServerScheduleCatalogEntry**) noexcept;
};
using GetServerNpcScheduleCatalogApi=const ServerNpcScheduleCatalogApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
