#include "native_server_recombinant_dispatch.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <string>
using namespace sin_port::native_weapon_tables;
struct F{
    alignas(4) unsigned char actor[0xc70]{},desc[0x48]{};
    unsigned parent_calls=0,activity_calls=0;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantDispatchApi>(GetProcAddress(server,server_recombinant_dispatch_export));
    assert(get&&!GetProcAddress(client,server_recombinant_dispatch_export)&&!get(2,sizeof(ServerRecombinantDispatchApi)));
    const auto* api=get(1,sizeof(ServerRecombinantDispatchApi));assert(api);
    F f;f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    ServerRecombinantPolicyBindings policy{};policy.conditions.context=&f;
    policy.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    ServerRecombinantDecisionBindings decision{};decision.policy=&policy;
    ServerRecombinantTaskBindings task{};task.context=&f;task.decision=&decision;
    task.parent=[](void* p,void*,const ServerNpcScheduleTask*,bool) noexcept ->int{++static_cast<F*>(p)->parent_calls;return 1;};
    task.activity=[](void* p,void*,int id) noexcept ->int{assert(id==921);++static_cast<F*>(p)->activity_calls;return 1;};
    ServerRecombinantDamageBindings d{&policy,&f};ServerRecombinantMinorBindings m{&d,&f,1};
    ServerRecombinantMinorTouchBindings touch{&m,&f};ServerRecombinantMinorTaskBindings mt{&task,&touch,&f,916,920,921};
    ServerRecombinantLaunchBindings launch{&mt,&f};ServerRecombinantDispatchBindings b{&task,&mt,&launch};
    unsigned major_count=0,minor_start=0,minor_run=0,launch_count=0;
    for(int id=0;id<160;++id){
        unsigned x=99;assert(api->route(recombinant_major_vtable,id,false,&x));major_count+=x==RecombinantDispatchMajor;
        assert(api->route(recombinant_major_vtable,id,true,&x)&&x==RecombinantDispatchInherited);
        assert(api->route(recombinant_minor_vtable,id,false,&x));minor_start+=x==RecombinantDispatchMinor;launch_count+=x==RecombinantDispatchLaunch;
        assert(api->route(recombinant_minor_vtable,id,true,&x));minor_run+=x==RecombinantDispatchMinor;
    }
    assert(major_count==5&&minor_start==6&&launch_count==1&&minor_run==5);
    unsigned x=99;assert(!api->route(0x2249aaa4u,140,false,&x));
    ServerRecombinantDispatchResult o{};ServerNpcScheduleTask t{};
    auto call=[&](int id,bool run=false){t.id=id;return api->execute(f.actor,sizeof(f.actor),&t,run,10.f,&b,&o);};
    f.put(f.actor,0,recombinant_major_vtable);f.actor[0xc28]=1;
    assert(call(121)&&o.kind==RecombinantMajor&&o.route==RecombinantDispatchMajor&&o.major.latch_cleared&&!f.actor[0xc28]&&o.delegated&&f.parent_calls==1);
    assert(call(140,true)&&o.route==RecombinantDispatchInherited&&f.parent_calls==2);
    f.put(f.actor,0,recombinant_minor_vtable);f.put(f.actor,0x238,0xffffffffu);
    assert(call(140)&&o.kind==RecombinantMinor&&o.route==RecombinantDispatchLaunch&&o.task_complete&&!o.delegated&&f.parent_calls==2);
    assert(call(142)&&o.route==RecombinantDispatchMinor&&f.activity_calls==1&&!o.delegated);
    assert(call(12)&&o.route==RecombinantDispatchInherited&&f.parent_calls==3);
    b.launch=nullptr;assert(!call(140)&&o.route==RecombinantDispatchLaunch&&!o.completed&&f.parent_calls==3);
    b.launch=&launch;auto bad=mt;bad.task=nullptr;b.minor=&bad;assert(!call(142)&&f.activity_calls==1);
    b.minor=&mt;auto other=policy;d.policy=&other;assert(!call(140)&&!o.completed);d.policy=&policy;
    f.put(f.actor,0,0x2249aaa4u);assert(!call(140)&&!o.completed&&f.parent_calls==3);
    FreeLibrary(server);FreeLibrary(client);return 0;
}
