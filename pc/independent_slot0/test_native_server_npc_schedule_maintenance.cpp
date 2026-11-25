#include "native_server_npc_schedule_maintenance.h"
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
using namespace sin_port::native_weapon_tables;
struct F{
    alignas(4) unsigned char actor[0xfcc]{},desc[0x48]{},rows[2][0x34]{},tasks[2][16]{};
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
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS native maintenance loop task lifecycle, failure schedule and bounded iterations");
}
