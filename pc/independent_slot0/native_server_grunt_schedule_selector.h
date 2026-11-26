#pragma once
#include "native_server_npc_behavior_selection.h"
#include "native_server_activity_translate.h"
#include "native_server_weapon_activity.h"
#include "native_server_npc_state_reconsider.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_grunt_schedule_selector_export[]="SiN_GetServerGruntScheduleSelectorApi";
struct ServerGruntScheduleSound {std::uint32_t token,flags,entity_handle;};
struct ServerGruntScheduleSelectorBindings {
    ServerNpcScheduleBindings schedule;
    ServerNpcBehaviorBindings behavior;
    ServerWeaponActivityBindings entities;
    void* context;
    // Original 2204e670's virtual activity lookup, then actual weighted choice.
    int (__cdecl *flinch_activity)(void*,void*,int*) noexcept;
    const ServerNpcActivityTranslateBindings* sequences;
    // +504(mask) and original sound->entity resolver/RTTI cast.
    int (__cdecl *sound)(void*,void*,unsigned,ServerGruntScheduleSound*) noexcept;
    int (__cdecl *grenade_owner)(void*,void* grenade,void**) noexcept;
    int (__cdecl *relation)(void*,void*,void*,int*) noexcept;
    int (__cdecl *kick_eligible)(void*,void*,bool*) noexcept;
    // Original 220a35c0, cue enum 0 danger/1 grenade (verified strings).
    int (__cdecl *cue)(void*,void*,unsigned,int,int) noexcept;
    int (__cdecl *sound_target)(void*,void*,std::uint32_t sound_token) noexcept;
    // 22358570 has two native writes to linked helper actor.
    int (__cdecl *mutable_storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // +848/+844 remain class-specific combat decision services.
    int (__cdecl *combat_special)(void*,void*,bool*) noexcept;
    int (__cdecl *combat_schedule)(void*,void*,int*) noexcept;
    // 2206b220 / +2fc / 2213c760 query leaves, no path reconstruction.
    int (__cdecl *investigate)(void*,void*,bool*) noexcept;
    int (__cdecl *squad_gate)(void*,void*,int,bool*) noexcept;
    int (__cdecl *parent)(void*,void*,int*) noexcept;
};
struct ServerGruntScheduleSelectorResult {
    int schedule,flinch_sequence;
    unsigned condition_tests,condition_clears,helper_cleared,flinch_probed,sound_queried,kick_transferred,parent_called;
    ServerNpcBehaviorResult behavior;
};
struct ServerGruntScheduleSelectorApi {
    std::uint32_t version,size;
    // 2235e4e0. Ground Grunt variants; not Jetpack +434 override.
    int (__cdecl *select)(void*,unsigned,const ServerGruntScheduleSelectorBindings*,ServerGruntScheduleSelectorResult*) noexcept;
};
using GetServerGruntScheduleSelectorApi=const ServerGruntScheduleSelectorApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_grunt_native_slot_selector_export[]="SiN_GetServerGruntNativeSlotSelectorApi";
struct ServerGruntNativeSlotSelectorResult {
    ServerGruntScheduleSelectorResult selection;
    ServerNpcGruntSquadSlotsResult slots;
    unsigned slot_calls;
};
struct ServerGruntNativeSlotSelectorApi {
    std::uint32_t version,size;
    int (__cdecl *select)(void*,unsigned,float,const ServerGruntScheduleSelectorBindings*,const ServerNpcGruntSquadSlotsBindings*,ServerGruntNativeSlotSelectorResult*) noexcept;
};
using GetServerGruntNativeSlotSelectorApi=const ServerGruntNativeSlotSelectorApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
