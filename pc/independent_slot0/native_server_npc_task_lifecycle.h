#pragma once
#include "native_server_npc_condition.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_task_lifecycle_export[]="SiN_GetServerNpcTaskLifecycleApi";
struct ServerNpcTaskLifecycleBindings {
    ServerNpcConditionMapBindings conditions;
    void* context;
    // 220b1b40 random source, same call order (integer then float).
    int (__cdecl *random_int)(void*,unsigned,unsigned,unsigned*) noexcept;
    int (__cdecl *random_float)(void*,float,float,double*) noexcept;
    int (__cdecl *cleanup)(void*,void* actor) noexcept; // +6d4
    bool record_failure;
    int (__cdecl *failure_name)(void*,int,std::uint32_t*) noexcept;
};
struct ServerNpcTaskLifecycleResult {unsigned cleanup,randomized,completed,failed,recorded;int status;};
struct ServerNpcTaskLifecycleApi {
    std::uint32_t version,size;
    int (__cdecl *complete)(void*,unsigned,float,bool,const ServerNpcTaskLifecycleBindings*,ServerNpcTaskLifecycleResult*) noexcept;
    int (__cdecl *fail)(void*,unsigned,float,int,const ServerNpcTaskLifecycleBindings*,ServerNpcTaskLifecycleResult*) noexcept;
    int (__cdecl *animation_finished)(const void*,unsigned,unsigned*) noexcept;
};
using GetServerNpcTaskLifecycleApi=const ServerNpcTaskLifecycleApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
