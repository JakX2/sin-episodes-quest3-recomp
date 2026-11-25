#pragma once
#include "native_server_npc_state_reconsider.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_special_schedule_export[]="SiN_GetServerNpcSpecialScheduleApi";
struct ServerNpcSpecialScheduleBindings {
    ServerNpcStateBindings state;
    void* context;
    int (__cdecl *clear_parent)(void*,void*) noexcept; // 22221890(null)
    int (__cdecl *reset_script)(void*,void*) noexcept; // 22048fe0
    int (__cdecl *death_query)(void*,void*,bool*) noexcept; // +2a0 with resolved global vector
    int (__cdecl *death_prepare)(void*,void*,int,int) noexcept; // +3f4(0,1)
};
struct ServerNpcSpecialScheduleResult {
    int schedule,state;
    unsigned override_condition,parent_cleared,controller_live,script_reset,death_query,death_prepared;
};
struct ServerNpcSpecialScheduleApi {
    std::uint32_t version,size;
    // The original 2204fe80 only calls this selector in states 4 and 7.
    int (__cdecl *select)(void*,unsigned,const ServerNpcSpecialScheduleBindings*,ServerNpcSpecialScheduleResult*) noexcept;
};
using GetServerNpcSpecialScheduleApi=const ServerNpcSpecialScheduleApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
