#pragma once
#include "native_server_npc_schedule_catalog.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_schedule_image_export[]="SiN_GetServerNpcScheduleImageApi";
struct ServerScheduleImageRow {
    std::uint32_t descriptor[13]; // original +0 global,+4 tasks,+8 count,+c mask,+2c name,+30 next
    unsigned task_offset;
    const ServerScheduleCatalogEntry* source;
};
struct ServerScheduleImage {
    std::uint32_t token_base,head,fallback_head;
    unsigned family,row_count,task_count;
    const ServerActivityDictionary* activity_owner;
    unsigned activity_generation;
    ServerScheduleImageRow rows[46];
    ServerNpcScheduleTask tasks[270];
};
struct ServerScheduleImageBindings {
    ServerNpcConditionMapBindings namespace_map;
    const ServerGruntRegistration* registration;
    void* context;
    // Bare original token retained in Phase1073; caller must explicitly
    // resolve its observed numeric-parser behavior, not infer a damage flag.
    int (__cdecl *damage_token)(void*,const char*,float*) noexcept;
};
struct ServerScheduleImageResult {unsigned rows,tasks,interrupts,private_activities,damage_tokens;int failed_schedule,failed_task,failed_condition;};
struct ServerNpcScheduleImageApi {
    std::uint32_t version,size;
    // Caller owns nonoverlapping token interval [base,base+137].
    // Build is transactional; old output remains unchanged on failure.
    int (__cdecl *build)(void*,unsigned,std::uint32_t,std::uint32_t,const ServerScheduleImageBindings*,ServerScheduleImage*,ServerScheduleImageResult*) noexcept;
    int (__cdecl *storage)(const ServerScheduleImage*,std::uint32_t,const void**,unsigned*) noexcept;
};
using GetServerNpcScheduleImageApi=const ServerNpcScheduleImageApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
