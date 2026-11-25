#include "native_server_weapon_script_view.h"
#include <cstring>
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
int __cdecl resolve(const ServerWeaponScriptView* v,std::uint16_t index,ServerWeaponScriptRecord* out) noexcept {
    if(!out)return 0;*out={};
    if(!v||v->count>65535||v->count*16u>v->bytes||(v->count&&!v->rows))return 0;
    if(index==0xffffu||index>=v->count){
        if(!v->fallback||!v->fallback_bytes)return 0;
        *out={v->fallback,v->fallback_bytes,1};return 1;
    }
    const auto token=read<std::uint32_t>(v->rows,index*16u+12u);
    if(!token||!v->storage||!v->storage(v->context,token,&out->data,&out->bytes)||!out->data||!out->bytes){*out={};return 0;}
    return 1;
}
int __cdecl weapon(const ServerWeaponScriptView* v,const void* w,unsigned bytes,ServerWeaponScriptRecord* out) noexcept {
    if(!out)return 0;*out={};return w&&bytes>=0x51e&&resolve(v,read<std::uint16_t>(w,0x51c),out);
}
int __cdecl value(const ServerWeaponScriptView* v,const void* w,unsigned bytes,unsigned slot,int* out) noexcept {
    if(!out)return 0;*out=0;
    unsigned offset=0,width=4;bool boolean=false;
    switch(slot){
    case 0x404:offset=0x160;break;case 0x408:offset=0x164;break;
    case 0x40c:offset=0x168;break;case 0x410:offset=0x16c;break;
    case 0x414:offset=0x170;break;case 0x418:offset=0x174;width=1;break;
    case 0x41c:offset=0x175;width=1;break;case 0x420:offset=0x178;break;
    case 0x424:offset=0x158;break;case 0x428:offset=0x15c;break;
    case 0x438:offset=0x160;boolean=true;break;case 0x43c:offset=0x164;boolean=true;break;
    case 0x440:offset=0x1bc;break;case 0x4a0:offset=0x720;break;
    default:return 0;
    }
    ServerWeaponScriptRecord rec{};
    if(!weapon(v,w,bytes,&rec)||rec.bytes<offset+width)return 0;
    const int result=width==1?read<std::uint8_t>(rec.data,offset):read<int>(rec.data,offset);
    *out=boolean?result!=-1:result;return 1;
}
int __cdecl interval(unsigned kind,double* out) noexcept {
    if(!out)return 0;*out=0;
    // Exact original binary32 values, not decimal double approximations.
    constexpr float values[]{0.f,.075f,.075f,0.f,.4f,.6f,.4f,.7f,.1f};
    if(kind>=sizeof(values)/sizeof(values[0]))return 0;
    *out=values[kind];return 1;
}
int __cdecl burst(const void* w,unsigned bytes,unsigned kind,int* out) noexcept {
    if(!out)return 0;*out=0;
    if(kind==WeaponActivityChaingun||kind==WeaponActivitySmg){*out=3;return 1;}
    if(kind==WeaponActivityAssault&&w&&bytes>=0x579){*out=read<std::uint8_t>(w,0x578)?1:3;return 1;}
    return 0;
}
const ServerWeaponScriptViewApi api{1,sizeof(ServerWeaponScriptViewApi),&resolve,&weapon,&value,&interval,&burst};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerWeaponScriptViewApi=_SiN_GetServerWeaponScriptViewApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponScriptViewApi* __cdecl
SiN_GetServerWeaponScriptViewApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerWeaponScriptViewApi)?&api:nullptr;
}
