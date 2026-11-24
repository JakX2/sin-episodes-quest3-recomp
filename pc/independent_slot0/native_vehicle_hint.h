#pragma once
#include "native_weapon_datatables.h"
namespace sin_port::native_weapon_tables {
inline constexpr char vehicle_hint_export[]="SiN_GetVehicleHintApi";
inline constexpr char hint_hud_export[]="SiN_GetHintHudApi";
inline constexpr char hint_message_name[]="HintText";
struct VehicleHintBindings {
    void* context;
    int (__cdecl *resolve)(void*,std::uint32_t,void**) noexcept;
    int (__cdecl *participant)(void*,void* player,unsigned*) noexcept;
    int (__cdecl *send)(void*,void* player,unsigned reliable,const char* name,const unsigned char*,unsigned bytes) noexcept;
    int (__cdecl *register_message)(void*,const char*,int bytes) noexcept;
};
struct VehicleHintApi {
    std::uint32_t version,size;
    int (__cdecl *enable)(HologramControl*,unsigned type,std::uint32_t word) noexcept;
    // Borrowed post-entry or pre-exit hint slice; not passenger physics.
    int (__cdecl *notify)(void* vehicle,unsigned bytes,const VehicleHintBindings*,int seat,unsigned entering) noexcept;
    int (__cdecl *encode)(const char* text,const float* duration,unsigned char*,unsigned capacity,unsigned* used) noexcept;
    int (__cdecl *register_message)(const VehicleHintBindings*) noexcept;
};
using GetVehicleHintApi=const VehicleHintApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
struct HintHudBindings {
    void* context;
    // Original 241517b0: clear old label children, retain raw token,
    // localize and expand bindings. UI host owns these operations.
    // built=0 means no localized display, not transport failure.
    int (__cdecl *rebuild)(void*,void* hud,const char* token,unsigned* built) noexcept;
    int (__cdecl *visible)(void*,void* hud,unsigned visible) noexcept;
    int (__cdecl *animation)(void*,const char*) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *register_message)(void*,const char*,int bytes) noexcept;
    int (__cdecl *find_hud)(void*,const char*,HologramControl**) noexcept;
};
struct HintHudApi {
    std::uint32_t version,size;
    int (__cdecl *receive)(HologramControl*,const HintHudBindings*,const unsigned char*,unsigned bytes) noexcept;
    int (__cdecl *tick)(HologramControl*,const HintHudBindings*) noexcept;
    int (__cdecl *register_message)(const HintHudBindings*) noexcept;
    int (__cdecl *route)(const HintHudBindings*,const unsigned char*,unsigned bytes) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetHintHudApi=const HintHudApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
