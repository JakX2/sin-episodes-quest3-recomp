#pragma once
#include "native_server_grunt_task_dispatch.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_jetpack_task_export[]="SiN_GetServerJetpackTaskApi";
enum ServerJetpackTaskRoute : unsigned {JetpackTaskBase,JetpackTaskNative,JetpackTaskHosted,JetpackTaskAttachment};
struct ServerJetpackTaskBindings {
    ServerGruntTaskBindings grunt;
    void* context;
    // Original actor/attachment world-dependent leaves; do not emulate paths.
    int (__cdecl *special)(void*,void*,const ServerNpcScheduleTask*) noexcept; // 25/166
    int (__cdecl *attachment_task)(void*,void*,void* attachment,const ServerNpcScheduleTask*) noexcept; // 9/10/40/43
    int (__cdecl *face_enemy)(void*,void*,void* attachment,void* enemy) noexcept; // +770
    int (__cdecl *inspect_clear)(void*,void*,float) noexcept; // 2204f5d0 after clearing target
    float default_inspect[3]; // resolved original globals 225885b8..c0
};
struct ServerJetpackTaskResult {
    unsigned route,attachment_live,enemy_live,delegated,mode_changed,timer_written,target_cleared;
    ServerGruntTaskResult grunt;
    ServerNpcTaskLifecycleResult lifecycle;
};
struct ServerJetpackTaskApi {
    std::uint32_t version,size;
    // Original 22364280 / 223633b0. All four Jetpack subclasses.
    int (__cdecl *route)(int,bool,unsigned*) noexcept;
    int (__cdecl *execute)(void*,unsigned,const ServerNpcScheduleTask*,bool,float,const ServerJetpackTaskBindings*,ServerJetpackTaskResult*) noexcept;
};
using GetServerJetpackTaskApi=const ServerJetpackTaskApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
