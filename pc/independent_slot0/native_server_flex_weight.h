#pragma once
#include <cstdint>
namespace sin_port::native_weapon_tables {
inline constexpr char server_flex_weight_export[]="SiN_GetServerFlexWeightApi";
inline constexpr unsigned server_flex_weight_offset=0x4a8,server_flex_weight_count=64;
inline constexpr unsigned server_flex_extent=0x5b8;
struct ServerFlexRange {float minimum,maximum;};
struct ServerFlexWeightBindings {
    void* context;
    // Resolve the original PE32 edict pointer token, OR flags with 1.
    int (__cdecl *dirty_edict)(void*,void*,std::uint32_t) noexcept;
};
struct ServerFlexWeightApi {
    std::uint32_t version,size;
    // Original 220f6f10 and 220f6fb0. Out-of-range index / absent studio
    // are successful no-op / zero, respectively.
    int (__cdecl *set)(void*,unsigned,const ServerFlexRange*,unsigned,int,float,const ServerFlexWeightBindings*) noexcept;
    int (__cdecl *get)(const void*,unsigned,const ServerFlexRange*,unsigned,int,float*) noexcept;
};
using GetServerFlexWeightApi=const ServerFlexWeightApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
