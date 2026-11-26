#include "native_server_npc_active_weapon_step.h"
#include <cmath>
#include <cstring>
#include <limits>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(std::uint32_t,std::uint32_t) noexcept;
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
bool set_tick(void* a,float time,float interval,const ServerNpcOuterCadenceBindings& b,ServerNpcOuterCadenceResult& o) noexcept {
    int tick=-1;
    if(time!=-1.f){
        const double q=std::trunc(double(time)/double(interval)+.5);
        if(!std::isfinite(q)||q<double(std::numeric_limits<int>::min())||q>double(std::numeric_limits<int>::max()))return false;
        tick=static_cast<int>(q);
    }
    // All reviewed +c0 targets are 22001740 RET, no callback needed.
    if(rd<int>(a,0x74)!=tick){wr<int>(a,0x74,tick);o.tick_written=1;}
    o.tick=tick;o.next_time=time;
    const auto flags=rd<unsigned>(a,0xb8);
    auto changed=[&](unsigned value){
        if(!b.notify_think_flags)return false;
        wr<unsigned>(a,0xb8,value);o.flags_changed=o.notify_called=1;
        return b.notify_think_flags(b.context,a)!=0;
    };
    if(flags&0x400000u){
        if(tick!=-1&&!changed(flags&~0x400000u))return false;
    }else if(tick==-1){
        // 2214bf00 checks strictly positive ticks, including named contexts.
        const int count=rd<int>(a,0x50);
        if(count>4096)return false;
        bool scheduled=false;
        if(count>0){
            void* rows=nullptr;
            if(!b.storage||!resolve(*b.storage,rd<unsigned>(a,0x44),16u*static_cast<unsigned>(count),&rows))return false;
            for(int i=0;i<count;++i)if(rd<int>(rows,16u*static_cast<unsigned>(i)+8)>0){scheduled=true;break;}
        }
        if(!scheduled&&!changed(flags|0x400000u))return false;
    }
    return true;
}
int __cdecl cadence_set(void* a,unsigned n,float time,float interval,std::uint32_t vt,const ServerNpcOuterCadenceBindings* b,ServerNpcOuterCadenceResult* out) noexcept {
    if(!out)return 0;*out={};
    if(!a||!b||!outer_class(vt,n)||!std::isfinite(time)||!std::isfinite(interval)||interval<=0)return 0;
    if(!set_tick(a,time,interval,*b,*out))return 0;out->completed=1;return 1;
}
int __cdecl cadence_finish(void* a,unsigned n,float now,float interval,std::uint32_t vt,bool pvs,bool think_called,const ServerNpcOuterCadenceBindings* b,ServerNpcOuterCadenceResult* out) noexcept {
    if(!out)return 0;*out={};
    if(!a||!b||!outer_class(vt,n)||!std::isfinite(now)||!std::isfinite(interval)||interval<=0)return 0;
    out->unscheduled=rd<int>(a,0x74)==-1;wr<std::uint8_t>(a,0x7c1,out->unscheduled!=0);
    out->mode_called=1;
    // 2203aa90 direct mode branches; mode callback is explicitly bounded.
    if(rd<int>(a,0x868)!=0){wr<int>(a,0x85c,4);out->mode_direct=1;}
    else if(!b->optimize_a||!b->optimize_b){wr<int>(a,0x85c,0);wr<int>(a,0x860,0);out->mode_direct=1;}
    else if(!b->dynamic_mode||!b->dynamic_mode(b->context,a,pvs))return 0;
    out->mode=rd<int>(a,0x85c);
    // Original rereads +7c1 after +4d4.
    if(!rd<std::uint8_t>(a,0x7c1)){
        wr<float>(a,0x864,0.f);out->deadline_written=1;out->deadline=0.f;out->tick=rd<int>(a,0x74);
    }else{
        if(think_called){
            constexpr float delay[]{.1f,.2f,.4f,.6f}; // 224dc174
            if(out->mode<0||out->mode>=4)return 0;
            out->deadline=static_cast<float>(double(now)+double(delay[out->mode]));if(!std::isfinite(out->deadline))return 0;
            wr<float>(a,0x864,out->deadline);out->deadline_written=1;
        }else out->deadline=rd<float>(a,0x864);
        const float next=static_cast<float>(double(now)+(rd<int>(a,0x860)&&out->mode?.2:.1));
        if(!std::isfinite(next)||!set_tick(a,next,interval,*b,*out))return 0;
    }
    out->completed=1;return 1;
}
struct OuterWakeRoute {
    const ServerNpcOuterWakeUpdateBindings& bindings;
    ServerNpcOuterWakeUpdateResult& result;
};
bool native_outer_wake(void*,unsigned,std::uint32_t,bool,const ServerNpcActiveWeaponStepBindings&,const OuterWakeRoute&) noexcept;
template<class Binding,class Step,class Execute>
int outer_run(void* a,unsigned n,const ServerNpcRunAnimationInput* in,float interval,const Binding* b,const ServerNpcActiveWeaponStepBindings* base,const ServerNpcOuterUpdateBindings* outer,Step* step,ServerNpcActivePostAnimationResult* tail,ServerNpcOuterUpdateResult* out,bool jet,Execute execute,const OuterWakeRoute* wake_route=nullptr) noexcept {
    if(!out||!step||!tail)return 0;
    *out={};*tail={};std::memset(step,0,sizeof(*step));step->animation.frame.frame.failed_event=-1;
    if(!a||!in||!std::isfinite(in->now)||!std::isfinite(interval)||interval<=0||!b||!base||!base->schedule||!outer||!outer->post)return 0;
    const auto vt=base->schedule->scheduler.original_vtable;
    const auto* cls=SiN_GetServerNpcSubclassSchedulerApi(1,sizeof(ServerNpcSubclassSchedulerApi));
    unsigned family=0;
    if(!outer_class(vt,n)||!cls||!cls->classify(vt,&family)||jet!=(family==NpcSubclassJetpack))return 0;
    const auto* condition=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    if(!condition||!outer->pvs)return 0;
    auto admission=outer->admission;admission.storage=outer->post;
    auto cadence=outer->cadence;cadence.storage=outer->post;
    const float now=in->now;
    if(rd<std::uint8_t>(a,0xbe1)){
        out->impact_called=1;if(!outer->pending_impact||!outer->pending_impact(outer->context,a))return 0;
    }
    if(!cadence_set(a,n,-1.f,interval,vt,&cadence,&out->clear))return 0;
    bool pvs=false;out->pvs_called=1;
    if(!outer->pvs(outer->context,rd<unsigned>(a,0x16c),&pvs))return 0;out->pvs=pvs;
    ServerNpcConditionResult mapped{};
    if(!condition->store(a,n,1,NpcConditionCurrent,pvs,&base->schedule->scheduler.maintenance.selection.grunt.schedule.conditions,&mapped))return 0;
    out->condition_mapped=mapped.mapped;
    if(rd<int>(a,0x868)>0){
        out->wake_called=1;
        if(wake_route){
            if(!native_outer_wake(a,n,vt,pvs,*base,*wake_route))return 0;
        }else if(!outer->wake||!outer->wake(outer->context,a,pvs))return 0;
    }
    if(rd<int>(a,0x85c)>=4||rd<int>(a,0x868)!=0)out->sleep_bypassed=1;
    else if(!outer->active_scene)out->scene_bypassed=1;
    else{
        if(!outer_script(a,n,now,vt,&admission,&out->script)||!outer_admit(a,n,now,vt,&admission,&out->admission))return 0;
        if(out->admission.admitted){
            out->active_called=1;
            if(!execute(a,n,in,interval,b,outer->post,step,tail))return 0;
            out->think_called=step->think_called;
        }else{wr<unsigned>(a,0x910,0x7f7fffffu);out->nav_sentinel_written=1;}
    }
    if(!cadence_finish(a,n,now,interval,vt,pvs,out->think_called!=0,&cadence,&out->finish))return 0;
    out->completed=1;return 1;
}
int __cdecl outer_ground(void* a,unsigned n,const ServerNpcRunAnimationInput* in,float interval,const ServerNpcActiveWeaponStepBindings* b,const ServerNpcOuterUpdateBindings* outer,ServerNpcActiveWeaponStepResult* step,ServerNpcActivePostAnimationResult* tail,ServerNpcOuterUpdateResult* out) noexcept {
    return outer_run(a,n,in,interval,b,b,outer,step,tail,out,false,&ground_post);
}
int __cdecl outer_jet(void* a,unsigned n,const ServerNpcRunAnimationInput* in,float interval,const ServerNpcActiveJetWeaponStepBindings* b,const ServerNpcOuterUpdateBindings* outer,ServerNpcActiveJetWeaponStepResult* step,ServerNpcActivePostAnimationResult* tail,ServerNpcOuterUpdateResult* out) noexcept {
    return outer_run(a,n,in,interval,b,b&&!b->base.think?&b->base:nullptr,outer,step,tail,out,true,&jet_post);
}
bool wake_transition_impl(void*,unsigned,std::uint32_t,bool,const ServerNpcWakeTransitionBindings&,ServerNpcWakeTransitionResult&,unsigned) noexcept;
struct WakeTransitionRoute {const ServerNpcWakeTransitionBindings& bindings;ServerNpcWakeTransitionResult& result;};
int wake_policy_impl(void* a,unsigned n,std::uint32_t vt,bool pvs,const ServerNpcWakePolicyBindings* b,ServerNpcWakePolicyResult* out,const WakeTransitionRoute* route) noexcept {
    if(!out)return 0;*out={};
    if(!a||!b||!outer_class(vt,n)||b->max_clients<0||b->max_clients>128)return 0;
    auto done=[&](){out->completed=1;return 1;};
    auto wake=[&](unsigned reason){
        ++out->wake_calls;out->reason=reason;
        if(route)return wake_transition_impl(a,n,vt,true,route->bindings,route->result,0);
        return b->wake&&b->wake(b->context,a,true)!=0;
    };
    if(rd<int>(a,0x868)<=0)return done();
    if(b->max_clients>1)return wake(NpcWakeMultiplayer)?done():0;
    void* player=nullptr;out->local_queried=1;
    if(!b->local_player||!b->local_player(b->context,&player))return 0;
    if(!player){
        out->missing_player=1;
        return b->missing_player&&b->missing_player(b->context)?done():0;
    }
    auto flags=[&](void* e,unsigned& value){
        unsigned bytes=0;if(!b->extent||!b->extent(b->context,e,&bytes)||bytes<0xc0)return false;
        value=rd<unsigned>(e,0xbc);return true;
    };
    const float radius=rd<float>(a,0x86c);
    if(!std::isfinite(radius))return 0;
    unsigned player_flags=0;
    if(double(radius)>0.1){
        if(!flags(player,player_flags))return 0;
        if(!(player_flags&0x8000u)){
            float own[3]{},other[3]{};
            if(!b->origin||!b->origin(b->context,a,own)||!b->origin(b->context,player,other))return 0;
            double d[3]{};
            for(unsigned i=0;i<3;++i){
                if(!std::isfinite(own[i])||!std::isfinite(other[i]))return 0;
                // 22027db0 stores each vector subtraction as float.
                d[i]=static_cast<float>(double(other[i])-own[i]);
                if(!std::isfinite(d[i]))return 0;
            }
            const double squared=d[0]*d[0]+d[1]*d[1]+d[2]*d[2];
            out->proximity_checked=1;out->distance_squared=static_cast<float>(squared);
            if(squared<=double(radius)*radius)return wake(NpcWakeProximity)?done():0;
        }
    }
    const int mode=rd<int>(a,0x868);
    if(mode==2)return !pvs||wake(NpcWakePvs)?done():0;
    if(mode!=1)return done();
    const auto* condition=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    constexpr int damage_conditions[]{16,17};
    for(int id:damage_conditions){
        ServerNpcConditionResult cr{};++out->damage_checked;
        if(!condition||!condition->test(a,n,id,NpcConditionCurrent,&b->conditions,&cr))return 0;
        if(cr.value)return wake(NpcWakeDamage)?done():0;
    }
    if(pvs)for(int i=1;i<=b->max_clients;++i){
        void* e=nullptr;++out->players_visited;
        if(!b->client||!b->client(b->context,i,&e))return 0;
        if(!e)continue;
        unsigned value=0;if(!flags(e,value))return 0;if(value&0x8000u)continue;
        bool visible=false;++out->visibility_calls;
        if(!b->player_visible||!b->player_visible(b->context,e,a,0x4081,&visible))return 0;
        if(visible&&!wake(NpcWakeVisiblePlayer))return 0;
        // Original loop does not return after a visibility wake.
    }
    // Verified all eight +500 targets return 0x141f, including danger bit8.
    if(rd<std::uint8_t>(a,0xb4)&1u)return done();
    int index=-1;if(!b->active_sound_head||!b->active_sound_head(b->context,&index))return 0;
    bool seen[64]{};
    while(index!=-1){
        if(index<0||index>=64||seen[index])return 0;seen[index]=true;
        const void* row=nullptr;unsigned bytes=0;
        if(!b->sound||!b->sound(b->context,index,&row,&bytes)||!row||bytes<0x2c)return 0;
        ++out->sounds_visited;
        if(rd<unsigned>(row,0xc)&8u){
            bool audible=false;++out->audible_calls;
            if(!b->sound_audible||!b->sound_audible(b->context,a,rd<unsigned>(a,0x898),row,&audible))return 0;
            if(audible){
                bool visible=false;++out->sound_visibility_calls;
                if(!b->sound_visible||!b->sound_visible(b->context,a,row,&visible))return 0;
                if(visible)return wake(NpcWakeSound)?done():0;
            }
        }
        index=rd<std::int16_t>(row,0x18);
    }
    return done();
}
// Original 22156900 passes a void variant into 22156680. Do not
// synthesize a target entity or invoke its input synchronously.
int __cdecl fire_void_output(void* output,unsigned n,void* activator,void* caller,unsigned caller_bytes,float extra,const ServerEntityVoidOutputBindings* b,ServerEntityVoidOutputResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!output||n<0x18||!b||(caller&&caller_bytes<0xa9)||!std::isfinite(extra))return 0;
    void* previous=output;unsigned link_off=0x14;
    unsigned token=rd<unsigned>(output,link_off),seen[256]{};
    while(token){
        if(o->visited>=256)return 0;
        for(unsigned i=0;i<o->visited;++i)if(seen[i]==token)return 0;
        seen[o->visited++]=token;
        void* row=nullptr;unsigned bytes=0;
        if(!b->resolve||!b->resolve(b->context,token,&row,&bytes)||!row||bytes<0x1c)return 0;
        ServerEntityVoidOutputAction action{};
        action.target=rd<unsigned>(row,0);action.input=rd<unsigned>(row,4);action.parameter=rd<unsigned>(row,8);
        action.action_id=rd<unsigned>(row,0x14);
        const float delay=rd<float>(row,0xc);
        action.variant_type=action.parameter?2u:0u;
        // Parameter override branch uses only the action delay.
        action.delay=action.parameter?delay:static_cast<float>(double(delay)+extra);
        if(!std::isfinite(action.delay)||!b->enqueue||!b->enqueue(b->context,activator,caller,&action))return 0;
        ++o->queued;if(action.parameter)++o->parameter_overrides;
        if(caller&&(rd<std::uint8_t>(caller,0xa8)&0x10u)){
            if(!b->debug_action||!b->debug_action(b->context,caller,&action))return 0;++o->debugged;
        }
        unsigned count=rd<unsigned>(row,0x10);
        if(count!=0xffffffffu){--count;wr<unsigned>(row,0x10,count);++o->countdowns;}
        const unsigned next=rd<unsigned>(row,0x18);
        if(!count){
            if(!b->release)return 0;
            wr<unsigned>(previous,link_off,next);
            if(!b->release(b->context,token,row))return 0;++o->removed;
        }else{previous=row;link_off=0x18;}
        token=next;
    }
    o->completed=1;return 1;
}
const ServerEntityVoidOutputApi void_output_api{1,sizeof(ServerEntityVoidOutputApi),&fire_void_output};
// Named event queue records from 22155300, stable sorted link insertion from
// 22154600. The independent allocator supplies a unique zero-free token.
int __cdecl enqueue_named_event(void* sentinel,unsigned n,std::uint32_t root,float now,std::uint32_t activator,std::uint32_t caller,const ServerEntityVoidOutputAction* action,const ServerEntityNamedEventQueueBindings* b,ServerEntityNamedEventQueueResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!sentinel||n<0x38||!root||!action||!b||!b->allocate||!std::isfinite(now)||!std::isfinite(action->delay))return 0;
    if(action->variant_type!=(action->parameter?2u:0u))return 0;
    o->fire_time=static_cast<float>(double(now)+action->delay);
    if(!std::isfinite(o->fire_time))return 0;
    auto normalize=[&](unsigned token,unsigned& value){
        value=0;if(!token)return true;
        bool nonempty=false;if(!b->string_nonempty||!b->string_nonempty(b->context,token,&nonempty))return false;
        if(nonempty)value=token;return true;
    };
    unsigned target=0,input=0;
    if(!normalize(action->target,target)||!normalize(action->input,input))return 0;
    void* previous=sentinel;unsigned prev_token=root,next=rd<unsigned>(sentinel,0x30),seen[512]{};
    void* successor=nullptr;
    while(next){
        if(next==root||o->visited>=512)return 0;
        for(unsigned i=0;i<o->visited;++i)if(seen[i]==next)return 0;
        seen[o->visited++]=next;
        unsigned bytes=0;
        if(!b->resolve||!b->resolve(b->context,next,&successor,&bytes)||!successor||bytes<0x38||successor==sentinel||rd<unsigned>(successor,0x34)!=prev_token)return 0;
        const float at=rd<float>(successor,0);if(!std::isfinite(at))return 0;
        // Equal deadlines remain in original insertion order.
        if(at>o->fire_time)break;
        previous=successor;prev_token=next;next=rd<unsigned>(successor,0x30);successor=nullptr;
    }
    void* row=nullptr;unsigned token=0,bytes=0;
    if(!b->allocate(b->context,&token,&row,&bytes)||!row||bytes<0x38||!token||token==root||row==sentinel||row==previous||row==successor)return 0;
    for(unsigned i=0;i<o->visited;++i)if(seen[i]==token)return 0;
    o->allocated=1;o->node=token;o->previous=prev_token;o->next=next;
    wr<float>(row,0,o->fire_time);wr<unsigned>(row,4,target);wr<unsigned>(row,8,input);
    wr<unsigned>(row,0xc,activator);wr<unsigned>(row,0x10,caller);wr<unsigned>(row,0x14,action->action_id);
    wr<unsigned>(row,0x18,0xffffffffu);wr<unsigned>(row,0x1c,action->parameter);
    // Unused void/string variant payload words are deterministic zero in
    // independent storage; they have no semantic content for these types.
    wr<unsigned>(row,0x20,0u);wr<unsigned>(row,0x24,0u);wr<unsigned>(row,0x28,0xffffffffu);wr<unsigned>(row,0x2c,action->variant_type);
    wr<unsigned>(row,0x30,next);wr<unsigned>(row,0x34,prev_token);
    wr<unsigned>(previous,0x30,token);if(successor)wr<unsigned>(successor,0x34,token);
    o->inserted=o->completed=1;return 1;
}
const ServerEntityNamedEventQueueApi named_event_queue_api{1,sizeof(ServerEntityNamedEventQueueApi),&enqueue_named_event};
// 22155530: service each due node under an engine lock, dispatch all name
// matches, then a live direct target, else class fallback if none matched.
// Re-read links after dispatch: AcceptInput can insert another earlier event.
int __cdecl service_events(void* sentinel,unsigned n,std::uint32_t root,float now,bool single_step,int* steps,const ServerEntityEventServiceBindings* b,ServerEntityEventServiceResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!sentinel||n<0x38||!root||!std::isfinite(now)||!b||(single_step&&!steps))return 0;
    if(b->entities.count>sin_independent::kEntityHandleCapacity||(b->entities.count&&!b->entities.entries))return 0;
    const sin_independent::EntityHandleView registry(b->entities.entries,b->entities.count);
    if(single_step&&*steps<=0){o->step_stopped=o->completed=1;return 1;}
    auto row_for=[&](unsigned token,void** row){
        unsigned bytes=0;*row=nullptr;
        return token&&token!=root&&b->resolve&&b->resolve(b->context,token,row,&bytes)&&*row&&*row!=sentinel&&bytes>=0x38;
    };
    for(unsigned processed=0;processed<512;++processed){
        const unsigned token=rd<unsigned>(sentinel,0x30);
        if(!token){o->completed=1;return 1;}
        void* row=nullptr;if(!row_for(token,&row)||rd<unsigned>(row,0x34)!=root)return 0;
        const float at=rd<float>(row,0);if(!std::isfinite(at))return 0;
        if(at>now){o->future_stopped=o->completed=1;return 1;}
        if(!b->lock||!b->lock(b->context,true))return 0;++o->locks;
        auto dispatch=[&](){
            const auto target=rd<unsigned>(row,4);
            bool found=false;
            auto deliver=[&](void* entity,unsigned& count){
                unsigned char variant[0x14];std::memcpy(variant,static_cast<unsigned char*>(row)+0x1c,sizeof(variant));
                if(!b->accept||!b->accept(b->context,entity,rd<unsigned>(row,8),registry.Resolve(rd<unsigned>(row,0xc)),registry.Resolve(rd<unsigned>(row,0x10)),variant,rd<unsigned>(row,0x14)))return false;
                ++count;++o->deliveries;found=true;return true;
            };
            if(target){
                if(!b->find_named)return false;
                void* prev=nullptr;
                for(unsigned i=0;;++i){
                    void* entity=nullptr;
                    if(!b->find_named(b->context,prev,target,registry.Resolve(rd<unsigned>(row,0xc)),registry.Resolve(rd<unsigned>(row,0x10)),&entity))return false;
                    if(!entity)break;
                    if(i==512||entity==prev||!deliver(entity,o->named))return false;prev=entity;
                }
            }
            if(void* direct=registry.Resolve(rd<unsigned>(row,0x18))){
                if(!deliver(direct,o->direct))return false;
            }else if(!found&&target){
                if(!b->find_class)return false;
                void* prev=nullptr;
                for(unsigned i=0;;++i){
                    void* entity=nullptr;if(!b->find_class(b->context,prev,target,&entity))return false;
                    if(!entity)break;
                    if(i==512||entity==prev||!deliver(entity,o->classname))return false;prev=entity;
                }
            }
            if(!found){
                ++o->missing;
                if(b->missing&&!b->missing(b->context,target,rd<unsigned>(row,8),registry.Resolve(rd<unsigned>(row,0x10))))return false;
            }
            const unsigned prev_token=rd<unsigned>(row,0x34),next=rd<unsigned>(row,0x30);
            void* prev=sentinel;void* successor=nullptr;
            if(prev_token!=root&&!row_for(prev_token,&prev))return false;
            if(prev==row||rd<unsigned>(prev,0x30)!=token)return false;
            if(next&&(!row_for(next,&successor)||successor==row||successor==prev||rd<unsigned>(successor,0x34)!=token))return false;
            if(!b->release)return false;
            wr<unsigned>(prev,0x30,next);if(successor)wr<unsigned>(successor,0x34,prev_token);
            if(!b->release(b->context,token,row))return false;++o->removed;return true;
        };
        const bool success=dispatch();
        if(success){++o->events;if(single_step&&*steps>0)--*steps;}
        const bool unlocked=b->lock(b->context,false)!=0;
        if(!success||!unlocked)return 0;
        if(single_step&&*steps<=0){o->step_stopped=o->completed=1;return 1;}
    }
    if(!rd<unsigned>(sentinel,0x30)){o->completed=1;return 1;}
    return 0;
}
const ServerEntityEventServiceApi event_service_api{1,sizeof(ServerEntityEventServiceApi),&service_events};
// Verified +34 datamap getters and +90 AcceptInput on the eight Grunt
// primary tables. Unknown classes are not silently treated as a Grunt.
unsigned input_datamap(unsigned vt) noexcept {
    switch(vt){
    case 0x2248df04u:return 0x22534674u;
    case 0x224954a4u:case 0x2249aaa4u:case 0x224a416cu:return 0x22534da4u;
    case 0x2248c514u:case 0x2248e83cu:case 0x2249964cu:case 0x224a4a4cu:return 0x22535b28u;
    default:return 0;
    }
}
// 223a5574 -> 223a8410 in the original C locale. A string pool
// supplies actual bytes and extent; a numeric token is not a name.
template<class B>bool input_text(const B& b,unsigned token,const char** text) noexcept {
    *text="";if(!token)return true;
    unsigned bytes=0;
    if(!b.string_text||!b.string_text(b.context,token,text,&bytes)||!*text||!bytes)return false;
    const unsigned bound=bytes<4096u?bytes:4096u;
    return std::memchr(*text,0,bound)!=nullptr;
}
bool input_name_equal(const ServerEntityAcceptInputBindings& b,unsigned left,unsigned right,bool& same) noexcept {
    if(b.name_equal)return b.name_equal(b.context,left,right,&same)!=0;
    const char* a=nullptr;const char* c=nullptr;
    if(!input_text(b,left,&a)||!input_text(b,right,&c))return false;
    auto lower=[](unsigned char x){return x>='A'&&x<='Z'?static_cast<unsigned char>(x+32):x;};
    while(*a&&lower(static_cast<unsigned char>(*a))==lower(static_cast<unsigned char>(*c))){++a;++c;}
    same=lower(static_cast<unsigned char>(*a))==lower(static_cast<unsigned char>(*c));return true;
}
// 22138740/221387f0: ASCII comparison until mismatch, then a star
// at the current pattern position accepts all remaining characters.
unsigned char search_lower(unsigned char x) noexcept {return x>='A'&&x<='Z'?static_cast<unsigned char>(x+32):x;}
bool search_equal(const char* a,const char* b) noexcept {
    while(*a&&search_lower(static_cast<unsigned char>(*a))==search_lower(static_cast<unsigned char>(*b))){++a;++b;}
    return search_lower(static_cast<unsigned char>(*a))==search_lower(static_cast<unsigned char>(*b));
}
bool search_pattern(const char* name,const char* pattern) noexcept {
    while(*name&&*pattern&&search_lower(static_cast<unsigned char>(*name))==search_lower(static_cast<unsigned char>(*pattern))){++name;++pattern;}
    return (!*name&&!*pattern)||*pattern=='*';
}
bool search_handle(void* entity,const ServerEntitySearchBindings& b,unsigned& handle) noexcept {
    unsigned bytes=0;
    if(!b.extent||!b.extent(b.context,entity,&bytes)||bytes<4)return false;
    if(input_datamap(rd<unsigned>(entity,0))){
        if(bytes<0x37c)return false;handle=rd<unsigned>(entity,0x378);return true;
    }
    return b.foreign_handle&&b.foreign_handle(b.context,entity,&handle)!=0;
}
int search_entities(void* previous,unsigned token,void* activator,void* caller,bool classname,const ServerEntitySearchBindings* b,ServerEntitySearchResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!b||b->entities.count>sin_independent::kEntityHandleCapacity||(b->entities.count&&!b->entities.entries))return 0;
    const char* pattern=nullptr;if(!input_text(*b,token,&pattern))return 0;
    auto done=[&](){o->matched=o->entity!=nullptr;o->completed=1;return 1;};
    if(!classname){
        if(!*pattern)return done();
        if(*pattern=='!'){
            o->contextual=1;if(previous)return done();
            const char* keyword=pattern+1;
            if(search_equal(keyword,"self"))o->entity=caller;
            else if(search_equal(keyword,"activator"))o->entity=activator;
            else if(search_equal(keyword,"player")){
                if(!b->player_one||!b->player_one(b->context,&o->entity))return 0;
            }else if(search_equal(keyword,"pvsplayer")){
                void* reference=caller?caller:activator;
                if(!reference){if(!b->player_one||!b->player_one(b->context,&o->entity))return 0;}
                else {
                    unsigned bytes=0;if(!b->extent||!b->extent(b->context,reference,&bytes)||bytes<0x170||!b->pvs_player||!b->pvs_player(b->context,rd<unsigned>(reference,0x16c),&o->entity))return 0;
                }
            }else if(search_equal(keyword,"picker")){
                void* player=nullptr;if(!b->player_one||!b->player_one(b->context,&player)||!b->picker||!b->picker(b->context,player,&o->entity))return 0;
            }else if(b->invalid_name&&!b->invalid_name(b->context,token))return 0;
            return done();
        }
    }
    const auto* entries=b->entities.entries;
    const sin_independent::EntityHandleView registry(entries,b->entities.count);
    const sin_independent::EntityHandleEntry* row=b->head;
    if(previous){
        unsigned handle=0;if(!search_handle(previous,*b,handle))return 0;
        if(registry.Resolve(handle)!=previous)return 0;
        row=entries[sin_independent::SplitEntityHandle(handle).entry].next;
    }
    bool seen[sin_independent::kEntityHandleCapacity]{};
    while(row){
        const auto address=reinterpret_cast<std::uintptr_t>(row),base=reinterpret_cast<std::uintptr_t>(entries);
        if(address<base)return 0;
        const auto delta=address-base;
        if(delta%sizeof(*row)||delta/sizeof(*row)>=b->entities.count)return 0;
        const auto i=delta/sizeof(*row);if(seen[i])return 0;seen[i]=true;++o->visited;
        void* entity=row->entity;
        if(!entity){++o->empty_slots;row=row->next;continue;}
        unsigned bytes=0;if(!b->extent||!b->extent(b->context,entity,&bytes)||bytes<(classname?0x10u:0xc4u))return 0;
        const unsigned own=rd<unsigned>(entity,classname?0xcu:0xc0u);
        if(classname||own){
            if(own==token){o->entity=entity;return done();}
            const char* text=nullptr;if(!input_text(*b,own,&text))return 0;
            if(search_pattern(text,pattern)){o->entity=entity;return done();}
        }
        row=row->next;
    }
    return done();
}
int __cdecl search_named(void* previous,unsigned token,void* activator,void* caller,const ServerEntitySearchBindings* b,ServerEntitySearchResult* o) noexcept {
    return search_entities(previous,token,activator,caller,false,b,o);
}
int __cdecl search_class(void* previous,unsigned token,const ServerEntitySearchBindings* b,ServerEntitySearchResult* o) noexcept {
    return search_entities(previous,token,nullptr,nullptr,true,b,o);
}
const ServerEntitySearchApi entity_search_api{1,sizeof(ServerEntitySearchApi),&search_named,&search_class};
// Original 223a56cb tail-jumps to 223a5673. Its decimal accumulation is
// explicitly modulo 2^32, including overflow and final two's-complement sign.
unsigned input_decimal(const char* p) noexcept {
    while(*p==' '||(*p>='\t'&&*p<='\r'))++p;
    const bool negative=*p=='-';if(negative||*p=='+')++p;
    unsigned v=0;
    while(*p>='0'&&*p<='9'){v=v*10u+static_cast<unsigned>(*p-'0');++p;}
    return negative?0u-v:v;
}
// Pure conversion leaves of 22155e60. Return false only for missing host
// services, distinguish a rejected conversion from a malformed binding.
bool input_convert(void* v,unsigned wanted,const ServerEntityAcceptInputBindings& b,bool& ok) noexcept {
    const unsigned type=rd<unsigned>(v,0x10);ok=true;
    if(type==wanted||wanted==20u)return true;
    if(!wanted){wr<unsigned>(v,0,0u);wr<unsigned>(v,0x10,0u);return true;}
    if(type==5u&&wanted==1u){wr<float>(v,0,static_cast<float>(rd<int>(v,0)));wr<unsigned>(v,0x10,1u);return true;}
    if(type==5u&&wanted==6u){wr<std::uint8_t>(v,0,rd<int>(v,0)!=0);wr<unsigned>(v,0x10,6u);return true;}
    if(type==1u&&wanted==5u){
        const double value=std::trunc(double(rd<float>(v,0)));
        const int i=(!std::isfinite(value)||value<double(std::numeric_limits<int>::min())||value>double(std::numeric_limits<int>::max()))?std::numeric_limits<int>::min():static_cast<int>(value);
        wr<int>(v,0,i);wr<unsigned>(v,0x10,5u);return true;
    }
    if(type==1u&&wanted==6u){wr<std::uint8_t>(v,0,rd<float>(v,0)!=0.f);wr<unsigned>(v,0x10,6u);return true;}
    // Void -> string is accepted by AcceptInput itself without conversion.
    if(type==2u&&(wanted==5u||wanted==6u)&&b.string_text&&!b.convert){
        const char* text=nullptr;if(!input_text(b,rd<unsigned>(v,0),&text))return false;
        const unsigned value=input_decimal(text);
        if(wanted==5u)wr<unsigned>(v,0,value);else wr<std::uint8_t>(v,0,value!=0);
        wr<unsigned>(v,0x10,wanted);return true;
    }
    if(type==2u&&wanted==13u&&b.search&&!b.convert){
        // 22156118 -> 221619e0 with null previous/activator/caller;
        // 2228f8e0 stores the matched entity's handle or -1.
        auto search=*b.search;search.entities=b.entities;
        ServerEntitySearchResult result{};
        const unsigned token=rd<unsigned>(v,0);
        if(token&&!search_named(nullptr,token,nullptr,nullptr,&search,&result))return false;
        unsigned handle=0xffffffffu;
        if(result.entity&&!search_handle(result.entity,search,handle))return false;
        wr<unsigned>(v,0xc,handle);wr<unsigned>(v,0x10,13u);return true;
    }
    if(type==13u&&wanted==2u&&b.search&&!b.convert){
        // 22156140 returns true for an invalid/stale handle without
        // changing the payload or type. A live handle yields targetname.
        if(b.entities.count>sin_independent::kEntityHandleCapacity||(b.entities.count&&!b.entities.entries))return false;
        void* entity=sin_independent::EntityHandleView(b.entities.entries,b.entities.count).Resolve(rd<unsigned>(v,0xc));
        if(entity){
            unsigned bytes=0;if(!b.search->extent||!b.search->extent(b.search->context,entity,&bytes)||bytes<0xc4)return false;
            wr<unsigned>(v,0,rd<unsigned>(entity,0xc0));wr<unsigned>(v,0x10,2u);
        }
        return true;
    }
    if(type==2u&&(wanted==1u||wanted==5u||wanted==6u||wanted==3u||wanted==9u||wanted==13u)){
        if(!b.convert)return false;return b.convert(b.context,v,wanted,&ok)!=0;
    }
    if(type==13u&&wanted==2u){if(!b.convert)return false;return b.convert(b.context,v,wanted,&ok)!=0;}
    ok=false;return true;
}
bool input_write(void* a,unsigned n,unsigned offset,const void* v,const ServerEntityAcceptInputBindings& b,unsigned& written) noexcept {
    unsigned size=0;const unsigned type=rd<unsigned>(v,0x10);
    switch(type){case 1:case 2:case 5:case 9:case 12:case 13:size=4;break;case 3:case 15:size=12;break;case 6:case 8:size=1;break;case 7:size=2;break;default:return true;}
    if(offset>n||size>n-offset)return false;
    void* dest=static_cast<unsigned char*>(a)+offset;
    if(type==6u)wr<std::uint8_t>(dest,0,rd<std::uint8_t>(v,0)!=0);
    else if(type==13u)wr<unsigned>(dest,0,rd<unsigned>(v,0xc));
    else if(type==12u){
        // Original writes a resolved pointer, not the serial handle.
        if(b.entities.count>sin_independent::kEntityHandleCapacity||(b.entities.count&&!b.entities.entries))return false;
        auto* e=sin_independent::EntityHandleView(b.entities.entries,b.entities.count).Resolve(rd<unsigned>(v,0xc));
        wr<unsigned>(dest,0,static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(e)));
    }else std::memcpy(dest,v,size);
    written=size;return true;
}
// Input-only projection of all 27 input-enabled rows in the four original
// NPC maps. Non-input descriptors are not synthesized as save/restore data.
struct NativeInputRow {
    unsigned index,type,name,handler;
    const char* text;
};
constexpr NativeInputRow base_npc_inputs[]{
    {121,2,0x223db450u,0x22031f90u,"SetRelationship"},
    {122,5,0x223db434u,0x22037a30u,"SetHealth"},
    {123,0,0x223db414u,0x22031fe0u,"BeginRappel"},
    {124,0,0x223db3f4u,0x22031ff0u,"PauseRappel"},
    {125,2,0x223db3d8u,0x22032000u,"SetSquad"},
    {126,0,0x223db3c4u,0x220320b0u,"Wake"},
    {127,2,0x223db3a0u,0x2203f1f0u,"ForgetEntity"},
    {128,1,0x223db374u,0x220320c0u,"IgnoreDangerSounds"},
    {129,0,0x223db360u,0x220424a0u,"Break"},
    {130,0,0x223db33cu,0x2202e7d0u,"StartScripting"},
    {131,0,0x223db318u,0x2202e7e0u,"StopScripting"},
    {132,0,0x223db2fcu,0x22044d80u,"GagEnable"},
    {133,0,0x223db2d8u,0x22042c00u,"GagDisable"},
    {134,2,0x223db2b8u,0x22032120u,"SetVoiceGroup"}
};
constexpr NativeInputRow grunt_inputs[]{
    {31,0,0x22490f34u,0x2235f260u,"LookOff"},
    {32,0,0x22490f20u,0x2235ddc0u,"LookOn"},
    {33,0,0x22485b74u,0x22357e30u,"StartPatrolling"},
    {34,0,0x22485b50u,0x22357e40u,"StopPatrolling"},
    {35,2,0x223f5624u,0x22357e50u,"Assault"},
    {36,6,0x22490eecu,0x22357ea0u,"ThrowGrenades"}
};
constexpr NativeInputRow jet_inputs[]{
    {19,2,0x22498948u,0x22365fe0u,"ChangePathTrack"},
    {20,0,0x22498914u,0x22363b80u,"EnableRelativePathnode"},
    {21,0,0x224988dcu,0x22363580u,"DisableRelativePathnode"},
    {22,0,0x224988b8u,0x22367470u,"ActivateJets"},
    {23,0,0x22498894u,0x22366110u,"DeactivateJets"},
    {24,0,0x2249886cu,0x22363be0u,"EnableForcePath"},
    {25,0,0x22498840u,0x22363c40u,"DisableForcePath"}
};
struct NativeInputMap {
    unsigned token,parent,count;
    const NativeInputRow* rows;
    unsigned input_count;
};
constexpr NativeInputMap npc_input_maps[]{
    {0x224dbd9cu,0x2251f258u,139,base_npc_inputs,14},
    {0x22534da4u,0x224dbd9cu,39,grunt_inputs,6},
    {0x22534674u,0x22534da4u,2,nullptr,0},
    {0x22535b28u,0x22534da4u,26,jet_inputs,7}
};
// 22032000 SetSquad, 220a8050 manager lookup, and fixed-array
// membership edits from 220aa0d0/220aa1b0. The allocator contract supplies
// original-address-shaped tokens (group+8 is its embedded vector token).
int native_set_squad(void* a,unsigned n,const ServerEntityInputData& d,const ServerEntityAcceptInputBindings& b,ServerEntityAcceptInputResult& o) noexcept {
    if(!b.squad)return -1;
    const auto& s=*b.squad;
    if(n<0x9cc||!s.head||!b.storage||b.entities.count>sin_independent::kEntityHandleCapacity||(b.entities.count&&!b.entities.entries))return 0;
    const sin_independent::EntityHandleView registry(b.entities.entries,b.entities.count);
    unsigned caps=rd<unsigned>(a,0x8ec);
    if(void* weapon=registry.Resolve(rd<unsigned>(a,0x78c))){
        unsigned extra=0;if(!s.weapon_capabilities||!s.weapon_capabilities(s.context,weapon,&extra))return 0;caps|=extra;
    }
    auto warning=[&](unsigned token){return !s.warning||s.warning(s.context,a,token);};
    if(!(caps&0x4000000u)){++o.squad_denied;return warning(0x223d9bc0u);}
    const unsigned name=rd<unsigned>(d.variant,0x10)==2u?rd<unsigned>(d.variant,0):0u;
    wr<unsigned>(a,0x9c8,name);
    auto remove=[&](unsigned group_token,bool notify){
        if(!group_token)return true;
        void* group=nullptr;if(!resolve(*b.storage,group_token,0x4c,&group))return false;
        const int count=rd<int>(group,0x48);if(count<0||count>16)return false;
        const unsigned handle=rd<unsigned>(a,0x378);
        if(registry.Resolve(handle)!=a)return false;
        int i=0;for(;i<count;++i)if(registry.Resolve(rd<unsigned>(group,8+4*i))==a)break;
        if(i==count)return warning(0x223ee75cu);
        auto* rows=static_cast<unsigned char*>(group)+8;
        std::memmove(rows+4*i,rows+4*(i+1),4u*static_cast<unsigned>(count-i-1));
        wr<int>(group,0x48,count-1);++o.squad_removed;
        if(notify){
            for(int j=0;j<rd<int>(group,0x48);++j){
                if(j>=16)return false;
                void* peer=registry.Resolve(rd<unsigned>(group,8+4*j));if(!peer)continue;
                if(!s.removed_notice||!s.removed_notice(s.context,peer,a))return false;++o.squad_notices;
            }
        }
        // All reviewed +690 virtuals are the direct +9c4 setter.
        wr<unsigned>(a,0x9c4,0u);wr<unsigned>(a,0x9c8,0u);return true;
    };
    if(!name){
        if(!remove(rd<unsigned>(a,0x9c4),true))return 0;
        wr<unsigned>(a,0x9c4,0u);return 1;
    }
    unsigned token=*s.head,seen[256]{};
    void* group=nullptr;
    while(token){
        if(o.squad_visited>=256)return 0;
        for(unsigned i=0;i<o.squad_visited;++i)if(seen[i]==token)return 0;
        seen[o.squad_visited++]=token;
        if(!resolve(*b.storage,token,0x4c,&group))return 0;
        bool same=false;if(!input_name_equal(b,rd<unsigned>(group,4),name,same))return 0;
        if(same)break;
        token=rd<unsigned>(group,0);group=nullptr;
    }
    if(!group){
        unsigned bytes=0,canonical=0;
        if(!s.allocate||!s.intern||!s.allocate(s.context,&token,&group,&bytes)||!token||token>0xfffffff7u||!group||bytes<0x78||
           !s.intern(s.context,name,&canonical))return 0;
        for(unsigned i=0;i<o.squad_visited;++i)if(seen[i]==token)return 0;
        // Original constructor initializes the embedded list and manager
        // fields, then links the named group at the global head.
        wr<unsigned>(group,4,canonical);wr<unsigned>(group,0x48,0u);
        wr<unsigned>(group,0x4c,token+8);wr<unsigned>(group,0x50,0u);
        wr<unsigned>(group,0x58,0xffffffffu);wr<unsigned>(group,0x5c,0u);
        for(unsigned off=0x60;off<=0x74;off+=4)wr<unsigned>(group,off,0u);
        wr<unsigned>(group,0,*s.head);*s.head=token;++o.squad_created;
    }
    const unsigned previous=rd<unsigned>(a,0x9c4);
    // 220aa1b0 skips dead members or the same group. InputSetSquad still
    // assigns the requested group token after that helper returns.
    if(rd<int>(a,0x8c)!=0||previous==token){wr<unsigned>(a,0x9c4,token);return 1;}
    if(previous&&!remove(previous,false))return 0;
    int count=rd<int>(group,0x48);if(count<0||count>16)return 0;
    if(count==16){
        if(!warning(0x223ee798u))return 0;
        --count;wr<int>(group,0x48,count);++o.squad_full;
    }
    const unsigned handle=rd<unsigned>(a,0x378);
    if(registry.Resolve(handle)!=a||token>0xfffffff7u)return 0;
    // 220a9790 appends and refreshes the embedded vector pointer at +4c.
    // Existing groups must expose this complete header.
    void* checked=nullptr;if(!resolve(*b.storage,token,0x50,&checked)||checked!=group)return 0;
    wr<unsigned>(group,8+4*count,handle);wr<int>(group,0x48,count+1);wr<unsigned>(group,0x4c,token+8);
    wr<unsigned>(a,0x9c4,token);wr<unsigned>(a,0x9c8,rd<unsigned>(group,4));++o.squad_added;
    if(count>0){
        void* leader=registry.Resolve(rd<unsigned>(group,8));
        if(!leader||!s.copy_leader_memory||!s.copy_leader_memory(s.context,a,leader))return 0;++o.squad_memory;
    }
    return 1;
}
// Original concrete handlers, independent from the descriptor projection.
// Return -1 for a handler not implemented here, 0 for malformed storage,
// and 1 for a successfully executed native leaf.
int native_npc_input(void* a,unsigned n,unsigned vt,unsigned handler,const ServerEntityInputData& d,const ServerEntityAcceptInputBindings& b,ServerEntityAcceptInputResult& o) noexcept {
    if(!input_datamap(vt))return 0;
    auto byte=[&](unsigned offset,std::uint8_t value){
        if(n<=offset)return 0;
        wr<std::uint8_t>(a,offset,value);return 1;
    };
    auto flag=[&](unsigned mask,bool set){
        if(n<0x204)return 0;
        const unsigned old=rd<unsigned>(a,0xb4),value=set?old|mask:old&~mask;
        if(old!=value){
            if(!dirty(a,*b.storage))return 0;
            ++o.handler_dirty;wr<unsigned>(a,0xb4,value);
        }
        return 1;
    };
    switch(handler){
    case 0x22032000u:return native_set_squad(a,n,d,b,o);
    case 0x2202e7d0u:return byte(0x968,1); // StartScripting
    case 0x2202e7e0u:return byte(0x968,0); // StopScripting
    case 0x22044d80u:return flag(2u,true);  // GagEnable
    case 0x22042c00u:return flag(2u,false);
    case 0x2235f260u:return flag(0x10000u,true); // LookOff
    case 0x2235ddc0u:return flag(0x10000u,false);
    case 0x22357e30u:return byte(0xc65,1); // StartPatrolling
    case 0x22357e40u:return byte(0xc65,0);
    case 0x22357ea0u: // ThrowGrenades: clear height even on false
        if(n<0xc74)return 0;
        wr<unsigned>(a,0xc70,0u);
        wr<std::uint8_t>(a,0xc6c,rd<unsigned>(d.variant,0x10)==6u?rd<std::uint8_t>(d.variant,0):0u);
        return 1;
    case 0x220320c0u:{ // IgnoreDangerSounds: positive duration else 10
        if(n<0xc10||!std::isfinite(b.now))return 0;
        const float requested=rd<unsigned>(d.variant,0x10)==1u?rd<float>(d.variant,0):0.f;
        const float duration=requested>0.f?requested:10.f;
        wr<float>(a,0xc0c,static_cast<float>(double(b.now)+duration));return 1;
    }
    case 0x22032120u:{ // SetVoiceGroup -> shared +5b0, 220448a0
        if(n<0x8ec)return 0;
        const unsigned type=rd<unsigned>(d.variant,0x10);
        if(type!=0u&&type!=2u)return -1; // formatted non-string variants still hosted
        unsigned token=type==2u?rd<unsigned>(d.variant,0):0u;
        if(token){
            const char* text=nullptr;if(!input_text(b,token,&text))return 0;
            if(!*text)token=0;
        }
        wr<unsigned>(a,0x8e8,token);return 1;
    }
    case 0x22363580u: // DisableRelativePathnode does not validate path
        if(input_datamap(vt)!=0x22535b28u)return 0;
        return byte(0x1084,0);
    case 0x22363b80u:case 0x22363be0u:case 0x22363c40u:{
        if(input_datamap(vt)!=0x22535b28u||n<0xff8)return 0;
        if(b.entities.count>sin_independent::kEntityHandleCapacity||(b.entities.count&&!b.entities.entries))return 0;
        const auto registry=sin_independent::EntityHandleView(b.entities.entries,b.entities.count);
        if(!registry.Resolve(rd<unsigned>(a,0xff4))){
            ++o.path_missing;
            const unsigned warning=handler==0x22363b80u?0x224982e8u:handler==0x22363be0u?0x22498324u:0x22498358u;
            return !b.path_warning||b.path_warning(b.context,a,warning);
        }
        if(handler==0x22363c40u)return byte(0x1095,0);
        // The original sets +1084, and conditionally +1085 if +b4 bit22.
        // EnableForcePath also writes +1095; validate extent before mutation.
        const bool pending=(rd<unsigned>(a,0xb4)&0x400000u)!=0;
        if(n<=0x1084||(pending&&n<=0x1085)||(handler==0x22363be0u&&n<=0x1095))return 0;
        wr<std::uint8_t>(a,0x1084,1);
        if(pending)wr<std::uint8_t>(a,0x1085,1);
        if(handler==0x22363be0u)wr<std::uint8_t>(a,0x1095,1);
        return 1;
    }
    default:return -1;
    }
}
const NativeInputMap* native_input_map(unsigned token) noexcept {
    for(const auto& m:npc_input_maps)if(m.token==token)return &m;
    return nullptr;
}
int __cdecl wake_transition(void*,unsigned,std::uint32_t,bool,const ServerNpcWakeTransitionBindings*,ServerNpcWakeTransitionResult*) noexcept;
int __cdecl accept_input(void* a,unsigned n,std::uint32_t vt,std::uint32_t input,void* activator,void* caller,const void* variant,unsigned vn,std::uint32_t id,const ServerEntityAcceptInputBindings* b,ServerEntityAcceptInputResult* o) noexcept {
    if(!o)return 0;*o={};
    unsigned token=input_datamap(vt),seen[32]{};
    if(!a||n<0x204||!token||!variant||vn<0x14||!b||!b->storage||(!b->name_equal&&!b->string_text))return 0;
    ServerEntityInputData data{};data.activator=activator;data.caller=caller;data.output_id=id;
    std::memcpy(data.variant,variant,0x14);
    auto rejected=[&](unsigned reason){
        o->reason=reason;if(b->diagnostic&&!b->diagnostic(b->context,a,caller,input,reason,data.variant))return 0;
        o->completed=1;return 1;
    };
    while(token){
        if(o->maps>=32)return 0;
        for(unsigned i=0;i<o->maps;++i)if(seen[i]==token)return 0;
        seen[o->maps++]=token;
        const auto* native=b->native_npc_datamaps?native_input_map(token):nullptr;
        void* map=nullptr;void* rows=nullptr;int count=0;unsigned parent=0;
        if(native){
            ++o->native_maps;count=static_cast<int>(native->count);parent=native->parent;
        }else{
            if(!resolve(*b->storage,token,0x10,&map))return 0;
            count=rd<int>(map,4);if(count<0||count>4096)return 0;
            if(count&&!resolve(*b->storage,rd<unsigned>(map,0),0x34u*static_cast<unsigned>(count),&rows))return 0;
            parent=rd<unsigned>(map,0xc);
        }
        unsigned native_index=0;
        for(int i=0;i<count;++i){
            ++o->rows;
            unsigned char projected[0x34]{};
            const NativeInputRow* descriptor=nullptr;
            const unsigned char* row=nullptr;
            if(native){
                if(native_index==native->input_count||native->rows[native_index].index!=static_cast<unsigned>(i))continue;
                descriptor=&native->rows[native_index++];
                wr<unsigned>(projected,0,descriptor->type);
                wr<std::uint16_t>(projected,0x12,8u);
                wr<unsigned>(projected,0x14,descriptor->name);
                wr<unsigned>(projected,0x1c,descriptor->handler);
                row=projected;
            }else row=static_cast<const unsigned char*>(rows)+0x34*i;
            const unsigned flags=rd<std::uint16_t>(row,0x12);
            if(!(flags&8u))continue;
            bool same=false;
            if(descriptor&&!b->name_equal){
                if(input==descriptor->name)same=true;
                else{const char* actual=nullptr;if(!input_text(*b,input,&actual))return 0;same=search_equal(descriptor->text,actual);}
            }else if(!input_name_equal(*b,rd<unsigned>(row,0x14),input,same))return 0;
            if(!same)continue;
            o->matched=1;o->map=token;o->row=static_cast<unsigned>(i);o->handler=rd<unsigned>(row,0x1c);
            o->expected_type=rd<unsigned>(row,0);o->actual_type=rd<unsigned>(data.variant,0x10);
            if(o->actual_type!=o->expected_type&&!(o->actual_type==0&&o->expected_type==2)){
                bool ok=false;if(!input_convert(data.variant,o->expected_type,*b,ok))return 0;
                if(!ok)return rejected(EntityInputBadConversion);
                o->converted=1;o->actual_type=rd<unsigned>(data.variant,0x10);
            }
            if(o->handler==0x220320b0u&&b->wake){
                // InputWake replaces the inputdata stack argument with true,
                // then tail-jumps to the authenticated +4d8 wake override.
                auto bound=*b->wake;bound.storage=b->storage;bound.entities=b->entities;
                ServerNpcWakeTransitionResult local{};
                auto* result=b->wake_result?b->wake_result:&local;
                if(!wake_transition(a,n,vt,true,&bound,result))return 0;
                o->native_wake=1;
            }else if(o->handler){
                const int status=b->native_npc_handlers?native_npc_input(a,n,vt,o->handler,data,*b,*o):-1;
                if(status==0)return 0;
                if(status>0)o->native_handler=1;
                else if(!b->handler||!b->handler(b->context,a,o->handler,&data))return 0;
            }
            else if(flags&4u){if(!input_write(a,n,rd<unsigned>(row,8),data.variant,*b,o->written))return 0;}
            if(!dirty(a,*b->storage))return 0;
            o->dirty=o->accepted=o->completed=1;return 1;
        }
        token=parent;
    }
    return rejected(EntityInputUnknown);
}
const ServerEntityAcceptInputApi accept_input_api{1,sizeof(ServerEntityAcceptInputApi),&accept_input};
bool wake_transition_impl(void* a,unsigned n,std::uint32_t vt,bool output,const ServerNpcWakeTransitionBindings& b,ServerNpcWakeTransitionResult& o,unsigned depth) noexcept;
// 2213c980. All authenticated Grunt primary +18/+1c virtuals
// are 22001bc0 (read +38) and 22001af0 (copy +240).
bool wake_refresh_entity(void* a,const ServerNpcWakeTransitionBindings& b) noexcept {
    const auto token=rd<unsigned>(a,0x16c);
    if(!token)return true;
    if(!b.storage)return false;
    void* word=nullptr;if(!resolve(*b.storage,token,4,&word))return false;
    const auto flags=rd<unsigned>(a,0xb8),kind=rd<unsigned>(a,0x38);
    const auto prior=rd<unsigned>(word,0);
    unsigned value=0;
    if((rd<unsigned>(a,0x78)&0x20u)||(!(flags&0x80u)&&(!kind||!rd<unsigned>(a,0x240))))
        value=(prior&~0x28u)|0x10u;
    else if(kind==1||(flags&0x20000u))
        value=(prior&~0x30u)|8u;
    else value=(prior&~0x18u)|0x20u;
    wr<unsigned>(word,0,value);return true;
}
// Original filtered 220a9300/220a93c0 for authenticated Grunt rows.
// Embedded collision component 22446954 +2c -> 221ffc00 (read +20),
// and each of the eight primary Grunt +694 targets -> 22044680 (false).
bool wake_squad_next(std::uint32_t token,int& cursor,void** member,const ServerNpcWakeTransitionBindings& b) noexcept {
    *member=nullptr;
    if(!b.storage||cursor< -1||cursor>=16||b.entities.count>sin_independent::kEntityHandleCapacity||(b.entities.count&&!b.entities.entries))return false;
    void* group=nullptr;if(!resolve(*b.storage,token,0x4c,&group))return false;
    const int count=rd<int>(group,0x48);if(count<0||count>16)return false;
    const sin_independent::EntityHandleView registry(b.entities.entries,b.entities.count);
    for(++cursor;cursor<count;++cursor){
        void* e=registry.Resolve(rd<unsigned>(group,8+4*cursor));if(!e)continue;
        unsigned bytes=0;if(!b.extent||!b.extent(b.context,e,&bytes)||bytes<0x208)return false;
        if(!rd<unsigned>(e,0x204)){
            if(rd<unsigned>(e,0xfc)!=0x22446954u)return false;
            if(!rd<unsigned>(e,0x11c))continue;
        }
        if(!outer_class(rd<unsigned>(e,0),bytes))return false; // foreign rejection virtual not recovered here
        *member=registry.Resolve(rd<unsigned>(group,8+4*cursor));
        return true;
    }
    return true;
}
bool wake_member(void* e,bool output,const ServerNpcWakeTransitionBindings& b,ServerNpcWakeTransitionResult& o,unsigned depth) noexcept {
    unsigned n=0;
    if(!e||!b.extent||!b.extent(b.context,e,&n)||n<4)return false;
    const auto vt=rd<std::uint32_t>(e,0);
    if(outer_class(vt,n))return wake_transition_impl(e,n,vt,output,b,o,depth);
    // Known class with insufficient extent is invalid, not an external class.
    const auto* cls=SiN_GetServerNpcSubclassSchedulerApi(1,sizeof(ServerNpcSubclassSchedulerApi));
    unsigned family=0;if(!cls||cls->classify(vt,&family))return false;
    if(!b.external_wake||!b.external_wake(b.context,e,output))return false;
    ++o.external_woken;return true;
}
bool wake_transition_impl(void* a,unsigned n,std::uint32_t vt,bool output,const ServerNpcWakeTransitionBindings& b,ServerNpcWakeTransitionResult& o,unsigned depth) noexcept {
    if(!a||!outer_class(vt,n)||depth>=16)return false;
    if(!rd<int>(a,0x868))return true;
    const auto* cls=SiN_GetServerNpcSubclassSchedulerApi(1,sizeof(ServerNpcSubclassSchedulerApi));
    unsigned family=0;if(!cls||!cls->classify(vt,&family))return false;
    if(family==NpcSubclassJetpack){
        // 22366e60 calls 22366a20 before resolving the linked NPC.
        // The original preparation returns immediately if either thrust
        // handle is live. Keep the explicit creation/initialization service
        // for the unresolved path where neither handle resolves.
        if(b.entities.count>sin_independent::kEntityHandleCapacity||(b.entities.count&&!b.entities.entries))return false;
        const sin_independent::EntityHandleView registry(b.entities.entries,b.entities.count);
        if(b.jet_prepare){
            if(!b.jet_prepare(b.context,a))return false;
        }else if(!registry.Resolve(rd<unsigned>(a,0xfe4))&&!registry.Resolve(rd<unsigned>(a,0xfe8)))return false;
        ++o.jet_prepared;
        void* linked=registry.Resolve(rd<unsigned>(a,0x1024));
        if(!linked||linked==a||!wake_member(linked,true,b,o,depth+1))return false;
        ++o.jet_linked;
        const auto token=rd<unsigned>(a,0x218);
        if(!token||!b.jet_attachment||!b.jet_attachment(b.context,token))return false;++o.jet_attachment;
    }
    // 22040ea0 rechecks sleep after the Jetpack-specific prefix.
    if(!rd<int>(a,0x868))return true;
    wr<int>(a,0x868,0);++o.transitions;
    const auto flags=rd<unsigned>(a,0x78);
    if(flags&0x20u){
        if(!b.storage||!dirty(a,*b.storage))return false;
        wr<unsigned>(a,0x78,flags&~0x20u);++o.flags_changed;
    }
    if(!(b.refresh_entity?b.refresh_entity(b.context,a):wake_refresh_entity(a,b)))return false;++o.refreshed;
    if(output){
        // 22156900 wraps a void variant and 22156680 returns immediately
        // when the output action-list head (+14) is null.
        if(b.output){if(!b.output(b.context,a,0xbc8u))return false;}
        else if(rd<unsigned>(a,0xbdc))return false;
        ++o.outputs;
    }
    if(!rd<std::uint8_t>(a,0x870)||!rd<unsigned>(a,0x9c4))return true;
    int cursor=-1;
    for(unsigned i=0;i<17;++i){
        void* member=nullptr;const int prior=cursor;
        const auto token=rd<unsigned>(a,0x9c4);
        if(b.squad_next? !b.squad_next(b.context,token,&cursor,&member): !wake_squad_next(token,cursor,&member,b))return false;
        if(!member)return true;
        if(i==16||cursor<=prior||cursor<0||cursor>=16)return false;
        ++o.squad_visited;
        unsigned bytes=0;if(!b.extent||!b.extent(b.context,member,&bytes)||bytes<0x871)return false;
        if(member==a||rd<int>(member,0x8c)!=0)continue;
        wr<std::uint8_t>(member,0x870,0);
        if(!wake_member(member,true,b,o,depth+1))return false;
        ++o.squad_woken;
    }
    return false;
}
int __cdecl wake_transition(void* a,unsigned n,std::uint32_t vt,bool output,const ServerNpcWakeTransitionBindings* b,ServerNpcWakeTransitionResult* o) noexcept {
    if(!o)return 0;*o={};if(!b||!wake_transition_impl(a,n,vt,output,*b,*o,0))return 0;o->completed=1;return 1;
}
int __cdecl wake_policy(void* a,unsigned n,std::uint32_t vt,bool pvs,const ServerNpcWakePolicyBindings* b,ServerNpcWakePolicyResult* out) noexcept {
    return wake_policy_impl(a,n,vt,pvs,b,out,nullptr);
}
bool native_outer_wake(void* a,unsigned n,std::uint32_t vt,bool pvs,const ServerNpcActiveWeaponStepBindings& base,const OuterWakeRoute& route) noexcept {
    auto policy=route.bindings.policy;
    auto transition=route.bindings.transition;
    const auto& selection=base.schedule->scheduler.maintenance.selection;
    policy.conditions=selection.grunt.schedule.conditions;
    transition.entities=selection.state.entities;
    transition.storage=route.bindings.outer.post;
    const WakeTransitionRoute bound{transition,route.result.transition};
    if(!wake_policy_impl(a,n,vt,pvs,&policy,&route.result.policy,&bound))return false;
    route.result.transition.completed=1;
    return true;
}
int __cdecl outer_ground_wake(void* a,unsigned n,const ServerNpcRunAnimationInput* in,float interval,const ServerNpcActiveWeaponStepBindings* b,const ServerNpcOuterWakeUpdateBindings* outer,ServerNpcActiveWeaponStepResult* step,ServerNpcActivePostAnimationResult* tail,ServerNpcOuterWakeUpdateResult* out) noexcept {
    if(!out)return 0;*out={};if(!outer)return 0;const OuterWakeRoute route{*outer,*out};
    return outer_run(a,n,in,interval,b,b,&outer->outer,step,tail,&out->outer,false,&ground_post,&route);
}
int __cdecl outer_jet_wake(void* a,unsigned n,const ServerNpcRunAnimationInput* in,float interval,const ServerNpcActiveJetWeaponStepBindings* b,const ServerNpcOuterWakeUpdateBindings* outer,ServerNpcActiveJetWeaponStepResult* step,ServerNpcActivePostAnimationResult* tail,ServerNpcOuterWakeUpdateResult* out) noexcept {
    if(!out)return 0;*out={};if(!outer)return 0;const OuterWakeRoute route{*outer,*out};
    return outer_run(a,n,in,interval,b,b&&!b->base.think?&b->base:nullptr,&outer->outer,step,tail,&out->outer,true,&jet_post,&route);
}
const ServerNpcOuterWakeUpdateApi outer_wake_api{1,sizeof(ServerNpcOuterWakeUpdateApi),&outer_ground_wake,&outer_jet_wake};
const ServerNpcWakeTransitionApi wake_transition_api{1,sizeof(ServerNpcWakeTransitionApi),&wake_transition};
const ServerNpcWakePolicyApi wake_api{1,sizeof(ServerNpcWakePolicyApi),&wake_policy};
const ServerNpcOuterUpdateApi outer_api{1,sizeof(ServerNpcOuterUpdateApi),&outer_ground,&outer_jet};
const ServerNpcOuterCadenceApi cadence_api{1,sizeof(ServerNpcOuterCadenceApi),&cadence_set,&cadence_finish};
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

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcOuterCadenceApi=_SiN_GetServerNpcOuterCadenceApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcOuterCadenceApi* __cdecl SiN_GetServerNpcOuterCadenceApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcOuterCadenceApi)?&cadence_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcOuterUpdateApi=_SiN_GetServerNpcOuterUpdateApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcOuterUpdateApi* __cdecl SiN_GetServerNpcOuterUpdateApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcOuterUpdateApi)?&outer_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcWakePolicyApi=_SiN_GetServerNpcWakePolicyApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcWakePolicyApi* __cdecl SiN_GetServerNpcWakePolicyApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcWakePolicyApi)?&wake_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcWakeTransitionApi=_SiN_GetServerNpcWakeTransitionApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcWakeTransitionApi* __cdecl SiN_GetServerNpcWakeTransitionApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcWakeTransitionApi)?&wake_transition_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcOuterWakeUpdateApi=_SiN_GetServerNpcOuterWakeUpdateApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcOuterWakeUpdateApi* __cdecl SiN_GetServerNpcOuterWakeUpdateApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcOuterWakeUpdateApi)?&outer_wake_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerEntityVoidOutputApi=_SiN_GetServerEntityVoidOutputApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerEntityVoidOutputApi* __cdecl SiN_GetServerEntityVoidOutputApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerEntityVoidOutputApi)?&void_output_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerEntityNamedEventQueueApi=_SiN_GetServerEntityNamedEventQueueApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerEntityNamedEventQueueApi* __cdecl SiN_GetServerEntityNamedEventQueueApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerEntityNamedEventQueueApi)?&named_event_queue_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerEntityEventServiceApi=_SiN_GetServerEntityEventServiceApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerEntityEventServiceApi* __cdecl SiN_GetServerEntityEventServiceApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerEntityEventServiceApi)?&event_service_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerEntityAcceptInputApi=_SiN_GetServerEntityAcceptInputApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerEntityAcceptInputApi* __cdecl SiN_GetServerEntityAcceptInputApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerEntityAcceptInputApi)?&accept_input_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerEntitySearchApi=_SiN_GetServerEntitySearchApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerEntitySearchApi* __cdecl SiN_GetServerEntitySearchApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerEntitySearchApi)?&entity_search_api:nullptr;
}
