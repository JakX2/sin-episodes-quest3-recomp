#include "native_server_npc_active_weapon_step.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSubclassSchedulerApi* __cdecl SiN_GetServerNpcSubclassSchedulerApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeWeaponPolicyThinkApi* __cdecl SiN_GetServerNpcNativeWeaponPolicyThinkApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScriptWeaponEventFrameApi* __cdecl SiN_GetServerNpcScriptWeaponEventFrameApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T rd(const void* a,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(a)+off,sizeof(v));return v;}
int __cdecl run(void* a,unsigned n,const ServerNpcRunAnimationInput* in,const ServerNpcActiveWeaponStepBindings* b,ServerNpcActiveWeaponStepResult* out) noexcept {
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
    if(!cls||!cls->classify(scheduler.original_vtable,&family)||family==NpcSubclassJetpack||
       n<(family==NpcSubclassElite?0xfe0u:0xfd8u)||t.actor_class!=NpcActivityGrunt||
       !g.ids||g.ids!=scheduler.grunt.registration||!g.ids->events_ready||!g.ids->activities_ready||
       !t.activities||!t.events||g.ids->activity_owner!=t.activities||g.ids->event_owner!=t.events||
       g.ids->activity_generation!=t.activities->names.generation||g.ids->event_generation!=t.events->generation||
       t.activities!=g.runtime.npc.frame.activities||t.events!=g.runtime.npc.frame.frame.dictionary.dictionary||
       t.activities!=g.runtime.weapon.animation.activities||t.events!=g.runtime.weapon.animation.events||
       t.events!=g.runtime.weapon.animation.dictionary.dictionary)return 0;
    const auto& registry=selection.state.entities;
    if(registry.count>sin_independent::kEntityHandleCapacity||(registry.count&&!registry.entries))return 0;
    const auto* frame=SiN_GetServerNpcScriptWeaponEventFrameApi(1,sizeof(ServerNpcScriptWeaponEventFrameApi));
    const auto* think=SiN_GetServerNpcNativeWeaponPolicyThinkApi(1,sizeof(ServerNpcNativeWeaponPolicyThinkApi));
    if(!frame||!think)return 0;
    const float deadline=rd<float>(a,0x864);
    if(!std::isfinite(deadline))return 0;
    // x87 comparison at 220433d7: equality is due, later deadline skips +41c.
    if(deadline<=in->now){
        if(!b->image||!b->think||!b->full||!b->attack||!b->facing||!b->target||!b->movement||
           !b->cue||!b->observe||!b->sound_cover||!b->tactical)return 0;
        out->think_called=1;
        if(!think->run(a,n,in->now,b->image,b->schedule,b->full,b->attack,b->facing,b->target,b->movement,b->cue,b->observe,b->sound_cover,b->tactical,
            b->think,&out->think,&out->outer_line,&out->inner_line,b->policy,&out->outer_weapon,&out->inner_weapon))return 0;
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
const ServerNpcActiveWeaponStepApi api{1,sizeof(ServerNpcActiveWeaponStepApi),&run};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcActiveWeaponStepApi=_SiN_GetServerNpcActiveWeaponStepApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcActiveWeaponStepApi* __cdecl SiN_GetServerNpcActiveWeaponStepApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcActiveWeaponStepApi)?&api:nullptr;
}
