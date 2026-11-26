#include "native_server_weapon_event_route.h"
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
#include <initializer_list>
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char actor[0x790]{},weapon[0x598]{};
    sin_independent::EntityHandleEntry slots[3]{};
    unsigned kind=WeaponActivityAssault,dirty=0;
    bool player=true;
    std::vector<int> trace;
    template<class T> void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T> T get(unsigned off){T v{};std::memcpy(&v,weapon+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto g=reinterpret_cast<GetServerWeaponEventRouteApi>(GetProcAddress(server,server_weapon_event_route_export));
    assert(g&&!GetProcAddress(client,server_weapon_event_route_export)&&!g(0,sizeof(ServerWeaponEventRouteApi))&&!g(1,0));
    const auto* api=g(1,sizeof(ServerWeaponEventRouteApi));assert(api);
    F f;f.slots[1].serial=7;f.slots[1].entity=reinterpret_cast<CBaseEntity*>(f.weapon);
    f.slots[2].serial=8;f.slots[2].entity=reinterpret_cast<CBaseEntity*>(f.actor);
    f.put(f.actor,0x78c,0x7001u);f.put(f.weapon,0x494,0x8002u);f.put(f.weapon,0x16c,0x1000u);
    ServerWeaponEventRouteBindings b{};b.context=&f;
    b.weapons={f.slots,3,&f,[](void* p,void* w,unsigned* out) noexcept -> int{auto& f=F::self(p);assert(w==f.weapon);*out=f.kind;return 1;},nullptr};
    b.extent=[](void* p,void* w,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(w==f.weapon);*n=sizeof(f.weapon);return 1;};
    b.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(token==0x1000);f.trace.push_back(90);*out=&f.dirty;*n=4;return 1;};
    b.sound.context=&f;
    b.sound.emit=[](void* p,void*,int id,float) noexcept -> int{F::self(p).trace.push_back(100+id);return 1;};
    b.sound.interval=[](void*,void*,double* out) noexcept -> int{*out=.0625;return 1;};
    b.combat_owner=[](void* p,void* e,void** out) noexcept -> int{auto& f=F::self(p);assert(e==f.actor);f.trace.push_back(40);*out=e;return 1;};
    b.is_player=[](void* p,void*,bool* out) noexcept -> int{auto& f=F::self(p);f.trace.push_back(41);*out=f.player;return 1;};
    b.prepare=[](void* p,void* w,void* a,unsigned,const ServerAnimationEvent*) noexcept -> int{auto& f=F::self(p);assert(w==f.weapon&&a==f.actor);f.trace.push_back(1);return 1;};
    b.primary_effect=[](void* p,void*,void*,unsigned,const ServerAnimationEvent*) noexcept -> int{F::self(p).trace.push_back(2);return 1;};
    b.flash=[](void* p,void* a) noexcept -> int{auto& f=F::self(p);assert(a==f.actor);f.trace.push_back(3);return 1;};
    b.secondary=[](void* p,void*,unsigned) noexcept -> int{F::self(p).trace.push_back(4);return 1;};
    b.grenade=[](void* p,void*,void* a,int id) noexcept -> int{auto& f=F::self(p);assert(a==f.actor);f.trace.push_back(id);return 1;};
    b.consume_grenade=[](void* p,void*,int type) noexcept -> int{assert(type==6);F::self(p).trace.push_back(5);return 1;};
    b.grenade_danger=[](void* p,void*,void*) noexcept -> int{F::self(p).trace.push_back(6);return 1;};
    b.melee=[](void* p,void*,void*,const ServerAnimationEvent*) noexcept -> int{F::self(p).trace.push_back(7);return 1;};
    b.base_contact=[](void* p,void*,void*,const ServerAnimationEvent*) noexcept -> int{F::self(p).trace.push_back(8);return 1;};
    b.diagnostic=[](void* p,void*,const ServerAnimationEvent*) noexcept{F::self(p).trace.push_back(9);};
    ServerWeaponEventRouteResult out{};ServerAnimationEvent event{0,"",0,0,0,f.actor};
    auto run=[&](unsigned kind,int id){
        f.trace.clear();f.kind=kind;event.event=id;return api->active(f.actor,sizeof(f.actor),&event,10,&b,&out);
    };
    f.put(f.weapon,0x4ec,3u);f.put(f.weapon,0x28,9.75f);f.put(f.weapon,0x2c,10.f);
    assert(run(WeaponActivityAssault,3002)&&out.route==WeaponEventPrimary&&out.sound.emitted==2&&out.clip_changed&&f.get<unsigned>(0x4ec)==2);
    assert((f.trace==std::vector<int>{1,102,102,2,3,90}));
    assert(run(WeaponActivityMagnum,3014)&&out.route==WeaponEventPrimary&&(f.trace==std::vector<int>{1,102,2,3,90}));
    assert(run(WeaponActivityNpcPistol,3014)&&out.route==WeaponEventPrimary&&f.get<unsigned>(0x4ec)==0);
    assert(run(WeaponActivityScatter,3004)&&out.route==WeaponEventPrimary&&f.get<unsigned>(0x4ec)==0xffffffffu&&(f.trace==std::vector<int>{1,102,3,90,2}));
    assert(run(WeaponActivitySmg,3002)&&out.route==WeaponEventPrimary&&(f.trace==std::vector<int>{1,102,2,3,90}));
    assert(run(WeaponActivityChaingun,3002)&&out.route==WeaponEventSpinStart&&f.get<int>(0x594)==1&&f.get<float>(0x590)==10.f&&(f.trace==std::vector<int>{113}));
    assert(run(WeaponActivityChaingun,3002)&&out.route==WeaponEventSpinIdle&&f.trace.empty());
    f.put(f.weapon,0x594,2);assert(run(WeaponActivityChaingun,3002)&&out.route==WeaponEventPrimary&&(f.trace==std::vector<int>{90,1,2,3,102})&&f.get<float>(0x58c)==10.f);
    for(unsigned kind:{WeaponActivityAssault,WeaponActivityMagnum,WeaponActivityNpcPistol,WeaponActivityScatter})
        assert(run(kind,3103)&&out.secondary_called&&(f.trace==std::vector<int>{4}));
    assert(run(WeaponActivitySmg,3103)&&out.route==WeaponEventDiagnostic&&(f.trace==std::vector<int>{9}));
    assert(run(WeaponActivityMelee,3001)&&out.route==WeaponEventMelee&&(f.trace==std::vector<int>{7}));
    assert(run(WeaponActivityBase,3001)&&out.route==WeaponEventSound&&(f.trace==std::vector<int>{111}));
    assert(run(WeaponActivityBase,3003)&&out.route==WeaponEventSound&&(f.trace==std::vector<int>{110}));
    assert(run(WeaponActivityBase,3200)&&out.route==WeaponEventBaseContact&&(f.trace==std::vector<int>{40,41,8}));
    f.player=false;assert(run(WeaponActivityBase,3200)&&(f.trace==std::vector<int>{40,41}));f.player=true;
    f.put(f.weapon,0x4e4,6);assert(run(WeaponActivityGrenade,3900)&&out.route==WeaponEventGrenadeReady&&f.get<std::uint8_t>(0x584)==1);
    assert(run(WeaponActivityGrenade,3013)&&out.grenade_consumed&&out.deadlines_written==3&&f.get<float>(0x49c)==10.5f&&f.get<float>(0x4a0)==10.5f&&f.get<unsigned>(0x4a4)==0x7f7fffffu);
    assert((f.trace==std::vector<int>{40,41,3013,5,90,90,90,40,6}));
    assert(run(WeaponActivityGrenade,3005)&&out.grenade_consumed&&!out.deadlines_written);
    f.put(f.actor,0x78c,0x8001u);assert(!run(WeaponActivityAssault,3103)&&f.trace.empty());

    auto gp=reinterpret_cast<GetServerNpcParentWeaponEventApi>(GetProcAddress(server,server_npc_parent_weapon_event_export));
    assert(gp&&!GetProcAddress(client,server_npc_parent_weapon_event_export)&&!gp(0,sizeof(ServerNpcParentWeaponEventApi))&&!gp(1,0));
    const auto* parent=gp(1,sizeof(ServerNpcParentWeaponEventApi));assert(parent);
    struct P {F* f;alignas(4) unsigned char info[0x164]{};} p{&f};
    f.put(p.info,0x160,28);f.put(f.weapon,0x51c,std::uint16_t{5});
    ServerNpcParentWeaponEventBindings pb{b,&p,
        [](void* p,std::uint16_t id,const void** out,unsigned* n) noexcept -> int {
            auto& q=*static_cast<P*>(p);assert(id==5);q.f->trace.push_back(80);*out=q.info;*n=sizeof(q.info);return 1;},
        [](void* p,void* a,int cond) noexcept -> int {
            auto& q=*static_cast<P*>(p);assert(a==q.f->actor);q.f->trace.push_back(200+cond);return 1;}
    };
    f.put(f.actor,0x78c,0x7001u);f.kind=WeaponActivityAssault;
    ServerNpcParentWeaponEventResult pr{};
    auto run_parent=[&](int id){f.trace.clear();event.event=id;return parent->handle(f.actor,sizeof(f.actor),&event,10,&pb,&pr);};
    assert(run_parent(20)==0&&!pr.recognized&&f.trace.empty());
    assert(run_parent(3103)==1&&pr.forwarded&&pr.weapon.secondary_called&&(f.trace==std::vector<int>{4}));
    f.put(f.weapon,0x4ec,3);
    assert(run_parent(3015)==1&&pr.sounded&&pr.clip_changed&&pr.conditions_cleared==3&&pr.clip==28);
    assert((f.trace==std::vector<int>{109,80,90,203,204,205}));
    assert(run_parent(3017)==1&&pr.sounded&&!pr.clip_changed&&!pr.conditions_cleared&&(f.trace==std::vector<int>{109}));
    assert(run_parent(3018)==1&&!pr.sounded&&!pr.clip_changed&&pr.conditions_cleared==3&&(f.trace==std::vector<int>{80,203,204,205}));
    f.put(f.actor,0x78c,0x8001u);
    assert(run_parent(3015)==1&&pr.recognized&&!pr.live&&f.trace.empty());
    assert(run_parent(3002)==1&&pr.recognized&&!pr.live&&f.trace.empty());
    f.put(f.actor,0x78c,0x7001u);b.combat_owner=nullptr;b.is_player=nullptr;
    b.extent=[](void* p,void* e,unsigned* n) noexcept -> int{
        auto& f=F::self(p);if(e==f.actor)*n=sizeof(f.actor);else if(e==f.weapon)*n=sizeof(f.weapon);else return 0;return 1;
    };
    for(unsigned vt:{0x2245222cu,0x224aec3cu}){
        f.put(f.actor,0,vt);assert(run(WeaponActivityBase,3200)&&out.owner_live&&out.player_owner&&(f.trace==std::vector<int>{8}));
        assert(run(WeaponActivityGrenade,3005)&&out.grenade_consumed&&(f.trace==std::vector<int>{3005,5,6}));
    }
    f.put(f.actor,0,0x2249aaa4u);
    assert(run(WeaponActivityBase,3200)&&out.owner_live&&!out.player_owner&&f.trace.empty());
    assert(!run(WeaponActivityGrenade,3005)&&out.owner_live&&!out.player_owner&&f.trace.empty());
    f.put(f.actor,0,0x2247ab9cu);
    assert(run(WeaponActivityBase,3200)&&!out.owner_live&&f.trace.empty());
    f.put(f.actor,0,0xdeadbeefu);
    assert(!run(WeaponActivityBase,3200)&&f.trace.empty());
    FreeLibrary(server);FreeLibrary(client);std::puts("PASS original eight weapon event routes and native state writes");
}
