#include "weapon_network_contract.h"
#include <cstring>
#include <cmath>
#if defined(_MSC_VER)
#define SIN_WNET_EXPORT __declspec(dllexport)
#if defined(_M_IX86)
#pragma comment(linker,"/EXPORT:SiN_GetWeaponNetworkApi=_SiN_GetWeaponNetworkApi")
#pragma comment(linker,"/EXPORT:SiN_GetInventoryNetworkApi=_SiN_GetInventoryNetworkApi")
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
bool encode_handle(std::uint32_t raw,std::uint32_t live,std::uint32_t& out) noexcept {
    if(live>1)return false;
    if(!live){out=wire_invalid_handle;return true;}
    if(raw==invalid_handle||(raw&0xfffu)>0x7ffu)return false;
    const auto wire=(raw&0xfffu)|(((raw>>12)&0x3ffu)<<11);
    if(wire==wire_invalid_handle)return false;
    out=wire;return true;
}
int SIN_WNET_CALL encode(const Values* in,const OwnerObservation* owner,std::uint32_t viewer,Payload* out) noexcept {
    if(!in||!owner||!out||viewer<1||viewer>256||!valid_owner(*owner)||
       in->words[Owner]!=owner->native_handle)return 0;
    Payload result{};
    result.present=public_mask;
    if(owner->resolved&&owner->player_eligible&&owner->client_slot==viewer)result.present|=local_mask;
    for(unsigned i=0;i<Owner;++i)
        if((result.present&(1u<<i))&&!encode_field(fields[i],in->words[i],result.values.words[i]))return 0;
    if(!encode_handle(owner->native_handle,owner->resolved,result.values.words[Owner]))return 0;
    *out=result;return 1;
}
bool decode_handle(std::uint32_t in,std::uint32_t& out) noexcept {
    if(in>wire_invalid_handle)return false;
    out=in==wire_invalid_handle?invalid_handle:((in>>11)<<12)|(in&0x7ffu);
    return true;
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
        return decode_handle(in,out);
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
constexpr InventoryInfo inventory_fields[]={
    {"m_iAmmo","DT_LocalPlayerExclusive",InventoryAmmo,5,0x64c,0xc00,32,4,10,1},
    {"m_flNextAttack","DT_BCCLocalPlayerExclusive",InventoryCombat,0,0x60c,0xbf8,1,4,0,4},
    {"m_hMyWeapons","DT_BCCLocalPlayerExclusive",InventoryCombat,1,0x6cc,0xc80,48,4,21,1},
    {"m_hActiveWeapon","DT_BaseCombatCharacter",InventoryPublic,2,0x78c,0xd40,1,4,21,1}
};
int SIN_WNET_CALL inventory_field(std::uint32_t i,InventoryInfo* out) noexcept {
    if(i>=4||!out)return 0;
    *out=inventory_fields[i];return 1;
}
std::uint32_t SIN_WNET_CALL inventory_groups(const InventoryRecipients* r,std::uint32_t viewer) noexcept {
    if(!r||viewer<1||viewer>256||r->ammo_slot>256||r->combat_slot>256)return 0;
    return InventoryPublic|(r->ammo_slot==viewer?InventoryAmmo:0u)|(r->combat_slot==viewer?InventoryCombat:0u);
}
int SIN_WNET_CALL inventory_encode(const InventoryValues* in,const InventoryHandleObservations* handles,
        const InventoryRecipients* recipients,std::uint32_t viewer,InventoryPayload* out) noexcept {
    if(!in||!handles||!out)return 0;
    InventoryPayload result{};
    result.present=inventory_groups(recipients,viewer);
    if(!result.present)return 0;
    if(result.present&InventoryAmmo){
        for(unsigned i=0;i<inventory_ammo_count;++i){
            if(in->ammo[i]>1023)return 0;
            result.values.ammo[i]=in->ammo[i];
        }
    }
    if(result.present&InventoryCombat){
        if(!finite_bits(in->next_attack))return 0;
        result.values.next_attack=in->next_attack;
        for(unsigned i=0;i<inventory_weapon_count;++i)
            if(!encode_handle(in->weapons[i],handles->weapons[i],result.values.weapons[i]))return 0;
    }
    if(!encode_handle(in->active_weapon,handles->active_weapon,result.values.active_weapon))return 0;
    *out=result;return 1;
}
int SIN_WNET_CALL inventory_decode(const InventoryPayload* in,std::uint32_t expected,InventoryValues* out) noexcept {
    if(!in||!out||!(expected&InventoryPublic)||(expected&~7u)||in->present!=expected)return 0;
    InventoryValues result=*out;
    for(unsigned i=0;i<inventory_ammo_count;++i){
        const auto value=in->values.ammo[i];
        if(expected&InventoryAmmo){
            if(value>1023)return 0;
            result.ammo[i]=value;
        }else if(value)return 0;
    }
    for(unsigned i=0;i<inventory_weapon_count;++i){
        const auto value=in->values.weapons[i];
        if(expected&InventoryCombat){
            if(!decode_handle(value,result.weapons[i]))return 0;
        }else if(value)return 0;
    }
    if(expected&InventoryCombat){
        if(!finite_bits(in->values.next_attack))return 0;
        result.next_attack=in->values.next_attack;
    }else if(in->values.next_attack)return 0;
    if(!decode_handle(in->values.active_weapon,result.active_weapon))return 0;
    *out=result;return 1;
}
const InventoryApi inventory_api{version,sizeof(InventoryApi),&inventory_field,&inventory_groups,
    &inventory_encode,&inventory_decode};
}
const Api& owned_api() noexcept{return api;}
const InventoryApi& owned_inventory_api() noexcept{return inventory_api;}
}
extern "C" SIN_WNET_EXPORT const sin_port::weapon_net::Api* SIN_WNET_CALL
SiN_GetWeaponNetworkApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::weapon_net;
    return version==sin_port::weapon_net::version&&size==sizeof(Api)?&owned_api():nullptr;
}

extern "C" SIN_WNET_EXPORT const sin_port::weapon_net::InventoryApi* SIN_WNET_CALL
SiN_GetInventoryNetworkApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::weapon_net;
    return version==sin_port::weapon_net::version&&size==sizeof(InventoryApi)?&owned_inventory_api():nullptr;
}
