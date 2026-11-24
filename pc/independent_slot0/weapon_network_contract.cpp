#include "weapon_network_contract.h"
#include <cstring>
#include <cmath>
#if defined(_MSC_VER)
#define SIN_WNET_EXPORT __declspec(dllexport)
#if defined(_M_IX86)
#pragma comment(linker,"/EXPORT:SiN_GetWeaponNetworkApi=_SiN_GetWeaponNetworkApi")
#endif
#else
#define SIN_WNET_EXPORT __attribute__((visibility("default")))
#endif
namespace sin_port::weapon_net {
namespace {
// Offsets are evidence only. Never used as offsets in a port object.
// Float props use NOSCALE, original stored nBits=0, scalar payload 32.
constexpr FieldInfo fields[]={
    {"m_iClip1",LocalWeapon,0,0x4ec,0x7e0,ClipPlusOne,8,1},
    {"m_iClip2",LocalWeapon,1,0x4f0,0x7e4,ClipPlusOne,8,1},
    {"m_iPrimaryAmmoType",LocalWeapon,2,0x4e4,0x7d8,SignedInt,8,0},
    {"m_iSecondaryAmmoType",LocalWeapon,3,0x4e8,0x7dc,SignedInt,8,0},
    {"m_nViewModelIndex",LocalWeapon,4,0x498,0x78c,UnsignedInt,1,1},
    {"m_flNextPrimaryAttack",LocalActive,0,0x49c,0x790,FloatBits,0,4},
    {"m_flNextSecondaryAttack",LocalActive,1,0x4a0,0x794,FloatBits,0,4},
    {"m_nNextThinkTick",LocalActive,2,0x74,0x84,SignedInt,32,0},
    {"m_flTimeWeaponIdle",LocalActive,3,0x4a4,0x798,FloatBits,0,4},
    {"m_iViewModelIndex",PublicWeapon,3,0x4b0,0x7a4,SignedInt,11,0},
    {"m_iWorldModelIndex",PublicWeapon,4,0x4b4,0x7a8,SignedInt,11,0},
    {"m_iState",PublicWeapon,5,0x4dc,0x7d0,UnsignedInt,8,1},
    {"m_hOwner",PublicWeapon,6,0x494,0x788,EntityHandle,21,1}
};
static_assert(sizeof(fields)/sizeof(fields[0])==field_count);
int SIN_WNET_CALL field(std::uint32_t id,FieldInfo* out) noexcept {
    if(id>=field_count||!out)return 0;
    *out=fields[id];return 1;
}
bool valid_owner(const OwnerObservation& o) noexcept {
    if(o.resolved>1||o.player_eligible>1)return false;
    if(o.resolved&&(o.native_handle==invalid_handle||(o.native_handle&0xfffu)>0x7ffu))return false;
    if(o.resolved&&o.player_eligible&&(o.client_slot<1||o.client_slot>256))return false;
    return true;
}
int SIN_WNET_CALL recipients(const OwnerObservation* o,RecipientMask* out) noexcept {
    if(!o||!out||!valid_owner(*o))return 0;
    RecipientMask result{};
    if(o->resolved&&o->player_eligible){
        const auto i=o->client_slot-1;
        result.words[i/32]=1u<<(i%32);
    }
    *out=result;return 1;
}
bool finite_bits(std::uint32_t bits) noexcept {
    float f=0.f;std::memcpy(&f,&bits,4);return std::isfinite(f);
}
std::int32_t signed_word(std::uint32_t bits) noexcept {
    std::int32_t value=0;std::memcpy(&value,&bits,4);return value;
}
bool encode_field(const FieldInfo& f,std::uint32_t input,std::uint32_t& out) noexcept {
    switch(f.kind){
    case ClipPlusOne:
        if(input!=invalid_handle&&input>254)return false;
        out=input+1u;return true;
    case SignedInt:
        if(f.bits==32){out=input;return true;}
        {
            const std::int32_t half=1<<(f.bits-1),v=signed_word(input);
            if(v< -half||v>=half)return false;
            out=input&((1u<<f.bits)-1);return true;
        }
    case UnsignedInt:
        if(input>=(1u<<f.bits))return false;
        out=input;return true;
    case FloatBits:
        if(!finite_bits(input))return false;
        out=input;return true;
    default:return false;
    }
}
int SIN_WNET_CALL encode(const Values* in,const OwnerObservation* owner,std::uint32_t viewer,Payload* out) noexcept {
    if(!in||!owner||!out||viewer<1||viewer>256||!valid_owner(*owner)||
       in->words[Owner]!=owner->native_handle)return 0;
    Payload result{};
    result.present=public_mask;
    if(owner->resolved&&owner->player_eligible&&owner->client_slot==viewer)result.present|=local_mask;
    for(unsigned i=0;i<Owner;++i)
        if((result.present&(1u<<i))&&!encode_field(fields[i],in->words[i],result.values.words[i]))return 0;
    auto wire=wire_invalid_handle;
    if(owner->resolved){
        wire=(owner->native_handle&0xfffu)|(((owner->native_handle>>12)&0x3ffu)<<11);
        // Reject the reserved wire sentinel collision instead of
        // publishing an apparently null live entity.
        if(wire==wire_invalid_handle)return 0;
    }
    result.values.words[Owner]=wire;
    *out=result;return 1;
}
bool decode_field(const FieldInfo& f,std::uint32_t in,std::uint32_t& out) noexcept {
    switch(f.kind){
    case ClipPlusOne:
        if(in>255)return false;
        out=in-1u;return true;
    case SignedInt:
        if(f.bits==32){out=in;return true;}
        {
            const auto mask=(1u<<f.bits)-1u;
            if(in>mask)return false;
            out=(in&(1u<<(f.bits-1)))?(in|~mask):in;return true;
        }
    case UnsignedInt:
        if(in>=(1u<<f.bits))return false;
        out=in;return true;
    case FloatBits:
        if(!finite_bits(in))return false;
        out=in;return true;
    case EntityHandle:
        if(in>wire_invalid_handle)return false;
        out=in==wire_invalid_handle?invalid_handle:((in>>11)<<12)|(in&0x7ffu);
        return true;
    default:return false;
    }
}
int SIN_WNET_CALL decode(const Payload* in,std::uint32_t local,Values* out) noexcept {
    if(!in||!out||local>1||in->present!=(local?all_mask:public_mask))return 0;
    Values result=*out;
    for(unsigned i=0;i<field_count;++i){
        if(in->present&(1u<<i)){
            if(!decode_field(fields[i],in->values.words[i],result.words[i]))return 0;
        }else if(in->values.words[i]!=0)return 0;
    }
    *out=result;return 1;
}
const Api api{version,sizeof(Api),&field,&recipients,&encode,&decode};
}
const Api& owned_api() noexcept{return api;}
}
extern "C" SIN_WNET_EXPORT const sin_port::weapon_net::Api* SIN_WNET_CALL
SiN_GetWeaponNetworkApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::weapon_net;
    return version==sin_port::weapon_net::version&&size==sizeof(Api)?&owned_api():nullptr;
}
