#include "native_server_recombinant_minor.h"
#include "native_server_recombinant_minor_damage_gate.h"
#include <cmath>
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerRecombinantMinorDamageGateApi* __cdecl SiN_GetServerRecombinantMinorDamageGateApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void wr(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
struct Call{
    void* a;unsigned n;
    const ServerRecombinantMinorBindings& b;
    ServerRecombinantMinorResult& o;
    const ServerRecombinantDamageBindings& d;
    const ServerRecombinantPolicyBindings& p;
    bool cond(int id,bool value){
        const auto* ca=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));ServerNpcConditionResult r{};
        ++o.condition_writes;return ca&&ca->store(a,n,id,NpcConditionCurrent,value,&p.conditions,&r);
    }
    void* resolve(unsigned h){return sin_independent::EntityHandleView(p.entries,p.entry_count).Resolve(h);}
    bool extent(void* e,unsigned need){unsigned n=0;return d.extent&&d.extent(d.context,e,&n)&&n>=need;}
    bool classify(void* e,int& cls){
        if(!extent(e,4))return false;
        const unsigned vt=rd<unsigned>(e,0);
        if(vt==0x2249f65cu||vt==recombinant_major_vtable||vt==recombinant_minor_vtable){cls=10;return true;}
        return d.classification&&d.classification(d.context,e,&cls);
    }
    bool velocity(const float v[3]){
        for(unsigned i=0;i<3;++i)if(!std::isfinite(v[i]))return false;
        if(!b.set_velocity||!b.set_velocity(b.context,a,v))return false;
        std::memcpy(o.velocity,v,12);o.velocity_set=1;return true;
    }
    bool detach(float now){
        if(!cond(67,true))return false;
        const float v[]{0,0,-.01f};
        if(!velocity(v))return false;
        wr<unsigned char>(a,0xc41,0);wr(a,0x7c,0x2237ca30u);
        o.detached=1;
        if(!b.schedule_touch_think||!b.schedule_touch_think(b.context,a,now,0))return false;
        o.think_scheduled=1;return true;
    }
    bool damage(const void* info,float now){
        // Original copy constructor 2202f480 copies 18 dwords, not
        // the caller's original buffer or its trailing bytes.
        alignas(4) unsigned char copy[0x48];
        std::memcpy(copy,info,sizeof(copy));
        if(void* source=resolve(rd<unsigned>(copy,0x24))){
            o.source_live=1;if(!classify(source,o.source_class))return false;
            if(o.source_class==10){o.immune=1;return true;}
        }
        if(rd<unsigned char>(a,0xc41)){
            void* attacker=resolve(rd<unsigned>(copy,0x28));o.attacker_live=attacker!=nullptr;
            void* player=nullptr;
            if(b.max_clients<=1&&(!b.local_player||!b.local_player(b.context,&player)))return false;
            // Original compares the actual pointers, even if both null.
            if(attacker==player){
                o.local_matches=1;
                for(unsigned i=0;i<3;++i){
                    o.impulse[i]=static_cast<float>(double(rd<float>(copy,4*i))*.05f);
                    if(!std::isfinite(o.impulse[i]))return false;
                }
                o.squared=double(o.impulse[2])*o.impulse[2]+double(o.impulse[1])*o.impulse[1]+double(o.impulse[0])*o.impulse[0];
                if(o.squared>62500.){
                    if(!detach(now))return false;
                }else{
                    if(rd<unsigned>(a,0xb8)&0x1000u){
                        if(!b.refresh_velocity||!b.refresh_velocity(b.context,a))return false;o.velocity_refreshed=1;
                    }
                    float v[3]{};
                    for(unsigned i=0;i<3;++i)v[i]=static_cast<float>(double(rd<float>(a,0x250+4*i))+o.impulse[i]);
                    if(!velocity(v))return false;
                }
            }
        }
        if(rd<unsigned>(copy,0x38)&8u){
            if(rd<unsigned char>(a,0xbf)&8u){
                if(!d.color_fade||!d.color_fade(d.context,a,5,30))return false;o.color_called=1;
                if(rd<int>(a,0x9c)<=1){
                    if(!b.entity_index||!b.entity_index(b.context,rd<unsigned>(a,0x16c),&o.entity_index))return false;
                    o.index_called=1;
                    if(o.entity_index%2){
                        bool same=false;++o.schedule_tests;
                        if(!p.schedule_matches||!p.schedule_matches(p.context,a,80,true,&same))return false;
                        if(!same){if(!b.set_schedule||!b.set_schedule(b.context,a,80))return false;o.schedule_set=1;}
                    }
                }
            }
            const ServerRecombinantIgniteRequest request{30.f,1,0,0,0,0};
            if(!d.parent_ignite||!d.parent_ignite(d.context,a,&request))return false;o.ignited=1;
        }
        if(!d.parent_damage||!d.parent_damage(d.context,a,copy,sizeof(copy),&o.damage_result))return false;
        o.parent_called=1;return true;
    }
    bool gather(){
        if(!b.parent_gather||!b.parent_gather(b.context,a))return false;
        o.parent_called=1;
        if(!cond(66,false))return false;
        void* ground=resolve(rd<unsigned>(a,0x238));o.ground_live=ground!=nullptr;
        if(!(rd<unsigned char>(a,0xbc)&1u)||!ground)return true;
        if(!extent(ground,0x21c)||!b.entity_index||!b.entity_index(b.context,rd<unsigned>(ground,0x16c),&o.entity_index))return false;
        o.index_called=1;if(!o.entity_index)return true;
        bool accepted=false;++o.special_calls;
        if(!b.npc_or_special||!b.npc_or_special(b.context,ground,&accepted))return false;
        if(!accepted){
            ++o.player_calls;
            if(!b.is_player||!b.is_player(b.context,ground,&accepted))return false;
        }
        if(!accepted){
            unsigned token=rd<unsigned>(ground,0x218);
            if(token){
                unsigned flags=0;++o.object_calls;
                if(!b.object_flags||!b.object_flags(b.context,token,&flags))return false;
                accepted=(flags&4u)!=0;
            }
        }
        o.ground_accepted=accepted;
        return !accepted||cond(66,true);
    }
};
bool init(void* a,unsigned n,const ServerRecombinantMinorBindings* b,ServerRecombinantMinorResult* o){
    if(!o)return false;*o={};o->source_class=-1;
    return a&&n>=0xc48&&rd<unsigned>(a,0)==recombinant_minor_vtable&&b&&b->damage&&b->damage->policy&&
        b->damage->policy->entry_count<=sin_independent::kEntityHandleCapacity&&
        (b->damage->policy->entries||!b->damage->policy->entry_count);
}
int __cdecl damage(void* a,unsigned n,const void* info,unsigned bytes,float now,const ServerRecombinantMinorBindings* b,ServerRecombinantMinorResult* o) noexcept {
    if(!init(a,n,b,o)||!info||bytes<0x48||!std::isfinite(now))return 0;
    Call c{a,n,*b,*o,*b->damage,*b->damage->policy};o->completed=c.damage(info,now);return o->completed;
}
int __cdecl detach(void* a,unsigned n,float now,const ServerRecombinantMinorBindings* b,ServerRecombinantMinorResult* o) noexcept {
    if(!init(a,n,b,o)||!std::isfinite(now))return 0;
    Call c{a,n,*b,*o,*b->damage,*b->damage->policy};o->completed=c.detach(now);return o->completed;
}
int __cdecl gather(void* a,unsigned n,const ServerRecombinantMinorBindings* b,ServerRecombinantMinorResult* o) noexcept {
    if(!init(a,n,b,o))return 0;
    Call c{a,n,*b,*o,*b->damage,*b->damage->policy};o->completed=c.gather();return o->completed;
}
const ServerRecombinantMinorApi api{1,sizeof(ServerRecombinantMinorApi),&damage,&detach,&gather};

