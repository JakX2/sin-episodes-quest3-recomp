#pragma once
#include "native_server_recombinant_minor_task.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_launch_export[]="SiN_GetServerRecombinantLaunchApi";
// Owner supplies the original engine trace result, not a second collision system.
struct ServerRecombinantTraceRequest {
    float start[3],end[3];
    unsigned mask,collision_token;
    bool upward;
};
struct ServerRecombinantLaunchBindings {
    const ServerRecombinantMinorTaskBindings* task;
    void* context;
    int (__cdecl *refresh_origin)(void*,void*) noexcept;
    int (__cdecl *trace)(void*,void*,const ServerRecombinantTraceRequest*,float*) noexcept;
    int (__cdecl *ground_object_effect)(void*,void*,void*,int,float) noexcept; // 22228160
    int (__cdecl *clear_ground)(void*,void*) noexcept; // 22221890(0)
    int (__cdecl *normalize)(void*,float[3]) noexcept; // original imported vector helper
    int (__cdecl *relocate)(void*,void*,const float[3],int) noexcept; // 22285ab0
    int (__cdecl *parent_failure)(void*,void*,int,int,int,int*) noexcept;
};
struct ServerRecombinantLaunchResult {
    unsigned completed,ground_live,object_checks,object_effect,random_calls,normalized,ground_cleared,
        origin_refreshes,trace_calls,clearance,relocated,velocity_set,condition_tests,delegated,handle_cleared;
    int selected;
    float fraction,direction[3],velocity[3],relocation[3];
    ServerRecombinantTraceRequest trace;
    ServerNpcTaskLifecycleResult lifecycle;
};
struct ServerRecombinantLaunchApi {
    unsigned version,size;
    int (__cdecl *start140)(void*,unsigned,float,const ServerRecombinantLaunchBindings*,ServerRecombinantLaunchResult*) noexcept;
    int (__cdecl *fail_select)(void*,unsigned,int,int,int,const ServerRecombinantLaunchBindings*,ServerRecombinantLaunchResult*) noexcept;
};
using GetServerRecombinantLaunchApi=const ServerRecombinantLaunchApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
