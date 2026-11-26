#include "native_server_weapon_post_frame.h"
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
    alignas(4) unsigned char actor[0xbe0]{},weapon[0x4c0]{},viewmodel[0x47c]{},desc[2][0x40]{};
    sin_independent::EntityHandleEntry slots[4]{};
    ServerActivityDescriptor rows[2]{};
    std::uint32_t gen=3;
    ServerActivityGenerationNode root{&gen,-1};
    ServerActivityCompositeView metadata{rows,2,&root,1};
    ServerStudioEvent weapon_event{.25f,31,0,"weapon",0},view_event{.25f,32,0,"view",0};
    std::vector<int> trace;
    bool player=true,lose_owner=false;
    template<class T> void put(void* a,unsigned off,T value){std::memcpy(static_cast<unsigned char*>(a)+off,&value,sizeof(value));}
    template<class T> T get(void* a,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(a)+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
    void seed(void* a,int seq,float cycle){
        put(a,0x1f0,std::uint8_t{1});put(a,0x28,9.75f);put(a,0x2c,10.f);
        put(a,0x3ec,seq);put(a,0x3c0,2.f);put(a,0x3e8,cycle);put(a,0x398,cycle);
        put(a,0x3e0,std::uint8_t{0});put(a,0x3e1,std::uint8_t{1});
    }
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto g=reinterpret_cast<GetServerWeaponPostFrameApi>(GetProcAddress(server,server_weapon_post_frame_export));
    assert(g&&!GetProcAddress(client,server_weapon_post_frame_export)&&!g(0,sizeof(ServerWeaponPostFrameApi))&&!g(1,0));
    const auto* api=g(1,sizeof(ServerWeaponPostFrameApi));assert(api);
    F f;f.slots[1].serial=7;f.slots[1].entity=reinterpret_cast<CBaseEntity*>(f.weapon);
    f.slots[2].serial=8;f.slots[2].entity=reinterpret_cast<CBaseEntity*>(f.actor);
    f.slots[3].serial=9;f.slots[3].entity=reinterpret_cast<CBaseEntity*>(f.viewmodel);
    f.put(f.actor,0x78c,0x7001u);f.put(f.actor,0xbdc,0x9003u);f.put(f.weapon,0x494,0x8002u);
    f.put(f.weapon,0x4bc,309);f.put(f.weapon,0x4a0,0xffffffffu);
    for(unsigned i=0;i<2;++i){f.rows[i]={f.desc[i],0x40,0};f.desc[i][0xd]=0x10;f.put(f.desc[i],0x10,309);f.put(f.desc[i],0x14,i?4:1);}
    ServerEventDictionaryEntry ar[2]{},er[2]{};
    ServerActivityDictionary activities{{ar,2,0,3,0},-1};ServerEventDictionary events{er,2,0,0,0};
    ServerWeaponPostFrameBindings b{};b.context=&f;
    b.weapons={f.slots,4,&f,[](void* p,void* w,unsigned* out) noexcept -> int{assert(w==F::self(p).weapon);*out=WeaponActivityAssault;return 1;},nullptr};
    b.extent=[](void* p,void* a,unsigned* out) noexcept -> int {
        auto& f=F::self(p);
        if(a==f.actor)*out=sizeof(f.actor);else if(a==f.weapon)*out=sizeof(f.weapon);else if(a==f.viewmodel)*out=sizeof(f.viewmodel);else return 0;
        return 1;};
    b.combat_owner=[](void* p,void* e,void** out) noexcept -> int {auto& f=F::self(p);assert(e==f.actor);f.trace.push_back(40);*out=e;return 1;};
    b.is_player=[](void* p,void* e,bool* out) noexcept -> int {auto& f=F::self(p);assert(e==f.actor);f.trace.push_back(41);*out=f.player;return 1;};
    b.weapon_event=[](void* p,void* w,void* player,const ServerAnimationEvent* e) noexcept -> int {
        auto& f=F::self(p);assert(w==f.weapon&&player==f.actor&&e->source==f.viewmodel&&e->event==32);f.trace.push_back(32);return 1;};
    auto& a=b.animation;a.context=&f;a.activities=&activities;a.events=&events;
    a.metadata=[](void* p,void*,const ServerActivityCompositeView** out) noexcept -> int {auto& f=F::self(p);f.trace.push_back(20);*out=&f.metadata;return 1;};
    a.random.random=[](void*,int,int,int,int* out) noexcept -> int {*out=0;return 1;};
    a.reset.context=&f;
    a.reset.duration=[](void*,void*,int,double* out) noexcept -> int {*out=1.;return 1;};
    a.reset.distance=[](void*,void*,int,double* out) noexcept -> int {*out=0;return 1;};
    a.reset.flags=[](void*,void*,int,unsigned* out) noexcept -> int {*out=1;return 1;};
    a.dictionary.dictionary=&events;
    a.dictionary.descriptor=[](void*,void*,int,void**,unsigned*,unsigned*) noexcept -> int {return 0;};
    b.frame.context=&f;b.frame.cycle.context=&f;
    b.frame.width_transition=[](void* p,void* e) noexcept -> int {auto& f=F::self(p);f.trace.push_back(e==f.weapon?10:11);return 1;};
    b.frame.cycle.duration=a.reset.duration;b.frame.cycle.movement_distance=a.reset.distance;
    b.frame.cycle.finish_threshold=[](void*,void*,int,double* out) noexcept -> int {*out=.9;return 1;};
    b.frame.cycle.invalidate=[](void*,void*,unsigned) noexcept -> int{return 1;};
    b.frame.cycle.cache=[](void*,void*,std::uint32_t,float) noexcept -> int{return 1;};
    b.view_frame=b.frame;
    b.events.context=&f;
    b.events.sequence=[](void* p,void* source,int,ServerAnimationEventSequence* out) noexcept -> int {
        auto& f=F::self(p);assert(source==f.weapon||source==f.viewmodel);*out={source==f.weapon?&f.weapon_event:&f.view_event,1,1,0};return 1;};
    b.events.cycle_rate=[](void*,void*,int,double* out) noexcept -> int {*out=1.;return 1;};
    b.events.emit=[](void* p,void* receiver,int layer,const ServerAnimationEvent* e) noexcept {
        auto& f=F::self(p);assert(receiver==f.actor&&layer==-1&&e->source==f.weapon&&e->event==31);f.trace.push_back(31);
        if(f.lose_owner)f.put(f.weapon,0x494,0xffffffffu);};
    b.view_events=b.events;b.view_events.emit=nullptr;
    ServerWeaponPostFrameResult out{};unsigned live=0;
    f.seed(f.weapon,0,.125f);f.seed(f.viewmodel,0,.125f);
    assert(api->notify(f.actor,sizeof(f.actor),10.125f,&b,&out,&live)&&live&&out.advanced&&!out.reselected&&out.events.emitted==1&&out.viewmodel_advanced&&out.viewmodel_forwarded==1);
    assert((f.trace==std::vector<int>{10,31,40,41,11,40,41,32}));
    assert(f.get<unsigned>(f.weapon,0x494)==0x8002u&&f.get<unsigned>(f.weapon,0x4a0)==0xffffffffu);
    f.trace.clear();f.player=false;f.seed(f.weapon,0,.125f);
    assert(api->run(f.weapon,sizeof(f.weapon),f.actor,10.125f,&b,&out)&&out.owner_live&&!out.player_owner&&!out.viewmodel_advanced);
    assert((f.trace==std::vector<int>{10,31,40,41}));
    f.trace.clear();f.player=true;f.lose_owner=true;f.seed(f.weapon,0,.125f);
    assert(api->run(f.weapon,sizeof(f.weapon),f.actor,10.125f,&b,&out)&&!out.owner_live&&!out.viewmodel_advanced);
    assert((f.trace==std::vector<int>{10,31}));
    f.trace.clear();f.lose_owner=false;f.put(f.weapon,0x494,0xffffffffu);
    f.seed(f.weapon,0,.875f);f.put(f.weapon,0x398,0.f);
    assert(api->run(f.weapon,sizeof(f.weapon),f.actor,10.125f,&b,&out)&&out.reselected&&out.sequence_set&&out.selected==1&&out.reset.reset.reset);
    assert(f.get<int>(f.weapon,0x3ec)==1&&f.get<float>(f.weapon,0x3e8)==.125f&&f.get<float>(f.weapon,0x3c0)==1.f);
    assert((f.trace==std::vector<int>{10,20}));
    f.trace.clear();f.seed(f.weapon,1,.875f);f.put(f.weapon,0x398,0.f);f.put(f.desc[1],0x14,-4);
    assert(api->run(f.weapon,sizeof(f.weapon),f.actor,10.125f,&b,&out)&&out.reselected&&out.sequence_set&&out.selected==1&&!out.reset.reset.reset);
    assert(f.get<float>(f.weapon,0x3c0)==2.f);
    f.trace.clear();f.put(f.actor,0x78c,0x8001u);
    assert(api->notify(f.actor,sizeof(f.actor),10.125f,&b,&out,&live)&&!live&&f.trace.empty());
    b.combat_owner=nullptr;b.is_player=nullptr;
    f.put(f.weapon,0x494,0x8002u);f.put(f.actor,0x78c,0x7001u);
    f.trace.clear();f.put(f.actor,0,0x2249aaa4u);f.seed(f.weapon,0,.125f);
    assert(api->run(f.weapon,sizeof(f.weapon),f.actor,10.125f,&b,&out)&&out.owner_live&&!out.player_owner&&!out.viewmodel_advanced);
    assert((f.trace==std::vector<int>{10,31}));
    f.trace.clear();f.put(f.actor,0,0x224aec3cu);f.seed(f.weapon,0,.125f);f.seed(f.viewmodel,0,.125f);
    assert(api->run(f.weapon,sizeof(f.weapon),f.actor,10.125f,&b,&out)&&out.owner_live&&out.player_owner&&out.viewmodel_forwarded==1);
    assert((f.trace==std::vector<int>{10,31,11,32}));
    f.trace.clear();f.put(f.actor,0,0x2247ab9cu);f.seed(f.weapon,0,.125f);
    assert(api->run(f.weapon,sizeof(f.weapon),f.actor,10.125f,&b,&out)&&!out.owner_live&&!out.viewmodel_advanced);
    f.trace.clear();f.put(f.actor,0,0xdeadbeefu);f.seed(f.weapon,0,.125f);
    assert(!api->run(f.weapon,sizeof(f.weapon),f.actor,10.125f,&b,&out)&&!out.viewmodel_advanced);
    FreeLibrary(server);FreeLibrary(client);std::puts("PASS native weapon loop reselection and player viewmodel event forwarding");
}
