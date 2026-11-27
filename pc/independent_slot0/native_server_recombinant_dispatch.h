#pragma once
#include "native_server_recombinant_launch.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_dispatch_export[]="SiN_GetServerRecombinantDispatchApi";
enum RecombinantDispatchRoute:unsigned {RecombinantDispatchInherited=0,RecombinantDispatchMajor=1,RecombinantDispatchMinor=2,RecombinantDispatchLaunch=3};
struct ServerRecombinantDispatchBindings {
    const ServerRecombinantTaskBindings* task;
    const ServerRecombinantMinorTaskBindings* minor;
    const ServerRecombinantLaunchBindings* launch;
};
struct ServerRecombinantDispatchResult {
    unsigned completed,kind,route,delegated,failed,task_complete;
    int task;
    ServerRecombinantTaskResult major;
    ServerRecombinantMinorTaskResult minor;
    ServerRecombinantLaunchResult launch;
};
struct ServerRecombinantDispatchApi {
    unsigned version,size;
    int (__cdecl *route)(unsigned,int,bool,unsigned*) noexcept;
    int (__cdecl *execute)(void*,unsigned,const ServerNpcScheduleTask*,bool,float,const ServerRecombinantDispatchBindings*,ServerRecombinantDispatchResult*) noexcept;
};
using GetServerRecombinantDispatchApi=const ServerRecombinantDispatchApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
