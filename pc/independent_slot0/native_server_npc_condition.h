#pragma once
#include <cstdint>
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_condition_export[]="SiN_GetServerNpcConditionApi";
inline constexpr int npc_global_condition_base=1000000000;
enum ServerNpcConditionBank : unsigned {NpcConditionCurrent=0,NpcConditionInterrupt=1,NpcConditionSnapshot=2,NpcConditionMask=3};
struct ServerNpcConditionMapBindings {
    void* context;
    // Original actor +760 class descriptor, first range at +34.
    int (__cdecl *descriptor)(void*,void*,const void**,unsigned*) noexcept;
    int (__cdecl *storage)(void*,std::uint32_t,const void**,unsigned*) noexcept;
};
struct ServerNpcConditionResult {int global,bit;unsigned mapped,visited,changed,value;};
// Internal original 22073700 range resolution; no condition-bank bounds applied here.
int resolve_npc_condition_range(void*,int,const ServerNpcConditionMapBindings*,ServerNpcConditionResult*) noexcept;
struct ServerNpcConditionApi {
    std::uint32_t version,size;
    // 22073700 / 220397c0 namespace resolution; -1 unmapped is a successful no-op.
    int (__cdecl *map)(void*,int,const ServerNpcConditionMapBindings*,ServerNpcConditionResult*) noexcept;
    int (__cdecl *test)(void*,unsigned,int,unsigned,const ServerNpcConditionMapBindings*,ServerNpcConditionResult*) noexcept;
    // 22039760/220398a0 current; 22039c70/22039cd0 interrupt.
    int (__cdecl *store)(void*,unsigned,int,unsigned,bool,const ServerNpcConditionMapBindings*,ServerNpcConditionResult*) noexcept;
    // Prefix of 2203b110: snapshot current then apply mask. No later AI logic.
    int (__cdecl *snapshot_mask)(void*,unsigned) noexcept;
};
using GetServerNpcConditionApi=const ServerNpcConditionApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
