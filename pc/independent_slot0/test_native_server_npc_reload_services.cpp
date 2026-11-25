#include "native_server_npc_reload_services.h"
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
    alignas(4) unsigned char actor[0xfcc]{},weapon[0x598]{},desc[0x48]{},row[16]{},info[0x168]{};
    sin_independent::EntityHandleEntry slots[2]{};
    unsigned dirty=0,kind=WeaponActivityAssault;
    std::vector<int> trace;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerNpcReloadServicesApi>(GetProcAddress(s,server_npc_reload_services_export));
    auto gp=reinterpret_cast<GetServerNpcParentWeaponEventApi>(GetProcAddress(s,server_npc_parent_weapon_event_export));
    auto gs=reinterpret_cast<GetServerWeaponEventSoundApi>(GetProcAddress(s,server_weapon_event_sound_export));
    assert(g&&gp&&gs&&!GetProcAddress(c,server_npc_reload_services_export)&&!g(0,sizeof(ServerNpcReloadServicesApi))&&!g(1,0));
    const auto* api=g(1,sizeof(ServerNpcReloadServicesApi));const auto* parent=gp(1,sizeof(ServerNpcParentWeaponEventApi));const auto* sound=gs(1,sizeof(ServerWeaponEventSoundApi));assert(api&&parent&&sound);
    F f;f.slots[1].serial=7;f.slots[1].entity=reinterpret_cast<CBaseEntity*>(f.weapon);
    f.put(f.actor,0x78c,0x7001u);f.put(f.weapon,0x16c,0x1234u);f.put(f.row,12,0x5678u);
    f.put(f.info,0x160,30);f.put(f.info,0x164,-1);
    f.put(f.desc,0x34,npc_global_condition_base+50);f.put(f.desc,0x38,3);f.put(f.desc,0x3c,5);
    f.put(f.actor,0x7d0,0xffffffffu);
    ServerNpcReloadServicesBindings b{};auto& w=b.original.weapon;w.context=&f;
    w.weapons={f.slots,2,&f,[](void* p,void* w,unsigned* out) noexcept -> int{auto& f=F::self(p);assert(w==f.weapon);*out=f.kind;return 1;},nullptr};
    w.extent=[](void* p,void* w,unsigned* out) noexcept -> int{auto& f=F::self(p);assert(w==f.weapon);*out=sizeof(f.weapon);return 1;};
    w.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(token==0x1234);f.trace.push_back(90);*out=&f.dirty;*n=4;return 1;};
    w.sound.context=&f;w.sound.emit=[](void* p,void*,int sound,float) noexcept -> int{F::self(p).trace.push_back(100+sound);return 1;};
    b.scripts={f.row,sizeof(f.row),1,f.info,sizeof(f.info),&f,
        [](void* p,std::uint32_t token,const void** data,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(token==0x5678);f.trace.push_back(80);*data=f.info;*n=sizeof(f.info);return 1;}};
    b.conditions.context=&f;b.conditions.descriptor=[](void* p,void* a,const void** data,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(a==f.actor);f.trace.push_back(70);*data=f.desc;*n=sizeof(f.desc);return 1;};
    assert(!b.original.script_info&&!b.original.clear_condition&&!w.sound.interval&&!w.sound.max_clip);
    ServerNpcReloadSession session{};assert(api->prepare(f.actor,sizeof(f.actor),&b,&session));
    ServerNpcReloadResult out{};
    assert(api->fixed_reset(&session,&out)&&out.live&&out.primary_changed&&out.secondary_changed&&out.conditions_cleared==3);
    assert(f.get<int>(f.weapon,0x4ec)==30&&f.get<int>(f.weapon,0x4f0)==-1&&f.get<unsigned>(f.actor,0x7d0)==(0xffffffffu&~(7u<<18)));
    assert((f.trace==std::vector<int>{109,80,90,80,90,70,70,70}));
    f.trace.clear();assert(api->fixed_reset(&session,&out)&&!out.primary_changed&&!out.secondary_changed);
    assert((f.trace==std::vector<int>{109,80,80,70,70,70}));
    f.put(f.weapon,0x4ec,1);f.trace.clear();ServerAnimationEvent e{3018,nullptr,0,0,0,f.actor};ServerNpcParentWeaponEventResult pr{};
    assert(parent->handle(f.actor,sizeof(f.actor),&e,10,&session.bound,&pr)==1&&pr.clip==30&&pr.conditions_cleared==3&&!pr.sounded);
    assert((f.trace==std::vector<int>{80,90,70,70,70}));
    f.trace.clear();f.kind=WeaponActivitySmg;f.put(f.weapon,0x588,3);f.put(f.weapon,0x584,3);f.put(f.weapon,0x4ec,3);
    ServerWeaponEventSoundResult sr{};
    assert(sound->play(f.weapon,sizeof(f.weapon),f.kind,1,0,&session.bound.weapon.sound,&sr)&&sr.last_sound==5&&session.burst_queries==1);
    f.trace.clear();f.kind=WeaponActivityAssault;f.put(f.weapon,0x28,9.875f);f.put(f.weapon,0x2c,10.f);f.put(f.weapon,0x580,0.f);
    assert(sound->assault_tick(f.weapon,sizeof(f.weapon),10,&session.bound.weapon.sound,&sr)&&sr.emitted==2&&session.sound_intervals==2);
    assert(f.get<float>(f.weapon,0x580)==static_cast<float>(static_cast<double>(static_cast<float>(10.+.075f))+.075f));
    f.trace.clear();f.put(f.actor,0x78c,0x8001u);f.put(f.actor,0x7d0,0xffffffffu);
    assert(api->fixed_reset(&session,&out)&&!out.live&&out.conditions_cleared==3&&(f.trace==std::vector<int>{70,70,70}));
    assert(f.get<unsigned>(f.actor,0x7d0)==(0xffffffffu&~(7u<<18)));
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS native script-backed dual clip reload, condition mapping and actual timing services");
}
