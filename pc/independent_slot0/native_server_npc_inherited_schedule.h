#pragma once
#include "native_server_npc_class_schedule_translation.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_inherited_schedule_export[]="SiN_GetServerNpcInheritedScheduleApi";
struct ServerNpcInheritedScheduleBindings {
    ServerWeaponActivityBindings entities;
    void* context;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *reset_script)(void*,void*) noexcept;
    int (__cdecl *component_descriptor)(void*,std::uint32_t,const void**,unsigned*) noexcept;
    int (__cdecl *storage)(void*,std::uint32_t,const void**,unsigned*) noexcept;
    int (__cdecl *component_translate)(void*,std::uint32_t,int,int*) noexcept;
    int (__cdecl *random)(void*,int,int,int*) noexcept;
};
struct ServerNpcInheritedScheduleResult {int input,output,component_local,phase;unsigned component,controller_live,reset;};
struct ServerNpcInheritedScheduleApi {
    std::uint32_t version,size;
    int (__cdecl *translate)(void*,unsigned,int,const ServerNpcInheritedScheduleBindings*,ServerNpcInheritedScheduleResult*) noexcept;
    int (__cdecl *burst_range)(unsigned,int*,int*) noexcept;
    int (__cdecl *burst)(void*,const ServerNpcInheritedScheduleBindings*,int*) noexcept;
};
using GetServerNpcInheritedScheduleApi=const ServerNpcInheritedScheduleApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_class_schedule_services_export[]="SiN_GetServerNpcClassScheduleServicesApi";
struct ServerNpcClassScheduleServicesResult {
    ServerNpcClassScheduleTranslationResult translation;
    ServerNpcInheritedScheduleResult inherited;
    unsigned inherited_calls,burst_calls,interval_calls;
};
struct ServerNpcClassScheduleServicesApi {
    std::uint32_t version,size;
    int (__cdecl *translate)(void*,unsigned,std::uint32_t,int,float,const ServerNpcClassScheduleTranslationBindings*,const ServerNpcInheritedScheduleBindings*,ServerNpcClassScheduleServicesResult*) noexcept;
};
using GetServerNpcClassScheduleServicesApi=const ServerNpcClassScheduleServicesApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
