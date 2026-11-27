#include "native_server_recombinant_preschedule.h"
#include <cmath>
#include <cstring>
extern "C" const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(unsigned,unsigned) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerRecombinantDamageApi* __cdecl SiN_GetServerRecombinantDamageApi(unsigned,unsigned) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void wr(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
struct Call{
    void* a;unsigned n;
    const ServerRecombinantPrescheduleBindings& b;
    ServerRecombinantPrescheduleResult& o;
    const ServerNpcConditionApi* ca;
    const ServerRecombinantPolicyBindings& p;
    void* resolve(unsigned h){return sin_independent::EntityHandleView(p.entries,p.entry_count).Resolve(h);}
    void* goal(){return resolve(rd<unsigned>(a,0xc44));}
    bool condition(int id,bool value){
        ServerNpcConditionResult r{};++o.condition_writes;
        return ca->store(a,n,id,NpcConditionCurrent,value,&p.conditions,&r)!=0;
    }
    bool match(int id,bool ideal,bool& yes){
        ++o.schedule_tests;return p.schedule_matches&&p.schedule_matches(p.context,a,id,ideal,&yes);
    }
    void clear_goal(){wr(a,0xc44,0xffffffffu);o.goal_cleared=1;}
    bool center(void* e,float v[3]){
        if(!b.world_center||!b.world_center(b.context,e,v))return false;
        return std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2]);
    }
    bool monitor(){
        void* enemy=nullptr;
        if(!b.enemy||!b.enemy(b.context,a,&enemy))return false;
        o.enemy_live=enemy!=nullptr;
        void* object=goal();o.goal_live=object!=nullptr;
        if(!object)return true; // stale non-null handle is not reset here
        if(!enemy){
            if(!condition(68,true))return false;
            clear_goal();return true;
        }
        float v[3]{};
        if(!center(object,v))return false;
        double d[3]{};
        for(unsigned i=0;i<3;++i){
            const float previous=rd<float>(a,0xc54+4*i);
            if(!std::isfinite(previous))return false;
            d[i]=std::fabs(double(v[i])-previous);
        }
        o.displacement=(d[2]+d[1])+d[0];
        if(o.displacement>256.){
            if(!condition(67,false)||!condition(68,true))return false;
            clear_goal();
        }else if(!condition(67,true)||!condition(68,false))return false;
        if(b.debug){
            // Retail resolves the handle again before dereferencing. A
            // cleared handle can lead to null; fail closed at that boundary.
            object=goal();
            if(!object){o.debug_invalid_goal=1;return false;}
            const float mins[]{-32,-32,-32},maxs[]{32,32,32};
            if(!center(object,v)||!b.debug_box||!b.debug_box(b.context,v,mins,maxs,255,255,255,1,.1f))return false;
            o.debug_called=1;
        }
        return true;
    }
    bool run(float now){
        const auto* damage=SiN_GetServerRecombinantDamageApi(1,sizeof(ServerRecombinantDamageApi));
        if(!damage||!damage->expire_major(a,n,now,&o.expiry))return false;
        if(rd<unsigned char>(a,0xc14)&&!rd<unsigned char>(a,0xc15)){
            if(rd<unsigned>(a,0xb8)&0x1000u){
                if(!b.refresh_velocity||!b.refresh_velocity(b.context,a))return false;
                o.velocity_refreshed=1;
            }
            const float z=rd<float>(a,0x258);
            if(!std::isfinite(z))return false;
            if(z<=0.f){wr<unsigned char>(a,0xc15,1);o.landing_latched=1;}
        }
        bool yes=false;
        if(!match(80,false,yes))return false;
        if(yes){
            o.goal_live=goal()!=nullptr;
            if(o.goal_live)return true;
            unsigned h=0xffffffffu;
            if(!b.find_object||!b.find_object(b.context,a,300,&h))return false;
            o.search_called=1;wr(a,0xc44,h);o.search_live=resolve(h)!=nullptr;
            return condition(67,o.search_live!=0);
        }
        if(!match(86,true,yes))return false;
        return !yes||monitor();
    }
};
bool init(void* a,unsigned n,const ServerRecombinantPrescheduleBindings* b,ServerRecombinantPrescheduleResult* o){
    if(!o)return false;*o={};o->prior_handle=o->final_handle=0xffffffffu;
    if(!a||n<0xc60||rd<unsigned>(a,0)!=recombinant_major_vtable||!b||!b->policy||
       b->policy->entry_count>sin_independent::kEntityHandleCapacity||(!b->policy->entries&&b->policy->entry_count))return false;
    o->prior_handle=o->final_handle=rd<unsigned>(a,0xc44);return true;
}
int execute(void* a,unsigned n,float now,bool run,const ServerRecombinantPrescheduleBindings* b,ServerRecombinantPrescheduleResult* o) noexcept {
    if(!init(a,n,b,o)||!std::isfinite(now))return 0;
    const auto* ca=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));if(!ca)return 0;
    Call c{a,n,*b,*o,ca,*b->policy};const bool ok=run?c.run(now):c.monitor();
    o->final_handle=rd<unsigned>(a,0xc44);o->completed=ok;return ok;
}
int __cdecl run_major(void* a,unsigned n,float now,const ServerRecombinantPrescheduleBindings* b,ServerRecombinantPrescheduleResult* o) noexcept{
    return execute(a,n,now,true,b,o);
}
int __cdecl monitor_major(void* a,unsigned n,const ServerRecombinantPrescheduleBindings* b,ServerRecombinantPrescheduleResult* o) noexcept{
    return execute(a,n,0.f,false,b,o);
}
const ServerRecombinantPrescheduleApi api{1,sizeof(ServerRecombinantPrescheduleApi),&run_major,&monitor_major};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerRecombinantPrescheduleApi=_SiN_GetServerRecombinantPrescheduleApi")
extern "C" const sin_port::native_weapon_tables::ServerRecombinantPrescheduleApi* __cdecl SiN_GetServerRecombinantPrescheduleApi(unsigned v,unsigned n) noexcept{
    using namespace sin_port::native_weapon_tables;return v==1&&n==sizeof(ServerRecombinantPrescheduleApi)?&api:nullptr;
}
