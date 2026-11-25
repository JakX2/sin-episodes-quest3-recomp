#include "native_server_grunt_combat_task.h"
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
    unsigned char a[0xfcc]{},desc[0x48]{},weapon[4]{},enemy[4]{};
    sin_independent::EntityHandleEntry slots[3]{};
    unsigned kind=WeaponActivitySmg;float stamp=0;double distance=512;bool aimed=true;std::vector<int> trace;
    template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<unsigned char*>(p)+o,&v,sizeof(v));}
    template<class T>T get(unsigned o){T v{};std::memcpy(&v,a+o,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerGruntCombatTaskApi>(GetProcAddress(s,server_grunt_combat_task_export));
    assert(g&&!GetProcAddress(c,server_grunt_combat_task_export)&&!g(0,sizeof(ServerGruntCombatTaskApi)));
    const auto* api=g(1,sizeof(ServerGruntCombatTaskApi));assert(api);
    F f;f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.slots[1].serial=7;f.slots[1].entity=reinterpret_cast<CBaseEntity*>(f.weapon);
    f.slots[2].serial=8;f.slots[2].entity=reinterpret_cast<CBaseEntity*>(f.enemy);
    f.put(f.a,0x78c,0x7001u);f.put(f.a,0x6cc,0x7001u);f.put(f.a,0x8a0,0x8002u);
    f.a[0x1f0]=1;
    ServerGruntCombatTaskBindings b{};b.context=&f;b.grunt.context=&f;b.weapon.context=&f;b.shared_smg_stamp=&f.stamp;
    b.grunt.entities={f.slots,3,&f,[](void* p,void*,unsigned* out) noexcept -> int{*out=F::self(p).kind;return 1;},nullptr};
    b.grunt.lifecycle.conditions.context=&f;b.grunt.lifecycle.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    b.weapon.random=[](void* p,int lo,int hi,int* out) noexcept -> int{assert(lo==4&&hi==7);F::self(p).trace.push_back(100);*out=4;return 1;};
    b.activity=[](void* p,void*,int id,bool restart) noexcept -> int{auto& f=F::self(p);if(restart)assert(f.get<int>(0x878)!=id);f.trace.push_back(id);return 1;};
    b.grunt.cue=[](void* p,void*,unsigned slot,int,int) noexcept -> int{F::self(p).trace.push_back(slot);return 1;};
    b.grunt.combat_target=[](void*,void* e,void** out) noexcept -> int{*out=e;return 1;};
    b.is_player=[](void*,void*,bool* out) noexcept -> int{*out=true;return 1;};
    b.grunt.target_gate=[](void*,void*,void*,bool* out) noexcept -> int{*out=false;return 1;};
    b.squad_count=[](void*,std::uint32_t s,int* out) noexcept -> int{assert(s==1);*out=3;return 1;};
    b.distance=[](void* p,void*,void*,double* out) noexcept -> int{*out=F::self(p).distance;return 1;};
    b.aim_query=[](void* p,void*,bool* out) noexcept -> int{*out=F::self(p).aimed;return 1;};
    b.aim_update=[](void* p,void*,bool yes) noexcept -> int{F::self(p).trace.push_back(yes?201:200);return 1;};
    b.grunt.turn_update=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(202);return 1;};
    b.grunt.parent=[](void* p,void*,const ServerNpcScheduleTask*,bool) noexcept -> int{F::self(p).trace.push_back(300);return 1;};
    ServerGruntCombatTaskResult o{};
    auto run=[&](int id,bool running,float now,float parameter=0.f){f.put(f.a,0x7a4,1);f.trace.clear();ServerNpcScheduleTask task{id,parameter};assert(api->execute(f.a,sizeof(f.a),&task,running,now,&b,&o));};
    run(3,false,10,1);assert(o.activity==61&&f.get<float>(0x9dc)==12.f&&(f.trace==std::vector<int>{0x7cc,61}));
    run(3,false,13,1);assert(o.base.lifecycle.completed);
    run(3,false,14,2);assert(o.activity==1&&f.get<float>(0x9dc)==16.f&&(f.trace==std::vector<int>{0x7e8,1}));
    f.put(f.a,0x878,16);run(67,false,20);assert(o.weapon_live&&o.burst==4&&f.get<int>(0xc98)==4&&f.get<float>(0x60c)==20.f&&f.a[0x200]&&(f.trace==std::vector<int>{100,16}));
    f.a[0x3e0]=1;f.put(f.a,0x3ec,7);f.put(f.a,0x884,7);
    run(1,true,20);assert(f.get<int>(0xc98)==3&&o.activity==16&&(f.trace==std::vector<int>{202,201,16}));
    f.put(f.a,0xc98,1);run(1,true,21);assert(o.base.lifecycle.completed&&f.get<int>(0xc98)==0);
    run(146,false,22);assert(o.activity==196);f.put(f.a,0xc98,0);run(146,true,23);assert(o.base.lifecycle.completed&&f.get<int>(0xc98)==-1&&o.aim_queries==2);
    f.put(f.a,0x8a0,0xffffffffu);run(146,true,24);assert(o.activity==196&&o.base.lifecycle.completed);
    f.put(f.a,0x8a0,0x8002u);f.put(f.a,0x9c4,1u);run(140,false,25,.5f);assert(!o.deadline_changed&&o.base.lifecycle.completed);
    f.distance=513;run(140,false,25,.5f);assert(o.deadline_changed&&f.get<float>(0x60c)==25.5f);
    run(67,true,30);assert(f.stamp==30&&o.shared_changed&&!o.base.lifecycle.completed);
    run(67,true,30.5f);assert(f.stamp==30&&!o.shared_changed);
    run(67,true,31);assert(f.stamp==0&&o.base.lifecycle.completed);
    f.kind=WeaponActivityAssault;run(67,true,32);assert(o.base.parent_called&&f.trace==std::vector<int>{300});
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS original Grunt task timers, burst control, and shared SMG latch");
}
