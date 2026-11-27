#include "native_server_recombinant_pounce.h"
#include <cmath>
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerRecombinantAirborneApi* __cdecl SiN_GetServerRecombinantAirborneApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void wr(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
bool finite(const float* v) noexcept{return std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2]);}
struct Call{
    void* a;unsigned n;
    const ServerRecombinantPounceBindings& b;ServerRecombinantPounceResult& o;
    const ServerRecombinantAirborneBindings& air;
    const ServerRecombinantMinorTouchBindings& t;
    const ServerRecombinantMinorBindings& m;
    bool refresh_origin(){
        if(!(rd<unsigned>(a,0xb8)&0x800u))return true;
        if(!air.refresh_origin||!air.refresh_origin(air.context,a))return false;++o.origin_refreshes;return true;
    }
    bool refresh_velocity(){
        if(!(rd<unsigned>(a,0xb8)&0x1000u))return true;
        if(!m.refresh_velocity||!m.refresh_velocity(m.context,a))return false;++o.velocity_refreshes;return true;
    }
    bool off(float now){
        if(!t.set_movement||!t.set_movement(t.context,a,3,0))return false;o.movement_set=1;
        void* e=nullptr;
        if(!b.event->enemy||!b.event->enemy(b.event->context,a,&e))return false;o.enemy_live=e!=nullptr;
        if(!e){
            const auto* api=SiN_GetServerRecombinantAirborneApi(1,sizeof(ServerRecombinantAirborneApi));
            return api&&api->leap(a,n,now,true,nullptr,&air,&o.fallback);
        }
        // Original uses the cached vectors, not a new enemy-position query.
        for(unsigned i=0;i<3;++i){
            o.snapshot[i]=rd<float>(a,0xc24+4*i);
            const float previous=rd<float>(a,0xc18+4*i);
            if(!std::isfinite(previous)||!std::isfinite(o.snapshot[i]))return false;
            o.direction[i]=static_cast<float>(double(o.snapshot[i])-previous);
            wr(a,0xc18+4*i,o.snapshot[i]);
        }
        const float sq=static_cast<float>(double(o.direction[0])*o.direction[0]+double(o.direction[1])*o.direction[1]);
        if(!std::isfinite(sq))return false;
        const double len=std::sqrt(double(sq));
        if(len>0.){o.direction[0]=static_cast<float>(o.direction[0]/len);o.direction[1]=static_cast<float>(o.direction[1]/len);}
        else o.direction[0]=o.direction[1]=0;
        auto draw=[&](unsigned i,float lo,float hi){
            ++o.random_calls;return b.event->random&&b.event->random(b.event->context,lo,hi,&o.draws[i])&&std::isfinite(o.draws[i])&&o.draws[i]>=lo&&o.draws[i]<=hi;
        };
        if(!draw(0,.5f,1.f)||!draw(1,-1.f,1.f))return false;
        const float x=o.direction[0],y=o.direction[1],side=o.draws[1];
        o.direction[0]=static_cast<float>(double(x)-double(y)*side);
        const float sideways=static_cast<float>(double(x)*side);
        o.direction[1]=static_cast<float>(double(y)+sideways);o.direction[2]=o.draws[0];
        if(!b.normalize||!b.normalize(b.context,o.direction)||!finite(o.direction))return false;o.normalized=1;
        for(unsigned i=0;i<3;++i)o.velocity[i]=static_cast<float>(double(o.direction[i])*250.);
        float yaw=0;++o.yaw_calls;
        if(!finite(o.velocity)||!b.yaw||!b.yaw(b.context,o.velocity,&yaw)||!std::isfinite(yaw))return false;
        o.yaw=static_cast<float>(double(yaw)-180.);
        ++o.motor_calls;const unsigned motor=rd<unsigned>(a,0x908);
        if(!motor||!b.motor_yaw||!b.motor_yaw(b.context,motor,o.yaw,-1.f))return false;
        wr(a,0x7c,recombinant_minor_detached_contact_marker);
        if(!air.clear_ground||!air.clear_ground(air.context,a))return false;o.ground_cleared=1;
        o.deadline=static_cast<float>(double(now)+.5);
        if(!std::isfinite(o.deadline))return false;wr(a,0xc38,o.deadline);
        if(!m.set_velocity||!m.set_velocity(m.context,a,o.velocity))return false;o.velocity_set=1;
        if(!refresh_origin())return false;
        std::memcpy(o.snapshot,static_cast<unsigned char*>(a)+0x2bc,12);
        if(!finite(o.snapshot))return false;
        std::memcpy(static_cast<unsigned char*>(a)+0xc24,o.snapshot,12);wr<unsigned char>(a,0xc41,1);
        if(!t.install_think||!t.install_think(t.context,a,recombinant_airborne_think_marker,0,0))return false;o.think_installed=1;
        if(!m.schedule_touch_think||!m.schedule_touch_think(m.context,a,now,0))return false;o.think_scheduled=1;
        return true;
    }
    bool slash(void* target){
        if(!target||!std::isfinite(b.slash_damage))return false;
        alignas(4) unsigned char info[0x48]{};
        o.damage=b.slash_damage;o.damage_type=4;
        if(!b.damage_info||!b.damage_info(b.context,a,o.damage,o.damage_type,info,sizeof(info)))return false;o.damage_prepared=1;
        if(!refresh_velocity()||!refresh_origin()||!refresh_velocity())return false;
        std::memcpy(o.origin,static_cast<unsigned char*>(a)+0x2bc,12);
        std::memcpy(o.velocity,static_cast<unsigned char*>(a)+0x250,12);
        if(!finite(o.origin)||!finite(o.velocity)||!b.force||!b.force(b.context,info,sizeof(info),a,o.velocity,o.origin,1.f))return false;o.force_calls=1;
        if(!b.apply_damage||!b.apply_damage(b.context,target,info,sizeof(info)))return false;o.damage_calls=1;
        return true;
    }
};
bool init(void* a,unsigned n,const ServerRecombinantPounceBindings* b,ServerRecombinantPounceResult* o){
    if(!o)return false;*o={};
    return a&&n>=0xc48&&rd<unsigned>(a,0)==recombinant_minor_vtable&&b&&b->event&&b->airborne&&b->airborne->touch&&b->airborne->touch->minor;
}
int __cdecl off(void* a,unsigned n,float now,const ServerRecombinantPounceBindings* b,ServerRecombinantPounceResult* o) noexcept{
    if(!init(a,n,b,o)||!std::isfinite(now))return 0;
    Call c{a,n,*b,*o,*b->airborne,*b->airborne->touch,*b->airborne->touch->minor};o->completed=c.off(now);return o->completed;
}
int __cdecl slash(void* a,unsigned n,void* target,const ServerRecombinantPounceBindings* b,ServerRecombinantPounceResult* o) noexcept{
    if(!init(a,n,b,o))return 0;
    Call c{a,n,*b,*o,*b->airborne,*b->airborne->touch,*b->airborne->touch->minor};o->completed=c.slash(target);return o->completed;
}
const ServerRecombinantPounceApi api{1,sizeof(ServerRecombinantPounceApi),&off,&slash};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantPounceApi=_SiN_GetServerRecombinantPounceApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantPounceApi* __cdecl SiN_GetServerRecombinantPounceApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerRecombinantPounceApi)?&api:nullptr;
}
