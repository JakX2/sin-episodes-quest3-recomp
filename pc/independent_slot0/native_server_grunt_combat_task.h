#pragma once
#include "native_server_npc_inherited_schedule.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_grunt_combat_task_export[]="SiN_GetServerGruntCombatTaskApi";
struct ServerGruntCombatTaskBindings {
    ServerGruntTaskBindings grunt;
    ServerNpcInheritedScheduleBindings weapon;
    void* context;
    float* shared_smg_stamp; // original process-wide 2259c7fc
    int (__cdecl *is_player)(void*,void*,bool*) noexcept;
    int (__cdecl *squad_count)(void*,std::uint32_t,int*) noexcept;
    int (__cdecl *distance)(void*,void*,void*,double*) noexcept;
    int (__cdecl *dirty)(void*,void*,std::uint32_t) noexcept;
    int (__cdecl *activity)(void*,void*,int,bool restart) noexcept;
    // Original +304 with enemy aim point; result used by native branches.
    int (__cdecl *aim_query)(void*,void*,bool*) noexcept;
    int (__cdecl *aim_update)(void*,void*,bool) noexcept; // 2206c710 / 2206bff0
};
struct ServerGruntCombatTaskResult {
    ServerGruntTaskResult base;
    unsigned recovered,weapon_live,deadline_changed,shared_changed,aim_queries,aim_updates;
    int burst,activity;
};
struct ServerGruntCombatTaskApi {
    std::uint32_t version,size;
    int (__cdecl *supports)(int,bool) noexcept;
    int (__cdecl *execute)(void*,unsigned,const ServerNpcScheduleTask*,bool,float,const ServerGruntCombatTaskBindings*,ServerGruntCombatTaskResult*) noexcept;
};
using GetServerGruntCombatTaskApi=const ServerGruntCombatTaskApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
struct ServerGruntCombatSchedulerStats {ServerGruntCombatTaskResult last;unsigned calls;};
struct ServerGruntHelperTaskBindings;
struct ServerGruntFullTaskStats;
struct ServerNpcSharedTaskBindings;
struct ServerNpcSharedTaskStats;
struct ServerNpcSharedAttackBindings;
struct ServerNpcSharedAttackStats;
struct ServerNpcSharedFacingBindings;
struct ServerNpcSharedFacingStats;
struct ServerNpcSharedTargetAnimationBindings;
struct ServerNpcSharedTargetAnimationStats;
struct ServerNpcSharedMovementBindings;
struct ServerNpcSharedMovementStats;
struct ServerNpcSharedCueBindings;
struct ServerNpcSharedCueStats;
struct ServerNpcSharedObservationBindings;
struct ServerNpcSharedObservationStats;
struct ServerNpcSharedSoundCoverBindings;
struct ServerNpcSharedSoundCoverStats;
struct ServerNpcScheduleServicesBindings;
struct ServerNpcScheduleServicesResult;
// Internal composition, same DLL only; no change to existing exported ABI.
int run_subclass_combat_internal(void*,unsigned,float,const ServerNpcSubclassSchedulerBindings*,ServerNpcSubclassSchedulerResult*,const ServerGruntCombatTaskBindings*,ServerGruntCombatSchedulerStats*,const ServerGruntHelperTaskBindings* = nullptr,ServerGruntFullTaskStats* = nullptr,const ServerNpcSharedTaskBindings* = nullptr,ServerNpcSharedTaskStats* = nullptr,const ServerNpcSharedAttackBindings* = nullptr,ServerNpcSharedAttackStats* = nullptr,const ServerNpcSharedFacingBindings* = nullptr,ServerNpcSharedFacingStats* = nullptr,const ServerNpcSharedTargetAnimationBindings* = nullptr,ServerNpcSharedTargetAnimationStats* = nullptr,const ServerNpcSharedMovementBindings* = nullptr,ServerNpcSharedMovementStats* = nullptr,const ServerNpcSharedCueBindings* = nullptr,ServerNpcSharedCueStats* = nullptr,const ServerNpcSharedObservationBindings* = nullptr,ServerNpcSharedObservationStats* = nullptr,const ServerNpcSharedSoundCoverBindings* = nullptr,ServerNpcSharedSoundCoverStats* = nullptr) noexcept;
int run_schedule_services_combat_internal(void*,unsigned,float,const ServerNpcScheduleServicesBindings*,ServerNpcScheduleServicesResult*,const ServerGruntCombatTaskBindings*,ServerGruntCombatSchedulerStats*,const ServerGruntHelperTaskBindings* = nullptr,ServerGruntFullTaskStats* = nullptr,const ServerNpcSharedTaskBindings* = nullptr,ServerNpcSharedTaskStats* = nullptr,const ServerNpcSharedAttackBindings* = nullptr,ServerNpcSharedAttackStats* = nullptr,const ServerNpcSharedFacingBindings* = nullptr,ServerNpcSharedFacingStats* = nullptr,const ServerNpcSharedTargetAnimationBindings* = nullptr,ServerNpcSharedTargetAnimationStats* = nullptr,const ServerNpcSharedMovementBindings* = nullptr,ServerNpcSharedMovementStats* = nullptr,const ServerNpcSharedCueBindings* = nullptr,ServerNpcSharedCueStats* = nullptr,const ServerNpcSharedObservationBindings* = nullptr,ServerNpcSharedObservationStats* = nullptr,const ServerNpcSharedSoundCoverBindings* = nullptr,ServerNpcSharedSoundCoverStats* = nullptr) noexcept;
}