// Phase 1625 originally duplicated the method already recovered in
// Phase 1613. Keep the public newer ABI but delegate to the single canonical
// +0x398 implementation, so its branch and x87 threshold behavior cannot drift.
int __cdecl minor_metric_gate(const void* actor,unsigned actor_bytes,
    const void* candidate,unsigned candidate_bytes,
    const ServerRecombinantMinorMetricGateBindings* b,
    ServerRecombinantMinorMetricGateResult* out) noexcept {
    if(!out)return 0;
    *out={};out->threshold=10.f;
    if(!b||!b->manager_suppresses)return 0;
    const auto* canonical=SiN_GetServerRecombinantMinorDamageGateApi(
        1,sizeof(ServerRecombinantMinorDamageGateApi));
    if(!canonical||!canonical->evaluate)return 0;
    ServerRecombinantMinorDamageGateBindings bindings{b->context,b->manager_suppresses};
    ServerRecombinantMinorDamageGateResult result{};
    const int ok=canonical->evaluate(const_cast<void*>(actor),actor_bytes,
        candidate,candidate_bytes,&bindings,&result);
    out->completed=result.completed;
    out->manager_calls=result.mode_queries;
    out->suppressed=result.suppressed;
    out->compared=result.damage_read;
    out->allowed=result.accepted;
    out->observed=result.damage;
    out->threshold=result.threshold;
    return ok;
}
const ServerRecombinantMinorMetricGateApi metric_api{
    1,sizeof(ServerRecombinantMinorMetricGateApi),&minor_metric_gate
};


