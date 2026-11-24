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

// Phase704: original SiN CHL2_Player, private HL2Local, and PlayerHand.
// Names reflect retail network tables, not a modern HL2 SDK substitute.
inline constexpr char derived_player_send_export[]="SiN_GetDerivedPlayerSendGraphApi";
inline constexpr char derived_player_recv_export[]="SiN_GetDerivedPlayerRecvGraphApi";
struct DerivedPlayerSendBindings {
    s::SendTable* player;
    s::SendTable* entity;
    const sin_independent::EntityHandleEntry* entries;
    std::uint32_t entry_count;
};
struct DerivedPlayerRecvBindings {RecvTable* player;RecvTable* entity;};
template<class Table> struct DerivedPlayerView {void* handle;Table* player;Table* local;Table* hand;};
using DerivedPlayerSendView=DerivedPlayerView<s::SendTable>;
using DerivedPlayerRecvView=DerivedPlayerView<RecvTable>;
using DerivedPlayerSendApi=GraphApi<DerivedPlayerSendBindings,DerivedPlayerSendView>;
using DerivedPlayerRecvApi=GraphApi<DerivedPlayerRecvBindings,DerivedPlayerRecvView>;
using GetDerivedPlayerSendApi=const DerivedPlayerSendApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
using GetDerivedPlayerRecvApi=const DerivedPlayerRecvApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase705: exact PE32 class record layouts, independently owned subset.
// Input order is the original client registration order for these 15
// classes. Caller supplies real entity creation functions, not metadata.
// This is intentionally NOT a complete retail class-list publication.
inline constexpr char server_class_export[]="SiN_GetServerClassSubsetApi";
inline constexpr char client_class_export[]="SiN_GetClientClassSubsetApi";
enum ClassSlot : unsigned {
    ClCombatWeapon,ClAnimating,ClOverlay,ClCombatCharacter,ClEntity,ClFlex,
    ClBasePlayer,ClHLCombatWeapon,ClMachineGun,ClSelectFire,ClHand,ClPlayer,
    ClMagnum,ClAssault,ClScatter,ClassSubsetCount
};
inline constexpr const char* class_names[ClassSubsetCount]={
    "CBaseCombatWeapon","CBaseAnimating","CBaseAnimatingOverlay","CBaseCombatCharacter",
    "CBaseEntity","CBaseFlex","CBasePlayer","CBaseHLCombatWeapon","CHLMachineGun",
    "CHLSelectFireMachineGun","CPlayerHand","CHL2_Player","CWeaponMagnum",
    "CWeaponAssaultRifle","CWeaponScatterGun"
};
inline constexpr const char* class_table_names[ClassSubsetCount]={
    "DT_BaseCombatWeapon","DT_BaseAnimating","DT_BaseAnimatingOverlay","DT_BaseCombatCharacter",
    "DT_BaseEntity","DT_BaseFlex","DT_BasePlayer","DT_BaseHLCombatWeapon","DT_HLMachineGun",
    "DT_HLSelectFireMachineGun","DT_PlayerHand","DT_HL2_Player","DT_WeaponMagnum",
    "DT_WeaponAssaultRifle","DT_WeaponScatterGun"
};
inline constexpr int class_table_counts[ClassSubsetCount]={7,18,2,3,24,7,15,1,1,1,1,6,1,1,1};
struct ServerClassRecord {
    const char* name=nullptr;
    s::SendTable* table=nullptr;
    ServerClassRecord* next=nullptr;
    int class_id=0;
    int instance_baseline=0xffff;
};
// Opaque IClientNetworkable pointer; creation ABI is two cdecl ints.
using CreateClientEntity=void* (__cdecl *)(int entity_number,int serial_number);
using CreateClientEvent=void* (__cdecl *)();
struct ClientClassRecord {
    CreateClientEntity create=nullptr;
    CreateClientEvent create_event=nullptr;
    const char* name=nullptr;
    RecvTable* table=nullptr;
    ClientClassRecord* next=nullptr;
    int class_id=0;
};
static_assert(sizeof(ServerClassRecord)==20&&offsetof(ServerClassRecord,table)==4&&
    offsetof(ServerClassRecord,next)==8&&offsetof(ServerClassRecord,class_id)==12&&
    offsetof(ServerClassRecord,instance_baseline)==16);
static_assert(sizeof(ClientClassRecord)==24&&offsetof(ClientClassRecord,name)==8&&
    offsetof(ClientClassRecord,table)==12&&offsetof(ClientClassRecord,next)==16&&
    offsetof(ClientClassRecord,class_id)==20);
