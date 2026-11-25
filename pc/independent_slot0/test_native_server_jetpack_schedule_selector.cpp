#include "native_server_jetpack_schedule_selector.h"
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
    alignas(4) unsigned char actor[0x1028]{},desc[0x48]{},enemy[0x90]{},attachment[4]{},target[4]{};
    sin_independent::EntityHandleEntry slots[4]{};
    int combat=0;bool investigate=false;unsigned sends=0;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    void set(int id){unsigned v=0,off=0x7cc+4*(id/32);std::memcpy(&v,actor+off,4);put(actor,off,v|(1u<<(id%32)));}
    void clear(){std::memset(actor+0x7cc,0,32);}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerJetpackSelectorApi>(GetProcAddress(s,server_jetpack_schedule_selector_export));
    assert(g&&!GetProcAddress(c,server_jetpack_schedule_selector_export)&&!g(0,sizeof(ServerJetpackSelectorApi)));
    const auto* api=g(1,sizeof(ServerJetpackSelectorApi));assert(api);
    F f;f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.slots[1].serial=7;f.slots[1].entity=reinterpret_cast<CBaseEntity*>(f.enemy);
    f.slots[2].serial=8;f.slots[2].entity=reinterpret_cast<CBaseEntity*>(f.attachment);
    f.slots[3].serial=9;f.slots[3].entity=reinterpret_cast<CBaseEntity*>(f.target);
    f.put(f.actor,0x8a0,0x7001u);f.put(f.actor,0x1024,0x8002u);
    ServerJetpackSelectorBindings b{};b.context=&f;b.grunt.context=&f;b.grunt.entities={f.slots,4,nullptr,nullptr,nullptr};
    b.grunt.schedule.conditions.context=&f;b.grunt.schedule.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    b.extent=[](void* p,void* e,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(e==f.enemy);*n=sizeof(f.enemy);return 1;};
    b.attachment_enemy=[](void* p,void* a,void* e,bool v) noexcept -> int{auto& f=F::self(p);assert(a==f.attachment&&e==f.enemy&&v);++f.sends;return 1;};
    b.grunt.combat_schedule=[](void* p,void*,int* id) noexcept -> int{*id=F::self(p).combat;return 1;};
    b.grunt.investigate=[](void* p,void*,bool* out) noexcept -> int{*out=F::self(p).investigate;return 1;};
    b.grunt.parent=[](void*,void*,int* id) noexcept -> int{*id=99;return 1;};
    ServerJetpackSelectorResult out{};auto choose=[&](int id,float now=10.f){assert(api->select(f.actor,sizeof(f.actor),now,&b,&out)&&out.schedule==id);};
    f.set(0x46);choose(0x69);assert(!f.sends);
    f.clear();f.set(0xb);f.set(0x4f);choose(0x76);assert(out.enemy_sent&&f.sends==1);
    f.clear();f.set(0x4f);choose(0x77);
    f.clear();f.put(f.actor,0x60c,11.f);choose(0x29);f.combat=0x55;choose(0x55,11.f);
    f.set(0x16);choose(0x29,11.f);
    f.clear();f.put(f.enemy,0x8c,1);f.put(f.actor,0x8a8,0x9003u);choose(0x29);
    f.set(0x4f);choose(0x78);
    f.clear();f.put(f.actor,0x8a8,0xa003u);f.put(f.actor,0x100c,1.f);choose(0x76);assert(out.inspection);
    f.put(f.actor,0xfec,2);f.put(f.actor,0x850,3);choose(0x55);
    f.combat=0;choose(0x76);
    f.put(f.actor,0x850,2);f.set(0x33);choose(0xa);
    f.set(0x10);f.investigate=true;choose(0x1c);
    f.clear();f.put(f.actor,0x850,4);f.set(0x4a);choose(0x6c);assert(out.ground);
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS native Jetpack selector priority, inspection and enemy gates");
}
