#pragma once
#include "native_server_recombinant_schedule.h"
#include "native_server_recombinant_preschedule.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_session_export[]="SiN_GetServerRecombinantSessionApi";
// The host retains ordinary Source MaintainSchedule, validity, state
// changes, task-status transitions, activity clocks and timing.
// This binds only the mutant virtual overrides and descriptor storage.
struct ServerRecombinantSessionBindings {
    const ServerRecombinantScheduleImage* image;
    const ServerRecombinantDispatchBindings* dispatch;
    const ServerRecombinantPrescheduleBindings* major_preschedule;
    // Storage/reset for the inherited schedule chain; no original pointer calls.
    ServerNpcScheduleBindings inherited;
    void* context;
    int (__cdecl *inherited_lookup)(void*,void*,int,unsigned*) noexcept;
};
struct ServerRecombinantSession {
    void* actor;
    unsigned bytes,kind;
    const ServerRecombinantSessionBindings* bindings;
    ServerNpcScheduleBindings schedule;
};
struct ServerRecombinantSessionResult {
    unsigned completed,kind,owned,delegated,token;
    int requested,translated,global;
    ServerRecombinantDecisionResult decision;
    ServerRecombinantMinorResult minor;
    ServerRecombinantPolicyResult policy;
    ServerRecombinantLaunchResult failure;
    ServerRecombinantPrescheduleResult preschedule;
    ServerNpcScheduleStateResult state;
    ServerRecombinantDispatchResult task;
};
struct ServerRecombinantSessionApi {
    unsigned version,size;
    // Caller owns nonmoving session and all nested bindings.
    int (__cdecl *bind)(void*,unsigned,const ServerRecombinantSessionBindings*,ServerRecombinantSession*) noexcept;
    int (__cdecl *select)(ServerRecombinantSession*,ServerRecombinantSessionResult*) noexcept; // +434 only
    int (__cdecl *fail_select)(ServerRecombinantSession*,int,int,int,ServerRecombinantSessionResult*) noexcept; // +438 only
    int (__cdecl *resolve)(ServerRecombinantSession*,int,ServerRecombinantSessionResult*) noexcept; // +43c and lookup
    int (__cdecl *gather)(ServerRecombinantSession*,ServerRecombinantSessionResult*) noexcept; // +420
    int (__cdecl *preschedule)(ServerRecombinantSession*,float,ServerRecombinantSessionResult*) noexcept; // +424
    int (__cdecl *interrupts)(ServerRecombinantSession*,ServerRecombinantSessionResult*) noexcept; // +490
    int (__cdecl *current_task)(ServerRecombinantSession*,bool,float,ServerRecombinantSessionResult*) noexcept; // +444/+44c
};
using GetServerRecombinantSessionApi=const ServerRecombinantSessionApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
