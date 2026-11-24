#include "original_ammo_definitions.h"
#if defined(_MSC_VER)
#define SIN_AMMO_EXPORT __declspec(dllexport)
#if defined(_M_IX86)
#pragma comment(linker,"/EXPORT:SiN_GetOriginalAmmoDefinitionApi=_SiN_GetOriginalAmmoDefinitionApi")
#endif
#else
#define SIN_AMMO_EXPORT __attribute__((visibility("default")))
#endif
namespace sin_port::original_ammo {
namespace {
constexpr Definition rows[]={
    {"MAG","",0x2u,4,15,0x40400000u,80,0x462a66bau,0x42800000u,0,4,8},
    {"AR","",0x2u,4,7,0x40400000u,330,0x44a6fe46u,0x42000000u,0,4,8},
    {"BUCK","",0x40000002u,1,6,0x40000000u,30,0x452395d1u,0x41c00000u,0,4,8},
    {"SR","",0x80000002u,0,20,0x41200000u,30,0x46262428u,0x42800000u,0,4,8},
    {"ROCKET","",0x8u,0,100,0x42480000u,5,0x0u,0x0u,0,4,8},
    {"ARNADE","Player.PickupSecondary",0x8u,0,100,0x41c80000u,3,0x0u,0x0u,0,4,8},
    {"GRENADE","",0x8u,0,100,0x42480000u,5,0x0u,0x0u,0,4,8},
    {"FLARE","",0x8u,0,0,0x0u,30,0x0u,0x0u,0,4,8},
    {"DUAG","Player.PickupSecondary",0x400002u,2,50,0x41a00000u,8,0x471fc04eu,0x43fa0000u,0,4,8},
    {"FLAK","Player.PickupSecondary",0x2u,0,10,0x40800000u,10,0x457f9a17u,0x0u,0,4,8},
    {"CHAIN","",0x40000002u,4,2,0x40000000u,750,0x4426fe46u,0x42000000u,0,4,8},
    {"PULSE","",0x400u,0,10,0x41200000u,10,0x457f9a17u,0x0u,0,4,8},
    {"HELIGUN","",0x42u,4,3,0x40c00000u,320,0x44a6fe46u,0x42800000u,3,4,8},
    {"HELIMISL","",0x8u,0,100,0x42480000u,320,0x0u,0x0u,0,4,8},
    {"TURRET1","",0x2u,4,2,0x3f800000u,320,0x4426fe46u,0x0u,0,4,8},
    {"TURRET2","",0x2u,4,4,0x40000000u,320,0x4426fe46u,0x0u,0,4,8},
    {"TURRET3","",0x2u,4,8,0x40800000u,320,0x4426fe46u,0x0u,0,4,8},
};
static_assert(sizeof(rows)/sizeof(rows[0])==count);
char fold(char c) noexcept {return c>='A'&&c<='Z'?static_cast<char>(c-'A'+'a'):c;}
bool equal_name(const char* a,const char* b) noexcept {
    if(!a)return false;
    for(std::size_t i=0;i<sizeof(rows[0].name);++i){
        if(fold(a[i])!=fold(b[i]))return false;
        if(!b[i])return true;
    }
    return false;
}
const Definition* row(std::int32_t index) noexcept {
    return index>=1&&index<=static_cast<std::int32_t>(count)?&rows[index-1]:nullptr;
}
std::uint32_t SIN_AMMO_CALL definition_count() noexcept {return count;}
std::int32_t SIN_AMMO_CALL index_of(const char* name) noexcept {
    for(std::uint32_t i=0;i<count;++i)
        if(equal_name(name,rows[i].name))return static_cast<std::int32_t>(i+1);
    return -1;
}
int SIN_AMMO_CALL definition(std::int32_t index,Definition* out) noexcept {
    const auto* r=row(index);
    if(!r||!out)return 0;
    *out=*r;return 1;
}
std::int32_t SIN_AMMO_CALL max_carry(std::int32_t index) noexcept {
    return original_max_carry(index);
}
const Api api{version,sizeof(Api),&definition_count,&index_of,&definition,&max_carry};
}
std::int32_t original_max_carry(std::int32_t index) noexcept {
    const auto* r=row(index);
    return r?r->max_carry:0;
}
}
extern "C" SIN_AMMO_EXPORT const sin_port::original_ammo::Api* SIN_AMMO_CALL
SiN_GetOriginalAmmoDefinitionApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::original_ammo;
    return version==sin_port::original_ammo::version&&size==sizeof(Api)?&api:nullptr;
}
