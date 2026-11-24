#pragma once
#include "native_weapon_datatables.h"
namespace sin_port::native_weapon_tables {
inline constexpr char radio_wire_export[]="SiN_GetRadioWireApi";
inline constexpr char radio_hud_export[]="SiN_GetRadioHudApi";
inline constexpr char radio_message_name[]="PoliceRadioMessage";
struct RadioWireApi {
    std::uint32_t version,size;
    // Original two signed 16-bit writes (active narrowed to uint8,
    // priority narrowed to int16), then NUL-terminated string.
    // Buffer begins at message payload, not the engine message header.
    int (__cdecl *encode)(unsigned active,int priority,const char* speaker,unsigned char* payload,unsigned capacity,unsigned* used) noexcept;
};
using GetRadioWireApi=const RadioWireApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
struct RadioHudBindings {
    void* context;
    // Host allocates a stable NUL-terminated copy, maintaining the
    // original string lifetime. Previous pointer isn't freed by receiver.
    int (__cdecl *store_speaker)(void*,const char*,const char**) noexcept;
    int (__cdecl *panel_flag)(void*,void* panel,unsigned virtual_offset,unsigned value) noexcept;
    // Transport success, not ignored animation sequence-start result.
    int (__cdecl *animation)(void*,const char* sequence) noexcept;
};
struct RadioIconRequest {char name[64];unsigned present;float width,height;};
struct RadioHudApi {
    std::uint32_t version,size;
    int (__cdecl *initialize)(HologramControl*) noexcept;
    int (__cdecl *reset)(HologramControl*) noexcept;
    int (__cdecl *receive)(HologramControl*,const RadioHudBindings*,const unsigned char*,unsigned bytes) noexcept;
    // Original paint resource selection only, not rendering or loading.
    int (__cdecl *icon)(HologramControl*,RadioIconRequest*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetRadioHudApi=const RadioHudApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