int __cdecl minor_origin_point(void* actor,unsigned bytes,float point[3],
    const ServerRecombinantMinorOriginPointBindings* b,
    ServerRecombinantMinorOriginPointResult* out) noexcept {
    if(!out)return 0;*out={};
    if(!actor||bytes<0x2c8u||rd<unsigned>(actor,0)!=recombinant_minor_vtable||!point)return 0;
    // Original code tests +b8 bit 11 and calls inherited 2213fa60
    // before reading the three world-origin floats at +2bc.
    if(rd<unsigned>(actor,0xb8)&0x800u){
        ++out->refresh_calls;
        if(!b||!b->refresh_origin||!b->refresh_origin(b->context,actor))return 0;
    }
    const float x=rd<float>(actor,0x2bc),y=rd<float>(actor,0x2c0),
        z=rd<float>(actor,0x2c4);
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z))return 0;
    const float shifted=static_cast<float>(double(z)+6.0);
    if(!std::isfinite(shifted))return 0;
    const float values[]{x,y,shifted};
    std::memcpy(point,values,sizeof(values));
    out->origin[0]=x;out->origin[1]=y;out->origin[2]=z;
    std::memcpy(out->point,values,sizeof(values));
    out->completed=1;
    return 1;
}
const ServerRecombinantMinorOriginPointApi origin_point_api{
    1,sizeof(ServerRecombinantMinorOriginPointApi),&minor_origin_point
};

}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantMinorApi=_SiN_GetServerRecombinantMinorApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantMinorApi* __cdecl SiN_GetServerRecombinantMinorApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerRecombinantMinorApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantMinorOriginPointApi=_SiN_GetServerRecombinantMinorOriginPointApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantMinorOriginPointApi* __cdecl
SiN_GetServerRecombinantMinorOriginPointApi(unsigned version,unsigned size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerRecombinantMinorOriginPointApi)?&origin_point_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantMinorMetricGateApi=_SiN_GetServerRecombinantMinorMetricGateApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantMinorMetricGateApi* __cdecl
SiN_GetServerRecombinantMinorMetricGateApi(unsigned version,unsigned size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerRecombinantMinorMetricGateApi)?&metric_api:nullptr;
}
