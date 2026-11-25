#pragma once
#include "native_server_npc_schedule_state.h"
#include "native_server_weapon_activity.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_state_reconsider_export[]="SiN_GetServerNpcStateReconsiderApi";
struct ServerNpcStateBindings {
    ServerNpcScheduleBindings schedule;
    ServerWeaponActivityBindings entities;
    void* context;
    int (__cdecl *clear_enemy)(void*,void*) noexcept; // 2203e740(0,1)
    int (__cdecl *state_changed)(void*,void*,int,int) noexcept; // +4d0
    int (__cdecl *lost_enemy)(void*,void*,void*) noexcept; // 22358810
    int (__cdecl *squad_enemy)(void*,std::uint32_t,void*) noexcept; // 220a9520
    int (__cdecl *state_query)(void*,void*,int,int*) noexcept; // 2203be80/2203c0f0/220415c0
};
struct ServerNpcStateResult {int current,ideal;unsigned reconsider,changed,enemy_cleared,patrol_set,lost_enemy_called,squad_called;};
struct ServerNpcStateApi {
    std::uint32_t version,size;
    int (__cdecl *should_reconsider)(void*,unsigned,const ServerNpcStateBindings*,ServerNpcStateResult*) noexcept;
    int (__cdecl *choose_ideal)(void*,unsigned,const ServerNpcStateBindings*,ServerNpcStateResult*) noexcept;
    int (__cdecl *apply)(void*,unsigned,int,float,const ServerNpcStateBindings*,ServerNpcStateResult*) noexcept;
};
using GetServerNpcStateApi=const ServerNpcStateApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_enemy_state_export[]="SiN_GetServerNpcEnemyStateApi";
struct ServerNpcEnemyStateBindings {
    void* context;
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
    ServerNpcConditionMapBindings conditions;
    int (__cdecl *enemy_changing)(void*,void*) noexcept; // actor +6e0
    int (__cdecl *release_squad_slot)(void*,std::uint32_t,void*,int) noexcept; // 220aaff0
    int (__cdecl *entity_handle)(void*,void*,std::uint32_t*) noexcept; // entity +8
};
struct ServerNpcEnemyStateResult {
    std::uint32_t old_handle,new_handle;
    unsigned changed,notified,squad_released,new_condition_set,new_condition_cleared,reset_fields;
};
struct ServerNpcEnemyConditionResult {
    unsigned prior_preserved,cleared,restored;
};
struct ServerNpcEnemyStateApi {
    std::uint32_t version,size;
    // Original 2203e740: target may be null; notify controls condition 0x19.
    int (__cdecl *set)(void*,unsigned,void*,bool,float,const ServerNpcEnemyStateBindings*,ServerNpcEnemyStateResult*) noexcept;
    // Original Grunt 22358ea0: clear 9 conditions and preserve prior 0x15.
    int (__cdecl *clear_combat_conditions)(void*,unsigned,const ServerNpcConditionMapBindings*,ServerNpcEnemyConditionResult*) noexcept;
};
using GetServerNpcEnemyStateApi=const ServerNpcEnemyStateApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_enemy_memory_export[]="SiN_GetServerNpcEnemyMemoryApi";
// Keys are original PE32 entity-address tokens, NOT serialized entity handles.
// The caller maps original 32-bit storage tokens to bounded native objects.
struct ServerNpcEnemyMemoryBindings {
    void* context;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
};
struct ServerNpcEnemyMemoryLookup {
    std::uint32_t key,record;
    unsigned index,visited,fallback_attempted,fallback_used,matched;
};
enum ServerNpcEnemyMemoryTimestamp : unsigned {
    NpcMemoryStamp1c=0,NpcMemoryStamp20=1,NpcMemoryStamp24=2,NpcMemoryStamp2c=3,NpcMemoryStamp30=4
};
struct ServerNpcEnemyMemoryApi {
    std::uint32_t version,size;
    // 2206b1a0: indexed tree + default-key retry when first key absent.
    int (__cdecl *lookup)(const void*,unsigned,std::uint32_t,bool,const ServerNpcEnemyMemoryBindings*,ServerNpcEnemyMemoryLookup*) noexcept;
    // 2206b400/450/4a0/560/5b0. Null input gives 0; missing record gives -FLT_MAX.
    int (__cdecl *timestamp)(const void*,unsigned,std::uint32_t,unsigned,const ServerNpcEnemyMemoryBindings*,ServerNpcEnemyMemoryLookup*,float*) noexcept;
    // 2206b360/3b0: cache exact vector bits in owner+24 or +30; miss preserves cache.
    int (__cdecl *position)(void*,unsigned,std::uint32_t,bool,const ServerNpcEnemyMemoryBindings*,ServerNpcEnemyMemoryLookup*,float[3]) noexcept;
    // 2206b270 writes record+2c if found.
    int (__cdecl *touch)(const void*,unsigned,std::uint32_t,float,const ServerNpcEnemyMemoryBindings*,ServerNpcEnemyMemoryLookup*) noexcept;
    // 2206b4f0: null false; missing record true; otherwise stamp1c+owner1c>now.
    int (__cdecl *recent)(const void*,unsigned,std::uint32_t,float,const ServerNpcEnemyMemoryBindings*,ServerNpcEnemyMemoryLookup*,bool*) noexcept;
};
using GetServerNpcEnemyMemoryApi=const ServerNpcEnemyMemoryApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
