#pragma once
#include "native_server_recombinant_minor.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_minor_touch_export[]="SiN_GetServerRecombinantMinorTouchApi";
inline constexpr unsigned recombinant_minor_contact_marker=0x2237c980u;
inline constexpr unsigned recombinant_minor_detached_contact_marker=0x2237ca30u;
inline constexpr unsigned recombinant_default_think_marker=0x22044fa0u;
struct ServerRecombinantMinorTouchBindings {
    const ServerRecombinantMinorBindings* minor;
    void* context;
    int (__cdecl *entity_handle)(void*,void*,unsigned*) noexcept; // original virtual +8
    int (__cdecl *release_player_object)(void*,void*) noexcept; // 22256b20 terminal local-player call
    int (__cdecl *set_movement)(void*,void*,int,int) noexcept; // 22143610
    int (__cdecl *install_think)(void*,void*,unsigned,int,int) noexcept; // opaque marker
};
struct ServerRecombinantMinorTouchResult {
    unsigned completed,marker,attached_cleared,object_checks,object_released,target_written,
        condition_writes,schedule_tests,velocity_set,movement_set,think_installed,think_scheduled,
        touch_cleared,cooldown_blocked,solid_blocked,grounded,detached;
    unsigned target_handle,final_marker;
    ServerRecombinantMinorResult detach;
};
struct ServerRecombinantMinorTouchApi {
    unsigned version,size;
    int (__cdecl *touch)(void*,unsigned,void*,unsigned,float,const ServerRecombinantMinorTouchBindings*,ServerRecombinantMinorTouchResult*) noexcept;
};
using GetServerRecombinantMinorTouchApi=const ServerRecombinantMinorTouchApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
