#include "native_server_recombinant_session.h"
#include <cmath>
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerRecombinantScheduleApi* __cdecl SiN_GetServerRecombinantScheduleApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerRecombinantDispatchApi* __cdecl SiN_GetServerRecombinantDispatchApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerRecombinantDecisionApi* __cdecl SiN_GetServerRecombinantDecisionApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerRecombinantPolicyApi* __cdecl SiN_GetServerRecombinantPolicyApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerRecombinantLaunchApi* __cdecl SiN_GetServerRecombinantLaunchApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerRecombinantMinorApi* __cdecl SiN_GetServerRecombinantMinorApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerRecombinantPrescheduleApi* __cdecl SiN_GetServerRecombinantPrescheduleApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerNpcScheduleStateApi* __cdecl SiN_GetServerNpcScheduleStateApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerNpcScheduleNamespaceApi* __cdecl SiN_GetServerNpcScheduleNamespaceApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
const ServerRecombinantScheduleApi* catalog() noexcept{return SiN_GetServerRecombinantScheduleApi(1,sizeof(ServerRecombinantScheduleApi));}
const ServerRecombinantDecisionApi* decisions() noexcept{return SiN_GetServerRecombinantDecisionApi(1,sizeof(ServerRecombinantDecisionApi));}
const ServerRecombinantPolicyBindings& policy(const ServerRecombinantSession& s) noexcept{return *s.bindings->dispatch->task->decision->policy;}
bool coherent(const ServerRecombinantSession& s) noexcept{
    const auto* b=s.bindings;
    if(!s.actor||!b||!b->image||!b->dispatch||!b->dispatch->task||!b->dispatch->task->decision||!b->dispatch->task->decision->policy||s.schedule.context!=&s)return false;
    const auto& p=policy(s);const auto& im=*b->image;const auto& q=b->inherited.conditions;
    if(s.bytes<(s.kind==RecombinantMajor?0xc6cu:0xc48u)||
       rd<unsigned>(s.actor,0)!=(s.kind==RecombinantMajor?recombinant_major_vtable:recombinant_minor_vtable)||
       im.kind!=s.kind||im.activities!=p.activities||!p.activities||
       q.context!=p.conditions.context||q.descriptor!=p.conditions.descriptor||q.storage!=p.conditions.storage||
       p.entry_count>sin_independent::kEntityHandleCapacity||(!p.entries&&p.entry_count))return false;
    auto d=b->dispatch;auto minor=d->minor;
    if(minor&&minor->task!=d->task)return false;
    if(d->launch&&d->launch->task!=minor)return false;
    if(minor&&minor->touch&&minor->touch->minor&&minor->touch->minor->damage&&minor->touch->minor->damage->policy!=&p)return false;
    if(b->major_preschedule&&b->major_preschedule->policy!=&p)return false;
    if(s.kind==RecombinantMinor&&(!minor||!minor->touch||!minor->touch->minor||!minor->touch->minor->damage||!d->launch))return false;
    unsigned token=0;const auto* api=catalog();return api&&api->find(&im,80,&token)&&token;
}
int __cdecl storage(void* context,unsigned token,const void** out,unsigned* n) noexcept{
    if(!out||!n)return 0;*out=nullptr;*n=0;
    auto* s=static_cast<ServerRecombinantSession*>(context);if(!s||!coherent(*s))return 0;
    const auto& im=*s->bindings->image;
    if(token>=im.base&&token-im.base<36u){const auto* api=catalog();return api&&api->storage(&im,token,out,n);}
    const auto& inherited=s->bindings->inherited;
    return inherited.storage&&inherited.storage(inherited.context,token,out,n);
}
int __cdecl reset(void* context,void* a,unsigned token) noexcept{
    auto* s=static_cast<ServerRecombinantSession*>(context);if(!s||a!=s->actor||!coherent(*s))return 0;
    const auto& b=s->bindings->inherited;return b.reset_movement&&b.reset_movement(b.context,a,token);
}
int __cdecl bind(void* a,unsigned n,const ServerRecombinantSessionBindings* b,ServerRecombinantSession* s) noexcept{
    if(!s)return 0;*s={};
    if(!a||n<4||!b||!b->inherited.reset_movement)return 0;
    unsigned vt=rd<unsigned>(a,0),kind=vt==recombinant_major_vtable?RecombinantMajor:vt==recombinant_minor_vtable?RecombinantMinor:0;
    if(!kind)return 0;
    s->actor=a;s->bytes=n;s->kind=kind;s->bindings=b;s->schedule={b->inherited.conditions,s,&storage,&reset};
    if(!coherent(*s)){*s={};return 0;}return 1;
}
bool begin(ServerRecombinantSession* s,ServerRecombinantSessionResult* o) noexcept{
    if(!o)return false;*o={};o->requested=o->translated=o->global=-1;
    if(!s||!coherent(*s))return false;o->kind=s->kind;return true;
}
int __cdecl select(ServerRecombinantSession* s,ServerRecombinantSessionResult* o) noexcept{
    if(!begin(s,o))return 0;
    const auto* api=decisions();
    if(!api||!api->select(s->actor,s->bytes,s->bindings->dispatch->task->decision,&o->decision))return 0;
    o->requested=o->decision.selected;o->delegated=o->decision.delegated;o->completed=1;return 1;
}
int __cdecl fail_select(ServerRecombinantSession* s,int schedule,int task,int reason,ServerRecombinantSessionResult* o) noexcept{
    if(!begin(s,o))return 0;
    const auto* api=SiN_GetServerRecombinantLaunchApi(1,sizeof(ServerRecombinantLaunchApi));
    const auto* b=s->bindings->dispatch->launch;
    if(!api||!b||!api->fail_select(s->actor,s->bytes,schedule,task,reason,b,&o->failure))return 0;
    o->requested=o->failure.selected;o->delegated=o->failure.delegated;o->completed=1;return 1;
}
int __cdecl resolve(ServerRecombinantSession* s,int id,ServerRecombinantSessionResult* o) noexcept{
    if(!begin(s,o))return 0;o->requested=id;
    const auto* api=SiN_GetServerRecombinantPolicyApi(1,sizeof(ServerRecombinantPolicyApi));
    const auto* ns=SiN_GetServerNpcScheduleNamespaceApi(1,sizeof(ServerNpcScheduleNamespaceApi));
    const auto* cat=catalog();
    if(!api||!ns||!cat||!api->schedule(s->actor,s->bytes,id,&policy(*s),&o->policy))return 0;
    o->translated=o->policy.output;
    if(!ns->to_global(s->actor,o->translated,&s->schedule.conditions,&o->global)||o->global<npc_global_condition_base)return 0;
    if(!cat->find(s->bindings->image,o->translated,&o->token))return 0;
    if(o->token)o->owned=1;
    else{
        ++o->delegated;
        if(!s->bindings->inherited_lookup||!s->bindings->inherited_lookup(s->bindings->context,s->actor,o->translated,&o->token))return 0;
        // A missing inherited descriptor is left to the owner to resolve.
        if(o->token>=s->bindings->image->base&&o->token-s->bindings->image->base<36u)return 0;
    }
    if(o->token){const void* row=nullptr;unsigned n=0;if(!storage(s,o->token,&row,&n)||!row||n<52||rd<int>(row,0)!=o->global)return 0;}
    o->completed=1;return 1;
}
int __cdecl gather(ServerRecombinantSession* s,ServerRecombinantSessionResult* o) noexcept{
    if(!begin(s,o))return 0;
    if(s->kind==RecombinantMajor){
        const auto* api=decisions();o->completed=api&&api->gather_major(s->actor,s->bytes,s->bindings->dispatch->task->decision,&o->decision);
    }else{
        const auto* api=SiN_GetServerRecombinantMinorApi(1,sizeof(ServerRecombinantMinorApi));
        o->completed=api&&api->gather(s->actor,s->bytes,s->bindings->dispatch->minor->touch->minor,&o->minor);
    }
    return o->completed;
}
int __cdecl preschedule(ServerRecombinantSession* s,float now,ServerRecombinantSessionResult* o) noexcept{
    if(!begin(s,o)||!std::isfinite(now))return 0;
    if(s->kind==RecombinantMinor){o->completed=1;return 1;} // original RET
    const auto* api=SiN_GetServerRecombinantPrescheduleApi(1,sizeof(ServerRecombinantPrescheduleApi));
    o->completed=api&&s->bindings->major_preschedule&&api->run_major(s->actor,s->bytes,now,s->bindings->major_preschedule,&o->preschedule);return o->completed;
}
int __cdecl interrupts(ServerRecombinantSession* s,ServerRecombinantSessionResult* o) noexcept{
    if(!begin(s,o))return 0;
    const auto* api=decisions();o->completed=api&&api->interrupts(s->actor,s->bytes,s->bindings->dispatch->task->decision,&o->decision);return o->completed;
}
int __cdecl current_task(ServerRecombinantSession* s,bool running,float now,ServerRecombinantSessionResult* o) noexcept{
    if(!begin(s,o)||!std::isfinite(now))return 0;
    const auto* sa=SiN_GetServerNpcScheduleStateApi(1,sizeof(ServerNpcScheduleStateApi));
    const auto* api=SiN_GetServerRecombinantDispatchApi(1,sizeof(ServerRecombinantDispatchApi));
    if(!sa||!api||!sa->inspect(s->actor,s->bytes,&s->schedule,&o->state)||!o->state.has_task)return 0;
    o->token=o->state.schedule;o->owned=o->token>=s->bindings->image->base&&o->token-s->bindings->image->base<36u;
    o->completed=api->execute(s->actor,s->bytes,&o->state.task,running,now,s->bindings->dispatch,&o->task);
    o->delegated=o->task.delegated;return o->completed;
}
const ServerRecombinantSessionApi api{1,sizeof(ServerRecombinantSessionApi),&bind,&select,&fail_select,&resolve,&gather,&preschedule,&interrupts,&current_task};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantSessionApi=_SiN_GetServerRecombinantSessionApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantSessionApi* __cdecl SiN_GetServerRecombinantSessionApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerRecombinantSessionApi)?&api:nullptr;
}
