#include "native_server_recombinant_event_session.h"
#include <cmath>
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerRecombinantEventsApi* __cdecl SiN_GetServerRecombinantEventsApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerRecombinantAirborneApi* __cdecl SiN_GetServerRecombinantAirborneApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerRecombinantSpitApi* __cdecl SiN_GetServerRecombinantSpitApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T> T rd(const void* p,unsigned at) noexcept {
    T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+at,sizeof(v));return v;
}
struct Frame {
    void* actor;
    unsigned bytes;
    float now;
    const ServerRecombinantEventSessionBindings& bindings;
    ServerRecombinantEventSessionResult& result;
    const ServerRecombinantEventBindings& original() const noexcept {return *bindings.events;}
};
int __cdecl enemy(void* ctx,void* actor,void** out) noexcept {
    auto& f=*static_cast<Frame*>(ctx);const auto& b=f.original();
    return b.enemy&&b.enemy(b.context,actor,out);
}
int __cdecl special(void* ctx,void* e,bool* out) noexcept {
    auto& f=*static_cast<Frame*>(ctx);const auto& b=f.original();
    return b.special&&b.special(b.context,e,out);
}
int __cdecl target(void* ctx,void* e,float out[3]) noexcept {
    auto& f=*static_cast<Frame*>(ctx);const auto& b=f.original();
    return b.target_point&&b.target_point(b.context,e,out);
}
int __cdecl center(void* ctx,void* e,float out[3]) noexcept {
    auto& f=*static_cast<Frame*>(ctx);const auto& b=f.original();
    return b.center&&b.center(b.context,e,out);
}
int __cdecl probe(void* ctx,void* e) noexcept {
    auto& f=*static_cast<Frame*>(ctx);const auto& b=f.original();
    return b.player_probe&&b.player_probe(b.context,e);
}
int __cdecl launch(void* ctx,void* actor,int mode,const float point[3],int arg) noexcept {
    auto& f=*static_cast<Frame*>(ctx);
    if(actor!=f.actor||arg!=0||(mode!=0&&mode!=1))return 0;
    const auto* api=SiN_GetServerRecombinantAirborneApi(1,sizeof(ServerRecombinantAirborneApi));
    ++f.result.leap_invocations;
    return api&&api->leap(actor,f.bytes,f.now,mode==1,point,f.bindings.airborne,&f.result.airborne);
}
int __cdecl slash(void* ctx,void* actor,void* target) noexcept {
    auto& f=*static_cast<Frame*>(ctx);const auto& b=f.original();
    ++f.result.delegated_effects;
    return b.pounce_slash&&b.pounce_slash(b.context,actor,target);
}
int __cdecl angular(void* ctx,void* entity,const float value[3]) noexcept {
    auto& f=*static_cast<Frame*>(ctx);const auto& b=f.original();
    ++f.result.delegated_effects;
    return b.angular_effect&&b.angular_effect(b.context,entity,value);
}
int __cdecl off(void* ctx,void* actor) noexcept {
    auto& f=*static_cast<Frame*>(ctx);const auto& b=f.original();
    ++f.result.delegated_effects;
    return b.pounce_off&&b.pounce_off(b.context,actor);
}
int __cdecl spit(void* ctx,void* actor) noexcept {
    auto& f=*static_cast<Frame*>(ctx);
    if(actor!=f.actor)return 0;
    ++f.result.spit_invocations;
    const auto* api=SiN_GetServerRecombinantSpitApi(1,sizeof(ServerRecombinantSpitApi));
    return api&&api->execute(actor,f.bytes,f.now,f.bindings.spit,&f.result.spit);
}
int __cdecl random(void* ctx,float lo,float hi,float* out) noexcept {
    auto& f=*static_cast<Frame*>(ctx);const auto& b=f.original();
    return b.random&&b.random(b.context,lo,hi,out);
}
int __cdecl parent(void* ctx,void* actor,const void* event,unsigned bytes) noexcept {
    auto& f=*static_cast<Frame*>(ctx);const auto& b=f.original();
    return b.parent&&b.parent(b.context,actor,event,bytes);
}
int __cdecl execute(void* a,unsigned n,const void* event,unsigned event_bytes,float now,
    const ServerRecombinantEventSessionBindings* b,ServerRecombinantEventSessionResult* out) noexcept {
    if(!out)return 0;*out={};
    if(!a||n<0xc48||rd<unsigned>(a,0)!=recombinant_minor_vtable||
       !event||event_bytes<4||!std::isfinite(now)||!b||!b->events||!b->airborne||!b->spit)return 0;
    const auto& e=*b->events;
    const auto& air=*b->airborne;
    const auto& sp=*b->spit;
    if(!e.policy||!e.registration||!air.touch||!air.touch->minor||
       !air.touch->minor->damage||!sp.event||sp.event!=&e||
       air.touch->minor->damage->policy!=e.policy)return 0;
    const auto* api=SiN_GetServerRecombinantEventsApi(1,sizeof(ServerRecombinantEventsApi));
    if(!api||!api->valid(e.registration))return 0;
    Frame f{a,n,now,*b,*out};
    auto wrapped=e;
    wrapped.context=&f;
    wrapped.enemy=&enemy;wrapped.special=&special;
    wrapped.target_point=&target;wrapped.center=&center;
    wrapped.player_probe=&probe;wrapped.launch=&launch;
    wrapped.pounce_slash=&slash;wrapped.angular_effect=&angular;
    wrapped.pounce_off=&off;wrapped.spit=&spit;
    wrapped.random=&random;wrapped.parent=&parent;
    out->completed=api->minor(a,n,event,event_bytes,&wrapped,&out->event);
    return out->completed;
}
const ServerRecombinantEventSessionApi api{1,sizeof(ServerRecombinantEventSessionApi),&execute};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantEventSessionApi=_SiN_GetServerRecombinantEventSessionApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantEventSessionApi* __cdecl SiN_GetServerRecombinantEventSessionApi(unsigned v,unsigned n) noexcept {
    using namespace sin_port::native_weapon_tables;
    return v==1&&n==sizeof(ServerRecombinantEventSessionApi)?&api:nullptr;
}
