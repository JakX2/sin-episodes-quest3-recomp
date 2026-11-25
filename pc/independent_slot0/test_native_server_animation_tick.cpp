#include "native_server_animation_tick.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <vector>
#include <cstdio>
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char actor[0xfcc]{},component[0x28]{},path[0x48]{},path2[0x48]{},weapon[4]{};
    bool promoted=false,replace=false,live=true;
    std::vector<int> calls;
    template<class T> void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T> T get(unsigned off){T v{};std::memcpy(&v,actor+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetServerNpcAnimationTickApi>(GetProcAddress(server,server_npc_animation_tick_export));
    assert(get&&!GetProcAddress(client,server_npc_animation_tick_export)&&!get(0,sizeof(ServerNpcAnimationTickApi))&&!get(1,0));
    auto* api=get(1,sizeof(ServerNpcAnimationTickApi));assert(api);
    F f;ServerNpcAnimationTickBindings b{};b.context=&f;
    ServerEventDictionaryEntry ae[1]{},ee[1]{};
    ServerActivityDictionary a{{ae,1,0,0,0},-1};ServerEventDictionary e{ee,1,0,0,0};
    b.transition.assignment.translation.actor_class=NpcActivityGrunt;
    b.transition.assignment.translation.activities=&a;b.transition.assignment.translation.events=&e;
    b.transition.assignment.translation.context=&f;
    // Absent metadata for pending setter, and separately for RunAnimation.
    b.transition.assignment.translation.metadata=[](void* p,void*,const ServerActivityCompositeView** out) noexcept -> int {
        F::self(p).calls.push_back(2);*out=nullptr;return 0;};
    b.frame.activities=&a;b.frame.frame.dictionary.dictionary=&e;
    b.frame.frame.sequence.frame.context=&f;
    b.frame.frame.sequence.frame.model_available=[](void* p,void*) noexcept -> int{F::self(p).calls.push_back(3);return 0;};
    b.storage=[](void* p,std::uint32_t w,const void** out,unsigned* n) noexcept -> int {
        auto& f=F::self(p);
        if(w==1){*out=f.component;*n=sizeof(f.component);}
        else if(w==2){*out=f.path;*n=sizeof(f.path);}
        else if(w==3){*out=f.path2;*n=sizeof(f.path2);}
        else return 0;return 1;
    };
    b.promote_path=[](void* p,void*,std::uint32_t c,bool* out) noexcept -> int {
        auto& f=F::self(p);assert(c==1);f.calls.push_back(1);if(f.replace)f.put(f.component,0x24,3u);*out=f.promoted;return 1;};
    b.weapon=[](void* p,std::uint32_t w,void** out) noexcept -> int {
        auto& f=F::self(p);assert(w==4);f.calls.push_back(4);*out=f.live?f.weapon:nullptr;return f.live;};
    b.weapon_post=[](void* p,void* weapon,void* actor) noexcept -> int {
        auto& f=F::self(p);assert(weapon==f.weapon&&actor==f.actor);f.calls.push_back(5);return 1;};
    f.put(f.actor,0x8f8,1u);f.put(f.actor,0x78c,4u);f.put(f.component,0x24,2u);
    f.put(f.path,0,1u);f.put(f.path,0x24,10);f.put(f.path2,0,1u);f.put(f.path2,0x24,11);
    ServerNpcRunAnimationInput in{10,true,false};ServerNpcAnimationTickResult out{};
    auto seed=[&](){f.calls.clear();f.put(f.actor,0x880,6);f.put(f.component,0x24,2u);};
    seed();assert(api->run(f.actor,sizeof(f.actor),&in,&b,&out)&&out.movement_checked&&out.movement_requested&&out.movement_activity==10&&out.weapon_posted);
    assert(f.get<int>(0x880)==10&&(f.calls==std::vector<int>{1,2,3,4,5}));
    seed();f.put(f.path,0x44,1u);
    assert(api->run(f.actor,sizeof(f.actor),&in,&b,&out)&&out.path_suppressed&&!out.movement_checked&&(f.calls==std::vector<int>{3,4,5}));
    seed();f.put(f.path,0x44,0u);f.promoted=true;
    assert(api->run(f.actor,sizeof(f.actor),&in,&b,&out)&&out.promoted&&!out.movement_requested&&f.get<int>(0x880)==6&&(f.calls==std::vector<int>{1,3,4,5}));
    seed();f.promoted=false;f.replace=true;
    assert(api->run(f.actor,sizeof(f.actor),&in,&b,&out)&&out.movement_activity==11&&f.get<int>(0x880)==11);
    seed();f.replace=false;f.put(f.path,0,0u);
    assert(api->run(f.actor,sizeof(f.actor),&in,&b,&out)&&out.movement_activity==1&&f.get<int>(0x880)==1);
    seed();f.put(f.actor,0x880,15);f.live=false;
    assert(api->run(f.actor,sizeof(f.actor),&in,&b,&out)&&!out.movement_checked&&!out.weapon_posted&&(f.calls==std::vector<int>{3,4}));
    f.calls.clear();f.put(f.actor,0x78c,0xffffffffu);
    assert(api->run(f.actor,sizeof(f.actor),&in,&b,&out)&&!out.weapon_posted&&(f.calls==std::vector<int>{3}));
    f.calls.clear();f.put(f.component,0x24,0u);assert(!api->run(f.actor,sizeof(f.actor),&in,&b,&out)&&f.calls.empty());
    FreeLibrary(server);FreeLibrary(client);std::puts("PASS native NPC animation tick path gates and weapon post ordering");
}
