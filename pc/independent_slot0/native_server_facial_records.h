#pragma once
#include "native_npc_scene_playback.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_facial_records_export[]="SiN_GetServerFacialRecordApi";
// Same config ABI, but for verified original base-flex/actor admission tables.
// Ordinary speech start/check/stop are native; forced speech stop delegates host.stop
// for the sound effect (its accepted value is ignored, as original AL is true).
inline constexpr char server_retail_facial_records_export[]="SiN_GetServerRetailFacialRecordApi";
struct ServerFacialRecordBindings {
    ActorEventRecordBindings host;
    bool base_flex,npc_random;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *now)(void*,float*) noexcept;
};
struct ServerFacialRecordApi {
    std::uint32_t version,size;
    // Caller-owned configuration must outlive the adapted record bindings.
    // Compatible with NativeSceneExecutionBindings dispatch/leaves.records.
    int (__cdecl *adapt)(const ServerFacialRecordBindings*,ActorEventRecordBindings*) noexcept;
};
using GetServerFacialRecordApi=const ServerFacialRecordApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
