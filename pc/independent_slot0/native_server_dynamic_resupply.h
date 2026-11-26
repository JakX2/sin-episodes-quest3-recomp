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
inline constexpr char health_classname[]="item_healthvial";
struct HealthBindings {
    void* context;
    int (__cdecl *vial_value)(void*,float*) noexcept;
    int (__cdecl *scale)(void*,float*) noexcept;
};
struct HealthResult {unsigned vial_reads,scale_reads,completed;};
struct FallbackBindings {
    void* context;
    int (__cdecl *index)(void*,const char*,int*) noexcept;
    int (__cdecl *has_weapon)(void*,int,bool*) noexcept;
    int (__cdecl *random_float)(void*,float,float,float*) noexcept;
};
struct FallbackResult {
    unsigned enabled,index_reads,ownership_reads,eligible,forced_magnum,random_called;
    float cumulative[count],total,draw;
    int selected;
    const char* classname;
};
struct Api {
    std::uint32_t version,size;
    // 222f8580: six desired ratios and pre-counted nearby pickup rows.
    int (__cdecl *ammo_ratios)(const float*,unsigned,AmmoRow*,unsigned,const Bindings*,Result*) noexcept;
    // 222f8aa0: health (+9c), max health (+98), desired (+394).
    int (__cdecl *health_ratio)(int,int,float,AmmoRow*,const HealthBindings*,HealthResult*) noexcept;
    // First strictly positive maximum, ties keep the earlier table row.
    int (__cdecl *greatest_deficit)(const AmmoRow*,unsigned,int*) noexcept;
    // 222f87a0: fallback selection, before original spawn helper.
    int (__cdecl *fallback)(unsigned,const float*,unsigned,const FallbackBindings*,FallbackResult*) noexcept;
};
using GetApi=const Api* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
