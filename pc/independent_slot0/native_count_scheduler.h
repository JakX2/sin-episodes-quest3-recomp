#pragma once
#include "native_count_interrogation.h"
namespace sin_port::native_weapon_tables {
inline constexpr char count_scheduler_export[]="SiN_GetCountSchedulerApi";
// Original server-global byte 2254c0a0, shared across actor selection.
// Busy is an adapter guard, not part of the original object layout.
struct CountSchedulerState {unsigned busy=0;unsigned char gathering=0;};
struct CountSchedulerBindings {
    void* context;
    // Vector insertion 22055670; original storage at +c1c/+c28.
    int (__cdecl *append)(void*,void* npc,std::uint32_t behavior) noexcept;
    // Existing base registration 22055890 owns its two standard behaviors.
    int (__cdecl *register_base)(void*,void* npc) noexcept;
    int (__cdecl *eligible)(void*,std::uint32_t behavior,unsigned*) noexcept;
    int (__cdecl *begin)(void*,std::uint32_t behavior) noexcept;
    int (__cdecl *gather)(void*,std::uint32_t behavior) noexcept;
    int (__cdecl *end)(void*,std::uint32_t behavior) noexcept;
    int (__cdecl *reset)(void*,void* npc) noexcept;
    int (__cdecl *changed)(void*,void* npc,std::uint32_t prior,std::uint32_t selected) noexcept;
    int (__cdecl *schedule)(void*,std::uint32_t behavior,int*) noexcept;
    int (__cdecl *is_local)(void*,std::uint32_t behavior,int schedule,unsigned*) noexcept;
    // Original namespace lookup via behavior +2c then 22073700.
    int (__cdecl *translate)(void*,std::uint32_t behavior,int schedule,int*) noexcept;
    int (__cdecl *base_schedule)(void*,void* npc,int*) noexcept;
    int (__cdecl *observe_other)(void*,std::uint32_t behavior,const void*,unsigned bytes) noexcept;
};
struct CountSchedulerApi {
    std::uint32_t version,size;
    int (__cdecl *register_interrogation)(HologramControl*,const CountSchedulerBindings*) noexcept;
    int (__cdecl *select)(HologramControl*,CountSchedulerState*,const CountSchedulerBindings*,int*) noexcept;
    // Original Count primary observation vslot; embedded receiver is
    // the already reconstructed CountInterrogationApi implementation.
    int (__cdecl *observe)(HologramControl*,const CountSchedulerBindings*,const CountBindings*,const void*,unsigned bytes) noexcept;
};
using GetCountSchedulerApi=const CountSchedulerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
