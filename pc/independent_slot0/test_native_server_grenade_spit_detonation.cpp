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
#include <limits>
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

    // Host contract: abort at the first failed service, report completed
    // stages, and never remove an entity following an earlier failure.
    // These are bridge-level guarantees, not additional retail decompilation.
    auto reset=[&](unsigned state,unsigned flags){
        f=F{};
        f.put(0x94,state);f.put(0xb8,flags);
        f.put(0x2bc,4.f);f.put(0x2c0,5.f);f.put(0x2c4,6.f);
        o={};
    };
    auto incomplete=[&](const char* trace){
        assert(!o.completed&&!o.removed&&!f.removes&&f.calls==trace);
    };
    reset(5u,0u);
    bad=b;
    bad.clear_state=[](void* ctx,void*) noexcept ->int{
        static_cast<F*>(ctx)->calls+='C';return 0;
    };
    assert(!api->mutant(f.actor,sizeof(f.actor),&bad,&o));
    incomplete("C");
    assert(f.get<unsigned>(0x94)==5u&&!o.state_cleared&&!o.random_calls);

    reset(5u,0u);
    bad=b;
    bad.random_float=[](void* ctx,float,float,float*) noexcept ->int{
        static_cast<F*>(ctx)->calls+='R';return 0;
    };
    assert(!api->mutant(f.actor,sizeof(f.actor),&bad,&o));
    incomplete("CR");
    assert(!f.get<unsigned>(0x94)&&o.state_cleared&&o.random_calls==1&&!o.sounds);

    reset(0u,0x800u);
    bad=b;
    bad.sound=[](void* ctx,void*,const char* name,int,int) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);
        assert(!std::strcmp(name,"GrenadeSpit.Acid"));
        f.calls+='S';return 0;
    };
    assert(!api->poison(f.actor,sizeof(f.actor),&bad,&o));
    incomplete("RS");
    assert(!o.sounds&&!o.refreshes&&!o.effects);

    reset(0u,0x800u);
    bad=b;
    bad.sound=[](void* ctx,void*,const char* name,int,int) noexcept ->int{
        auto& f=*static_cast<F*>(ctx);
        f.calls+='S';
        if(!std::strcmp(name,"GrenadeSpit.Acid")){++f.sounds;return 1;}
        assert(!std::strcmp(name,"GrenadeSpit.Hit"));return 0;
    };
    assert(!api->poison(f.actor,sizeof(f.actor),&bad,&o));
    incomplete("RSS");
    assert(o.sounds==1&&!o.refreshes&&!o.effects);

    reset(0u,0x800u);
    bad=b;
    bad.refresh_origin=[](void* ctx,void*) noexcept ->int{
        static_cast<F*>(ctx)->calls+='O';return 0;
    };
    assert(!api->poison(f.actor,sizeof(f.actor),&bad,&o));
    incomplete("RSSO");
    assert(o.sounds==2&&o.refreshes==1&&!o.effects);

    reset(0u,0u);
    bad=b;
    bad.poison_effect=[](void* ctx,void*,const ServerGrenadeSpitPoisonEffect*) noexcept ->int{
        static_cast<F*>(ctx)->calls+='E';return 0;
    };
    assert(!api->poison(f.actor,sizeof(f.actor),&bad,&o));
    incomplete("RSSE");
    assert(o.sounds==2&&!o.refreshes&&!o.effects);

    reset(0u,0u);
    bad=b;
    bad.remove=[](void* ctx,void*) noexcept ->int{
        static_cast<F*>(ctx)->calls+='D';return 0;
    };
    assert(!api->poison(f.actor,sizeof(f.actor),&bad,&o));
    incomplete("RSSED");
    assert(o.sounds==2&&o.effects==1&&f.effects==1);

    // A non-finite origin must never be submitted to the effect provider.
    reset(0u,0u);
    f.put(0x2c4,std::numeric_limits<float>::quiet_NaN());
    assert(!api->poison(f.actor,sizeof(f.actor),&b,&o));
    incomplete("RSS");
    assert(!o.effects);

    // Both endpoints of the random contract and the exact accepted extents.
    for(float endpoint : {90.f,110.f}){
        reset(0u,0u);f.draw=endpoint;
        assert(api->mutant(f.actor,0x98,&b,&o)&&o.completed&&o.draw==endpoint&&f.calls=="RSSD");
        reset(0u,0u);f.draw=endpoint;
        assert(api->poison(f.actor,0x2c8,&b,&o)&&o.completed&&o.draw==endpoint&&f.calls=="RSSED");
    }
    reset(5u,0u);
    f.draw=std::numeric_limits<float>::quiet_NaN();
    assert(!api->mutant(f.actor,sizeof(f.actor),&b,&o));
    incomplete("CR");
    assert(o.state_cleared&&!o.sounds);
    reset(5u,0u);
    assert(!api->mutant(f.actor,sizeof(f.actor),&b,nullptr)&&f.calls.empty());
    assert(!api->mutant(nullptr,sizeof(f.actor),&b,&o)&&f.calls.empty());
    assert(!api->mutant(f.actor,sizeof(f.actor),nullptr,&o)&&f.calls.empty());
    assert(f.get<unsigned>(0x94)==5u);
    assert(!get(1,sizeof(ServerGrenadeSpitDetonationApi)-1));
    FreeLibrary(server);FreeLibrary(client);return 0;
}
