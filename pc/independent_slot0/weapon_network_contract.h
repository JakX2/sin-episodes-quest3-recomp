#pragma once
#include <cstdint>
#if defined(_MSC_VER)
#define SIN_WNET_CALL __cdecl
#else
#define SIN_WNET_CALL
#endif
namespace sin_port::weapon_net {
inline constexpr char export_name[]="SiN_GetWeaponNetworkApi";
inline constexpr std::uint32_t version=1,field_count=13;
enum Field : std::uint32_t {
    Clip1=0,Clip2,PrimaryAmmo,SecondaryAmmo,ViewModelSlot,
    NextPrimary,NextSecondary,NextThinkTick,IdleTime,
    ViewModelIndex,WorldModelIndex,State,Owner
};
enum ValueKind : std::uint32_t {SignedInt=0,UnsignedInt,ClipPlusOne,FloatBits,EntityHandle};
enum Group : std::uint32_t {LocalWeapon=0,LocalActive,PublicWeapon};
inline constexpr std::uint32_t local_mask=0x1ffu,public_mask=0x1e00u,all_mask=0x1fffu;
inline constexpr std::uint32_t invalid_handle=0xffffffffu,wire_invalid_handle=0x1fffffu;
struct FieldInfo {
    char name[32];
    std::uint32_t group,slot,server_offset,client_offset,kind,bits,flags;
};
struct Values {std::uint32_t words[field_count];};
// This is a port-owned scalar payload, NOT a Source packet, flattened
// sendtable, or bitstream. Inactive fields must be zero on export.
struct Payload {std::uint32_t present;Values values;};
struct OwnerObservation {
    std::uint32_t native_handle,resolved,player_eligible,client_slot;
};
struct RecipientMask {std::uint32_t words[8];};
struct Api {
    std::uint32_t version,size;
    int (SIN_WNET_CALL *field)(std::uint32_t,FieldInfo*) noexcept;
    int (SIN_WNET_CALL *recipients)(const OwnerObservation*,RecipientMask*) noexcept;
    int (SIN_WNET_CALL *encode)(const Values*,const OwnerObservation*,std::uint32_t,Payload*) noexcept;
    // Merge only present fields into the caller's state. local_expected
    // is the caller's verified recipient context; no ownership inferred
    // from a truncated decoded handle.
    int (SIN_WNET_CALL *decode)(const Payload*,std::uint32_t,Values*) noexcept;
};
using GetApi=const Api* (SIN_WNET_CALL *)(std::uint32_t,std::uint32_t) noexcept;
const Api& owned_api() noexcept;
static_assert(sizeof(Values)==52&&sizeof(Payload)==56&&sizeof(RecipientMask)==32);
}
