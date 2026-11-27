#include "native_server_recombinant_airborne.h"
#include <cmath>
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void wr(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
bool finite(const float* v) noexcept{return std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2]);}
struct Call{
    void* a;unsigned n;float now;
    const ServerRecombinantAirborneBindings& b;ServerRecombinantAirborneResult& o;
    const ServerRecombinantMinorTouchBindings& t;const ServerRecombinantMinorBindings& m;
    bool refresh(){
        if(!(rd<unsigned>(a,0xb8)&0x800u))return true;
        ++o.origin_refreshes;return b.refresh_origin&&b.refresh_origin(b.context,a);
    }
    bool install(unsigned marker){
        o.think_marker=marker;
        if(!t.install_think||!t.install_think(t.context,a,marker,0,0))return false;
        o.think_installed=1;return true;
    }
    bool schedule(float when){
        o.next_think=when;
        if(!std::isfinite(when)||!m.schedule_touch_think||!m.schedule_touch_think(m.context,a,when,0))return false;
        o.think_scheduled=1;return true;
    }
    bool launch(const float v[3]){
        if(!finite(v))return false;std::memcpy(o.velocity,v,12);
        wr(a,0x7c,recombinant_minor_contact_marker);o.contact_marker=recombinant_minor_contact_marker;
        const auto* ca=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));ServerNpcConditionResult cr{};
        if(!m.damage||!m.damage->policy||!ca||!ca->store(a,n,58,NpcConditionCurrent,true,&m.damage->policy->conditions,&cr))return false;
        ++o.condition_writes;
        if(!b.clear_ground||!b.clear_ground(b.context,a))return false;o.ground_cleared=1;
        o.contact_deadline=static_cast<float>(double(now)+.5);
        if(!std::isfinite(o.contact_deadline))return false;wr(a,0xc38,o.contact_deadline);
        std::memcpy(o.relocation,static_cast<unsigned char*>(a)+0x360,12);
        o.relocation[2]=static_cast<float>(double(o.relocation[2])+1.);
        if(!finite(o.relocation)||!b.relocate||!b.relocate(b.context,a,o.relocation,0))return false;o.relocated=1;
        if(!m.set_velocity||!m.set_velocity(m.context,a,o.velocity))return false;o.velocity_set=1;
        if(!refresh())return false;
        std::memcpy(o.snapshot,static_cast<unsigned char*>(a)+0x2bc,12);
        if(!finite(o.snapshot))return false;
        std::memcpy(static_cast<unsigned char*>(a)+0xc24,o.snapshot,12);
        wr<unsigned char>(a,0xc41,1);o.attached=1;
        return install(recombinant_airborne_think_marker)&&schedule(now);
    }
    bool leap(bool straight,const float* point){
        float v[3]{};
        if(straight){
            float angles[3]{};std::memcpy(angles,static_cast<unsigned char*>(a)+0x36c,12);
            ++o.forward_calls;if(!finite(angles)||!b.forward||!b.forward(b.context,angles,v)||!finite(v))return false;
            for(float& x:v)x=static_cast<float>(double(x)*350.);
        }else{
            if(!point||!finite(point)||!std::isfinite(b.gravity)||!refresh())return false;
            float origin[3]{};std::memcpy(origin,static_cast<unsigned char*>(a)+0x2bc,12);
            o.gravity=b.gravity<1.f?1.f:b.gravity;++o.ballistic_calls;
            if(!finite(origin)||!b.ballistic||!b.ballistic(b.context,origin,point,o.gravity,3.f,v))return false;
        }
        return launch(v);
    }
    bool think(){
        o.ai_deadline=rd<float>(a,0xc34);
        if(!std::isfinite(o.ai_deadline))return false;
        if(now>o.ai_deadline){
            if(!b.ai_update||!b.ai_update(b.context,a))return false;++o.ai_updates;
            o.ai_deadline=static_cast<float>(double(now)+.1);
            if(!std::isfinite(o.ai_deadline))return false;wr(a,0xc34,o.ai_deadline);
        }
        o.grounded=(rd<unsigned char>(a,0xbc)&1)!=0;
        if(o.grounded)return install(recombinant_default_think_marker)&&schedule(static_cast<float>(double(now)+.1));
        return schedule(now);
    }
};
bool init(void* a,unsigned n,float now,const ServerRecombinantAirborneBindings* b,ServerRecombinantAirborneResult* o){
    if(!o)return false;*o={};
    return a&&n>=0xc48&&rd<unsigned>(a,0)==recombinant_minor_vtable&&std::isfinite(now)&&b&&b->touch&&b->touch->minor;
}
int __cdecl leap(void* a,unsigned n,float now,bool straight,const float point[3],const ServerRecombinantAirborneBindings* b,ServerRecombinantAirborneResult* o) noexcept{
    if(!init(a,n,now,b,o))return 0;
    Call c{a,n,now,*b,*o,*b->touch,*b->touch->minor};o->completed=c.leap(straight,point);return o->completed;
}
int __cdecl launch_velocity(void* a,unsigned n,float now,const float v[3],const ServerRecombinantAirborneBindings* b,ServerRecombinantAirborneResult* o) noexcept{
    if(!init(a,n,now,b,o)||!v)return 0;
    Call c{a,n,now,*b,*o,*b->touch,*b->touch->minor};o->completed=c.launch(v);return o->completed;
}
int __cdecl think(void* a,unsigned n,float now,const ServerRecombinantAirborneBindings* b,ServerRecombinantAirborneResult* o) noexcept{
    if(!init(a,n,now,b,o))return 0;
    Call c{a,n,now,*b,*o,*b->touch,*b->touch->minor};o->completed=c.think();return o->completed;
}
const ServerRecombinantAirborneApi api{1,sizeof(ServerRecombinantAirborneApi),&leap,&launch_velocity,&think};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantAirborneApi=_SiN_GetServerRecombinantAirborneApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantAirborneApi* __cdecl SiN_GetServerRecombinantAirborneApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerRecombinantAirborneApi)?&api:nullptr;
}
