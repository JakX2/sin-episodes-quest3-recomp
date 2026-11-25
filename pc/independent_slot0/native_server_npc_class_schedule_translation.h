#pragma once
#include "native_server_npc_schedule_maintenance.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_class_schedule_translation_export[]="SiN_GetServerNpcClassScheduleTranslationApi";
struct ServerNpcClassScheduleTranslationBindings {
    ServerNpcConditionMapBindings conditions;
    ServerWeaponActivityBindings entities;
    void* context;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *parent)(void*,void*,int,int*) noexcept;
    int (__cdecl *rules_int)(void*,int,bool*) noexcept;
    int (__cdecl *rules_float)(void*,float,bool*) noexcept;
    int (__cdecl *random)(void*,int,int,int*) noexcept;
    int (__cdecl *squad_slot)(void*,void*,int,bool*) noexcept;
    int (__cdecl *actor_gate)(void*,void*,unsigned,int,bool*) noexcept;
    int (__cdecl *cue)(void*,void*,unsigned,int,int) noexcept;
    int (__cdecl *enemy_notice)(void*,void*,void*) noexcept;
    int (__cdecl *enemy_capability)(void*,void*,bool*,unsigned*) noexcept;
    int (__cdecl *weapon_capability)(void*,void*,unsigned*) noexcept;
    int (__cdecl *is_player)(void*,void*,bool*) noexcept;
    // Original distance/helper leaves. No movement reconstruction.
    int (__cdecl *distance)(void*,void*,void*,bool,double*) noexcept;
    int (__cdecl *crouch_probe)(void*,void*,void*,void*,bool*) noexcept;
    int (__cdecl *burst)(void*,void*,int*) noexcept;
    int (__cdecl *interval)(void*,void*,double*) noexcept;
    int (__cdecl *pending)(void*,void*,int) noexcept;
    int (__cdecl *dirty)(void*,void*,std::uint32_t) noexcept;
};
struct ServerNpcClassScheduleTranslationResult {
    int input,output;
    unsigned subclass,steps,condition_tests,random_calls,parent_calls,custom_calls,cue_calls,pending_calls,dirty_calls;
};
struct ServerNpcClassScheduleTranslationApi {
    std::uint32_t version,size;
    int (__cdecl *translate)(void*,unsigned,std::uint32_t,int,float,const ServerNpcClassScheduleTranslationBindings*,ServerNpcClassScheduleTranslationResult*) noexcept;
};
using GetServerNpcClassScheduleTranslationApi=const ServerNpcClassScheduleTranslationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
