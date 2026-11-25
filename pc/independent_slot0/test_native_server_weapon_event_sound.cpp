#include "native_server_weapon_event_sound.h"
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
    alignas(4) unsigned char weapon[0x58c]{};
    std::vector<int> sounds;
    std::vector<float> times;
    int max_clip=30;double interval=.0625;
    unsigned queries=0;
    template<class T> void put(unsigned off,T v){std::memcpy(weapon+off,&v,sizeof(v));}
    template<class T> T get(unsigned off){T v{};std::memcpy(&v,weapon+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetServerWeaponEventSoundApi>(GetProcAddress(server,server_weapon_event_sound_export));
    assert(get&&!GetProcAddress(client,server_weapon_event_sound_export)&&!get(0,sizeof(ServerWeaponEventSoundApi))&&!get(1,0));
    const auto* api=get(1,sizeof(ServerWeaponEventSoundApi));assert(api);
    F f;ServerWeaponEventSoundBindings b{&f,
        [](void* p,void* w,int id,float time) noexcept -> int {auto& f=F::self(p);assert(w==f.weapon);f.sounds.push_back(id);f.times.push_back(time);return 1;},
        [](void* p,void*,int* out) noexcept -> int {auto& f=F::self(p);++f.queries;*out=f.max_clip;return 1;},
        [](void* p,void*,double* out) noexcept -> int {*out=F::self(p).interval;return 1;}
    };
    ServerWeaponEventSoundResult out{};
    auto play=[&](unsigned kind,int mode,int clip,int target,int sound){
        f.put(0x588,mode);f.put(0x4ec,clip);f.put(0x584,target);f.sounds.clear();f.queries=0;
        return api->play(f.weapon,sizeof(f.weapon),kind,sound,0,&b,&out);
    };
    for(unsigned kind:{WeaponActivitySmg,WeaponActivityChaingun}){
        assert(play(kind,0,1,30,1)&&!out.emitted&&!f.queries);
        assert(play(kind,1,1,30,1)&&out.emitted==1&&out.last_sound==1&&!f.queries);
        assert(play(kind,2,1,30,1)&&!out.emitted&&!f.queries);
        assert(play(kind,3,29,30,1)&&out.emitted==1&&out.last_sound==1&&f.queries==1);
        assert(play(kind,3,30,30,1)&&out.emitted==1&&out.last_sound==5&&f.queries==1);
        assert(play(kind,3,20,20,1)&&!out.emitted&&f.queries==1);
        assert(play(kind,3,19,20,1)&&out.emitted==1&&out.last_sound==1);
        assert(play(kind,3,21,20,1)&&!out.emitted);
        assert(play(kind,0,0,0,2)&&out.emitted==1&&out.last_sound==2&&!f.queries);
    }
    assert(play(WeaponActivityAssault,0,0,0,1)&&out.emitted==1&&out.last_sound==1);
    f.sounds.clear();f.times.clear();f.put(0x580,8.f);f.put(0x2c,10.f);f.put(0x28,9.75f);
    assert(api->assault_tick(f.weapon,sizeof(f.weapon),10,&b,&out)&&out.emitted==2);
    assert((f.times==std::vector<float>{10.f,10.0625f})&&f.get<float>(0x580)==10.125f);
    f.sounds.clear();f.times.clear();f.put(0x580,10.f);f.put(0x28,9.9375f);
    assert(api->assault_tick(f.weapon,sizeof(f.weapon),10,&b,&out)&&out.emitted==1&&f.get<float>(0x580)==10.0625f);
    f.put(0x580,10.f);f.put(0x28,10.125f);
    assert(api->assault_tick(f.weapon,sizeof(f.weapon),10,&b,&out)&&!out.emitted);
    f.put(0x580,10.25f);f.put(0x28,9.f);
    assert(api->assault_tick(f.weapon,sizeof(f.weapon),10,&b,&out)&&!out.emitted&&f.get<float>(0x580)==10.25f);
    FreeLibrary(server);FreeLibrary(client);std::puts("PASS native weapon animation sound gate and bounded scheduler");
}
