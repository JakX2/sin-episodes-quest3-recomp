#pragma once
// Win32 descriptor graph with original property offsets. It is NOT
// registered with the retail engine and does not construct any entity.
// Only use with a host-owned compatible object/table and verified
// live owner/recipient callbacks. Original absolute addresses are evidence.
#include "server_send_datatable_core.h"
#include "weapon_network_contract.h"
#include "server_entity_handle_core.h"
#include <cstddef>
#include <cstdint>

namespace sin_independent::send_schema {
struct DVariant {
    std::uint32_t words[3]{};
    int type=0;
};
static_assert(sizeof(DVariant)==16&&offsetof(DVariant,type)==12);
}
namespace sin_port::native_weapon_tables {
namespace s=sin_independent::send_schema;
namespace n=sin_port::weapon_net;
struct RecvProp;
struct RecvTable;
struct RecvData {
    const RecvProp* property=nullptr;
    s::DVariant value{};
    int element=0,object_id=0;
};
using RecvProxy=void (__cdecl *)(const RecvData*,void*,void*);
using RecvTableProxy=void (__cdecl *)(const RecvProp*,void**,void*,int);
using RecvArrayLength=void (__cdecl *)(void*,int,int);
struct RecvProp {
    const char* name=nullptr;
    int type=0,flags=0,string_size=0;
    bool inside_array=false;
    unsigned char padding[3]{};
    const void* extra=nullptr;
    RecvProp* array_prop=nullptr;
    RecvArrayLength array_length=nullptr;
    RecvProxy proxy=nullptr;
    RecvTableProxy table_proxy=nullptr;
    RecvTable* table=nullptr;
    int offset=0,stride=-1,elements=1;
    const char* parent_array=nullptr;
};
struct RecvTable {
    RecvProp* props=nullptr;
    int count=0;
    void* decoder=nullptr;
    const char* name=nullptr;
    bool initialized=false,in_main_list=false;
};
static_assert(sizeof(RecvProp)==60&&offsetof(RecvProp,proxy)==32&&
    offsetof(RecvProp,table_proxy)==36&&offsetof(RecvProp,table)==40&&
    offsetof(RecvProp,offset)==44&&offsetof(RecvProp,parent_array)==56);
static_assert(sizeof(RecvTable)==20&&offsetof(RecvTable,decoder)==8&&
    offsetof(RecvTable,name)==12&&offsetof(RecvTable,initialized)==16);
static_assert(sizeof(RecvData)==28&&offsetof(RecvData,value)==4);
enum TableId : std::uint32_t {
    LocalWeapon=0,LocalActive,BaseCombat,BaseHL,MachineGun,SelectFire,Magnum,AssaultRifle,Scattergun,TableCount
};
inline constexpr const char* table_names[TableCount]={
    "DT_LocalWeaponData","DT_LocalActiveWeaponData","DT_BaseCombatWeapon",
    "DT_BaseHLCombatWeapon","DT_HLMachineGun","DT_HLSelectFireMachineGun",
    "DT_WeaponMagnum","DT_WeaponAssaultRifle","DT_WeaponScatterGun"
};
inline constexpr int table_counts[TableCount]={5,4,7,1,1,1,1,1,1};
inline constexpr std::uint32_t version=1;
inline constexpr char send_export[]="SiN_GetWeaponSendGraphApi";
inline constexpr char recv_export[]="SiN_GetWeaponRecvGraphApi";
inline constexpr char resolved_send_export[]="SiN_GetResolvedWeaponSendGraphApi";
struct SendBindings {
    s::SendTable* base_animating;
    // Real host must implement the original eligibility and live handle
    // resolution. Never substitute an unrestricted local table proxy.
    // The DLL can reject its own standard proxy addresses only. Foreign
    // function-pointer identity cannot certify callback semantics.
    s::TableProxy local_weapon,local_active;
    s::VarProxy owner_handle;
};
struct RecvBindings {RecvTable* base_animating;};
// Borrowed live native dependencies; the host guarantees the registry
// entry points at the original handle-interface subobject, and keeps
// all objects/edict storage stable for the synchronous engine callbacks.
// No original fixed address or engine-wide global is accessed.
struct ResolvedSendBindings {
    s::SendTable* base_animating;
    void* engine_server; // original VEngineServer021
    const sin_independent::EntityHandleEntry* entries;
    std::uint32_t entry_count;
    const void* edict_base; // original 20-byte edict array, including world
    std::uint32_t max_clients;
};
template<class Table> struct View {void* handle;Table* tables[TableCount];};
using SendView=View<s::SendTable>;
using RecvView=View<RecvTable>;
template<class Bindings,class V> struct GraphApi {
    std::uint32_t version,size;
    // Caller passes empty output. On failure leaves it unchanged.
    int (__cdecl *create)(const Bindings*,V*) noexcept;
    // Refuses while owned table has engine precalc/decoder/flags. Caller
    // must already have detached ALL engine references and stopped use.
    int (__cdecl *destroy)(V*) noexcept;
};
using SendApi=GraphApi<SendBindings,SendView>;
using RecvApi=GraphApi<RecvBindings,RecvView>;
using GetSendApi=const SendApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
using GetRecvApi=const RecvApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
using ResolvedSendApi=GraphApi<ResolvedSendBindings,SendView>;
using GetResolvedSendApi=const ResolvedSendApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
// Actual Array3 graphs, not DPT_Array. The ammo parent property is a
// component for the unfinished 27-property DT_LocalPlayerExclusive.
inline constexpr char inventory_send_export[]="SiN_GetInventorySendGraphApi";
inline constexpr char inventory_recv_export[]="SiN_GetInventoryRecvGraphApi";
struct InventorySendBindings {
    s::SendTable* base_flex;
    void* engine_server;
    const sin_independent::EntityHandleEntry* entries;
    std::uint32_t entry_count;
    const void* edict_base;
    std::uint32_t max_clients;
};
struct InventoryRecvBindings {RecvTable* base_flex;};
template<class Table,class Prop> struct InventoryView {
    void* handle;
    Table* ammo_table;
    Table* weapons_table;
    Table* combat_local;
    Table* combat_base;
    Prop* ammo_property;
};
struct InventorySendView:InventoryView<s::SendTable,s::SendProp> {
    // Original object-ID recipient proxy for the future BasePlayer
    // localdata parent. Caller must use it only on compatible storage.
    s::TableProxy player_local_proxy;
};
using InventoryRecvView=InventoryView<RecvTable,RecvProp>;
using InventorySendApi=GraphApi<InventorySendBindings,InventorySendView>;
using InventoryRecvApi=GraphApi<InventoryRecvBindings,InventoryRecvView>;
using GetInventorySendApi=const InventorySendApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
using GetInventoryRecvApi=const InventoryRecvApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase697. Borrowed BaseEntity and handle registry must outlive graph.
// Detach dependent weapon graphs and engine references before destroying.
inline constexpr char anim_send_export[]="SiN_GetAnimatingSendGraphApi";
inline constexpr char anim_recv_export[]="SiN_GetAnimatingRecvGraphApi";
struct AnimSendBindings {
    s::SendTable* base_entity;
    const sin_independent::EntityHandleEntry* entries;
    std::uint32_t entry_count;
};
struct AnimRecvBindings {RecvTable* base_entity;};
template<class Table> struct AnimView {
    void* handle;
    Table* base_animating;
    Table* server_anim_data;
    Table* pose;
    Table* controllers;
};
using AnimSendView=AnimView<s::SendTable>;
using AnimRecvView=AnimView<RecvTable>;
using AnimSendApi=GraphApi<AnimSendBindings,AnimSendView>;
using AnimRecvApi=GraphApi<AnimRecvBindings,AnimRecvView>;
using GetAnimSendApi=const AnimSendApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
using GetAnimRecvApi=const AnimRecvApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase698. DT_OverlayVars uses a dynamic UtlVector and is deliberately
// borrowed, not represented by a fixed array. Both dependencies must outlive
// this graph; detach dependent inventory graphs before destroying.
inline constexpr char flex_send_export[]="SiN_GetFlexSendGraphApi";
inline constexpr char flex_recv_export[]="SiN_GetFlexRecvGraphApi";
template<class Table> struct FlexBindings {
    Table* base_animating;
    Table* overlay_vars;
};
using FlexSendBindings=FlexBindings<s::SendTable>;
using FlexRecvBindings=FlexBindings<RecvTable>;
template<class Table> struct FlexView {
    void* handle;
    Table* overlay;
    Table* flex;
    Table* weights;
};
using FlexSendView=FlexView<s::SendTable>;
using FlexRecvView=FlexView<RecvTable>;
using FlexSendApi=GraphApi<FlexSendBindings,FlexSendView>;
using FlexRecvApi=GraphApi<FlexRecvBindings,FlexRecvView>;
using GetFlexSendApi=const FlexSendApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
using GetFlexRecvApi=const FlexRecvApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase699: native dynamic vector descriptor graph. Original 20-byte x86
// CUtlVector header. Host owns backing allocation, element initialization,
// and (client) interpolation unregister/re-register on logical resize.
// Reserve must preserve count and existing elements; resize must set count.
inline constexpr char overlay_send_export[]="SiN_GetOverlaySendGraphApi";
inline constexpr char overlay_recv_export[]="SiN_GetOverlayRecvGraphApi";
inline constexpr int overlay_limit=15;
struct LayerVector {
    unsigned char* memory=nullptr;
    int capacity=0,grow_size=0,count=0;
    unsigned char* elements=nullptr;
};
static_assert(sizeof(LayerVector)==20&&offsetof(LayerVector,count)==12&&offsetof(LayerVector,elements)==16);
using LayerHostOp=int (__cdecl *)(void* context,void* entity,int offset,int stride,int count) noexcept;
struct OverlaySendBindings {void* context;LayerHostOp reserve;};
struct OverlayRecvBindings {void* context;LayerHostOp reserve,resize;};
template<class Table> struct OverlayView {
    void* handle;
    Table* overlay_vars;
    Table* vector_table;
    Table* length_table;
    Table* layer;
};
using OverlaySendView=OverlayView<s::SendTable>;
using OverlayRecvView=OverlayView<RecvTable>;
using OverlaySendApi=GraphApi<OverlaySendBindings,OverlaySendView>;
using OverlayRecvApi=GraphApi<OverlayRecvBindings,OverlayRecvView>;
using GetOverlaySendApi=const OverlaySendApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
using GetOverlayRecvApi=const OverlayRecvApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase700: DT_LocalPlayerExclusive. Borrowed DT_Local and Phase696
// ammo descriptor must outlive this graph. Client invalidation callback
// implements original InvalidatePhysicsRecursive(4) on the host entity;
// it runs before writing changed local velocity. No registration here.
inline constexpr char player_local_send_export[]="SiN_GetPlayerLocalSendGraphApi";
inline constexpr char player_local_recv_export[]="SiN_GetPlayerLocalRecvGraphApi";
using VelocityInvalidation=int (__cdecl *)(void* context,void* entity,int flags) noexcept;
struct PlayerLocalSendBindings {
    s::SendTable* local;
    const s::SendProp* ammo;
    const sin_independent::EntityHandleEntry* entries;
    std::uint32_t entry_count;
};
struct PlayerLocalRecvBindings {
    RecvTable* local;
    const RecvProp* ammo;
    void* context;
    VelocityInvalidation invalidate_velocity;
};
template<class Table> struct PlayerLocalView {void* handle;Table* table;};
using PlayerLocalSendView=PlayerLocalView<s::SendTable>;
using PlayerLocalRecvView=PlayerLocalView<RecvTable>;
using PlayerLocalSendApi=GraphApi<PlayerLocalSendBindings,PlayerLocalSendView>;
using PlayerLocalRecvApi=GraphApi<PlayerLocalRecvBindings,PlayerLocalRecvView>;
using GetPlayerLocalSendApi=const PlayerLocalSendApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
using GetPlayerLocalRecvApi=const PlayerLocalRecvApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase701: public player and nested life-state descriptors. Existing
// combat-character and private-player graphs are borrowed for their full
// lifetime. The host must detach engine/derived graph references first.
inline constexpr char base_player_send_export[]="SiN_GetBasePlayerSendGraphApi";
inline constexpr char base_player_recv_export[]="SiN_GetBasePlayerRecvGraphApi";
struct BasePlayerSendBindings {
    s::SendTable* combat;
    s::SendTable* private_player;
    const sin_independent::EntityHandleEntry* entries;
    std::uint32_t entry_count;
};
struct BasePlayerRecvBindings {RecvTable* combat;RecvTable* private_player;};
template<class Table> struct BasePlayerView {void* handle;Table* player;Table* state;};
using BasePlayerSendView=BasePlayerView<s::SendTable>;
using BasePlayerRecvView=BasePlayerView<RecvTable>;
using BasePlayerSendApi=GraphApi<BasePlayerSendBindings,BasePlayerSendView>;
using BasePlayerRecvApi=GraphApi<BasePlayerRecvBindings,BasePlayerRecvView>;
using GetBasePlayerSendApi=const BasePlayerSendApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
using GetBasePlayerRecvApi=const BasePlayerRecvApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase702: owner-local player subobject, not a full CBasePlayer.
// FovSample supplies the original current-time word and either a
// truncated displayed FOV (if local player available) or fallback default.
inline constexpr char local_data_send_export[]="SiN_GetLocalDataSendGraphApi";
inline constexpr char local_data_recv_export[]="SiN_GetLocalDataRecvGraphApi";
struct FovSample {
    std::uint32_t time_bits;
    int displayed_fov;
    int default_fov;
    std::uint32_t has_displayed_fov;
};
using SampleFov=int (__cdecl *)(void* context,FovSample* out) noexcept;
struct LocalDataSendBindings {
    const sin_independent::EntityHandleEntry* entries;
    std::uint32_t entry_count;
};
struct LocalDataRecvBindings {void* context;SampleFov sample_fov;};
template<class Table> struct LocalDataView {void* handle;Table* table;Table* area_bits;};
using LocalDataSendView=LocalDataView<s::SendTable>;
using LocalDataRecvView=LocalDataView<RecvTable>;
using LocalDataSendApi=GraphApi<LocalDataSendBindings,LocalDataSendView>;
using LocalDataRecvApi=GraphApi<LocalDataRecvBindings,LocalDataRecvView>;
using GetLocalDataSendApi=const LocalDataSendApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
using GetLocalDataRecvApi=const LocalDataRecvApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase703: DT_BaseEntity, AnimTimeMustBeFirst and PredictableId.
// CollisionProperty stays an explicitly borrowed engine/SDK dependency.
// Clock and transform sampling reflect live host state; no new physics
// or prop implementation. Entity-change notifications run after writes.
inline constexpr char entity_send_export[]="SiN_GetBaseEntitySendGraphApi";
inline constexpr char entity_recv_export[]="SiN_GetBaseEntityRecvGraphApi";
struct NetworkClock {int tick;float interval;};
using SampleClock=int (__cdecl *)(void* context,NetworkClock* out) noexcept;
using SampleTransform=int (__cdecl *)(void* context,const void* entity,int angles,std::uint32_t* xyz) noexcept;
using EntityNotify=void (__cdecl *)(void* context,void* entity) noexcept;
struct BaseEntitySendBindings {
    s::SendTable* collision;
    const sin_independent::EntityHandleEntry* entries;
    std::uint32_t entry_count;
    void* context;
    SampleClock clock;
    SampleTransform transform;
};
struct BaseEntityRecvBindings {
    RecvTable* collision;
    void* context;
    SampleClock clock;
    EntityNotify effects_changed,interpolation_changed;
};
template<class Table> struct BaseEntityView {void* handle;Table* entity;Table* anim_time;Table* predictable;};
using BaseEntitySendView=BaseEntityView<s::SendTable>;
using BaseEntityRecvView=BaseEntityView<RecvTable>;
using BaseEntitySendApi=GraphApi<BaseEntitySendBindings,BaseEntitySendView>;
using BaseEntityRecvApi=GraphApi<BaseEntityRecvBindings,BaseEntityRecvView>;
using GetBaseEntitySendApi=const BaseEntitySendApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
using GetBaseEntityRecvApi=const BaseEntityRecvApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
