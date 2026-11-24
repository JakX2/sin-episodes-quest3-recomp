#include "native_weapon_datatables.h"
#include <cstring>
#include <new>
#include <initializer_list>
#if defined(SIN_ROLE_server)
#include "client_known_gamesystem_init.h"
#endif
namespace sin_port::native_weapon_tables {
namespace {
constexpr unsigned max_props=7;
constexpr unsigned ammo_elements=32,weapon_elements=48;
void element_names(char (&names)[weapon_elements][4]) noexcept {
    for(unsigned i=0;i<weapon_elements;++i){
        names[i][0]=static_cast<char>('0'+i/100);
        names[i][1]=static_cast<char>('0'+(i/10)%10);
        names[i][2]=static_cast<char>('0'+i%10);
        names[i][3]=0;
    }
}
template<class Table> bool valid_flex(const Table* base) noexcept {
    return base&&base->name&&std::strcmp(base->name,"DT_BaseFlex")==0&&
        base->count>0&&base->props;
}
template<class G,class V> bool inventory_matches(const G& g,const V& v) noexcept {
    return v.ammo_table==&g.ammo_table&&v.weapons_table==&g.weapons_table&&
        v.combat_local==&g.combat_local&&v.combat_base==&g.combat_base&&v.ammo_property==&g.ammo_property;
}
template<class V> bool inventory_empty(const V& v) noexcept {
    return !v.handle&&!v.ammo_table&&!v.weapons_table&&!v.combat_local&&!v.combat_base&&!v.ammo_property;
}
constexpr unsigned parents[6]={BaseCombat,BaseHL,MachineGun,BaseHL,SelectFire,BaseHL};
// BaseHL->BaseCombat, MachineGun->BaseHL, SelectFire->MachineGun,
// Magnum->BaseHL, AssaultRifle->SelectFire, Scattergun->BaseHL.
template<class Table> bool valid_base(const Table* base) noexcept {
    return base&&base->name&&std::strcmp(base->name,"DT_BaseAnimating")==0&&
        base->count>0&&base->props;
}
template<class V> bool empty(const V& v) noexcept {
    if(v.handle)return false;
    for(auto* t:v.tables)if(t)return false;
    return true;
}
template<class Graph,class V> bool matches(const Graph& graph,const V& v) noexcept {
    for(unsigned i=0;i<TableCount;++i)if(v.tables[i]!=&graph.tables[i])return false;
    return true;
}
#if defined(SIN_ROLE_server)
namespace mem=sin_independent::vgui_layout;
namespace methods=sin_independent::known_game_system;
constexpr std::size_t edict_stride=20,owner_offset=0x494,player_edict_offset=0x16c;
struct ResolvedContext {
    void* engine=nullptr;
    const sin_independent::EntityHandleEntry* entries=nullptr;
    std::uint32_t count=0;
    const void* edicts=nullptr;
    std::uint32_t clients=0;
};
template<class T> bool read_at(const void* p,std::size_t offset,T& out) noexcept {
    const auto address=reinterpret_cast<std::uintptr_t>(p);
    if(!p||address>(static_cast<std::uintptr_t>(-1))-offset)return false;
    const auto* field=reinterpret_cast<const void*>(address+offset);
    if(!mem::Readable(field,sizeof(T)))return false;
    std::memcpy(&out,field,sizeof(T));return true;
}
void* resolve_handle(const ResolvedContext& c,std::uint32_t raw) noexcept {
    if(raw==sin_independent::kInvalidEntityHandle||!c.entries)return nullptr;
    const auto parts=sin_independent::SplitEntityHandle(raw);
    if(parts.entry>=c.count)return nullptr;
    // Check only the bounded entry actually needed by this operation.
    const auto* entry=c.entries+parts.entry;
    if(!mem::Readable(entry,sizeof(*entry)))return nullptr;
    return sin_independent::EntityHandleView(c.entries,c.count).Resolve(raw);
}
bool is_player(void* entity,bool& player) {
    methods::CallResult refused=methods::CallResult::InvalidObject;
    auto* method=methods::VerifiedMethod(entity,72,refused);
    if(!method)return false;
    using Player=bool (__thiscall *)(void*);
    player=reinterpret_cast<Player>(method)(entity);return true;
}
bool player_index(const ResolvedContext& c,void* entity,unsigned& slot) {
    methods::CallResult refused=methods::CallResult::InvalidObject;
    using Index=int (__thiscall *)(void*,const void*);
    const void* edict=nullptr;
    if(!read_at(entity,player_edict_offset,edict)||!edict)return false;
    const auto base=reinterpret_cast<std::uintptr_t>(c.edicts);
    const auto address=reinterpret_cast<std::uintptr_t>(edict);
    if(!base||address<base)return false;
    const auto delta=address-base;
    if(delta%edict_stride)return false;
    const auto candidate=delta/edict_stride;
    if(candidate<1||candidate>c.clients||!mem::Readable(edict,edict_stride))return false;
    auto* index_method=methods::VerifiedMethod(c.engine,18,refused);
    if(!index_method)return false;
    const int actual=reinterpret_cast<Index>(index_method)(c.engine,edict);
    if(actual!=static_cast<int>(candidate))return false;
    slot=static_cast<unsigned>(actual);return true;
}
bool client_slot(const ResolvedContext& c,void* handle_object,unsigned& slot) noexcept {
    methods::CallResult refused=methods::CallResult::InvalidObject;
    auto* base_method=methods::VerifiedMethod(handle_object,63,refused);
    if(!base_method)return false;
    using Base=void* (__thiscall *)(void*);
    try {
        void* entity=reinterpret_cast<Base>(base_method)(handle_object);
        bool player=false;
        return is_player(entity,player)&&player&&player_index(c,entity,slot);
    }catch(...){return false;}
}
void* __cdecl resolved_local(const s::SendProp* prop,const void*,const void* data,
        s::Recipients* recipients,int) {
    if(!recipients)return nullptr;
    // Original success clears all words; failure returns null and
    // does not clear. Port clears also on failure, to avoid stale masks.
    for(auto& word:recipients->words)word=0;
    if(!prop||!prop->extra)return nullptr;
    const auto& context=*static_cast<const ResolvedContext*>(prop->extra);
    std::uint32_t raw=0;
    if(!read_at(data,owner_offset,raw))return nullptr;
    auto* handle=resolve_handle(context,raw);
    unsigned slot=0;
    if(!handle||!client_slot(context,handle,slot))return nullptr;
    const auto bit=slot-1;
    recipients->words[bit/32]=1u<<(bit%32);
    return const_cast<void*>(data);
}
void __cdecl resolved_handle(const s::SendProp* prop,const void*,const void* data,
        s::DVariant* out,int,int) {
    if(!out)return;
    out->words[0]=n::wire_invalid_handle;
    if(!prop||!prop->extra)return;
    const auto& context=*static_cast<const ResolvedContext*>(prop->extra);
    std::uint32_t raw=0;
    if(!read_at(data,0,raw)||!resolve_handle(context,raw))return;
    const auto entry=raw&0xfffu;
    if(entry>0x7ffu)return;
    out->words[0]=entry|(((raw>>12)&0x3ffu)<<11);
}
void __cdecl raw_send(const s::SendProp*,const void*,const void* data,s::DVariant* out,int,int){
    if(data&&out)std::memcpy(&out->words[0],data,4);
}
void __cdecl clip_send(const s::SendProp*,const void*,const void* data,s::DVariant* out,int,int){
    if(data&&out){
        std::uint32_t v=0;std::memcpy(&v,data,4);out->words[0]=v+1u;
    }
}
struct SendGraph {
    s::SendTable tables[TableCount];
    s::SendProp props[TableCount][max_props];
    ResolvedContext context{};
    bool build(const SendBindings& b) noexcept {
        if(!valid_base(b.base_animating)||!b.local_weapon||!b.local_active||!b.owner_handle||
           b.local_weapon==&s::DirectTable||b.local_weapon==&s::PointerTable||
           b.local_active==&s::DirectTable||b.local_active==&s::PointerTable)return false;
        for(unsigned i=0;i<TableCount;++i)
            if(!tables[i].Construct(props[i],table_counts[i],table_names[i]))return false;
        if(!s::MakeDataTableProp(props[BaseCombat][0],"baseclass",0,b.base_animating,&s::DirectTable)||
           !s::MakeDataTableProp(props[BaseCombat][1],"LocalWeaponData",0,&tables[LocalWeapon],b.local_weapon)||
           !s::MakeDataTableProp(props[BaseCombat][2],"LocalActiveWeaponData",0,&tables[LocalActive],b.local_active))return false;
        for(unsigned i=BaseHL;i<TableCount;++i)
            if(!s::MakeDataTableProp(props[i][0],"baseclass",0,&tables[parents[i-BaseHL]],&s::DirectTable))return false;
        for(unsigned i=0;i<n::field_count;++i){
            n::FieldInfo f{};if(!n::owned_api().field(i,&f))return false;
            // FieldInfo.name is a copied buffer: use graph-owned storage.
            std::memcpy(names[i],f.name,sizeof(f.name));
            auto& q=props[f.group][f.slot];
            q.name=names[i];q.type=f.kind==n::FloatBits?1:0;q.bits=static_cast<int>(f.bits);
            q.flags=static_cast<int>(f.flags);q.offset=static_cast<int>(f.server_offset);
            q.scalarProxy=f.kind==n::ClipPlusOne?&clip_send:(f.kind==n::EntityHandle?b.owner_handle:&raw_send);
            // NOSCALE ignores quantization range. Normalize unused
            // low/high/multiplier to zero; not an exact original byte copy.
        }
        return true;
    }
    char names[n::field_count][32]{};
};
int __cdecl create(const SendBindings* b,SendView* out) noexcept {
    if(!b||!out||!empty(*out))return 0;
    auto* g=new(std::nothrow) SendGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    SendView v{};v.handle=g;
    for(unsigned i=0;i<TableCount;++i)v.tables[i]=&g->tables[i];
    *out=v;return 1;
}
int __cdecl destroy(SendView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<SendGraph*>(v->handle);
    if(!matches(*g,*v))return 0;
    for(const auto& t:g->tables)if(t.precalc||t.initialized||t.written)return 0;
    delete g;*v={};return 1;
}
int __cdecl create_resolved(const ResolvedSendBindings* b,SendView* out) noexcept {
    if(!b||!out||!empty(*out)||!b->entries||!b->entry_count||
       b->entry_count>sin_independent::kEntityHandleCapacity||
       !b->edict_base||!b->max_clients||b->max_clients>256||
       !mem::Readable(b->entries,sizeof(*b->entries))||
       !mem::Readable(b->edict_base,edict_stride*(b->max_clients+1u)))return 0;
    methods::CallResult refused=methods::CallResult::InvalidObject;
    if(!methods::VerifiedMethod(b->engine_server,18,refused))return 0;
    const SendBindings native{b->base_animating,&resolved_local,&resolved_local,&resolved_handle};
    if(!create(&native,out))return 0;
    auto* graph=static_cast<SendGraph*>(out->handle);
    graph->context={b->engine_server,b->entries,b->entry_count,b->edict_base,b->max_clients};
    graph->props[BaseCombat][1].extra=&graph->context;
    graph->props[BaseCombat][2].extra=&graph->context;
    graph->props[BaseCombat][6].extra=&graph->context;
    return 1;
}
const SendApi api{version,sizeof(SendApi),&create,&destroy};
const ResolvedSendApi resolved_api{version,sizeof(ResolvedSendApi),&create_resolved,&destroy};
// Original BCC recipient proxy has direct player and separate indirect
// object branch. Unlike weapon local proxies, returns pData even empty.
void* __cdecl combat_local_proxy(const s::SendProp* prop,const void* structure,const void* data,
        s::Recipients* recipients,int) {
    if(!recipients)return const_cast<void*>(data);
    for(auto& word:recipients->words)word=0;
    if(!prop||!prop->extra||!structure)return const_cast<void*>(data);
    const auto& context=*static_cast<const ResolvedContext*>(prop->extra);
    auto* entity=const_cast<void*>(structure);
    try {
        bool player=false;
        if(!is_player(entity,player))return const_cast<void*>(data);
        if(!player){
            methods::CallResult refused=methods::CallResult::InvalidObject;
            const auto getter=methods::VerifiedMethod(entity,76,refused);
            if(!getter)return const_cast<void*>(data);
            using Get=void* (__thiscall *)(void*);
            using Resolve=void* (__thiscall *)(void*,int);
            void* intermediate=reinterpret_cast<Get>(getter)(entity);
            const auto resolve=methods::VerifiedMethod(intermediate,2,refused);
            if(!resolve)return const_cast<void*>(data);
            entity=reinterpret_cast<Resolve>(resolve)(intermediate,0);
            if(!is_player(entity,player)||!player)return const_cast<void*>(data);
        }
        unsigned slot=0;
        if(player_index(context,entity,slot)){
            const auto bit=slot-1;recipients->words[bit/32]=1u<<(bit%32);
        }
    }catch(...){}
    return const_cast<void*>(data);
}
void* __cdecl player_local_proxy(const s::SendProp*,const void*,const void* data,s::Recipients* recipients,int id){
    if(recipients){
        for(auto& word:recipients->words)word=0;
        if(id>=1&&id<=256){
            const auto bit=static_cast<unsigned>(id-1);
            recipients->words[bit/32]=1u<<(bit%32);
        }
    }
    return const_cast<void*>(data);
}
struct InventorySendGraph {
    ResolvedContext context{};
    s::SendTable ammo_table,weapons_table,combat_local,combat_base;
    s::SendProp ammo_property,ammo_template,weapon_template;
    s::SendProp ammo[ammo_elements],weapons[weapon_elements],local_props[2],base_props[3];
    char names[weapon_elements][4]{};
    bool build(const InventorySendBindings& b) noexcept {
        if(!valid_flex(b.base_flex)||!b.entries||!b.entry_count||
           b.entry_count>sin_independent::kEntityHandleCapacity||!b.edict_base||
           !b.max_clients||b.max_clients>256||!mem::Readable(b.entries,sizeof(*b.entries))||
           !mem::Readable(b.edict_base,edict_stride*(b.max_clients+1u)))return false;
        methods::CallResult refused=methods::CallResult::InvalidObject;
        if(!methods::VerifiedMethod(b.engine_server,18,refused))return false;
        context={b.engine_server,b.entries,b.entry_count,b.edict_base,b.max_clients};
        element_names(names);
        if(!ammo_table.Construct(ammo,ammo_elements,"m_iAmmo")||
           !weapons_table.Construct(weapons,weapon_elements,"m_hMyWeapons")||
           !combat_local.Construct(local_props,2,"DT_BCCLocalPlayerExclusive")||
           !combat_base.Construct(base_props,3,"DT_BaseCombatCharacter"))return false;
        ammo_template.name="m_iAmmo";ammo_template.offset=0x64c;
        ammo_template.bits=10;ammo_template.flags=1;ammo_template.scalarProxy=&raw_send;
        weapon_template.name="m_hMyWeapons";weapon_template.offset=0x6cc;
        weapon_template.bits=21;weapon_template.flags=1;weapon_template.scalarProxy=&resolved_handle;
        weapon_template.extra=&context;
        for(unsigned i=0;i<ammo_elements;++i){
            ammo[i]=ammo_template;ammo[i].name=names[i];ammo[i].offset=static_cast<int>(i*4);
            ammo[i].excludeOrParent=ammo_template.name;
        }
        for(unsigned i=0;i<weapon_elements;++i){
            weapons[i]=weapon_template;weapons[i].name=names[i];weapons[i].offset=static_cast<int>(i*4);
            weapons[i].excludeOrParent=weapon_template.name;
        }
        if(!s::MakeDataTableProp(ammo_property,"m_iAmmo",0x64c,&ammo_table,&s::DirectTable)||
           !s::MakeDataTableProp(local_props[1],"m_hMyWeapons",0x6cc,&weapons_table,&s::DirectTable)||
           !s::MakeDataTableProp(base_props[0],"baseclass",0,b.base_flex,&s::DirectTable)||
           !s::MakeDataTableProp(base_props[1],"bcc_localdata",0,&combat_local,&combat_local_proxy))return false;
        ammo_property.arrayProp=&ammo_template;local_props[1].arrayProp=&weapon_template;
        base_props[1].extra=&context;
        local_props[0].name="m_flNextAttack";local_props[0].type=1;
        local_props[0].offset=0x60c;local_props[0].flags=4;local_props[0].scalarProxy=&raw_send;
        base_props[2].name="m_hActiveWeapon";base_props[2].bits=21;base_props[2].flags=1;
        base_props[2].offset=0x78c;base_props[2].scalarProxy=&resolved_handle;base_props[2].extra=&context;
        return true;
    }
};
int __cdecl create_inventory(const InventorySendBindings* b,InventorySendView* out) noexcept {
    if(!b||!out||!inventory_empty(*out)||out->player_local_proxy)return 0;
    auto* g=new(std::nothrow) InventorySendGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    InventorySendView v{};v.handle=g;v.ammo_table=&g->ammo_table;v.weapons_table=&g->weapons_table;
    v.combat_local=&g->combat_local;v.combat_base=&g->combat_base;v.ammo_property=&g->ammo_property;
    v.player_local_proxy=&player_local_proxy;*out=v;return 1;
}
int __cdecl destroy_inventory(InventorySendView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<InventorySendGraph*>(v->handle);
    if(!inventory_matches(*g,*v)||v->player_local_proxy!=&player_local_proxy)return 0;
    for(const auto* t:{&g->ammo_table,&g->weapons_table,&g->combat_local,&g->combat_base})
        if(t->precalc||t->initialized||t->written)return 0;
    delete g;*v={};return 1;
}
const InventorySendApi inventory_api{version,sizeof(InventorySendApi),&create_inventory,&destroy_inventory};
#elif defined(SIN_ROLE_client)
void __cdecl raw_recv(const RecvData* data,void*,void* out){
    if(data&&out)std::memcpy(out,&data->value.words[0],4);
}
void __cdecl clip_recv(const RecvData* data,void*,void* out){
    if(data&&out){
        const auto v=data->value.words[0]-1u;std::memcpy(out,&v,4);
    }
}
void __cdecl handle_recv(const RecvData* data,void*,void* out){
    if(data&&out){
        const auto wire=data->value.words[0];
        // Valid network inputs only. Do not reconstruct a live identity
        // from an unrepresentable received value.
        const auto v=wire>=n::wire_invalid_handle?n::invalid_handle:((wire>>11)<<12)|(wire&0x7ffu);
        std::memcpy(out,&v,4);
    }
}
void __cdecl direct_recv(const RecvProp*,void** out,void* data,int){
    if(out)*out=data;
}
void nested(RecvProp& p,const char* name,RecvTable* t) noexcept {
    p.name=name;p.type=5;p.table=t;p.table_proxy=&direct_recv;
}
struct RecvGraph {
    RecvTable tables[TableCount];
    RecvProp props[TableCount][max_props];
    char names[n::field_count][32]{};
    bool build(const RecvBindings& b) noexcept {
        if(!valid_base(b.base_animating))return false;
        for(unsigned i=0;i<TableCount;++i){
            tables[i].props=props[i];tables[i].count=table_counts[i];tables[i].name=table_names[i];
        }
        nested(props[BaseCombat][0],"baseclass",b.base_animating);
        nested(props[BaseCombat][1],"LocalWeaponData",&tables[LocalWeapon]);
        nested(props[BaseCombat][2],"LocalActiveWeaponData",&tables[LocalActive]);
        for(unsigned i=BaseHL;i<TableCount;++i)nested(props[i][0],"baseclass",&tables[parents[i-BaseHL]]);
        for(unsigned i=0;i<n::field_count;++i){
            n::FieldInfo f{};if(!n::owned_api().field(i,&f))return false;
            std::memcpy(names[i],f.name,sizeof(f.name));
            auto& q=props[f.group][f.slot];
            q.name=names[i];q.type=f.kind==n::FloatBits?1:0;q.offset=static_cast<int>(f.client_offset);
            q.proxy=f.kind==n::ClipPlusOne?&clip_recv:(f.kind==n::EntityHandle?&handle_recv:&raw_recv);
            // Original client factories use flags=0 for these props;
            // server wire flags are not copied into RecvProp.
        }
        return true;
    }
};
int __cdecl create(const RecvBindings* b,RecvView* out) noexcept {
    if(!b||!out||!empty(*out))return 0;
    auto* g=new(std::nothrow) RecvGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    RecvView v{};v.handle=g;
    for(unsigned i=0;i<TableCount;++i)v.tables[i]=&g->tables[i];
    *out=v;return 1;
}
int __cdecl destroy(RecvView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<RecvGraph*>(v->handle);
    if(!matches(*g,*v))return 0;
    for(const auto& t:g->tables)if(t.decoder||t.initialized||t.in_main_list)return 0;
    delete g;*v={};return 1;
}
const RecvApi api{version,sizeof(RecvApi),&create,&destroy};
struct InventoryRecvGraph {
    RecvTable ammo_table,weapons_table,combat_local,combat_base;
    RecvProp ammo_property,ammo[ammo_elements],weapons[weapon_elements],local_props[2],base_props[3];
    char names[weapon_elements][4]{};
    bool build(const InventoryRecvBindings& b) noexcept {
        if(!valid_flex(b.base_flex))return false;
        element_names(names);
        ammo_table={ammo,ammo_elements,nullptr,"m_iAmmo"};
        weapons_table={weapons,weapon_elements,nullptr,"m_hMyWeapons"};
        combat_local={local_props,2,nullptr,"DT_BCCLocalPlayerExclusive"};
        combat_base={base_props,3,nullptr,"DT_BaseCombatCharacter"};
        for(unsigned i=0;i<ammo_elements;++i){
            ammo[i].name=names[i];ammo[i].offset=static_cast<int>(i*4);
            ammo[i].proxy=&raw_recv;ammo[i].parent_array="m_iAmmo";
        }
        for(unsigned i=0;i<weapon_elements;++i){
            weapons[i].name=names[i];weapons[i].offset=static_cast<int>(i*4);
            weapons[i].proxy=&handle_recv;weapons[i].parent_array="m_hMyWeapons";
        }
        nested(ammo_property,"m_iAmmo",&ammo_table);ammo_property.offset=0xc00;
        nested(local_props[1],"m_hMyWeapons",&weapons_table);local_props[1].offset=0xc80;
        // Original RecvPropArray3 parent has no array_prop pointer.
        local_props[0].name="m_flNextAttack";local_props[0].type=1;
        local_props[0].offset=0xbf8;local_props[0].proxy=&raw_recv;
        nested(base_props[0],"baseclass",b.base_flex);
        nested(base_props[1],"bcc_localdata",&combat_local);
        base_props[2].name="m_hActiveWeapon";base_props[2].offset=0xd40;base_props[2].proxy=&handle_recv;
        return true;
    }
};
int __cdecl create_inventory(const InventoryRecvBindings* b,InventoryRecvView* out) noexcept {
    if(!b||!out||!inventory_empty(*out))return 0;
    auto* g=new(std::nothrow) InventoryRecvGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    *out={g,&g->ammo_table,&g->weapons_table,&g->combat_local,&g->combat_base,&g->ammo_property};return 1;
}
int __cdecl destroy_inventory(InventoryRecvView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<InventoryRecvGraph*>(v->handle);
    if(!inventory_matches(*g,*v))return 0;
    for(const auto* t:{&g->ammo_table,&g->weapons_table,&g->combat_local,&g->combat_base})
        if(t->decoder||t->initialized||t->in_main_list)return 0;
    delete g;*v={};return 1;
}
const InventoryRecvApi inventory_api{version,sizeof(InventoryRecvApi),&create_inventory,&destroy_inventory};
#else
#error Native weapon graph requires one explicit module role
#endif
}
}
#if defined(SIN_ROLE_server)
#pragma comment(linker,"/EXPORT:SiN_GetWeaponSendGraphApi=_SiN_GetWeaponSendGraphApi")
#pragma comment(linker,"/EXPORT:SiN_GetResolvedWeaponSendGraphApi=_SiN_GetResolvedWeaponSendGraphApi")
#pragma comment(linker,"/EXPORT:SiN_GetInventorySendGraphApi=_SiN_GetInventorySendGraphApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::SendApi* __cdecl
SiN_GetWeaponSendGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(SendApi)?&api:nullptr;
}
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ResolvedSendApi* __cdecl
SiN_GetResolvedWeaponSendGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ResolvedSendApi)?&resolved_api:nullptr;
}
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::InventorySendApi* __cdecl
SiN_GetInventorySendGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(InventorySendApi)?&inventory_api:nullptr;
}
#else
#pragma comment(linker,"/EXPORT:SiN_GetWeaponRecvGraphApi=_SiN_GetWeaponRecvGraphApi")
#pragma comment(linker,"/EXPORT:SiN_GetInventoryRecvGraphApi=_SiN_GetInventoryRecvGraphApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::RecvApi* __cdecl
SiN_GetWeaponRecvGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(RecvApi)?&api:nullptr;
}
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::InventoryRecvApi* __cdecl
SiN_GetInventoryRecvGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(InventoryRecvApi)?&inventory_api:nullptr;
}
#endif
