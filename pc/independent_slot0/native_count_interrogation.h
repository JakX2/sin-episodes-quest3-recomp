#pragma once
#include "native_weapon_datatables.h"
namespace sin_port::native_weapon_tables {
inline constexpr char count_export[]="SiN_GetCountInterrogationApi";
struct CountCriteria {alignas(4) unsigned char bytes[0x200];};
struct CountBindings {
    void* context;
    int (__cdecl *resolve)(void*,std::uint32_t,void**) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    // NPC vslot +5bc. The original re-queries the response expeditor.
    int (__cdecl *expeditor)(void*,void* npc,void**) noexcept;
    int (__cdecl *speaking)(void*,void* expeditor,unsigned*) noexcept;
    int (__cdecl *criteria)(void*,const void* record,float,CountCriteria*) noexcept;
    // Transport success distinct from the original ignored speech result.
    int (__cdecl *speak)(void*,void* expeditor,const char*,const CountCriteria*,unsigned* played) noexcept;
    // Host validates the original active behavior pointer before dispatch.
    int (__cdecl *behavior_damage)(void*,std::uint32_t behavior,const PlayerDamageInfo*,int*) noexcept;
    int (__cdecl *base_damage)(void*,void* npc,const PlayerDamageInfo*,int*) noexcept;
};
struct CountInterrogationApi {
    std::uint32_t version,size;
    // Only behavior constructor scalar leaves; owner/vtable remain host-owned.
    int (__cdecl *initialize)(HologramControl*) noexcept;
    int (__cdecl *input)(HologramControl*,unsigned start) noexcept;
    int (__cdecl *enabled)(HologramControl*,unsigned*) noexcept;
    // Called by the engine's active behavior dispatch, not by the map input.
    // The original receiver does not re-test the enable byte here.
    int (__cdecl *observe)(HologramControl*,const CountBindings*,const void* observation,unsigned bytes) noexcept;
    int (__cdecl *take_damage)(HologramControl*,const CountBindings*,const PlayerDamageInfo*,int*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetCountInterrogationApi=const CountInterrogationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
