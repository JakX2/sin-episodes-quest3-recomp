#include "native_weapon_datatables.h"
#include <cstring>
#include <new>
#include <initializer_list>
#include "client_known_gamesystem_init.h"
namespace sin_port::native_weapon_tables {
namespace {
constexpr unsigned max_props=7;
template<class B> bool player_local_deps(const B& b,int count,int ammo_offset) noexcept {
    return b.local&&b.local->name&&std::strcmp(b.local->name,"DT_Local")==0&&
        b.local->count==count&&b.local->props&&b.ammo&&b.ammo->name&&
        std::strcmp(b.ammo->name,"m_iAmmo")==0&&b.ammo->type==5&&b.ammo->offset==ammo_offset&&
        b.ammo->table&&b.ammo->table->props&&b.ammo->table->count==32&&b.ammo->table->name&&
        std::strcmp(b.ammo->table->name,"m_iAmmo")==0;
}
template<class V> bool player_local_empty(const V& v) noexcept {return !v.handle&&!v.table;}
// Native callbacks borrow synchronous host-owned entity/vector memory.
// Readability is a range check, not proof of live object provenance.
bool layer_header(const void* entity,int offset,LayerVector& v) noexcept {
    if(!entity)return false;
    const auto base=reinterpret_cast<std::uintptr_t>(entity);
    if(base>static_cast<std::uintptr_t>(-1)-static_cast<unsigned>(offset))return false;
    const auto* address=reinterpret_cast<const void*>(base+static_cast<unsigned>(offset));
    if(!sin_independent::vgui_layout::Readable(address,sizeof(v)))return false;
    std::memcpy(&v,address,sizeof(v));
    return v.count>=0&&v.capacity>=v.count&&(!v.capacity||v.memory);
}
bool layer_reserve(const void* entity,int offset,int stride,void* context,LayerHostOp reserve) noexcept {
    LayerVector before{},after{};
    if(!layer_header(entity,offset,before))return false;
    // Original ensure callback initializes dormant slots even if storage
    // already has enough capacity. It never changes logical count.
    if(before.count<overlay_limit&&!reserve(context,const_cast<void*>(entity),offset,stride,overlay_limit))return false;
    return layer_header(entity,offset,after)&&after.count==before.count&&after.capacity>=overlay_limit&&after.memory;
}
void* layer_address(const void* entity,int offset,int stride,int index) noexcept {
    LayerVector v{};
    if(index<0||index>=overlay_limit||!layer_header(entity,offset,v)||index>=v.capacity||!v.memory)return nullptr;
    const auto base=reinterpret_cast<std::uintptr_t>(v.memory);
    const auto delta=static_cast<unsigned>(index*stride);
    if(base>static_cast<std::uintptr_t>(-1)-delta)return nullptr;
    auto* address=reinterpret_cast<void*>(base+delta);
    return sin_independent::vgui_layout::Readable(address,static_cast<unsigned>(stride))?address:nullptr;
}
template<class V> bool overlay_empty(const V& v) noexcept {
    return !v.handle&&!v.overlay_vars&&!v.vector_table&&!v.length_table&&!v.layer;
}
template<class G,class V> bool overlay_matches(const G& g,const V& v) noexcept {
    return v.overlay_vars==&g.overlay&&v.vector_table==&g.vector&&v.length_table==&g.length&&v.layer==&g.layer;
}
template<class B> bool flex_deps(const B& b) noexcept {
    return b.base_animating&&b.base_animating->name&&
        std::strcmp(b.base_animating->name,"DT_BaseAnimating")==0&&
        b.base_animating->count==18&&b.base_animating->props&&
        b.overlay_vars&&b.overlay_vars->name&&std::strcmp(b.overlay_vars->name,"DT_OverlayVars")==0&&
        b.overlay_vars->count==1&&b.overlay_vars->props;
}
template<class V> bool flex_empty(const V& v) noexcept {
    return !v.handle&&!v.overlay&&!v.flex&&!v.weights;
}
template<class G,class V> bool flex_matches(const G& g,const V& v) noexcept {
    return v.overlay==&g.overlay&&v.flex==&g.flex&&v.weights==&g.weights;
}
template<class T> bool anim_base_valid(const T* t) noexcept {
    return t&&t->name&&std::strcmp(t->name,"DT_BaseEntity")==0&&t->props&&t->count==24;
}
template<class V> bool anim_empty(const V& v) noexcept {
    return !v.handle&&!v.base_animating&&!v.server_anim_data&&!v.pose&&!v.controllers;
}
template<class G,class V> bool anim_matches(const G& g,const V& v) noexcept {
    return v.base_animating==&g.base&&v.server_anim_data==&g.cycle&&v.pose==&g.pose&&v.controllers==&g.controllers;
}
constexpr unsigned ammo_elements=32,weapon_elements=48;
template<std::size_t N> void element_names(char (&names)[N][4]) noexcept {
    static_assert(N<=1000);
    for(unsigned i=0;i<N;++i){
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

// Original 0x2213eb50 uses slot49, then returned object's byte +0x460.
void* __cdecl server_anim_proxy(const s::SendProp*,const void* structure,const void* data,s::Recipients*,int){
    methods::CallResult refused=methods::CallResult::InvalidObject;
    const auto method=methods::VerifiedMethod(const_cast<void*>(structure),49,refused);
    if(!method)return nullptr;
    using Get=void* (__thiscall *)(void*);
    try {
        auto* entity=reinterpret_cast<Get>(method)(const_cast<void*>(structure));
        unsigned char client_side=0;
        return read_at(entity,0x460,client_side)&&!client_side?const_cast<void*>(data):nullptr;
    }catch(...){return nullptr;}
}
void __cdecl byte_send(const s::SendProp*,const void*,const void* data,s::DVariant* out,int,int){
    if(data&&out)out->words[0]=*static_cast<const unsigned char*>(data);
}
void __cdecl vector_send(const s::SendProp*,const void*,const void* data,s::DVariant* out,int,int){
    if(data&&out)std::memcpy(out->words,data,12);
}
void anim_int(s::SendProp& p,const char* name,int offset,int bits,int flags,bool byte=false) noexcept {
    p.name=name;p.offset=offset;p.bits=bits;p.flags=flags;p.scalarProxy=byte?&byte_send:&raw_send;
}
void anim_float(s::SendProp& p,const char* name,int offset,int bits,int flags,float low,float high,float multiplier) noexcept {
    p.name=name;p.type=1;p.offset=offset;p.bits=bits;p.flags=flags;
    p.low=low;p.high=high;p.multiplier=multiplier;p.scalarProxy=&raw_send;
}
struct AnimSendGraph {
    ResolvedContext context{};
    s::SendTable base,cycle,pose,controllers;
    s::SendProp props[18],cycle_prop,pose_props[24],controller_props[4],pose_template,controller_template;
    char names[weapon_elements][4]{};
    bool build(const AnimSendBindings& b) noexcept {
        if(!anim_base_valid(b.base_entity)||!b.entries||!b.entry_count||
           b.entry_count>sin_independent::kEntityHandleCapacity||!mem::Readable(b.entries,sizeof(*b.entries)))return false;
        context.entries=b.entries;context.count=b.entry_count;
        element_names(names);
        if(!base.Construct(props,18,"DT_BaseAnimating")||
           !cycle.Construct(&cycle_prop,1,"DT_ServerAnimationData")||
           !pose.Construct(pose_props,24,"m_flPoseParameter")||
           !controllers.Construct(controller_props,4,"m_flEncodedController"))return false;
        if(!s::MakeDataTableProp(props[0],"baseclass",0,b.base_entity,&s::DirectTable)||
           !s::MakeDataTableProp(props[7],"m_flPoseParameter",0x3f0,&pose,&s::DirectTable)||
           !s::MakeDataTableProp(props[10],"m_flEncodedController",0x450,&controllers,&s::DirectTable)||
           !s::MakeDataTableProp(props[17],"serveranimdata",0,&cycle,&server_anim_proxy))return false;
        anim_int(props[1],"m_nForceBone",0x3a0,8,0);
        props[2].name="m_vecForce";props[2].offset=0x3a4;props[2].type=2;props[2].flags=4;
        props[2].scalarProxy=&vector_send; // NOSCALE: unused range normalized
        anim_int(props[3],"m_nSkin",0x3b0,10,0);
        anim_int(props[4],"m_nBody",0x3b4,32,0);
        anim_int(props[5],"m_nHitboxSet",0x3b8,2,1);
        anim_float(props[6],"m_flModelWidthScale",0x3bc,6,0x10,1.0f/64,1,64);
        anim_float(pose_template,"m_flPoseParameter",0x3f0,11,0,0,1,2047);
        props[7].arrayProp=&pose_template;
        for(unsigned i=0;i<24;++i){
            pose_props[i]=pose_template;pose_props[i].name=names[i];
            pose_props[i].offset=static_cast<int>(i*4);pose_props[i].excludeOrParent=pose_template.name;
        }
        anim_int(props[8],"m_nSequence",0x3ec,10,1);
        anim_float(props[9],"m_flPlaybackRate",0x3c0,8,0x10,-3.9375f,12,16);
        anim_float(controller_template,"m_flEncodedController",0x450,11,8,0,1.0f-1.0f/2048,2048);
        props[10].arrayProp=&controller_template;
        for(unsigned i=0;i<4;++i){
            controller_props[i]=controller_template;controller_props[i].name=names[i];
            controller_props[i].offset=static_cast<int>(i*4);controller_props[i].excludeOrParent=controller_template.name;
        }
        anim_int(props[11],"m_bClientSideAnimation",0x460,1,1,true);
        anim_int(props[12],"m_bClientSideFrameReset",0x461,1,1,true);
        anim_int(props[13],"m_nNewSequenceParity",0x464,3,0x401);
        anim_int(props[14],"m_nResetEventsParity",0x468,3,0x401);
        anim_int(props[15],"m_nMuzzleFlashParity",0x46c,2,0x401,true);
        anim_int(props[16],"m_hLightingOrigin",0x470,21,5);
        props[16].scalarProxy=&resolved_handle;props[16].extra=&context;
        anim_float(cycle_prop,"m_flCycle",0x3e8,15,8,0,1.0f-1.0f/32768,32768);
        return true;
    }
};

int __cdecl create_anim(const AnimSendBindings* b,AnimSendView* out) noexcept {
    if(!b||!out||!anim_empty(*out))return 0;
    auto* g=new(std::nothrow) AnimSendGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    *out={g,&g->base,&g->cycle,&g->pose,&g->controllers};return 1;
}
int __cdecl destroy_anim(AnimSendView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<AnimSendGraph*>(v->handle);
    if(!anim_matches(*g,*v))return 0;
    for(const auto* t:{&g->base,&g->cycle,&g->pose,&g->controllers})
        if(t->precalc||t->initialized||t->written)return 0;
    delete g;*v={};return 1;
}
const AnimSendApi anim_api{version,sizeof(AnimSendApi),&create_anim,&destroy_anim};

struct FlexSendGraph {
    s::SendTable overlay,flex,weights;
    s::SendProp overlay_props[2],flex_props[7],weight_props[64],weight_template;
    char names[64][4]{};
    bool build(const FlexSendBindings& b) noexcept {
        if(!flex_deps(b))return false;
        element_names(names);
        if(!overlay.Construct(overlay_props,2,"DT_BaseAnimatingOverlay")||
           !flex.Construct(flex_props,7,"DT_BaseFlex")||
           !weights.Construct(weight_props,64,"m_flexWeight"))return false;
        if(!s::MakeDataTableProp(overlay_props[0],"baseclass",0,b.base_animating,&s::DirectTable)||
           !s::MakeDataTableProp(overlay_props[1],"overlay_vars",0,b.overlay_vars,&s::DirectTable)||
           !s::MakeDataTableProp(flex_props[0],"baseclass",0,&overlay,&s::DirectTable)||
           !s::MakeDataTableProp(flex_props[1],"m_flexWeight",0x4a8,&weights,&s::DirectTable))return false;
        anim_float(weight_template,"m_flexWeight",0x4a8,12,8,0,1.0f-1.0f/4096,4096);
        flex_props[1].arrayProp=&weight_template;
        for(unsigned i=0;i<64;++i){
            weight_props[i]=weight_template;weight_props[i].name=names[i];
            weight_props[i].offset=static_cast<int>(i*4);weight_props[i].excludeOrParent=weight_template.name;
        }
        anim_int(flex_props[2],"m_blinktoggle",0x5b4,1,1);
        flex_props[3].name="m_viewtarget";flex_props[3].offset=0x5a8;
        flex_props[3].type=2;flex_props[3].flags=2;flex_props[3].scalarProxy=&vector_send;
        // COORD/NOSCALE omit quantized range: unused fields normalized.
        anim_float(flex_props[4],"m_vecViewOffset[0]",0x37c,0,4,0,0,0);
        anim_float(flex_props[5],"m_vecViewOffset[1]",0x380,0,4,0,0,0);
        anim_float(flex_props[6],"m_vecViewOffset[2]",0x384,0,4,0,0,0);
        return true;
    }
};

int __cdecl create_flex(const FlexSendBindings* b,FlexSendView* out) noexcept {
    if(!b||!out||!flex_empty(*out))return 0;
    auto* g=new(std::nothrow) FlexSendGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    *out={g,&g->overlay,&g->flex,&g->weights};return 1;
}
int __cdecl destroy_flex(FlexSendView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<FlexSendGraph*>(v->handle);
    if(!flex_matches(*g,*v))return 0;
    for(const auto* t:{&g->overlay,&g->flex,&g->weights})
        if(t->precalc||t->initialized||t->written)return 0;
    delete g;*v={};return 1;
}
const FlexSendApi flex_api{version,sizeof(FlexSendApi),&create_flex,&destroy_flex};

constexpr int layer_offset=0x494,layer_stride=0x40;
// Original 0x2225f2f0 reads logical count, clamps upper bound.
void __cdecl layer_length_send(const s::SendProp*,const void* entity,const void*,s::DVariant* out,int,int){
    if(!out)return;
    LayerVector v{};
    out->words[0]=layer_header(entity,layer_offset,v)?static_cast<unsigned>(v.count>overlay_limit?overlay_limit:v.count):0u;
}
// Original 0x2225f2d0 ensures capacity before returning pData.
void* __cdecl layer_reserve_send(const s::SendProp* p,const void* entity,const void* data,s::Recipients*,int){
    if(!p||!p->extra)return nullptr;
    const auto& b=*static_cast<const OverlaySendBindings*>(p->extra);
    return layer_reserve(entity,layer_offset,layer_stride,b.context,b.reserve)?const_cast<void*>(data):nullptr;
}
// Original 0x2225f370 uses stored stride field as element index.
void* __cdecl layer_element_send(const s::SendProp* p,const void* entity,const void*,s::Recipients*,int){
    return p?layer_address(entity,layer_offset,layer_stride,p->stride):nullptr;
}
struct OverlaySendGraph {
    OverlaySendBindings binding{};
    s::SendTable overlay,vector,length,layer;
    s::SendProp parent,children[16],length_prop,fields[5];
    char names[15][4]{};
    bool build(const OverlaySendBindings& b) noexcept {
        if(!b.reserve)return false;
        binding=b;element_names(names);
        if(!overlay.Construct(&parent,1,"DT_OverlayVars")||
           !vector.Construct(children,16,"_ST_m_AnimOverlay_15")||
           !length.Construct(&length_prop,1,"_LPT_m_AnimOverlay_15")||
           !layer.Construct(fields,5,"DT_Animationlayer"))return false;
        if(!s::MakeDataTableProp(parent,"m_AnimOverlay",0,&vector,&s::DirectTable)||
           !s::MakeDataTableProp(children[0],"lengthproxy",0,&length,&layer_reserve_send))return false;
        children[0].extra=&binding;
        anim_int(length_prop,"lengthprop15",0,4,1);
        length_prop.scalarProxy=&layer_length_send;length_prop.extra=&binding;
        for(int i=0;i<overlay_limit;++i){
            auto& p=children[i+1];
            if(!s::MakeDataTableProp(p,names[i],0,&layer,&layer_element_send))return false;
            p.stride=i;p.flags=0x200;p.extra=&binding;
        }
        anim_int(fields[0],"sequence",8,10,1);
        anim_float(fields[1],"cycle",12,15,8,0,1.0f-1.0f/32768,32768);
        anim_float(fields[2],"prevcycle",16,15,8,0,1.0f-1.0f/32768,32768);
        anim_float(fields[3],"weight",24,8,0,0,1,255);
        anim_int(fields[4],"order",52,4,1);
        return true;
    }
};

int __cdecl create_overlay(const OverlaySendBindings* b,OverlaySendView* out) noexcept {
    if(!b||!out||!overlay_empty(*out))return 0;
    auto* g=new(std::nothrow) OverlaySendGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    *out={g,&g->overlay,&g->vector,&g->length,&g->layer};return 1;
}
int __cdecl destroy_overlay(OverlaySendView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<OverlaySendGraph*>(v->handle);
    if(!overlay_matches(*g,*v))return 0;
    for(const auto* t:{&g->overlay,&g->vector,&g->length,&g->layer})
        if(t->precalc||t->initialized||t->written)return 0;
    delete g;*v={};return 1;
}
const OverlaySendApi overlay_api{version,sizeof(OverlaySendApi),&create_overlay,&destroy_overlay};

void* __cdecl array_noop(const s::SendProp*,const void*,const void*,s::Recipients*,int){
    // Original 0x2225e5b0 is a bare ret; this callback is never used
    // as a datatable proxy. Return null rather than undefined register data.
    return nullptr;
}
struct PlayerLocalSendGraph {
    ResolvedContext context{};
    s::SendTable table;
    s::SendProp props[27];
    bool build(const PlayerLocalSendBindings& b) noexcept {
        if(!player_local_deps(b,48,0x64c)||!b.ammo->arrayProp||!b.entries||!b.entry_count||
           b.entry_count>sin_independent::kEntityHandleCapacity||!mem::Readable(b.entries,sizeof(*b.entries)))return false;
        context.entries=b.entries;context.count=b.entry_count;
        if(!table.Construct(props,27,"DT_LocalPlayerExclusive")||
           !s::MakeDataTableProp(props[0],"m_Local",0x794,b.local,&s::DirectTable))return false;
        anim_float(props[1],"m_vecViewOffset[0]",0x37c,8,0,-32,32,255.0f/64);
        anim_float(props[2],"m_vecViewOffset[1]",0x380,8,0,-32,32,255.0f/64);
        anim_float(props[3],"m_vecViewOffset[2]",0x384,10,0x400,0,128,1023.0f/128);
        anim_float(props[4],"m_flFriction",0x2a8,8,8,0,4.0f-4.0f/256,64);
        props[5]=*b.ammo;
        anim_int(props[6],"m_fOnTarget",0x94c,2,1,true);
        anim_int(props[7],"m_bOnUseTarget",0x94d,2,1,true);
        anim_int(props[8],"m_nTickBase",0x1274,32,0x400);
        anim_int(props[9],"m_nNextThinkTick",0x74,32,0);
        const auto handle=[&](unsigned i,const char* name,int offset){
            // Original 0x2227eb70 forwards its fourth stack arg (4)
            // OR 1 as the flags to SendPropInt, not the fifth arg.
            anim_int(props[i],name,offset,21,5);
            props[i].scalarProxy=&resolved_handle;props[i].extra=&context;
        };
        handle(10,"m_hLastWeapon",0x1280);handle(11,"m_hGroundEntity",0x238);
        anim_float(props[12],"m_vecVelocity[0]",0x2c8,20,0x400,-2048,2048,1048575.0f/4096);
        anim_float(props[13],"m_vecVelocity[1]",0x2cc,20,0x400,-2048,2048,1048575.0f/4096);
        anim_float(props[14],"m_vecVelocity[2]",0x2d0,16,0x400,-2048,2048,65535.0f/4096);
        anim_float(props[15],"m_vecBaseVelocity",0x244,20,0,-1000,1000,1048575.0f/2000);
        props[15].type=2;props[15].scalarProxy=&vector_send;
        handle(16,"m_hViewModel",0xbdc);
        // Original 0x2225ee70 creates an unlinked DPT_Array descriptor.
        // Engine preprocessing links the immediately preceding element.
        // Do not confuse this with numbered Array3 or pretend it is linked.
        props[17].name="m_hViewModel";props[17].type=4;props[17].elements=2;
        props[17].stride=4;props[17].tableProxy=&array_noop;
        handle(18,"m_hConstraintEntity",0x12a8);
        props[19].name="m_vecConstraintCenter";props[19].offset=0x12ac;
        props[19].type=2;props[19].flags=4;props[19].scalarProxy=&vector_send;
        anim_float(props[20],"m_flConstraintRadius",0x12b8,0,4,0,0,0);
        anim_float(props[21],"m_flConstraintWidth",0x12bc,0,4,0,0,0);
        anim_float(props[22],"m_flConstraintSpeedFactor",0x12c0,0,4,0,0,0);
        anim_float(props[23],"m_fViewConstraint",0x12c4,0,4,0,0,0);
        anim_float(props[24],"m_flDeathTime",0x9d8,0,4,0,0,0);
        anim_int(props[25],"m_nWaterLevel",0x298,2,1);
        anim_float(props[26],"m_flLaggedMovementValue",0x12c8,0,4,0,0,0);
        return true;
    }
};

int __cdecl create_player_local(const PlayerLocalSendBindings* b,PlayerLocalSendView* out) noexcept {
    if(!b||!out||!player_local_empty(*out))return 0;
    auto* g=new(std::nothrow) PlayerLocalSendGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    *out={g,&g->table};return 1;
}
int __cdecl destroy_player_local(PlayerLocalSendView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<PlayerLocalSendGraph*>(v->handle);
    if(v->table!=&g->table||g->table.precalc||g->table.initialized||g->table.written)return 0;
    delete g;*v={};return 1;
}
const PlayerLocalSendApi player_local_api{version,sizeof(PlayerLocalSendApi),&create_player_local,&destroy_player_local};
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

void __cdecl byte_recv(const RecvData* data,void*,void* out){
    if(data&&out)*static_cast<unsigned char*>(out)=static_cast<unsigned char>(data->value.words[0]);
}
void __cdecl vector_recv(const RecvData* data,void*,void* out){
    if(data&&out)std::memcpy(out,data->value.words,12);
}
void anim_recv(RecvProp& p,const char* name,int offset,int type=0,RecvProxy proxy=&raw_recv) noexcept {
    p.name=name;p.offset=offset;p.type=type;p.proxy=proxy;
}
struct AnimRecvGraph {
    RecvTable base,cycle,pose,controllers;
    RecvProp props[18],cycle_prop,pose_props[24],controller_props[4];
    char names[weapon_elements][4]{};
    bool build(const AnimRecvBindings& b) noexcept {
        if(!anim_base_valid(b.base_entity))return false;
        element_names(names);
        base={props,18,nullptr,"DT_BaseAnimating"};cycle={&cycle_prop,1,nullptr,"DT_ServerAnimationData"};
        pose={pose_props,24,nullptr,"m_flPoseParameter"};controllers={controller_props,4,nullptr,"m_flEncodedController"};
        nested(props[0],"baseclass",b.base_entity);
        anim_recv(props[1],"m_nSequence",0x6e0);
        anim_recv(props[2],"m_nForceBone",0x510);
        anim_recv(props[3],"m_vecForce",0x504,2,&vector_recv);
        anim_recv(props[4],"m_nSkin",0x4a0);
        anim_recv(props[5],"m_nBody",0x4a4);
        anim_recv(props[6],"m_nHitboxSet",0x4a8);
        anim_recv(props[7],"m_flModelWidthScale",0x544,1);
        nested(props[8],"m_flPoseParameter",&pose);props[8].offset=0x548;
        anim_recv(props[9],"m_flPlaybackRate",0x4c8,1);
        nested(props[10],"m_flEncodedController",&controllers);props[10].offset=0x66c;
        for(unsigned i=0;i<24;++i){
            anim_recv(pose_props[i],names[i],static_cast<int>(i*4),1);
            pose_props[i].parent_array="m_flPoseParameter";
        }
        for(unsigned i=0;i<4;++i){
            anim_recv(controller_props[i],names[i],static_cast<int>(i*4),1);
            controller_props[i].parent_array="m_flEncodedController";
        }
        anim_recv(props[11],"m_bClientSideAnimation",0x6b0,0,&byte_recv);
        anim_recv(props[12],"m_bClientSideFrameReset",0x4f4,0,&byte_recv);
        anim_recv(props[13],"m_nNewSequenceParity",0x6b4);
        anim_recv(props[14],"m_nResetEventsParity",0x6b8);
        anim_recv(props[15],"m_nMuzzleFlashParity",0x784,0,&byte_recv);
        anim_recv(props[16],"m_hLightingOrigin",0x780,0,&handle_recv);
        nested(props[17],"serveranimdata",&cycle);
        anim_recv(cycle_prop,"m_flCycle",0x6e4,1);
        return true;
    }
};

int __cdecl create_anim(const AnimRecvBindings* b,AnimRecvView* out) noexcept {
    if(!b||!out||!anim_empty(*out))return 0;
    auto* g=new(std::nothrow) AnimRecvGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    *out={g,&g->base,&g->cycle,&g->pose,&g->controllers};return 1;
}
int __cdecl destroy_anim(AnimRecvView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<AnimRecvGraph*>(v->handle);
    if(!anim_matches(*g,*v))return 0;
    for(const auto* t:{&g->base,&g->cycle,&g->pose,&g->controllers})
        if(t->decoder||t->initialized||t->in_main_list)return 0;
    delete g;*v={};return 1;
}
const AnimRecvApi anim_api{version,sizeof(AnimRecvApi),&create_anim,&destroy_anim};

struct FlexRecvGraph {
    RecvTable overlay,flex,weights;
    RecvProp overlay_props[2],flex_props[7],weight_props[64];
    char names[64][4]{};
    bool build(const FlexRecvBindings& b) noexcept {
        if(!flex_deps(b))return false;
        element_names(names);
        overlay={overlay_props,2,nullptr,"DT_BaseAnimatingOverlay"};
        flex={flex_props,7,nullptr,"DT_BaseFlex"};weights={weight_props,64,nullptr,"m_flexWeight"};
        nested(overlay_props[0],"baseclass",b.base_animating);
        nested(overlay_props[1],"overlay_vars",b.overlay_vars);
        nested(flex_props[0],"baseclass",&overlay);
        nested(flex_props[1],"m_flexWeight",&weights);flex_props[1].offset=0x860;
        for(unsigned i=0;i<64;++i){
            anim_recv(weight_props[i],names[i],static_cast<int>(i*4),1);
            weight_props[i].parent_array="m_flexWeight";
        }
        anim_recv(flex_props[2],"m_blinktoggle",0xac0);
        anim_recv(flex_props[3],"m_viewtarget",0x824,2,&vector_recv);
        anim_recv(flex_props[4],"m_vecViewOffset[0]",0xe0,1);
        anim_recv(flex_props[5],"m_vecViewOffset[1]",0xe4,1);
        anim_recv(flex_props[6],"m_vecViewOffset[2]",0xe8,1);
        return true;
    }
};

int __cdecl create_flex(const FlexRecvBindings* b,FlexRecvView* out) noexcept {
    if(!b||!out||!flex_empty(*out))return 0;
    auto* g=new(std::nothrow) FlexRecvGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    *out={g,&g->overlay,&g->flex,&g->weights};return 1;
}
int __cdecl destroy_flex(FlexRecvView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<FlexRecvGraph*>(v->handle);
    if(!flex_matches(*g,*v))return 0;
    for(const auto* t:{&g->overlay,&g->flex,&g->weights})
        if(t->decoder||t->initialized||t->in_main_list)return 0;
    delete g;*v={};return 1;
}
const FlexRecvApi flex_api{version,sizeof(FlexRecvApi),&create_flex,&destroy_flex};

constexpr int layer_offset=0x7a4,layer_stride=0x20;
// Original 0x24127100 forwards length to entity-specific resize,
// including interpolation vector teardown/rebuild (0x240c3500).
void __cdecl layer_length_recv(const RecvData* d,void* entity,void*){
    if(!d||!d->property||!d->property->extra||d->value.words[0]>overlay_limit)return;
    const auto& b=*static_cast<const OverlayRecvBindings*>(d->property->extra);
    LayerVector v{};
    if(!layer_header(entity,layer_offset,v))return;
    if(v.count!=static_cast<int>(d->value.words[0]))
        (void)b.resize(b.context,entity,layer_offset,layer_stride,static_cast<int>(d->value.words[0]));
}
void __cdecl layer_reserve_recv(const RecvProp* p,void** out,void* data,int){
    if(!out)return;
    *out=nullptr;
    if(!p||!p->extra)return;
    const auto& b=*static_cast<const OverlayRecvBindings*>(p->extra);
    if(layer_reserve(data,layer_offset,layer_stride,b.context,b.reserve))*out=data;
}
void __cdecl layer_element_recv(const RecvProp* p,void** out,void* data,int){
    if(out)*out=p?layer_address(data,layer_offset,layer_stride,p->stride):nullptr;
}
struct OverlayRecvGraph {
    OverlayRecvBindings binding{};
    RecvTable overlay,vector,length,layer;
    RecvProp parent,children[16],length_prop,fields[5];
    char names[15][4]{};
    bool build(const OverlayRecvBindings& b) noexcept {
        if(!b.reserve||!b.resize)return false;
        binding=b;element_names(names);
        overlay={&parent,1,nullptr,"DT_OverlayVars"};vector={children,16,nullptr,"_ST_m_AnimOverlay_15"};
        length={&length_prop,1,nullptr,"_LPT_m_AnimOverlay_15"};layer={fields,5,nullptr,"DT_Animationlayer"};
        nested(parent,"m_AnimOverlay",&vector);nested(children[0],"lengthproxy",&length);
        children[0].table_proxy=&layer_reserve_recv;children[0].extra=&binding;
        anim_recv(length_prop,"lengthprop15",0,0,&layer_length_recv);length_prop.extra=&binding;
        for(int i=0;i<overlay_limit;++i){
            auto& p=children[i+1];nested(p,names[i],&layer);
            p.stride=i;p.table_proxy=&layer_element_recv;p.extra=&binding;
        }
        anim_recv(fields[0],"sequence",0);
        anim_recv(fields[1],"cycle",20,1);
        anim_recv(fields[2],"prevcycle",4,1);
        anim_recv(fields[3],"weight",8,1);
        anim_recv(fields[4],"order",12);
        return true;
    }
};

int __cdecl create_overlay(const OverlayRecvBindings* b,OverlayRecvView* out) noexcept {
    if(!b||!out||!overlay_empty(*out))return 0;
    auto* g=new(std::nothrow) OverlayRecvGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    *out={g,&g->overlay,&g->vector,&g->length,&g->layer};return 1;
}
int __cdecl destroy_overlay(OverlayRecvView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<OverlayRecvGraph*>(v->handle);
    if(!overlay_matches(*g,*v))return 0;
    for(const auto* t:{&g->overlay,&g->vector,&g->length,&g->layer})
        if(t->decoder||t->initialized||t->in_main_list)return 0;
    delete g;*v={};return 1;
}
const OverlayRecvApi overlay_api{version,sizeof(OverlayRecvApi),&create_overlay,&destroy_overlay};

void __cdecl velocity_recv(const RecvData* d,void* entity,void*){
    if(!d||!d->property||!d->property->extra||!entity)return;
    const int offset=d->property->offset;
    if(offset!=0xec&&offset!=0xf0&&offset!=0xf4)return;
    const auto base=reinterpret_cast<std::uintptr_t>(entity);
    if(base>static_cast<std::uintptr_t>(-1)-0xf8u)return;
    auto* velocity=reinterpret_cast<void*>(base+0xecu);
    if(!sin_independent::vgui_layout::Readable(velocity,12))return;
    // Host guarantees writable entity storage and synchronous lifetime.
    float xyz[3]{},incoming=0;
    std::memcpy(xyz,velocity,12);std::memcpy(&incoming,&d->value.words[0],4);
    const unsigned index=static_cast<unsigned>((offset-0xec)/4);
    if(xyz[index]==incoming)return; // numeric equality, including signed zero
    std::memcpy(&xyz[index],&d->value.words[0],4);
    const auto& b=*static_cast<const PlayerLocalRecvBindings*>(d->property->extra);
    if(b.invalidate_velocity(b.context,entity,4))std::memcpy(velocity,xyz,12);
}
struct PlayerLocalRecvGraph {
    PlayerLocalRecvBindings binding{};
    RecvTable table;
    RecvProp props[27];
    bool build(const PlayerLocalRecvBindings& b) noexcept {
        if(!player_local_deps(b,49,0xc00)||b.ammo->array_prop||!b.invalidate_velocity)return false;
        binding=b;table={props,27,nullptr,"DT_LocalPlayerExclusive"};
        nested(props[0],"m_Local",b.local);props[0].offset=0xd68;
        anim_recv(props[1],"m_vecViewOffset[0]",0xe0,1);
        anim_recv(props[2],"m_vecViewOffset[1]",0xe4,1);
        anim_recv(props[3],"m_vecViewOffset[2]",0xe8,1);
        anim_recv(props[4],"m_flFriction",0x1ec,1);
        props[5]=*b.ammo;
        anim_recv(props[6],"m_fOnTarget",0xf10,0,&byte_recv);
        anim_recv(props[7],"m_bOnUseTarget",0xf11,0,&byte_recv);
        anim_recv(props[8],"m_nTickBase",0x102c);
        anim_recv(props[9],"m_nNextThinkTick",0x84);
        anim_recv(props[10],"m_hLastWeapon",0x1040,0,&handle_recv);
        anim_recv(props[11],"m_hGroundEntity",0x1e0,0,&handle_recv);
        anim_recv(props[12],"m_vecVelocity[0]",0xec,1,&velocity_recv);
        anim_recv(props[13],"m_vecVelocity[1]",0xf0,1,&velocity_recv);
        anim_recv(props[14],"m_vecVelocity[2]",0xf4,1,&velocity_recv);
        for(unsigned i=12;i<=14;++i)props[i].extra=&binding;
        anim_recv(props[15],"m_vecBaseVelocity",0x124,2,&vector_recv);
        anim_recv(props[16],"m_hViewModel[0]",0x1058,0,&handle_recv);
        props[17].name="m_hViewModel";props[17].type=4;props[17].elements=2;props[17].stride=4;
        anim_recv(props[18],"m_hConstraintEntity",0xf48,0,&handle_recv);
        anim_recv(props[19],"m_vecConstraintCenter",0xf4c,2,&vector_recv);
        anim_recv(props[20],"m_flConstraintRadius",0xf58,1);
        anim_recv(props[21],"m_flConstraintWidth",0xf5c,1);
        anim_recv(props[22],"m_flConstraintSpeedFactor",0xf60,1);
        anim_recv(props[23],"m_fViewConstraint",0xf64,1);
        anim_recv(props[24],"m_flDeathTime",0xf74,1);
        anim_recv(props[25],"m_nWaterLevel",0x1dc);
        anim_recv(props[26],"m_flLaggedMovementValue",0x1174,1);
        return true;
    }
};

int __cdecl create_player_local(const PlayerLocalRecvBindings* b,PlayerLocalRecvView* out) noexcept {
    if(!b||!out||!player_local_empty(*out))return 0;
    auto* g=new(std::nothrow) PlayerLocalRecvGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    *out={g,&g->table};return 1;
}
int __cdecl destroy_player_local(PlayerLocalRecvView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<PlayerLocalRecvGraph*>(v->handle);
    if(v->table!=&g->table||g->table.decoder||g->table.initialized||g->table.in_main_list)return 0;
    delete g;*v={};return 1;
}
const PlayerLocalRecvApi player_local_api{version,sizeof(PlayerLocalRecvApi),&create_player_local,&destroy_player_local};
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
#pragma comment(linker,"/EXPORT:SiN_GetAnimatingSendGraphApi=_SiN_GetAnimatingSendGraphApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::AnimSendApi* __cdecl
SiN_GetAnimatingSendGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(AnimSendApi)?&anim_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetFlexSendGraphApi=_SiN_GetFlexSendGraphApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::FlexSendApi* __cdecl
SiN_GetFlexSendGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(FlexSendApi)?&flex_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetOverlaySendGraphApi=_SiN_GetOverlaySendGraphApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::OverlaySendApi* __cdecl
SiN_GetOverlaySendGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(OverlaySendApi)?&overlay_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetPlayerLocalSendGraphApi=_SiN_GetPlayerLocalSendGraphApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PlayerLocalSendApi* __cdecl
SiN_GetPlayerLocalSendGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PlayerLocalSendApi)?&player_local_api:nullptr;
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
#pragma comment(linker,"/EXPORT:SiN_GetAnimatingRecvGraphApi=_SiN_GetAnimatingRecvGraphApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::AnimRecvApi* __cdecl
SiN_GetAnimatingRecvGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(AnimRecvApi)?&anim_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetFlexRecvGraphApi=_SiN_GetFlexRecvGraphApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::FlexRecvApi* __cdecl
SiN_GetFlexRecvGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(FlexRecvApi)?&flex_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetOverlayRecvGraphApi=_SiN_GetOverlayRecvGraphApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::OverlayRecvApi* __cdecl
SiN_GetOverlayRecvGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(OverlayRecvApi)?&overlay_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetPlayerLocalRecvGraphApi=_SiN_GetPlayerLocalRecvGraphApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PlayerLocalRecvApi* __cdecl
SiN_GetPlayerLocalRecvGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PlayerLocalRecvApi)?&player_local_api:nullptr;
}
#endif
