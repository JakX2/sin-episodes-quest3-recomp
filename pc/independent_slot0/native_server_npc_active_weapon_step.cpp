#include "native_server_npc_active_weapon_step.h"
#include <cmath>
#include <cstring>
#include <limits>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponActivityApi* __cdecl SiN_GetServerWeaponActivityApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSubclassSchedulerApi* __cdecl SiN_GetServerNpcSubclassSchedulerApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeWeaponPolicyThinkApi* __cdecl SiN_GetServerNpcNativeWeaponPolicyThinkApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeJetWeaponPolicyThinkApi* __cdecl SiN_GetServerNpcNativeJetWeaponPolicyThinkApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScriptWeaponEventFrameApi* __cdecl SiN_GetServerNpcScriptWeaponEventFrameApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* a,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(a)+off,sizeof(v));return v;}
template<class ThinkBinding,class ThinkApi,class Result>
int run_impl(void* a,unsigned n,const ServerNpcRunAnimationInput* in,const ServerNpcActiveWeaponStepBindings* b,const ThinkBinding* think_binding,const ThinkApi* think,bool jet,Result* out) noexcept {
    if(!out)return 0;
    // Large scheduler result; avoid recursive aggregate temporary on MSVC x86.
    std::memset(out,0,sizeof(*out));out->animation.frame.frame.failed_event=-1;
    if(!a||n<0xfd8||!in||!std::isfinite(in->now)||!b||!b->schedule||!b->policy||
       !b->policy->ammo||!b->policy->ammo->scripts||!b->policy->melee||!b->frame)return 0;
    const auto& scheduler=b->schedule->scheduler;
    const auto& selection=scheduler.maintenance.selection;
    const auto& g=b->frame->frame.frame;const auto& t=g.runtime.npc.transition.assignment.translation;
    const auto* cls=SiN_GetServerNpcSubclassSchedulerApi(1,sizeof(ServerNpcSubclassSchedulerApi));
    unsigned family=0;
    if(!cls||!cls->classify(scheduler.original_vtable,&family)||jet!=(family==NpcSubclassJetpack)||
       n<(jet?0x1085u:family==NpcSubclassElite?0xfe0u:0xfd8u)||t.actor_class!=(jet?NpcActivityJetpack:NpcActivityGrunt)||
       !g.ids||g.ids!=scheduler.grunt.registration||!g.ids->events_ready||!g.ids->activities_ready||
       !t.activities||!t.events||g.ids->activity_owner!=t.activities||g.ids->event_owner!=t.events||
       g.ids->activity_generation!=t.activities->names.generation||g.ids->event_generation!=t.events->generation||
       t.activities!=g.runtime.npc.frame.activities||t.events!=g.runtime.npc.frame.frame.dictionary.dictionary||
       t.activities!=g.runtime.weapon.animation.activities||t.events!=g.runtime.weapon.animation.events||
       t.events!=g.runtime.weapon.animation.dictionary.dictionary)return 0;
    const auto& registry=selection.state.entities;
    if(registry.count>sin_independent::kEntityHandleCapacity||(registry.count&&!registry.entries))return 0;
    const auto* frame=SiN_GetServerNpcScriptWeaponEventFrameApi(1,sizeof(ServerNpcScriptWeaponEventFrameApi));
    if(!frame||!think)return 0;
    const float deadline=rd<float>(a,0x864);
    if(!std::isfinite(deadline))return 0;
    // x87 comparison at 220433d7: equality is due, later deadline skips +41c.
    if(deadline<=in->now){
        if(!b->image||!think_binding||!b->full||!b->attack||!b->facing||!b->target||!b->movement||
           !b->cue||!b->observe||!b->sound_cover||!b->tactical)return 0;
        out->think_called=1;
        if(!think->run(a,n,in->now,b->image,b->schedule,b->full,b->attack,b->facing,b->target,b->movement,b->cue,b->observe,b->sound_cover,b->tactical,
            think_binding,&out->think,&out->outer_line,&out->inner_line,b->policy,&out->outer_weapon,&out->inner_weapon))return 0;
        out->think_completed=1;
    }else{
        if(rd<std::uint8_t>(a,0x7b8)){
            out->reset_called=1;if(!b->reset_movement||!b->reset_movement(b->context,a))return 0;
        }
        if(rd<std::uint8_t>(a,0x7b9)){
            out->motor_called=1;if(!b->motor_activity||!b->motor_activity(b->context,rd<std::uint32_t>(a,0x908),-1))return 0;
        }
    }
    auto bound=*b->frame;
    // Copy authoritative services from the schedule side; do not let animation
    // and reload observe a different registry, clip script or condition map.
    bound.frame.frame.runtime.inherited.weapons=registry;
    bound.scripts=*b->policy->ammo->scripts;
    bound.conditions=selection.grunt.schedule.conditions;
    out->animation_called=1;
    if(!frame->run(a,n,in,&bound,&out->animation))return 0;
    out->completed=1;return 1;
}
int __cdecl run(void* a,unsigned n,const ServerNpcRunAnimationInput* in,const ServerNpcActiveWeaponStepBindings* b,ServerNpcActiveWeaponStepResult* out) noexcept {
    return run_impl(a,n,in,b,b?b->think:nullptr,SiN_GetServerNpcNativeWeaponPolicyThinkApi(1,sizeof(ServerNpcNativeWeaponPolicyThinkApi)),false,out);
}
int __cdecl run_jet(void* a,unsigned n,const ServerNpcRunAnimationInput* in,const ServerNpcActiveJetWeaponStepBindings* b,ServerNpcActiveJetWeaponStepResult* out) noexcept {
    return run_impl(a,n,in,b&&!b->base.think?&b->base:nullptr,b?b->think:nullptr,SiN_GetServerNpcNativeJetWeaponPolicyThinkApi(1,sizeof(ServerNpcNativeJetWeaponPolicyThinkApi)),true,out);
}
template<class T>void wr(void* a,unsigned off,T v) noexcept{std::memcpy(static_cast<unsigned char*>(a)+off,&v,sizeof(v));}
bool resolve(const ServerNpcActivePostAnimationBindings& b,std::uint32_t token,unsigned need,void** p) noexcept {
    unsigned n=0;*p=nullptr;return token&&b.resolve&&b.resolve(b.context,token,p,&n)&&*p&&n>=need;
}
bool dirty(void* a,const ServerNpcActivePostAnimationBindings& b) noexcept {
    if(rd<std::uint8_t>(a,0x1f0)){wr<std::uint8_t>(a,0x200,1);return true;}
    const auto token=rd<std::uint32_t>(a,0x16c);if(!token)return true;
    void* word=nullptr;if(!resolve(b,token,4,&word))return false;
    wr<std::uint32_t>(word,0,rd<std::uint32_t>(word,0)|1u);return true;
}
int __cdecl post(void* a,unsigned n,float now,float interval,const ServerNpcScheduleServicesBindings* s,const ServerNpcActivePostAnimationBindings* b,ServerNpcActivePostAnimationResult* out) noexcept {
    if(!out)return 0;*out={};
    if(!a||n<0xc12||!s||!b||!std::isfinite(now)||!std::isfinite(interval)||interval<=0)return 0;
    unsigned family=0;const auto* cls=SiN_GetServerNpcSubclassSchedulerApi(1,sizeof(ServerNpcSubclassSchedulerApi));
    const auto* activity=SiN_GetServerWeaponActivityApi(1,sizeof(ServerWeaponActivityApi));
    if(!cls||!activity||!cls->classify(s->scheduler.original_vtable,&family))return 0;
    const auto& registry=s->scheduler.maintenance.selection.state.entities;
    if(registry.count>sin_independent::kEntityHandleCapacity||(registry.count&&!registry.entries))return 0;
    if(rd<int>(a,0x8c)==0){
        const double elapsed=rd<std::uint32_t>(a,0x910)==0x7f7fffffu?0.1:double(now)-rd<float>(a,0x910);
        const double ticks=std::trunc(elapsed/double(interval)+0.5);
        if(!std::isfinite(ticks)||ticks<double(std::numeric_limits<int>::min())||ticks>double(std::numeric_limits<int>::max()))return 0;
        out->navigation_delta=static_cast<float>(ticks*double(interval));if(!std::isfinite(out->navigation_delta)||!b->navigation_update)return 0;
        wr<std::uint8_t>(a,0x794,1);out->navigation_called=1;
        if(!b->navigation_update(b->context,a,rd<std::uint32_t>(a,0x8f8),out->navigation_delta))return 0;
        wr<float>(a,0x910,now);wr<std::uint8_t>(a,0x794,0);
    }
    // Reload navigation and path pointers after navigation callback.
    void* nav=nullptr;void* path=nullptr;
    if(!resolve(*b,rd<std::uint32_t>(a,0x8f8),0x28,&nav)||!resolve(*b,rd<std::uint32_t>(nav,0x24),0x48,&path))return 0;
    out->path_active=rd<std::uint32_t>(path,0x44)!=0;
    if(rd<std::uint8_t>(a,0xc11)!=out->path_active){
        if(!dirty(a,*b))return 0;wr<std::uint8_t>(a,0xc11,static_cast<std::uint8_t>(out->path_active));out->path_changed=1;
    }
    out->prepare_called=1;if(!b->prepare_pose||!b->prepare_pose(b->context,a))return 0;
    const float prev=rd<float>(a,0x2c),older=rd<float>(a,0x28);
    if(!std::isfinite(prev)||!std::isfinite(older))return 0;
    const double delta=prev<now?double(now)-prev:double(prev)-older;
    out->pose_delta=static_cast<float>(delta<0?0:delta>double(0.2f)?double(0.2f):delta);
    if(!activity->capabilities(a,n,&registry,&out->capabilities_first))return 0;
    if(out->capabilities_first&0x20000000u){out->capability_called=1;if(!b->capability_pose||!b->capability_pose(b->context,a))return 0;}
    if(!activity->capabilities(a,n,&registry,&out->capabilities_second))return 0;
    if(out->capabilities_second&0x800000u){out->timed_called=1;if(!b->timed_pose||!b->timed_pose(b->context,a,out->pose_delta))return 0;}
    out->finish_called=1;if(!b->finish_pose||!b->finish_pose(b->context,a))return 0;
    out->motor_called=1;if(!b->motor_finish||!b->motor_finish(b->context,a,rd<std::uint32_t>(a,0x908)))return 0;
    if(rd<float>(a,0x30)!=now){if(!dirty(a,*b))return 0;wr<float>(a,0x30,now);out->timestamp_changed=1;}
    out->completed=1;return 1;
}
int __cdecl ground_post(void* a,unsigned n,const ServerNpcRunAnimationInput* in,float interval,const ServerNpcActiveWeaponStepBindings* b,const ServerNpcActivePostAnimationBindings* p,ServerNpcActiveWeaponStepResult* step,ServerNpcActivePostAnimationResult* out) noexcept {
    if(!out)return 0;*out={};
    if(!p||!std::isfinite(interval)||interval<=0||!run(a,n,in,b,step))return 0;
    return post(a,n,in->now,interval,b->schedule,p,out);
}
int __cdecl jet_post(void* a,unsigned n,const ServerNpcRunAnimationInput* in,float interval,const ServerNpcActiveJetWeaponStepBindings* b,const ServerNpcActivePostAnimationBindings* p,ServerNpcActiveJetWeaponStepResult* step,ServerNpcActivePostAnimationResult* out) noexcept {
    if(!out)return 0;*out={};
    if(!p||!std::isfinite(interval)||interval<=0||!run_jet(a,n,in,b,step))return 0;
    return post(a,n,in->now,interval,b->base.schedule,p,out);
}
bool outer_class(std::uint32_t vt,unsigned n) noexcept {
    const auto* cls=SiN_GetServerNpcSubclassSchedulerApi(1,sizeof(ServerNpcSubclassSchedulerApi));
    unsigned family=0;return cls&&cls->classify(vt,&family)&&n>=(family==NpcSubclassJetpack?0x1085u:family==NpcSubclassElite?0xfe0u:0xfd8u);
}
int __cdecl outer_admit(void* a,unsigned n,float now,std::uint32_t vt,const ServerNpcOuterAdmissionBindings* b,ServerNpcOuterAdmissionResult* out) noexcept {
    if(!out)return 0;*out={};
    if(!a||!b||!b->storage||!outer_class(vt,n)||!std::isfinite(now))return 0;
    // 22040ce0: normal AI enabled; optional single-step gate.
    auto playback=[&](float value){
        if(rd<float>(a,0x3c0)!=value){if(!dirty(a,*b->storage))return false;wr<float>(a,0x3c0,value);out->playback_changed=1;}
        return true;
    };
    if(!(b->global_flags&1u)&&b->enabled){
        if(!(b->global_flags&2u)){out->admitted=out->completed=1;return 1;}
        out->step_checked=1;
        if(rd<int>(a,0xbf4)<b->step_threshold){
            if(!playback(1.f))return 0;out->admitted=out->completed=1;return 1;
        }
        void* nav=nullptr;
        if(!resolve(*b->storage,rd<std::uint32_t>(a,0x8f8),0x28,&nav))return 0;
        if(const auto token=rd<std::uint32_t>(nav,0x24)){
            void* path=nullptr;if(!resolve(*b->storage,token,4,&path))return 0;
            out->path_active=rd<std::uint32_t>(path,0)!=0;
        }
        if(!out->path_active&&!playback(0.f))return 0;
        out->completed=1;return 1;
    }
    // Disabled branch: shared diagnostic throttle, then +4dc(1).
    if(!b->warning_deadline||!std::isfinite(*b->warning_deadline))return 0;
    if(now>=*b->warning_deadline){
        const float deadline=static_cast<float>(double(now)+5.);
        if(!std::isfinite(deadline)||!b->warning)return 0;
        *b->warning_deadline=deadline;out->warning_called=1;
        if(!b->warning(b->context,a))return 0;
    }
    out->pause_called=1;
    if(!b->pause_activity||!b->pause_activity(b->context,a,1))return 0;
    wr<int>(a,0xfc8,-1);out->marker_reset=out->completed=1;return 1;
}
int __cdecl outer_script(void* a,unsigned n,float now,std::uint32_t vt,const ServerNpcOuterAdmissionBindings* b,ServerNpcOuterScriptGateResult* out) noexcept {
    if(!out)return 0;*out={};
    if(!a||!b||!b->storage||!outer_class(vt,n)||!std::isfinite(now))return 0;
    bool value=false;
    if(const auto token=rd<std::uint32_t>(a,0xc18)){
        out->component_called=1;if(!b->component_gate||!b->component_gate(b->context,token,&value))return 0;
    }else value=rd<int>(a,0x850)==4||rd<std::uint8_t>(a,0x968)||rd<float>(a,0x97c)>now||(rd<unsigned>(a,0xb4)&0x1000u);
    if(!value&&rd<std::uint8_t>(a,0xc10)==1){
        bool hit=false;out->probe_called=1;
        if(!b->movement_probe||!b->movement_probe(b->context,a,5,&hit))return 0;value=!hit;
    }
    out->value=value;
    if(rd<std::uint8_t>(a,0xc10)!=value){if(!dirty(a,*b->storage))return 0;wr<std::uint8_t>(a,0xc10,value);out->changed=1;}
    out->completed=1;return 1;
}
const ServerNpcOuterAdmissionApi admission_api{1,sizeof(ServerNpcOuterAdmissionApi),&outer_admit,&outer_script};
const ServerNpcActivePostAnimationApi post_api{1,sizeof(ServerNpcActivePostAnimationApi),&post,&ground_post,&jet_post};
const ServerNpcActiveWeaponStepApi api{1,sizeof(ServerNpcActiveWeaponStepApi),&run};
const ServerNpcActiveJetWeaponStepApi jet_api{1,sizeof(ServerNpcActiveJetWeaponStepApi),&run_jet};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcActiveWeaponStepApi=_SiN_GetServerNpcActiveWeaponStepApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcActiveWeaponStepApi* __cdecl SiN_GetServerNpcActiveWeaponStepApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcActiveWeaponStepApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcActiveJetWeaponStepApi=_SiN_GetServerNpcActiveJetWeaponStepApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcActiveJetWeaponStepApi* __cdecl SiN_GetServerNpcActiveJetWeaponStepApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcActiveJetWeaponStepApi)?&jet_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcActivePostAnimationApi=_SiN_GetServerNpcActivePostAnimationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcActivePostAnimationApi* __cdecl SiN_GetServerNpcActivePostAnimationApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcActivePostAnimationApi)?&post_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcOuterAdmissionApi=_SiN_GetServerNpcOuterAdmissionApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcOuterAdmissionApi* __cdecl SiN_GetServerNpcOuterAdmissionApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcOuterAdmissionApi)?&admission_api:nullptr;
}
