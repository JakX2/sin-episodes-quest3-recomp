#include "native_server_recombinant_dispatch.h"
#include <cmath>
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerRecombinantTaskApi* __cdecl SiN_GetServerRecombinantTaskApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerRecombinantMinorTaskApi* __cdecl SiN_GetServerRecombinantMinorTaskApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerRecombinantLaunchApi* __cdecl SiN_GetServerRecombinantLaunchApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
int __cdecl route(unsigned vt,int id,bool running,unsigned* out) noexcept{
    if(!out)return 0;*out=RecombinantDispatchInherited;
    if(vt==recombinant_major_vtable){
        const auto* api=SiN_GetServerRecombinantTaskApi(1,sizeof(ServerRecombinantTaskApi));
        if(!api)return 0;if(api->supports_major(id,running))*out=RecombinantDispatchMajor;return 1;
    }
    if(vt==recombinant_minor_vtable){
        const auto* api=SiN_GetServerRecombinantMinorTaskApi(1,sizeof(ServerRecombinantMinorTaskApi));
        if(!api)return 0;
        switch(api->route(id,running)){
        case MinorTaskInherited:return 1;
        case MinorTaskNative:*out=RecombinantDispatchMinor;return 1;
        case MinorTaskUnrecovered:
            if(!running&&id==140){*out=RecombinantDispatchLaunch;return 1;}
        }
    }
    return 0;
}
int __cdecl execute(void* a,unsigned n,const ServerNpcScheduleTask* t,bool running,float now,const ServerRecombinantDispatchBindings* b,ServerRecombinantDispatchResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!a||n<4||!t||!std::isfinite(now)||!b||!b->task||!b->task->decision||!b->task->decision->policy)return 0;
    unsigned vt=0;std::memcpy(&vt,a,4);o->task=t->id;
    if(!route(vt,t->id,running,&o->route))return 0;
    o->kind=vt==recombinant_major_vtable?RecombinantMajor:RecombinantMinor;
    // Reject contradictory services instead of switching actor identity or
    // using different condition/registry owners on the same invocation.
    if(b->minor&&b->minor->task!=b->task)return 0;
    if(b->launch&&b->launch->task!=b->minor)return 0;
    if(b->minor&&b->minor->touch&&b->minor->touch->minor&&b->minor->touch->minor->damage&&
       b->minor->touch->minor->damage->policy!=b->task->decision->policy)return 0;
    if(n<(o->kind==RecombinantMajor?0xc54u:0xc48u))return 0;
    switch(o->route){
    case RecombinantDispatchInherited:
        o->delegated=1;
        o->completed=b->task->parent&&b->task->parent(b->task->context,a,t,running);
        break;
    case RecombinantDispatchMajor:{
        const auto* api=SiN_GetServerRecombinantTaskApi(1,sizeof(ServerRecombinantTaskApi));
        o->completed=api&&api->major(a,n,t,running,now,b->task,&o->major);
        o->delegated=o->major.delegated;o->failed=o->major.lifecycle.failed;o->task_complete=o->major.lifecycle.completed;break;
    }
    case RecombinantDispatchMinor:{
        const auto* api=SiN_GetServerRecombinantMinorTaskApi(1,sizeof(ServerRecombinantMinorTaskApi));
        o->completed=api&&b->minor&&api->execute(a,n,t,running,now,b->minor,&o->minor);
        o->delegated=o->minor.delegated;o->failed=o->minor.lifecycle.failed;o->task_complete=o->minor.lifecycle.completed;break;
    }
    case RecombinantDispatchLaunch:{
        const auto* api=SiN_GetServerRecombinantLaunchApi(1,sizeof(ServerRecombinantLaunchApi));
        o->completed=api&&b->launch&&api->start140(a,n,now,b->launch,&o->launch);
        o->failed=o->launch.lifecycle.failed;o->task_complete=o->launch.lifecycle.completed;break;
    }}
    return o->completed;
}
const ServerRecombinantDispatchApi api{1,sizeof(ServerRecombinantDispatchApi),&route,&execute};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantDispatchApi=_SiN_GetServerRecombinantDispatchApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantDispatchApi* __cdecl SiN_GetServerRecombinantDispatchApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerRecombinantDispatchApi)?&api:nullptr;
}
