#pragma once
// Win32 descriptor graph with original property offsets. It is NOT
// registered with the retail engine and does not construct any entity.
// Only use with a host-owned compatible object/table and verified
// live owner/recipient callbacks. Original absolute addresses are evidence.
#include "server_send_datatable_core.h"
#include "weapon_network_contract.h"
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
}
