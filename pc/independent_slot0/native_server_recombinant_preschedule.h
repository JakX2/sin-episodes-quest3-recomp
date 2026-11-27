#pragma once
#include "native_server_recombinant_damage.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_preschedule_export[]="SiN_GetServerRecombinantPrescheduleApi";
struct ServerRecombinantPrescheduleBindings {
    const ServerRecombinantPolicyBindings* policy;
    void* context;
    int (__cdecl *refresh_velocity)(void*,void*) noexcept; // 2213d750
    int (__cdecl *enemy)(void*,void*,void**) noexcept; // +154
    int (__cdecl *world_center)(void*,void*,float[3]) noexcept; // +214
    // Original 22379470 is still a separate owner boundary. Returns
    // the selected serial handle (or invalid). Not a generic physics port.
    int (__cdecl *find_object)(void*,void*,int,unsigned*) noexcept;
    bool debug;
    int (__cdecl *debug_box)(void*,const float[3],const float[3],const float[3],int,int,int,int,float) noexcept;
};
struct ServerRecombinantPrescheduleResult {
    unsigned completed,velocity_refreshed,landing_latched,schedule_tests,search_called,search_live,
        enemy_live,goal_live,goal_cleared,condition_writes,debug_called,debug_invalid_goal;
    unsigned prior_handle,final_handle;
    double displacement;
    ServerRecombinantDamageExpiryResult expiry;
};
struct ServerRecombinantPrescheduleApi {
    unsigned version,size;
    int (__cdecl *run_major)(void*,unsigned,float,const ServerRecombinantPrescheduleBindings*,ServerRecombinantPrescheduleResult*) noexcept;
    int (__cdecl *monitor_major)(void*,unsigned,const ServerRecombinantPrescheduleBindings*,ServerRecombinantPrescheduleResult*) noexcept;
};
using GetServerRecombinantPrescheduleApi=const ServerRecombinantPrescheduleApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
