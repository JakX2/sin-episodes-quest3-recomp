#include "native_server_npc_class_schedule_translation.h"
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
    alignas(4) unsigned char actor[0x1088]{},desc[0x48]{},enemy[0x90]{},weapon[4]{};
    sin_independent::EntityHandleEntry slots[3]{};
    bool gate=false,slot=false,rules=false,crouch=false;int random=0;double distance=256;
    unsigned notices=0,cues=0,pending=0;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(unsigned off){T v{};std::memcpy(&v,actor+off,sizeof(v));return v;}
    void set(int id){unsigned off=0x7cc+4*(id/32);put(actor,off,get<unsigned>(off)|(1u<<(id%32)));}
    void clear(){std::memset(actor+0x7cc,0,32);}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerNpcClassScheduleTranslationApi>(GetProcAddress(s,server_npc_class_schedule_translation_export));
    assert(g&&!GetProcAddress(c,server_npc_class_schedule_translation_export)&&!g(0,sizeof(ServerNpcClassScheduleTranslationApi)));
    const auto* api=g(1,sizeof(ServerNpcClassScheduleTranslationApi));assert(api);
    F f;f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.slots[1].serial=7;f.slots[1].entity=reinterpret_cast<CBaseEntity*>(f.enemy);
    f.slots[2].serial=8;f.slots[2].entity=reinterpret_cast<CBaseEntity*>(f.weapon);
    ServerNpcClassScheduleTranslationBindings b{};b.context=&f;b.entities={f.slots,3,nullptr,nullptr,nullptr};
    b.conditions.context=&f;b.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    b.parent=[](void*,void*,int id,int* out) noexcept -> int{*out=id;return 1;};
    b.extent=[](void* p,void* e,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(e==f.enemy);*n=sizeof(f.enemy);return 1;};
    b.rules_int=[](void* p,int id,bool* out) noexcept -> int{assert(id==3);*out=F::self(p).rules;return 1;};
    b.rules_float=[](void* p,float x,bool* out) noexcept -> int{assert(x==20.f);*out=F::self(p).rules;return 1;};
    b.random=[](void* p,int,int,int* out) noexcept -> int{*out=F::self(p).random;return 1;};
    b.actor_gate=[](void* p,void*,unsigned,int,bool* out) noexcept -> int{*out=F::self(p).gate;return 1;};
    b.squad_slot=[](void* p,void*,int,bool* out) noexcept -> int{*out=F::self(p).slot;return 1;};
    b.cue=[](void* p,void*,unsigned v,int x,int y) noexcept -> int{assert(v==0x7e8&&x==0&&y==2);++F::self(p).cues;return 1;};
    b.enemy_notice=[](void* p,void*,void*) noexcept -> int{++F::self(p).notices;return 1;};
    b.enemy_capability=[](void*,void*,bool* npc,unsigned* caps) noexcept -> int{*npc=true;*caps=0;return 1;};
    b.weapon_capability=[](void*,void*,unsigned* caps) noexcept -> int{*caps=0x4000;return 1;};
    b.is_player=[](void*,void*,bool* out) noexcept -> int{*out=true;return 1;};
    b.distance=[](void* p,void*,void*,bool,double* out) noexcept -> int{*out=F::self(p).distance;return 1;};
    b.crouch_probe=[](void* p,void*,void*,void*,bool* out) noexcept -> int{*out=F::self(p).crouch;return 1;};
    b.burst=[](void*,void*,int* out) noexcept -> int{*out=3;return 1;};
    b.interval=[](void*,void*,double* out) noexcept -> int{*out=.075f;return 1;};
    b.pending=[](void* p,void*,int id) noexcept -> int{assert(id==0xc4);++F::self(p).pending;return 1;};
    ServerNpcClassScheduleTranslationResult out{};
    unsigned v=0x2249aaa4;
    auto choose=[&](int input,int expected){assert(api->translate(f.actor,sizeof(f.actor),v,input,10,&b,&out)&&out.output==expected);};
    choose(0x12,0x52);choose(0x1a,0x5e);choose(0x19,0x5d);
    f.random=1;f.set(0x15);choose(0x19,0x60);
    f.put(f.actor,0x9c4,1u);f.rules=f.slot=true;choose(0x19,0x61);assert(f.cues==1);
    f.clear();choose(0x62,0x4f);f.put(f.actor,0x8a0,0x7001u);choose(0x62,0x51);assert(out.steps==2);
    choose(0x24,0x67);assert(f.notices==1);
    choose(0x57,0x58);assert(f.actor[0xc64]);f.gate=true;choose(0x21,0x6a);
    f.random=19;choose(0x30,0x63);f.random=20;choose(0x30,0x54);f.rules=false;choose(0x30,0x31);
    f.gate=false;choose(0x29,0x5b);assert(f.actor[0xc64]);
    f.set(3);choose(0x29,0x31);f.clear();f.put(f.actor,0x78c,0x8002u);f.actor[0xc66]=1;
    f.distance=257;f.crouch=true;choose(0x29,0x5b);assert(!f.actor[0xc64]);
    choose(0x2a,0x2a);f.actor[0xc67]=1;f.distance=256;choose(0x50,0x56);assert(!f.actor[0xc67]);
    f.actor[0x1f0]=1;f.put(f.actor,0x878,0xc4);choose(0x6b,0x6b);
    assert(f.actor[0xca0]&&f.actor[0x200]&&f.get<int>(0xc98)==3&&f.get<float>(0xc9c)==.075f&&f.get<int>(0x878)==0&&f.pending==1);
    v=0x2248df04;choose(0x19,0x76);choose(0x27,0x77);choose(0x54,0x31);choose(0x12,0x12);assert(!out.parent_calls);
    v=0x2249964c;choose(0x29,0x7a);choose(0x58,2);choose(0x70,0x7b);choose(0x30,0x31);
    choose(0x17,0x17);f.actor[0x1084]=1;choose(0x17,0x76);
    f.set(0x14);f.gate=true;choose(0x62,0x7a);assert(out.steps==2);
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS original class schedule translation and recursive virtual dispatch");
}
