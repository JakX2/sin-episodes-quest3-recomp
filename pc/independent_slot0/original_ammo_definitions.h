#pragma once
// Shared original 2006 SiN client/server ammo registration metadata.
// Original x86 registration arguments, independently decoded from both
// DLLs. No engine pointer, native CAmmoDef layout, or raw PE bytes.
#include <cstdint>
#include <cstddef>
#if defined(_MSC_VER)
#define SIN_AMMO_CALL __cdecl
#else
#define SIN_AMMO_CALL
#endif
namespace sin_port::original_ammo {
inline constexpr char export_name[]="SiN_GetOriginalAmmoDefinitionApi";
inline constexpr std::uint32_t version=1, count=17;
struct Definition {
    char name[16],pickup_sound[32];
    std::uint32_t damage_type;
    std::int32_t tracer_type,player_damage;
    std::uint32_t npc_damage_bits;
    std::int32_t max_carry;
    std::uint32_t impulse_bits,extra_float_bits;
    std::int32_t flags,min_splash,max_splash;
};
static_assert(sizeof(Definition)==88);
struct Api {
    std::uint32_t version,size;
    std::uint32_t (SIN_AMMO_CALL *definition_count)() noexcept;
    std::int32_t (SIN_AMMO_CALL *index)(const char*) noexcept;
    int (SIN_AMMO_CALL *definition)(std::int32_t,Definition*) noexcept;
    std::int32_t (SIN_AMMO_CALL *max_carry)(std::int32_t) noexcept;
};
using GetApi=const Api* (SIN_AMMO_CALL *)(std::uint32_t,std::uint32_t) noexcept;
// Both module builds compile the exact same source. The server ammo
// service uses this lookup if a host does not override the definition.
std::int32_t original_max_carry(std::int32_t index) noexcept;
}
