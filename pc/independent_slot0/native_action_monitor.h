#pragma once
#include "native_weapon_datatables.h"
namespace sin_port::native_weapon_tables {
inline constexpr char action_monitor_export[]="SiN_GetActionMonitorApi";
// Original 0x158-byte entity observation, with fixed-width name/class
// strings and six engine-supplied transform words. No live pointers.
struct ActionEntitySample {
    char name[256],classname[64];
    float transform[6];
};
static_assert(sizeof(ActionEntitySample)==0x158);
struct ActionMonitorBindings {
    void* context;
    // Resolve previous target and owner by serial-validated packed handle.
    int (__cdecl *resolve)(void*,std::uint32_t,void**) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *packed_handle)(void*,void* entity,std::uint32_t*) noexcept;
    // Host captures existing entity names and transforms, not traces.
    int (__cdecl *sample)(void*,void* entity,ActionEntitySample*) noexcept;
};
struct ActionLookSample {
    void* target;
    std::uint32_t detail; // original third look-record scalar at +2e0
    const char* part; // null clears only first byte of 32-byte part
};
struct ActionCriteria {char text[512];};
struct ActionMonitorApi {
    std::uint32_t version,size;
    // Complete monitor storage >=0x5e9; no vtable/enable flag construction.
    int (__cdecl *update_look)(HologramControl*,const ActionMonitorBindings*,const ActionLookSample*) noexcept;
    // Original CLookAction/CUseAction record pointer, not monitor base.
    // Format independently of live response-system parser.
    int (__cdecl *look_criteria)(const void*,unsigned,float,ActionCriteria*) noexcept;
    int (__cdecl *use_criteria)(const void*,unsigned,ActionCriteria*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetActionMonitorApi=const ActionMonitorApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
