#include "native_server_npc_shared_task.h"
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
struct F {
    unsigned char a[0xc1c]{},desc[0x48]{},component[9]{},row[52]{},task[8]{};
    bool component_handles=false,aimed=false;int draw=2;std::vector<int> trace;
    template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<unsigned char*>(p)+o,&v,sizeof(v));}
    template<class T>T get(unsigned o){T v{};std::memcpy(&v,a+o,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerNpcSharedTaskApi>(GetProcAddress(s,server_npc_shared_task_export));
    assert(g&&!GetProcAddress(c,server_npc_shared_task_export)&&!g(0,sizeof(ServerNpcSharedTaskApi))&&!g(1,0));
    const auto* api=g(1,sizeof(ServerNpcSharedTaskApi));assert(api);
    F f;const int global=npc_global_condition_base;
    f.put(f.desc,4,global+100);f.put(f.desc,8,0);f.put(f.desc,12,127);f.put(f.desc,16,global+227);
    f.put(f.desc,0x34,global);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.put(f.row,0,global+125);f.put(f.row,4,2u);f.put(f.row,8,1);f.put(f.task,0,2);
    ServerNpcSharedTaskBindings b{};b.context=&f;b.lifecycle.conditions.context=&f;
    b.lifecycle.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    b.random=[](void* p,int lo,int hi,int* out) noexcept -> int{auto& f=F::self(p);assert(lo==1&&hi==3);*out=f.draw;f.trace.push_back(1);return 1;};
    b.component_storage=[](void* p,std::uint32_t tok,void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(tok==77);*out=f.component;*n=sizeof(f.component);return 1;};
    b.component_task=[](void* p,std::uint32_t,const ServerNpcScheduleTask*,bool) noexcept -> int{auto& f=F::self(p);assert(f.component[8]==1);f.component[8]=f.component_handles;f.trace.push_back(2);return 1;};
    b.special67=[](void* p,void*,const ServerNpcScheduleTask*,bool) noexcept -> int{F::self(p).trace.push_back(3);return 1;};
    b.parent=[](void* p,void*,const ServerNpcScheduleTask*,bool) noexcept -> int{F::self(p).trace.push_back(4);return 1;};
    b.pending=[](void* p,void*,int id) noexcept -> int{F::self(p).trace.push_back(id);return 1;};
    b.aim_query=[](void* p,void*,bool* out) noexcept -> int{auto& f=F::self(p);*out=f.aimed;f.trace.push_back(5);return 1;};
    b.aim_update=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(6);return 1;};
    b.turn=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(7);return 1;};
    ServerNpcScheduleResolutionBindings re{};re.context=&f;re.head=1;re.grunt.schedule.context=&f;re.grunt.schedule.conditions=b.lifecycle.conditions;
    re.translate=[](void*,void*,int id,int* out) noexcept -> int{*out=id;return 1;};
    re.grunt.schedule.storage=[](void* p,std::uint32_t tok,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);if(tok==1){*out=f.row;*n=52;return 1;}if(tok==2){*out=f.task;*n=8;return 1;}return 0;};
    re.grunt.schedule.reset_movement=[](void*,void*,std::uint32_t) noexcept -> int{return 1;};b.resolution=&re;
    int tick=0;unsigned active=0,expired=0;
    assert(api->wait(f.a,sizeof(f.a),10,.13f,0,&b,&tick)&&tick==2&&f.get<float>(0x9dc)==10.2f);
    assert(api->wait_state(f.a,sizeof(f.a),10.19f,&active,&expired)&&active&&!expired);
    assert(api->wait_state(f.a,sizeof(f.a),10.2f,&active,&expired)&&expired);
    assert(api->cancel_wait(f.a,sizeof(f.a))&&api->wait_state(f.a,sizeof(f.a),10,&active,&expired)&&!active&&!expired);
    assert(api->wait(f.a,sizeof(f.a),10,-.11f,0,&b,&tick)&&tick==-1&&f.get<float>(0x9dc)==9.9f);
    assert(api->wait(f.a,sizeof(f.a),10,0,.3f,&b,&tick)&&tick==2);
    ServerNpcSharedTaskResult o{};
    auto run=[&](int id,bool running,float parameter,float now){
        f.put(f.a,0x7a4,1);f.trace.clear();ServerNpcScheduleTask task{id,parameter};assert(api->execute(f.a,sizeof(f.a),&task,running,now,&b,&o));};
    run(2,false,.13f,10);assert(o.native&&o.timer_written&&o.ticks==2);run(2,true,0,10.19f);assert(!o.lifecycle.completed);run(2,true,0,10.2f);assert(o.lifecycle.completed);
    run(5,false,.3f,10);assert(o.ticks==2&&f.trace==std::vector<int>{1});run(5,true,0,10.2f);assert(o.lifecycle.completed&&(f.trace==std::vector<int>{5,6}));
    f.aimed=true;run(4,true,0,11);assert(o.lifecycle.completed&&f.trace==std::vector<int>{5});
    run(88,false,49.9f,11);assert(f.get<int>(0x7bc)==49&&o.lifecycle.completed);
    f.put(f.a,0x988,4u);run(119,false,2,11);assert(f.get<unsigned>(0x988)==6&&o.lifecycle.completed);
    run(86,false,45,11);assert(o.activity==45&&f.trace==std::vector<int>{45});
    f.put(f.a,0x878,45);run(86,false,0,11);assert(!f.get<int>(0x878)&&f.trace.empty());
    f.put(f.a,0x3ec,3);f.put(f.a,0x884,3);run(86,true,0,11);assert(o.lifecycle.completed);
    run(92,false,45,11);assert(o.activity==45);run(92,true,0,11);assert(!o.lifecycle.completed&&f.trace==std::vector<int>{7});
    f.a[0x3e0]=1;run(92,true,0,11);assert(o.lifecycle.completed);
    run(95,false,54,11);assert(o.activity==54);
    run(87,false,25,11);assert(o.schedule_set&&f.get<int>(0x79c)==global+125&&f.get<unsigned>(0x798)==1&&o.lifecycle.completed);
    f.put(f.a,0xc18,77u);f.component_handles=true;run(88,false,12,11);assert(o.component_handled&&!o.native&&f.trace==std::vector<int>{2});
    f.component_handles=false;run(88,false,12,11);assert(o.component_called&&!o.component_handled&&o.native&&f.get<int>(0x7bc)==12);
    run(67,true,0,11);assert(o.special_called&&!o.component_called&&f.trace==std::vector<int>{3});
    run(116,false,0,11);assert(o.parent_called&&(f.trace==std::vector<int>{2,4}));
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS original shared task timer, schedule and component routes");
}
