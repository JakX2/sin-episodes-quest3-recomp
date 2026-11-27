#include "native_server_recombinant_damage.h"
#include <cmath>
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerRecombinantPendingActivityApi* __cdecl SiN_GetServerRecombinantPendingActivityApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerRecombinantRegistrationApi* __cdecl SiN_GetServerRecombinantRegistrationApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void wr(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
bool major(void* a,unsigned n,unsigned extent) noexcept{return a&&n>=extent&&rd<unsigned>(a,0)==recombinant_major_vtable;}
int __cdecl ignite_major(void* a,unsigned n,const ServerRecombinantIgniteRequest* in,const ServerRecombinantDamageBindings* b,ServerRecombinantIgniteResult* o) noexcept {
    if(!o)return 0;*o={};o->activity=-1;
    if(!major(a,n,0x87c)||!in||!b||!std::isfinite(in->lifetime)||!b->parent_ignite||!b->random_float)return 0;
    if(!b->parent_ignite(b->context,a,in))return 0;o->parent_called=1;
    // 22379ee8 snapshots the current health as float after inherited Ignite.
    o->old_health=rd<int>(a,0x9c);
    auto draw=[&](double& limit){
        float x=0;
        if(!b->random_float(b->context,-2.f,2.f,&x)||!std::isfinite(x)||x<-2.f||x>2.f)return false;
        o->draws[o->random_calls++]=x;limit=(double(x)+10.)*5.;return true;
    };
    double limit=0;
    if(!draw(limit))return 0;
    if(limit>static_cast<float>(o->old_health))o->new_health=rd<int>(a,0x9c);
    else{
        // A second independent draw, not the first draw or a min().
        if(!draw(limit))return 0;o->new_health=static_cast<int>(limit);
    }
    if(rd<int>(a,0x9c)!=o->new_health){
        if(!b->health_changed||!b->health_changed(b->context,a))return 0;
        wr(a,0x9c,o->new_health);o->health_written=1;
    }
    const int activity=rd<int>(a,0x878);
    o->activity=activity==6?112:activity==10?113:activity==1?111:-1;
    if(o->activity!=-1){
        if(!b->pending_activity||!b->pending_activity(b->context,a,o->activity))return 0;o->pending_called=1;
    }
    o->completed=1;return 1;
}
int __cdecl damage_major(void* a,unsigned n,const void* info,unsigned bytes,float now,const ServerRecombinantDamageBindings* b,ServerRecombinantDamageResult* o) noexcept {
    if(!o)return 0;*o={};o->source_class=-1;o->ignition.activity=-1;
    if(!major(a,n,0xc44)||!info||bytes<0x3c||!b||!b->policy||!std::isfinite(now))return 0;
    const auto& p=*b->policy;
    if(p.entry_count>sin_independent::kEntityHandleCapacity||(!p.entries&&p.entry_count))return 0;
    const sin_independent::EntityHandleView registry(p.entries,p.entry_count);
    if(void* source=registry.Resolve(rd<unsigned>(info,0x24))){
        o->source_live=1;unsigned extent=0;
        if(!b->extent||!b->extent(b->context,source,&extent)||extent<4)return 0;
        const unsigned vt=rd<unsigned>(source,0);
        if(vt==0x2249f65cu||vt==recombinant_major_vtable||vt==recombinant_minor_vtable)o->source_class=10;
        else if(!b->classification||!b->classification(b->context,source,&o->source_class))return 0;
        if(o->source_class==10){o->immune=o->completed=1;return 1;}
    }
    const unsigned flags=rd<unsigned>(info,0x38);
    if((flags&8u)&&(rd<unsigned char>(a,0xbf)&8u)&&!(flags&0x20000000u)){
        o->immune=o->completed=1;return 1;
    }
    if(!b->parent_damage)return 0;
    if(flags&8u){
        if(!b->color_fade||!b->color_fade(b->context,a,5,50))return 0;o->color_called=1;
    }
    // 22379050 reads state again after the color operation. Only the
    // non-burning branch accumulates damage and refreshes the deadline.
    if(!(rd<unsigned char>(a,0xbf)&8u)&&(flags&8u)){
        const float amount=rd<float>(info,0x2c),prior=rd<float>(a,0xc3c);
        o->accumulated_damage=static_cast<float>(double(prior)+amount);
        o->deadline=static_cast<float>(double(now)+5.);
        if(!std::isfinite(amount)||!std::isfinite(prior)||!std::isfinite(o->accumulated_damage)||!std::isfinite(o->deadline))return 0;
        wr(a,0xc3c,o->accumulated_damage);wr(a,0xc40,o->deadline);o->accumulated=1;
        if(double(o->accumulated_damage)>=double(rd<int>(a,0x98))*.1){
            o->threshold_reached=1;
            const ServerRecombinantIgniteRequest request{100.f,1,0,0,0,0};
            if(!ignite_major(a,n,&request,b,&o->ignition))return 0;o->ignited=1;
        }
    }
    if(!b->parent_damage(b->context,a,info,bytes,&o->damage_result))return 0;
    o->parent_called=o->completed=1;return 1;
}
int __cdecl expire_major(void* a,unsigned n,float now,ServerRecombinantDamageExpiryResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!major(a,n,0xc44)||!std::isfinite(now))return 0;
    o->deadline=rd<float>(a,0xc40);o->prior=rd<float>(a,0xc3c);
    if(!std::isfinite(o->deadline)||!std::isfinite(o->prior))return 0;
    // Deadline is left untouched in the original, including repeat frames.
    if(o->deadline!=0.f&&now>=o->deadline){wr(a,0xc3c,0.f);o->cleared=1;}
    o->completed=1;return 1;
}
const ServerRecombinantDamageApi api{1,sizeof(ServerRecombinantDamageApi),&damage_major,&ignite_major,&expire_major};

