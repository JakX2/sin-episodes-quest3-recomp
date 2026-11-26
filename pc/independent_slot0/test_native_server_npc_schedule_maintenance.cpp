#include "native_server_npc_schedule_services.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>
#include <initializer_list>
using namespace sin_port::native_weapon_tables;
struct F{
    alignas(4) unsigned char actor[0x1088]{},desc[0x48]{},rows[2][0x34]{},tasks[2][16]{};
    std::vector<int> trace;
    int mode=0;bool yield=false;
    unsigned budget_calls=0;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(unsigned off){T v{};std::memcpy(&v,actor+off,sizeof(v));return v;}
    void condition(int id){unsigned off=0x7cc+4*(id/32);put(actor,off,get<unsigned>(off)|(1u<<(id%32)));}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerNpcScheduleMaintenanceApi>(GetProcAddress(s,server_npc_schedule_maintenance_export));
    assert(g&&!GetProcAddress(c,server_npc_schedule_maintenance_export)&&!g(0,sizeof(ServerNpcScheduleMaintenanceApi))&&!g(1,0));
    const auto* api=g(1,sizeof(ServerNpcScheduleMaintenanceApi));assert(api);
    F f;const int global=npc_global_condition_base;
    f.put(f.desc,4,global);f.put(f.desc,8,0);f.put(f.desc,12,127);f.put(f.desc,16,global+127);
    f.put(f.desc,0x34,global);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.put(f.rows[0],0,global+1);f.put(f.rows[0],4,3u);f.put(f.rows[0],8,2);f.put(f.rows[0],0x30,2u);
    f.put(f.rows[1],0,global+2);f.put(f.rows[1],4,4u);f.put(f.rows[1],8,1);
    f.put(f.tasks[0],0,10);f.put(f.tasks[0],8,11);f.put(f.tasks[1],0,20);
    ServerNpcScheduleMaintenanceBindings b{};b.context=&f;b.selection.context=&f;b.selection.head=1;
    auto& sb=b.selection.grunt.schedule;sb.context=&f;sb.conditions.context=&f;
    sb.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    sb.storage=[](void* p,std::uint32_t tok,const void** out,unsigned* n) noexcept -> int{
        auto& f=F::self(p);if(tok<=2&&tok){*out=f.rows[tok-1];*n=sizeof(f.rows[0]);}
        else if(tok==3||tok==4){*out=f.tasks[tok-3];*n=16;}else return 0;return 1;};
    sb.reset_movement=[](void* p,void*,std::uint32_t) noexcept -> int{F::self(p).trace.push_back(30);return 1;};
    b.selection.state.schedule=sb;b.selection.grunt.context=&f;
    b.selection.grunt.parent=[](void*,void*,int* out) noexcept -> int{*out=1;return 1;};
    b.selection.state.state_query=[](void*,void*,int,int* out) noexcept -> int{*out=-1;return 1;};
    b.selection.translate=[](void*,void*,int id,int* out) noexcept -> int{*out=id;return 1;};
    b.selection.fail_select=[](void*,void*,int sched,int task,int failure,int* out) noexcept -> int{assert(sched==1&&task==10&&failure==7);*out=2;return 1;};
    b.validity.schedule=sb;b.validity.base_build=[](void*,void*) noexcept -> int{return 1;};
    b.validity.mobility=[](void*,float,bool* out) noexcept -> int{*out=false;return 1;};
    b.invalidated=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(40);return 1;};
    b.before_selection=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(41);return 1;};
    b.start_task=[](void* p,void*,const ServerNpcScheduleTask* t) noexcept -> int{
        auto& f=F::self(p);assert(f.get<int>(0x7a4)==1&&f.get<float>(0x7ac)==10.f);f.trace.push_back(t->id);
        if(f.mode==1||f.mode==4||(f.mode==0&&t->id==10))f.put(f.actor,0x7a4,4);
        if(f.mode==2)f.put(f.actor,0x7a4,2);
        if(f.mode==3)f.put(f.actor,0x7b4,1);
        return 1;};
    b.run_task=[](void* p,void*,const ServerNpcScheduleTask* t) noexcept -> int{F::self(p).trace.push_back(100+t->id);return 1;};
    b.after_start=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(50);return 1;};
    b.motion_gate=[](void*,void*,bool* out) noexcept -> int{*out=true;return 1;};
    b.after_run=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(51);return 1;};
    b.transition=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(60);return 1;};
    b.idle=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(61);return 1;};
    b.budget=[](void* p,void*,bool* out) noexcept -> int{auto& f=F::self(p);++f.budget_calls;*out=f.yield;return 1;};
    auto reset=[&](){std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0x850,1);f.put(f.actor,0x858,1);f.trace.clear();f.budget_calls=0;f.yield=false;};
    ServerNpcScheduleMaintenanceResult out{};reset();
    assert(api->run(f.actor,sizeof(f.actor),10,&b,&out)&&out.iterations==2&&out.selected==1&&out.started==2&&out.advanced==1&&out.runs==1&&out.transitions==3);
    assert((f.trace==std::vector<int>{40,41,30,10,60,11,50,60,111,51,60})&&f.actor[0x98a]==4);
    reset();f.mode=1;f.yield=true;
    assert(api->run(f.actor,sizeof(f.actor),10,&b,&out)&&out.iterations==1&&out.started==1&&out.budget_yield&&!out.advanced&&out.transitions==2);
    reset();f.mode=2;
    assert(api->run(f.actor,sizeof(f.actor),10,&b,&out)&&out.started==1&&!out.runs&&out.transitions==2);
    reset();f.mode=3;
    assert(api->run(f.actor,sizeof(f.actor),10,&b,&out)&&out.runs==8&&out.inner_limit&&out.iterations==1);
    reset();f.mode=3;b.check_inner_budget=true;f.yield=true;
    assert(api->run(f.actor,sizeof(f.actor),10,&b,&out)&&out.runs==1&&out.budget_yield&&!out.inner_limit);b.check_inner_budget=false;
    reset();f.mode=4;
    assert(api->run(f.actor,sizeof(f.actor),10,&b,&out)&&out.iterations==10&&out.iteration_limit&&out.started==10&&out.advanced==9);
    reset();f.mode=0;f.put(f.actor,0x798,1u);f.put(f.actor,0x7a4,1);f.put(f.actor,0x7b0,7);f.condition(0x21);
    assert(api->run(f.actor,sizeof(f.actor),10,&b,&out)&&out.failed_selected==1&&out.state.schedule==2&&f.get<int>(0x79c)==global+2&&out.runs==1);
    reset();b.selection.head=0;
    assert(api->run(f.actor,sizeof(f.actor),10,&b,&out)&&out.missing_schedule&&out.selected==2&&!out.transitions&&f.trace.back()==61);

    auto gg=reinterpret_cast<GetServerGruntSchedulerApi>(GetProcAddress(s,server_grunt_scheduler_export));
    assert(gg&&!GetProcAddress(c,server_grunt_scheduler_export)&&!gg(0,sizeof(ServerGruntSchedulerApi))&&!gg(1,0));
    const auto* grunt=gg(1,sizeof(ServerGruntSchedulerApi));assert(grunt);
    ServerGruntSchedulerBindings gb{};gb.maintenance=b;gb.maintenance.selection.head=1;
    gb.maintenance.start_task=nullptr;gb.maintenance.run_task=nullptr;
    gb.tasks.context=&f;
    gb.tasks.special=[](void* p,void*,const ServerNpcScheduleTask* task,bool running) noexcept -> int {
        auto& f=F::self(p);assert(task->id==149&&!running&&f.get<float>(0x9f0)==.75f);f.trace.push_back(149);return 1;};
    gb.tasks.finish_hook=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(158);return 1;};
    gb.maintenance.selection.fail_select=[](void*,void*,int sched,int task,int failure,int* out) noexcept -> int{
        assert(sched==1&&task==149&&failure==6);*out=2;return 1;};
    f.put(f.tasks[0],0,147);f.put(f.tasks[0],8,149);f.put(f.tasks[1],0,158);
    reset();f.actor[0xca0]=1;
    ServerGruntSchedulerResult gout{};
    assert(grunt->run(f.actor,sizeof(f.actor),10,&gb,&gout));
    assert(gout.maintenance.started==2&&gout.maintenance.advanced==1&&gout.maintenance.runs==2&&gout.native_tasks==3&&gout.special_tasks==1&&gout.task_failures==1);
    assert(!f.actor[0xca0]&&f.get<int>(0x7b0)==6&&(f.get<unsigned>(0x7d0)&2u)&&gout.last.lifecycle.failed);
    assert((f.trace==std::vector<int>{40,41,30,50,60,149,50,60,60}));
    f.trace.clear();f.yield=true;
    assert(grunt->run(f.actor,sizeof(f.actor),10,&gb,&gout)&&gout.maintenance.failed_selected==1&&gout.maintenance.state.schedule==2&&gout.native_tasks==1&&gout.last.lifecycle.completed&&gout.maintenance.budget_yield);
    assert(f.get<int>(0x7b0)==0&&!(f.get<unsigned>(0x7d0)&2u)&&f.get<int>(0x7a4)==4);
    assert((f.trace==std::vector<int>{40,41,30,158,60,60}));
    gb.maintenance.selection.jetpack=true;f.trace.clear();
    assert(!grunt->run(f.actor,sizeof(f.actor),10,&gb,&gout)&&f.trace.empty());

    auto gs=reinterpret_cast<GetServerNpcSubclassSchedulerApi>(GetProcAddress(s,server_npc_subclass_scheduler_export));
    assert(gs&&!GetProcAddress(c,server_npc_subclass_scheduler_export)&&!gs(0,sizeof(ServerNpcSubclassSchedulerApi))&&!gs(1,0));
    const auto* sub=gs(1,sizeof(ServerNpcSubclassSchedulerApi));assert(sub);
    unsigned kind=99;assert(sub->classify(0x2248df04,&kind)&&kind==NpcSubclassElite);
    for(unsigned v:{0x224954a4u,0x2249aaa4u,0x224a416cu})assert(sub->classify(v,&kind)&&kind==NpcSubclassGround);
    for(unsigned v:{0x2248c514u,0x2248e83cu,0x2249964cu,0x224a4a4cu})assert(sub->classify(v,&kind)&&kind==NpcSubclassJetpack);
    assert(!sub->classify(0,&kind));
    ServerNpcSubclassSchedulerBindings cb{};cb.maintenance=gb.maintenance;cb.grunt=gb.tasks;
    cb.original_vtable=0x2248df04;cb.maintenance.selection.jetpack=false;
    f.put(f.tasks[0],0,3);f.put(f.tasks[0],4,1.f);
    reset();f.yield=true;ServerNpcSubclassSchedulerResult sout{};
    assert(sub->run(f.actor,sizeof(f.actor),10,&cb,&sout)&&sout.subclass==NpcSubclassElite&&sout.task_calls==1&&sout.elite.lifecycle.completed&&!sout.delegated);
    assert((f.trace==std::vector<int>{40,41,30,60,60}));
    cb.original_vtable=0x2249964c;f.put(f.rows[0],0,global+0x76);f.put(f.tasks[0],0,160);
    reset();f.yield=true;
    assert(sub->run(f.actor,sizeof(f.actor),10,&cb,&sout)&&sout.subclass==NpcSubclassJetpack&&sout.jet_selector.schedule==0x76&&sout.maintenance.selection.requested==0x76&&sout.jetpack.lifecycle.completed);
    assert(f.get<int>(0xfec)==2&&f.get<int>(0x79c)==global+0x76&&sout.task_calls==1&&!sout.delegated);
    assert((f.trace==std::vector<int>{40,41,30,60,60}));
    cb.original_vtable=0;f.trace.clear();assert(!sub->run(f.actor,sizeof(f.actor),10,&cb,&sout)&&f.trace.empty());

    auto gsvc=reinterpret_cast<GetServerNpcScheduleServicesApi>(GetProcAddress(s,server_npc_schedule_services_export));
    assert(gsvc&&!GetProcAddress(c,server_npc_schedule_services_export)&&!gsvc(0,sizeof(ServerNpcScheduleServicesApi))&&!gsvc(1,0));
    const auto* svc=gsvc(1,sizeof(ServerNpcScheduleServicesApi));assert(svc);
    ServerNpcScheduleServicesBindings sc{};sc.scheduler=cb;sc.scheduler.original_vtable=0x2249aaa4;
    sc.scheduler.maintenance.selection.translate=nullptr;sc.scheduler.maintenance.selection.special_select=nullptr;
    alignas(4) unsigned char controller[0x3b0]{};
    sin_independent::EntityHandleEntry slots[2]{};slots[1].serial=7;slots[1].entity=reinterpret_cast<CBaseEntity*>(controller);
    sc.scheduler.maintenance.selection.state.entities={slots,2,nullptr,nullptr,nullptr};
    sc.inherited.extent=[](void*,void*,unsigned* n) noexcept -> int{*n=0x3b0;return 1;};
    f.put(controller,0x3ac,2);f.put(f.rows[0],0,global+0x38);f.put(f.tasks[0],0,158);
    reset();f.put(f.actor,0x850,4);f.put(f.actor,0x858,4);f.put(f.actor,0x970,0x7001u);f.yield=true;
    ServerNpcScheduleServicesResult svout{};
    assert(svc->run(f.actor,sizeof(f.actor),10,&sc,&svout)&&svout.special_selections==1&&svout.translations==1&&svout.inherited_calls==1);
    assert(svout.special.controller_live&&svout.special.schedule==0x36&&svout.translation.inherited.phase==2&&svout.translation.translation.output==0x38);
    assert(svout.scheduler.maintenance.selection.requested==0x36&&svout.scheduler.maintenance.selection.translated==0x38&&svout.scheduler.maintenance.state.schedule==1&&f.get<int>(0x79c)==global+0x36);
    assert(svout.scheduler.grunt.lifecycle.completed&&(f.trace==std::vector<int>{40,41,30,158,60,60}));
    // Native Elite direct remap, no inherited parent call.
    reset();f.yield=true;sc.scheduler.original_vtable=0x2248df04;
    sc.scheduler.maintenance.selection.grunt.parent=[](void*,void*,int* out) noexcept -> int{*out=0x19;return 1;};
    f.put(f.rows[0],0,global+0x76);f.put(f.tasks[0],0,3);f.put(f.tasks[0],4,1.f);
    assert(svc->run(f.actor,sizeof(f.actor),10,&sc,&svout)&&svout.translations==1&&!svout.special_selections&&!svout.inherited_calls&&svout.translation.translation.output==0x76&&svout.scheduler.elite.lifecycle.completed);
    // Full Jetpack selector and translator: ordinary patrol76 unchanged.
    reset();f.yield=true;sc.scheduler.original_vtable=0x2249964c;f.put(f.tasks[0],0,160);
    assert(svc->run(f.actor,sizeof(f.actor),10,&sc,&svout)&&svout.scheduler.jet_selector.schedule==0x76&&svout.inherited_calls==1&&svout.translation.translation.output==0x76&&f.get<int>(0xfec)==2);
    assert(!svc->run(f.actor,0x1084,10,&sc,&svout));
    {
        auto get_gather=reinterpret_cast<GetServerNpcGatherSchedulerApi>(GetProcAddress(s,server_npc_gather_scheduler_export));
        assert(get_gather&&!GetProcAddress(c,server_npc_gather_scheduler_export)&&!get_gather(0,sizeof(ServerNpcGatherSchedulerApi))&&!get_gather(1,0));
        const auto* gsched=get_gather(1,sizeof(ServerNpcGatherSchedulerApi));assert(gsched);
        ServerNpcGatherSchedulerBindings combined{};combined.scheduler=cb;combined.scheduler.original_vtable=0x2249aaa4;
        combined.scheduler.maintenance.before_selection=nullptr;
        combined.gather.max_clients=2;
        combined.gather.base.gate.context=&f;
        combined.gather.base.gate.storage.context=&f;
        // The scheduler fixture uses a zero-initialized task array as its motor storage.
        combined.gather.base.gate.storage.storage=[](void* p,std::uint32_t token,void** ptr,unsigned* n) noexcept -> int{
            if(token!=55)return 0;*ptr=F::self(p).tasks;*n=sizeof(F::self(p).tasks);return 1;
        };
        combined.gather.base.gate.pvs_index=[](void* p,unsigned,bool* yes) noexcept -> int{F::self(p).trace.push_back(42);*yes=false;return 1;};
        f.put(f.rows[0],0,global+1);f.put(f.tasks[0],0,158);f.put(f.tasks[0],4,0.f);
        auto setup=[&](){reset();f.yield=true;f.put(f.actor,0x908,55u);for(unsigned i=0;i<8;++i)f.put(f.actor,0x82c+4*i,0xffffffffu);};
        ServerNpcGatherSchedulerResult gout2;
        setup();f.condition(7);
        assert(gsched->run(f.actor,sizeof(f.actor),10,&combined,&gout2)&&gout2.gather_calls==1&&gout2.gather.completed&&gout2.gather.base_called&&gout2.gather.base.post.skipped_perception&&gout2.gather.base.post.cleared_sense_conditions==1&&gout2.scheduler.grunt.lifecycle.completed);
        assert((f.trace==std::vector<int>{40,42,30,158,60,60}));
        setup();f.actor[0x84d]=f.actor[0x84e]=1;
        assert(gsched->run(f.actor,sizeof(f.actor),10,&combined,&gout2)&&!gout2.gather_calls&&gout2.scheduler.grunt.lifecycle.completed);
        assert((f.trace==std::vector<int>{40,30,158,60,60}));
        setup();f.condition(0x3f);
        assert(gsched->run(f.actor,sizeof(f.actor),10,&combined,&gout2)&&!gout2.gather_calls);
    }
    {
        const auto ng=reinterpret_cast<GetServerNpcNativeRequestSchedulerApi>(GetProcAddress(s,server_npc_native_request_scheduler_export));
        assert(ng&&!GetProcAddress(c,server_npc_native_request_scheduler_export)&&!ng(0,sizeof(ServerNpcNativeRequestSchedulerApi))&&!ng(1,0));
        const auto* native=ng(1,sizeof(ServerNpcNativeRequestSchedulerApi));assert(native);
        ServerNpcGruntAssaultScheduleBindings schedule{};
        schedule.admission.actor_token=0x10000u;
        ServerNpcGruntAssaultActivationBindings activation{};activation.schedule=&schedule;
        ServerNpcGruntAssaultArbitrationBindings arbitration{};arbitration.activation=&activation;
        arbitration.shared_select=[](void*,void*,int* id) noexcept -> int{*id=18;return 1;};
        arbitration.shared_translate=[](void*,void*,int id,int* result) noexcept -> int{*result=id;return 1;};
        ServerNpcGruntScheduleAcquireBindings ac{};ac.arbitration=&arbitration;
        ServerNpcGruntScheduleRequestBindings request{};request.acquisition=&ac;
        ServerNpcGruntSelectorTailBindings tail{};tail.request=&request;
        ServerNpcGruntCombatSelectorBindings combat{};combat.tail=&tail;
        ServerNpcGruntSpecialCombatBindings special{};special.combat=&combat;
        ServerNpcGruntScheduleSceneServices scene{};
        ServerNpcGruntScheduleLineServices line{};line.scene=&scene;
        ServerNpcNativeRequestSchedulerBindings nb{};nb.scheduler=cb;nb.scheduler.original_vtable=0x2249aaa4;nb.selector=&special;nb.line=&line;
        nb.scheduler.maintenance.selection.head=1;nb.scheduler.maintenance.selection.translate=nullptr;nb.scheduler.maintenance.selection.fail_select=nullptr;
        nb.scheduler.maintenance.selection.special_select=nullptr;nb.scheduler.maintenance.selection.grunt.parent=nullptr;
        static ServerNpcNativeRequestSchedulerResult nr{};
        f.put(f.rows[0],0,global+82);f.put(f.rows[0],4,3u);f.put(f.rows[0],8,2);
        f.put(f.rows[1],0,global+94);f.put(f.rows[1],4,4u);f.put(f.rows[1],8,1);
        f.put(f.tasks[0],0,147);f.put(f.tasks[0],8,158);f.put(f.tasks[1],0,158);
        reset();f.put(f.actor,0x850,4);f.put(f.actor,0x858,4);f.actor[0xca0]=1;f.yield=true;
        assert(native->run(f.actor,sizeof(f.actor),10,&nb,&nr,nullptr,nullptr));
        assert(nr.requests==1&&!nr.failure_requests&&nr.request.completed&&nr.request.requested==18&&nr.request.acquisition.translated==82&&nr.request.acquisition.translated_native);
        assert(nr.scheduler.maintenance.selection.token==1&&nr.scheduler.maintenance.state.schedule==1&&nr.scheduler.maintenance.started==1&&nr.scheduler.maintenance.runs==1&&nr.scheduler.grunt.lifecycle.completed&&!f.actor[0xca0]);
        assert(f.get<int>(0x79c)==global+18&&f.get<int>(0x7a4)==4);
        f.trace.clear();assert(native->run(f.actor,sizeof(f.actor),10,&nb,&nr,nullptr,nullptr)&&nr.requests==0&&nr.scheduler.maintenance.advanced==1&&nr.scheduler.grunt.lifecycle.completed&&nr.scheduler.maintenance.state.index==1);
        // Original failure path uses +7bc and native translation, not the old fail_select callback.
        reset();f.put(f.actor,0x798,1u);f.put(f.actor,0x7a4,1);f.put(f.actor,0x7b0,7);f.put(f.actor,0x7bc,26);f.condition(33);f.yield=true;
        assert(native->run(f.actor,sizeof(f.actor),10,&nb,&nr,nullptr,nullptr)&&nr.requests==1&&nr.failure_requests==1&&nr.request.current_schedule==82&&nr.request.current_task==147&&nr.request.failure_reason==7);
        assert(nr.request.requested==26&&nr.request.acquisition.translated==94&&nr.scheduler.maintenance.state.schedule==2&&f.get<int>(0x79c)==global+94&&nr.scheduler.grunt.lifecycle.completed);
        // Condition63 keeps the original forced71 path and bypasses shared selection.
        reset();f.condition(63);f.yield=true;f.put(f.rows[0],0,global+71);f.put(f.tasks[0],0,158);
        arbitration.shared_select=nullptr;
        assert(native->run(f.actor,sizeof(f.actor),10,&nb,&nr,nullptr,nullptr)&&nr.request.forced_schedule&&nr.request.requested==71&&nr.scheduler.maintenance.state.schedule==1&&nr.scheduler.grunt.lifecycle.completed);
        // Elite shares +494/+498 but has its own +43c mapping and task dispatcher.
        nb.scheduler.original_vtable=0x2248df04;arbitration.shared_select=[](void*,void*,int* id) noexcept -> int{*id=25;return 1;};
        arbitration.shared_translate=nullptr;
        f.put(f.rows[0],0,global+118);f.put(f.tasks[0],0,3);f.put(f.tasks[0],4,1.f);
        reset();f.put(f.actor,0x850,4);f.put(f.actor,0x858,4);f.yield=true;
        assert(native->run(f.actor,sizeof(f.actor),10,&nb,&nr,nullptr,nullptr)&&nr.request.requested==25&&nr.request.acquisition.translated==118&&nr.scheduler.elite.lifecycle.completed);
        assert(nr.scheduler.subclass==NpcSubclassElite&&f.get<int>(0x79c)==global+25);
        assert(!native->run(f.actor,0xfdf,10,&nb,&nr,nullptr,nullptr));
        nb.scheduler.original_vtable=0x2249964c;assert(!native->run(f.actor,sizeof(f.actor),10,&nb,&nr,nullptr,nullptr));
        const auto jg=reinterpret_cast<GetServerNpcNativeJetRequestSchedulerApi>(GetProcAddress(s,server_npc_native_jet_request_scheduler_export));
        assert(jg&&!GetProcAddress(c,server_npc_native_jet_request_scheduler_export)&&!jg(0,sizeof(ServerNpcNativeJetRequestSchedulerApi))&&!jg(1,0));
        const auto* jet_api=jg(1,sizeof(ServerNpcNativeJetRequestSchedulerApi));assert(jet_api);
        const float invalid[3]{};
        ServerNpcGruntJetSelectorBindings jet{};jet.combat=&combat;jet.invalid_point=invalid;
        ServerNpcNativeJetRequestSchedulerBindings jb{};jb.scheduler=nb.scheduler;jb.selector=&jet;jb.line=&line;
        static ServerNpcNativeJetRequestSchedulerResult jr{};
        arbitration.shared_select=nullptr;arbitration.shared_translate=[](void*,void*,int id,int* v) noexcept -> int{*v=id;return 1;};
        f.put(f.tasks[0],0,160);
        for(unsigned vt:{0x2248c514u,0x2248e83cu,0x2249964cu,0x224a4a4cu}){
            jb.scheduler.original_vtable=vt;reset();f.yield=true;
            assert(jet_api->run(f.actor,sizeof(f.actor),10,&jb,&jr,nullptr,nullptr)&&jr.base.request.requested==118&&jr.base.request.acquisition.translated==118&&jr.tail.completed&&jr.tail.schedule==118);
            assert(jr.base.scheduler.subclass==NpcSubclassJetpack&&jr.base.scheduler.jetpack.lifecycle.completed&&f.get<int>(0xfec)==2);
        }
        // Jet failure translates 41 directly to 122; no shared translation.
        f.put(f.rows[1],0,global+122);f.put(f.tasks[1],0,160);
        arbitration.shared_translate=nullptr;reset();f.yield=true;f.put(f.actor,0x798,1u);f.put(f.actor,0x7a4,1);f.put(f.actor,0x7b0,7);f.put(f.actor,0x7bc,41);f.condition(33);
        assert(jet_api->run(f.actor,sizeof(f.actor),10,&jb,&jr,nullptr,nullptr)&&jr.base.failure_requests==1&&jr.base.request.requested==41&&jr.base.request.acquisition.translated==122&&jr.base.scheduler.maintenance.state.schedule==2&&jr.base.scheduler.jetpack.lifecycle.completed);
        assert(!jet_api->run(f.actor,0x1084,10,&jb,&jr,nullptr,nullptr));
        jb.scheduler.original_vtable=0x2249aaa4;assert(!jet_api->run(f.actor,sizeof(f.actor),10,&jb,&jr,nullptr,nullptr));
    }
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS native maintenance loop task lifecycle, failure schedule and bounded iterations");
}
