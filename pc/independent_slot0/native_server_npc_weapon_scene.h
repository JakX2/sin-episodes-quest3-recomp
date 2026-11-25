#pragma once
#include <cstdint>
#include "native_server_facial_records.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_weapon_scene_export[]="SiN_GetServerNpcWeaponSceneApi";
struct ServerNpcWeaponSceneBindings {
    void* context;
    // Resolved weapon identities (0 means absent); actual engine handle services remain external.
    int (__cdecl *active)(void*,void* actor,std::uint32_t*) noexcept;
    int (__cdecl *weapon_at)(void*,void* actor,unsigned slot,std::uint32_t*) noexcept;
    int (__cdecl *gesture)(void*,void* actor,int activity,int autokill,int* layer) noexcept;
    int (__cdecl *select)(void*,void* actor,std::uint32_t weapon) noexcept;
    int (__cdecl *storage)(void*,std::uint32_t token,void**,unsigned*) noexcept;
    int (__cdecl *duration)(void*,void* actor,int sequence,double*) noexcept;
};
struct ServerNpcWeaponSceneApi {
    std::uint32_t version,size;
    // Recognized type16 holster/unholster: 1; unrelated: 0; invalid/host failure: -1.
    int (__cdecl *begin)(void* actor,unsigned,void* record,unsigned,const void* event,unsigned,const ServerNpcWeaponSceneBindings*) noexcept;
    // Subtype2/3 completion policy (out is original AL); same status convention.
    int (__cdecl *check)(void* actor,unsigned,const void* record,unsigned,const void* event,unsigned,float time,const ServerNpcWeaponSceneBindings*,unsigned*) noexcept;
    // Original per-frame and stop virtuals return true without mutation for subtype2/3.
    int (__cdecl *noop)(const void* record,unsigned,const void* event,unsigned,unsigned*) noexcept;
};
using GetServerNpcWeaponSceneApi=const ServerNpcWeaponSceneApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_scene_records_export[]="SiN_GetServerNpcSceneRecordApi";
struct ServerNpcSceneRecordBindings {
    ServerFacialRecordBindings facial;
    ServerNpcWeaponSceneBindings weapons;
};
struct ServerNpcSceneRecordApi {
    std::uint32_t version,size;
    // Caller-owned NPC config; native weapon commands plus retail facial policy.
    int (__cdecl *adapt)(const ServerNpcSceneRecordBindings*,ActorEventRecordBindings*) noexcept;
};
using GetServerNpcSceneRecordApi=const ServerNpcSceneRecordApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
