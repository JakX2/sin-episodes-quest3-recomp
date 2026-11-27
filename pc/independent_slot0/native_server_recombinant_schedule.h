#pragma once
#include "native_server_recombinant_dispatch.h"
#include "native_server_npc_behavior_selection.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_schedule_export[]="SiN_GetServerRecombinantScheduleApi";
struct ServerRecombinantScheduleTask {int id;float parameter;const char* activity;bool schedule_reference;};
struct ServerRecombinantScheduleDefinition {
    unsigned kind;int local;const char* name;unsigned source_va,parser_call;
    const ServerRecombinantScheduleTask* tasks;unsigned task_count;
    const int* conditions;unsigned condition_count;
};
struct ServerRecombinantScheduleRow {
    unsigned descriptor[13];
    unsigned offset;
    const ServerRecombinantScheduleDefinition* source;
};
// Caller owns stable nonoverlapping descriptor tokens [base,base+35].
// Fallback descriptor is externally owned; no inherited schedules copied.
struct ServerRecombinantScheduleImage {
    unsigned kind,base,head,fallback,rows,task_count,generation;
    const ServerActivityDictionary* activities;
    ServerRecombinantScheduleRow entries[12];
    ServerNpcScheduleTask tasks[64];
};
struct ServerRecombinantScheduleBindings {
    ServerNpcConditionMapBindings namespaces;
    const ServerActivityDictionary* activities;
};
struct ServerRecombinantScheduleResult {
    unsigned rows,tasks,interrupts,private_activities;
    int failed_schedule,failed_task,failed_condition;
};
struct ServerRecombinantScheduleApi {
    unsigned version,size;
    int (__cdecl *catalog)(const ServerRecombinantScheduleDefinition**,unsigned*) noexcept;
    int (__cdecl *build)(void*,unsigned,unsigned,unsigned,const ServerRecombinantScheduleBindings*,ServerRecombinantScheduleImage*,ServerRecombinantScheduleResult*) noexcept;
    int (__cdecl *storage)(const ServerRecombinantScheduleImage*,unsigned,const void**,unsigned*) noexcept;
    int (__cdecl *find)(const ServerRecombinantScheduleImage*,int,unsigned*) noexcept;
};
using GetServerRecombinantScheduleApi=const ServerRecombinantScheduleApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
