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
        // 22366e60 prepares attachments before resolving the linked NPC.
        if(!b.jet_prepare||!b.jet_prepare(b.context,a))return false;++o.jet_prepared;
        if(b.entities.count>sin_independent::kEntityHandleCapacity||(b.entities.count&&!b.entities.entries))return false;
        void* linked=sin_independent::EntityHandleView(b.entities.entries,b.entities.count).Resolve(rd<unsigned>(a,0x1024));
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
    if(output){if(!b.output||!b.output(b.context,a,0xbc8u))return false;++o.outputs;}
    if(!rd<std::uint8_t>(a,0x870)||!rd<unsigned>(a,0x9c4))return true;
    int cursor=-1;
    for(unsigned i=0;i<17;++i){
        void* member=nullptr;const int prior=cursor;
        if(!b.squad_next||!b.squad_next(b.context,rd<unsigned>(a,0x9c4),&cursor,&member))return false;
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
