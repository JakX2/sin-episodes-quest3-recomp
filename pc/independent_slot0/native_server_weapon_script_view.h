#pragma once
#include "native_server_weapon_event_route.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_weapon_script_view_export[]="SiN_GetServerWeaponScriptViewApi";
struct ServerWeaponScriptView {
    const void* rows;unsigned bytes,count; // PE32 rows, stride 16, info token +12
    const void* fallback;unsigned fallback_bytes;
    void* context;
    int (__cdecl *storage)(void*,std::uint32_t,const void**,unsigned*) noexcept;
};
struct ServerWeaponScriptRecord {const void* data;unsigned bytes,used_default;};
struct ServerWeaponScriptViewApi {
    std::uint32_t version,size;
    // 222b5bd0: sentinel or out-of-range selects default, not failure.
    int (__cdecl *resolve)(const ServerWeaponScriptView*,std::uint16_t,ServerWeaponScriptRecord*) noexcept;
    int (__cdecl *weapon)(const ServerWeaponScriptView*,const void*,unsigned,ServerWeaponScriptRecord*) noexcept;
    // Verified common virtuals +404 through +440 and base +4a0.
    int (__cdecl *value)(const ServerWeaponScriptView*,const void*,unsigned,unsigned,int*) noexcept;
    // Original class +3ac float constant, returned promoted to double.
    int (__cdecl *interval)(unsigned,double*) noexcept;
    // +4f8 for the three verified classes, not a clip-capacity getter.
    int (__cdecl *burst)(const void*,unsigned,unsigned,int*) noexcept;
};
using GetServerWeaponScriptViewApi=const ServerWeaponScriptViewApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
