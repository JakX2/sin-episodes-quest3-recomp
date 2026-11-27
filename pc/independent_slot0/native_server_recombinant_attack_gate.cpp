#include "native_server_recombinant_attack_gate.h"
#include <cmath>
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
bool finite(const float* v) noexcept{return std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2]);}
struct Call{
    void* a;unsigned n;
    const ServerRecombinantAttackGateBindings& b;
    ServerRecombinantAttackGateResult& o;
    const ServerRecombinantEventBindings& e;
    bool enemy(void*& v){++o.enemy_queries;return e.enemy&&e.enemy(e.context,a,&v);}
    bool refresh(){
        if(!(rd<unsigned>(a,0xb8)&0x800u))return true;
        ++o.refreshes;return b.refresh_origin&&b.refresh_origin(b.context,a);
    }
    bool target(void* entity,float v[3]){
        ++o.target_queries;return e.target_point&&e.target_point(e.context,entity,v)&&finite(v);
    }
    bool origin(void* entity,float v[3]){
        ++o.origin_calls;return b.origin&&b.origin(b.context,entity,v)&&finite(v);
    }
    bool range(float distance){
        if(distance<0){o.condition=36;return false;}
        if(distance>350){o.condition=37;return false;}
        return true;
    }
    bool spit(float now,float distance){
        void* entity=nullptr;if(!enemy(entity))return false;if(!entity)return true;
        const auto* ca=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));ServerNpcConditionResult cr{};
        ++o.condition_tests;
        if(!ca||!ca->test(a,n,10,NpcConditionCurrent,&e.policy->conditions,&cr))return false;
        if(!cr.value||!range(distance))return true;
        o.deadline=rd<float>(a,0xc3c);if(!std::isfinite(o.deadline))return false;
        if(now<o.deadline)return true;
        float own[3]{},other[3]{};
        if(!origin(a,own)||!enemy(entity)||!entity||!origin(entity,other))return false;
        o.height=static_cast<float>(std::fabs(double(own[2])-other[2]));
        if(o.height<=256.f){o.condition=21;o.accepted=1;}
        return true;
    }
    bool leap(float now,float distance){
        o.deadline=rd<float>(a,0x60c);if(!std::isfinite(o.deadline))return false;
        if(now<o.deadline)return true;
        o.grounded=(rd<unsigned char>(a,0xbc)&1)!=0;
        if(!o.grounded||!range(distance))return true;
        void* entity=nullptr;
        if(!enemy(entity))return false;if(!entity)return true;
        if(!refresh())return false;
        float v[3]{};
        if(!target(entity,v))return false;
        const float own=rd<float>(a,0x2c4);if(!std::isfinite(own))return false;
        o.height=static_cast<float>(std::fabs(double(v[2])-own));
        if(o.height>150.f)return true;
        bool cast=false;++o.casts;if(!b.enemy_cast||!b.enemy_cast(b.context,entity,&cast))return false;
        const unsigned hull=rd<unsigned>(a,0x610);
        ++o.hull_calls;if(!b.hull_mins||!b.hull_mins(b.context,hull,o.trace.mins)||!finite(o.trace.mins)||!refresh())return false;
        ++o.hull_calls;if(!b.hull_maxs||!b.hull_maxs(b.context,hull,o.trace.maxs)||!finite(o.trace.maxs))return false;
        // 2237af80 target helper, two enemy queries and +120 witness.
        if(!enemy(entity)||!entity||!enemy(entity)||!entity)return false;
        bool special=false;if(!e.special||!e.special(e.context,entity,&special))return false;
        if(special){if(!target(entity,o.trace.end))return false;}
        else{
            ++o.target_queries;
            if(!e.center||!e.center(e.context,entity,o.trace.end)||!finite(o.trace.end))return false;
            o.trace.end[2]=static_cast<float>(double(o.trace.end[2])+10.);
        }
        if(!e.player_probe||!e.player_probe(e.context,entity))return false;
        std::memcpy(o.trace.start,static_cast<const unsigned char*>(a)+0x2bc,12);
        if(!finite(o.trace.start)||!finite(o.trace.end))return false;
        o.trace.mask=0x200400bu;o.trace.group=0;
        void* hit=nullptr;++o.traces;
        if(!b.trace||!b.trace(b.context,a,&o.trace,&hit)||!enemy(entity))return false;
        bool allowed=hit==entity;
        if(!allowed&&hit){++o.held_calls;if(!b.held_object||!b.held_object(b.context,hit,&allowed))return false;}
        if(!allowed&&cast&&!hit)allowed=true;
        if(allowed){o.condition=20;o.accepted=1;}
        return true;
    }
};
int execute(void* a,unsigned n,float now,float distance,bool leap,const ServerRecombinantAttackGateBindings* b,ServerRecombinantAttackGateResult* o) noexcept{
    if(!o)return 0;*o={};
    if(!a||n<0xc48||rd<unsigned>(a,0)!=recombinant_minor_vtable||!std::isfinite(now)||!std::isfinite(distance)||
       !b||!b->event||!b->event->policy)return 0;
    Call c{a,n,*b,*o,*b->event};o->completed=leap?c.leap(now,distance):c.spit(now,distance);return o->completed;
}
int __cdecl leap(void* a,unsigned n,float now,float distance,const ServerRecombinantAttackGateBindings* b,ServerRecombinantAttackGateResult* o) noexcept{return execute(a,n,now,distance,true,b,o);}
int __cdecl spit(void* a,unsigned n,float now,float distance,const ServerRecombinantAttackGateBindings* b,ServerRecombinantAttackGateResult* o) noexcept{return execute(a,n,now,distance,false,b,o);}
const ServerRecombinantAttackGateApi api{1,sizeof(ServerRecombinantAttackGateApi),&leap,&spit};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantAttackGateApi=_SiN_GetServerRecombinantAttackGateApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantAttackGateApi* __cdecl SiN_GetServerRecombinantAttackGateApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerRecombinantAttackGateApi)?&api:nullptr;
}
