#include "native_server_elite_task.h"
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
    alignas(4) unsigned char actor[0xfe0]{},weapon[0x598]{},desc[0x48]{},info[0x168]{},target[4]{};
    sin_independent::EntityHandleEntry slots[3]{};
    std::vector<int> trace;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerEliteTaskApi>(GetProcAddress(s,server_elite_task_export));
    assert(g&&!GetProcAddress(c,server_elite_task_export)&&!g(0,sizeof(ServerEliteTaskApi)));
    const auto* api=g(1,sizeof(ServerEliteTaskApi));assert(api);
    F f;f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.slots[1].serial=7;f.slots[1].entity=reinterpret_cast<CBaseEntity*>(f.weapon);
    f.slots[2].serial=8;f.slots[2].entity=reinterpret_cast<CBaseEntity*>(f.target);
    f.put(f.actor,0xfdc,0x7001u);f.put(f.actor,0x78c,0x7001u);f.put(f.actor,0x8a0,0x8002u);
    ServerEliteTaskBindings b{};b.context=&f;b.grunt.context=&f;b.grunt.entities={f.slots,3,&f,[](void*,void*,unsigned* out) noexcept -> int{*out=WeaponActivityChaingun;return 1;},nullptr};
    b.grunt.lifecycle.conditions.context=&f;b.grunt.lifecycle.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    b.grunt.special=[](void* p,void*,const ServerNpcScheduleTask* t,bool) noexcept -> int{F::self(p).trace.push_back(t->id);return 1;};
    b.grunt.combat_target=[](void*,void* e,void** out) noexcept -> int{*out=e;return 1;};
    b.grunt.target_gate=[](void*,void*,void*,bool* out) noexcept -> int{*out=false;return 1;};
    b.grunt.cue=[](void* p,void*,unsigned slot,int x,int y) noexcept -> int{assert(slot==0x7cc&&x==2&&y==2);F::self(p).trace.push_back(90);return 1;};
    b.is_player=[](void*,void*,bool* out) noexcept -> int{*out=true;return 1;};
    b.idle=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(91);return 1;};
    b.stop_sound=[](void* p,void*,int id) noexcept -> int{assert(id==13);F::self(p).trace.push_back(-id);return 1;};
    auto& w=b.reload.original.weapon;w.context=&f;w.extent=[](void* p,void* w,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(w==f.weapon);*n=sizeof(f.weapon);return 1;};
    w.sound.context=&f;w.sound.emit=[](void* p,void*,int id,float) noexcept -> int{F::self(p).trace.push_back(id);return 1;};
    f.put(f.info,0x160,30);f.put(f.weapon,0x51c,std::uint16_t{0xffff});b.reload.scripts={nullptr,0,0,f.info,sizeof(f.info),nullptr,nullptr};
    ServerEliteTaskResult out{};
    auto run=[&](int id,bool running,float now,float param=0.f){f.put(f.actor,0x7a4,1);f.trace.clear();ServerNpcScheduleTask t{id,param};assert(api->execute(f.actor,sizeof(f.actor),&t,running,now,&b,&out));};
    run(0x43,false,10);assert(out.spin_changed&&!out.spin_ready&&!out.delegated&&f.get<int>(f.weapon,0x594)==1&&f.trace==std::vector<int>{13});
    assert(api->spin_update(f.weapon,sizeof(f.weapon),10.99f,&b,&out)&&!out.spin_changed);
    assert(api->spin_update(f.weapon,sizeof(f.weapon),11.f,&b,&out)&&out.spin_changed&&f.get<int>(f.weapon,0x594)==2);
    run(0x43,true,11);assert(out.lifecycle.completed);
    run(0x43,false,11);assert(out.spin_ready&&out.delegated&&f.actor[0xfd9]);
    run(0x47,false,11);assert(out.timer_written&&f.get<float>(f.actor,0x9dc)==12.f&&f.get<int>(f.weapon,0x594)==3);
    run(0x47,true,11.5f);assert(!out.lifecycle.completed);
    f.put(f.actor,0x7cc,56u);f.put(f.weapon,0x4f0,9);
    run(0x47,true,12);assert(out.lifecycle.completed&&out.reload.clip_changed&&out.reload.conditions_cleared==3&&f.get<int>(f.weapon,0x4ec)==30&&f.get<int>(f.weapon,0x4f0)==9);
    f.trace.clear();assert(api->spin_update(f.weapon,sizeof(f.weapon),12,&b,&out)&&out.spin_changed&&f.get<int>(f.weapon,0x594)==0&&(f.trace==std::vector<int>{-13,14}));
    run(3,false,10,1.9f);assert(out.cue_called&&out.idle_called&&f.get<float>(f.actor,0x9dc)==12.f&&(f.trace==std::vector<int>{90,91}));
    run(3,false,13,1.f);assert(out.lifecycle.completed&&!out.cue_called);
    run(3,false,14,2.f);assert(!out.lifecycle.completed&&!out.cue_called);

    auto gn=reinterpret_cast<GetServerEliteNativeTaskApi>(GetProcAddress(s,server_elite_native_task_export));
    assert(gn&&!GetProcAddress(c,server_elite_native_task_export)&&!gn(0,sizeof(ServerEliteNativeTaskApi))&&!gn(1,0));
    const auto* native=gn(1,sizeof(ServerEliteNativeTaskApi));assert(native);
    ServerGruntCombatTaskBindings combat{};combat.context=&f;combat.weapon.context=&f;
    combat.weapon.random=[](void* p,int lo,int hi,int* out) noexcept -> int{assert(lo==1&&hi==3);F::self(p).trace.push_back(100);*out=2;return 1;};
    combat.activity=[](void* p,void* a,int id,bool restart) noexcept -> int{
        auto& f=F::self(p);assert(a==f.actor&&id==16&&restart&&f.actor[0xfd9]);f.trace.push_back(101);return 1;};
    b.grunt.special=nullptr;
    b.grunt.parent=[](void* p,void*,const ServerNpcScheduleTask* t,bool running) noexcept -> int{
        assert(t->id==67&&running);F::self(p).trace.push_back(102);return 1;};
    f.put(f.actor,0x6cc,0x7001u);
    f.put(f.weapon,0x594,2);f.put(f.actor,0x7a4,1);f.trace.clear();
    ServerNpcScheduleTask fire{67,0};ServerGruntCombatSchedulerStats stats{};
    assert(native->execute(f.actor,sizeof(f.actor),&fire,false,20,&b,&combat,&out,&stats));
    assert(out.spin_ready&&out.delegated&&f.actor[0xfd9]&&stats.calls==1&&stats.last.burst==2&&f.get<int>(f.actor,0xc98)==2&&(f.trace==std::vector<int>{100,101}));
    f.trace.clear();
    assert(native->execute(f.actor,sizeof(f.actor),&fire,true,20,&b,&combat,&out,&stats)&&stats.calls==2&&out.grunt.parent_called&&f.trace==std::vector<int>{102});
    f.put(f.weapon,0x594,1);f.actor[0xfd9]=0;f.trace.clear();
    assert(native->execute(f.actor,sizeof(f.actor),&fire,false,21,&b,&combat,&out,&stats)&&!out.delegated&&stats.calls==2&&f.trace.empty());
    f.put(f.weapon,0x594,2);
    assert(native->execute(f.actor,sizeof(f.actor),&fire,true,22,&b,&combat,&out,&stats)&&out.lifecycle.completed&&!out.delegated&&stats.calls==2);
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS original Elite task and Chaingun state transitions");
}
