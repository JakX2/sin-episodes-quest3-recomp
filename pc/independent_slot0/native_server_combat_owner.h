#pragma once
#include <cstdint>
#include <cstring>
namespace sin_port::native_weapon_tables {
// Exact authenticated original primary vtables; never invoke raw PE32 addresses.
enum class NativeCombatOwnerKind : int {Unknown=-1,Noncombat=0,Npc=1,Player=2};
inline NativeCombatOwnerKind native_combat_owner_kind(std::uint32_t vt) noexcept {
    switch(vt){
    case 0x2248df04u:case 0x224954a4u:case 0x2249aaa4u:case 0x224a416cu:
    case 0x2248c514u:case 0x2248e83cu:case 0x2249964cu:case 0x224a4a4cu:return NativeCombatOwnerKind::Npc;
    case 0x2245222cu:case 0x224aec3cu:return NativeCombatOwnerKind::Player;
    case 0x2245a554u:case 0x22478d0cu:case 0x2247875cu:case 0x224776acu:case 0x2247a1e4u:
    case 0x22477c8cu:case 0x22478214u:case 0x2247924cu:case 0x2247ab9cu:return NativeCombatOwnerKind::Noncombat;
    default:return NativeCombatOwnerKind::Unknown;
    }
}
using NativeOwnerExtent=int (__cdecl *)(void*,void*,unsigned*) noexcept;
inline NativeCombatOwnerKind bounded_combat_owner_kind(void* entity,void* context,NativeOwnerExtent extent) noexcept {
    unsigned n=0;if(!entity||!extent||!extent(context,entity,&n)||n<4)return NativeCombatOwnerKind::Unknown;
    std::uint32_t vt=0;std::memcpy(&vt,entity,4);return native_combat_owner_kind(vt);
}
// +fc: 22044820 identity or 220017a0 null. 1 combat, 0 noncombat, -1 unsupported.
inline int native_combat_owner(void* context,void* entity,NativeOwnerExtent extent,void** out) noexcept {
    if(!out)return -1;*out=nullptr;
    const auto k=bounded_combat_owner_kind(entity,context,extent);
    if(k==NativeCombatOwnerKind::Unknown)return -1;
    if(k==NativeCombatOwnerKind::Noncombat)return 0;
    *out=entity;return 1;
}
// +120: 22253c30 MOV AL,1; RET or 220017e0 XOR AL,AL; RET.
inline int native_combat_is_player(void* context,void* entity,NativeOwnerExtent extent,bool* out) noexcept {
    if(!out)return 0;*out=false;
    const auto k=bounded_combat_owner_kind(entity,context,extent);
    if(k==NativeCombatOwnerKind::Unknown)return 0;
    *out=k==NativeCombatOwnerKind::Player;return 1;
}
}