struct ServerClassBindings {s::SendTable* tables[ClassSubsetCount];};
struct ClientClassBindings {RecvTable* tables[ClassSubsetCount];CreateClientEntity creators[ClassSubsetCount];};
template<class Record> struct ClassView {void* handle;Record* head;Record* records;unsigned count;};
using ServerClassView=ClassView<ServerClassRecord>;
using ClientClassView=ClassView<ClientClassRecord>;
using ServerClassApi=GraphApi<ServerClassBindings,ServerClassView>;
using ClientClassApi=GraphApi<ClientClassBindings,ClientClassView>;
using GetServerClassApi=const ServerClassApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
using GetClientClassApi=const ClientClassApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase706: one allocation owns all reconstructed network descriptors
// and class records. All host services and collision schema are borrowed;
// caller keeps them alive and detaches every engine reference before
// destruction. Never publish this incomplete subset as the retail list.
inline constexpr char aggregate_send_export[]="SiN_GetNetworkSendAggregateApi";
inline constexpr char aggregate_recv_export[]="SiN_GetNetworkRecvAggregateApi";
inline constexpr unsigned aggregate_table_count=35;
struct AggregateSendBindings {
    BaseEntitySendBindings entity;
    OverlaySendBindings overlay;
    void* engine_server;
    const void* edict_base;
    std::uint32_t max_clients;
};
struct AggregateRecvBindings {
    BaseEntityRecvBindings entity;
    OverlayRecvBindings overlay;
    LocalDataRecvBindings local;
    void* velocity_context;
    VelocityInvalidation invalidate_velocity;
    CreateClientEntity creators[ClassSubsetCount];
};
template<class Table,class Record> struct AggregateView {
    void* handle;
    Record* class_head;
    Record* class_records;
    unsigned class_count;
    // Stable inventory in shared role order, not flattened wire indices.
    Table* tables[aggregate_table_count];
};
using AggregateSendView=AggregateView<s::SendTable,ServerClassRecord>;
using AggregateRecvView=AggregateView<RecvTable,ClientClassRecord>;
using AggregateSendApi=GraphApi<AggregateSendBindings,AggregateSendView>;
using AggregateRecvApi=GraphApi<AggregateRecvBindings,AggregateRecvView>;
using GetAggregateSendApi=const AggregateSendApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
using GetAggregateRecvApi=const AggregateRecvApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase708: five original client entity creator wrapper behaviors.
// Host supplies compatible native construction (including vtables and
// nontrivial base/interpolation state) and lifetime management. No
// original fixed addresses are used. This is not a full entity class.
inline constexpr char client_creator_export[]="SiN_GetClientEntityCreatorApi";
enum class CreatorCleanup : int {Raw=0,Constructed=1,InitAttempted=2};
using AllocateEntity=void* (__cdecl *)(void* context,unsigned slot,unsigned bytes) noexcept;
using ConstructEntity=int (__cdecl *)(void* context,unsigned slot,void* object,unsigned bytes) noexcept;
using CleanupEntity=void (__cdecl *)(void* context,unsigned slot,void* object,unsigned bytes,CreatorCleanup stage) noexcept;
struct ClientCreatorBindings {
    void* context;
    AllocateEntity allocate;
    ConstructEntity construct;
    CleanupEntity cleanup;
};
struct ClientCreatorApi {
    std::uint32_t version,size;
    unsigned (__cdecl *object_size)(unsigned slot) noexcept;
    // Returns IClientNetworkable subobject (+8), host owns object.
    // A failed construct callback must unwind partial construction
    // before returning; cleanup(Raw) will then release allocation.
    void* (__cdecl *create)(const ClientCreatorBindings*,unsigned slot,int index,int serial) noexcept;
    // Leaf scalar writes only, invoked at the correct point by the
    // host constructor. Does not install vtables or register state.
    int (__cdecl *hl2local_defaults)(void* player,unsigned bytes) noexcept;
    int (__cdecl *player_tail_defaults)(void* player,unsigned bytes) noexcept;
    int (__cdecl *weapon_tail_defaults)(void* weapon,unsigned bytes) noexcept;
};
using GetClientCreatorApi=const ClientCreatorApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase709: network-index subset of the original client entity list.
// Original has 4096 entries, 0..2047 reserved for networked entities;
// client-only free-list allocation is deliberately not implemented here.
inline constexpr char client_registry_export[]="SiN_GetClientEntityRegistryApi";
inline constexpr unsigned client_network_limit=2048;
struct ClientRegistryEntry {
    void* entity;
    std::uint32_t serial;
    ClientRegistryEntry* prev;
    ClientRegistryEntry* next;
};
static_assert(sizeof(ClientRegistryEntry)==16&&offsetof(ClientRegistryEntry,serial)==4&&
              offsetof(ClientRegistryEntry,prev)==8&&offsetof(ClientRegistryEntry,next)==12);
