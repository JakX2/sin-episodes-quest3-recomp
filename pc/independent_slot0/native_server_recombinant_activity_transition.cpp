#include "native_server_recombinant_activity_transition.h"
#include <cmath>
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerRecombinantRegistrationApi* __cdecl SiN_GetServerRecombinantRegistrationApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerNpcActivityAssignmentApi* __cdecl SiN_GetServerNpcActivityAssignmentApi(unsigned,unsigned) noexcept;
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
struct AssignmentFrame {
    void* actor;
    unsigned bytes;
    bool is_major;
    const ServerNpcActivityAssignmentBindings& original;
    const ServerRecombinantActivityTransitionBindings& transition;
    ServerRecombinantActivityAssignmentResult& result;
};
template<class Invoke>
int compose(void* a,unsigned n,const ServerRecombinantActivityAssignmentBindings* b,
    ServerRecombinantActivityAssignmentResult* o,Invoke invoke) noexcept {
    if(!o)return 0;*o={};
    if(!a||n<0x90cu||!b||!b->assignment||!b->transition||
       b->assignment->translation.actor_class!=NpcActivityHosted)return 0;
    const unsigned vt=rd<unsigned>(a,0);
    if(vt!=recombinant_minor_vtable&&vt!=recombinant_major_vtable)return 0;
    const bool is_major=vt==recombinant_major_vtable;
    if(is_major){
        if(n<0xc16u||!b->transition->policy)return 0;
        const auto* ids=b->transition->major_activities;
        const auto* reg=SiN_GetServerRecombinantRegistrationApi(1,sizeof(ServerRecombinantRegistrationApi));
        if(!reg||!ids||ids->kind!=RecombinantMajor||!reg->valid(ids)||
           b->transition->policy->activities!=ids->owner)return 0;
    }
    const auto* assignment=SiN_GetServerNpcActivityAssignmentApi(1,sizeof(ServerNpcActivityAssignmentApi));
    if(!assignment)return 0;
    AssignmentFrame frame{a,n,is_major,*b->assignment,*b->transition,*o};
    auto bound=frame.original;
    // Only the assignment's top-level context is rebound. Translation,
    // sequence reset and dictionary each retain their separate contexts.
    bound.context=&frame;
    bound.weapon_notify=[](void* p,void* a,std::uint32_t h,int act,float duration) noexcept ->int{
        auto& f=*static_cast<AssignmentFrame*>(p);
        return f.actor==a&&f.original.weapon_notify&&f.original.weapon_notify(f.original.context,a,h,act,duration);
    };
    bound.capabilities=[](void* p,void* a,unsigned* out) noexcept ->int{
        auto& f=*static_cast<AssignmentFrame*>(p);
        return f.actor==a&&f.original.capabilities&&f.original.capabilities(f.original.context,a,out);
    };
    bound.component_notify=[](void* p,void* a,std::uint32_t c) noexcept ->int{
        auto& f=*static_cast<AssignmentFrame*>(p);
        return f.actor==a&&f.original.component_notify&&f.original.component_notify(f.original.context,a,c);
    };
    bound.locomotion=[](void* p,void* a,int act,bool* out) noexcept ->int{
        auto& f=*static_cast<AssignmentFrame*>(p);
        return f.actor==a&&f.original.locomotion&&f.original.locomotion(f.original.context,a,act,out);
    };
    bound.view_vector=[](void* p,void* a,int act,float* out) noexcept ->int{
        auto& f=*static_cast<AssignmentFrame*>(p);
        return f.actor==a&&f.original.view_vector&&f.original.view_vector(f.original.context,a,act,out);
    };
    bound.view_dirty=[](void* p,void* a) noexcept ->int{
        auto& f=*static_cast<AssignmentFrame*>(p);
        return f.actor==a&&f.original.view_dirty&&f.original.view_dirty(f.original.context,a);
    };
    bound.activity_changed=[](void* p,void* a,int act) noexcept ->int{
        auto& f=*static_cast<AssignmentFrame*>(p);
        if(f.actor!=a)return 0;
        ++f.result.transition_calls;
        return f.is_major?major(a,f.bytes,act,&f.transition,&f.result.transition):
            minor(a,f.bytes,act,&f.transition,&f.result.transition);
    };
    return invoke(*assignment,bound,o->assignment);
}
int __cdecl composed_assign(void* a,unsigned n,const ServerNpcActivityAssignmentInput* in,
    const ServerRecombinantActivityAssignmentBindings* b,ServerRecombinantActivityAssignmentResult* o) noexcept {
    return compose(a,n,b,o,[&](const ServerNpcActivityAssignmentApi& api,const ServerNpcActivityAssignmentBindings& bound,ServerNpcActivityAssignmentResult& r){
        return api.assign(a,n,in,&bound,&r);
    });
}
int __cdecl composed_request(void* a,unsigned n,int in,
    const ServerRecombinantActivityAssignmentBindings* b,ServerRecombinantActivityAssignmentResult* o) noexcept {
    return compose(a,n,b,o,[&](const ServerNpcActivityAssignmentApi& api,const ServerNpcActivityAssignmentBindings& bound,ServerNpcActivityAssignmentResult& r){
        return api.request(a,n,in,&bound,&r);
    });
}
const ServerRecombinantActivityAssignmentApi composed_api{1,sizeof(ServerRecombinantActivityAssignmentApi),&composed_assign,&composed_request};

}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantActivityTransitionApi=_SiN_GetServerRecombinantActivityTransitionApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantActivityTransitionApi* __cdecl SiN_GetServerRecombinantActivityTransitionApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;
    return v==1&&n==sizeof(ServerRecombinantActivityTransitionApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantActivityAssignmentApi=_SiN_GetServerRecombinantActivityAssignmentApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantActivityAssignmentApi* __cdecl SiN_GetServerRecombinantActivityAssignmentApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;
    return v==1&&n==sizeof(ServerRecombinantActivityAssignmentApi)?&composed_api:nullptr;
}
