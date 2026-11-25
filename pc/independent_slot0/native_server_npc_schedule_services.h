#pragma once
#include "native_server_npc_inherited_schedule.h"
#include "native_server_npc_special_schedule.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_schedule_services_export[]="SiN_GetServerNpcScheduleServicesApi";
struct ServerNpcScheduleServicesBindings {
    ServerNpcSubclassSchedulerBindings scheduler;
    ServerNpcClassScheduleTranslationBindings translation;
    ServerNpcInheritedScheduleBindings inherited;
    ServerNpcSpecialScheduleBindings special;
};
struct ServerNpcScheduleServicesResult {
    ServerNpcSubclassSchedulerResult scheduler;
    ServerNpcClassScheduleServicesResult translation;
    ServerNpcSpecialScheduleResult special;
    unsigned translations,special_selections,inherited_calls,burst_calls,interval_calls;
};
struct ServerNpcScheduleServicesApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerNpcScheduleServicesBindings*,ServerNpcScheduleServicesResult*) noexcept;
};
using GetServerNpcScheduleServicesApi=const ServerNpcScheduleServicesApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