using RegistryNotify=void (__cdecl *)(void* context,void* entity,std::uint32_t handle) noexcept;
struct ClientRegistryBindings {
    const std::uint16_t* initial_serials; // 2048 seeds, copied during create
    void* context;
    RegistryNotify added,removed;
};
struct ClientRegistryView {
    void* handle;
    const ClientRegistryEntry* entries;
};
struct ClientRegistryState {
    const ClientRegistryEntry* head;
    const ClientRegistryEntry* tail;
    unsigned count;
};
struct ClientRegistryApi {
    std::uint32_t version,size;
    int (__cdecl *create)(const ClientRegistryBindings*,ClientRegistryView*) noexcept;
    int (__cdecl *destroy)(ClientRegistryView*) noexcept;
    // Entity is the original primary handle interface, not +4 or +8.
    // Calls its verified vslot1 setter with packed handle by reference.
    int (__cdecl *insert)(ClientRegistryView*,void*,int index,int serial,std::uint32_t* handle) noexcept;
    int (__cdecl *remove)(ClientRegistryView*,std::uint32_t handle) noexcept;
    void* (__cdecl *resolve)(const ClientRegistryView*,std::uint32_t handle) noexcept;
    int (__cdecl *state)(const ClientRegistryView*,ClientRegistryState*) noexcept;
};
using GetClientRegistryApi=const ClientRegistryApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase710: recovered shared client slot37 initialization sequence.
// Only the original networked classes with the Phase708 object sizes.
// The host supplies collision registration and the live interpolation
// duration policy; no HL2 physics or model implementation is included.
inline constexpr char client_init_export[]="SiN_GetClientEntityInitApi";
struct ClientInterpEntry {
    std::uint32_t kind;
    void* data;
    void* watcher;
};
struct ClientInterpMap {
    const ClientInterpEntry* entries;
    int capacity,grow,count;
    const ClientInterpEntry* elements;
    std::uint32_t reserved;
    const ClientInterpMap* parent;
};
static_assert(sizeof(ClientInterpEntry)==12&&sizeof(ClientInterpMap)==28&&
    offsetof(ClientInterpMap,count)==12&&offsetof(ClientInterpMap,parent)==24);
