#include "native_server_recombinant_launch.h"
#include <cmath>
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerNpcTaskLifecycleApi* __cdecl SiN_GetServerNpcTaskLifecycleApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void wr(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
struct Call{
    void* a;unsigned n;
    const ServerRecombinantLaunchBindings& b;
    ServerRecombinantLaunchResult& o;
    const ServerRecombinantTaskBindings& task;
    const ServerRecombinantPolicyBindings& p;
    bool vector(const float v[3]){return std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2]);}
    bool refresh(){
        if(!(rd<unsigned>(a,0xb8)&0x800))return true;
        ++o.origin_refreshes;return b.refresh_origin&&b.refresh_origin(b.context,a);
    }
    bool trace(bool up){
        o.trace.upward=up;o.trace.mask=0x202400bu;
        o.trace.collision_token=rd<unsigned>(a,0x214);
        if(!refresh())return false;
        for(unsigned i=0;i<3;++i)o.trace.end[i]=rd<float>(a,0x2bc+4*i);
        o.trace.end[2]=static_cast<float>(double(o.trace.end[2])+(up?1.:-12.));
        if(!refresh())return false;
        for(unsigned i=0;i<3;++i)o.trace.start[i]=rd<float>(a,0x2bc+4*i);
        if(!vector(o.trace.start)||!vector(o.trace.end))return false;
        ++o.trace_calls;
        if(!b.trace||!b.trace(b.context,a,&o.trace,&o.fraction)||!std::isfinite(o.fraction)||o.fraction<0||o.fraction>1)return false;
        o.clearance=up?o.fraction==1.f:o.fraction!=1.f;
        return true;
    }
    bool complete(float now){
        const auto* life=SiN_GetServerNpcTaskLifecycleApi(1,sizeof(ServerNpcTaskLifecycleApi));
        auto binding=task.lifecycle;binding.conditions=p.conditions;
        return life&&life->complete(a,n,now,false,&binding,&o.lifecycle);
    }
    bool launch(float now){
        const auto* touch=b.task->touch;
        if(!touch||!touch->minor||!touch->minor->damage)return false;
        const auto& m=*touch->minor;const auto& damage=*m.damage;
        if(p.entry_count>sin_independent::kEntityHandleCapacity||(!p.entries&&p.entry_count))return false;
        void* ground=sin_independent::EntityHandleView(p.entries,p.entry_count).Resolve(rd<unsigned>(a,0x238));
        o.ground_live=ground!=nullptr;
        if(!ground)return complete(now);
        unsigned size=0;
        if(!damage.extent||!damage.extent(damage.context,ground,&size)||size<0x21c)return false;
        if(const unsigned token=rd<unsigned>(ground,0x218)){
            unsigned flags=0;++o.object_checks;
            if(!m.object_flags||!m.object_flags(m.context,token,&flags))return false;
            if(flags&4u){
                if(!b.ground_object_effect||!b.ground_object_effect(b.context,a,ground,1,.5f))return false;
                o.object_effect=1;
            }
        }
        // Original loops until one of two signed draws is nonzero.
        // Bound a hostile owner RNG to 64 pairs, never substitute a direction.
        if(!p.random)return false;
        for(unsigned i=0;i<64;++i){
            for(unsigned axis=0;axis<2;++axis){
                int v=0;++o.random_calls;
                if(!p.random(p.context,-1,1,&v)||v<-1||v>1)return false;
                o.direction[axis]=static_cast<float>(v);
            }
            if(o.direction[0]!=0||o.direction[1]!=0)break;
        }
        if(o.direction[0]==0&&o.direction[1]==0)return false;
        if(!b.normalize||!b.normalize(b.context,o.direction)||!vector(o.direction))return false;
        o.normalized=1;
        if(!b.clear_ground||!b.clear_ground(b.context,a))return false;
        o.ground_cleared=1;
        if(!trace(true))return false;
        if(o.clearance){
            for(unsigned i=0;i<3;++i)o.relocation[i]=rd<float>(a,0x360+4*i);
            o.relocation[2]=static_cast<float>(double(o.relocation[2])+1.);
            if(!vector(o.relocation)||!b.relocate||!b.relocate(b.context,a,o.relocation,0))return false;
            o.relocated=1;
        }
        for(unsigned i=0;i<3;++i){
            const float scaled=static_cast<float>(double(o.direction[i])*200.);
            o.velocity[i]=static_cast<float>(double(scaled)+(i==2?200.:0.));
        }
        if(!vector(o.velocity)||!m.set_velocity||!m.set_velocity(m.context,a,o.velocity))return false;
        o.velocity_set=1;return true;
    }
    bool fail_select(int schedule,int id,int reason,bool major){
        if(major){wr(a,0xc68,0xffffffffu);o.handle_cleared=1;}
        else if(schedule==85||(schedule==23&&id==100)||schedule==72||schedule==73){
            if(schedule==85||(schedule==23&&id==100)){
                const auto* ca=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));ServerNpcConditionResult r{};
                ++o.condition_tests;
                if(!ca||!ca->test(a,n,10,NpcConditionCurrent,&p.conditions,&r))return false;
                if(r.value){o.selected=41;return true;}
            }
            if(!(rd<unsigned char>(a,0xbc)&1)){o.selected=86;return true;}
            if(!trace(false))return false;
            if(!o.clearance){o.selected=86;return true;}
        }
        ++o.delegated;return b.parent_failure&&b.parent_failure(b.context,a,schedule,id,reason,&o.selected);
    }
};
bool init(void* a,unsigned n,const ServerRecombinantLaunchBindings* b,ServerRecombinantLaunchResult* o){
    if(!o)return false;*o={};o->selected=-1;
    return a&&n>=0xc48&&b&&b->task&&b->task->task&&b->task->task->decision&&b->task->task->decision->policy;
}
int __cdecl launch(void* a,unsigned n,float now,const ServerRecombinantLaunchBindings* b,ServerRecombinantLaunchResult* o) noexcept {
    if(!init(a,n,b,o)||rd<unsigned>(a,0)!=recombinant_minor_vtable||!std::isfinite(now))return 0;
    const auto& t=*b->task->task;Call c{a,n,*b,*o,t,*t.decision->policy};
    o->completed=c.launch(now);return o->completed;
}
int __cdecl failure(void* a,unsigned n,int schedule,int id,int reason,const ServerRecombinantLaunchBindings* b,ServerRecombinantLaunchResult* o) noexcept {
    if(!init(a,n,b,o))return 0;
    const unsigned vt=rd<unsigned>(a,0);const bool major=vt==recombinant_major_vtable;
    if((major&&n<0xc6c)||(!major&&vt!=recombinant_minor_vtable))return 0;
    const auto& t=*b->task->task;Call c{a,n,*b,*o,t,*t.decision->policy};
    o->completed=c.fail_select(schedule,id,reason,major);return o->completed;
}
const ServerRecombinantLaunchApi api{1,sizeof(ServerRecombinantLaunchApi),&launch,&failure};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantLaunchApi=_SiN_GetServerRecombinantLaunchApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantLaunchApi* __cdecl SiN_GetServerRecombinantLaunchApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerRecombinantLaunchApi)?&api:nullptr;
}
