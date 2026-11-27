#include "native_server_grenade_spit_detonation.h"
#include <cmath>
#include <cstring>
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned at) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+at,sizeof(v));return v;}
template<class T>void wr(void* p,unsigned at,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+at,&v,sizeof(v));}
bool finite3(const float v[3]) noexcept{return std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2]);}
int run(void* a,unsigned n,bool poison,const ServerGrenadeSpitDetonationBindings* b,ServerGrenadeSpitDetonationResult* o) noexcept {
    if(!o)return 0;*o={};o->variant=poison?2:1;
    if(!a||n<(poison?0x2c8u:0x98u)||!b)return 0;
    if(rd<unsigned>(a,0x94)!=0){
        if(!b->clear_state||!b->clear_state(b->context,a))return 0;
        wr(a,0x94,0u);o->state_cleared=1;
    }
    // Both exported methods draw [90,110] and discard the result,
    // before emitting the two sound events in this exact order.
    ++o->random_calls;
    if(!b->random_float||!b->random_float(b->context,90.f,110.f,&o->draw)||
       !std::isfinite(o->draw)||o->draw<90.f||o->draw>110.f)return 0;
    auto sound=[&](const char* name){
        if(!b->sound||!b->sound(b->context,a,name,0,0))return false;
        ++o->sounds;return true;
    };
    if(!sound("GrenadeSpit.Acid")||!sound("GrenadeSpit.Hit"))return 0;
    if(poison){
        if(rd<unsigned>(a,0xb8)&0x800u){
            ++o->refreshes;
            if(!b->refresh_origin||!b->refresh_origin(b->context,a))return 0;
        }
        auto& fx=o->effect;
        std::memcpy(fx.origin,static_cast<const unsigned char*>(a)+0x2bc,12);
        fx.arg0=10.f;fx.arg1=175.f;fx.arg2=10.f;fx.arg3=-1.f;fx.mode=0;
        if(!finite3(fx.origin)||!b->poison_effect||!b->poison_effect(b->context,a,&fx))return 0;
        o->effects=1;
    }
    if(!b->remove||!b->remove(b->context,a))return 0;
    o->removed=o->completed=1;return 1;
}
int __cdecl mutant(void* a,unsigned n,const ServerGrenadeSpitDetonationBindings* b,ServerGrenadeSpitDetonationResult* o) noexcept{
    return run(a,n,false,b,o);
}
int __cdecl poison(void* a,unsigned n,const ServerGrenadeSpitDetonationBindings* b,ServerGrenadeSpitDetonationResult* o) noexcept{
    return run(a,n,true,b,o);
}
const ServerGrenadeSpitDetonationApi api{1,sizeof(ServerGrenadeSpitDetonationApi),&mutant,&poison};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerGrenadeSpitDetonationApi=_SiN_GetServerGrenadeSpitDetonationApi")
extern "C" const sin_port::native_weapon_tables::ServerGrenadeSpitDetonationApi* __cdecl SiN_GetServerGrenadeSpitDetonationApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;
    return v==1&&n==sizeof(ServerGrenadeSpitDetonationApi)?&api:nullptr;
}
