#include "native_server_grenade_spit_detonation.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <string>
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char actor[0x300]{};
    std::string calls;
    unsigned clears=0,sounds=0,removes=0,effects=0;
    float draw=101.f;
    template<class T>void put(unsigned off,T v){std::memcpy(actor+off,&v,sizeof(v));}
    template<class T>T get(unsigned off){T v{};std::memcpy(&v,actor+off,sizeof(v));return v;}
};
int main(int argc,char** argv){
    assert(argc==3);
    auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerGrenadeSpitDetonationApi>(GetProcAddress(server,server_grenade_spit_detonation_export));
    assert(get&&!GetProcAddress(client,server_grenade_spit_detonation_export)&&!get(2,sizeof(ServerGrenadeSpitDetonationApi)));
    const auto* api=get(1,sizeof(ServerGrenadeSpitDetonationApi));assert(api);
    F f;f.put(0x94,5u);f.put(0x2bc,1.f);f.put(0x2c0,2.f);f.put(0x2c4,3.f);
    ServerGrenadeSpitDetonationBindings b{};b.context=&f;
    b.clear_state=[](void* ctx,void* a) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(a==f.actor&&f.get<unsigned>(0x94)==5u);
        ++f.clears;f.calls+='C';return 1;
    };
    b.random_float=[](void* ctx,float lo,float hi,float* out) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(lo==90.f&&hi==110.f);
        f.calls+='R';*out=f.draw;return 1;
    };
    b.sound=[](void* ctx,void* a,const char* s,int x,int y) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(a==f.actor&&!x&&!y);
        if(f.sounds%2==0)assert(!std::strcmp(s,"GrenadeSpit.Acid"));
        else assert(!std::strcmp(s,"GrenadeSpit.Hit"));
        ++f.sounds;f.calls+='S';return 1;
    };
    b.refresh_origin=[](void* ctx,void* a) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(a==f.actor);f.calls+='O';f.put(0x2bc,4.f);f.put(0x2c0,5.f);f.put(0x2c4,6.f);return 1;
    };
    b.poison_effect=[](void* ctx,void* a,const ServerGrenadeSpitPoisonEffect* q) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(a==f.actor&&q->origin[0]==4.f&&q->origin[1]==5.f&&q->origin[2]==6.f&&
            q->arg0==10.f&&q->arg1==175.f&&q->arg2==10.f&&q->arg3==-1.f&&!q->mode);
        ++f.effects;f.calls+='E';return 1;
    };
    b.remove=[](void* ctx,void* a) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);assert(a==f.actor);++f.removes;f.calls+='D';return 1;
    };
    ServerGrenadeSpitDetonationResult o{};
    assert(api->mutant(f.actor,sizeof(f.actor),&b,&o)&&o.variant==1&&o.state_cleared&&o.random_calls==1&&o.sounds==2&&!o.effects&&o.removed&&o.draw==101.f&&f.calls=="CRSSD");
    assert(!f.get<unsigned>(0x94)&&f.clears==1&&f.removes==1);
    f.calls.clear();f.put(0xb8,0x800u);
    assert(api->poison(f.actor,sizeof(f.actor),&b,&o)&&o.variant==2&&!o.state_cleared&&o.refreshes==1&&o.effects==1&&o.sounds==2&&o.removed&&f.calls=="RSSOED");
    assert(f.clears==1&&f.removes==2&&f.effects==1);
    f.calls.clear();f.put(0xb8,0u);
    // Without stale origin, Poison retains the prior correct origin.
    assert(api->poison(f.actor,sizeof(f.actor),&b,&o)&&o.refreshes==0&&o.effect.origin[0]==4.f&&f.calls=="RSSED");
    auto bad=b;bad.poison_effect=nullptr;f.calls.clear();
    assert(!api->poison(f.actor,sizeof(f.actor),&bad,&o)&&!o.removed&&o.sounds==2);
    f.calls.clear();f.draw=111.f;
    assert(!api->mutant(f.actor,sizeof(f.actor),&b,&o)&&!o.sounds&&!o.removed&&f.calls=="R");
    assert(!api->mutant(f.actor,0x97,&b,&o));
    assert(!api->poison(f.actor,0x2c7,&b,&o));
    FreeLibrary(server);FreeLibrary(client);return 0;
}