struct MajorPendingFrame {
    void* actor;
    unsigned bytes;
    ServerRecombinantDamageBindings source;
    const ServerRecombinantPendingActivityBindings* pending;
    ServerRecombinantDamagePendingResult* result;
};
template<class Invoke>
int compose_pending(void* a,unsigned n,const ServerRecombinantDamagePendingBindings* b,
    ServerRecombinantDamagePendingResult* o,Invoke invoke) noexcept {
    if(!o)return 0;*o={};o->requested_activity=-1;
    if(!major(a,n,0xc16u)||!b||!b->damage||!b->pending||
       !b->pending->pending||!b->pending->activity||
       b->pending->pending->assignment.translation.actor_class!=NpcActivityHosted)return 0;
    const auto& activity=*b->pending->activity;
    const auto* ids=activity.major_activities;
    const auto* reg=SiN_GetServerRecombinantRegistrationApi(1,sizeof(ServerRecombinantRegistrationApi));
    const auto* pending=SiN_GetServerRecombinantPendingActivityApi(1,sizeof(ServerRecombinantPendingActivityApi));
    if(!pending||!reg||!activity.policy||!ids||ids->kind!=RecombinantMajor||
       !reg->valid(ids)||activity.policy->activities!=ids->owner)return 0;
    MajorPendingFrame frame{a,n,*b->damage,b->pending,o};
    auto wrapped=frame.source;wrapped.context=&frame;
    wrapped.extent=[](void* p,void* entity,unsigned* n) noexcept ->int{
        auto& f=*static_cast<MajorPendingFrame*>(p);
        return f.source.extent&&f.source.extent(f.source.context,entity,n);
    };
    wrapped.classification=[](void* p,void* entity,int* category) noexcept ->int{
        auto& f=*static_cast<MajorPendingFrame*>(p);
        return f.source.classification&&f.source.classification(f.source.context,entity,category);
    };
    wrapped.color_fade=[](void* p,void* actor,int x,int y) noexcept ->int{
        auto& f=*static_cast<MajorPendingFrame*>(p);
        return actor==f.actor&&f.source.color_fade&&f.source.color_fade(f.source.context,actor,x,y);
    };
    wrapped.parent_damage=[](void* p,void* actor,const void* info,unsigned bytes,int* value) noexcept ->int{
        auto& f=*static_cast<MajorPendingFrame*>(p);
        return actor==f.actor&&f.source.parent_damage&&
            f.source.parent_damage(f.source.context,actor,info,bytes,value);
    };
    wrapped.parent_ignite=[](void* p,void* actor,const ServerRecombinantIgniteRequest* request) noexcept ->int{
        auto& f=*static_cast<MajorPendingFrame*>(p);
        return actor==f.actor&&f.source.parent_ignite&&
            f.source.parent_ignite(f.source.context,actor,request);
    };
    wrapped.random_float=[](void* p,float lo,float hi,float* out) noexcept ->int{
        auto& f=*static_cast<MajorPendingFrame*>(p);
        return f.source.random_float&&f.source.random_float(f.source.context,lo,hi,out);
    };
    wrapped.health_changed=[](void* p,void* actor) noexcept ->int{
        auto& f=*static_cast<MajorPendingFrame*>(p);
        return actor==f.actor&&f.source.health_changed&&f.source.health_changed(f.source.context,actor);
    };
    wrapped.pending_activity=[](void* p,void* actor,int activity) noexcept ->int{
        auto& f=*static_cast<MajorPendingFrame*>(p);
        if(actor!=f.actor)return 0;
        ++f.result->pending_calls;
        f.result->requested_activity=activity;
        const auto* api=SiN_GetServerRecombinantPendingActivityApi(1,sizeof(ServerRecombinantPendingActivityApi));
        return api&&api->set_pending(actor,f.bytes,activity,f.pending,&f.result->pending);
    };
    return invoke(wrapped,*o);
}
int __cdecl composed_damage_major(void* a,unsigned n,const void* info,unsigned info_bytes,float now,
    const ServerRecombinantDamagePendingBindings* b,ServerRecombinantDamagePendingResult* o) noexcept {
    return compose_pending(a,n,b,o,[&](const ServerRecombinantDamageBindings& wrapped,ServerRecombinantDamagePendingResult& out) {
        return damage_major(a,n,info,info_bytes,now,&wrapped,&out.damage);
    });
}
int __cdecl composed_ignite_major(void* a,unsigned n,const ServerRecombinantIgniteRequest* in,
    const ServerRecombinantDamagePendingBindings* b,ServerRecombinantDamagePendingResult* o) noexcept {
    return compose_pending(a,n,b,o,[&](const ServerRecombinantDamageBindings& wrapped,ServerRecombinantDamagePendingResult& out) {
        return ignite_major(a,n,in,&wrapped,&out.ignition);
    });
}
const ServerRecombinantDamagePendingApi pending_damage_api{
    1,sizeof(ServerRecombinantDamagePendingApi),&composed_damage_major,&composed_ignite_major
};

}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantDamageApi=_SiN_GetServerRecombinantDamageApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantDamageApi* __cdecl SiN_GetServerRecombinantDamageApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerRecombinantDamageApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantDamagePendingApi=_SiN_GetServerRecombinantDamagePendingApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantDamagePendingApi* __cdecl SiN_GetServerRecombinantDamagePendingApi(unsigned v,unsigned n) noexcept {
    using namespace sin_port::native_weapon_tables;
    return v==1&&n==sizeof(ServerRecombinantDamagePendingApi)?&pending_damage_api:nullptr;
}
