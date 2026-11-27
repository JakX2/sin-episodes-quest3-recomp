#pragma once
#include "native_server_npc_condition.h"
#include "native_server_activity_dictionary.h"
#include "native_server_weapon_activity.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_policy_export[]="SiN_GetServerRecombinantPolicyApi";
inline constexpr unsigned recombinant_major_vtable=0x224a0d8c, recombinant_minor_vtable=0x224a2974;
enum RecombinantKind:unsigned {RecombinantMajor=1,RecombinantMinor=2};
struct ServerRecombinantPolicyBindings {
    void* context;
    ServerNpcConditionMapBindings conditions;
    const sin_independent::EntityHandleEntry* entries;
    unsigned entry_count;
    const ServerActivityDictionary* activities;
    // Inherited Source services remain engine-owned.
    int (__cdecl *schedule_matches)(void*,void*,int,bool,bool*) noexcept;
    int (__cdecl *parent_schedule)(void*,void*,int,int*) noexcept;
    int (__cdecl *parent_activity)(void*,void*,int,int*) noexcept;
    int (__cdecl *random)(void*,int,int,int*) noexcept;
    int (__cdecl *sound)(void*,void*,const char*,int,int) noexcept;
};
struct ServerRecombinantPolicyResult {
    unsigned completed,kind,condition_tests,schedule_tests,delegated,weapon_live,custom_activity,random_calls,sound_calls,suppressed;
    int input,output,random_value;
    float yaw;
};
struct ServerRecombinantPolicyApi {
    unsigned version,size;
    // Original virtual +43c / +338 / +5e0 / +598.
    int (__cdecl *schedule)(void*,unsigned,int,const ServerRecombinantPolicyBindings*,ServerRecombinantPolicyResult*) noexcept;
    int (__cdecl *activity)(void*,unsigned,int,const ServerRecombinantPolicyBindings*,ServerRecombinantPolicyResult*) noexcept;
    int (__cdecl *yaw)(void*,unsigned,const ServerRecombinantPolicyBindings*,ServerRecombinantPolicyResult*) noexcept;
    int (__cdecl *pain)(void*,unsigned,const ServerRecombinantPolicyBindings*,ServerRecombinantPolicyResult*) noexcept;
};
using GetServerRecombinantPolicyApi=const ServerRecombinantPolicyApi* (__cdecl *)(unsigned,unsigned) noexcept;

inline constexpr char server_recombinant_major_scale_export[]="SiN_GetServerRecombinantMajorAltitudeScaleApi";
// Original Major virtual +0x5d8 (0x22377150): transform an in/out scalar.
// Two engine-provided vector arguments are used only when mode == 2.
struct ServerRecombinantMajorScaleResult {
    unsigned completed,modified;
    int mode;
    float input,output,factor,height_delta;
};
struct ServerRecombinantMajorScaleApi {
    unsigned version,size;
    int (__cdecl *scale)(const void*,unsigned,int,const float[3],const float[3],
        float*,ServerRecombinantMajorScaleResult*) noexcept;
};
using GetServerRecombinantMajorScaleApi=const ServerRecombinantMajorScaleApi* (__cdecl *)(unsigned,unsigned) noexcept;


inline constexpr char server_recombinant_fail_schedule_export[]="SiN_GetServerRecombinantFailScheduleApi";
// Authenticated class-specific virtual +0x438. The inherited generic
// failure scheduler and the Minor's collision-based clearance probe stay
// host-owned; only SiN's conditional routing and Major handle reset are here.
struct ServerRecombinantFailScheduleBindings {
    const ServerRecombinantPolicyBindings* policy;
    void* context;
    int (__cdecl *ground_clear)(void*,void*,bool*) noexcept; // original 2237c190
    int (__cdecl *parent)(void*,void*,int,int,int,int*) noexcept; // inherited 2204e6d0
};
struct ServerRecombinantFailScheduleResult {
    unsigned completed,kind,condition_tests,ground_probes,delegated,handle_cleared;
    int input,reason,arg3,output;
    unsigned condition_met,ground_clear;
};
struct ServerRecombinantFailScheduleApi {
    unsigned version,size;
    int (__cdecl *evaluate)(void*,unsigned,int,int,int,
        const ServerRecombinantFailScheduleBindings*,ServerRecombinantFailScheduleResult*) noexcept;
};
using GetServerRecombinantFailScheduleApi=const ServerRecombinantFailScheduleApi* (__cdecl *)(unsigned,unsigned) noexcept;

}
