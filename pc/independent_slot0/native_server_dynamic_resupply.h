#pragma once
#include <cstdint>
namespace sin_port::native_resupply {
inline constexpr char export_name[]="SiN_GetServerDynamicResupplyApi";
inline constexpr unsigned count=6;
struct AmmoType {const char* classname;const char* name;int units;float full_weight;};
inline constexpr AmmoType ammo_types[count]{
    {"item_ammo_magnum","MAG",10,.5f},
    {"item_ammo_ar","AR",30,.5f},
    {"item_ammo_buckshot","BUCK",10,.5f},
    {"item_ammo_ar_grenade","ARNADE",1,.5f},
    {"item_ammo_flak","FLAK",1,.5f},
    {"item_ammo_duag","DUAG",5,.5f}
};
struct AmmoRow {float desired,current,delta;std::int32_t nearby;};
static_assert(sizeof(AmmoRow)==16);
struct Bindings {
    void* context;
    int (__cdecl *index)(void*,const char*,int*) noexcept;
    int (__cdecl *has_weapon)(void*,int,bool*) noexcept;
    int (__cdecl *max_carry)(void*,int,int*) noexcept;
    int (__cdecl *reserve)(void*,int,int*) noexcept;
    int (__cdecl *scale)(void*,float*) noexcept;
};
struct Result {unsigned rows,owned,index_native,max_native,reserve_reads,scale_reads;};
struct Api {
    std::uint32_t version,size;
    // 222f8580: six desired ratios and pre-counted nearby pickup rows.
    int (__cdecl *ammo_ratios)(const float*,unsigned,AmmoRow*,unsigned,const Bindings*,Result*) noexcept;
};
using GetApi=const Api* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
