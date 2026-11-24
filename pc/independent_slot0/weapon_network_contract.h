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
// Separate original inventory arrays and public active-weapon field.
// Port scalar payload; never a Source flattened table or wire packet.
inline constexpr char inventory_export_name[]="SiN_GetInventoryNetworkApi";
inline constexpr std::uint32_t inventory_ammo_count=32,inventory_weapon_count=48;
enum InventoryGroup : std::uint32_t {InventoryAmmo=1,InventoryCombat=2,InventoryPublic=4};
struct InventoryInfo {
    char name[32],table[40];
    std::uint32_t group,slot,server_offset,client_offset,elements,stride,bits,flags;
};
struct InventoryValues {
    std::uint32_t ammo[inventory_ammo_count],weapons[inventory_weapon_count],next_attack,active_weapon;
};
struct InventoryHandleObservations {
    std::uint32_t weapons[inventory_weapon_count],active_weapon;
};
// Slots are host-observed one-based client IDs. Zero means no recipient.
// Ammo is the player table's own object-ID recipient. Combat is the
// character proxy's direct/indirect resolved eligible player recipient.
struct InventoryRecipients {std::uint32_t ammo_slot,combat_slot;};
struct InventoryPayload {std::uint32_t present;InventoryValues values;};
struct InventoryApi {
    std::uint32_t version,size;
    int (SIN_WNET_CALL *field)(std::uint32_t,InventoryInfo*) noexcept;
    std::uint32_t (SIN_WNET_CALL *groups)(const InventoryRecipients*,std::uint32_t) noexcept;
    int (SIN_WNET_CALL *encode)(const InventoryValues*,const InventoryHandleObservations*,
        const InventoryRecipients*,std::uint32_t,InventoryPayload*) noexcept;
    int (SIN_WNET_CALL *decode)(const InventoryPayload*,std::uint32_t,InventoryValues*) noexcept;
};
using GetInventoryApi=const InventoryApi* (SIN_WNET_CALL *)(std::uint32_t,std::uint32_t) noexcept;
const InventoryApi& owned_inventory_api() noexcept;
static_assert(sizeof(InventoryValues)==328&&sizeof(InventoryPayload)==332);
static_assert(sizeof(Values)==52&&sizeof(Payload)==56&&sizeof(RecipientMask)==32);
}
