#include "native_server_npc_schedule_resolution.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace sin_port::native_weapon_tables;
struct F{
    alignas(4) unsigned char actor[0xdd8]{},desc[0x48]{},rows[3][0x34]{},tasks[8]{};
    unsigned translations=0,component=0,deactivated=0;int translated=-1;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(unsigned off){T v{};std::memcpy(&v,actor+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerNpcScheduleResolutionApi>(GetProcAddress(s,server_npc_schedule_resolution_export));
    auto gn=reinterpret_cast<GetServerNpcScheduleNamespaceApi>(GetProcAddress(s,server_npc_schedule_namespace_export));
    assert(g&&gn&&!GetProcAddress(c,server_npc_schedule_resolution_export)&&!g(0,sizeof(ServerNpcScheduleResolutionApi))&&!g(1,0));
    const auto* api=g(1,sizeof(ServerNpcScheduleResolutionApi));const auto* ns=gn(1,sizeof(ServerNpcScheduleNamespaceApi));assert(api&&ns);
    F f;const int global=npc_global_condition_base;
    f.put(f.desc,4,global+100);f.put(f.desc,8,0);f.put(f.desc,12,127);f.put(f.desc,16,global+227);
    f.put(f.desc,0x34,global);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.put(f.rows[0],0,global+101);f.put(f.rows[0],0x30,2u);
    f.put(f.rows[1],0,global+0x68+100);f.put(f.rows[1],0x30,3u);
    f.put(f.rows[2],0,global+0x47+100);f.put(f.rows[2],4,4u);f.put(f.rows[2],8,1);
    f.put(f.tasks,0,99);f.put(f.actor,0x850,1);f.put(f.actor,0x858,1);
    ServerNpcScheduleResolutionBindings b{};b.context=&f;b.head=1;
    auto& sb=b.grunt.schedule;sb.context=&f;sb.conditions.context=&f;
    sb.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    sb.storage=[](void* p,std::uint32_t tok,const void** out,unsigned* n) noexcept -> int {
        auto& f=F::self(p);if(tok>=1&&tok<=3){*out=f.rows[tok-1];*n=sizeof(f.rows[0]);}
        else if(tok==4){*out=f.tasks;*n=8;}else return 0;return 1;};
    b.state.schedule=sb;
    b.translate=[](void* p,void*,int id,int* out) noexcept -> int{auto& f=F::self(p);++f.translations;*out=f.translated<0?id:f.translated;return 1;};
    b.component_schedule=[](void* p,std::uint32_t token,int,std::uint32_t* out) noexcept -> int{auto& f=F::self(p);assert(token==9);*out=f.component;return 1;};
    b.deactivate=[](void* p,void*,std::uint32_t token) noexcept -> int{auto& f=F::self(p);assert(token==9);++f.deactivated;f.put(f.actor,0xc18,0u);return 1;};
    b.fail_select=[](void*,void*,int sched,int task,int failure,int* out) noexcept -> int{assert(sched==0x47&&task==99&&failure==7);*out=0x68;return 1;};
    ServerNpcScheduleResolutionResult out{};int id=-1;
    assert(ns->to_global(f.actor,0x68,&sb.conditions,&id)&&id==global+204);
    assert(ns->to_local(f.actor,id,&sb.conditions,&id)&&id==0x68);
    assert(api->find(f.actor,sizeof(f.actor),0x68,&b,&out)&&out.token==2&&out.visited==2);
    assert(api->find(f.actor,sizeof(f.actor),120,&b,&out)&&out.token==1&&out.fallback&&out.visited==4);
    f.put(f.actor,0xc18,9u);f.component=3;
    assert(api->find(f.actor,sizeof(f.actor),0x68,&b,&out)&&out.token==3&&out.component_hit&&!out.visited);
    f.component=0;f.put(f.actor,0x7d4,1u<<6); // condition46
    assert(api->select(f.actor,sizeof(f.actor),10,false,&b,&out)&&out.token==2&&out.requested==0x68&&out.component_deactivated&&f.deactivated==1&&f.get<int>(0x79c)==global+204);
    f.put(f.actor,0x7d0,1u<<31); // condition3f
    assert(api->select(f.actor,sizeof(f.actor),10,false,&b,&out)&&out.token==3&&out.forced&&f.get<int>(0x79c)==global+204);
    f.put(f.actor,0x798,3u);f.put(f.actor,0x7b0,7);f.put(f.actor,0xc18,9u);
    assert(api->select(f.actor,sizeof(f.actor),10,true,&b,&out)&&out.token==2&&out.failed&&!out.component_deactivated&&f.get<unsigned>(0xc18)==9);
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS native schedule namespace, linked resolution, fallback and Grunt selection");
}
