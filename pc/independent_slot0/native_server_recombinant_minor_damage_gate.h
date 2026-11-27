#pragma once
#include "native_server_recombinant_minor.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_minor_damage_gate_export[]="SiN_GetServerRecombinantMinorDamageGateApi";
// Exact original Minor-only +0x398 virtual behavior. Shared game-mode
// predicate remains owned by the original engine (+0x170).
struct ServerRecombinantMinorDamageGateBindings {
    void* context;
    int (__cdecl *mode_blocks)(void*,bool*) noexcept; // original global 22572198 +170
};
struct ServerRecombinantMinorDamageGateResult {
    unsigned completed,mode_queries,suppressed,damage_read,accepted;
    float damage,threshold;
};
struct ServerRecombinantMinorDamageGateApi {
    unsigned version,size;
    int (__cdecl *evaluate)(void*,unsigned,const void*,unsigned,
        const ServerRecombinantMinorDamageGateBindings*,ServerRecombinantMinorDamageGateResult*) noexcept;
};
using GetServerRecombinantMinorDamageGateApi=const ServerRecombinantMinorDamageGateApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
