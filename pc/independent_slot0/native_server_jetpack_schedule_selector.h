#pragma once
#include "native_server_grunt_schedule_selector.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_jetpack_schedule_selector_export[]="SiN_GetServerJetpackScheduleSelectorApi";
struct ServerJetpackSelectorBindings {
    ServerGruntScheduleSelectorBindings grunt;
    void* context;
    float default_inspect[3];
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *attachment_enemy)(void*,void* attachment,void* enemy,bool) noexcept;
};
struct ServerJetpackSelectorResult {
    int schedule;
    unsigned condition_tests,enemy_live,attachment_live,enemy_sent,inspection,ground,parent,combat;
    ServerNpcBehaviorResult behavior;
    ServerGruntScheduleSelectorResult grunt;
};
struct ServerJetpackSelectorApi {
    std::uint32_t version,size;
    int (__cdecl *select)(void*,unsigned,float,const ServerJetpackSelectorBindings*,ServerJetpackSelectorResult*) noexcept;
};
using GetServerJetpackSelectorApi=const ServerJetpackSelectorApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
