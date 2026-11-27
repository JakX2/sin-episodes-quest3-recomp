#include "native_server_recombinant_major_events.h"
#include <cmath>
#include <cstring>
#include <limits>
extern "C" const sin_port::native_weapon_tables::ServerEventDictionaryApi* __cdecl SiN_GetServerEventDictionaryApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
constexpr ServerRecombinantEventDefinition defs[]{
    {"AE_MELEE_ATTACK_RIGHT_MEDIUM",0x2259d1ec,0x2237a67c},
    {"AE_MELEE_ATTACK_LEFT_MEDIUM",0x2259d1e4,0x2237a696},
    {"AE_MELEE_ATTACK_RIGHT_HEAVY",0x2259d1e8,0x2237a6b0},
    {"AE_MELEE_ATTACK_LEFT_HEAVY",0x2259d1e0,0x2237a6ca},
    {"AE_MELEE_ATTACK_RIGHT",0x2259d1d8,0x2237a6e4},
    {"AE_MELEE_ATTACK_LEFT",0x2259d1dc,0x2237a701},
    {"AE_ATTACK_BOTH",0x2259d1d4,0x2237a71b}
};
template<class T>T rd(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
bool finite(const float* v) noexcept{return std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2]);}
int __cdecl definitions(const ServerRecombinantEventDefinition** out,unsigned* n) noexcept{
    if(!out||!n)return 0;*out=defs;*n=7;return 1;
}
int __cdecl valid(const ServerRecombinantMajorEventRegistration* r) noexcept{
    if(!r||!r->ready||r->count!=7||!r->owner||r->generation!=r->owner->generation)return 0;
    const auto* api=SiN_GetServerEventDictionaryApi(1,sizeof(ServerEventDictionaryApi));if(!api)return 0;
    for(unsigned i=0;i<7;++i){
        int id=-1;if(!api->lookup(r->owner,defs[i].name,&id)||id<0||id!=r->ids[i])return 0;
        bool found=false;for(unsigned j=0;j<r->owner->count;++j)if(r->owner->entries[j].id==id){found=r->owner->entries[j].private_event;break;}
        if(!found)return 0;
    }
    return 1;
}
int __cdecl register_major(ServerEventDictionary* d,ServerRecombinantMajorEventRegistration* r) noexcept{
    if(!r)return 0;*r={};for(auto& id:r->ids)id=-1;
    const auto* api=SiN_GetServerEventDictionaryApi(1,sizeof(ServerEventDictionaryApi));if(!api||!d)return 0;
    r->owner=d;r->generation=d->generation;
    for(unsigned i=0;i<7;++i){if(!api->private_id(d,defs[i].name,&r->ids[i]))return 0;++r->count;}
    r->ready=1;if(!valid(r)){r->ready=0;return 0;}return 1;
}
int __cdecl major(void* a,unsigned n,const void* event,unsigned bytes,const ServerRecombinantMajorEventBindings* b,ServerRecombinantMajorEventResult* o) noexcept{
    if(!o)return 0;*o={};o->route=-1;
    if(!a||n<0x378||rd<unsigned>(a,0)!=recombinant_major_vtable||!event||bytes<4||!b||!b->policy||!valid(b->registration))return 0;
    o->event=rd<int>(event,0);
    for(int i=0;i<7;++i)if(b->registration->ids[i]==o->event){o->route=i;break;}
    if(o->route<0){o->delegated=1;o->completed=b->parent&&b->parent(b->context,a,event,bytes);return o->completed;}
    o->custom=1;
    const bool both=o->route==6,right=(o->route%2)==0,heavy=o->route==2||o->route==3;
    auto& q=o->request;
    const double damage=both?double(b->normal_damage)*2.:heavy?b->heavy_damage:b->normal_damage;
    if(!std::isfinite(damage)||damage<double(std::numeric_limits<int>::min())||damage>double(std::numeric_limits<int>::max()))return 0;
    q.damage=static_cast<int>(damage);q.side=both?2:right?1:0;
    std::memcpy(o->angles,static_cast<const unsigned char*>(a)+0x36c,12);
    if(!finite(o->angles))return 0;
    if(both){
        for(int& v:o->draws){++o->random_calls;if(!b->policy->random||!b->policy->random(b->policy->context,-2,2,&v)||v<-2||v>2)return 0;}
        q.angular[0]=5.f;q.angular[1]=float(o->draws[1]);q.angular[2]=float(o->draws[0]);
        ++o->forward_calls;if(!b->forward||!b->forward(b->context,o->angles,o->forward)||!finite(o->forward))return 0;
        for(unsigned i=0;i<3;++i)q.impulse[i]=static_cast<float>(double(o->forward[i])*200.);
    }else{
        q.angular[0]=-3.f;q.angular[1]=right?-5.f:5.f;q.angular[2]=-2.f;
        ++o->basis_calls;if(!b->basis||!b->basis(b->context,o->angles,o->forward,o->right)||!finite(o->forward)||!finite(o->right))return 0;
        for(unsigned i=0;i<3;++i){
            const float f=static_cast<float>(double(o->forward[i])*200.);
            const float s=static_cast<float>(double(o->right[i])*(right?100.:-100.));
            q.impulse[i]=static_cast<float>(double(f)+s);
        }
    }
    if(!finite(q.impulse))return 0;
    void* enemy=nullptr;++o->enemy_calls;if(!b->enemy||!b->enemy(b->context,a,&enemy))return 0;
    bool player=false;if(enemy){++o->player_calls;if(!b->is_player||!b->is_player(b->context,enemy,&player))return 0;}
    q.reach=player?85.f:65.f;
    void* hit=nullptr;++o->melee_calls;
    if(!b->melee||!b->melee(b->context,a,&q,&hit))return 0;
    o->hit=hit!=nullptr;o->completed=1;return 1;
}
const ServerRecombinantMajorEventsApi api{1,sizeof(ServerRecombinantMajorEventsApi),&definitions,&register_major,&valid,&major};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantMajorEventsApi=_SiN_GetServerRecombinantMajorEventsApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantMajorEventsApi* __cdecl SiN_GetServerRecombinantMajorEventsApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerRecombinantMajorEventsApi)?&api:nullptr;
}
