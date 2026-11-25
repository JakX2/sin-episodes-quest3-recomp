#include "native_server_grunt_injury_task.h"
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
    unsigned char a[0xfcc]{},enemy[0xfcc]{},desc[0x48]{};
    sin_independent::EntityHandleEntry slots[2]{};
    std::vector<int> trace;bool player=true,npc=true,gate=false,path=true,found=true;unsigned caps=0;double angle=.006,dot=1;
    template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<unsigned char*>(p)+o,&v,sizeof(v));}
    template<class T>T get(void* p,unsigned o){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+o,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerGruntInjuryTaskApi>(GetProcAddress(s,server_grunt_injury_task_export));
    assert(g&&!GetProcAddress(c,server_grunt_injury_task_export)&&!g(0,sizeof(ServerGruntInjuryTaskApi)));
    const auto* api=g(1,sizeof(ServerGruntInjuryTaskApi));assert(api);
    F f;f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.slots[1].serial=7;f.slots[1].entity=reinterpret_cast<CBaseEntity*>(f.enemy);
    f.put(f.a,0x8a0,0x7001u);f.put(f.a,0x9c4,1u);
    f.put(f.enemy,0x9c,20);f.put(f.a,0x9c,35);
    ServerGruntInjuryTaskBindings b{};b.context=&f;b.combat.context=&f;auto& gb=b.combat.grunt;gb.context=&f;gb.entities={f.slots,2,nullptr,nullptr,nullptr};
    gb.lifecycle.conditions.context=&f;gb.lifecycle.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    gb.parent=[](void* p,void*,const ServerNpcScheduleTask*,bool) noexcept -> int{F::self(p).trace.push_back(1);return 1;};
    gb.cue=[](void* p,void*,unsigned slot,int x,int y) noexcept -> int{assert(slot==0x7ec&&x==-1&&y==2);F::self(p).trace.push_back(2);return 1;};
    b.cue_finish=[](void* p,void*,int x,int y) noexcept -> int{assert(x==2&&y==0);F::self(p).trace.push_back(3);return 1;};
    b.squad_flag=[](void* p,std::uint32_t s,unsigned v) noexcept -> int{assert(s==1&&v==0x20000000);F::self(p).trace.push_back(4);return 1;};
    b.extent=[](void* p,void*,unsigned* n) noexcept -> int{*n=sizeof(F::self(p).enemy);return 1;};
    b.is_player=[](void* p,void*,bool* out) noexcept -> int{*out=F::self(p).player;return 1;};
    b.npc_caps=[](void* p,void*,bool* npc,unsigned* caps) noexcept -> int{auto& f=F::self(p);*npc=f.npc;*caps=f.caps;return 1;};
    b.damage=[](void* p,void* a,void* e,float amount,int type,float lo,float hi) noexcept -> int{auto& f=F::self(p);assert(a==f.a&&e==f.enemy&&amount==35&&type==0&&lo==-.1f&&hi==.1f&&!f.a[0xcb1]&&f.get<unsigned>(e,0x980)==77);f.trace.push_back(5);return 1;};
    b.path_gate=[](void* p,void*,void*,bool* out) noexcept -> int{*out=F::self(p).gate;return 1;};
    b.target_path=[](void* p,void*,void*,bool* out) noexcept -> int{*out=F::self(p).path;return 1;};
    b.failure_token=[](void*,const char* s,int* out) noexcept -> int{assert(!std::strcmp(s,"Squadmate got up"));*out=12345;return 1;};
    gb.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(token==2);*out=f.enemy;*n=sizeof(f.enemy);return 1;};
    b.face_helper=[](void* p,void*,void* e) noexcept -> int{auto& f=F::self(p);assert(e==f.enemy);f.trace.push_back(6);return 1;};
    b.face_grenade=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(7);return 1;};
    b.yaw_error=[](void* p,void*,double* out) noexcept -> int{*out=F::self(p).angle;return 1;};
    b.dodge_probe=[](void* p,void*,void*,bool* out) noexcept -> int{*out=F::self(p).gate;return 1;};
    b.dodge_path=[](void* p,void*,bool* out) noexcept -> int{*out=F::self(p).path;return 1;};
    b.combat.squad_count=[](void*,std::uint32_t,int* out) noexcept -> int{*out=3;return 1;};
    gb.combat_target=[](void*,void* e,void** out) noexcept -> int{*out=e;return 1;};
    gb.target_gate=[](void*,void*,void*,bool* out) noexcept -> int{*out=true;return 1;};
    b.cover_gate=[](void*,void*,bool* out) noexcept -> int{*out=true;return 1;};
    b.cover_candidate=[](void* p,void*,bool* found,double* dot) noexcept -> int{auto& f=F::self(p);*found=f.found;*dot=f.dot;return 1;};
    b.combat.activity=[](void* p,void*,int id,bool restart) noexcept -> int{assert(!restart);F::self(p).trace.push_back(id);return 1;};
    ServerGruntInjuryTaskResult out{};
    auto run=[&](int id,bool running=false,float param=0.f,bool clear=true){
        if(clear)f.put(f.a,0x7d0,0u);f.put(f.a,0x7a4,1);f.trace.clear();
        ServerNpcScheduleTask task{id,param};assert(api->execute(f.a,sizeof(f.a),&task,running,10,&b,&out));};
    run(98);assert(out.cue_called&&out.combat.base.lifecycle.completed&&(f.trace==std::vector<int>{4,2,3}));
    f.a[0x98b]=0x20;f.caps=0x2000;run(98);assert(out.combat.base.parent_called&&f.trace==std::vector<int>{1});
    run(141);assert(out.activity==57);f.dot=0;run(141);assert(out.activity==56);f.found=false;run(141);assert(!out.combat.base.activity_requested&&!out.combat.base.lifecycle.completed);
    f.put(f.a,0xc3c,0x7001u);f.put(f.a,0xc40,77u);f.a[0xcb1]=1;run(144);assert(out.attacker_live&&out.damage_called&&out.combat.base.lifecycle.completed);
    run(149);assert(out.failure==11&&out.combat.base.lifecycle.failed);
    f.put(f.a,0x8a8,0x7001u);run(149);assert(out.path_called&&out.combat.base.lifecycle.completed);
    f.path=false;run(149);assert(out.failure==11&&out.combat.base.lifecycle.failed);
    run(150);assert(out.failure==12345&&out.combat.base.lifecycle.failed);
    f.put(f.a,0xca4,2u);run(150);assert(out.combat.base.lifecycle.completed&&f.trace==std::vector<int>{6});
    f.put(f.a,0x7d0,2u);run(139,true,0,false);assert(!out.forced_completion);f.angle=.0059;run(139,true,0,false);assert(out.forced_completion&&out.combat.base.lifecycle.completed);
    f.path=true;run(152);assert(out.failure==8&&!out.combat.base.lifecycle.completed&&out.path_called==1);
    f.gate=true;run(152);assert(!out.failure&&out.combat.base.lifecycle.completed);
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS native Grunt injury, helper, facing and task-failure routes");
}
