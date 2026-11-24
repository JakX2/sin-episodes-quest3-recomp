#pragma once
#include "native_weapon_datatables.h"
namespace sin_port::native_weapon_tables {
inline constexpr char companion_reactions_export[]="SiN_GetCompanionReactionApi";
inline constexpr unsigned jc_bytes=0x1000;
struct JcProfileBindings {
    void* context;
    int (__cdecl *health_dirty)(void*,void* npc) noexcept;
    int (__cdecl *npc_initialize)(void*,void* npc) noexcept;
};
struct JessicaReactionBindings {
    void* context;
    int (__cdecl *base_update)(void*,void* npc) noexcept;
    int (__cdecl *resolve)(void*,std::uint32_t,void**) noexcept;
    // Existing original collision/capability restoration and subsequent
    // base helper, in their exact order; not reimplemented here.
    int (__cdecl *restore_unparented)(void*,void* npc) noexcept;
    int (__cdecl *post_restore)(void*,void* npc) noexcept;
    int (__cdecl *condition)(void*,void* npc,unsigned condition,unsigned*) noexcept;
    // Reuse response dispatch; original speech result is ignored.
    int (__cdecl *speak)(void*,void* npc,const char* concept_name,unsigned arg1,unsigned arg2) noexcept;
    // Runtime event identifier comes from host registration, not a fixed
    // guessed enum. Original matching event consumes regardless of speech.
    int (__cdecl *speech_ready)(void*,void* npc,unsigned*) noexcept;
    int (__cdecl *base_event)(void*,void* npc,int id,std::uint32_t arg1,std::uint32_t arg2,unsigned*) noexcept;
    int (__cdecl *base_schedule)(void*,void* npc,int requested,int*) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *secondary_timestamp)(void*,void* npc,float*) noexcept;
};
struct CompanionReactionApi {
    std::uint32_t version,size;
    // JC spawn tail after all ordinary engine/model setup.
    int (__cdecl *jc_profile)(HologramControl*,const JcProfileBindings*) noexcept;
    int (__cdecl *jessica_update)(HologramControl*,const JessicaReactionBindings*) noexcept;
    int (__cdecl *jessica_event)(HologramControl*,const JessicaReactionBindings*,int registered_id,int event_id,std::uint32_t arg1,std::uint32_t arg2,unsigned* handled) noexcept;
    int (__cdecl *jessica_schedule)(HologramControl*,const JessicaReactionBindings*,int requested,int* result) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetCompanionReactionApi=const CompanionReactionApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
