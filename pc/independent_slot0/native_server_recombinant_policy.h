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
}
