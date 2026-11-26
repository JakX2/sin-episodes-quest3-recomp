#include "native_server_grunt_schedule_selector.h"
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
    alignas(4) unsigned char actor[0xdd8]{},helper[0xca8]{},desc[0x48]{},schedule[4]{},grenade[4]{};
    sin_independent::EntityHandleEntry slots[2]{};
    unsigned char squad_group[0x78]{},squad_rows[32]{};
    ServerGruntScheduleSound sound{};
    unsigned cues=0;int cue=-1,relation=3;bool kick=false,investigate=false,squad=false;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
    void set(int id){const auto off=0x7cc+4*(id/32);put(actor,off,get<unsigned>(actor,off)|(1u<<(id%32)));}
    void clear(){std::memset(actor+0x7cc,0,32);}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerGruntScheduleSelectorApi>(GetProcAddress(s,server_grunt_schedule_selector_export));
    assert(g&&!GetProcAddress(c,server_grunt_schedule_selector_export)&&!g(0,sizeof(ServerGruntScheduleSelectorApi))&&!g(1,0));
    const auto* api=g(1,sizeof(ServerGruntScheduleSelectorApi));assert(api);
    F f;f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.put(f.desc,4,npc_global_condition_base);f.put(f.desc,8,0);f.put(f.desc,12,127);
    f.slots[1].serial=7;f.slots[1].entity=reinterpret_cast<CBaseEntity*>(f.grenade);
    ServerGruntScheduleSelectorBindings b{};b.context=&f;b.schedule.context=&f;b.schedule.conditions.context=&f;
    b.schedule.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    b.schedule.storage=[](void* p,std::uint32_t token,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);if(token!=1)return 0;*out=f.schedule;*n=4;return 1;};
    b.entities={f.slots,2,nullptr,nullptr,nullptr};
    b.sound=[](void* p,void*,unsigned mask,ServerGruntScheduleSound* out) noexcept -> int{assert(mask==0xfff1f);*out=F::self(p).sound;return 1;};
    b.grenade_owner=[](void* p,void* e,void** out) noexcept -> int{auto& f=F::self(p);assert(e==f.grenade);*out=f.helper;return 1;};
    b.relation=[](void* p,void*,void*,int* out) noexcept -> int{*out=F::self(p).relation;return 1;};
    b.kick_eligible=[](void* p,void*,bool* out) noexcept -> int{*out=F::self(p).kick;return 1;};
    b.cue=[](void* p,void*,unsigned cue,int a,int b) noexcept -> int{auto& f=F::self(p);assert(a==0&&b==1);++f.cues;f.cue=cue;return 1;};
    b.mutable_storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(token==2);*out=f.helper;*n=sizeof(f.helper);return 1;};
    b.parent=[](void*,void*,int* out) noexcept -> int{*out=99;return 1;};
    b.combat_special=[](void*,void*,bool* out) noexcept -> int{*out=true;return 1;};
    b.investigate=[](void* p,void*,bool* out) noexcept -> int{*out=F::self(p).investigate;return 1;};
    b.squad_gate=[](void* p,void*,int v,bool* out) noexcept -> int{assert(v==4);*out=F::self(p).squad;return 1;};
    ServerGruntScheduleSelectorResult out{};auto choose=[&](int expected){assert(api->select(f.actor,sizeof(f.actor),&b,&out)&&out.schedule==expected);};
    f.actor[0xb6]=2;f.set(0x46);f.set(0x4c);f.set(0x48);choose(0x6f);assert(f.actor[0xcb0]);choose(0x68);
    f.clear();f.set(0x4c);f.set(0x48);choose(0x73);assert(!out.sound_queried&&!out.kick_transferred);
    f.clear();f.set(0x48);choose(0x75);
    f.clear();f.set(0x38);choose(0x42);f.clear();f.set(0x37);choose(0x6d);
    f.put(f.actor,0x850,6);choose(99);assert(out.parent_called);
    f.put(f.actor,0x850,4);f.set(0x4a);choose(0x6c);
    f.clear();f.put(f.actor,0xca4,1u);choose(0x6c);f.put(f.actor,0xca4,0u);
    f.set(0x49);f.set(0x47);f.put(f.actor,0xca8,2u);f.put(f.helper,0xca4,9u);choose(0x71);
    assert(out.condition_clears==2&&out.helper_cleared&&!f.get<unsigned>(f.actor,0xca8)&&!f.get<unsigned>(f.helper,0xca4)&&f.get<unsigned>(f.helper,0x8a8)==0xffffffffu);
    choose(99);f.set(0x47);choose(99);f.put(f.actor,0x850,1);choose(0x6b);
    f.clear();f.set(0x4b);choose(0x6e);f.clear();f.actor[0xc65]=1;choose(0x64);
    f.actor[0xc65]=0;f.set(0x44);choose(0x64);f.put(f.actor,0xdd4,1u);choose(99);
    f.clear();f.put(f.actor,0x850,3);choose(0x55);
    f.put(f.actor,0x850,2);f.set(0x33);f.put(f.actor,0x9c4,1u);f.squad=true;choose(0xa);
    auto ng=reinterpret_cast<GetServerGruntNativeSlotSelectorApi>(GetProcAddress(s,server_grunt_native_slot_selector_export));
    assert(ng&&!GetProcAddress(c,server_grunt_native_slot_selector_export)&&!ng(0,sizeof(ServerGruntNativeSlotSelectorApi))&&!ng(1,0));
    const auto* native=ng(1,sizeof(ServerGruntNativeSlotSelectorApi));assert(native);
    ServerNpcGruntSquadSlotsBindings sb{};sb.context=sb.storage.context=&f;
    sb.storage.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);if(token==1){*out=f.squad_group;*n=sizeof(f.squad_group);return 1;}if(token==100){*out=f.squad_rows;*n=sizeof(f.squad_rows);return 1;}return 0;};
    sb.resize=[](void* p,std::uint32_t old,unsigned cap,std::uint32_t* token,void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(!old&&cap==4);*token=100;*out=f.squad_rows;*n=sizeof(f.squad_rows);return 1;};
    auto legacy=b.squad_gate;b.squad_gate=nullptr;f.put(f.actor,0x8a0,0xffffffffu);f.put(f.actor,0x9cc,-1);
    ServerGruntNativeSlotSelectorResult ns{};
    assert(native->select(f.actor,sizeof(f.actor),10,&b,&sb,&ns)&&ns.selection.schedule==0xa&&ns.slot_calls==1&&ns.slots.new_set&&ns.slots.bitmap==(1u<<4));
    f.put(f.actor,0x9cc,-1);
    assert(native->select(f.actor,sizeof(f.actor),10,&b,&sb,&ns)&&ns.selection.schedule==99&&ns.selection.parent_called&&ns.slot_calls==1&&!ns.slots.allowed);
    b.squad_gate=legacy;
    f.clear();f.set(0x30);f.sound={1,8,0x7001};f.kick=true;choose(0x73);
    assert(out.kick_transferred&&f.get<unsigned>(f.actor,0xcb4)==0x7001&&f.get<unsigned>(f.actor,0xcb8)==0xffffffffu&&f.get<unsigned>(f.actor,0x8a8)==0x7001);
    f.kick=false;f.relation=2;choose(0x5e);assert(f.cues==1&&f.cue==1);
    f.put(f.actor,0x798,1u);f.put(f.actor,0x79c,npc_global_condition_base+0x5e);choose(0x5f);
    assert(f.cues==2);
    f.clear();f.actor[0x988]=0x40;f.set(0x11);f.investigate=true;choose(0x1c);
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS native ground Grunt schedule selector priority and state transitions");
}
