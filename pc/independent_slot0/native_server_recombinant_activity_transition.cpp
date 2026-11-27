#include "native_server_recombinant_activity_transition.h"
#include <cmath>
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerRecombinantRegistrationApi* __cdecl SiN_GetServerRecombinantRegistrationApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned at) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+at,sizeof(v));return v;}
template<class T>void wr(void* p,unsigned at,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+at,&v,sizeof(v));}
bool draw(const ServerRecombinantActivityTransitionBindings& b,float hi,ServerRecombinantActivityTransitionResult& o) noexcept {
    o.low=hi==1.1f?.9f:0.f;o.high=hi;
    ++o.random_calls;
    return b.random_float&&b.random_float(b.context,o.low,hi,&o.draw)&&
        std::isfinite(o.draw)&&o.draw>=o.low&&o.draw<=hi;
}
int __cdecl major(void* a,unsigned n,int activity,const ServerRecombinantActivityTransitionBindings* b,ServerRecombinantActivityTransitionResult* o) noexcept {
    if(!o)return 0;*o={};o->kind=RecombinantMajor;o->activity=activity;
    if(!a||n<0xc16||rd<unsigned>(a,0)!=recombinant_major_vtable||!b||!b->policy)return 0;
    o->previous=rd<int>(a,0x878);
    const auto* reg=SiN_GetServerRecombinantRegistrationApi(1,sizeof(ServerRecombinantRegistrationApi));
    const auto* ids=b->major_activities;
    if(!reg||!ids||ids->kind!=RecombinantMajor||!reg->valid(ids)||
       b->policy->activities!=ids->owner)return 0;
    if(activity==ids->ids[1]){
        o->frenzy=1;
        ++o->sound_calls;
        if(!b->policy->sound||!b->policy->sound(b->policy->context,a,"NPC_Leperkin.Frenzy",0,0))return 0;
        if(!draw(*b,1.1f,*o))return 0;
        ++o->scalar_calls;if(!b->set_frenzy_scalar||!b->set_frenzy_scalar(b->context,a,o->draw))return 0;
    }
    if(activity==29){
        wr<unsigned char>(a,0xc14,1);wr<unsigned char>(a,0xc15,0);o->latch_written=1;
        if(o->previous==27){wr<unsigned char>(a,0xc14,0);wr<unsigned char>(a,0xc15,0);o->latch_cleared=1;}
    }else{
        if(o->previous==29){wr<unsigned char>(a,0xc14,0);wr<unsigned char>(a,0xc15,0);o->latch_cleared=1;}
        if(activity==27){wr<unsigned char>(a,0xc14,1);wr<unsigned char>(a,0xc15,0);o->latch_written=1;}
    }
    o->latch_a=rd<unsigned char>(a,0xc14);o->latch_b=rd<unsigned char>(a,0xc15);
    o->completed=1;return 1;
}
int __cdecl minor(void* a,unsigned n,int activity,const ServerRecombinantActivityTransitionBindings* b,ServerRecombinantActivityTransitionResult* o) noexcept {
    if(!o)return 0;*o={};o->kind=RecombinantMinor;o->activity=activity;
    if(!a||n<4||rd<unsigned>(a,0)!=recombinant_minor_vtable||!b)return 0;
    float max=0;
    if(activity==1)max=.75f;
    else if(activity==10)max=.25f;
    else{ o->completed=1;return 1;}
    if(!draw(*b,max,*o))return 0;
    ++o->scalar_calls;
    if(!b->set_minor_scalar||!b->set_minor_scalar(b->context,a,o->draw))return 0;
    o->completed=1;return 1;
}
const ServerRecombinantActivityTransitionApi api{1,sizeof(ServerRecombinantActivityTransitionApi),&major,&minor};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantActivityTransitionApi=_SiN_GetServerRecombinantActivityTransitionApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantActivityTransitionApi* __cdecl SiN_GetServerRecombinantActivityTransitionApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;
    return v==1&&n==sizeof(ServerRecombinantActivityTransitionApi)?&api:nullptr;
}