using AttachClientCollision=int (__cdecl *)(void* context,void* collision_subobject) noexcept;
using SampleInterpDuration=int (__cdecl *)(void* context,void* object,int flags,float* duration) noexcept;
enum class ClientInitStage:int {CollisionAttempted=1,InterpolationAttempted=2};
using RollbackClientInit=void (__cdecl *)(void* context,void* object,ClientInitStage stage) noexcept;
struct ClientInitBindings {
    ClientRegistryView* registry;
    void* context;
    AttachClientCollision attach_collision;
    SampleInterpDuration duration;
    RollbackClientInit rollback;
};
struct ClientInitApi {
    std::uint32_t version,size;
    // 1 success, 0 rejected/rolled back, -1 rollback could not remove
    // the registry entry (host must retain object and recover handle).
    // On success or retained failure, writes packed handle to output.
    // Host callbacks must not mutate registry or invalidate entity/vtables.
    int (__cdecl *initialize)(const ClientInitBindings*,unsigned slot,void* object,
        int index,int serial,std::uint32_t* handle) noexcept;
};
using GetClientInitApi=const ClientInitApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase711: explicit single-threaded class creator binding. The original
// cdecl class ABI has no context argument, so only one session is active
// per client DLL. Do not retain thunk pointers after unbind. Retail
// getters remain disabled; this is the reconstructed 15-class subset.
inline constexpr char spawn_session_export[]="SiN_GetClientSpawnSessionApi";
struct ClientSpawnBindings {
    AggregateRecvView* aggregate;
    ClientCreatorBindings creator;
    ClientInitBindings init;
};
struct ClientSpawnView {void* handle;};
struct ClientSpawnRecord {
    void* primary;
    void* networkable;
    std::uint32_t handle;
    unsigned slot;
    // 1: initialized, 0: retained after incomplete rollback.
    unsigned initialized;
};
struct ClientSpawnApi {
    std::uint32_t version,size;
    int (__cdecl *bind)(const ClientSpawnBindings*,ClientSpawnView*) noexcept;
    // Only a live matching generation may be released. Removes registry
    // entry first, then host cleanup. Successful initialized objects use
    // InitAttempted cleanup; host owns remaining service teardown.
    int (__cdecl *release)(ClientSpawnView*,std::uint32_t handle) noexcept;
    int (__cdecl *record)(const ClientSpawnView*,unsigned index,ClientSpawnRecord*) noexcept;
    int (__cdecl *unbind)(ClientSpawnView*) noexcept;
};
using GetClientSpawnApi=const ClientSpawnApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase713: extension for the remaining SiN weapons and enemy classes.
// Separate from the version-1 15-class aggregate so its ABI stays stable.
// The two lists are subsets, not a complete retail engine registry.
inline constexpr char extra_send_export[]="SiN_GetExtraEntitySendApi";
inline constexpr char extra_recv_export[]="SiN_GetExtraEntityRecvApi";
enum ExtraEntitySlot : unsigned {ExAI,ExBludgeon,ExElite,ExGrunt,ExHologram,ExSMG,ExShockwave,ExGrenade,ExChaingun,ExNPCPistol,ExMelee,ExtraEntityCount};
inline constexpr const char* extra_class_names[ExtraEntityCount]={"CAI_BaseNPC","CBaseHLBludgeonWeapon","CNPC_EliteGrunt","CNPC_GruntBase","CNPC_Hologram","CWeaponSMG","CQuadShockwave","CWeaponGrenade","CWeaponChaingun","CWeaponNPCPistol","CWeaponMelee"};
inline constexpr const char* extra_table_names[ExtraEntityCount]={"DT_AI_BaseNPC","DT_BaseHLBludgeonWeapon","DT_NPC_EliteGrunt","DT_NPC_GruntBase","DT_NPC_Hologram","DT_WeaponSMG","DT_QuadShockwave","DT_WeaponGrenade","DT_WeaponChaingun","DT_WeaponNPCPistol","DT_WeaponMelee"};
inline constexpr int extra_table_counts[ExtraEntityCount]={7,1,1,3,3,1,2,1,1,1,1};
struct ExtraSendBindings {AggregateSendView* base;};
struct ExtraRecvBindings {AggregateRecvView* base;CreateClientEntity creators[ExtraEntityCount];};
template<class Table,class Record> struct ExtraEntityView {
    void* handle;Record* head;Record* records;Table* tables[ExtraEntityCount];
};
using ExtraSendView=ExtraEntityView<s::SendTable,ServerClassRecord>;
using ExtraRecvView=ExtraEntityView<RecvTable,ClientClassRecord>;
using ExtraSendApi=GraphApi<ExtraSendBindings,ExtraSendView>;
using ExtraRecvApi=GraphApi<ExtraRecvBindings,ExtraRecvView>;
using GetExtraSendApi=const ExtraSendApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
using GetExtraRecvApi=const ExtraRecvApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase714: original npc_hologram behavior on host-owned PE32 objects.
// No rendering, collision implementation or original fixed-address calls.
inline constexpr char hologram_server_export[]="SiN_GetHologramServerApi";
inline constexpr char hologram_client_export[]="SiN_GetHologramClientApi";
enum class HologramCommand : unsigned {ResetAfterSpawn,On,Off,Pulse,Think,Activate,StartLoop};
enum class HologramBaseCall : unsigned {Think,Activate};
enum class HologramSound : unsigned {On,Off,Loop,StopLoop};
using HologramClock=int (__cdecl *)(void* context,float* now) noexcept;
using HologramDirty=int (__cdecl *)(void* context,void* object,unsigned offset) noexcept;
using HologramAudio=int (__cdecl *)(void* context,void* object,HologramSound) noexcept;
using HologramBase=int (__cdecl *)(void* context,void* object,HologramBaseCall) noexcept;
// One control per live object. Calls must be serialized; callbacks may
// inspect but must not destroy, relocate or reenter the same object.
struct HologramControl {void* object;unsigned bytes;unsigned busy=0,blocked=0;};
struct HologramServerBindings {
    void* context;HologramClock clock;HologramDirty dirty;
    HologramAudio audio;HologramBase base;
};
struct HologramServerApi {
    std::uint32_t version,size;
    // 1 completed (including inapplicable pulse); 0 preflight refusal;
    // -1 callback failed, possibly after side effects. Block until host
    // reconciles real state and explicitly acknowledges.
    int (__cdecl *dispatch)(HologramControl*,const HologramServerBindings*,HologramCommand,float pulse_duration) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
struct HologramFlicker {unsigned visible=0;}; // one shared instance per client runtime
struct HologramClientApi {
    std::uint32_t version,size;
    // Object fields are read from the actual RecvTable layout. Advances
    // at most one threshold per call, as in the original.
    int (__cdecl *visibility)(void* object,unsigned bytes,float now,HologramFlicker*,unsigned* visible) noexcept;
};
using GetHologramServerApi=const HologramServerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
using GetHologramClientApi=const HologramClientApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase715: SiN grunt's client death-effect override. Host implements
// ordinary Source ragdoll, bodygroup and detached model services.
inline constexpr char grunt_death_export[]="SiN_GetGruntDeathApi";
enum class GruntHeadPath:unsigned {None,EffectRecord,TimedModel};
struct GruntHeadEffect {
    const char* model;
    void* source;
    float position[3],velocity[3],angles[3];
    float lifetime; // 4.0 only on TimedModel path
    float orientation[3]; // EffectRecord source vslot10
    int group;
    int entity_index; // -1 on EffectRecord path
};
struct GruntDeathBindings {
    void* context;
    int (__cdecl *base)(void*,void* source,int argument,void** ragdoll) noexcept;
    int (__cdecl *suppressed)(void*,unsigned* value) noexcept;
    int (__cdecl *find_group)(void*,void* ragdoll,const char*,int* group) noexcept;
    int (__cdecl *group_count)(void*,void* ragdoll,int group,int* count) noexcept;
    int (__cdecl *set_group)(void*,void* ragdoll,int group,int value) noexcept;
    int (__cdecl *random_float)(void*,float low,float high,float* value) noexcept;
    int (__cdecl *random_int)(void*,int low,int high,int* value) noexcept;
    int (__cdecl *orientation)(void*,void* source,float* xyz) noexcept;
    int (__cdecl *position)(void*,void* source,float* xyz) noexcept;
    int (__cdecl *emit)(void*,GruntHeadPath,const GruntHeadEffect*) noexcept;
};
struct GruntDeathResult {void* ragdoll;GruntHeadPath path;};
struct GruntDeathApi {
    std::uint32_t version,size;
    // Returns 1 on completed path, including all original skips;
    // -1 uncertain callback failure; 0 rejected preflight.
    int (__cdecl *run)(HologramControl*,const GruntDeathBindings*,int argument,GruntDeathResult*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetGruntDeathApi=const GruntDeathApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase716: map-specific keypad classes and VGuiScreen wire metadata.
// Rendering, input widgets and game event delivery remain host services.
inline constexpr char keypad_send_export[]="SiN_GetKeypadSendApi";
inline constexpr char keypad_recv_export[]="SiN_GetKeypadRecvApi";
inline constexpr char keypad_server_export[]="SiN_GetKeypadServerApi";
inline constexpr char keypad_client_export[]="SiN_GetKeypadClientApi";
enum KeypadTableSlot:unsigned {KeyScreen,KeyCode,KeyProp,KeyTableCount};
inline constexpr const char* keypad_classes[]={"CVGuiScreen","CKeypad","CPropKeypad"};
inline constexpr const char* keypad_tables[]={"DT_VGuiScreen","DTKeypad","DT_CPropKeypad"};
inline constexpr int keypad_counts[]={7,3,3};
struct KeypadSendBindings {AggregateSendView* base;};
struct KeypadRecvBindings {AggregateRecvView* base;CreateClientEntity creators[KeyTableCount];};
template<class Table,class Record> struct KeypadView {
    void* handle;Record* head;Record* records;Table* tables[KeyTableCount];
};
using KeypadSendView=KeypadView<s::SendTable,ServerClassRecord>;
using KeypadRecvView=KeypadView<RecvTable,ClientClassRecord>;
using KeypadSendApi=GraphApi<KeypadSendBindings,KeypadSendView>;
using KeypadRecvApi=GraphApi<KeypadRecvBindings,KeypadRecvView>;
using GetKeypadSendApi=const KeypadSendApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
using GetKeypadRecvApi=const KeypadRecvApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
enum class KeypadEvent:unsigned {Correct,Incorrect};
struct KeypadServerBindings {
    void* context;
    int (__cdecl *dirty)(void*,void*,unsigned) noexcept;
    int (__cdecl *output)(void*,void*,unsigned offset,void* activator,void* caller,float delay) noexcept;
};
struct KeypadServerApi {
    std::uint32_t version,size;
    int (__cdecl *enable)(HologramControl*,const KeypadServerBindings*,unsigned enabled) noexcept;
    // Event listener receives literal game-event names, not external
    // authentication or security credentials. Original listener has
    // no disabled gate. Unrelated names are ignored.
    int (__cdecl *event)(HologramControl*,const KeypadServerBindings*,const char*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
struct KeypadClientBindings {
    void* context;
    int (__cdecl *append_command)(void*,int command) noexcept;
    int (__cdecl *read_text)(void*,char* buffer,unsigned capacity) noexcept;
    int (__cdecl *create_event)(void*,const char* name,unsigned force,void** event) noexcept;
    int (__cdecl *fire_event)(void*,void* event,unsigned local_only) noexcept;
    int (__cdecl *clear_text)(void*) noexcept;
    int (__cdecl *base_command)(void*,int command) noexcept;
};
struct KeypadClientApi {
    std::uint32_t version,size;
    // Null object = unresolved keypad, base command still runs.
    int (__cdecl *command)(void* object,unsigned bytes,const KeypadClientBindings*,int command) noexcept;
};
using GetKeypadServerApi=const KeypadServerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
using GetKeypadClientApi=const KeypadClientApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase717: CPropKeypad event, string output and display path.
inline constexpr char prop_keypad_server_export[]="SiN_GetPropKeypadServerApi";
inline constexpr char prop_keypad_client_export[]="SiN_GetPropKeypadClientApi";
struct PropKeypadServerBindings {
    void* context;
    int (__cdecl *dirty)(void*,void*,unsigned) noexcept;
    int (__cdecl *entity_index)(void*,void*,int*) noexcept;
    int (__cdecl *event_index)(void*,void*,int*) noexcept;
    int (__cdecl *event_command)(void*,void*,char*,unsigned) noexcept;
    int (__cdecl *output)(void*,void*,unsigned,const char*,void*,void*,float) noexcept;
    int (__cdecl *sound)(void*,void*,const char*) noexcept;
    int (__cdecl *base_event)(void*,void*,void*) noexcept;
};
struct PropKeypadServerApi {
    std::uint32_t version,size;
    int (__cdecl *enable)(HologramControl*,const PropKeypadServerBindings*,unsigned) noexcept;
    // SetInput/Clear: null means empty, copy at most 30 chars.
    int (__cdecl *set_input)(HologramControl*,const PropKeypadServerBindings*,const char*) noexcept;
    int (__cdecl *event)(HologramControl*,const PropKeypadServerBindings*,const char*,void*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
struct PropKeypadClientBindings {
    void* context;
    int (__cdecl *create_event)(void*,const char*,unsigned,void**) noexcept;
    int (__cdecl *entity_index)(void*,void*,int*) noexcept;
    int (__cdecl *set_int)(void*,void*,const char*,int) noexcept;
    int (__cdecl *set_string)(void*,void*,const char*,const char*) noexcept;
    int (__cdecl *fire_event)(void*,void*,unsigned) noexcept;
    int (__cdecl *base_command)(void*,const char*) noexcept;
    int (__cdecl *display)(void*,const char*) noexcept;
    int (__cdecl *base_tick)(void*) noexcept;
};
struct PropKeypadClientApi {
    std::uint32_t version,size;
    // Object is a host-resolved/type-checked CPropKeypad or null.
    int (__cdecl *command)(void*,unsigned,const PropKeypadClientBindings*,const char*) noexcept;
    int (__cdecl *tick)(void*,unsigned,const PropKeypadClientBindings*) noexcept;
};
using GetPropKeypadServerApi=const PropKeypadServerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
using GetPropKeypadClientApi=const PropKeypadClientApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase718: original CPropPhone table and server polling behavior.
// Host owns the existing BaseAnimating, screen attachment, serial-checked
// lookup, KeyValues phonebook, sound and think scheduler services.
inline constexpr char phone_send_export[]="SiN_GetPropPhoneSendApi";
inline constexpr char phone_recv_export[]="SiN_GetPropPhoneRecvApi";
inline constexpr char phone_server_export[]="SiN_GetPropPhoneServerApi";
struct PhoneSendBindings {AggregateSendView* base;};
struct PhoneRecvBindings {AggregateRecvView* base;CreateClientEntity creator;};
template<class Table,class Record> struct PhoneView {void* handle;Table* table;Record* record;};
using PhoneSendView=PhoneView<s::SendTable,ServerClassRecord>;
using PhoneRecvView=PhoneView<RecvTable,ClientClassRecord>;
using PhoneSendApi=GraphApi<PhoneSendBindings,PhoneSendView>;
using PhoneRecvApi=GraphApi<PhoneRecvBindings,PhoneRecvView>;
using GetPhoneSendApi=const PhoneSendApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
using GetPhoneRecvApi=const PhoneRecvApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
struct PhoneBindings {
    void* context;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *schedule)(void*,void* phone,float time) noexcept;
    // A successful callback may return the invalid handle if no screen
    // was allocated; the original then returns without scheduling.
    int (__cdecl *create_screen)(void*,void* phone,const char* classname,std::uint32_t*) noexcept;
    int (__cdecl *resolve_screen)(void*,std::uint32_t,HologramControl**) noexcept;
    // Existing host attaches keypad to the named attachment, applies
    // panelname, size/flags and original spawn service in order.
    int (__cdecl *configure_screen)(void*,void* phone,void* screen,const char* attachment,const char* panel,float width,float height) noexcept;
    int (__cdecl *remove_screen)(void*,std::uint32_t,void* screen) noexcept;
    int (__cdecl *lookup)(void*,const void* phonebook,const char* digits,unsigned* found,const char** sound) noexcept;
    int (__cdecl *sound)(void*,void* phone,const char* name) noexcept;
    PropKeypadServerBindings keypad;
};
struct PhoneServerApi {
    std::uint32_t version,size;
    int (__cdecl *defaults)(void* phone,unsigned bytes) noexcept;
    int (__cdecl *enable)(HologramControl*,const PhoneBindings*,unsigned enabled) noexcept;
    int (__cdecl *think)(HologramControl*,const PhoneBindings*) noexcept;
    int (__cdecl *dial)(HologramControl*,const PhoneBindings*,const char* number) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetPhoneServerApi=const PhoneServerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase719: CAntigenDispenser's original healing and use controller.
// Host owns base entity services, serial validation, ordinary health
// changes, pose lookup, sound and steam entity creation/removal.
inline constexpr char dispenser_export[]="SiN_GetAntigenDispenserApi";
enum class DispenserSound:unsigned {UseLoop,StopLoop,Stop,Empty};
struct DispenserUseObservation {
    unsigned is_player;
    int health,max_health;
    unsigned within_use_predicate;
    std::uint32_t handle;
};
struct DispenserBindings {
    void* context;
    int (__cdecl *observe_use)(void*,void* dispenser,void* activator,DispenserUseObservation*) noexcept;
    int (__cdecl *resolve)(void*,std::uint32_t handle,void** entity) noexcept;
    // Host updates normal absolute transforms and returns positions.
    int (__cdecl *position)(void*,void* entity,float xyz[3]) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *heal)(void*,void* user,int amount,unsigned flags,int* accepted) noexcept;
    int (__cdecl *sound)(void*,void* dispenser,DispenserSound) noexcept;
    // Host performs the original env_steam creation/attachment setup
    // and publishes handle through this output; null allocation is OK.
    int (__cdecl *create_steam)(void*,void* dispenser,std::uint32_t*) noexcept;
    int (__cdecl *remove)(void*,void* entity) noexcept;
    int (__cdecl *install_use_think)(void*,void* dispenser) noexcept;
    int (__cdecl *schedule)(void*,void* dispenser,float time) noexcept;
    int (__cdecl *pose)(void*,void* dispenser,const char* name,float value) noexcept;
    int (__cdecl *advance_animation)(void*,void* dispenser) noexcept;
};
struct DispenserApi {
    std::uint32_t version,size;
    // Only verified leaf fields, after base constructor; no vtable or
    // output-object construction. Host owns complete entity lifetime.
    int (__cdecl *defaults)(void*,unsigned bytes) noexcept;
    int (__cdecl *use)(HologramControl*,const DispenserBindings*,void* activator) noexcept;
    int (__cdecl *think_use)(HologramControl*,const DispenserBindings*) noexcept;
    int (__cdecl *stop_spray)(HologramControl*,const DispenserBindings*) noexcept;
    int (__cdecl *update_pose)(HologramControl*,const DispenserBindings*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetDispenserApi=const DispenserApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase720: dispenser container insertion/ejection and spawn state.
// Source handles entity construction, transform, bodygroup and game
// rules callbacks; the port owns original scalar transitions and order.
inline constexpr char dispenser_container_export[]="SiN_GetAntigenContainerApi";
enum class ContainerNotice:unsigned {Inserted,Ejected};
enum class ContainerSound:unsigned {Load,Eject};
struct ContainerObservation {void* container;std::uint32_t flags;float amount;};
struct ContainerBindings {
    DispenserBindings dispenser;
    int (__cdecl *cast_container)(void*,void* entity,ContainerObservation*) noexcept;
    int (__cdecl *bodygroup)(void*,void* object,int group,int value) noexcept;
    int (__cdecl *notice)(void*,void* object,void* container,ContainerNotice) noexcept;
    int (__cdecl *sound)(void*,void* object,ContainerSound) noexcept;
    int (__cdecl *output)(void*,void* object,unsigned offset,void* activator,void* caller,float delay) noexcept;
    // Host creates prop_antigen_explosive_barrel at dispenser transform
    // and invokes original spawn service. Reports compatible container.
    int (__cdecl *create_ejected)(void*,void* object,const char* classname,void** container) noexcept;
    // Original setter at 222ff6e0: amount and percentFull pose, unclamped.
    int (__cdecl *container_amount)(void*,void* container,float amount,float pose) noexcept;
    // Original ejection helper 22139c90, arg1 (entity message).
    int (__cdecl *eject_message)(void*,void* object,int value) noexcept;
    int (__cdecl *clear_think)(void*,void* object) noexcept;
};
struct ContainerApi {
    std::uint32_t version,size;
    int (__cdecl *spawn_state)(HologramControl*,const ContainerBindings*) noexcept;
    int (__cdecl *touch)(HologramControl*,const ContainerBindings*,void* entity) noexcept;
    int (__cdecl *eject)(HologramControl*,const ContainerBindings*) noexcept;
};
using GetContainerApi=const ContainerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase721: item_dna_scanner. Host supplies base spawn/animation,
// class-name and player queries, audio and ordinary Source map outputs.
inline constexpr char dna_scanner_export[]="SiN_GetDnaScannerApi";
struct DnaScannerBindings {
    void* context;
    int (__cdecl *classname)(void*,void* entity,const char** name) noexcept;
    int (__cdecl *is_player)(void*,void* entity,unsigned*) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *sound)(void*,void* scanner,const char* name) noexcept;
    int (__cdecl *output)(void*,void* scanner,unsigned offset,void* activator,void* caller,float delay) noexcept;
};
struct DnaScannerApi {
    std::uint32_t version,size;
    // Original post-base spawn fields only. Does not create outputs
    // or install a raw original callback address.
    int (__cdecl *spawn_state)(HologramControl*) noexcept;
    int (__cdecl *capabilities)(const void*,unsigned bytes,std::uint32_t base,std::uint32_t*) noexcept;
    int (__cdecl *touch)(HologramControl*,const DnaScannerBindings*,void* other) noexcept;
    int (__cdecl *use)(HologramControl*,const DnaScannerBindings*,void* activator) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetDnaScannerApi=const DnaScannerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Phase722: original CHealthKit, CHealthVial and CMegaHealth pickup
// leaf methods. Base item touch eligibility and lifetime remain host work.
inline constexpr char health_pickup_export[]="SiN_GetHealthPickupApi";
enum class HealthPickupKind:unsigned {Kit,Vial,Mega};
struct HealthPickupBindings {
    void* context;
    int (__cdecl *skill_amount)(void*,HealthPickupKind,float*) noexcept;
    // Original player vslot108, before each changed max-health write.
    int (__cdecl *dirty_player)(void*,void* player) noexcept;
    // Actual original player slot57 has FLOAT amount and DWORD flags.
    int (__cdecl *heal)(void*,void* player,float amount,unsigned flags,int* accepted) noexcept;
    int (__cdecl *pickup_message)(void*,void* player,const char* message,const char* classname) noexcept;
    int (__cdecl *pickup_sound)(void*,void* item,void* player,const char* sound) noexcept;
    int (__cdecl *should_respawn)(void*,void* item,unsigned*) noexcept;
    int (__cdecl *respawn)(void*,void* item) noexcept;
    int (__cdecl *remove)(void*,void* item) noexcept;
};
struct HealthPickupApi {
    std::uint32_t version,size;
    // Owns original CHL2_Player max-health setter and overhealth byte.
    int (__cdecl *set_max)(HologramControl* player,const HealthPickupBindings*,int maximum) noexcept;
    // 1 completed (accepted indicates consumed), 0 refused preflight,
    // -1 uncertain host failure. Player and item lifetime remain stable
    // through all callbacks, removal must be deferred until return.
    int (__cdecl *pickup)(HologramControl* item,HologramControl* player,
        const HealthPickupBindings*,HealthPickupKind,unsigned* accepted) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetHealthPickupApi=const HealthPickupApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
