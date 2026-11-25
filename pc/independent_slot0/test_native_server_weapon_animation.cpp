#include "native_server_weapon_animation.h"
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
    alignas(4) unsigned char owner[0x790]{},weapon[0x4c0]{},desc[2][0x40]{};
    sin_independent::EntityHandleEntry slots[2]{};
    ServerActivityDescriptor rows[2]{};
    std::uint32_t gen=3,dirty=0;
    ServerActivityGenerationNode root{&gen,-1};
    ServerActivityCompositeView view{rows,2,&root,1};
    unsigned durations=0,registrations=0,lookups=0;
    double own=3.;
    template<class T> void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T> T get(unsigned off){T v{};std::memcpy(&v,weapon+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetServerWeaponAnimationApi>(GetProcAddress(server,server_weapon_animation_export));
    assert(get&&!GetProcAddress(client,server_weapon_animation_export)&&!get(0,sizeof(ServerWeaponAnimationApi))&&!get(1,0));
    auto* api=get(1,sizeof(ServerWeaponAnimationApi));assert(api);
    F f;f.slots[1].serial=9;f.slots[1].entity=reinterpret_cast<CBaseEntity*>(f.weapon);f.put(f.owner,0x78c,0x9001u);
    for(unsigned i=0;i<2;++i){f.rows[i]={f.desc[i],0x40,0};f.desc[i][0xd]=0x10;f.put(f.desc[i],0x10,i?157:309);f.put(f.desc[i],0x14,1);}
    ServerEventDictionaryEntry ar[2]{},er[2]{};
    ServerActivityDictionary activities{{ar,2,0,3,0},-1};ServerEventDictionary events{er,2,0,0,0};
    ServerWeaponAnimationDispatchBindings b{};b.context=&f;
    b.weapons={f.slots,2,&f,[](void* p,void* w,unsigned* out) noexcept -> int{assert(w==F::self(p).weapon);*out=WeaponActivityAssault;return 1;},nullptr};
    b.extent=[](void* p,void* w,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(w==f.weapon);*n=sizeof(f.weapon);return 1;};
    auto& a=b.animation;a.context=&f;a.activities=&activities;a.events=&events;
    a.metadata=[](void* p,void* w,const ServerActivityCompositeView** out) noexcept -> int{auto& f=F::self(p);assert(w==f.weapon);++f.lookups;*out=&f.view;return 1;};
    a.random.random=[](void*,int lo,int hi,int,int* out) noexcept -> int{assert(lo==0&&hi==0);*out=0;return 1;};
    a.reset.context=&f;
    a.reset.duration=[](void* p,void*,int,double* out) noexcept -> int{auto& f=F::self(p);++f.durations;*out=f.own;return 1;};
    a.reset.distance=[](void*,void*,int,double* out) noexcept -> int{*out=0;return 1;};
    a.reset.flags=[](void*,void*,int,unsigned* out) noexcept -> int{*out=1;return 1;};
    a.reset.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(token==0x1234u);*out=&f.dirty;*n=4;return 1;};
    a.dictionary.context=&f;a.dictionary.dictionary=&events;
    a.dictionary.descriptor=[](void* p,void*,int,void**,unsigned*,unsigned*) noexcept -> int{++F::self(p).registrations;return 0;};
    f.put(f.weapon,0x16c,0x1234u);f.put(f.weapon,0x3ec,0);f.put(f.weapon,0x3e8,.6f);f.put(f.weapon,0x3c0,1.f);
    f.weapon[0x3e1]=1;f.weapon[0x3e0]=1;f.put(f.weapon,0x464,7u);
    ServerWeaponAnimationDispatchResult out{};
    assert(api->notify(f.owner,sizeof(f.owner),309,1.5f,&b,&out)&&out.live&&out.animation.assigned&&!out.animation.sequence_changed&&out.animation.cycle_changed);
    assert(out.animation.sequence==0&&out.animation.reset.reset.reset&&f.get<unsigned>(0x464)==0&&f.get<int>(0x4bc)==309);
    assert(f.get<float>(0x3e8)==0.f&&f.get<float>(0x3c0)==2.f&&f.dirty==1&&f.registrations==1&&f.durations==2);
    // Missing request falls back to 157 but retains original activity.
    f.own=30.;f.dirty=0;
    assert(api->notify(f.owner,sizeof(f.owner),999,1.f,&b,&out)&&out.animation.fallback&&out.animation.selections==2&&out.animation.sequence==1);
    assert(f.get<int>(0x4bc)==999&&f.get<float>(0x3c0)==12.f&&f.dirty==1);
    // Nonpositive NPC duration resets playback to 1; duration only used by ResetSequenceInfo.
    const auto prior=f.durations;
    assert(api->notify(f.owner,sizeof(f.owner),157,0.f,&b,&out)&&out.animation.assigned&&!out.animation.sequence_changed&&f.get<float>(0x3c0)==1.f&&f.durations==prior+1);
    f.put(f.desc[1],0x10,42);const auto registrations=f.registrations;
    assert(api->notify(f.owner,sizeof(f.owner),999,1.f,&b,&out)&&out.animation.sequence==-1&&!out.animation.assigned&&f.registrations==registrations&&f.get<int>(0x4bc)==157);
    f.put(f.owner,0x78c,0xa001u);const auto lookups=f.lookups;
    assert(api->notify(f.owner,sizeof(f.owner),309,1.f,&b,&out)&&!out.live&&f.lookups==lookups);
    FreeLibrary(server);FreeLibrary(client);std::puts("PASS native weapon activity assignment, fallback and playback cap");
}
