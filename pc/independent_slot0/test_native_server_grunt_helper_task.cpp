#include "native_server_grunt_helper_task.h"
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
struct F {
    unsigned char a[0xfcc]{},peer[0xfcc]{},other[0xfcc]{},desc[0x48]{};
    sin_independent::EntityHandleEntry slots[3]{};
    ServerGruntHelperCandidate rows[2]{{peer,sizeof(peer),2},{other,sizeof(other),3}};
    float target=20;double d1=100,d2=1100;bool gate=false,success=true;int path_status=1,activity=-1;
    unsigned stopped=0,followed=0,fallback=0,nearest_calls=0;
    template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<unsigned char*>(p)+o,&v,sizeof(v));}
    template<class T>T get(void* p,unsigned o){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+o,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerGruntHelperTaskApi>(GetProcAddress(s,server_grunt_helper_task_export));
    assert(g&&!GetProcAddress(c,server_grunt_helper_task_export)&&!g(0,sizeof(ServerGruntHelperTaskApi)));
    const auto* api=g(1,sizeof(ServerGruntHelperTaskApi));assert(api);
    F f;f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.slots[1].serial=7;f.slots[1].entity=reinterpret_cast<CBaseEntity*>(f.peer);
    f.slots[2].serial=8;f.slots[2].entity=reinterpret_cast<CBaseEntity*>(f.a);
    f.put(f.a,0x8a0,0x7001u);
    ServerGruntHelperTaskBindings b{};b.context=&f;b.injury.context=&f;auto& gb=b.injury.combat.grunt;
    gb.entities={f.slots,3,nullptr,nullptr,nullptr};gb.lifecycle.conditions.context=&f;
    gb.lifecycle.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    b.origin=[](void* p,void* e,float* v) noexcept -> int{auto& f=F::self(p);v[0]=e==f.a?0.f:f.target;v[1]=v[2]=0;return 1;};
    b.distance=[](void* p,void*,void* e,double* out) noexcept -> int{auto& f=F::self(p);*out=e==f.peer?f.d1:f.d2;return 1;};
    b.injury.path_gate=[](void* p,void*,void*,bool* out) noexcept -> int{*out=F::self(p).gate;return 1;};
    b.path_start=[](void* p,void*,void*,bool* out) noexcept -> int{*out=F::self(p).success;return 1;};
    b.path_fallback=[](void* p,void*,void*) noexcept -> int{++F::self(p).fallback;return 1;};
    b.movement_activity=[](void* p,void*,int* out) noexcept -> int{*out=F::self(p).activity;return 1;};
    b.path_activity=[](void* p,void*,int id) noexcept -> int{assert(id==(F::self(p).activity==-1?6:10));return 1;};
    b.path_status=[](void* p,void*,int* out) noexcept -> int{*out=F::self(p).path_status;return 1;};
    b.path_stop=[](void* p,void*,bool running) noexcept -> int{F::self(p).stopped+=running?2:1;return 1;};
    b.path_follow=[](void*,void*,bool* out) noexcept -> int{*out=true;return 1;};
    b.path_follow_update=[](void* p,void*,bool yes) noexcept -> int{assert(yes);++F::self(p).followed;return 1;};
    b.path_tolerance=[](void*,void*,float v) noexcept -> int{assert(v==10.f);return 1;};
    b.path_rebuild=[](void* p,void*,bool* out) noexcept -> int{*out=F::self(p).success;return 1;};
    b.squad_candidates=[](void* p,std::uint32_t squad,const ServerGruntHelperCandidate** rows,unsigned* n) noexcept -> int{assert(squad==1);auto& f=F::self(p);*rows=f.rows;*n=2;return 1;};
    b.nearest=[](void* p,std::uint32_t,void*,ServerGruntHelperCandidate* out) noexcept -> int{auto& f=F::self(p);++f.nearest_calls;*out=f.rows[0];return 1;};
    b.self_identity=[](void*,void*,std::uint32_t* token,std::uint32_t* handle) noexcept -> int{*token=1;*handle=0x8002;return 1;};
    ServerGruntHelperTaskResult out{};
    auto run=[&](int id,bool running=false){
        f.put(f.a,0x7a4,1);f.put(f.a,0x7d0,0u);
        ServerNpcScheduleTask task{id,10.f};assert(api->execute(f.a,sizeof(f.a),&task,running,10,&b,&out));};
    f.target=10;run(143);assert(out.injury.combat.base.lifecycle.completed&&!out.path_started);
    f.target=20;f.put(f.a,0x988,3u);run(143);
    assert(out.path_started&&out.origin_saved&&f.get<unsigned>(f.a,0x988)==1&&f.get<float>(f.a,0x9f0)==0.f&&f.get<float>(f.a,0xa00)==20.f);
    f.target=29;run(143,true);assert(!out.path_rebuilt);
    f.target=30;run(143,true);assert(out.path_rebuilt&&f.get<float>(f.a,0xa00)==30.f);
    f.target=10;run(143,true);assert(out.path_stopped&&f.stopped==2&&out.injury.combat.base.lifecycle.completed);
    f.target=20;f.success=false;run(143);assert(out.failure==11&&f.fallback==1);
    f.success=true;f.path_status=0;run(143);assert(out.path_stopped&&f.stopped==3&&out.injury.combat.base.lifecycle.completed);
    f.put(f.a,0x8a0,0xffffffffu);run(143,true);assert(out.failure==6);
    f.put(f.a,0x9c4,1u);f.d1=1200;run(151);assert(!out.helper_assigned);
    f.d1=100;f.peer[0xca0]=1;f.other[0xca0]=1;run(151);assert(!out.helper_assigned&&f.nearest_calls==1);
    f.other[0xca0]=0;run(151);assert(out.helper_assigned&&out.candidates==2&&f.a[0xca0]&&f.get<unsigned>(f.a,0xca8)==3&&f.get<unsigned>(f.other,0xca4)==1&&f.get<unsigned>(f.other,0x8a8)==0x8002&&(f.get<unsigned>(f.other,0x7d4)&(1u<<10)));
    f.peer[0xca0]=0;f.put(f.a,0xca8,0u);run(151);assert(out.helper_assigned&&!out.candidates&&f.get<unsigned>(f.a,0xca8)==2);
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS native Grunt move-to-enemy and helper recruitment decisions");
}
