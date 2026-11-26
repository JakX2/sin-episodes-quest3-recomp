#pragma once
#include "native_server_npc_schedule_image.h"
#include "native_server_npc_weapon_runtime.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_active_weapon_step_export[]="SiN_GetServerNpcActiveWeaponStepApi";
// Original active-update segment 220433d7..2204342a after 22040ce0 accepted.
// This is not the full 22043260 outer update or its post-animation tail.
struct ServerNpcActiveWeaponStepBindings {
    const ServerScheduleImage* image;
    const ServerNpcScheduleServicesBindings* schedule;
    const ServerGruntHelperTaskBindings* full;
    const ServerNpcSharedAttackBindings* attack;
    const ServerNpcSharedFacingBindings* facing;
    const ServerNpcSharedTargetAnimationBindings* target;
    const ServerNpcSharedMovementBindings* movement;
    const ServerNpcSharedCueBindings* cue;
    const ServerNpcSharedObservationBindings* observe;
    const ServerNpcSharedSoundCoverBindings* sound_cover;
    const ServerNpcSharedTacticalCoverBindings* tactical;
    const ServerNpcNativeCombatLineThinkBindings* think;
    const ServerNpcWeaponPolicyBindings* policy;
    const ServerNpcScriptWeaponEventFrameBindings* frame;
    void* context;
    int (__cdecl *reset_movement)(void*,void*) noexcept; // 2204d350(0,0)
    int (__cdecl *motor_activity)(void*,std::uint32_t,int) noexcept; // motor +48(-1)
};
struct ServerNpcActiveWeaponStepResult {
    ServerNpcNativeRequestThinkResult think;
    ServerNpcGruntNativeLineAttackResult outer_line,inner_line;
    ServerNpcWeaponPolicyResult outer_weapon,inner_weapon;
    ServerNpcScriptWeaponEventFrameResult animation;
    unsigned think_called,think_completed,reset_called,motor_called,animation_called,completed;
};
struct ServerNpcActiveWeaponStepApi {
    std::uint32_t version,size;
    // Grunt/Elite only. Future think deadline still advances animation.
    // All weapon resolution, script reads and condition mapping are shared.
    int (__cdecl *run)(void*,unsigned,const ServerNpcRunAnimationInput*,const ServerNpcActiveWeaponStepBindings*,ServerNpcActiveWeaponStepResult*) noexcept;
};
using GetServerNpcActiveWeaponStepApi=const ServerNpcActiveWeaponStepApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_active_jet_weapon_step_export[]="SiN_GetServerNpcActiveJetWeaponStepApi";
struct ServerNpcActiveJetWeaponStepBindings {
    ServerNpcActiveWeaponStepBindings base; // base.think must be null
    const ServerNpcNativeJetCombatLineThinkBindings* think;
};
struct ServerNpcActiveJetWeaponStepResult {
    ServerNpcNativeJetRequestThinkResult think;
    ServerNpcGruntNativeLineAttackResult outer_line,inner_line;
    ServerNpcWeaponPolicyResult outer_weapon,inner_weapon;
    ServerNpcScriptWeaponEventFrameResult animation;
    unsigned think_called,think_completed,reset_called,motor_called,animation_called,completed;
};
struct ServerNpcActiveJetWeaponStepApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const ServerNpcRunAnimationInput*,const ServerNpcActiveJetWeaponStepBindings*,ServerNpcActiveJetWeaponStepResult*) noexcept;
};
using GetServerNpcActiveJetWeaponStepApi=const ServerNpcActiveJetWeaponStepApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
