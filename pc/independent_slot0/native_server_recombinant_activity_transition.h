#pragma once
#include "native_server_recombinant_registration.h"
#include "native_server_activity_transition.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_activity_transition_export[]="SiN_GetServerRecombinantActivityTransitionApi";
// Original Major/Minor virtual +4e4. Host owns inherited activity playback,
// sound and shared Source scalar setters; this class only reconstructs SiN
// choice of values, ordering and the Major attack latch.
struct ServerRecombinantActivityTransitionBindings {
    const ServerRecombinantPolicyBindings* policy;
    const ServerRecombinantActivityRegistration* major_activities;
    void* context;
    int (__cdecl *random_float)(void*,float,float,float*) noexcept;
    int (__cdecl *set_frenzy_scalar)(void*,void*,float) noexcept; // original 22191a70
    int (__cdecl *set_minor_scalar)(void*,void*,float) noexcept; // original 220405c0
};
struct ServerRecombinantActivityTransitionResult {
    unsigned completed,kind,frenzy,sound_calls,random_calls,scalar_calls,latch_written,latch_cleared;
    int activity,previous;
    float low,high,draw;
    unsigned char latch_a,latch_b;
};
struct ServerRecombinantActivityTransitionApi {
    unsigned version,size;
    int (__cdecl *major)(void*,unsigned,int,const ServerRecombinantActivityTransitionBindings*,ServerRecombinantActivityTransitionResult*) noexcept;
    int (__cdecl *minor)(void*,unsigned,int,const ServerRecombinantActivityTransitionBindings*,ServerRecombinantActivityTransitionResult*) noexcept;
};
using GetServerRecombinantActivityTransitionApi=const ServerRecombinantActivityTransitionApi* (__cdecl *)(unsigned,unsigned) noexcept;

inline constexpr char server_recombinant_activity_assignment_export[]="SiN_GetServerRecombinantActivityAssignmentApi";
// Synchronous composition of the existing activity assignment and the
// authenticated Major/Minor +4e4 override. Other callbacks keep their owners.
// The original current activity is still visible during the override.
struct ServerRecombinantActivityAssignmentBindings {
    const ServerNpcActivityAssignmentBindings* assignment;
    const ServerRecombinantActivityTransitionBindings* transition;
};
struct ServerRecombinantActivityAssignmentResult {
    ServerNpcActivityAssignmentResult assignment;
    ServerRecombinantActivityTransitionResult transition;
    unsigned transition_calls;
};
struct ServerRecombinantActivityAssignmentApi {
    unsigned version,size;
    int (__cdecl *assign)(void*,unsigned,const ServerNpcActivityAssignmentInput*,const ServerRecombinantActivityAssignmentBindings*,ServerRecombinantActivityAssignmentResult*) noexcept;
    int (__cdecl *request)(void*,unsigned,int,const ServerRecombinantActivityAssignmentBindings*,ServerRecombinantActivityAssignmentResult*) noexcept;
};
using GetServerRecombinantActivityAssignmentApi=const ServerRecombinantActivityAssignmentApi* (__cdecl *)(unsigned,unsigned) noexcept;

inline constexpr char server_recombinant_pending_activity_export[]="SiN_GetServerRecombinantPendingActivityApi";
// Compose the shared pending activity lifecycle with the SiN-specific
// class override, without modifying either existing ABI.
struct ServerRecombinantPendingActivityBindings {
    const ServerNpcTransitionBindings* pending;
    const ServerRecombinantActivityTransitionBindings* activity;
};
struct ServerRecombinantPendingActivityResult {
    ServerNpcTransitionResult pending;
    ServerRecombinantActivityTransitionResult activity;
    unsigned transition_calls;
};
struct ServerRecombinantPendingActivityApi {
    unsigned version,size;
    int (__cdecl *set_pending)(void*,unsigned,int,const ServerRecombinantPendingActivityBindings*,ServerRecombinantPendingActivityResult*) noexcept;
    int (__cdecl *update)(void*,unsigned,const ServerRecombinantPendingActivityBindings*,ServerRecombinantPendingActivityResult*) noexcept;
};
using GetServerRecombinantPendingActivityApi=const ServerRecombinantPendingActivityApi* (__cdecl *)(unsigned,unsigned) noexcept;

inline constexpr char server_recombinant_task_activity_export[]="SiN_GetServerRecombinantTaskActivityApi";
// Original Major StartTask 54/55 requests an activity through 22049c20.
// Compose the native pending lifecycle at that call site; other task
// services retain their original owners and context.
struct ServerRecombinantTaskActivityBindings {
    const ServerRecombinantTaskBindings* task;
    const ServerRecombinantPendingActivityBindings* pending;
};
struct ServerRecombinantTaskActivityResult {
    ServerRecombinantTaskResult task;
    ServerRecombinantPendingActivityResult pending;
    unsigned pending_calls;
};
struct ServerRecombinantTaskActivityApi {
    unsigned version,size;
    int (__cdecl *major)(void*,unsigned,const ServerNpcScheduleTask*,bool,float,
        const ServerRecombinantTaskActivityBindings*,ServerRecombinantTaskActivityResult*) noexcept;
};
using GetServerRecombinantTaskActivityApi=const ServerRecombinantTaskActivityApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
