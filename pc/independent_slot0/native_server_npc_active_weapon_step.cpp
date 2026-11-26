#include "native_server_npc_active_weapon_step.h"
#include "native_server_map_entity.h"
extern "C" const sin_port::native_map::Api* __cdecl SiN_GetServerMapEntityApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_resupply::Api* __cdecl SiN_GetServerDynamicResupplyApi(std::uint32_t,std::uint32_t) noexcept;
#include <cmath>
#include <cstring>
#include <limits>
#include <initializer_list>
#include <charconv>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponActivityApi* __cdecl SiN_GetServerWeaponActivityApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSubclassSchedulerApi* __cdecl SiN_GetServerNpcSubclassSchedulerApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeWeaponPolicyThinkApi* __cdecl SiN_GetServerNpcNativeWeaponPolicyThinkApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeJetWeaponPolicyThinkApi* __cdecl SiN_GetServerNpcNativeJetWeaponPolicyThinkApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScriptWeaponEventFrameApi* __cdecl SiN_GetServerNpcScriptWeaponEventFrameApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcEnemyStateApi* __cdecl SiN_GetServerNpcEnemyStateApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcEnemyMemoryMutationApi* __cdecl SiN_GetServerNpcEnemyMemoryMutationApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcEnemyMemoryTreeApi* __cdecl SiN_GetServerNpcEnemyMemoryTreeApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntMemoryReportApi* __cdecl SiN_GetServerNpcGruntMemoryReportApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntSquadLeaveApi* __cdecl SiN_GetServerNpcGruntSquadLeaveApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntLifeDispatchApi* __cdecl SiN_GetServerNpcGruntLifeDispatchApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntGroundLifeApi* __cdecl SiN_GetServerNpcGruntGroundLifeApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntEliteLifeApi* __cdecl SiN_GetServerNpcGruntEliteLifeApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcJetpackDeathThinkApi* __cdecl SiN_GetServerNpcJetpackDeathThinkApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntRagdollRouteApi* __cdecl SiN_GetServerNpcGruntRagdollRouteApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntSentenceRequestApi* __cdecl SiN_GetServerNpcGruntSentenceRequestApi(std::uint32_t,std::uint32_t) noexcept;
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
bool think_tick(float time,float interval,int& tick) noexcept {
    tick=-1;
    if(!std::isfinite(time)||!std::isfinite(interval)||interval<=0)return false;
    if(time!=-1.f){
        const double q=std::trunc(double(time)/double(interval)+.5);
        if(!std::isfinite(q)||q<double(std::numeric_limits<int>::min())||q>double(std::numeric_limits<int>::max()))return false;
        tick=static_cast<int>(q);
    }
    return true;
}
bool think_flags(void* a,bool scheduled,const ServerNpcOuterCadenceBindings& b,ServerNpcOuterCadenceResult& o) noexcept {
    const auto flags=rd<unsigned>(a,0xb8);
    auto changed=[&](unsigned value){
        if(!b.notify_think_flags)return false;
        wr<unsigned>(a,0xb8,value);o.flags_changed=o.notify_called=1;
        return b.notify_think_flags(b.context,a)!=0;
    };
    if(flags&0x400000u){
        if(scheduled&&!changed(flags&~0x400000u))return false;
    }else if(!scheduled){
        // 2214bf00 re-reads live default and named ticks, strictly positive.
        if(rd<int>(a,0x74)>0)return true;
        const int count=rd<int>(a,0x50);
        if(count>4096)return false;
        if(count>0){
            void* rows=nullptr;
            if(!b.storage||!resolve(*b.storage,rd<unsigned>(a,0x44),16u*static_cast<unsigned>(count),&rows))return false;
            for(int i=0;i<count;++i)if(rd<int>(rows,16u*static_cast<unsigned>(i)+8)>0)return true;
        }
        if(!changed(flags|0x400000u))return false;
    }
    return true;
}
bool set_tick(void* a,float time,float interval,const ServerNpcOuterCadenceBindings& b,ServerNpcOuterCadenceResult& o) noexcept {
    int tick=-1;if(!think_tick(time,interval,tick))return false;
    // All reviewed +c0 targets are 22001740 RET, no callback needed.
    if(rd<int>(a,0x74)!=tick){wr<int>(a,0x74,tick);o.tick_written=1;}
    o.tick=tick;o.next_time=time;
    return think_flags(a,tick!=-1,b,o);
}
int __cdecl cadence_set(void* a,unsigned n,float time,float interval,std::uint32_t vt,const ServerNpcOuterCadenceBindings* b,ServerNpcOuterCadenceResult* out) noexcept {
    if(!out)return 0;*out={};
    if(!a||!b||(!outer_class(vt,n)&&!(((vt==sin_port::native_map::auto_vtable&&n>=sin_port::native_map::auto_extent)||(vt==sin_port::native_map::timer_vtable&&n>=sin_port::native_map::timer_extent))&&rd<unsigned>(a,0)==vt))||!std::isfinite(time)||!std::isfinite(interval)||interval<=0)return 0;
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
bool native_outer_wake(void*,unsigned,std::uint32_t,bool,float,const ServerNpcActiveWeaponStepBindings&,const OuterWakeRoute&) noexcept;
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
            if(!native_outer_wake(a,n,vt,pvs,in->now,*base,*wake_route))return 0;
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
int fire_output_impl(void* output,unsigned n,void* activator,void* caller,unsigned caller_bytes,float extra,const void* variant,const ServerEntityVoidOutputBindings* b,ServerEntityVoidOutputResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!output||n<0x18||!b||(caller&&caller_bytes<0xa9)||!std::isfinite(extra)||(variant&&!b->enqueue_variant))return 0;
    unsigned payload[5]{};if(variant)std::memcpy(payload,variant,sizeof(payload));
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
        action.variant_type=action.parameter?2u:variant?payload[4]:0u;
        // Parameter override branch uses only the action delay.
        action.delay=action.parameter?delay:static_cast<float>(double(delay)+extra);
        if(!std::isfinite(action.delay))return 0;
        if(variant&&!action.parameter){
            if(!b->enqueue_variant(b->context,activator,caller,&action,payload,sizeof(payload)))return 0;
        }else if(!b->enqueue||!b->enqueue(b->context,activator,caller,&action))return 0;
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
int __cdecl fire_void_output(void* output,unsigned n,void* activator,void* caller,unsigned caller_bytes,float extra,const ServerEntityVoidOutputBindings* b,ServerEntityVoidOutputResult* o) noexcept {
    return fire_output_impl(output,n,activator,caller,caller_bytes,extra,nullptr,b,o);
}
int __cdecl fire_stored_output(void* output,unsigned n,void* activator,void* caller,unsigned caller_bytes,float extra,const void* variant,unsigned vn,const ServerEntityVoidOutputBindings* b,ServerEntityVoidOutputResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!variant||vn<0x14)return 0;
    return fire_output_impl(output,n,activator,caller,caller_bytes,extra,variant,b,o);
}
const ServerEntityVoidOutputApi void_output_api{1,sizeof(ServerEntityVoidOutputApi),&fire_void_output,&fire_stored_output};
// Named event queue records from 22155300, stable sorted link insertion from
// 22154600. The independent allocator supplies a unique zero-free token.
int enqueue_event(void* sentinel,unsigned n,std::uint32_t root,float now,std::uint32_t activator,std::uint32_t caller,const ServerEntityVoidOutputAction* action,unsigned direct,const void* variant,const ServerEntityNamedEventQueueBindings* b,ServerEntityNamedEventQueueResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!sentinel||n<0x38||!root||!action||!b||!b->allocate||!std::isfinite(now)||!std::isfinite(action->delay))return 0;
    if(!variant&&action->variant_type!=(action->parameter?2u:0u))return 0;
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
    wr<unsigned>(row,0x18,direct);
    if(variant)std::memcpy(static_cast<unsigned char*>(row)+0x1c,variant,0x14);
    else{
        wr<unsigned>(row,0x1c,action->parameter);
        // Unused named void/string variant payload words are deterministic.
        wr<unsigned>(row,0x20,0u);wr<unsigned>(row,0x24,0u);wr<unsigned>(row,0x28,0xffffffffu);wr<unsigned>(row,0x2c,action->variant_type);
    }
    wr<unsigned>(row,0x30,next);wr<unsigned>(row,0x34,prev_token);
    wr<unsigned>(previous,0x30,token);if(successor)wr<unsigned>(successor,0x34,token);
    o->inserted=o->completed=1;return 1;
}
int __cdecl enqueue_named_event(void* sentinel,unsigned n,std::uint32_t root,float now,std::uint32_t activator,std::uint32_t caller,const ServerEntityVoidOutputAction* action,const ServerEntityNamedEventQueueBindings* b,ServerEntityNamedEventQueueResult* o) noexcept {
    return enqueue_event(sentinel,n,root,now,activator,caller,action,0xffffffffu,nullptr,b,o);
}
int __cdecl enqueue_direct_event(void* sentinel,unsigned n,std::uint32_t root,float now,std::uint32_t target,std::uint32_t input,std::uint32_t activator,std::uint32_t caller,const void* variant,unsigned variant_bytes,float delay,std::uint32_t id,const ServerEntityNamedEventQueueBindings* b,ServerEntityNamedEventQueueResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!variant||variant_bytes<0x14)return 0;
    ServerEntityVoidOutputAction action{};action.input=input;action.delay=delay;action.action_id=id;
    // Original 221553e0 stores a direct handle and copies all five variant
    // words. The handle can become stale before the event is delivered.
    return enqueue_event(sentinel,n,root,now,activator,caller,&action,target,variant,b,o);
}
int __cdecl enqueue_named_variant(void* sentinel,unsigned n,std::uint32_t root,float now,std::uint32_t activator,std::uint32_t caller,const ServerEntityVoidOutputAction* action,const void* variant,unsigned vn,const ServerEntityNamedEventQueueBindings* b,ServerEntityNamedEventQueueResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!action||action->parameter||!variant||vn<0x14||rd<unsigned>(variant,0x10)!=action->variant_type)return 0;
    return enqueue_event(sentinel,n,root,now,activator,caller,action,0xffffffffu,variant,b,o);
}
const ServerEntityNamedEventQueueApi named_event_queue_api{1,sizeof(ServerEntityNamedEventQueueApi),&enqueue_named_event,&enqueue_direct_event,&enqueue_named_variant};
// 22155530: service each due node under an engine lock, dispatch all name
// matches, then a live direct target, else class fallback if none matched.
// Re-read links after dispatch: AcceptInput can insert another earlier event.
unsigned input_datamap(unsigned vt) noexcept;
int logic_kill_requested(unsigned,unsigned,const ServerEntityAcceptInputBindings&) noexcept;
int __cdecl search_named(void*,unsigned,void*,void*,const ServerEntitySearchBindings*,ServerEntitySearchResult*) noexcept;
int __cdecl search_class(void*,unsigned,const ServerEntitySearchBindings*,ServerEntitySearchResult*) noexcept;
int __cdecl accept_input(void*,unsigned,std::uint32_t,std::uint32_t,void*,void*,const void*,unsigned,std::uint32_t,const ServerEntityAcceptInputBindings*,ServerEntityAcceptInputResult*) noexcept;
int __cdecl service_events(void* sentinel,unsigned n,std::uint32_t root,float now,bool single_step,int* steps,const ServerEntityEventServiceBindings* b,ServerEntityEventServiceResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!sentinel||n<0x38||!root||!b||(!b->live_now&&!std::isfinite(now))||((single_step||b->live_single_step)&&!steps))return 0;
    if(b->entities.count>sin_independent::kEntityHandleCapacity||(b->entities.count&&!b->entities.entries))return 0;
    const sin_independent::EntityHandleView registry(b->entities.entries,b->entities.count);
    auto stepping=[&](){++o->step_reads;return b->live_single_step?*b->live_single_step!=0:single_step;};
    auto clock=[&](){++o->clock_reads;return b->live_now?*b->live_now:now;};
    if(stepping()&&*steps<=0){o->step_stopped=o->completed=1;return 1;}
    if(b->native_input&&!b->native_search)return 0;
    ServerEntitySearchBindings native_search{};
    if(b->native_search){native_search=*b->native_search;native_search.entities=b->entities;}
    auto find_named=[&](void* prev,unsigned name,void* activator,void* caller,void** entity){
        if(b->find_named)return b->find_named(b->context,prev,name,activator,caller,entity)!=0;
        if(!b->native_search)return false;
        ServerEntitySearchResult result{};++o->native_named_searches;
        if(!search_named(prev,name,activator,caller,&native_search,&result))return false;
        *entity=result.entity;return true;
    };
    auto find_class=[&](void* prev,unsigned name,void** entity){
        if(b->find_class)return b->find_class(b->context,prev,name,entity)!=0;
        if(!b->native_search)return false;
        ServerEntitySearchResult result{};++o->native_class_searches;
        if(!search_class(prev,name,&native_search,&result))return false;
        *entity=result.entity;return true;
    };
    unsigned dispatch_token=0;
    auto accept=[&](void* entity,unsigned input,void* activator,void* caller,const void* variant,unsigned id){
        if(b->native_input){
            unsigned bytes=0;
            if(!native_search.extent||!native_search.extent(native_search.context,entity,&bytes)||bytes<4)return false;
            const unsigned vt=rd<unsigned>(entity,0);
            const int shared_kill=logic_kill_requested(vt,input,*b->native_input);
            if(shared_kill<0)return false;
            if(shared_kill||input_datamap(vt)||(vt==sin_port::native_resupply::resupply_vtable&&b->native_input->resupply)||
               (vt==sin_port::native_map::relay_vtable&&b->native_input->relay)||
               (vt==sin_port::native_map::timer_vtable&&b->native_input->timer)||
               (vt==sin_port::native_map::branch_vtable&&b->native_input->branch)||
               (vt==sin_port::native_map::case_vtable&&b->native_input->logic_case)||
               (vt==sin_port::native_map::compare_vtable&&b->native_input->logic_compare)||
               (vt==sin_port::native_map::counter_vtable&&b->native_input->math_counter)||
               (vt==sin_port::native_map::navigation_vtable&&b->native_input->logic_navigation)||
               (vt==sin_port::native_map::autosave_vtable&&b->native_input->logic_autosave)||
               (vt==sin_port::native_map::multicompare_vtable&&b->native_input->logic_multicompare)||
               (vt==sin_port::native_map::remap_vtable&&b->native_input->math_remap)||
               (vt==sin_port::native_map::colorblend_vtable&&b->native_input->math_colorblend)){
                auto bound=*b->native_input;bound.entities=b->entities;bound.search=&native_search;bound.now=clock();if(!std::isfinite(bound.now))return false;
                sin_port::native_map::RelayBindings relay{};
                sin_port::native_map::RelayQueueBindings relay_queue{};
                if(vt==sin_port::native_map::relay_vtable&&bound.relay){
                    relay=*bound.relay;
                    if(relay.native_queue){
                        relay_queue=*relay.native_queue;
                        // Do not dispatch into a second queue or registry. The
                        // active node cannot be canceled before service unlinks it.
                        if(relay_queue.root!=sentinel||relay_queue.root_token!=root)return false;
                        relay_queue.root_bytes=n;relay_queue.entries=b->entities.entries;relay_queue.entry_count=b->entities.count;
                        relay_queue.now=b->live_now?b->live_now:&bound.now;
                        relay_queue.protected_node=&dispatch_token;relay.native_queue=&relay_queue;
                    }
                    bound.relay=&relay;
                }
                sin_port::native_map::MultiCompareBindings multi{};
                sin_port::native_map::RelayQueueBindings multi_queue{};
                if(vt==sin_port::native_map::multicompare_vtable&&bound.logic_multicompare){
                    multi=*bound.logic_multicompare;
                    if(multi.native_queue){
                        multi_queue=*multi.native_queue;
                        if(multi_queue.root!=sentinel||multi_queue.root_token!=root)return false;
                        multi_queue.root_bytes=n;multi_queue.entries=b->entities.entries;multi_queue.entry_count=b->entities.count;
                        multi_queue.now=b->live_now?b->live_now:&bound.now;
                        multi_queue.protected_node=&dispatch_token;multi.native_queue=&multi_queue;
                    }
                    bound.logic_multicompare=&multi;
                }
                sin_port::native_map::RemapBindings remap{};
                sin_port::native_map::RelayQueueBindings remap_queue{};
                if(vt==sin_port::native_map::remap_vtable&&bound.math_remap){
                    remap=*bound.math_remap;
                    if(remap.native_queue){
                        remap_queue=*remap.native_queue;
                        if(remap_queue.root!=sentinel||remap_queue.root_token!=root)return false;
                        remap_queue.root_bytes=n;remap_queue.entries=b->entities.entries;remap_queue.entry_count=b->entities.count;
                        remap_queue.now=b->live_now?b->live_now:&bound.now;
                        remap_queue.protected_node=&dispatch_token;remap.native_queue=&remap_queue;
                    }
                    bound.math_remap=&remap;
                }
                sin_port::native_map::ColorBlendBindings blend{};
                sin_port::native_map::RelayQueueBindings blend_queue{};
                if(vt==sin_port::native_map::colorblend_vtable&&bound.math_colorblend){
                    blend=*bound.math_colorblend;
                    if(blend.native_queue){
                        blend_queue=*blend.native_queue;
                        if(blend_queue.root!=sentinel||blend_queue.root_token!=root)return false;
                        blend_queue.root_bytes=n;blend_queue.entries=b->entities.entries;blend_queue.entry_count=b->entities.count;
                        blend_queue.now=b->live_now?b->live_now:&bound.now;
                        blend_queue.protected_node=&dispatch_token;blend.native_queue=&blend_queue;
                    }
                    bound.math_colorblend=&blend;
                }
                sin_port::native_map::CaseBindings cases{};
                sin_port::native_map::RelayQueueBindings case_queue{};
                if(vt==sin_port::native_map::case_vtable&&bound.logic_case){
                    cases=*bound.logic_case;
                    if(cases.native_queue){
                        case_queue=*cases.native_queue;
                        if(case_queue.root!=sentinel||case_queue.root_token!=root)return false;
                        case_queue.root_bytes=n;case_queue.entries=b->entities.entries;case_queue.entry_count=b->entities.count;
                        case_queue.now=b->live_now?b->live_now:&bound.now;
                        case_queue.protected_node=&dispatch_token;cases.native_queue=&case_queue;
                    }
                    bound.logic_case=&cases;
                }
                sin_port::native_map::CounterBindings counter{};
                sin_port::native_map::CompareBindings compare{};
                sin_port::native_map::RelayQueueBindings numeric_queue{};
                if(vt==sin_port::native_map::counter_vtable&&bound.math_counter){
                    counter=*bound.math_counter;
                    if(counter.native_queue){
                        numeric_queue=*counter.native_queue;
                        if(numeric_queue.root!=sentinel||numeric_queue.root_token!=root)return false;
                        numeric_queue.root_bytes=n;numeric_queue.entries=b->entities.entries;numeric_queue.entry_count=b->entities.count;
                        numeric_queue.now=b->live_now?b->live_now:&bound.now;
                        numeric_queue.protected_node=&dispatch_token;counter.native_queue=&numeric_queue;
                    }
                    bound.math_counter=&counter;
                }else if(vt==sin_port::native_map::compare_vtable&&bound.logic_compare){
                    compare=*bound.logic_compare;
                    if(compare.native_queue){
                        numeric_queue=*compare.native_queue;
                        if(numeric_queue.root!=sentinel||numeric_queue.root_token!=root)return false;
                        numeric_queue.root_bytes=n;numeric_queue.entries=b->entities.entries;numeric_queue.entry_count=b->entities.count;
                        numeric_queue.now=b->live_now?b->live_now:&bound.now;
                        numeric_queue.protected_node=&dispatch_token;compare.native_queue=&numeric_queue;
                    }
                    bound.logic_compare=&compare;
                }
                sin_port::native_map::BranchBindings branch{};
                sin_port::native_map::TimerBindings timer{};
                sin_port::native_map::RelayQueueBindings trigger_queue{};
                auto bind_trigger_queue=[&](const sin_port::native_map::RelayQueueBindings* route){
                    trigger_queue=*route;
                    if(trigger_queue.root!=sentinel||trigger_queue.root_token!=root)return false;
                    trigger_queue.root_bytes=n;trigger_queue.entries=b->entities.entries;trigger_queue.entry_count=b->entities.count;
                    trigger_queue.now=b->live_now?b->live_now:&bound.now;
                    trigger_queue.protected_node=&dispatch_token;return true;
                };
                if(vt==sin_port::native_map::branch_vtable&&bound.branch){
                    branch=*bound.branch;
                    if(branch.native_queue){
                        if(!bind_trigger_queue(branch.native_queue))return false;
                        branch.native_queue=&trigger_queue;
                    }
                    bound.branch=&branch;
                }
                if(vt==sin_port::native_map::timer_vtable&&bound.timer){
                    timer=*bound.timer;timer.now=b->live_now?b->live_now:&bound.now;
                    if(timer.cadence){timer.entries=b->entities.entries;timer.entry_count=b->entities.count;}
                    if(timer.native_queue){
                        if(!bind_trigger_queue(timer.native_queue))return false;
                        timer.native_queue=&trigger_queue;
                    }
                    bound.timer=&timer;
                }
                ServerEntityAcceptInputResult result{};
                if(!accept_input(entity,bytes,vt,input,activator,caller,variant,0x14,id,&bound,&result)||!result.completed)return false;
                // Retail ignores the virtual AcceptInput return. A rejected
                // input is a completed delivery, not a missing target.
                ++o->native_inputs;if(!result.accepted)++o->rejected_inputs;return true;
            }
        }
        if(!b->accept||!b->accept(b->context,entity,input,activator,caller,variant,id))return false;
        ++o->foreign_inputs;return true;
    };
    auto row_for=[&](unsigned token,void** row){
        unsigned bytes=0;*row=nullptr;
        return token&&token!=root&&b->resolve&&b->resolve(b->context,token,row,&bytes)&&*row&&*row!=sentinel&&bytes>=0x38;
    };
    for(unsigned processed=0;processed<512;++processed){
        const unsigned token=rd<unsigned>(sentinel,0x30);
        if(!token){o->completed=1;return 1;}
        void* row=nullptr;if(!row_for(token,&row)||rd<unsigned>(row,0x34)!=root)return 0;
        const float at=rd<float>(row,0);if(!std::isfinite(at))return 0;
        const float current=clock();if(!std::isfinite(current))return 0;
        if(at>current){o->future_stopped=o->completed=1;return 1;}
        if(!b->lock||!b->lock(b->context,true))return 0;++o->locks;
        dispatch_token=token;
        auto dispatch=[&](){
            const auto target=rd<unsigned>(row,4);
            bool found=false;
            auto deliver=[&](void* entity,unsigned& count){
                unsigned char variant[0x14];std::memcpy(variant,static_cast<unsigned char*>(row)+0x1c,sizeof(variant));
                if(!accept(entity,rd<unsigned>(row,8),registry.Resolve(rd<unsigned>(row,0xc)),registry.Resolve(rd<unsigned>(row,0x10)),variant,rd<unsigned>(row,0x14)))return false;
                ++count;++o->deliveries;found=true;return true;
            };
            if(target){
                void* prev=nullptr;
                for(unsigned i=0;;++i){
                    void* entity=nullptr;
                    if(!find_named(prev,target,registry.Resolve(rd<unsigned>(row,0xc)),registry.Resolve(rd<unsigned>(row,0x10)),&entity))return false;
                    if(!entity)break;
                    if(i==512||entity==prev||!deliver(entity,o->named))return false;prev=entity;
                }
            }
            if(void* direct=registry.Resolve(rd<unsigned>(row,0x18))){
                if(!deliver(direct,o->direct))return false;
            }else if(!found&&target){
                void* prev=nullptr;
                for(unsigned i=0;;++i){
                    void* entity=nullptr;if(!find_class(prev,target,&entity))return false;
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
        bool stop=false;
        if(success){
            ++o->events;
            // 2215599f reloads live mode after releasing the node. The
            // budget decision precedes engine unlock, not another read after it.
            if(stepping()){
                if(*steps>0){--*steps;++o->steps_consumed;}
                stop=*steps<=0;
            }
        }
        const bool unlocked=b->lock(b->context,false)!=0;
        if(!success||!unlocked)return 0;
        if(stop){o->step_stopped=o->completed=1;return 1;}
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
int logic_kill_requested(unsigned vt,unsigned input,const ServerEntityAcceptInputBindings& b) noexcept {
    if(!sin_port::native_map::kill_available(b.logic_kill)||!sin_port::native_map::logic_kill_extent(vt))return 0;
    bool same=false;
    if(b.name_equal){
        if(!input_name_equal(b,0x2240b624u,input,same))return -1;
    }else if(input==0x2240b624u)same=true;
    else{
        const char* actual=nullptr;if(!input_text(b,input,&actual))return -1;
        same=search_equal("Kill",actual);
    }
    return same?1:0;
}
bool search_pattern(const char* name,const char* pattern) noexcept {
    while(*name&&*pattern&&search_lower(static_cast<unsigned char>(*name))==search_lower(static_cast<unsigned char>(*pattern))){++name;++pattern;}
    return (!*name&&!*pattern)||*pattern=='*';
}
bool search_handle(void* entity,const ServerEntitySearchBindings& b,unsigned& handle) noexcept {
    unsigned bytes=0;
    if(!b.extent||!b.extent(b.context,entity,&bytes)||bytes<4)return false;
    if(input_datamap(rd<unsigned>(entity,0))||rd<unsigned>(entity,0)==sin_port::native_resupply::resupply_vtable){
        if(bytes<0x37c)return false;handle=rd<unsigned>(entity,0x378);return true;
    }
    return b.foreign_handle&&b.foreign_handle(b.context,entity,&handle)!=0;
}
int search_entities(void* previous,unsigned token,void* activator,void* caller,bool classname,const ServerEntitySearchBindings* b,ServerEntitySearchResult* o,const char* supplied_pattern=nullptr) noexcept {
    if(!o)return 0;*o={};
    if(!b||b->entities.count>sin_independent::kEntityHandleCapacity||(b->entities.count&&!b->entities.entries))return 0;
    const char* pattern=supplied_pattern;if(!pattern&&!input_text(*b,token,&pattern))return 0;
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
            if(!supplied_pattern&&own==token){o->entity=entity;return done();}
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
// 22155fad -> 223a5c8b -> 223b02f6. The original C-locale
// grammar is decimal prefix, including D/d exponent, not modern
// hex-float / NaN / Inf syntax. input_text bounds its NUL to 4096 bytes.
// Normalize only the accepted prefix; host from_chars rounds to double
// without depending on locale, then store the original float payload.
// Rare extreme decimal rounding differences from the old CRT remain.
bool input_decimal_float(const char* p,float& value) noexcept {
    while(*p==' '||(*p>='\t'&&*p<='\r'))++p;
    const bool neg=*p=='-';if(neg||*p=='+')++p;
    char decimal[4097]{};unsigned used=0,digits=0,before=0,first_nonzero=4096;
    if(neg)decimal[used++]='-';
    auto digit=[](char ch){return ch>='0'&&ch<='9';};
    auto append_digit=[&](){
        if(*p!='0'&&first_nonzero==4096)first_nonzero=digits;
        decimal[used++]=*p++;++digits;
    };
    while(digit(*p))append_digit();
    before=digits;
    if(*p=='.'){
        decimal[used++]=*p++;
        while(digit(*p))append_digit();
    }
    if(!digits){value=0.f;return true;}
    int exponent=0;
    if(*p=='e'||*p=='E'||*p=='d'||*p=='D'){
        const char* e=p+1;const bool minus=*e=='-';
        if(minus||*e=='+')++e;
        if(digit(*e)){
            decimal[used++]='e';if(minus)decimal[used++]='-';
            while(digit(*e)){
                if(exponent<10000){exponent=exponent*10+(*e-'0');if(exponent>10000)exponent=10000;}
                decimal[used++]=*e++;
            }
            if(minus)exponent=-exponent;
        }
    }
    if(first_nonzero==4096){value=neg?-0.f:0.f;return true;}
    double converted=0;
    const auto parsed=std::from_chars(decimal,decimal+used,converted,std::chars_format::general);
    if(parsed.ec==std::errc::result_out_of_range){
        const int magnitude=static_cast<int>(before)-static_cast<int>(first_nonzero)-1+exponent;
        value=std::copysign(magnitude>=0?std::numeric_limits<float>::infinity():0.f,neg?-1.f:1.f);
        return true;
    }
    if(parsed.ec!=std::errc{}||parsed.ptr!=decimal+used)return false;
    value=static_cast<float>(converted);return true;
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
    unsigned offset=0,flags=8;
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
constexpr NativeInputRow relay_inputs[]{
    {2,0,0x223edd00u,0x2217ded0u,"Enable"},
    {3,0,0x224228a8u,0x2217dee0u,"EnableRefire"},
    {4,0,0x223edce8u,0x2217df10u,"Disable"},
    {5,0,0x223d4914u,0x2217df20u,"Toggle"},
    {6,0,0x22422b7cu,0x2217df50u,"Trigger"},
    {7,0,0x22422b58u,0x2217def0u,"CancelPending"}
};
constexpr NativeInputRow colorblend_inputs[]{{0,1,0x2242014cu,0x2217a740u,"InValue"}};
constexpr NativeInputRow remap_inputs[]{{0,1,0x2242014cu,0x2217a610u,"InValue"}};
constexpr NativeInputRow multicompare_inputs[]{
    {5,20,0x2241ffb8u,0x2217a080u,"InputValue"},
    {6,20,0x2241ff14u,0x22179270u,"CompareValues"}
};
constexpr NativeInputRow autosave_inputs[]{{1,0,0x22420850u,0x22178dd0u,"Save"}};
constexpr NativeInputRow base_kill_inputs[]{{78,0,0x2240b624u,0x2213cdc0u,"Kill"}};
constexpr NativeInputRow navigation_inputs[]{
    {2,0,0x223d4940u,0x22178190u,"TurnOn"},
    {3,0,0x223d4928u,0x22178130u,"TurnOff"},
    {4,0,0x223d4914u,0x221781c0u,"Toggle"}
};
constexpr NativeInputRow counter_inputs[]{
    {4,1,0x22420330u,0x2217add0u,"Add"},
    {5,1,0x2242031cu,0x2217ae10u,"Divide"},
    {6,1,0x22420300u,0x2217aea0u,"Multiply"},
    {7,1,0x224202e4u,0x2217aee0u,"SetValue"},
    {8,1,0x224202c0u,0x22179a80u,"SetValueNoFire"},
    {9,1,0x224202a4u,0x2217af20u,"Subtract"},
    {10,1,0x22420288u,0x2217ad10u,"SetHitMax"},
    {11,1,0x2242026cu,0x2217ad70u,"SetHitMin"}
};
constexpr NativeInputRow compare_inputs[]{
    {2,1,0x224202e4u,0x22179b30u,"SetValue"},
    {3,1,0x22420704u,0x2217af50u,"SetValueCompare"},
    {4,1,0x224206dcu,0x22179b60u,"SetCompareValue"},
    {5,0,0x224206c4u,0x2217af80u,"Compare"}
};
constexpr NativeInputRow case_inputs[]{
    {16,20,0x2242014cu,0x2217ab00u,"InValue"},
    {17,0,0x224203b8u,0x22178cb0u,"PickRandom"}
};
constexpr NativeInputRow branch_inputs[]{
    {1,6,0x224202e4u,0,"SetValue",0x394,0xe},
    {2,6,0x2242080cu,0x22179ba0u,"SetValueTest"},
    {3,0,0x223d4914u,0x22178d70u,"Toggle"},
    {4,0,0x224207f0u,0x22179be0u,"ToggleTest"},
    {5,0,0x224207dcu,0x22179c20u,"Test"}
};
constexpr NativeInputRow timer_inputs[]{
    {3,1,0x22420090u,0x221793e0u,"RefireTime"},
    {4,0,0x22420054u,0x221793d0u,"FireTimer"},
    {5,0,0x223edd00u,0x22178b00u,"Enable"},
    {6,0,0x223edce8u,0x22178b20u,"Disable"},
    {7,0,0x223d4914u,0x22178b40u,"Toggle"},
    {8,5,0x22420030u,0,"UseRandomTime",0x3e8,0xe},
    {9,1,0x22420004u,0,"LowerRandomBound",0x3ec,0xe},
    {10,1,0x2241ffd8u,0,"UpperRandomBound",0x3f0,0xe}
};
constexpr NativeInputRow resupply_inputs[]{
    {1,0,0x2240b624u,0x222f8520u,"Kill"},
    {2,0,0x2246b484u,0x222f9120u,"CalculateType"},
    {3,0,0x2246b460u,0x222f8f00u,"BecomeMaster"}
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
bool copy_native_squad_memory(void* a,unsigned n,unsigned vt,void* leader,const ServerEntityAcceptInputBindings& b,ServerEntityAcceptInputResult& o) noexcept {
    const auto& s=*b.squad;
    unsigned extent=0;
    if(leader==a||!s.leader_extent||!s.leader_extent(s.context,leader,&extent)||extent<0x988||!input_datamap(rd<unsigned>(leader,0)))return false;
    auto report=*s.native_memory;report.original_vtable=vt;
    report.mutation.entries=b.entities.entries;report.mutation.count=b.entities.count;
    const auto& store=report.mutation.storage;
    auto resolve_memory=[&](unsigned token,unsigned need,void** p,unsigned* bytes){
        *p=nullptr;*bytes=0;
        return token&&store.storage&&store.storage(store.context,token,p,bytes)&&*p&&*bytes>=need;
    };
    void* memory=nullptr;unsigned bytes=0;
    if(!resolve_memory(rd<unsigned>(leader,0x984),0x40,&memory,&bytes))return false;
    const auto* tree=SiN_GetServerNpcEnemyMemoryTreeApi(1,sizeof(ServerNpcEnemyMemoryTreeApi));
    const auto* report_api=SiN_GetServerNpcGruntMemoryReportApi(1,sizeof(ServerNpcGruntMemoryReportApi));
    ServerNpcEnemyMemoryTreeResult validated{};
    if(!tree||!report_api||!tree->validate(memory,bytes,&store,&validated))return false;
    if(!validated.count)return true;
    void* nodes=nullptr;unsigned node_bytes=0;
    if(!resolve_memory(rd<unsigned>(memory,4),12,&nodes,&node_bytes))return false;
    // Match 2206b7b0/2206b130: in-order indexed tree, skip records
    // whose EHANDLE no longer resolves. Do not copy raw memory records.
    unsigned order[255]{},stack[255]{},depth=0,count=0,i=rd<std::uint8_t>(memory,0x10);
    bool seen[255]{};
    auto node=[&](unsigned idx){return static_cast<unsigned char*>(nodes)+12*idx;};
    while(i!=255||depth){
        while(i!=255){
            if(i>=255||seen[i]||node_bytes<12*(i+1)||depth>=255)return false;
            seen[i]=true;stack[depth++]=i;i=rd<std::uint8_t>(node(i),0);
        }
        i=stack[--depth];order[count++]=i;i=rd<std::uint8_t>(node(i),1);
    }
    if(count!=validated.count)return false;
    const sin_independent::EntityHandleView registry(b.entities.entries,b.entities.count);
    for(unsigned k=0;k<count;++k){
        void* record=nullptr;unsigned record_bytes=0;
        if(!resolve_memory(rd<unsigned>(node(order[k]),8),16,&record,&record_bytes))return false;
        ++o.squad_memory_visited;
        void* target=registry.Resolve(rd<unsigned>(record,0));
        if(!target){++o.squad_memory_stale;continue;}
        float position[3];std::memcpy(position,static_cast<unsigned char*>(record)+4,12);
        ServerNpcGruntMemoryReportResult result{};
        if(!report_api->report(a,n,target,position,leader,b.now,&report,&result))return false;
        ++o.squad_memory_reported;o.squad_memory_created+=result.mutation.created;o.squad_memory_updated+=result.mutation.updated;
    }
    return true;
}
int native_set_squad(void* a,unsigned n,unsigned vt,const ServerEntityInputData& d,const ServerEntityAcceptInputBindings& b,ServerEntityAcceptInputResult& o) noexcept {
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
        if(s.native_leave){
            // Both entry points must resolve the same group, not parallel
            // shadow copies. The input owner supplies registry/class identity.
            auto leave=*s.native_leave;
            leave.sentence.entries=b.entities.entries;leave.sentence.count=b.entities.count;
            auto& speech=leave.sentence.speech;speech.original_vtable=vt;
            void* check=nullptr;unsigned bytes=0;
            if(!speech.squad_storage||!speech.squad_storage(speech.context,group_token,&check,&bytes)||check!=group||bytes<0x58)return false;
            const auto* api=SiN_GetServerNpcGruntSquadLeaveApi(1,sizeof(ServerNpcGruntSquadLeaveApi));
            if(!api||!api->leave(a,n,group_token,handle,notify,b.now,&leave,&o.squad_leave))return false;
            o.squad_removed+=o.squad_leave.removed;o.squad_notices+=o.squad_leave.notifications;
            o.squad_native_notices+=o.squad_leave.native_notifications;o.squad_external_notices+=o.squad_leave.external_notifications;
            o.squad_stale_notices+=o.squad_leave.stale;
            if(o.squad_leave.missing)return warning(0x223ee75cu);
            return true;
        }
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
        if(!leader)return 0;
        if(s.native_memory){if(!copy_native_squad_memory(a,n,vt,leader,b,o))return 0;}
        else if(!s.copy_leader_memory||!s.copy_leader_memory(s.context,a,leader))return 0;
        ++o.squad_memory;
    }
    return 1;
}
// InputBeginRappel/InputPauseRappel tail through the shared Grunt +6f0/+6f4
// thunks to the embedded +cfc behavior. Map the private condition ID through
// its registered class descriptor, then use the original actor condition bank.
int native_rappel_input(void* a,unsigned n,bool begin,const ServerEntityAcceptInputBindings& b,ServerEntityAcceptInputResult& o) noexcept {
    if(!b.rappel)return -1;
    if(n<0xd20||rd<unsigned>(a,0xcfc)!=0x223f956cu||rd<unsigned>(a,0xd00)!=static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(a)))return 0;
    auto* component=static_cast<unsigned char*>(a)+0xcfc;
    struct RangeContext {const ServerNpcActivePostAnimationBindings* storage;} range{b.storage};
    ServerNpcConditionMapBindings map{};
    map.context=&range;
    map.descriptor=[](void* p,void*,const void** data,unsigned* bytes) noexcept -> int{
        const auto& q=*static_cast<RangeContext*>(p);
        void* row=nullptr;*bytes=0;
        if(!q.storage||!q.storage->resolve||!q.storage->resolve(q.storage->context,0x2254c648u,&row,bytes)||!row)return 0;
        *data=row;return 1;
    };
    map.storage=[](void* p,unsigned token,const void** data,unsigned* bytes) noexcept -> int{
        const auto& q=*static_cast<RangeContext*>(p);
        void* row=nullptr;*bytes=0;
        if(!q.storage||!q.storage->resolve||!q.storage->resolve(q.storage->context,token,&row,bytes)||!row)return 0;
        *data=row;return 1;
    };
    ServerNpcConditionResult mapped{};
    if(!resolve_npc_condition_range(component,begin?100000:100001,&map,&mapped))return 0;
    o.rappel_mapped=mapped.mapped;
    const auto* condition=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    ServerNpcConditionResult stored{};
    if(!condition||!condition->store(a,n,mapped.global,NpcConditionCurrent,true,b.rappel->actor_conditions,&stored))return 0;
    o.rappel_condition=stored.mapped;
    if(!begin)return 1;
    // 220ddc70: set condition first, optional transform detach, then snapshot
    // the owner's current three words (not a pre-detach position).
    if(rd<unsigned>(a,0xb8)&0x800u){
        if(!b.rappel->detach||!b.rappel->detach(b.rappel->context,a))return 0;
        ++o.rappel_detached;
    }
    std::memcpy(component+0x18,static_cast<unsigned char*>(a)+0x2bc,12);++o.rappel_snapshot;
    return 1;
}
// Original 22030a00: space-only tokenization, triples of target,
// disposition, priority. Missing priority is INT_MIN, not zero for updates.
int native_relationship(void* a,unsigned n,const ServerEntityInputData& d,const ServerEntityAcceptInputBindings& b,ServerEntityAcceptInputResult& o) noexcept {
    if(!b.relationship||!b.search)return -1;
    if(n<0x64c||b.entities.count>sin_independent::kEntityHandleCapacity||(b.entities.count&&!b.entities.entries))return 0;
    const auto& services=*b.relationship;
    const unsigned type=rd<unsigned>(d.variant,0x10);
    if(type!=0&&type!=2)return -1;
    const char* text=nullptr;if(!input_text(b,type==2?rd<unsigned>(d.variant,0):0u,&text))return 0;
    char buffer[1000]{};
    const auto len=std::strlen(text);std::memcpy(buffer,text,len<999?len:999);
    char* cursor=buffer;
    auto next=[&]()->char*{
        while(*cursor==' ')++cursor;
        if(!*cursor)return nullptr;
        char* start=cursor;while(*cursor&&*cursor!=' ')++cursor;
        if(*cursor)*cursor++=0;return start;
    };
    auto warn=[&](unsigned token,const char* detail){
        ++o.relationship_warnings;return !services.warning||services.warning(services.context,a,token,detail);
    };
    auto search=*b.search;search.entities=b.entities;
    const sin_independent::EntityHandleView registry(b.entities.entries,b.entities.count);
    auto edit=[&](void* entity,unsigned cls,unsigned disposition,unsigned priority){
        const int count=rd<int>(a,0x644);int capacity=rd<int>(a,0x63c);
        if(count<0||capacity<count||capacity>4096)return false;
        void* rows=nullptr;
        if(count&&!resolve(*b.storage,rd<unsigned>(a,0x638),16u*static_cast<unsigned>(count),&rows))return false;
        for(int i=count-1;i>=0;--i){
            auto* row=static_cast<unsigned char*>(rows)+16*i;
            if(entity?registry.Resolve(rd<unsigned>(row,0))==entity:rd<unsigned>(row,4)==cls){
                wr<unsigned>(row,8,disposition);
                if(priority!=0x80000000u)wr<unsigned>(row,12,priority);
                ++o.relationship_updated;return true;
            }
        }
        unsigned handle=0xffffffffu;if(entity&&!search_handle(entity,search,handle))return false;
        if(count==capacity){
            if(!services.grow||!services.grow(services.context,a,static_cast<unsigned>(count+1)))return false;
            capacity=rd<int>(a,0x63c);
            if(capacity<=count||capacity>4096||rd<int>(a,0x644)!=count)return false;
        }
        const unsigned token=rd<unsigned>(a,0x638);
        if(!resolve(*b.storage,token,16u*static_cast<unsigned>(count+1),&rows))return false;
        auto* row=static_cast<unsigned char*>(rows)+16*count;
        wr<unsigned>(row,0,handle);wr<unsigned>(row,4,entity?0u:cls);
        wr<unsigned>(row,8,disposition);wr<unsigned>(row,12,priority==0x80000000u?0u:priority);
        wr<int>(a,0x644,count+1);wr<unsigned>(a,0x648,token);
        ++o.relationship_added;return true;
    };
    for(char* target=next();target;target=next()){
        const char* disposition=next();if(!disposition)return warn(0x223d98a8u,text);
        unsigned kind=0;
        if(search_equal(disposition,"D_HT"))kind=1;
        else if(search_equal(disposition,"D_FR"))kind=2;
        else if(search_equal(disposition,"D_LI"))kind=3;
        else if(search_equal(disposition,"D_NU"))kind=4;
        else return warn(0x223d98d0u,disposition);
        const char* p=next();const unsigned priority=p?input_decimal(p):0x80000000u;
        ++o.relationship_specs;
        void* previous=nullptr;bool found=false;
        for(unsigned i=0;;++i){
            ServerEntitySearchResult result{};
            if(!search_entities(previous,0,nullptr,nullptr,false,&search,&result,target))return 0;
            if(!result.entity)break;
            if(i==4096||result.entity==previous||!edit(result.entity,0,kind,priority))return 0;
            ++o.relationship_entities;found=true;previous=result.entity;
        }
        if(found)continue;
        unsigned cls=1;bool known=search_equal(target,"player")||search_equal(target,"!player");
        if(!known){
            if(!services.classify||!services.classify(services.context,target,&cls,&known))return 0;
        }
        if(known){if(!edit(nullptr,cls,kind,priority))return 0;++o.relationship_classes;}
        else if(!warn(0x223d9910u,target))return 0;
    }
    return 1;
}
// 2203f1f0: search once, clear the current enemy if equal, then
// remove the exact pointer-keyed memory record via the native tree API.
int native_forget(void* a,unsigned n,unsigned vt,const ServerEntityInputData& d,const ServerEntityAcceptInputBindings& b,ServerEntityAcceptInputResult& o) noexcept {
    if(!b.forget||!b.search)return -1;
    if(n<0xc04||!std::isfinite(b.now)||b.entities.count>sin_independent::kEntityHandleCapacity||(b.entities.count&&!b.entities.entries))return 0;
    const unsigned type=rd<unsigned>(d.variant,0x10);
    if(type!=0&&type!=2)return -1;
    const auto& f=*b.forget;
    const unsigned token=type==2?rd<unsigned>(d.variant,0):0u;
    const char* text=nullptr;if(!input_text(b,token,&text))return 0;
    const auto length=std::strlen(text);
    if(f.developer&&length&&text[length-1]=='*'){
        if(f.warning&&!f.warning(f.context,0x223da448u))return 0;++o.forget_warning;
    }
    auto search=*b.search;search.entities=b.entities;ServerEntitySearchResult target{};
    if(!search_named(nullptr,token,nullptr,nullptr,&search,&target))return 0;
    if(!target.entity)return 1;
    ++o.forget_found;
    const sin_independent::EntityHandleView registry(b.entities.entries,b.entities.count);
    if(registry.Resolve(rd<unsigned>(a,0x8a0))==target.entity){
        const auto* api=SiN_GetServerNpcEnemyStateApi(1,sizeof(ServerNpcEnemyStateApi));if(!api)return 0;
        struct EnemyContext {const ServerNpcEnemyStateApi* api;const ServerNpcEnemyStateBindings* original;unsigned bytes;} ctx{api,&f.enemy,n};
        auto enemy=f.enemy;enemy.entries=b.entities.entries;enemy.count=b.entities.count;enemy.context=&ctx;
        enemy.enemy_changing=[](void* p,void* a) noexcept -> int{
            auto& q=*static_cast<EnemyContext*>(p);ServerNpcEnemyConditionResult result{};
            return q.api->clear_combat_conditions(a,q.bytes,&q.original->conditions,&result);
        };
        enemy.release_squad_slot=[](void* p,unsigned group,void* enemy,int slot) noexcept -> int{
            const auto& q=*static_cast<EnemyContext*>(p)->original;
            return q.release_squad_slot&&q.release_squad_slot(q.context,group,enemy,slot);
        };
        ServerNpcEnemyStateResult result{};
        if(!api->set(a,n,nullptr,true,b.now,&enemy,&result))return 0;
        if(rd<int>(a,0x858)!=2)wr<int>(a,0x858,2);
        ++o.forget_cleared;
    }
    auto mem=f.memory;mem.entries=b.entities.entries;mem.count=b.entities.count;
    void* owner=nullptr;unsigned bytes=0,key=0;
    if(!mem.entity_key||!mem.entity_key(mem.context,target.entity,&key)||!key||
       !mem.storage.storage||!mem.storage.storage(mem.storage.context,rd<unsigned>(a,0x984),&owner,&bytes)||!owner||bytes<0x40)return 0;
    const auto* mutation=SiN_GetServerNpcEnemyMemoryMutationApi(1,sizeof(ServerNpcEnemyMemoryMutationApi));
    ServerNpcEnemyMemoryMutationResult result{};
    if(!mutation||!mutation->remove(owner,bytes,key,&mem,&result))return 0;
    o.forget_removed=result.removed;return 1;
}
// 22357e50 -> 220cfc00, 220ce430, 220ce960. Selection uses
// priority first, distance second, and stops on the first non-rally cast.
int native_assault(void* a,unsigned n,const ServerEntityInputData& d,const ServerEntityAcceptInputBindings& b,ServerEntityAcceptInputResult& o) noexcept {
    if(!b.assault||!b.search)return -1;
    if(n<0xde4||rd<unsigned>(a,0xdb8)!=0x223f5544u||rd<unsigned>(a,0xdbc)!=static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(a))||
       b.entities.count>sin_independent::kEntityHandleCapacity||(b.entities.count&&!b.entities.entries))return 0;
    const auto& svc=*b.assault;
    const unsigned type=rd<unsigned>(d.variant,0x10);
    if(type!=0&&type!=2)return -1;
    const unsigned name=type==2?rd<unsigned>(d.variant,0):0u;
    auto search=*b.search;search.entities=b.entities;
    const sin_independent::EntityHandleView registry(b.entities.entries,b.entities.count);
    auto warn=[&](unsigned token){return !svc.warning||svc.warning(svc.context,a,token);};
    auto extent=[&](void* entity,unsigned need){
        unsigned bytes=0;return search.extent&&search.extent(search.context,entity,&bytes)&&bytes>=need;
    };
    auto position=[&](void* entity,float* p){
        if(!extent(entity,0x2c8))return false;
        if(rd<unsigned>(entity,0xb8)&0x800u){if(!svc.detach||!svc.detach(svc.context,entity))return false;}
        std::memcpy(p,static_cast<unsigned char*>(entity)+0x2bc,12);
        return std::isfinite(p[0])&&std::isfinite(p[1])&&std::isfinite(p[2]);
    };
    auto dist=[](const float* a,const float* b){
        const float x=a[0]-b[0],y=a[1]-b[1],z=a[2]-b[2];
        return double(x)*x+double(y)*y+double(z)*z;
    };
    auto reset=[&](){
        // Grunt never has the npc_monk exception in 220ce960.
        for(unsigned off:{0x7a8u,0x7acu,0x7a4u,0x79cu,0x798u,0x7a0u,0x7b4u})wr<unsigned>(a,off,0u);
        wr<std::uint8_t>(a,0x7b8,0);wr<std::uint8_t>(a,0x7b9,0);
        for(unsigned i=0;i<8;++i)wr<unsigned>(a,0x82c+4*i,0xffffffffu);
        ++o.assault_reset;
    };
    void* best=nullptr;void* prev=nullptr;int priority=-1;
    for(unsigned i=0;;++i){
        ServerEntitySearchResult found{};
        if(!search_named(prev,name,nullptr,nullptr,&search,&found))return 0;
        if(!found.entity)break;
        if(i==4096||found.entity==prev||!svc.point_type)return 0;
        bool rally=false;if(!svc.point_type(svc.context,found.entity,true,&rally))return 0;
        if(!rally)break;
        prev=found.entity;
        if(!extent(prev,0x3c0))return 0;
        ++o.assault_candidates;
        if(registry.Resolve(rd<unsigned>(prev,0x3bc)))continue;
        const int p=rd<int>(prev,0x3a0);
        if(p>priority){best=prev;priority=p;}
        else if(p==priority){
            if(!best)return 0; // original assumes a valid prior best
            float owner[3],candidate[3],old[3];
            if(!position(a,owner)||!position(prev,candidate)||!position(best,old))return 0;
            const float candidate_distance=static_cast<float>(dist(candidate,owner));
            if(dist(old,owner)>candidate_distance)best=prev;
        }
    }
    if(!best){++o.assault_missing;return warn(0x223f3cd0u);}
    const unsigned self=rd<unsigned>(a,0x378);unsigned selected=0;
    if(registry.Resolve(self)!=a||!search_handle(best,search,selected))return 0;
    // 220ce430 returns false if ownership became live; retail caller
    // ignores that return and still stores the selected handle.
    if(!registry.Resolve(rd<unsigned>(best,0x3bc))){wr<unsigned>(best,0x3bc,self);++o.assault_reserved;}
    wr<unsigned>(a,0xdd0,selected);++o.assault_selected;
    if(!registry.Resolve(selected)){
        if(!warn(0x223f3ca0u))return 0;wr<unsigned>(a,0xdd4,0u);++o.assault_missing;reset();return 1;
    }
    wr<unsigned>(a,0xdd4,3u);
    // 220ce960 resets behavior-local fields before resolving assault point.
    wr<std::uint8_t>(a,0xddc,0);wr<std::uint8_t>(a,0xddd,0);
    wr<unsigned>(a,0xdcc,0xffffffffu);wr<std::uint8_t>(a,0xdde,0);
    wr<unsigned>(a,0xde0,0u);wr<unsigned>(a,0xdd8,0u);
    ServerEntitySearchResult linked{};
    if(!search_named(nullptr,rd<unsigned>(best,0x394),nullptr,nullptr,&search,&linked))return 0;
    bool assault=false;if(linked.entity&&(!svc.point_type||!svc.point_type(svc.context,linked.entity,false,&assault)))return 0;
    if(assault){
        unsigned handle=0;if(!search_handle(linked.entity,search,handle))return 0;
        wr<unsigned>(a,0xdcc,handle);++o.assault_linked;
    }else{
        if(!warn(0x223f3bc0u))return 0;wr<unsigned>(a,0xdd4,0u);++o.assault_missing;
    }
    reset();return 1;
}
// 220424a0 extracts activator and tail-dispatches 220422d0.
// Shared Grunt +1ac is RET; +18 reads model token at +38.
// Engine physics/collision and break-model generation are not rebuilt.
int native_break_input(void* a,unsigned n,const ServerEntityInputData& d,const ServerEntityAcceptInputBindings& b,ServerEntityAcceptInputResult& o) noexcept {
    if(!b.break_input)return -1;
    if(n<0x630)return 0;
    const auto& svc=*b.break_input;
    if(!svc.non_solid||!svc.effect||!svc.defer_remove)return 0;
    const unsigned physics=rd<unsigned>(a,0x218);
    if(physics&&!svc.physics_snapshot)return 0;
    auto refresh=[&](unsigned mask,unsigned original){
        return !(rd<unsigned>(a,0xb8)&mask)||(svc.refresh_transform&&svc.refresh_transform(svc.context,a,original));
    };
    if(rd<int>(a,0x94)){wr<int>(a,0x94,0);++o.break_damage_disabled;}
    if(!svc.non_solid(svc.context,a,rd<std::uint16_t>(a,0x120)|4u))return 0;
    ++o.break_non_solid;
    ServerNpcBreakEffectRequest req{};
    // The input wrapper passes activator, but 220422d0 does not use it.
    (void)d;req.actor=a;req.physics=physics;
    if(physics){
        if(!svc.physics_snapshot(svc.context,physics,req.velocity,req.angular_velocity,req.origin,req.angles))return 0;
        ++o.break_physics_snapshot;
    }else{
        if(!refresh(0x1000u,0x2213d750u))return 0;
        std::memcpy(req.velocity,static_cast<unsigned char*>(a)+0x250,12);
        std::memcpy(req.angular_velocity,static_cast<unsigned char*>(a)+0x25c,12);
        if(!refresh(0x800u,0x2213fa60u))return 0;
        std::memcpy(req.origin,static_cast<unsigned char*>(a)+0x2bc,12);
        if(!refresh(0x800u,0x2213fa60u))return 0;
        std::memcpy(req.angles,static_cast<unsigned char*>(a)+0x354,12);
    }
    // The original request points at the actor's current origin/angles
    // even on the physics branch, after these two final refresh checks.
    if(!refresh(0x800u,0x2213fa60u)||!refresh(0x800u,0x2213fa60u))return 0;
    std::memcpy(req.origin,static_cast<unsigned char*>(a)+0x2bc,12);
    std::memcpy(req.angles,static_cast<unsigned char*>(a)+0x354,12);
    req.model=rd<unsigned>(a,0x38);req.skin=rd<unsigned>(a,0x62c);
    req.collision_group=rd<unsigned>(a,0x214);if(!req.collision_group)req.collision_group=4;
    req.scale=100.f;req.limit=-1;req.ignore_limit=false;
    if(!svc.effect(svc.context,&req))return 0;++o.break_effect;
    if(!svc.defer_remove(svc.context,a))return 0;++o.break_removed;
    return 1;
}
// 22366a20: create both trails before configuring either. The retail
// routine returns normally on a null/stale member of the resulting pair.
bool fresh_jet_thrust(void* actor,unsigned n,const ServerWeaponActivityBindings& entities,const ServerNpcActivePostAnimationBindings* storage,const ServerNpcJetThrustBindings& b,ServerNpcJetThrustResult& o) noexcept {
    if(n<0x102c||!storage||entities.count>sin_independent::kEntityHandleCapacity||(entities.count&&!entities.entries))return false;
    const sin_independent::EntityHandleView registry(entities.entries,entities.count);
    if(registry.Resolve(rd<unsigned>(actor,0xfe4))||registry.Resolve(rd<unsigned>(actor,0xfe8))){++o.reused;return true;}
    if(!b.create)return false;
    for(unsigned off:{0xfe4u,0xfe8u}){
        unsigned handle=0xffffffffu;
        if(!b.create(b.context,0x223d3248u,&handle))return false;
        wr<unsigned>(actor,off,handle);
        if(handle!=0xffffffffu)++o.created;
    }
    if(!registry.Resolve(rd<unsigned>(actor,0xfe4))||!registry.Resolve(rd<unsigned>(actor,0xfe8))){++o.partial;return true;}
    if(!b.extent||!b.lifetime||!b.attach_parent||!b.attachment_index)return false;
    constexpr unsigned offsets[]{0x3b0,0x3b4,0x3c4,0x3c8,0x3cc,0x3bc,0x3c0,0x3dc,0x394,0x398,0x39c,0x3a0,0x3a4,0x3a8};
    constexpr float values[]{10.f,1.f,8.f,20.f,10.f,15.f,25.f,.5f,.25f,.25f,.25f,0.f,0.f,0.f};
    for(unsigned side=0;side<2;++side){
        auto trail=[&](){return static_cast<void*>(registry.Resolve(rd<unsigned>(actor,0xfe4+4*side)));};
        auto extent=[&](void* e){unsigned bytes=0;return e&&b.extent(b.context,e,&bytes)&&bytes>=0x3e0;};
        for(unsigned i=0;i<14;++i){
            void* e=trail();if(!extent(e))return false;
            // x87 unordered/unequal writes, equality (including +/-0) skips.
            if(rd<float>(e,offsets[i])!=values[i]){
                if(!dirty(e,*storage))return false;
                wr<float>(e,offsets[i],values[i]);++o.writes;
            }
        }
        void* e=trail();if(!extent(e)||!b.lifetime(b.context,e,500.f))return false;++o.lifetimes;
        e=trail();if(!extent(e)||!b.attach_parent(b.context,e,actor,0x1028u))return false;++o.parents;
        unsigned index=0;
        if(!b.attachment_index(b.context,actor,0x1028u,side?0x22498830u:0x22498838u,&index))return false;
        e=trail();if(!extent(e))return false;
        if(rd<unsigned>(e,0x3d4)!=index){if(!dirty(e,*storage))return false;wr<unsigned>(e,0x3d4,index);++o.writes;}
        ++o.indices;++o.configured;
    }
    return true;
}
// 22140d20 (outgoing, flag clear) and 22140d90 (incoming, flag set).
// Each +c4 vector row is 12 bytes, with modifier token +0 and next +8.
bool self_damage_scale(void* a,const ServerNpcActivePostAnimationBindings& storage,bool incoming,float& factor,unsigned& visited) noexcept {
    factor=1.f;
    unsigned index=rd<unsigned>(a,0xd0);bool seen[4096]{};
    while(index!=0xffffffffu){
        if(index>=4096||seen[index])return false;seen[index]=true;++visited;
        void* rows=nullptr;if(!resolve(storage,rd<unsigned>(a,0xc4),12u*(index+1),&rows))return false;
        const auto* row=static_cast<const unsigned char*>(rows)+12u*index;
        void* modifier=nullptr;if(!resolve(storage,rd<unsigned>(row,0),9,&modifier))return false;
        if((rd<std::uint8_t>(modifier,8)!=0)==incoming){
            factor=static_cast<float>(double(factor)*rd<float>(modifier,0));
            if(!std::isfinite(factor))return false;
        }
        index=rd<unsigned>(row,8);
    }
    return true;
}
// Shared native 22156900 -> 22156680 -> 22155300/22154600 path.
// Actor identity is revalidated for every emission, including recursive
// wake events from different squad members.
bool fire_actor_output_queue(void* actor,unsigned extent,unsigned off,float now,const ServerWeaponActivityBindings& entities,const ServerEntityNativeOutputQueueBindings& queue,ServerEntityVoidOutputResult& result) noexcept {
    if(!actor||extent<0x37c||off>extent||extent-off<0x18||!queue.actions||!queue.queue||!std::isfinite(now)||
       entities.count>sin_independent::kEntityHandleCapacity||(entities.count&&!entities.entries))return false;
    const unsigned handle=rd<unsigned>(actor,0x378);
    if(rd<unsigned>(actor,off+0x14)&&sin_independent::EntityHandleView(entities.entries,entities.count).Resolve(handle)!=actor)return false;
    struct Context {void* actor;unsigned handle;float now;const ServerEntityNativeOutputQueueBindings* queue;} ctx{actor,handle,now,&queue};
    ServerEntityVoidOutputBindings actions{};actions.context=&ctx;
    actions.resolve=[](void* p,unsigned token,void** data,unsigned* bytes) noexcept -> int{
        const auto& src=*static_cast<Context*>(p)->queue->actions;
        return src.resolve&&src.resolve(src.context,token,data,bytes);
    };
    actions.release=[](void* p,unsigned token,void* data) noexcept -> int{
        const auto& src=*static_cast<Context*>(p)->queue->actions;
        return src.release&&src.release(src.context,token,data);
    };
    actions.debug_action=[](void* p,void* caller,const ServerEntityVoidOutputAction* action) noexcept -> int{
        const auto& src=*static_cast<Context*>(p)->queue->actions;
        return src.debug_action&&src.debug_action(src.context,caller,action);
    };
    actions.enqueue=[](void* p,void* activator,void* caller,const ServerEntityVoidOutputAction* action) noexcept -> int{
        const auto& q=*static_cast<Context*>(p);const auto& queue=*q.queue;
        if(activator!=q.actor||caller!=q.actor)return 0;
        ServerEntityNamedEventQueueResult result{};
        return enqueue_named_event(queue.sentinel,queue.sentinel_bytes,queue.sentinel_token,q.now,q.handle,q.handle,action,queue.queue,&result);
    };
    return fire_void_output(static_cast<unsigned char*>(actor)+off,extent-off,actor,actor,extent,0.f,&actions,&result);
}
// Original +d0 on every authenticated Grunt-family primary table is a
// six-byte MOV EAX,constant; RET. Only use this for the proven self attacker.
void bind_self_classification(unsigned vt,ServerNpcGruntGroundDeathBindings& b) noexcept {
    if(b.classification)return;
    switch(vt){
    case 0x2248df04u:case 0x2249aaa4u:case 0x2248c514u:case 0x2248e83cu:
        b.classification=+[](void*,void*,int* v) noexcept -> int{*v=11;return 1;};break;
    case 0x224954a4u:case 0x224a416cu:case 0x2249964cu:case 0x224a4a4cu:
        b.classification=+[](void*,void*,int* v) noexcept -> int{*v=6;return 1;};break;
    default:break;
    }
}
bool jet_primary_class(unsigned vt) noexcept {
    return vt==0x2248c514u||vt==0x2248e83cu||vt==0x2249964cu||vt==0x224a4a4cu;
}
bool jet_primary_token(unsigned token) noexcept {
    return !token||token==0x22365760u||token==0x221c0d40u;
}
int __cdecl jet_primary_install(void* a,unsigned n,unsigned vt,unsigned token) noexcept {
    if(!a||n<0x1074u||!jet_primary_class(vt)||rd<unsigned>(a,0)!=vt||!jet_primary_token(token))return 0;
    // 2214d6f0 null-context branch writes only +8.
    wr<unsigned>(a,8,token);return 1;
}
bool native_jet_death_effect(void* actor,unsigned n,void* subject,bool terminal,bool mode4,const ServerNpcJetDeathEffectBindings& b,ServerNpcJetDeathEffectResult& o) noexcept {
    o={};
    if(!actor||!subject||n<0x360u||!b.center||!b.burst||!b.light)return false;
    unsigned extent=n;
    if(subject!=actor&&(!b.extent||!b.extent(b.context,subject,&extent)||extent<0x360u))return false;
    // Immediate branch already refreshed its actor before invoking this
    // adapter. Terminal branch refreshes the independently resolved subject.
    if(terminal&&(rd<unsigned>(subject,0xb8)&0x800u)){
        if(!b.refresh_origin||!b.refresh_origin(b.context,subject))return false;
        o.refreshed=1;
    }
    if(!b.center(b.context,subject,o.burst_center))return false;++o.centers;
    for(unsigned i=0;i<3;++i){
        o.angles[i]=rd<float>(subject,0x354+4*i);
        if(!std::isfinite(o.burst_center[i])||!std::isfinite(o.angles[i]))return false;
    }
    // 22130fd0 translates its boolean inputs to the 22130df0 flag word.
    o.magnitude=terminal?30:256;o.radius=256;
    o.flags=terminal&&mode4?0x428u:0x429u;o.extra=terminal&&mode4?1000.f:0.f;
    if(!b.burst(b.context,o.burst_center,o.angles,actor,actor,o.magnitude,o.radius,o.flags,o.extra,0))return false;
    o.burst=1;
    // The second +214 call always uses the NPC, not the ragdoll/attachment.
    if(!b.center(b.context,actor,o.light_center))return false;++o.centers;
    for(float v:o.light_center)if(!std::isfinite(v))return false;
    if(!b.light(b.context,o.light_center,255,180,100,0,100.f,.1f,0.f))return false;
    o.light=o.completed=1;return true;
}
struct JetPrimaryRoute {
    const ServerNpcJetpackPrimaryThinkBindings& b;
    ServerNpcJetpackPrimaryThinkResult& out;
    void* actor;unsigned extent,vt;float interval;
};
bool jet_primary_death(JetPrimaryRoute& route,unsigned handle) noexcept {
    const auto& b=route.b;auto& o=route.out;
    if(!b.death||!b.death_result)return false;
    auto bound=*b.death;
    auto& identity=bound.ground.base.combat.death.event.prefix.removal.removal.leave.sentence;
    identity.entries=b.entities.entries;identity.count=b.entities.count;identity.speech.original_vtable=route.vt;
    // Delayed damage can have a foreign attacker. Only the serial-valid
    // self record permits constant classification without a host callback.
    if(sin_independent::EntityHandleView(b.entities.entries,b.entities.count).Resolve(rd<unsigned>(route.actor,0x1054))==route.actor)
        bind_self_classification(route.vt,bound.ground);
    bound.context=&route;
    bound.time=[](void* p,float* time) noexcept -> int{
        auto& q=*static_cast<JetPrimaryRoute*>(p);return q.b.clock(q.b.context,time);
    };
    bound.motion=[](void* p,void* a,void* rag,int mode) noexcept -> int{
        const auto& s=*static_cast<JetPrimaryRoute*>(p)->b.death;return s.motion&&s.motion(s.context,a,rag,mode);
    };
    bound.effect_begin=[](void* p,void* a) noexcept -> int{
        const auto& s=*static_cast<JetPrimaryRoute*>(p)->b.death;return s.effect_begin&&s.effect_begin(s.context,a);
    };
    bound.terminal_effect=[](void* p,void* a,void* e,bool mode4) noexcept -> int{
        auto& q=*static_cast<JetPrimaryRoute*>(p);
        if(a!=q.actor)return 0;
        if(q.b.effects)return native_jet_death_effect(a,q.extent,e,true,mode4,*q.b.effects,q.out.effects);
        const auto& s=*q.b.death;return s.terminal_effect&&s.terminal_effect(s.context,a,e,mode4);
    };
    bound.effect_end=[](void* p,void* a) noexcept -> int{
        const auto& s=*static_cast<JetPrimaryRoute*>(p)->b.death;return s.effect_end&&s.effect_end(s.context,a);
    };
    bound.remove=[](void* p,void* e) noexcept -> int{
        const auto& s=*static_cast<JetPrimaryRoute*>(p)->b.death;return s.remove&&s.remove(s.context,e);
    };
    bound.install_think=[](void* p,void* a,unsigned token) noexcept -> int{
        auto& q=*static_cast<JetPrimaryRoute*>(p);
        if(a!=q.actor||token!=0x221c0d40u||!jet_primary_install(a,q.extent,q.vt,token))return 0;
        q.out.installed=1;return 1;
    };
    bound.schedule=[](void* p,void* a,float time) noexcept -> int{
        auto& q=*static_cast<JetPrimaryRoute*>(p);
        if(a!=q.actor||!set_tick(a,time,q.interval,q.b.cadence,q.out.reschedule))return 0;
        q.out.reschedule.completed=q.out.rescheduled=1;return 1;
    };
    const auto* api=SiN_GetServerNpcJetpackDeathThinkApi(1,sizeof(ServerNpcJetpackDeathThinkApi));
    if(!api||!api->run(route.actor,route.extent,handle,&bound,b.death_result))return false;
    o.death_called=1;return true;
}
int __cdecl jet_primary_run(void* a,unsigned n,unsigned vt,unsigned handle,int tick,float interval,const ServerNpcJetpackPrimaryThinkBindings* b,ServerNpcJetpackPrimaryThinkResult* o) noexcept {
    if(!o)return 0;*o={};o->due_tick=o->last_tick=-1;
    if(!a||n<0x1074u||!jet_primary_class(vt)||rd<unsigned>(a,0)!=vt||!b||!std::isfinite(interval)||interval<=0||
       b->entities.count>sin_independent::kEntityHandleCapacity||(b->entities.count&&!b->entities.entries)||
       sin_independent::EntityHandleView(b->entities.entries,b->entities.count).Resolve(handle)!=a)return 0;
    auto idle=[&](){o->native_result=o->completed=1;return 1;};
    if(rd<unsigned>(a,0xb8)&0x400000u){o->no_think=1;return idle();}
    o->due_tick=rd<int>(a,0x74);
    if(o->due_tick<=0||o->due_tick>tick){o->not_due=1;return idle();}
    // Unsupported executable identities fail closed rather than call PE VAs.
    if(!jet_primary_token(rd<unsigned>(a,8))||!b->clock||!b->record||!b->scope)return 0;
    float now=0;if(!b->clock(b->context,&now)||!std::isfinite(now))return 0;
    const double scheduled=double(o->due_tick)*double(interval);
    o->recorded_time=scheduled>double(now)?now:static_cast<float>(scheduled);
    if(!std::isfinite(o->recorded_time)||!b->record(b->context,a,tick,o->recorded_time,rd<int>(a,0x74)))return 0;
    o->recorded=1;
    // 22222b90 -> 2214d870(-1,-1), default branch invokes 2214bf30
    // twice. The second check sees changes made by the first notification.
    if(!set_tick(a,-1.f,interval,b->cadence,o->clear)||!think_flags(a,false,b->cadence,o->clear))return 0;
    o->clear.completed=1;
    if(!b->scope(b->context,a,true))return 0;o->entered=1;
    // 221658e0 -> +bc -> 22002ca0 reloads +8 after engine scope entry.
    o->token=rd<unsigned>(a,8);bool ok=jet_primary_token(o->token);
    if(ok&&o->token==0x22365760u){
        JetPrimaryRoute route{*b,*o,a,n,vt,interval};ok=jet_primary_death(route,handle);
    }else if(ok&&o->token==0x221c0d40u){
        // 221c0d40: +1b4 is RET on all four tables. Clear positive
        // health before the diagnostic; always request deferred removal.
        if(rd<int>(a,0x9c)>0){
            wr<int>(a,0x9c,0);o->health_zeroed=1;
            if(b->positive_health_warning)ok=b->positive_health_warning(b->context,a)!=0;
        }
        if(ok){ok=b->remove&&b->remove(b->context,a);o->removed=ok;}
    }
    const bool closed=b->scope(b->context,a,false)!=0;o->closed=closed;
    if(!ok||!closed)return 0;
    // 2214bfa0 stores a fresh live clock converted to tick, not due tick.
    if(!b->clock(b->context,&o->last_time)||!think_tick(o->last_time,interval,o->last_tick))return 0;
    wr<int>(a,0x34,o->last_tick);
    o->native_result=(rd<unsigned>(a,0xb8)&1u)==0;o->completed=1;return 1;
}
const ServerNpcJetpackPrimaryThinkApi jet_primary_api{1,sizeof(ServerNpcJetpackPrimaryThinkApi),&jet_primary_install,&jet_primary_run};
// Original resupply +c0 is 22001740 RET and +bc is 22002ca0,
 // the same primary-context plumbing as the recovered Jetpack path.
struct ResupplyPrimaryRoute {
    const ServerResupplyPrimaryThinkBindings& b;
    ServerResupplyPrimaryThinkResult& out;
    void* actor;
    float interval;
};
int __cdecl resupply_schedule(void* p,void* a,float time) noexcept {
    auto& q=*static_cast<ResupplyPrimaryRoute*>(p);
    if(a!=q.actor||!set_tick(a,time,q.interval,q.b.cadence,q.out.reschedule))return 0;
    q.out.reschedule.completed=q.out.rescheduled=1;return 1;
}
bool resupply_primary_start(void* a,unsigned n,unsigned handle,float interval,const ServerResupplyPrimaryThinkBindings* b,ServerResupplyPrimaryThinkResult* o) noexcept {
    if(!o)return false;*o={};o->due_tick=o->last_tick=-1;
    return a&&n>=0x3b0&&rd<unsigned>(a,0)==sin_port::native_resupply::resupply_vtable&&b&&
        std::isfinite(interval)&&interval>0&&b->entities.count<=sin_independent::kEntityHandleCapacity&&
        (!b->entities.count||b->entities.entries)&&
        sin_independent::EntityHandleView(b->entities.entries,b->entities.count).Resolve(handle)==a;
}
int __cdecl resupply_primary_spawn(void* a,unsigned n,unsigned handle,float interval,const ServerResupplyPrimaryThinkBindings* b,ServerResupplyPrimaryThinkResult* o) noexcept {
    if(!resupply_primary_start(a,n,handle,interval,b,o)||!b->lifecycle)return 0;
    const auto* api=SiN_GetServerDynamicResupplyApi(1,sizeof(sin_port::native_resupply::Api));if(!api)return 0;
    ResupplyPrimaryRoute route{*b,*o,a,interval};
    auto bound=*b->lifecycle;bound.schedule=&resupply_schedule;bound.schedule_context=&route;
    bound.entries=b->entities.entries;bound.entry_count=b->entities.count;
    if(!api->spawn(a,n,&bound,&o->spawn))return 0;
    o->completed=o->native_result=1;return 1;
}
int __cdecl resupply_primary_run(void* a,unsigned n,unsigned handle,int tick,float interval,const ServerResupplyPrimaryThinkBindings* b,ServerResupplyPrimaryThinkResult* o) noexcept {
    if(!resupply_primary_start(a,n,handle,interval,b,o))return 0;
    auto idle=[&](){o->native_result=o->completed=1;return 1;};
    if(rd<unsigned>(a,0xb8)&0x400000u){o->no_think=1;return idle();}
    o->due_tick=rd<int>(a,0x74);
    if(o->due_tick<=0||o->due_tick>tick){o->not_due=1;return idle();}
    const auto supported=[](unsigned token){return token==0||token==sin_port::native_resupply::check_pvs_think;};
    if(!supported(rd<unsigned>(a,8))||!b->clock||!b->record||!b->scope)return 0;
    float now=0;if(!b->clock(b->context,&now)||!std::isfinite(now))return 0;
    const double scheduled=double(o->due_tick)*double(interval);
    o->recorded_time=scheduled>double(now)?now:float(scheduled);
    if(!std::isfinite(o->recorded_time)||!b->record(b->context,a,tick,o->recorded_time,rd<int>(a,0x74)))return 0;
    o->recorded=1;
    if(!set_tick(a,-1.f,interval,b->cadence,o->clear)||!think_flags(a,false,b->cadence,o->clear))return 0;
    o->clear.completed=1;
    if(!b->scope(b->context,a,true))return 0;o->entered=1;
    o->token=rd<unsigned>(a,8);bool ok=supported(o->token);
    if(ok&&!o->token)o->null_token=1;
    else if(ok){
        const auto* api=SiN_GetServerDynamicResupplyApi(1,sizeof(sin_port::native_resupply::Api));
        if(!api||!b->dispatch||!b->dispatch->evaluation)ok=false;
        else{
            ResupplyPrimaryRoute route{*b,*o,a,interval};
            auto bound=*b->dispatch;auto eval=*bound.evaluation;
            eval.entries=b->entities.entries;eval.entry_count=b->entities.count;
            if(b->lifecycle&&b->lifecycle->master_handle)eval.master_handle=b->lifecycle->master_handle;
            bound.evaluation=&eval;bound.schedule=&resupply_schedule;bound.schedule_context=&route;
            o->invoked=1;ok=api->check_pvs(a,n,b->debug,&bound,&o->dispatch)!=0;
        }
    }
    const bool closed=b->scope(b->context,a,false)!=0;o->closed=closed;
    if(!ok||!closed)return 0;
    if(!b->clock(b->context,&o->last_time)||!think_tick(o->last_time,interval,o->last_tick))return 0;
    wr<int>(a,0x34,o->last_tick);
    o->native_result=(rd<unsigned>(a,0xb8)&1u)==0;o->completed=1;return 1;
}
const ServerResupplyPrimaryThinkApi resupply_primary_api{1,sizeof(ServerResupplyPrimaryThinkApi),&resupply_primary_spawn,&resupply_primary_run};
struct AntigenPrimaryRoute {
    const ServerAntigenPrimaryThinkBindings& b;
    ServerAntigenPrimaryThinkResult& out;
    void* actor;float interval;
};
int __cdecl antigen_primary_schedule(void* p,void* a,float time) noexcept {
    auto& q=*static_cast<AntigenPrimaryRoute*>(p);
    if(a!=q.actor||!set_tick(a,time,q.interval,q.b.cadence,q.out.reschedule))return 0;
    q.out.reschedule.completed=q.out.rescheduled=1;return 1;
}
bool antigen_primary_start(void* a,unsigned n,unsigned handle,float interval,const ServerAntigenPrimaryThinkBindings* b,ServerAntigenPrimaryThinkResult* o) noexcept {
    if(!o)return false;*o={};o->due_tick=o->last_tick=-1;
    return a&&n>=native_resupply::antigen_extent&&rd<unsigned>(a,0)==native_resupply::antigen_vtable&&b&&
        std::isfinite(interval)&&interval>0&&b->entities.count<=sin_independent::kEntityHandleCapacity&&
        (!b->entities.count||b->entities.entries)&&sin_independent::EntityHandleView(b->entities.entries,b->entities.count).Resolve(handle)==a;
}
unsigned antigen_primary_kind(void* a,unsigned n) noexcept {
    if(!a||n<4)return 0;
    using namespace native_resupply;
    const auto vt=rd<unsigned>(a,0);
    if(vt==antigen_vtable&&n>=antigen_extent)return 1;
    if(vt==antigen_gas_vtable&&n>=antigen_gas_extent&&rd<int>(a,0x398)==1)return 2;
    if(vt==antigen_canister_vtable&&n>=antigen_canister_extent)return 3;
    return 0;
}
bool antigen_aux_start(void* a,unsigned n,unsigned handle,float interval,const ServerAntigenPrimaryThinkBindings* b,ServerAntigenPrimaryThinkResult* o) noexcept {
    if(!o)return false;*o={};o->due_tick=o->last_tick=-1;
    return antigen_primary_kind(a,n)&&b&&std::isfinite(interval)&&interval>0&&
        b->entities.count<=sin_independent::kEntityHandleCapacity&&(!b->entities.count||b->entities.entries)&&
        sin_independent::EntityHandleView(b->entities.entries,b->entities.count).Resolve(handle)==a;
}
int __cdecl antigen_primary_schedule_aux(void* a,unsigned n,unsigned handle,float interval,float time,const ServerAntigenPrimaryThinkBindings* b,ServerAntigenPrimaryThinkResult* o) noexcept {
    if(!antigen_aux_start(a,n,handle,interval,b,o))return 0;
    using namespace native_resupply;
    const auto kind=antigen_primary_kind(a,n),token=rd<unsigned>(a,8);
    if(kind==1||(kind==2&&token!=antigen_gas_think_token&&token!=0)||(kind==3&&token!=antigen_canister_idle&&token!=0))return 0;
    AntigenPrimaryRoute q{*b,*o,a,interval};
    if(!antigen_primary_schedule(&q,a,time))return 0;o->completed=o->native_result=1;return 1;
}
void bind_antigen_control(native_resupply::AntigenBindings& c,AntigenPrimaryRoute& q) noexcept {
    c.entries=q.b.entities.entries;c.entry_count=q.b.entities.count;
    c.schedule=&antigen_primary_schedule;c.schedule_context=&q;
}
int __cdecl antigen_primary_spawn(void* a,unsigned n,unsigned handle,float interval,const ServerAntigenPrimaryThinkBindings* b,ServerAntigenPrimaryThinkResult* o) noexcept {
    if(!antigen_primary_start(a,n,handle,interval,b,o)||!b->interaction)return 0;
    const auto* api=SiN_GetServerDynamicResupplyApi(1,sizeof(native_resupply::Api));if(!api)return 0;
    AntigenPrimaryRoute q{*b,*o,a,interval};auto bound=*b->interaction;bind_antigen_control(bound.control,q);
    if(!api->antigen_spawn(a,n,&bound,&o->interaction))return 0;
    o->completed=o->native_result=1;return 1;
}
int __cdecl antigen_primary_use(void* a,unsigned n,unsigned handle,float interval,void* pl,unsigned bytes,int use,const ServerAntigenPrimaryThinkBindings* b,ServerAntigenPrimaryThinkResult* o) noexcept {
    if(!antigen_primary_start(a,n,handle,interval,b,o)||!b->interaction)return 0;
    const auto* api=SiN_GetServerDynamicResupplyApi(1,sizeof(native_resupply::Api));if(!api)return 0;
    AntigenPrimaryRoute q{*b,*o,a,interval};auto bound=*b->interaction;bind_antigen_control(bound.control,q);
    if(!api->antigen_use_event(a,n,pl,bytes,use,&bound,&o->interaction))return 0;
    o->completed=o->native_result=1;return 1;
}
int __cdecl antigen_primary_button(void* a,unsigned n,unsigned handle,float interval,const char* name,int index,const char* command,const ServerAntigenPrimaryThinkBindings* b,ServerAntigenPrimaryThinkResult* o) noexcept {
    if(!antigen_primary_start(a,n,handle,interval,b,o)||!b->interaction)return 0;
    const auto* api=SiN_GetServerDynamicResupplyApi(1,sizeof(native_resupply::Api));if(!api)return 0;
    AntigenPrimaryRoute q{*b,*o,a,interval};auto bound=*b->interaction;bind_antigen_control(bound.control,q);
    if(!api->antigen_button_event(a,n,name,index,command,&bound,&o->interaction))return 0;
    o->completed=o->native_result=1;return 1;
}
int __cdecl antigen_primary_run(void* a,unsigned n,unsigned handle,int tick,float interval,const ServerAntigenPrimaryThinkBindings* b,ServerAntigenPrimaryThinkResult* o) noexcept {
    if(!antigen_aux_start(a,n,handle,interval,b,o))return 0;
    const unsigned kind=antigen_primary_kind(a,n);
    auto idle=[&](){o->native_result=o->completed=1;return 1;};
    if(rd<unsigned>(a,0xb8)&0x400000u){o->no_think=1;return idle();}
    o->due_tick=rd<int>(a,0x74);
    if(o->due_tick<=0||o->due_tick>tick){o->not_due=1;return idle();}
    using namespace native_resupply;
    auto supported=[&](unsigned v){return !v||(kind==1&&(v==antigen_animation_token||v==antigen_think_token||v==antigen_eject_token))||(kind==2&&v==antigen_gas_think_token)||(kind==3&&v==antigen_canister_idle);};
    if(!supported(rd<unsigned>(a,8))||!b->clock||!b->record||!b->scope)return 0;
    float now=0;if(!b->clock(b->context,&now)||!std::isfinite(now))return 0;
    const double scheduled=double(o->due_tick)*double(interval);
    o->recorded_time=scheduled>double(now)?now:float(scheduled);
    if(!std::isfinite(o->recorded_time)||!b->record(b->context,a,tick,o->recorded_time,rd<int>(a,0x74)))return 0;
    o->recorded=1;
    if(!set_tick(a,-1.f,interval,b->cadence,o->clear)||!think_flags(a,false,b->cadence,o->clear))return 0;
    o->clear.completed=1;
    if(!b->scope(b->context,a,true))return 0;o->entered=1;
    o->token=rd<unsigned>(a,8);bool ok=supported(o->token);
    if(ok&&!o->token)o->null_token=1;
    else if(ok){
        const auto* api=SiN_GetServerDynamicResupplyApi(1,sizeof(Api));
        AntigenPrimaryRoute q{*b,*o,a,interval};
        if(!api)ok=false;
        else if(kind==2){
            if(!b->gas)ok=false;
            else{
                auto bound=*b->gas;bound.schedule=&antigen_primary_schedule;bound.schedule_context=&q;
                AntigenRadiusBindings radius{};
                if(bound.native_radius){radius=*bound.native_radius;radius.entries=b->entities.entries;radius.entry_count=b->entities.count;bound.native_radius=&radius;}
                o->invoked=1;ok=api->antigen_gas_think(a,n,&bound,&o->gas)!=0;
            }
        }else if(kind==3){
            if(!b->canister)ok=false;
            else{auto bound=*b->canister;bound.schedule=&antigen_primary_schedule;bound.schedule_context=&q;o->invoked=1;ok=api->canister_idle(a,n,&bound,&o->canister)!=0;}
        }else if(o->token==antigen_eject_token){
            if(!b->exchange)ok=false;
            else{auto bound=*b->exchange;bind_antigen_control(bound.control,q);o->invoked=1;ok=api->antigen_eject(a,n,&bound,&o->exchange)!=0;}
        }else if(!b->interaction)ok=false;
        else{
            auto bound=*b->interaction;bind_antigen_control(bound.control,q);o->invoked=1;
            ok=o->token==antigen_animation_token?api->antigen_animation_think(a,n,&bound,&o->interaction)!=0:
                api->antigen_think(a,n,&bound.control,&o->healing)!=0;
        }
    }
    const bool closed=b->scope(b->context,a,false)!=0;o->closed=closed;
    if(!ok||!closed)return 0;
    if(!b->clock(b->context,&o->last_time)||!think_tick(o->last_time,interval,o->last_tick))return 0;
    wr<int>(a,0x34,o->last_tick);
    o->native_result=(rd<unsigned>(a,0xb8)&1u)==0;o->completed=1;return 1;
}
const ServerAntigenPrimaryThinkApi antigen_primary_api{1,sizeof(ServerAntigenPrimaryThinkApi),&antigen_primary_spawn,&antigen_primary_use,&antigen_primary_button,&antigen_primary_run,&antigen_primary_schedule_aux};
struct JetInitialThinkRoute {
    const ServerNpcJetpackDeathBranchBindings* source;
    const ServerNpcJetpackDeathModeBindings* mode_source;
    ServerNpcOuterCadenceBindings cadence;
    ServerEntityAcceptInputResult* out;
    void* actor;unsigned extent,vt;float interval;
    const ServerNpcInputJetThinkScheduleBindings* initial;
    const ServerWeaponActivityBindings* entities;
};
void bind_jet_death_pickup(ServerNpcJetpackDeathModeBindings& bound,JetInitialThinkRoute& route) noexcept {
    bound.context=&route;
    bound.random_int=[](void* p,int lo,int hi,int* v) noexcept -> int{
        const auto& s=*static_cast<JetInitialThinkRoute*>(p)->mode_source;
        return s.random_int&&s.random_int(s.context,lo,hi,v);
    };
    bound.effect_rule=[](void* p,void* a,bool* yes) noexcept -> int{
        const auto& s=*static_cast<JetInitialThinkRoute*>(p)->mode_source;
        return s.effect_rule&&s.effect_rule(s.context,a,yes);
    };
    bound.force_mode3=[](void* p,bool* yes) noexcept -> int{
        const auto& s=*static_cast<JetInitialThinkRoute*>(p)->mode_source;
        return s.force_mode3&&s.force_mode3(s.context,yes);
    };
    bound.tail=[](void* p,void* a,void* damage,unsigned mode,unsigned token) noexcept -> int{
        const auto& s=*static_cast<JetInitialThinkRoute*>(p)->mode_source;
        return s.tail&&s.tail(s.context,a,damage,mode,token);
    };
    bound.effect=[](void* p,void* a) noexcept -> int{
        auto& q=*static_cast<JetInitialThinkRoute*>(p);auto& o=*q.out;
        const auto& b=*q.initial->pickup;
        if(a!=q.actor||q.extent<0x360u)return 0;
        ++o.health_jet_pickup_native;
        if(rd<unsigned>(a,0xb8)&0x800u){
            if(!b.refresh_origin||!b.refresh_origin(b.context,a))return 0;
            ++o.health_jet_pickup_refreshed;
        }
        if(!b.center(b.context,a,o.health_jet_pickup_center))return 0;
        for(float v:o.health_jet_pickup_center)if(!std::isfinite(v))return 0;
        // 22146c40 allocates by classname with forced edict -1. Even
        // a null entity passes through DispatchSpawn's null path.
        void* item=nullptr;
        if(!b.create(b.context,"item_healthvial",-1,&item))return 0;
        if(item){
            ++o.health_jet_pickup_created;
            if(!b.position||!b.position(b.context,item,o.health_jet_pickup_center))return 0;
            ++o.health_jet_pickup_positioned;
            for(unsigned i=0;i<3;++i){o.health_jet_pickup_angles[i]=rd<float>(a,0x354+4*i);if(!std::isfinite(o.health_jet_pickup_angles[i]))return 0;}
            if(!b.orientation||!b.orientation(b.context,item,o.health_jet_pickup_angles))return 0;
            ++o.health_jet_pickup_oriented;
            if(!b.owner||!b.owner(b.context,item,nullptr))return 0;
            ++o.health_jet_pickup_owner;
        }
        if(!b.spawn(b.context,item))return 0;
        ++o.health_jet_pickup_spawned;return 1;
    };
}
void bind_jet_initial_think(ServerNpcJetpackDeathBranchBindings& bound,JetInitialThinkRoute& route) noexcept {
    bound.context=&route;
    // The original branch has one context for all its services. Preserve
    // each hosted effect's context; replace only the two default-think leaves.
    bound.remove=[](void* p,void* e) noexcept -> int{
        const auto& s=*static_cast<JetInitialThinkRoute*>(p)->source;return s.remove&&s.remove(s.context,e);
    };
    bound.powerdown=[](void* p,void* a,const char* cue) noexcept -> int{
        const auto& s=*static_cast<JetInitialThinkRoute*>(p)->source;return s.powerdown&&s.powerdown(s.context,a,cue);
    };
    bound.sentence=[](void* p,void* a,const char* cue) noexcept -> int{
        auto& q=*static_cast<JetInitialThinkRoute*>(p);const auto& s=*q.source;
        if(a!=q.actor)return 0;
        if(!q.initial->sentence)return s.sentence&&s.sentence(s.context,a,cue);
        if(!cue||std::strcmp(cue,"SINTEK_JETPACK_DEATH"))return 0;
        float now=0;if(!s.time||!s.time(s.context,&now)||!std::isfinite(now))return 0;
        auto sentence=*q.initial->sentence;
        sentence.entries=q.entities->entries;sentence.count=q.entities->count;
        sentence.speech.original_vtable=q.vt;
        const auto* api=SiN_GetServerNpcGruntSentenceRequestApi(1,sizeof(ServerNpcGruntSentenceRequestApi));
        // Original pushes mode 0, priority 1, group token 22498818.
        // The caller ignores the signed sentence result, not adapter failure.
        if(!api||!api->dispatch(a,q.extent,now,cue,1,0,&sentence,q.initial->sentence_request,
            &q.out->health_jet_sentence,&q.out->health_jet_request))return 0;
        ++q.out->health_jet_sentence_native;return 1;
    };
    bound.ignite=[](void* p,void* a,float time,int flags,void* x,void* y,float radius,int last) noexcept -> int{
        auto& q=*static_cast<JetInitialThinkRoute*>(p);const auto& s=*q.source;
        if(a!=q.actor)return 0;
        const auto* ignition=q.initial->ignition;
        if(!ignition)return s.ignite&&s.ignite(s.context,a,time,flags,x,y,radius,last);
        // Only the exact delayed-death +2d4 call is admitted here.
        // 2248c514's 22353da0 wrapper admits its nonzero final byte;
        // the other three enter 223595a0 directly. Null attacker and
        // inflictor write invalid handle and empty name before base.
        if(time!=1.f||flags!=1||x||y||radius!=25.f||last!=1||q.extent<0xc44u||!ignition->base)return 0;
        ++q.out->health_jet_ignition_native;
        wr<unsigned>(a,0xc3c,0xffffffffu);wr<unsigned>(a,0xc40,0u);
        ++q.out->health_jet_ignition_attribution;
        if(!ignition->base(ignition->context,a,time,flags,x,y,radius,last))return 0;
        ++q.out->health_jet_ignition_base;return 1;
    };
    bound.ragdoll=[](void* p,void* a,void* damage,const float* force) noexcept -> int{
        auto& q=*static_cast<JetInitialThinkRoute*>(p);
        if(a!=q.actor)return 0;
        if(q.initial->ragdoll){
            auto native=*q.initial->ragdoll;
            native.entities.entries=q.entities->entries;native.entities.count=q.entities->count;
            native.entities.speech.original_vtable=q.vt;
            const auto* api=SiN_GetServerNpcGruntRagdollRouteApi(1,sizeof(ServerNpcGruntRagdollRouteApi));
            if(!api||!api->run(a,q.extent,damage,0x48,force,&native,q.initial->ragdoll_result))return 0;
            ++q.out->health_jet_ragdoll_native;
            // 223668ac ignores AL; a completed native false return also
            // proceeds to stamp and initial scheduling.
            return 1;
        }
        const auto& s=*q.source;return s.ragdoll&&s.ragdoll(s.context,a,damage,force);
    };
    bound.time=[](void* p,float* time) noexcept -> int{
        const auto& s=*static_cast<JetInitialThinkRoute*>(p)->source;return s.time&&s.time(s.context,time);
    };
    bound.effect_begin=[](void* p,void* a) noexcept -> int{
        const auto& s=*static_cast<JetInitialThinkRoute*>(p)->source;return s.effect_begin&&s.effect_begin(s.context,a);
    };
    bound.refresh_origin=[](void* p,void* a) noexcept -> int{
        const auto& s=*static_cast<JetInitialThinkRoute*>(p)->source;return s.refresh_origin&&s.refresh_origin(s.context,a);
    };
    bound.immediate_effect=[](void* p,void* a) noexcept -> int{
        auto& q=*static_cast<JetInitialThinkRoute*>(p);
        if(a!=q.actor)return 0;
        if(q.initial->effects)return native_jet_death_effect(a,q.extent,a,false,false,*q.initial->effects,q.out->health_jet_effects);
        const auto& s=*q.source;return s.immediate_effect&&s.immediate_effect(s.context,a);
    };
    bound.effect_end=[](void* p,void* a) noexcept -> int{
        const auto& s=*static_cast<JetInitialThinkRoute*>(p)->source;return s.effect_end&&s.effect_end(s.context,a);
    };
    bound.install_think=[](void* p,void* a,unsigned token) noexcept -> int{
        auto& q=*static_cast<JetInitialThinkRoute*>(p);
        if(a!=q.actor||token!=0x22365760u||!jet_primary_install(a,q.extent,q.vt,token))return 0;
        ++q.out->health_jet_think_installed;return 1;
    };
    bound.schedule=[](void* p,void* a,float time) noexcept -> int{
        auto& q=*static_cast<JetInitialThinkRoute*>(p);
        if(a!=q.actor||!set_tick(a,time,q.interval,q.cadence,q.out->health_jet_cadence))return 0;
        q.out->health_jet_cadence.completed=1;++q.out->health_jet_think_scheduled;return 1;
    };
}
bool native_self_damage(void* a,unsigned n,unsigned vt,void* info,const ServerEntityAcceptInputBindings& b,ServerEntityAcceptInputResult& o) noexcept {
    const auto& dispatch=*b.native_health;
    if(n<0xdb2||!b.storage||!std::isfinite(b.now)||b.entities.count>sin_independent::kEntityHandleCapacity||(b.entities.count&&!b.entities.entries))return false;
    const sin_independent::EntityHandleView registry(b.entities.entries,b.entities.count);
    // This specialized route is only valid for the original type-zero
    // SetHealth self-inflictor/self-attacker record.
    if(rd<unsigned>(info,0x38)||registry.Resolve(rd<unsigned>(info,0x24))!=a||registry.Resolve(rd<unsigned>(info,0x28))!=a)return false;
    // 22358e40 -> 22033cf0 self-attacker bypasses friendly rejection,
    // then 2213c880 consults the live damage-filter parent at +a4.
    if(void* parent=registry.Resolve(rd<unsigned>(a,0xa4))){
        bool admitted=false;
        if(parent==a||!dispatch.parent_admit||!dispatch.parent_admit(dispatch.context,parent,info,&admitted))return false;
        if(!admitted){wr<std::uint8_t>(a,0xa28,1);++o.health_admission_blocked;return true;}
    }
    // 2220dbf0 can defer before copying or applying either scale.
    if(dispatch.defer_damage){
        if(!dispatch.enqueue||!dispatch.enqueue(dispatch.context,a,info,0x48))return false;
        ++o.health_deferred;return true;
    }
    const bool ground=dispatch.ground_death&&(vt==0x224954a4u||vt==0x2249aaa4u||vt==0x224a416cu);
    const bool elite=dispatch.subclass_death&&vt==0x2248df04u;
    const bool jet=dispatch.subclass_death&&(vt==0x2248c514u||vt==0x2248e83cu||vt==0x2249964cu||vt==0x224a4a4cu);
    if(!dispatch.life||(ground?!dispatch.ground_result:(elite||jet)?!dispatch.subclass_result:!dispatch.life_result))return false;
    // Reject undersized subclass storage before mutating the damage record.
    if((elite&&n<0xfe0u)||(jet&&n<0x1074u))return false;
    const auto* initial=jet?dispatch.jet_think:nullptr;
    if(initial&&(!initial->cadence||!std::isfinite(initial->interval)||initial->interval<=0||rd<unsigned>(a,0)!=vt||
        (initial->ragdoll&&!initial->ragdoll_result)||
        (bool(initial->sentence)!=bool(initial->sentence_request))||
        (initial->ignition&&!initial->ignition->base)||
        (initial->pickup&&(!initial->pickup->center||!initial->pickup->create||!initial->pickup->spawn))))return false;
    if(!self_damage_scale(a,*b.storage,false,o.health_outgoing_scale,o.health_multiplier_rows))return false;
    wr<float>(info,0x2c,static_cast<float>(double(rd<float>(info,0x2c))*o.health_outgoing_scale));
    if(!self_damage_scale(a,*b.storage,true,o.health_incoming_scale,o.health_multiplier_rows))return false;
    o.health_scaled_amount=static_cast<float>(double(rd<float>(info,0x2c))*o.health_incoming_scale);
    if(!std::isfinite(o.health_scaled_amount))return false;
    wr<float>(info,0x2c,o.health_scaled_amount);
    auto life=*dispatch.life;
    auto& identity=life.combat.alive.chain.detail.damage.base.chain.prefix.damage.reaction.sentence;
    identity.entries=b.entities.entries;identity.count=b.entities.count;identity.speech.original_vtable=vt;
    struct OutputRoute {
        void* actor;unsigned extent;float now;
        const ServerNpcHealthOutputQueueBindings* queue;
        const ServerNpcGruntDamageOutputBindings* original;
        ServerEntityAcceptInputResult* result;
        const ServerWeaponActivityBindings* entities;
    } route{a,n,b.now,dispatch.outputs,&dispatch.life->combat.alive.chain.detail.damage.base.chain.outputs,&o,&b.entities};
    if(dispatch.outputs){
        if(!route.queue->actions||!route.queue->queue)return false;
        auto& outputs=life.combat.alive.chain.detail.damage.base.chain.outputs;
        outputs.context=&route;
        outputs.output=[](void* p,void* actor,unsigned off) noexcept -> int{
            auto& q=*static_cast<OutputRoute*>(p);
            if(actor!=q.actor||(off!=0xa48u&&off!=0xa78u&&off!=0xb68u&&off!=0xb80u)||q.extent<off+0x18)return 0;
            ServerEntityVoidOutputResult result{};
            if(!fire_actor_output_queue(actor,q.extent,off,q.now,*q.entities,*q.queue,result))return 0;
            ++q.result->health_outputs;q.result->health_output_actions+=result.queued;q.result->health_output_removed+=result.removed;
            q.result->health_output_enqueued+=result.queued;return 1;
        };
        // The authenticated original Grunt +120 returns false and the
        // CBaseNPC cast of self succeeds. Only local-player lookup is hosted.
        outputs.is_player=[](void* p,void* e,bool* player) noexcept -> int{
            auto& q=*static_cast<OutputRoute*>(p);if(e!=q.actor)return 0;*player=false;return 1;
        };
        outputs.cast_npc=[](void* p,void* e,void** npc,unsigned* bytes) noexcept -> int{
            auto& q=*static_cast<OutputRoute*>(p);if(e!=q.actor)return 0;*npc=e;*bytes=q.extent;return 1;
        };
        outputs.local_player=[](void* p,void** player) noexcept -> int{
            const auto& src=*static_cast<OutputRoute*>(p)->original;
            return src.local_player&&src.local_player(src.context,player);
        };
    }
    if(ground){
        ServerNpcGruntGroundLifeBindings bound{};bound.life=life;bound.ground=*dispatch.ground_death;
        bind_self_classification(vt,bound.ground);
        const auto* api=SiN_GetServerNpcGruntGroundLifeApi(1,sizeof(ServerNpcGruntGroundLifeApi));
        if(!api||!api->run(a,n,rd<unsigned>(a,0x378),info,0x48,b.now,&bound,dispatch.ground_result))return false;
        ++o.health_ground_route;o.health_ground_death=dispatch.ground_result->ground_called;
        if(dispatch.life_result&&dispatch.life_result!=&dispatch.ground_result->life)*dispatch.life_result=dispatch.ground_result->life;
    }else if(elite||jet){
        // Phase 1228's class-aware dispatcher owns the Elite wrapper and
        // Jetpack branch orchestration. Do not route either through the
        // plain ground death entry, or use an unrelated nested life identity.
        auto bound=*dispatch.subclass_death;
        bound.combat.combat.life=life;
        bind_self_classification(vt,bound.combat.combat.ground);
        JetInitialThinkRoute think{};
        if(initial){
            think.source=&dispatch.subclass_death->combat.branch;
            think.cadence=*initial->cadence;think.cadence.storage=b.storage;
            think.out=&o;think.actor=a;think.extent=n;think.vt=vt;think.interval=initial->interval;
            think.initial=initial;think.entities=&b.entities;
            bind_jet_initial_think(bound.combat.branch,think);
            if(initial->pickup){think.mode_source=&dispatch.subclass_death->combat.combat.jet;bind_jet_death_pickup(bound.combat.combat.jet,think);}
        }
        const auto* api=SiN_GetServerNpcGruntEliteLifeApi(1,sizeof(ServerNpcGruntEliteLifeApi));
        if(!api||!api->run(a,n,rd<unsigned>(a,0x378),info,0x48,b.now,&bound,dispatch.subclass_result))return false;
        o.health_elite_route=elite;o.health_jet_route=jet;
        auto& nested=dispatch.subclass_result->combat.combat.life;
        o.health_subclass_death=nested.death_called;
        if(dispatch.life_result&&dispatch.life_result!=&nested)*dispatch.life_result=nested;
    }else{
        const auto* api=SiN_GetServerNpcGruntLifeDispatchApi(1,sizeof(ServerNpcGruntLifeDispatchApi));
        if(!api||!api->run(a,n,info,0x48,b.now,&life,dispatch.life_result))return false;
    }
    // 2213bc80 immediately skips its player-attacker statistics branch
    // for the authenticated Grunt self attacker (+120 returns false).
    ++o.health_life_called;return true;
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
    case 0x220424a0u:return native_break_input(a,n,d,b,o);
    case 0x22357e50u:return native_assault(a,n,d,b,o);
    case 0x22031f90u:return native_relationship(a,n,d,b,o);
    case 0x2203f1f0u:return native_forget(a,n,vt,d,b,o);
    case 0x22037a30u:{ // SetHealth: heal through 222a57a0/22140be0,
        // or lower by constructing damage info via 221f89c0/221f8550.
        if(n<0x37c)return 0;
        const int requested=rd<unsigned>(d.variant,0x10)==5u?rd<int>(d.variant,0):0;
        const int prior=rd<int>(a,0x9c);
        if(requested==prior)return 1;
        const unsigned delta=static_cast<unsigned>(requested)-static_cast<unsigned>(prior);
        // x86 SUB/CDQ/XOR/SUB implements wrapping signed absolute value.
        const unsigned magnitude=delta&0x80000000u?0u-delta:delta;
        int signed_magnitude=0;std::memcpy(&signed_magnitude,&magnitude,4);
        const float amount=static_cast<float>(signed_magnitude);
        if(requested>prior){
            const int maximum=rd<int>(a,0x98);
            if(!rd<unsigned>(a,0x16c)||rd<int>(a,0x94)<2||prior>=maximum){++o.health_blocked;return 1;}
            const double truncated=std::trunc(double(amount));
            const int added=truncated<double(std::numeric_limits<int>::min())||truncated>double(std::numeric_limits<int>::max())?std::numeric_limits<int>::min():static_cast<int>(truncated);
            const unsigned bits=static_cast<unsigned>(prior)+static_cast<unsigned>(added);
            int next=0;std::memcpy(&next,&bits,4);
            // Shared +1b4 is RET and +120 returns false on all eight tables.
            wr<int>(a,0x9c,next);if(next>maximum)wr<int>(a,0x9c,maximum);
            ++o.health_healed;return 1;
        }
        if(!b.health_damage&&!b.native_health)return -1;
        alignas(4) unsigned char info[0x48]{};
        const unsigned self=rd<unsigned>(a,0x378);
        wr<unsigned>(info,0x24,self);wr<unsigned>(info,0x28,self);
        wr<float>(info,0x2c,amount);wr<float>(info,0x30,amount);
        wr<unsigned>(info,0x34,0x7f7fffffu);wr<unsigned>(info,0x40,0xffffffffu);
        if(b.native_health){if(!native_self_damage(a,n,vt,info,b,o))return 0;}
        else if(!b.health_damage(b.context,a,info,sizeof(info)))return 0;
        ++o.health_damage;return 1;
    }
    case 0x22032000u:return native_set_squad(a,n,vt,d,b,o);
    case 0x22031fe0u:return native_rappel_input(a,n,true,b,o);
    case 0x22031ff0u:return native_rappel_input(a,n,false,b,o);
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
    case 0x22365fe0u:{ // ChangePathTrack
        if(!b.search||!b.jet)return -1;
        if(input_datamap(vt)!=0x22535b28u||n<0x1094)return 0;
        const unsigned type=rd<unsigned>(d.variant,0x10);
        if(type!=0u&&type!=2u)return -1;
        if(b.entities.count>sin_independent::kEntityHandleCapacity||(b.entities.count&&!b.entities.entries))return 0;
        auto search=*b.search;search.entities=b.entities;
        const sin_independent::EntityHandleView registry(b.entities.entries,b.entities.count);
        ServerEntitySearchResult found{};
        const unsigned name=type==2u?rd<unsigned>(d.variant,0):0u;
        if(!search_named(nullptr,name,nullptr,nullptr,&search,&found))return 0;
        unsigned handle=0xffffffffu;
        if(found.entity&&!search_handle(found.entity,search,handle))return 0;
        wr<unsigned>(a,0xff4,handle);
        void* path=registry.Resolve(handle);
        // The original fetches the path's handle again for each cached field.
        for(unsigned off:{0x1088u,0x108cu}){
            unsigned cached=0xffffffffu;
            if(path&&!search_handle(path,search,cached))return 0;
            wr<unsigned>(a,off,cached);
        }
        wr<unsigned>(a,0x1090,static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(path)));
        wr<std::uint8_t>(a,0xff8,0);++o.path_changed;
        if(!registry.Resolve(rd<unsigned>(a,0xff4))){
            ++o.path_missing;return !b.path_warning||b.path_warning(b.context,a,0x224984f8u);
        }
        if(!b.jet->reset_navigation||!b.jet->reset_navigation(b.jet->context,a,rd<unsigned>(a,0x8f8)))return 0;
        ++o.path_reset;return 1;
    }
    case 0x22367470u:{ // ActivateJets -> 22366a20
        if(input_datamap(vt)!=0x22535b28u||n<0xfec)return 0;
        if(b.entities.count>sin_independent::kEntityHandleCapacity||(b.entities.count&&!b.entities.entries))return 0;
        const sin_independent::EntityHandleView registry(b.entities.entries,b.entities.count);
        if(registry.Resolve(rd<unsigned>(a,0xfe4))||registry.Resolve(rd<unsigned>(a,0xfe8))){++o.thrust_reused;return 1;}
        if(b.jet&&b.jet->fresh){
            if(!fresh_jet_thrust(a,n,b.entities,b.storage,*b.jet->fresh,o.thrust))return 0;
            ++o.thrust_prepared;return 1;
        }
        if(!b.jet||!b.jet->prepare_thrust)return -1;
        if(!b.jet->prepare_thrust(b.jet->context,a))return 0;
        ++o.thrust_prepared;return 1;
    }
    case 0x22366110u:{ // DeactivateJets -> 22363f30; right before left
        if(input_datamap(vt)!=0x22535b28u||n<0xfec)return 0;
        if(b.entities.count>sin_independent::kEntityHandleCapacity||(b.entities.count&&!b.entities.entries))return 0;
        const sin_independent::EntityHandleView registry(b.entities.entries,b.entities.count);
        for(unsigned off:{0xfe8u,0xfe4u}){
            void* thrust=registry.Resolve(rd<unsigned>(a,off));if(!thrust)continue;
            if(!b.jet||!b.jet->remove_thrust||!b.jet->remove_thrust(b.jet->context,thrust))return 0;
            wr<unsigned>(a,off,0xffffffffu);++o.thrust_removed;
        }
        return 1;
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
constexpr NativeInputMap colorblend_map{0x224fb510u,0x224f0350u,6,colorblend_inputs,1};
int native_colorblend_input(void* a,unsigned n,unsigned handler,const ServerEntityInputData& data,const ServerEntityAcceptInputBindings& b) noexcept {
    if(!b.math_colorblend||handler!=0x2217a740u)return -1;
    const auto* api=SiN_GetServerMapEntityApi(1,sizeof(sin_port::native_map::Api));
    sin_port::native_map::ColorBlendResult result{};
    return api&&api->colorblend_input(a,n,"InValue",data.activator,rd<unsigned>(data.variant,0x10),rd<float>(data.variant,0),b.math_colorblend,&result)&&result.completed&&result.handled?1:0;
}
constexpr NativeInputMap remap_map{0x224fb4f8u,0x224f0350u,6,remap_inputs,1};
int native_remap_input(void* a,unsigned n,unsigned handler,const ServerEntityInputData& data,const ServerEntityAcceptInputBindings& b) noexcept {
    if(!b.math_remap||handler!=0x2217a610u)return -1;
    const auto* api=SiN_GetServerMapEntityApi(1,sizeof(sin_port::native_map::Api));
    sin_port::native_map::RemapResult result{};
    return api&&api->remap_input(a,n,"InValue",data.activator,rd<unsigned>(data.variant,0x10),rd<float>(data.variant,0),b.math_remap,&result)&&result.completed&&result.handled?1:0;
}
constexpr NativeInputMap multicompare_map{0x224fb4b0u,0x224f0350u,7,multicompare_inputs,2};
int native_multicompare_input(void* a,unsigned n,unsigned handler,const ServerEntityInputData& data,const ServerEntityAcceptInputBindings& b) noexcept {
    if(!b.logic_multicompare)return -1;
    const auto* api=SiN_GetServerMapEntityApi(1,sizeof(sin_port::native_map::Api));
    if(handler==0x2213cdc0u&&sin_port::native_map::kill_available(b.logic_multicompare->kill)){
        sin_port::native_map::EntityKillResult result{};
        return api&&api->multicompare_kill(a,n,b.entities.entries,b.entities.count,b.logic_multicompare->kill,&result)&&result.completed&&result.handled?1:0;
    }
    const char* text=nullptr;
    for(const auto& row:multicompare_inputs)if(row.handler==handler){text=row.text;break;}
    if(!text)return -1;
    sin_port::native_map::MultiCompareResult result{};
    return api&&api->multicompare_input(a,n,text,data.activator,data.variant,sizeof(data.variant),data.output_id,b.logic_multicompare,&result)&&result.completed&&result.handled?1:0;
}
constexpr NativeInputMap autosave_map{0x224fb5b8u,0x224f0350u,2,autosave_inputs,1};
constexpr NativeInputMap logic_base_kill_map{0x224f0350u,0,115,base_kill_inputs,1};
int native_autosave_input(void* a,unsigned n,unsigned handler,const ServerEntityAcceptInputBindings& b) noexcept {
    if(!b.logic_autosave)return -1;
    const auto* api=SiN_GetServerMapEntityApi(1,sizeof(sin_port::native_map::Api));
    sin_port::native_map::AutosaveResult result{};
    if(handler==0x22178dd0u)
        return api&&api->autosave_input(a,n,"Save",b.logic_autosave,&result)&&result.completed&&result.handled?1:0;
    if(handler==0x2213cdc0u&&(b.logic_autosave->remove||b.logic_autosave->deferred))
        return api&&api->autosave_kill(a,n,b.entities.entries,b.entities.count,b.logic_autosave,&result)&&result.completed&&result.handled?1:0;
    return -1;
}
constexpr NativeInputMap navigation_map{0x224fb0c8u,0x224f0350u,5,navigation_inputs,3};
int native_navigation_input(void* a,unsigned n,unsigned handler,const ServerEntityAcceptInputBindings& b) noexcept {
    if(!b.logic_navigation)return -1;
    const char* text=nullptr;
    for(const auto& row:navigation_inputs)if(row.handler==handler){text=row.text;break;}
    if(!text)return -1;
    const auto* api=SiN_GetServerMapEntityApi(1,sizeof(sin_port::native_map::Api));
    sin_port::native_map::NavigationResult result{};
    return api&&api->navigation_input(a,n,text,b.logic_navigation,&result)&&result.completed&&result.handled?1:0;
}
constexpr NativeInputMap counter_map{0x224fb558u,0x224f0350u,15,counter_inputs,8};
int native_counter_input(void* a,unsigned n,unsigned handler,const ServerEntityInputData& data,const ServerEntityAcceptInputBindings& b) noexcept {
    if(!b.math_counter)return -1;
    const char* text=nullptr;
    for(const auto& row:counter_inputs)if(row.handler==handler){text=row.text;break;}
    if(!text)return -1;
    const auto* api=SiN_GetServerMapEntityApi(1,sizeof(sin_port::native_map::Api));
    sin_port::native_map::CounterResult result{};
    return api&&api->counter_input(a,n,text,data.activator,rd<unsigned>(data.variant,0x10),rd<float>(data.variant,0),b.math_counter,&result)&&result.completed&&result.handled?1:0;
}
constexpr NativeInputMap compare_map{0x224fb588u,0x224f0350u,10,compare_inputs,4};
int native_compare_input(void* a,unsigned n,unsigned handler,const ServerEntityInputData& data,const ServerEntityAcceptInputBindings& b) noexcept {
    if(!b.logic_compare)return -1;
    const char* text=nullptr;
    for(const auto& row:compare_inputs)if(row.handler==handler){text=row.text;break;}
    if(!text)return -1;
    const auto* api=SiN_GetServerMapEntityApi(1,sizeof(sin_port::native_map::Api));
    sin_port::native_map::CompareResult result{};
    return api&&api->compare_input(a,n,text,data.activator,rd<unsigned>(data.variant,0x10),rd<float>(data.variant,0),b.logic_compare,&result)&&result.completed&&result.handled?1:0;
}
constexpr NativeInputMap case_map{0x224fb570u,0x224f0350u,35,case_inputs,2};
int native_case_input(void* a,unsigned n,unsigned handler,const ServerEntityInputData& data,const ServerEntityAcceptInputBindings& b) noexcept {
    if(!b.logic_case)return -1;
    const char* text=nullptr;
    for(const auto& row:case_inputs)if(row.handler==handler){text=row.text;break;}
    if(!text)return -1;
    const auto* api=SiN_GetServerMapEntityApi(1,sizeof(sin_port::native_map::Api));
    sin_port::native_map::CaseResult result{};
    auto bound=*b.logic_case;bound.entries=b.entities.entries;bound.entry_count=b.entities.count;
    if(b.search&&b.search->extent){bound.entity_context=b.search->context;bound.entity_extent=b.search->extent;}
    return api&&api->case_input(a,n,text,data.activator,data.variant,sizeof(data.variant),&bound,&result)&&result.completed&&result.handled?1:0;
}
constexpr NativeInputMap branch_map{0x224fb5a0u,0x224f0350u,8,branch_inputs,5};
int native_branch_input(void* a,unsigned n,unsigned handler,const ServerEntityInputData& data,const ServerEntityAcceptInputBindings& b) noexcept {
    if(!b.branch)return -1;
    const char* text=nullptr;
    for(const auto& row:branch_inputs)if(row.handler==handler&&handler){text=row.text;break;}
    if(!text)return -1;
    const auto* api=SiN_GetServerMapEntityApi(1,sizeof(sin_port::native_map::Api));
    sin_port::native_map::BranchResult result{};
    return api&&api->branch_input(a,n,text,data.activator,rd<unsigned>(data.variant,0x10),rd<unsigned char>(data.variant,0),b.branch,&result)&&result.completed&&result.handled?1:0;
}
constexpr NativeInputMap timer_map{0x224fb4c8u,0x224f0350u,14,timer_inputs,8};
int native_timer_input(void* a,unsigned n,unsigned handler,const ServerEntityInputData& data,const ServerEntityAcceptInputBindings& b) noexcept {
    if(!b.timer)return -1;
    const char* text=nullptr;
    for(const auto& row:timer_inputs)if(row.handler==handler){text=row.text;break;}
    if(!text)return -1;
    const auto* api=SiN_GetServerMapEntityApi(1,sizeof(sin_port::native_map::Api));
    sin_port::native_map::TimerResult result{};
    return api&&api->timer_input(a,n,text,rd<unsigned>(data.variant,0x10),rd<float>(data.variant,0),b.timer,&result)&&result.completed&&result.handled?1:0;
}
constexpr NativeInputMap relay_map{0x224fda70u,0x224f0350u,9,relay_inputs,6};
int native_relay_input(void* a,unsigned n,unsigned handler,const ServerEntityInputData& data,const ServerEntityAcceptInputBindings& b) noexcept {
    if(!b.relay)return -1;
    const char* text=nullptr;
    for(const auto& row:relay_inputs)if(row.handler==handler){text=row.text;break;}
    if(!text)return -1;
    const auto* api=SiN_GetServerMapEntityApi(1,sizeof(sin_port::native_map::Api));
    sin_port::native_map::RelayResult result{};
    return api&&api->relay_input(a,n,text,data.activator,b.relay,&result)&&result.completed&&result.handled?1:0;
}
constexpr NativeInputMap resupply_map{0x22528b58u,0x224f0350u,6,resupply_inputs,3};
int native_resupply_input(void* a,unsigned n,unsigned handler,const ServerEntityAcceptInputBindings& b,ServerEntityAcceptInputResult& o) noexcept {
    using namespace sin_port::native_resupply;
    if(!b.resupply)return -1;
    if(handler!=0x222f8520u&&handler!=0x222f9120u&&handler!=0x222f8f00u)return -1;
    const auto* api=SiN_GetServerDynamicResupplyApi(1,sizeof(Api));if(!api)return 0;
    const auto& s=*b.resupply;
    if(handler==0x222f9120u){
        if(!s.dispatch||!s.dispatch->evaluation)return 0;
        auto dispatch=*s.dispatch;
        auto evaluation=*dispatch.evaluation;
        evaluation.entries=b.entities.entries;evaluation.entry_count=b.entities.count;
        if(s.lifecycle&&s.lifecycle->master_handle)evaluation.master_handle=s.lifecycle->master_handle;
        dispatch.evaluation=&evaluation;
        DispatchResult local{};auto* result=s.dispatch_result?s.dispatch_result:&local;
        if(!api->calculate_type(a,n,s.debug,&dispatch,result))return 0;
    }else{
        if(!s.lifecycle)return 0;
        auto lifecycle=*s.lifecycle;lifecycle.entries=b.entities.entries;lifecycle.entry_count=b.entities.count;
        LifecycleResult local{};auto* result=s.lifecycle_result?s.lifecycle_result:&local;
        if(!(handler==0x222f8520u?api->kill(a,n,&lifecycle,result):api->become_master(a,n,&lifecycle,result)))return 0;
    }
    o.resupply_handler=1;return 1;
}
const NativeInputMap* native_input_map(unsigned token) noexcept {
    for(const auto& m:npc_input_maps)if(m.token==token)return &m;
    return nullptr;
}
int __cdecl wake_transition(void*,unsigned,std::uint32_t,bool,const ServerNpcWakeTransitionBindings*,ServerNpcWakeTransitionResult*) noexcept;
// Original logic_auto (8 rows) and logic_menulistener (16 rows) have
// no derived input descriptors. All thirteen maps parent CBaseEntity.
constexpr NativeInputMap auto_kill_map{0x224fb270u,0x224f0350u,8,nullptr,0};
constexpr NativeInputMap menu_kill_map{0x224fb5e8u,0x224f0350u,16,nullptr,0};
const NativeInputMap* shared_kill_map(unsigned vt) noexcept {
    using namespace sin_port::native_map;
    switch(vt){
    case relay_vtable:return &relay_map;
    case auto_vtable:return &auto_kill_map;
    case timer_vtable:return &timer_map;
    case branch_vtable:return &branch_map;
    case case_vtable:return &case_map;
    case compare_vtable:return &compare_map;
    case counter_vtable:return &counter_map;
    case menu_vtable:return &menu_kill_map;
    case navigation_vtable:return &navigation_map;
    case autosave_vtable:return &autosave_map;
    case multicompare_vtable:return &multicompare_map;
    case remap_vtable:return &remap_map;
    case colorblend_vtable:return &colorblend_map;
    default:return nullptr;
    }
}
int __cdecl accept_input(void* a,unsigned n,std::uint32_t vt,std::uint32_t input,void* activator,void* caller,const void* variant,unsigned vn,std::uint32_t id,const ServerEntityAcceptInputBindings* b,ServerEntityAcceptInputResult* o) noexcept {
    if(!o)return 0;*o={};
    const int shared_kill=b?logic_kill_requested(vt,input,*b):0;
    if(shared_kill<0)return 0;
    const auto* kill_map=shared_kill?shared_kill_map(vt):nullptr;
    if(shared_kill&&(!kill_map||!a||n<sin_port::native_map::logic_kill_extent(vt)||rd<unsigned>(a,0)!=vt))return 0;
    unsigned token=kill_map?kill_map->token:
        vt==sin_port::native_map::colorblend_vtable&&b&&b->math_colorblend?colorblend_map.token:
        vt==sin_port::native_map::remap_vtable&&b&&b->math_remap?remap_map.token:
        vt==sin_port::native_map::multicompare_vtable&&b&&b->logic_multicompare?multicompare_map.token:
        vt==sin_port::native_map::autosave_vtable&&b&&b->logic_autosave?autosave_map.token:
        vt==sin_port::native_map::navigation_vtable&&b&&b->logic_navigation?navigation_map.token:
        vt==sin_port::native_map::counter_vtable&&b&&b->math_counter?counter_map.token:
        vt==sin_port::native_map::compare_vtable&&b&&b->logic_compare?compare_map.token:
        vt==sin_port::native_map::case_vtable&&b&&b->logic_case?case_map.token:
        vt==sin_port::native_map::branch_vtable&&b&&b->branch?branch_map.token:
        vt==sin_port::native_map::timer_vtable&&b&&b->timer?timer_map.token:
        vt==sin_port::native_map::relay_vtable&&b&&b->relay?relay_map.token:
        vt==sin_port::native_resupply::resupply_vtable&&b&&b->resupply?resupply_map.token:input_datamap(vt),seen[32]{};
    if(vt==sin_port::native_map::relay_vtable&&n<sin_port::native_map::relay_extent)return 0;
    if(vt==sin_port::native_map::timer_vtable&&(!a||n<sin_port::native_map::timer_extent||rd<unsigned>(a,0)!=vt))return 0;
    if(vt==sin_port::native_map::branch_vtable&&(!a||n<sin_port::native_map::branch_extent||rd<unsigned>(a,0)!=vt))return 0;
    if(vt==sin_port::native_map::case_vtable&&(!a||n<sin_port::native_map::case_extent||rd<unsigned>(a,0)!=vt))return 0;
    if(vt==sin_port::native_map::compare_vtable&&(!a||n<sin_port::native_map::compare_extent||rd<unsigned>(a,0)!=vt))return 0;
    if(vt==sin_port::native_map::counter_vtable&&(!a||n<sin_port::native_map::counter_extent||rd<unsigned>(a,0)!=vt))return 0;
    if(vt==sin_port::native_map::navigation_vtable&&(!a||n<sin_port::native_map::navigation_extent||rd<unsigned>(a,0)!=vt||rd<unsigned>(a,0x394)!=sin_port::native_map::navigation_listener_vtable))return 0;
    if(vt==sin_port::native_map::autosave_vtable&&(!a||n<sin_port::native_map::autosave_extent||rd<unsigned>(a,0)!=vt))return 0;
    if(vt==sin_port::native_map::multicompare_vtable&&(!a||n<sin_port::native_map::multicompare_extent||rd<unsigned>(a,0)!=vt))return 0;
    if(vt==sin_port::native_map::remap_vtable&&(!a||n<sin_port::native_map::remap_extent||rd<unsigned>(a,0)!=vt))return 0;
    if(vt==sin_port::native_map::colorblend_vtable&&(!a||n<sin_port::native_map::colorblend_extent||rd<unsigned>(a,0)!=vt))return 0;
    if(!a||n<0x204||!token||!variant||vn<0x14||!b||!b->storage||(!b->name_equal&&!b->string_text))return 0;
    if(vt==sin_port::native_map::timer_vtable&&b->timer&&b->timer->cadence){
        const auto& t=*b->timer;
        if(t.entry_count>sin_independent::kEntityHandleCapacity||(!t.entries&&t.entry_count)||
           sin_independent::EntityHandleView(t.entries,t.entry_count).Resolve(rd<unsigned>(a,0x378))!=a)return 0;
    }
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
        // Project only the authenticated inherited Kill descriptor.
        // Other inherited inputs retain owner storage.
        bool base_kill=false;
        const bool native_kill=shared_kill||(vt==sin_port::native_map::autosave_vtable&&b->logic_autosave&&(b->logic_autosave->remove||b->logic_autosave->deferred))||
            (vt==sin_port::native_map::multicompare_vtable&&b->logic_multicompare&&sin_port::native_map::kill_available(b->logic_multicompare->kill));
        if(token==logic_base_kill_map.token&&native_kill){
            if(shared_kill)base_kill=true;
            else if(b->name_equal){
                if(!input_name_equal(*b,0x2240b624u,input,base_kill))return 0;
            }else if(input==0x2240b624u)base_kill=true;
            else{const char* actual=nullptr;if(!input_text(*b,input,&actual))return 0;base_kill=search_equal("Kill",actual);}
        }
        const auto* native=base_kill?&logic_base_kill_map:
            kill_map&&token==kill_map->token?kill_map:
            token==colorblend_map.token&&b->math_colorblend?&colorblend_map:
            token==remap_map.token&&b->math_remap?&remap_map:
            token==multicompare_map.token&&b->logic_multicompare?&multicompare_map:
            token==autosave_map.token&&b->logic_autosave?&autosave_map:
            token==navigation_map.token&&b->logic_navigation?&navigation_map:
            token==counter_map.token&&b->math_counter?&counter_map:
            token==compare_map.token&&b->logic_compare?&compare_map:
            token==case_map.token&&b->logic_case?&case_map:
            token==branch_map.token&&b->branch?&branch_map:
            token==timer_map.token&&b->timer?&timer_map:
            token==relay_map.token&&b->relay?&relay_map:
            token==resupply_map.token&&b->resupply?&resupply_map:b->native_npc_datamaps?native_input_map(token):nullptr;
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
                wr<unsigned>(projected,8,descriptor->offset);
                wr<std::uint16_t>(projected,0x12,static_cast<std::uint16_t>(descriptor->flags));
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
                if(o->actual_type==2u&&o->expected_type==1u&&!b->convert){
                    // Decode before handler dispatch; preserve all other
                    // variant words. Verify the actual converted type.
                    const char* text=nullptr;float value=0.f;
                    if(!input_text(*b,rd<unsigned>(data.variant,0),&text)||!input_decimal_float(text,value))return 0;
                    const unsigned float_type=1;
                    std::memcpy(data.variant,&value,sizeof(value));
                    std::memcpy(data.variant+0x10,&float_type,sizeof(float_type));
                    o->actual_type=rd<unsigned>(data.variant,0x10);
                    if(o->actual_type!=float_type)return 0;
                    o->converted=1;
                }else{
                    bool ok=false;if(!input_convert(data.variant,o->expected_type,*b,ok))return 0;
                    if(!ok)return rejected(EntityInputBadConversion);
                    o->converted=1;o->actual_type=rd<unsigned>(data.variant,0x10);
                }
            }
            if(o->handler==0x220320b0u&&b->wake){
                // InputWake replaces the inputdata stack argument with true,
                // then tail-jumps to the authenticated +4d8 wake override.
                auto bound=*b->wake;bound.storage=b->storage;bound.entities=b->entities;bound.output_now=b->now;
                ServerNpcWakeTransitionResult local{};
                auto* result=b->wake_result?b->wake_result:&local;
                if(!wake_transition(a,n,vt,true,&bound,result))return 0;
                o->native_wake=1;
            }else if(o->handler){
                int shared_status=-1;
                if(shared_kill&&base_kill&&o->handler==0x2213cdc0u){
                    const auto* api=SiN_GetServerMapEntityApi(1,sizeof(sin_port::native_map::Api));
                    sin_port::native_map::EntityKillResult result{};
                    shared_status=api&&api->logic_kill(a,n,vt,b->entities.entries,b->entities.count,b->logic_kill,&result)&&result.completed&&result.handled?1:0;
                }
                const int status=shared_status>=0?shared_status:
                    vt==sin_port::native_map::colorblend_vtable?native_colorblend_input(a,n,o->handler,data,*b):
                    vt==sin_port::native_map::remap_vtable?native_remap_input(a,n,o->handler,data,*b):
                    vt==sin_port::native_map::multicompare_vtable?native_multicompare_input(a,n,o->handler,data,*b):
                    vt==sin_port::native_map::autosave_vtable?native_autosave_input(a,n,o->handler,*b):
                    vt==sin_port::native_map::navigation_vtable?native_navigation_input(a,n,o->handler,*b):
                    vt==sin_port::native_map::counter_vtable?native_counter_input(a,n,o->handler,data,*b):
                    vt==sin_port::native_map::compare_vtable?native_compare_input(a,n,o->handler,data,*b):
                    vt==sin_port::native_map::case_vtable?native_case_input(a,n,o->handler,data,*b):
                    vt==sin_port::native_map::branch_vtable?native_branch_input(a,n,o->handler,data,*b):
                    vt==sin_port::native_map::timer_vtable?native_timer_input(a,n,o->handler,data,*b):
                    vt==sin_port::native_map::relay_vtable?native_relay_input(a,n,o->handler,data,*b):
                    vt==sin_port::native_resupply::resupply_vtable?native_resupply_input(a,n,o->handler,*b,*o):b->native_npc_handlers?native_npc_input(a,n,vt,o->handler,data,*b,*o):-1;
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
        }else if(!registry.Resolve(rd<unsigned>(a,0xfe4))&&!registry.Resolve(rd<unsigned>(a,0xfe8))){
            if(!b.fresh_thrust||!fresh_jet_thrust(a,n,b.entities,b.storage,*b.fresh_thrust,o.thrust))return false;
        }
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
        else if(b.output_queue){
            ServerEntityVoidOutputResult result{};
            if(!fire_actor_output_queue(a,n,0xbc8u,b.output_now,b.entities,*b.output_queue,result))return false;
            ++o.output_queue_calls;o.output_queue_actions+=result.queued;o.output_queue_removed+=result.removed;
        }else if(rd<unsigned>(a,0xbdc))return false;
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
bool native_outer_wake(void* a,unsigned n,std::uint32_t vt,bool pvs,float now,const ServerNpcActiveWeaponStepBindings& base,const OuterWakeRoute& route) noexcept {
    auto policy=route.bindings.policy;
    auto transition=route.bindings.transition;
    const auto& selection=base.schedule->scheduler.maintenance.selection;
    policy.conditions=selection.grunt.schedule.conditions;
    transition.entities=selection.state.entities;
    transition.storage=route.bindings.outer.post;transition.output_now=now;
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

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcJetpackPrimaryThinkApi=_SiN_GetServerNpcJetpackPrimaryThinkApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcJetpackPrimaryThinkApi* __cdecl SiN_GetServerNpcJetpackPrimaryThinkApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcJetpackPrimaryThinkApi)?&jet_primary_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerResupplyPrimaryThinkApi=_SiN_GetServerResupplyPrimaryThinkApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerResupplyPrimaryThinkApi* __cdecl SiN_GetServerResupplyPrimaryThinkApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerResupplyPrimaryThinkApi)?&resupply_primary_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerAntigenPrimaryThinkApi=_SiN_GetServerAntigenPrimaryThinkApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerAntigenPrimaryThinkApi* __cdecl SiN_GetServerAntigenPrimaryThinkApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerAntigenPrimaryThinkApi)?&antigen_primary_api:nullptr;
}
