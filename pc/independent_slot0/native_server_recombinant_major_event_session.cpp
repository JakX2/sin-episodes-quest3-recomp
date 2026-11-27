#include "native_server_recombinant_major_event_session.h"
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerRecombinantMajorEventsApi* __cdecl SiN_GetServerRecombinantMajorEventsApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerRecombinantMeleeApi* __cdecl SiN_GetServerRecombinantMeleeApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned at) noexcept{T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+at,sizeof(v));return v;}
struct Frame {
    void* actor;
    unsigned bytes;
    const ServerRecombinantMajorEventSessionBindings& bound;
    ServerRecombinantMajorEventSessionResult& result;
    const ServerRecombinantMajorEventBindings& source() const noexcept{return *bound.events;}
};
int __cdecl basis(void* ctx,const float angle[3],float fwd[3],float right[3]) noexcept{
    auto& f=*static_cast<Frame*>(ctx);const auto& b=f.source();
    return b.basis&&b.basis(b.context,angle,fwd,right);
}
int __cdecl forward(void* ctx,const float angle[3],float vec[3]) noexcept{
    auto& f=*static_cast<Frame*>(ctx);const auto& b=f.source();
    return b.forward&&b.forward(b.context,angle,vec);
}
int __cdecl enemy(void* ctx,void* actor,void** out) noexcept{
    auto& f=*static_cast<Frame*>(ctx);const auto& b=f.source();
    return b.enemy&&b.enemy(b.context,actor,out);
}
int __cdecl player(void* ctx,void* entity,bool* out) noexcept{
    auto& f=*static_cast<Frame*>(ctx);const auto& b=f.source();
    return b.is_player&&b.is_player(b.context,entity,out);
}
int __cdecl melee(void* ctx,void* actor,const ServerRecombinantMajorMeleeRequest* request,void** hit) noexcept{
    auto& f=*static_cast<Frame*>(ctx);const auto& b=f.source();
    if(actor!=f.actor||!hit)return 0;*hit=nullptr;
    if(!f.bound.melee) {
        ++f.result.delegated_melee;
        return b.melee&&b.melee(b.context,actor,request,hit);
    }
    const auto* api=SiN_GetServerRecombinantMeleeApi(1,sizeof(ServerRecombinantMeleeApi));
    ++f.result.melee_invocations;
    if(!api||!api->execute(actor,f.bytes,request,f.bound.melee,&f.result.melee))return 0;
    *hit=f.result.melee.hit;return 1;
}
int __cdecl parent(void* ctx,void* actor,const void* event,unsigned bytes) noexcept{
    auto& f=*static_cast<Frame*>(ctx);const auto& b=f.source();
    return b.parent&&b.parent(b.context,actor,event,bytes);
}
int __cdecl execute(void* a,unsigned n,const void* record,unsigned event_bytes,
    const ServerRecombinantMajorEventSessionBindings* bindings,ServerRecombinantMajorEventSessionResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!a||n<0xc48||rd<unsigned>(a,0)!=recombinant_major_vtable||
       !record||event_bytes<4||!bindings||!bindings->events||
       !bindings->events->policy||!bindings->events->registration||
       (bindings->melee&&bindings->melee->event!=bindings->events))return 0;
    const auto* api=SiN_GetServerRecombinantMajorEventsApi(1,sizeof(ServerRecombinantMajorEventsApi));
    if(!api||!api->valid(bindings->events->registration))return 0;
    Frame f{a,n,*bindings,*out};
    auto wrapped=*bindings->events;
    wrapped.context=&f;wrapped.basis=&basis;wrapped.forward=&forward;
    wrapped.enemy=&enemy;wrapped.is_player=&player;wrapped.melee=&melee;wrapped.parent=&parent;
    out->completed=api->major(a,n,record,event_bytes,&wrapped,&out->event);
    return out->completed;
}
const ServerRecombinantMajorEventSessionApi api{1,sizeof(ServerRecombinantMajorEventSessionApi),&execute};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantMajorEventSessionApi=_SiN_GetServerRecombinantMajorEventSessionApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantMajorEventSessionApi* __cdecl SiN_GetServerRecombinantMajorEventSessionApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;
    return v==1&&n==sizeof(ServerRecombinantMajorEventSessionApi)?&api:nullptr;
}
