#include "native_server_npc_shared_attack.h"
#include "native_server_npc_inherited_schedule.h"
#include "native_server_weapon_script_view.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTaskLifecycleApi* __cdecl SiN_GetServerNpcTaskLifecycleApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTransitionApi* __cdecl SiN_GetServerNpcTransitionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponActivityApi* __cdecl SiN_GetServerWeaponActivityApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcInheritedScheduleApi* __cdecl SiN_GetServerNpcInheritedScheduleApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponScriptViewApi* __cdecl SiN_GetServerWeaponScriptViewApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* a,unsigned off) noexcept{T v{};std::memcpy(&v,static_cast<const unsigned char*>(a)+off,sizeof(v));return v;}
template<class T>void wr(void* a,unsigned off,T v) noexcept{std::memcpy(static_cast<unsigned char*>(a)+off,&v,sizeof(v));}
bool draw_count(void* a,const ServerNpcTaskLifecycleBindings& b) noexcept{
    const unsigned low=rd<std::uint16_t>(a,0x9b0),high=rd<std::uint16_t>(a,0x9b2);
    unsigned v=0;
    if(low>high||!b.random_int||!b.random_int(b.context,low,high,&v)||v<low||v>high)return false;
    wr(a,0x9ae,static_cast<std::uint16_t>(v));return true;
}
bool draw_delay(void* a,unsigned offset,const ServerNpcTaskLifecycleBindings& b,double& value) noexcept{
    const float lo=rd<float>(a,offset),hi=rd<float>(a,offset+4);
    return std::isfinite(lo)&&std::isfinite(hi)&&lo<=hi&&b.random_float&&b.random_float(b.context,lo,hi,&value)&&std::isfinite(value)&&value>=lo&&value<=hi;
}
int __cdecl advance_timer(void* a,unsigned n,float now,const ServerNpcTaskLifecycleBindings* b,unsigned* reset) noexcept{
    if(!reset)return 0;*reset=0;
    if(!a||n<0x9c4||!b||!std::isfinite(now)||!std::isfinite(rd<float>(a,0x9a8)))return 0;
    const auto count=static_cast<std::uint16_t>(rd<std::uint16_t>(a,0x9ae)-1u);
    wr(a,0x9ae,count);double delay=0;
    if(!count){
        if(!draw_count(a,*b)||!draw_delay(a,0x9b4,*b,delay))return 0;
        const float deadline=static_cast<float>(static_cast<double>(now)+delay);
        if(!std::isfinite(deadline))return 0;
        wr<std::uint8_t>(a,0x9ac,1);wr(a,0x9a8,deadline);*reset=1;return 1;
    }
    wr<std::uint8_t>(a,0x9ac,0);
    if(!draw_delay(a,0x9bc,*b,delay))return 0;
    const double sum=static_cast<double>(rd<float>(a,0x9a8))+delay;
    const float deadline=static_cast<float>(sum<now?now:sum);
    if(!std::isfinite(deadline))return 0;
    wr(a,0x9a8,deadline);return 1;
}
int __cdecl refresh(void* a,unsigned n,const ServerWeaponActivityBindings* b,void* context,int (__cdecl *notify)(void*,std::uint32_t) noexcept,ServerNpcAttackParameterResult* out) noexcept{
    if(!out)return 0;*out={};
    if(!a||n<0xc1c||!b||b->count>sin_independent::kEntityHandleCapacity||(b->count&&!b->entries))return 0;
    out->component=rd<std::uint32_t>(a,0xc18);
    if(out->component&&!notify)return 0;
    void* weapon=sin_independent::EntityHandleView(b->entries,b->count).Resolve(rd<std::uint32_t>(a,0x78c));
    if(!weapon||!b->classify||!b->classify(b->context,weapon,&out->weapon_class))return 0;
    const auto* ranges=SiN_GetServerNpcInheritedScheduleApi(1,sizeof(ServerNpcInheritedScheduleApi));
    const auto* script=SiN_GetServerWeaponScriptViewApi(1,sizeof(ServerWeaponScriptViewApi));
    int low=0,high=0;double interval=0;
    if(!ranges||!script||!ranges->burst_range(out->weapon_class,&low,&high)||!script->interval(out->weapon_class,&interval)||
       low<0||high<low||high>65535||!std::isfinite(interval))return 0;
    const float delay=static_cast<float>(interval);
    if(!std::isfinite(delay))return 0;
    out->lower=static_cast<unsigned>(low);out->upper=static_cast<unsigned>(high);
    out->short_delay=delay;out->long_low=.3f;out->long_high=.6f;
    wr(a,0x9bc,delay);wr(a,0x9c0,delay);
    wr(a,0x9b0,static_cast<std::uint16_t>(low));wr(a,0x9b2,static_cast<std::uint16_t>(high));
    wr(a,0x9b4,.3f);wr(a,0x9b8,.6f);
    if(out->component){
        if(!notify(context,out->component))return 0;out->component_notified=1;
    }
    return 1;
}
const ServerNpcAttackParameterApi parameter_api{1,sizeof(ServerNpcAttackParameterApi),&refresh};
int __cdecl supports(int id) noexcept{return id>=67&&id<=73;}
int __cdecl execute(void* a,unsigned n,const ServerNpcScheduleTask* t,bool running,float now,const ServerNpcSharedAttackBindings* b,ServerNpcSharedAttackResult* out) noexcept{
    if(!out)return 0;*out={};out->activity=-1;
    if(!a||n<0xc1c||!t||!b||!supports(t->id)||!std::isfinite(now))return 0;
    out->task=t->id;
    const auto* weapons=SiN_GetServerWeaponActivityApi(1,sizeof(ServerWeaponActivityApi));
    const auto* life=SiN_GetServerNpcTaskLifecycleApi(1,sizeof(ServerNpcTaskLifecycleApi));
    if(!weapons||!life)return 0;
    const auto& s=b->shared;
    auto caps=[&](){return weapons->capabilities(a,n,&b->weapons,&out->capabilities)!=0;};
    if(t->id==67){if(!caps())return 0;out->special=(out->capabilities&0x1000000u)!=0;}
    if(!out->special)if(const auto token=rd<std::uint32_t>(a,0xc18)){
        void* component=nullptr;unsigned size=0;
        if(!s.component_storage||!s.component_storage(s.context,token,&component,&size)||!component||size<9||!s.component_task)return 0;
        wr<std::uint8_t>(component,8,1);out->component_called=1;
        if(!s.component_task(s.context,token,t,running))return 0;
        if(rd<std::uint8_t>(component,8)){out->component_handled=1;return 1;}
    }
    auto pending=[&](int id){
        if(rd<int>(a,0x878)==id)wr(a,0x878,0);
        out->activity=id;
        if(s.transition){
            const auto* api=SiN_GetServerNpcTransitionApi(1,sizeof(ServerNpcTransitionApi));
            return api&&api->set_pending(a,n,id,s.transition,&out->transition);
        }
        return s.pending&&s.pending(s.context,a,id);
    };
    auto complete=[&](){return life->complete(a,n,now,false,&s.lifecycle,&out->lifecycle)!=0;};
    auto fail=[&](int code){out->failed_code=code;return life->fail(a,n,now,code,&s.lifecycle,&out->lifecycle)!=0;};
    auto timer_active=[&](){return rd<std::uint8_t>(a,0x9ac)&&rd<float>(a,0x9a8)>now;};
    auto advance=[&](){
        wr(a,0x99c,now);out->timer_advanced=1;
        if(!advance_timer(a,n,now,&s.lifecycle,&out->timer_reset))return false;
        if(timer_active()){
            if(b->refresh_parameters){
                if(!b->refresh_parameters(b->context,a))return false;
            }else{
                ServerNpcAttackParameterResult parameters{};
                if(!refresh(a,n,&b->weapons,nullptr,nullptr,&parameters))return false;
            }
            out->parameters_refreshed=1;
        }
        const float deadline=rd<float>(a,0x9a8);
        if(!std::isfinite(deadline))return false;
        if(deadline!=rd<float>(a,0x60c)){
            if(rd<std::uint8_t>(a,0x1f0))wr<std::uint8_t>(a,0x200,1);
            else if(const auto token=rd<std::uint32_t>(a,0x16c)){if(!b->dirty||!b->dirty(b->context,a,token))return false;}
            wr(a,0x60c,deadline);out->deadline_changed=1;
        }
        return true;
    };
    if(!running){
        if(out->special){
            if(!std::isfinite(rd<float>(a,0x9a8)))return 0;
            if(timer_active())return fail(10);
            if(rd<float>(a,0x9a8)<=now)return advance()&&pending(16);
            return pending(72);
        }
        constexpr int activities[]{16,17,64,65,66,93,94};
        if(t->id<=70)wr(a,0x99c,now);
        return pending(activities[t->id-67]);
    }
    out->turned=1;
    if(!s.turn||!s.turn(s.context,a))return 0;
    float vector[3]{};
    if(!b->aim_vector||!b->aim_vector(b->context,a,vector)||!std::isfinite(vector[0])||!std::isfinite(vector[1])||!std::isfinite(vector[2]))return 0;
    if(!out->special&&!caps())return 0;
    if((out->special||!(out->capabilities&4u))&&(vector[0]!=0||vector[1]!=0||vector[2]!=0)){
        bool aligned=false;
        if(t->id==67||t->id==71){
            // Both retail run paths re-query capability before alignment.
            if(!caps())return 0;
            if(out->capabilities&0x20000000u){
                ++out->aim_queries;
                if(!b->aim_aligned||!b->aim_aligned(b->context,a,vector,&aligned))return 0;
            }
        }
        ++out->aim_updates;
        if(!b->aim_apply||!b->aim_apply(b->context,a,vector,aligned))return 0;
    }
    unsigned finished=0;if(!life->animation_finished(a,n,&finished))return 0;
    if(!finished)return 1;
    if(out->special){
        if(!std::isfinite(rd<float>(a,0x9a8)))return 0;
        if(timer_active())return complete();
        if(rd<float>(a,0x9a8)<=now)return advance()&&pending(16);
        return 1;
    }
    if(t->id==71){
        if(!draw_count(a,s.lifecycle))return 0;
        wr(a,0x9a8,now);wr<std::uint8_t>(a,0x9ac,0);out->timer_reset=1;
    }
    return complete();
}
const ServerNpcSharedAttackApi api{1,sizeof(ServerNpcSharedAttackApi),&supports,&advance_timer,&execute};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcSharedAttackApi=_SiN_GetServerNpcSharedAttackApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSharedAttackApi* __cdecl SiN_GetServerNpcSharedAttackApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcSharedAttackApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcAttackParameterApi=_SiN_GetServerNpcAttackParameterApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcAttackParameterApi* __cdecl SiN_GetServerNpcAttackParameterApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcAttackParameterApi)?&parameter_api:nullptr;
}
