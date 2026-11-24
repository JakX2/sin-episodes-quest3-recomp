#include "native_weapon_datatables.h"
#include <cstring>
#include <new>
#include <initializer_list>
#include <cmath>
#include <limits>
#include "client_known_gamesystem_init.h"
namespace sin_port::native_weapon_tables {
namespace {
constexpr unsigned max_props=7;
bool hologram_writable(void* object,unsigned bytes) noexcept {
    if(!object||reinterpret_cast<std::uintptr_t>(object)%4||
       !sin_independent::vgui_layout::Readable(object,bytes))return false;
    MEMORY_BASIC_INFORMATION area{};
    if(VirtualQuery(object,&area,sizeof(area))!=sizeof(area))return false;
    const auto prot=area.Protect&0xffu;
    return prot==PAGE_READWRITE||prot==PAGE_WRITECOPY||
        prot==PAGE_EXECUTE_READWRITE||prot==PAGE_EXECUTE_WRITECOPY;
}
template<class T> T hologram_read(const void* object,unsigned offset) noexcept {
    T value{};std::memcpy(&value,static_cast<const unsigned char*>(object)+offset,sizeof(value));
    return value;
}
template<class T> void hologram_write(void* object,unsigned offset,T value) noexcept {
    std::memcpy(static_cast<unsigned char*>(object)+offset,&value,sizeof(value));
}
template<class V> bool aggregate_empty(const V& v) noexcept {
    if(v.handle||v.class_head||v.class_records||v.class_count)return false;
    for(const auto* tab:v.tables)if(tab)return false;
    return true;
}
template<class G,class V> bool aggregate_matches(const G& g,const V& v) noexcept {
    if(v.class_head!=g.classes.head||v.class_records!=g.classes.records||v.class_count!=ClassSubsetCount)return false;
    for(unsigned i=0;i<aggregate_table_count;++i)if(v.tables[i]!=g.tables[i])return false;
    return true;
}
// Class table identity is stricter than matching a display name: all
// reconstructed baseclass links must reference the very same instances.
template<class B> bool class_tables_valid(const B& b) noexcept {
    constexpr int parents[ClassSubsetCount]={ClAnimating,ClEntity,ClAnimating,ClFlex,-1,
        ClOverlay,ClCombatCharacter,ClCombatWeapon,ClHLCombatWeapon,ClMachineGun,
        ClEntity,ClBasePlayer,ClHLCombatWeapon,ClSelectFire,ClHLCombatWeapon};
    for(unsigned i=0;i<ClassSubsetCount;++i){
        const auto* t=b.tables[i];
        if(!t||!t->name||std::strcmp(t->name,class_table_names[i])||
            t->count!=class_table_counts[i]||!t->props)return false;
        if(parents[i]>=0){
            const auto& prop=t->props[0];
            if(!prop.name||std::strcmp(prop.name,"baseclass")||prop.type!=5||prop.offset!=0||
                prop.table!=b.tables[parents[i]])return false;
        }
        for(unsigned j=0;j<i;++j)if(t==b.tables[j])return false;
    }
    return true;
}
template<class V> bool class_empty(const V& v) noexcept {
    return !v.handle&&!v.head&&!v.records&&!v.count;
}
template<class G,class V> bool class_matches(const G& g,const V& v) noexcept {
    return v.head==g.head&&v.records==g.records&&v.count==ClassSubsetCount;
}
template<class V> bool derived_empty(const V& v) noexcept {
    return !v.handle&&!v.player&&!v.local&&!v.hand;
}
template<class G,class V> bool derived_matches(const G& g,const V& v) noexcept {
    return v.player==&g.player&&v.local==&g.local&&v.hand==&g.hand;
}
struct DerivedLocalField {const char* name;int offset;int type;int width;int bits;int flags;};
constexpr DerivedLocalField derived_local_fields[]={
    {"m_fFlashlightPower",4,1,4,10,0x11},
    {"m_fSprintPower",8,1,4,10,0x11},
    {"m_fAccuracy",0xc,1,4,0,4},
    {"m_bZooming",0x10,0,1,1,1},
    {"m_bZoomScope",0x11,0,1,1,1},
    {"m_bitsActiveDevices",0x14,0,4,3,1},
    {"m_iSquadMemberCount",0x18,0,4,32,0},
    {"m_iSquadMedicCount",0x1c,0,4,32,0},
    {"m_fSquadInFollowMode",0x20,0,1,1,1},
    {"m_bWeaponLowered",0x21,0,1,1,1},
    {"m_poisonTimer",0x24,1,4,0,4},
    {"m_breathTimer",0x28,1,4,0,4},
    {"m_hLadder",0x2c,0,4,21,5},
};
static_assert(sizeof(derived_local_fields)/sizeof(derived_local_fields[0])==13);
template<class V> bool entity_empty(const V& v) noexcept {
    return !v.handle&&!v.entity&&!v.anim_time&&!v.predictable;
}
template<class G,class V> bool entity_matches(const G& g,const V& v) noexcept {
    return v.entity==&g.entity&&v.anim_time==&g.anim_time&&v.predictable==&g.predictable;
}
bool sample_network_clock(void* context,SampleClock fn,NetworkClock& clock) noexcept {
    return fn&&fn(context,&clock)&&std::isfinite(clock.interval)&&clock.interval>0.0f;
}
std::int64_t network_tick_base(int tick) noexcept {
    return (static_cast<std::int64_t>(tick)/100)*100;
}
#if defined(SIN_ROLE_server)
// Original x87 conversion truncates toward zero. Match its indefinite
// signed-DWORD result for non-finite/out-of-range inputs without UB.
std::int32_t trunc_native_i32(double value) noexcept {
    if(!std::isfinite(value)||value>=2147483648.0||value<=-2147483649.0)
        return (std::numeric_limits<std::int32_t>::min)();
    return static_cast<std::int32_t>(value);
}
#endif
// Directly observed DT_Local constructor arguments. Unused range
// fields for coordinate/no-scale encodings are normalized as elsewhere.
enum class LocalKind {I,B,F,V,H};
struct LocalField {
    const char* name;int server_offset,client_offset;LocalKind kind;
    int bits,flags;float low,high,multiplier;
};
constexpr LocalField local_fields[]={
    {"m_iHideHUD",0x24,0x24,LocalKind::I,11,0x1,0,0,0},
    {"m_iFOV",0x28,0x28,LocalKind::I,9,0x0,0,0,0},
    {"m_flFOVRate",0x2c,0x2c,LocalKind::F,0,0x4,0,0,0},
    {"m_iDefaultFOV",0x30,0x38,LocalKind::I,9,0x0,0,0,0},
    {"m_bDucked",0x40,0x3c,LocalKind::B,1,0x1,0,0,0},
    {"m_bDucking",0x41,0x3d,LocalKind::B,1,0x1,0,0,0},
    {"m_bInDuckJump",0x42,0x3e,LocalKind::B,1,0x1,0,0,0},
    {"m_flDucktime",0x44,0x40,LocalKind::F,12,0x408,0,2047.5f,2},
    {"m_flDuckJumpTime",0x48,0x44,LocalKind::F,12,0x8,0,2047.5f,2},
    {"m_flJumpTime",0x4c,0x48,LocalKind::F,12,0x8,0,2047.5f,2},
    {"m_flFallVelocity",0x54,0x50,LocalKind::F,17,0x400,-4096,4096,131071.0f/8192},
    {"m_vecPunchAngle",0x5c,0x64,LocalKind::V,0,0x402,0,0,0},
    {"m_vecPunchAngleVel",0x68,0xa0,LocalKind::V,0,0x2,0,0,0},
    {"m_bDrawViewmodel",0x74,0xac,LocalKind::B,1,0x1,0,0,0},
    {"m_bWearingSuit",0x75,0xad,LocalKind::B,1,0x1,0,0,0},
    {"m_bPoisoned",0x76,0xae,LocalKind::B,1,0x1,0,0,0},
    {"m_flStepSize",0x78,0xb0,LocalKind::F,16,0x10,128.0f/65536,128,512},
    {"m_bAllowAutoMovement",0x7c,0xb4,LocalKind::B,1,0x1,0,0,0},
    {"m_skybox3d.scale",0x84,0xbc,LocalKind::I,12,0x0,0,0,0},
    {"m_skybox3d.origin",0x88,0xc0,LocalKind::V,0,0x2,0,0,0},
    {"m_skybox3d.area",0x94,0xcc,LocalKind::I,8,0x1,0,0,0},
    {"m_skybox3d.fog.enable",0x9c,0xd4,LocalKind::B,1,0x1,0,0,0},
    {"m_skybox3d.fog.blend",0x9d,0xd5,LocalKind::B,1,0x1,0,0,0},
    {"m_skybox3d.fog.dirPrimary",0xa0,0xd8,LocalKind::V,0,0x2,0,0,0},
    {"m_skybox3d.fog.colorPrimary",0xac,0xe4,LocalKind::I,32,0x1,0,0,0},
    {"m_skybox3d.fog.colorSecondary",0xb0,0xe8,LocalKind::I,32,0x1,0,0,0},
    {"m_skybox3d.fog.start",0xb4,0xec,LocalKind::F,0,0x4,0,0,0},
    {"m_skybox3d.fog.end",0xb8,0xf0,LocalKind::F,0,0x4,0,0,0},
    {"m_fog.enable",0xc4,0xfc,LocalKind::B,1,0x1,0,0,0},
    {"m_fog.blend",0xc5,0xfd,LocalKind::B,1,0x1,0,0,0},
    {"m_fog.dirPrimary",0xc8,0x100,LocalKind::V,0,0x2,0,0,0},
    {"m_fog.colorPrimary",0xd4,0x10c,LocalKind::I,32,0x1,0,0,0},
    {"m_fog.colorSecondary",0xd8,0x110,LocalKind::I,32,0x1,0,0,0},
    {"m_fog.start",0xdc,0x114,LocalKind::F,0,0x4,0,0,0},
    {"m_fog.end",0xe0,0x118,LocalKind::F,0,0x4,0,0,0},
    {"m_fog.farz",0xe4,0x11c,LocalKind::F,0,0x4,0,0,0},
    {"m_audio.localSound[0]",0xec,0x124,LocalKind::V,0,0x2,0,0,0},
    {"m_audio.localSound[1]",0xf8,0x130,LocalKind::V,0,0x2,0,0,0},
    {"m_audio.localSound[2]",0x104,0x13c,LocalKind::V,0,0x2,0,0,0},
    {"m_audio.localSound[3]",0x110,0x148,LocalKind::V,0,0x2,0,0,0},
    {"m_audio.localSound[4]",0x11c,0x154,LocalKind::V,0,0x2,0,0,0},
    {"m_audio.localSound[5]",0x128,0x160,LocalKind::V,0,0x2,0,0,0},
    {"m_audio.localSound[6]",0x134,0x16c,LocalKind::V,0,0x2,0,0,0},
    {"m_audio.localSound[7]",0x140,0x178,LocalKind::V,0,0x2,0,0,0},
    {"m_audio.soundscapeIndex",0x14c,0x184,LocalKind::I,17,0x0,0,0,0},
    {"m_audio.localBits",0x150,0x188,LocalKind::I,8,0x1,0,0,0},
    {"m_audio.ent",0x154,0x18c,LocalKind::H,21,0x5,0,0,0},
};
static_assert(sizeof(local_fields)/sizeof(local_fields[0])==47);
template<class V> bool local_data_empty(const V& v) noexcept {return !v.handle&&!v.table&&!v.area_bits;}
template<class T> bool named_table(const T* t,const char* name,int count) noexcept {
    return t&&t->name&&std::strcmp(t->name,name)==0&&t->count==count&&t->props;
}
template<class B> bool base_player_deps(const B& b) noexcept {
    return named_table(b.combat,"DT_BaseCombatCharacter",3)&&
           named_table(b.private_player,"DT_LocalPlayerExclusive",27);
}
template<class V> bool base_player_empty(const V& v) noexcept {
    return !v.handle&&!v.player&&!v.state;
}
template<class G,class V> bool base_player_matches(const G& g,const V& v) noexcept {
    return v.player==&g.player&&v.state==&g.state;
}
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
bool build_resolved_graph(SendGraph& graph,const ResolvedSendBindings& b) noexcept {
    if(!b.entries||!b.entry_count||b.entry_count>sin_independent::kEntityHandleCapacity||
       !b.edict_base||!b.max_clients||b.max_clients>256||
       !mem::Readable(b.entries,sizeof(*b.entries))||
       !mem::Readable(b.edict_base,edict_stride*(b.max_clients+1u)))return false;
    methods::CallResult refused=methods::CallResult::InvalidObject;
    if(!methods::VerifiedMethod(b.engine_server,18,refused))return false;
    const SendBindings native{b.base_animating,&resolved_local,&resolved_local,&resolved_handle};
    if(!graph.build(native))return false;
    graph.context={b.engine_server,b.entries,b.entry_count,b.edict_base,b.max_clients};
    graph.props[BaseCombat][1].extra=&graph.context;
    graph.props[BaseCombat][2].extra=&graph.context;
    graph.props[BaseCombat][6].extra=&graph.context;
    return true;
}
int __cdecl create_resolved(const ResolvedSendBindings* b,SendView* out) noexcept {
    if(!b||!out||!empty(*out))return 0;
    auto* graph=new(std::nothrow) SendGraph;
    if(!graph)return 0;
    if(!build_resolved_graph(*graph,*b)){delete graph;return 0;}
    out->handle=graph;
    for(unsigned i=0;i<TableCount;++i)out->tables[i]=&graph->tables[i];
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

void __cdecl player_flags_send(const s::SendProp*,const void*,const void* data,s::DVariant* out,int,int){
    if(!data||!out)return;
    std::uint32_t v=0;std::memcpy(&v,data,4);out->words[0]=v&0x1ffu;
}
void __cdecl player_place_send(const s::SendProp*,const void*,const void* data,s::DVariant* out,int,int){
    // Original 0x2225e580 forwards borrowed char pointer, not the text.
    if(out)std::memcpy(&out->words[0],&data,sizeof(data));
}
struct BasePlayerSendGraph {
    ResolvedContext context{};
    s::SendTable player,state;
    s::SendProp props[15],deadflag;
    bool build(const BasePlayerSendBindings& b) noexcept {
        if(!base_player_deps(b)||!b.entries||!b.entry_count||
           b.entry_count>sin_independent::kEntityHandleCapacity||!mem::Readable(b.entries,sizeof(*b.entries)))return false;
        context.entries=b.entries;context.count=b.entry_count;
        if(!player.Construct(props,15,"DT_BasePlayer")||!state.Construct(&deadflag,1,"DT_PlayerState")||
           !s::MakeDataTableProp(props[0],"baseclass",0,b.combat,&s::DirectTable)||
           !s::MakeDataTableProp(props[1],"pl",0x904,&state,&s::DirectTable)||
           !s::MakeDataTableProp(props[14],"localdata",0,b.private_player,&player_local_proxy))return false;
        anim_int(deadflag,"deadflag",4,1,1,true);
        const auto handle=[&](unsigned i,const char* name,int off){
            anim_int(props[i],name,off,21,5);
            props[i].scalarProxy=&resolved_handle;props[i].extra=&context;
        };
        handle(2,"m_hVehicle",0x9ac);
        anim_int(props[3],"m_nVehicleView",0x9b0,32,0);
        anim_int(props[4],"m_MoveType",0x204,4,1);
        anim_int(props[5],"m_iHealth",0x9c,10,0);
        anim_int(props[6],"m_lifeState",0x8c,3,1);
        anim_float(props[7],"m_flMaxspeed",0x1234,12,8,0,2048.0f-2048.0f/4096,2);
        anim_int(props[8],"m_fFlags",0xbc,9,0x401);
        props[8].scalarProxy=&player_flags_send;
        anim_int(props[9],"m_iObserverMode",0x9dc,3,1);
        handle(10,"m_hObserverTarget",0x9e4);
        handle(11,"m_hObserverViewModel",0xbd4);
        props[12].name="m_hObserverViewModel";props[12].type=4;props[12].elements=2;
        props[12].stride=4;props[12].tableProxy=&array_noop;
        props[13].name="m_szLastPlaceName";props[13].type=3;
        props[13].offset=0x12e8;props[13].scalarProxy=&player_place_send;
        return true;
    }
};

int __cdecl create_base_player(const BasePlayerSendBindings* b,BasePlayerSendView* out) noexcept {
    if(!b||!out||!base_player_empty(*out))return 0;
    auto* g=new(std::nothrow) BasePlayerSendGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    *out={g,&g->player,&g->state};return 1;
}
int __cdecl destroy_base_player(BasePlayerSendView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<BasePlayerSendGraph*>(v->handle);
    if(!base_player_matches(*g,*v))return 0;
    for(const auto* tab:{&g->player,&g->state})if(tab->precalc||tab->initialized||tab->written)return 0;
    delete g;*v={};return 1;
}
const BasePlayerSendApi base_player_api{version,sizeof(BasePlayerSendApi),&create_base_player,&destroy_base_player};

struct LocalDataSendGraph {
    ResolvedContext context{};
    s::SendTable table,area;
    s::SendProp props[48],area_props[32],area_template;
    char names[32][4]{};
    bool build(const LocalDataSendBindings& b) noexcept {
        if(!b.entries||!b.entry_count||b.entry_count>sin_independent::kEntityHandleCapacity||
            !mem::Readable(b.entries,sizeof(*b.entries)))return false;
        context.entries=b.entries;context.count=b.entry_count;
        element_names(names);
        if(!table.Construct(props,48,"DT_Local")||!area.Construct(area_props,32,"m_chAreaBits")||
           !s::MakeDataTableProp(props[0],"m_chAreaBits",4,&area,&s::DirectTable))return false;
        anim_int(area_template,"m_chAreaBits",4,8,1,true);
        props[0].arrayProp=&area_template;
        for(int i=0;i<32;++i){
            area_props[i]=area_template;area_props[i].name=names[i];area_props[i].offset=i;
            area_props[i].excludeOrParent=area_template.name;
        }
        for(unsigned i=0;i<47;++i){
            const auto& f=local_fields[i];auto& q=props[i+1];
            if(f.kind==LocalKind::F||f.kind==LocalKind::V){
                anim_float(q,f.name,f.server_offset,f.bits,f.flags,f.low,f.high,f.multiplier);
                if(f.kind==LocalKind::V){q.type=2;q.scalarProxy=&vector_send;}
            }else{
                anim_int(q,f.name,f.server_offset,f.bits,f.flags,f.kind==LocalKind::B);
                if(f.kind==LocalKind::H){q.scalarProxy=&resolved_handle;q.extra=&context;}
            }
        }
        return true;
    }
};

int __cdecl create_local_data(const LocalDataSendBindings* b,LocalDataSendView* out) noexcept {
    if(!b||!out||!local_data_empty(*out))return 0;
    auto* g=new(std::nothrow) LocalDataSendGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    *out={g,&g->table,&g->area};return 1;
}
int __cdecl destroy_local_data(LocalDataSendView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<LocalDataSendGraph*>(v->handle);
    if(v->table!=&g->table||v->area_bits!=&g->area)return 0;
    for(const auto* tab:{&g->table,&g->area})if(tab->precalc||tab->initialized||tab->written)return 0;
    delete g;*v={};return 1;
}
const LocalDataSendApi local_data_api{version,sizeof(LocalDataSendApi),&create_local_data,&destroy_local_data};

void __cdecl entity_time_send(const s::SendProp* p,const void* entity,const void*,s::DVariant* out,int,int){
    if(!out)return;
    out->words[0]=0;
    if(!p||!p->extra||(p->offset!=0x2c&&p->offset!=0x30))return;
    const auto& b=*static_cast<const BaseEntitySendBindings*>(p->extra);
    NetworkClock clock{};float time=0;
    if(!read_at(entity,static_cast<unsigned>(p->offset),time)||!sample_network_clock(b.context,b.clock,clock))return;
    const auto ticks=trunc_native_i32(static_cast<double>(time)/clock.interval+0.5);
    const auto base=network_tick_base(clock.tick);
    if(ticks>=base-(p->offset==0x2c?100:0))
        out->words[0]=static_cast<std::uint32_t>(ticks-base)&0xffu;
}
void __cdecl entity_transform_send(const s::SendProp* p,const void* entity,const void*,s::DVariant* out,int,int){
    if(!out)return;
    if(!p||!p->extra||!entity)return;
    const auto& b=*static_cast<const BaseEntitySendBindings*>(p->extra);
    std::uint32_t xyz[3]{};
    const bool angles=p->offset==0x36c;
    if(!b.transform(b.context,entity,angles?1:0,xyz))return;
    if(angles){
        for(auto& word:xyz){
            float angle=0;std::memcpy(&angle,&word,4);
            const auto units=static_cast<std::uint32_t>(trunc_native_i32(static_cast<double>(angle)*(65536.0f/360.0f)))&0xffffu;
            const float wrapped=static_cast<float>(units)*(360.0f/65536.0f);
            std::memcpy(&word,&wrapped,4);
        }
    }
    std::memcpy(out->words,xyz,12);
}
void __cdecl entity_move_send(const s::SendProp* p,const void* entity,const void*,s::DVariant* out,int,int){
    if(!p||!out)return;
    std::uint32_t value=0;
    const unsigned off=p->bits==4?0x204u:0x208u;
    if(read_at(entity,off,value))out->words[0]=value;
}
void* __cdecl predictable_recipient(const s::SendProp*,const void* entity,const void* data,s::Recipients* mask,int){
    if(!mask)return nullptr;
    std::uint32_t id=0;
    // Invalid ID: original leaves mask untouched. Returning null is
    // the signal to omit the nested table.
    if(!read_at(entity,0x3c,id)||!id)return nullptr;
    for(auto& word:mask->words)word=0;
    const unsigned index=(id>>1)&31u;
    mask->words[index/32]=1u<<(index%32);
    return const_cast<void*>(data);
}
struct BaseEntitySendGraph {
    ResolvedContext context{};
    BaseEntitySendBindings binding{};
    s::SendTable entity,anim_time,predictable;
    s::SendProp props[24],time_prop,predict_props[2];
    bool build(const BaseEntitySendBindings& b) noexcept {
        if(!named_table(b.collision,"DT_CollisionProperty",8)||!b.clock||!b.transform||
            !b.entries||!b.entry_count||b.entry_count>sin_independent::kEntityHandleCapacity||
            !mem::Readable(b.entries,sizeof(*b.entries)))return false;
        context.entries=b.entries;context.count=b.entry_count;binding=b;
        if(!entity.Construct(props,24,"DT_BaseEntity")||
           !anim_time.Construct(&time_prop,1,"DT_AnimTimeMustBeFirst")||
           !predictable.Construct(predict_props,2,"DT_PredictableId")||
           !s::MakeDataTableProp(props[0],"AnimTimeMustBeFirst",0,&anim_time,&server_anim_proxy)||
           !s::MakeDataTableProp(props[4],"m_Collision",0xfc,b.collision,&s::DirectTable)||
           !s::MakeDataTableProp(props[21],"predictable_id",0,&predictable,&predictable_recipient))return false;
        anim_int(time_prop,"m_flAnimTime",0x2c,8,0x401);
        anim_int(props[1],"m_flSimulationTime",0x30,8,0x401);
        for(auto* prop:{&time_prop,&props[1]}){prop->scalarProxy=&entity_time_send;prop->extra=&binding;}
        props[2].name="m_vecOrigin";props[2].type=2;props[2].flags=0x402;props[2].offset=0x360;
        props[2].scalarProxy=&entity_transform_send;props[2].extra=&binding;
        anim_int(props[3],"m_nModelIndex",0x38,11,0);
        anim_int(props[5],"m_nRenderFX",0x1c,8,1);
        anim_int(props[6],"m_nRenderMode",0x20,8,1);
        anim_int(props[7],"m_fEffects",0x78,8,1);
        anim_int(props[8],"m_clrRender",0x24,32,1);
        anim_int(props[9],"m_iTeamNum",0x22c,6,0);
        anim_int(props[10],"m_CollisionGroup",0x214,5,1);
        anim_float(props[11],"m_flElasticity",0x21c,0,2,0,0,0);
        anim_float(props[12],"m_flShadowCastDistance",0x220,12,1,0,4096,4095.0f/4096);
        const auto handle=[&](unsigned i,const char* name,int off){
            anim_int(props[i],name,off,21,5);props[i].scalarProxy=&resolved_handle;props[i].extra=&context;
        };
        handle(13,"m_hOwnerEntity",0x20c);handle(14,"m_hEffectEntity",0x210);handle(15,"moveparent",0xf0);
        anim_int(props[16],"m_iParentAttachment",0xec,6,1,true);
        anim_int(props[17],"movetype",0,4,1);props[17].scalarProxy=&entity_move_send;
        anim_int(props[18],"movecollide",0,3,1);props[18].scalarProxy=&entity_move_send;
        anim_float(props[19],"m_angRotation",0x36c,13,0x400,0,360,8191.0f/360);
        props[19].type=2;props[19].scalarProxy=&entity_transform_send;props[19].extra=&binding;
        anim_int(props[20],"m_iTextureFrameIndex",0x2d4,8,1);
        anim_int(predict_props[0],"m_PredictableID",0x3c,31,1);
        anim_int(predict_props[1],"m_bIsPlayerSimulated",0x388,1,1,true);
        anim_int(props[22],"m_bSimulatedEveryTick",0x2d8,1,1,true);
        anim_int(props[23],"m_bAnimatedEveryTick",0x2d9,1,1,true);
        return true;
    }
};

int __cdecl create_entity(const BaseEntitySendBindings* b,BaseEntitySendView* out) noexcept {
    if(!b||!out||!entity_empty(*out))return 0;
    auto* g=new(std::nothrow) BaseEntitySendGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    *out={g,&g->entity,&g->anim_time,&g->predictable};return 1;
}
int __cdecl destroy_entity(BaseEntitySendView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<BaseEntitySendGraph*>(v->handle);
    if(!entity_matches(*g,*v))return 0;
    for(const auto* tab:{&g->entity,&g->anim_time,&g->predictable})if(tab->precalc||tab->initialized||tab->written)return 0;
    delete g;*v={};return 1;
}
const BaseEntitySendApi entity_api{version,sizeof(BaseEntitySendApi),&create_entity,&destroy_entity};

struct DerivedPlayerSendGraph {
    ResolvedContext context{};
    s::SendTable player,local,hand;
    s::SendProp props[6],local_props[13],hand_prop;
    bool build(const DerivedPlayerSendBindings& b) noexcept {
        if(!named_table(b.player,"DT_BasePlayer",15)||!named_table(b.entity,"DT_BaseEntity",24)||
           !b.entries||!b.entry_count||b.entry_count>sin_independent::kEntityHandleCapacity||
           !mem::Readable(b.entries,sizeof(*b.entries)))return false;
        context.entries=b.entries;context.count=b.entry_count;
        if(!player.Construct(props,6,"DT_HL2_Player")||!local.Construct(local_props,13,"DT_HL2Local")||
           !hand.Construct(&hand_prop,1,"DT_PlayerHand")||
           !s::MakeDataTableProp(props[0],"baseclass",0,b.player,&s::DirectTable)||
           !s::MakeDataTableProp(props[1],"m_HL2Local",0x137c,&local,&player_local_proxy)||
           !s::MakeDataTableProp(hand_prop,"baseclass",0,b.entity,&s::DirectTable))return false;
        anim_int(props[2],"m_fIsSprinting",0x13d9,1,1,true);
        anim_float(props[3],"m_flFlashTime",0x1510,0,4,0,0,0);
        anim_float(props[4],"m_flFlashFadeTime",0x1514,0,4,0,0,0);
        anim_float(props[5],"m_flFlashStartAlpha",0x1518,0,4,0,0,0);
        for(unsigned i=0;i<13;++i){
            const auto& f=derived_local_fields[i];auto& q=local_props[i];
            if(f.type==1){
                if(i<2)anim_float(q,f.name,f.offset,f.bits,f.flags,100.0f/1024,100,1024.0f/100);
                else anim_float(q,f.name,f.offset,f.bits,f.flags,0,0,0);
            }else anim_int(q,f.name,f.offset,f.bits,f.flags,f.width==1);
            if(i==12){q.scalarProxy=&resolved_handle;q.extra=&context;}
        }
        return true;
    }
};

int __cdecl create_derived_player(const DerivedPlayerSendBindings* b,DerivedPlayerSendView* out) noexcept {
    if(!b||!out||!derived_empty(*out))return 0;
    auto* g=new(std::nothrow) DerivedPlayerSendGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    *out={g,&g->player,&g->local,&g->hand};return 1;
}
int __cdecl destroy_derived_player(DerivedPlayerSendView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<DerivedPlayerSendGraph*>(v->handle);
    if(!derived_matches(*g,*v))return 0;
    for(const auto* tab:{&g->player,&g->local,&g->hand})if(tab->precalc||tab->initialized||tab->written)return 0;
    delete g;*v={};return 1;
}
const DerivedPlayerSendApi derived_api{version,sizeof(DerivedPlayerSendApi),&create_derived_player,&destroy_derived_player};

int ascii_class_compare(const char* a,const char* b) noexcept {
    const auto fold=[](unsigned char ch){return ch>='A'&&ch<='Z'?ch+('a'-'A'):ch;};
    while(*a&&fold(static_cast<unsigned char>(*a))==fold(static_cast<unsigned char>(*b))){++a;++b;}
    return fold(static_cast<unsigned char>(*a))-fold(static_cast<unsigned char>(*b));
}
struct ServerClassGraph {
    ServerClassRecord records[ClassSubsetCount]{};
    ServerClassRecord* head=nullptr;
    bool build(const ServerClassBindings& b) noexcept {
        if(!class_tables_valid(b))return false;
        for(unsigned i=0;i<ClassSubsetCount;++i){
            auto& rec=records[i];rec.name=class_names[i];rec.table=b.tables[i];
            // Original constructor inserts after equal names. Our fixed
            // subset has unique names and uses ASCII-only originals.
            auto** link=&head;
            while(*link&&ascii_class_compare((*link)->name,rec.name)<=0)link=&(*link)->next;
            rec.next=*link;*link=&rec;
        }
        return true;
    }
};

int __cdecl create_class_subset(const ServerClassBindings* b,ServerClassView* out) noexcept {
    if(!b||!out||!class_empty(*out))return 0;
    auto* g=new(std::nothrow) ServerClassGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    *out={g,g->head,g->records,ClassSubsetCount};return 1;
}
int __cdecl destroy_class_subset(ServerClassView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<ServerClassGraph*>(v->handle);
    if(!class_matches(*g,*v))return 0;
    for(const auto& rec:g->records){
        const auto* tab=rec.table;
        if(!tab||rec.class_id!=0||rec.instance_baseline!=0xffff||tab->precalc||tab->initialized||tab->written)return 0;
    }
    delete g;*v={};return 1;
}
const ServerClassApi class_api{version,sizeof(ServerClassApi),&create_class_subset,&destroy_class_subset};

struct AggregateSendGraph {
    using Table=s::SendTable;
    BaseEntitySendGraph entity;
    AnimSendGraph anim;
    OverlaySendGraph overlay;
    FlexSendGraph flex;
    InventorySendGraph inventory;
    LocalDataSendGraph local;
    PlayerLocalSendGraph private_player;
    BasePlayerSendGraph player;
    DerivedPlayerSendGraph derived;
    SendGraph weapons;
    ServerClassGraph classes;
    bool extra_bound=false;
    Table* tables[aggregate_table_count]{};
    bool build(const AggregateSendBindings& b) noexcept {
        const auto* entries=b.entity.entries;const auto count=b.entity.entry_count;
        if(!entity.build(b.entity)||!anim.build({&entity.entity,entries,count})||
           !overlay.build(b.overlay)||!flex.build({&anim.base,&overlay.overlay})||
           !inventory.build({&flex.flex,b.engine_server,entries,count,b.edict_base,b.max_clients})||
           !local.build({entries,count})||
           !private_player.build({&local.table,&inventory.ammo_property,entries,count})||
           !player.build({&inventory.combat_base,&private_player.table,entries,count})||
           !derived.build({&player.player,&entity.entity,entries,count})||
           !build_resolved_graph(weapons,{&anim.base,b.engine_server,entries,count,b.edict_base,b.max_clients}))return false;
        if(!classes.build({{&weapons.tables[BaseCombat],&anim.base,&flex.overlay,&inventory.combat_base,
            &entity.entity,&flex.flex,&player.player,&weapons.tables[BaseHL],
            &weapons.tables[MachineGun],&weapons.tables[SelectFire],&derived.hand,&derived.player,
            &weapons.tables[Magnum],&weapons.tables[AssaultRifle],&weapons.tables[Scattergun]}}))return false;
        Table* fixed[]={&entity.entity,&entity.anim_time,&entity.predictable,
            &anim.base,&anim.cycle,&anim.pose,&anim.controllers,
            &overlay.overlay,&overlay.vector,&overlay.length,&overlay.layer,
            &flex.overlay,&flex.flex,&flex.weights,
            &inventory.ammo_table,&inventory.weapons_table,&inventory.combat_local,&inventory.combat_base,
            &local.table,&local.area,&private_player.table,&player.player,&player.state,
            &derived.player,&derived.local,&derived.hand};
        static_assert(sizeof(fixed)/sizeof(fixed[0])+TableCount==aggregate_table_count);
        unsigned i=0;for(auto* tab:fixed)tables[i++]=tab;
        for(auto& tab:weapons.tables)tables[i++]=&tab;
        return true;
    }
};

int __cdecl create_aggregate(const AggregateSendBindings* b,AggregateSendView* out) noexcept {
    if(!b||!out||!aggregate_empty(*out))return 0;
    auto* g=new(std::nothrow) AggregateSendGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    out->handle=g;out->class_head=g->classes.head;out->class_records=g->classes.records;
    out->class_count=ClassSubsetCount;
    for(unsigned i=0;i<aggregate_table_count;++i)out->tables[i]=g->tables[i];
    return 1;
}
int __cdecl destroy_aggregate(AggregateSendView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<AggregateSendGraph*>(v->handle);
    if(g->extra_bound||!aggregate_matches(*g,*v))return 0;
    // Preflight every owned object, including nested-only tables. No
    // partial teardown if an engine marker is still set anywhere.
    for(const auto* tab:g->tables)if(!tab||tab->precalc||tab->initialized||tab->written)return 0;
    for(const auto& rec:g->classes.records)if(rec.class_id!=0||rec.instance_baseline!=0xffff)return 0;
    delete g;*v={};return 1;
}
const AggregateSendApi aggregate_api{version,sizeof(AggregateSendApi),&create_aggregate,&destroy_aggregate};

struct ExtraSendGraph {
    AggregateSendGraph* base=nullptr;
    s::SendTable tables[ExtraEntityCount];
    s::SendProp props[ExtraEntityCount][7];
    ServerClassRecord records[ExtraEntityCount]{};
    ServerClassRecord* head=nullptr;
    bool build(AggregateSendGraph& core,const ExtraSendBindings&) noexcept {
        s::SendTable* parents[]={&core.inventory.combat_base,
            &core.weapons.tables[BaseHL],
            &tables[ExGrunt],
            &tables[ExAI],
            &tables[ExAI],
            &core.weapons.tables[SelectFire],
            &core.inventory.combat_base,
            &core.weapons.tables[BaseHL],
            &core.weapons.tables[SelectFire],
            &core.weapons.tables[BaseHL],
            &tables[ExBludgeon]};
        for(unsigned i=0;i<ExtraEntityCount;++i)
            if(!tables[i].Construct(props[i],extra_table_counts[i],extra_table_names[i]))return false;
        for(unsigned i=0;i<ExtraEntityCount;++i){
            if(!s::MakeDataTableProp(props[i][0],"baseclass",0,parents[i],&s::DirectTable))return false;
            auto& rec=records[i];rec.name=extra_class_names[i];rec.table=&tables[i];
            auto** link=&head;
            while(*link&&ascii_class_compare((*link)->name,rec.name)<=0)link=&(*link)->next;
            rec.next=*link;*link=&rec;
        }
        auto* ai=props[ExAI];
        anim_int(ai[1],"m_lifeState",0x8c,3,1);
        anim_int(ai[2],"m_bPerformAvoidance",0xc10,1,1,true);
        anim_int(ai[3],"m_bIsMoving",0xc11,1,1,true);
        anim_int(ai[4],"m_bFadeCorpse",0xc12,1,1,true);
        anim_int(ai[5],"m_iDeathPose",0x890,10,0);
        anim_int(ai[6],"m_iDeathFrame",0x894,5,0);
        auto* grunt=props[ExGrunt];
        anim_int(grunt[1],"m_receivedDecapitatingBlow",0xc44,1,1,true);
        anim_float(grunt[2],"m_decapitateVelocity",0xc48,0,4,0,0,0);
        grunt[2].type=2;grunt[2].scalarProxy=&vector_send;
        auto* holo=props[ExHologram];
        anim_int(holo[1],"m_state",0x1014,32,0);
        anim_float(holo[2],"m_timeEffectStops",0x1018,0,4,0,0,0);
        auto& target=props[ExShockwave][1];
        anim_float(target,"m_vecTargetPos",0x798,0,2,0,0,0);
        target.type=2;target.scalarProxy=&vector_send;
        return true;
    }
};

int __cdecl create_extra(const ExtraSendBindings* b,ExtraSendView* out) noexcept {
    if(!b||!b->base||!b->base->handle||!out||out->handle||out->head||out->records)return 0;
    for(auto* table:out->tables)if(table)return 0;
    auto* core=static_cast<AggregateSendGraph*>(b->base->handle);
    if(core->extra_bound||!aggregate_matches(*core,*b->base))return 0;
    auto* g=new(std::nothrow) ExtraSendGraph;
    if(!g)return 0;
    if(!g->build(*core,*b)){delete g;return 0;}
    g->base=core;core->extra_bound=true;
    out->handle=g;out->head=g->head;out->records=g->records;
    for(unsigned i=0;i<ExtraEntityCount;++i)out->tables[i]=&g->tables[i];
    return 1;
}
int __cdecl destroy_extra(ExtraSendView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<ExtraSendGraph*>(v->handle);
    if(v->head!=g->head||v->records!=g->records)return 0;
    for(unsigned i=0;i<ExtraEntityCount;++i){
        const auto& tab=g->tables[i];const auto& rec=g->records[i];
        if(v->tables[i]!=&tab||rec.class_id!=0||rec.instance_baseline!=0xffff||tab.precalc||tab.initialized||tab.written)return 0;
    }
    g->base->extra_bound=false;
    delete g;*v={};return 1;
}
const ExtraSendApi extra_api{version,sizeof(ExtraSendApi),&create_extra,&destroy_extra};

constexpr unsigned holo_state=0x1014,holo_end=0x1018,holo_mode=0x3b0,holo_fx=0x1c;
struct HologramServerRun {
    HologramControl& control;
    HologramServerBindings host;
    void* object() const noexcept {return control.object;}
    int state() const noexcept {return hologram_read<int>(object(),holo_state);}
    bool integer(unsigned offset,int value) noexcept {
        if(hologram_read<int>(object(),offset)==value)return true;
        if(!host.dirty(host.context,object(),offset))return false;
        hologram_write(object(),offset,value);return true;
    }
    bool timer(double value) noexcept {
        if(!std::isfinite(value)||value>(std::numeric_limits<float>::max)()||
           value<-(std::numeric_limits<float>::max)())return false;
        // Original compares the old float against the extended-precision
        // sum before storing the rounded DWORD. NaN compares unequal.
        if(static_cast<double>(hologram_read<float>(object(),holo_end))==value)return true;
        if(!host.dirty(host.context,object(),holo_end))return false;
        hologram_write(object(),holo_end,static_cast<float>(value));return true;
    }
    bool time(float& now) noexcept {
        return host.clock(host.context,&now)&&std::isfinite(now);
    }
    bool sound(HologramSound which) noexcept {return host.audio(host.context,object(),which)!=0;}
    bool run(HologramCommand cmd,float duration) noexcept {
        float now=0;
        switch(cmd){
        case HologramCommand::ResetAfterSpawn:
            return integer(holo_state,0)&&timer(0);
        case HologramCommand::On:
            return integer(holo_state,1)&&time(now)&&timer(static_cast<double>(now)+2.0)&&
                integer(holo_mode,0)&&sound(HologramSound::On)&&sound(HologramSound::StopLoop);
        case HologramCommand::Off:
            return integer(holo_state,3)&&time(now)&&timer(static_cast<double>(now)+1.0)&&
                integer(holo_mode,0)&&sound(HologramSound::StopLoop)&&sound(HologramSound::Off);
        case HologramCommand::Pulse:
            if(state()!=2)return true;
            return integer(holo_state,4)&&time(now)&&timer(static_cast<double>(now)+duration)&&
                integer(holo_mode,1)&&integer(holo_fx,13);
        case HologramCommand::Activate:
            return host.base(host.context,object(),HologramBaseCall::Activate)&&
                (state()!=2||sound(HologramSound::Loop));
        case HologramCommand::StartLoop:
            return integer(holo_state,2)&&sound(HologramSound::Loop);
        case HologramCommand::Think: {
            if(!host.base(host.context,object(),HologramBaseCall::Think))return false;
            const int current=state();
            if(current!=1&&current!=3&&current!=4)return true;
            if(!time(now))return false;
            // Strict original elapsed predicate: equality and unordered
            // comparisons skip. State 1/3 keep their old deadline.
            if(!(hologram_read<float>(object(),holo_end)<now))return true;
            if(current==1)return integer(holo_state,2)&&sound(HologramSound::Loop);
            if(current==3)return integer(holo_state,0);
            // Original state-4 settling path samples clock again.
            return integer(holo_state,2)&&time(now)&&timer(now)&&integer(holo_mode,0)&&integer(holo_fx,0);
        }
        default:return false;
        }
    }
};
int __cdecl hologram_dispatch(HologramControl* ctrl,const HologramServerBindings* b,
        HologramCommand cmd,float duration) noexcept {
    if(!ctrl||!b||ctrl->busy||ctrl->blocked||ctrl->bytes<0x101c||
       !hologram_writable(ctrl->object,0x101c)||!b->clock||!b->dirty||!b->audio||!b->base||
       static_cast<unsigned>(cmd)>static_cast<unsigned>(HologramCommand::StartLoop)||
       (cmd==HologramCommand::Pulse&&!std::isfinite(duration)))return 0;
    ctrl->busy=1;
    HologramServerRun run{*ctrl,*b};
    const bool ok=run.run(cmd,duration);
    ctrl->busy=0;
    if(!ok){ctrl->blocked=1;return -1;}
    return 1;
}
int __cdecl hologram_ack(HologramControl* ctrl) noexcept {
    if(!ctrl||ctrl->busy||!ctrl->blocked||ctrl->bytes<0x101c||
       !hologram_writable(ctrl->object,0x101c))return 0;
    ctrl->blocked=0;return 1;
}
const HologramServerApi hologram_api{version,sizeof(HologramServerApi),&hologram_dispatch,&hologram_ack};
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

void __cdecl player_place_recv(const RecvData* d,void*,void* out){
    // Original 0x24126ca0 copies until NUL or string_size, and always
    // terminates the last byte. Stage to avoid partial writes on bad source.
    if(!d||!d->property||!out||d->property->string_size!=18)return;
    const char* src=nullptr;std::memcpy(&src,&d->value.words[0],sizeof(src));
    if(!src)return;
    char staged[18]{};unsigned n=0;
    for(;n<18;++n){
        if(!sin_independent::vgui_layout::Readable(src+n,1))return;
        staged[n]=src[n];
        if(!staged[n]){++n;break;}
    }
    std::memcpy(out,staged,n);
    static_cast<char*>(out)[17]=0;
}
struct BasePlayerRecvGraph {
    RecvTable player,state;
    RecvProp props[15],deadflag;
    bool build(const BasePlayerRecvBindings& b) noexcept {
        if(!base_player_deps(b))return false;
        player={props,15,nullptr,"DT_BasePlayer"};
        state={&deadflag,1,nullptr,"DT_PlayerState"};
        nested(props[0],"baseclass",b.combat);
        nested(props[1],"localdata",b.private_player);
        nested(props[2],"pl",&state);props[2].offset=0xefc;
        anim_recv(deadflag,"deadflag",4,0,&byte_recv);
        anim_recv(props[3],"m_hVehicle",0xf7c,0,&handle_recv);
        anim_recv(props[4],"m_nVehicleView",0xf84);
        anim_recv(props[5],"m_MoveType",0x144);
        anim_recv(props[6],"m_iHealth",0xfc0);
        anim_recv(props[7],"m_lifeState",0x94);
        anim_recv(props[8],"m_flMaxspeed",0xfbc,1);
        anim_recv(props[9],"m_fFlags",0x2e0);
        anim_recv(props[10],"m_iObserverMode",0xf68);
        anim_recv(props[11],"m_hObserverTarget",0xf6c,0,&handle_recv);
        anim_recv(props[12],"m_hObserverViewModel[0]",0x1060,0,&handle_recv);
        props[13].name="m_hObserverViewModel";props[13].type=4;props[13].elements=2;props[13].stride=4;
        anim_recv(props[14],"m_szLastPlaceName",0x1188,3,&player_place_recv);
        props[14].string_size=18;
        return true;
    }
};

int __cdecl create_base_player(const BasePlayerRecvBindings* b,BasePlayerRecvView* out) noexcept {
    if(!b||!out||!base_player_empty(*out))return 0;
    auto* g=new(std::nothrow) BasePlayerRecvGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    *out={g,&g->player,&g->state};return 1;
}
int __cdecl destroy_base_player(BasePlayerRecvView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<BasePlayerRecvGraph*>(v->handle);
    if(!base_player_matches(*g,*v))return 0;
    for(const auto* tab:{&g->player,&g->state})if(tab->decoder||tab->initialized||tab->in_main_list)return 0;
    delete g;*v={};return 1;
}
const BasePlayerRecvApi base_player_api{version,sizeof(BasePlayerRecvApi),&create_base_player,&destroy_base_player};

void __cdecl local_fov_recv(const RecvData* d,void* object,void*){
    if(!d||!d->property||!d->property->extra||!object)return;
    // Host owns writable original-layout subobject. Original callback
    // ignores pOut and uses the struct base for four distinct fields.
    auto* bytes=static_cast<unsigned char*>(object);
    std::uint32_t old=0;std::memcpy(&old,bytes+0x28,4);
    if(old==d->value.words[0])return;
    const auto& b=*static_cast<const LocalDataRecvBindings*>(d->property->extra);
    FovSample sample{};
    if(!b.sample_fov(b.context,&sample)||sample.has_displayed_fov>1)return;
    std::uint32_t start=old;
    if(sample.has_displayed_fov)std::memcpy(&start,&sample.displayed_fov,4);
    else if(!old)std::memcpy(&start,&sample.default_fov,4);
    std::memcpy(bytes+0x30,&start,4);
    std::memcpy(bytes+0x34,&sample.time_bits,4);
    std::memcpy(bytes+0x28,&d->value.words[0],4);
}
void __cdecl local_default_fov_recv(const RecvData* d,void* object,void*){
    if(!d||!object)return;
    auto* bytes=static_cast<unsigned char*>(object);
    std::uint32_t old=0;std::memcpy(&old,bytes+0x38,4);
    if(old==d->value.words[0])return;
    const std::uint32_t zero=0;
    std::memcpy(bytes+0x2c,&zero,4);
    std::memcpy(bytes+0x38,&d->value.words[0],4);
}
struct LocalDataRecvGraph {
    LocalDataRecvBindings binding{};
    RecvTable table,area;
    RecvProp props[49],area_props[32];
    char names[32][4]{};
    bool build(const LocalDataRecvBindings& b) noexcept {
        if(!b.sample_fov)return false;
        binding=b;element_names(names);
        table={props,49,nullptr,"DT_Local"};area={area_props,32,nullptr,"m_chAreaBits"};
        nested(props[0],"m_chAreaBits",&area);props[0].offset=4;
        for(int i=0;i<32;++i){
            anim_recv(area_props[i],names[i],i,0,&byte_recv);
            area_props[i].parent_array="m_chAreaBits";
        }
        // The client-only base velocity precedes the two punch-angle slots.
        anim_recv(props[12],"m_vecClientBaseVelocity",0x58,2,&vector_recv);
        for(unsigned i=0;i<47;++i){
            const auto& f=local_fields[i];auto& q=props[i<11?i+1:i+2];
            const int type=f.kind==LocalKind::F?1:f.kind==LocalKind::V?2:0;
            const auto proxy=f.kind==LocalKind::B?&byte_recv:
                f.kind==LocalKind::V?&vector_recv:f.kind==LocalKind::H?&handle_recv:&raw_recv;
            anim_recv(q,f.name,f.client_offset,type,proxy);
        }
        props[2].proxy=&local_fov_recv;props[2].extra=&binding;
        props[4].proxy=&local_default_fov_recv;
        return true;
    }
};

int __cdecl create_local_data(const LocalDataRecvBindings* b,LocalDataRecvView* out) noexcept {
    if(!b||!out||!local_data_empty(*out))return 0;
    auto* g=new(std::nothrow) LocalDataRecvGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    *out={g,&g->table,&g->area};return 1;
}
int __cdecl destroy_local_data(LocalDataRecvView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<LocalDataRecvGraph*>(v->handle);
    if(v->table!=&g->table||v->area_bits!=&g->area)return 0;
    for(const auto* tab:{&g->table,&g->area})if(tab->decoder||tab->initialized||tab->in_main_list)return 0;
    delete g;*v={};return 1;
}
const LocalDataRecvApi local_data_api{version,sizeof(LocalDataRecvApi),&create_local_data,&destroy_local_data};

void __cdecl entity_time_recv(const RecvData* d,void* entity,void*){
    if(!d||!d->property||!d->property->extra||!entity||d->value.words[0]>255)return;
    const int off=d->property->offset;
    if(off!=0x64&&off!=0x6c)return;
    const auto& b=*static_cast<const BaseEntityRecvBindings*>(d->property->extra);
    NetworkClock clock{};if(!sample_network_clock(b.context,b.clock,clock))return;
    auto tick=network_tick_base(clock.tick)+d->value.words[0];
    // Original performs the lower-bound loop, then upper-bound loop.
    // In the exact +/-128 case the final result is tick-128.
    const auto low=static_cast<std::int64_t>(clock.tick)-127,high=static_cast<std::int64_t>(clock.tick)+127;
    if(tick<low)tick+=((low-tick+255)/256)*256;
    if(tick>high)tick-=((tick-high+255)/256)*256;
    const float time=static_cast<float>(static_cast<double>(tick)*clock.interval);
    std::memcpy(static_cast<unsigned char*>(entity)+off,&time,4);
}
void __cdecl entity_effect_recv(const RecvData* d,void* entity,void*){
    if(!d||!d->property||!d->property->extra||!entity)return;
    auto* field=static_cast<unsigned char*>(entity)+0x78;
    std::uint32_t old=0;std::memcpy(&old,field,4);
    if(old==d->value.words[0])return;
    std::memcpy(field,&d->value.words[0],4);
    const auto& b=*static_cast<const BaseEntityRecvBindings*>(d->property->extra);
    b.effects_changed(b.context,entity);
}
void __cdecl entity_move_recv(const RecvData* d,void* entity,void*){
    if(!d||!d->property||!entity)return;
    auto* bytes=static_cast<unsigned char*>(entity);
    // Both descriptors carry offset zero; distinguish the two callbacks
    // by the literal property name, not by an assumed entity offset.
    if(!std::strcmp(d->property->name,"movetype")){
        std::memcpy(bytes+0x144,&d->value.words[0],4);
        const std::uint32_t zero=0;std::memcpy(bytes+0x148,&zero,4);
    }else if(!std::strcmp(d->property->name,"movecollide")){
        std::memcpy(bytes+0x148,&d->value.words[0],4);
    }
}
void __cdecl entity_tick_recv(const RecvData* d,void* entity,void* out){
    if(!d||!d->property||!d->property->extra||!entity||!out)return;
    const unsigned char old=*static_cast<const unsigned char*>(out);
    if(old==(d->value.words[0]!=0?1u:0u))return;
    byte_recv(d,entity,out);
    const auto& b=*static_cast<const BaseEntityRecvBindings*>(d->property->extra);
    b.interpolation_changed(b.context,entity);
}
struct BaseEntityRecvGraph {
    BaseEntityRecvBindings binding{};
    RecvTable entity,anim_time,predictable;
    RecvProp props[24],time_prop,predict_props[2];
    bool build(const BaseEntityRecvBindings& b) noexcept {
        if(!named_table(b.collision,"DT_CollisionProperty",8)||!b.clock||!b.effects_changed||!b.interpolation_changed)return false;
        binding=b;
        entity={props,24,nullptr,"DT_BaseEntity"};
        anim_time={&time_prop,1,nullptr,"DT_AnimTimeMustBeFirst"};
        predictable={predict_props,2,nullptr,"DT_PredictableId"};
        nested(props[0],"AnimTimeMustBeFirst",&anim_time);
        anim_recv(time_prop,"m_flAnimTime",0x64,0,&entity_time_recv);time_prop.extra=&binding;
        anim_recv(props[1],"m_flSimulationTime",0x6c,0,&entity_time_recv);props[1].extra=&binding;
        anim_recv(props[2],"m_vecOrigin",0x2c8,2,&vector_recv);
        anim_recv(props[3],"m_angRotation",0x2d4,2,&vector_recv);
        anim_recv(props[4],"m_nModelIndex",0x8c);
        anim_recv(props[5],"m_fEffects",0x78,0,&entity_effect_recv);props[5].extra=&binding;
        anim_recv(props[6],"m_nRenderMode",0x7c);
        anim_recv(props[7],"m_nRenderFX",0x54);
        anim_recv(props[8],"m_clrRender",0x5c);
        anim_recv(props[9],"m_iTeamNum",0xa0);
        anim_recv(props[10],"m_CollisionGroup",0x2e4);
        anim_recv(props[11],"m_flElasticity",0x1d0,1);
        anim_recv(props[12],"m_flShadowCastDistance",0x1d4,1);
        anim_recv(props[13],"m_hOwnerEntity",0x464,0,&handle_recv);
        anim_recv(props[14],"m_hEffectEntity",0x468,0,&handle_recv);
        anim_recv(props[15],"moveparent",0x160,0,&handle_recv);
        anim_recv(props[16],"m_iParentAttachment",0x150,0,&byte_recv);
        anim_recv(props[17],"movetype",0,0,&entity_move_recv);
        anim_recv(props[18],"movecollide",0,0,&entity_move_recv);
        nested(props[19],"m_Collision",b.collision);props[19].offset=0x16c;
        anim_recv(props[20],"m_iTextureFrameIndex",0x46c);
        nested(props[21],"predictable_id",&predictable);
        anim_recv(predict_props[0],"m_PredictableID",0xa4);
        anim_recv(predict_props[1],"m_bIsPlayerSimulated",0x458,0,&byte_recv);
        anim_recv(props[22],"m_bSimulatedEveryTick",0x470,0,&entity_tick_recv);props[22].extra=&binding;
        anim_recv(props[23],"m_bAnimatedEveryTick",0x471,0,&entity_tick_recv);props[23].extra=&binding;
        return true;
    }
};

int __cdecl create_entity(const BaseEntityRecvBindings* b,BaseEntityRecvView* out) noexcept {
    if(!b||!out||!entity_empty(*out))return 0;
    auto* g=new(std::nothrow) BaseEntityRecvGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    *out={g,&g->entity,&g->anim_time,&g->predictable};return 1;
}
int __cdecl destroy_entity(BaseEntityRecvView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<BaseEntityRecvGraph*>(v->handle);
    if(!entity_matches(*g,*v))return 0;
    for(const auto* tab:{&g->entity,&g->anim_time,&g->predictable})if(tab->decoder||tab->initialized||tab->in_main_list)return 0;
    delete g;*v={};return 1;
}
const BaseEntityRecvApi entity_api{version,sizeof(BaseEntityRecvApi),&create_entity,&destroy_entity};

struct DerivedPlayerRecvGraph {
    RecvTable player,local,hand;
    RecvProp props[6],local_props[13],hand_prop;
    bool build(const DerivedPlayerRecvBindings& b) noexcept {
        if(!named_table(b.player,"DT_BasePlayer",15)||!named_table(b.entity,"DT_BaseEntity",24))return false;
        player={props,6,nullptr,"DT_HL2_Player"};local={local_props,13,nullptr,"DT_HL2Local"};
        hand={&hand_prop,1,nullptr,"DT_PlayerHand"};
        nested(props[0],"baseclass",b.player);
        nested(props[1],"m_HL2Local",&local);props[1].offset=0x11c8;
        nested(hand_prop,"baseclass",b.entity);
        anim_recv(props[2],"m_fIsSprinting",0x1224,0,&byte_recv);
        anim_recv(props[3],"m_flFlashTime",0x123c,1);
        anim_recv(props[4],"m_flFlashFadeTime",0x1240,1);
        anim_recv(props[5],"m_flFlashStartAlpha",0x1244,1);
        for(unsigned i=0;i<13;++i){
            const auto& f=derived_local_fields[i];
            anim_recv(local_props[i],f.name,f.offset,f.type,i==12?&handle_recv:f.width==1?&byte_recv:&raw_recv);
        }
        return true;
    }
};

int __cdecl create_derived_player(const DerivedPlayerRecvBindings* b,DerivedPlayerRecvView* out) noexcept {
    if(!b||!out||!derived_empty(*out))return 0;
    auto* g=new(std::nothrow) DerivedPlayerRecvGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    *out={g,&g->player,&g->local,&g->hand};return 1;
}
int __cdecl destroy_derived_player(DerivedPlayerRecvView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<DerivedPlayerRecvGraph*>(v->handle);
    if(!derived_matches(*g,*v))return 0;
    for(const auto* tab:{&g->player,&g->local,&g->hand})if(tab->decoder||tab->initialized||tab->in_main_list)return 0;
    delete g;*v={};return 1;
}
const DerivedPlayerRecvApi derived_api{version,sizeof(DerivedPlayerRecvApi),&create_derived_player,&destroy_derived_player};

struct ClientClassGraph {
    ClientClassRecord records[ClassSubsetCount]{};
    ClientClassRecord* head=nullptr;
    bool build(const ClientClassBindings& b) noexcept {
        if(!class_tables_valid(b))return false;
        for(unsigned i=0;i<ClassSubsetCount;++i)if(!b.creators[i])return false;
        for(unsigned i=0;i<ClassSubsetCount;++i){
            auto& rec=records[i];rec.name=class_names[i];rec.table=b.tables[i];
            rec.create=b.creators[i];rec.next=head;head=&rec;
        }
        return true;
    }
};

int __cdecl create_class_subset(const ClientClassBindings* b,ClientClassView* out) noexcept {
    if(!b||!out||!class_empty(*out))return 0;
    auto* g=new(std::nothrow) ClientClassGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    *out={g,g->head,g->records,ClassSubsetCount};return 1;
}
int __cdecl destroy_class_subset(ClientClassView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<ClientClassGraph*>(v->handle);
    if(!class_matches(*g,*v))return 0;
    for(const auto& rec:g->records){
        const auto* tab=rec.table;
        if(!tab||rec.class_id!=0||tab->decoder||tab->initialized||tab->in_main_list)return 0;
    }
    delete g;*v={};return 1;
}
const ClientClassApi class_api{version,sizeof(ClientClassApi),&create_class_subset,&destroy_class_subset};

struct AggregateRecvGraph {
    using Table=RecvTable;
    BaseEntityRecvGraph entity;
    AnimRecvGraph anim;
    OverlayRecvGraph overlay;
    FlexRecvGraph flex;
    InventoryRecvGraph inventory;
    LocalDataRecvGraph local;
    PlayerLocalRecvGraph private_player;
    BasePlayerRecvGraph player;
    DerivedPlayerRecvGraph derived;
    RecvGraph weapons;
    ClientClassGraph classes;
    bool spawn_bound=false,extra_bound=false;
    Table* tables[aggregate_table_count]{};
    bool build(const AggregateRecvBindings& b) noexcept {
        if(!entity.build(b.entity)||!anim.build({&entity.entity})||
           !overlay.build(b.overlay)||!flex.build({&anim.base,&overlay.overlay})||
           !inventory.build({&flex.flex})||!local.build(b.local)||
           !private_player.build({&local.table,&inventory.ammo_property,b.velocity_context,b.invalidate_velocity})||
           !player.build({&inventory.combat_base,&private_player.table})||
           !derived.build({&player.player,&entity.entity})||
           !weapons.build({&anim.base}))return false;
        ClientClassBindings cb{{&weapons.tables[BaseCombat],&anim.base,&flex.overlay,&inventory.combat_base,
            &entity.entity,&flex.flex,&player.player,&weapons.tables[BaseHL],
            &weapons.tables[MachineGun],&weapons.tables[SelectFire],&derived.hand,&derived.player,
            &weapons.tables[Magnum],&weapons.tables[AssaultRifle],&weapons.tables[Scattergun]},{}};
        for(unsigned i=0;i<ClassSubsetCount;++i)cb.creators[i]=b.creators[i];
        if(!classes.build(cb))return false;
        Table* fixed[]={&entity.entity,&entity.anim_time,&entity.predictable,
            &anim.base,&anim.cycle,&anim.pose,&anim.controllers,
            &overlay.overlay,&overlay.vector,&overlay.length,&overlay.layer,
            &flex.overlay,&flex.flex,&flex.weights,
            &inventory.ammo_table,&inventory.weapons_table,&inventory.combat_local,&inventory.combat_base,
            &local.table,&local.area,&private_player.table,&player.player,&player.state,
            &derived.player,&derived.local,&derived.hand};
        static_assert(sizeof(fixed)/sizeof(fixed[0])+TableCount==aggregate_table_count);
        unsigned i=0;for(auto* tab:fixed)tables[i++]=tab;
        for(auto& tab:weapons.tables)tables[i++]=&tab;
        return true;
    }
};

int __cdecl create_aggregate(const AggregateRecvBindings* b,AggregateRecvView* out) noexcept {
    if(!b||!out||!aggregate_empty(*out))return 0;
    auto* g=new(std::nothrow) AggregateRecvGraph;
    if(!g)return 0;
    if(!g->build(*b)){delete g;return 0;}
    out->handle=g;out->class_head=g->classes.head;out->class_records=g->classes.records;
    out->class_count=ClassSubsetCount;
    for(unsigned i=0;i<aggregate_table_count;++i)out->tables[i]=g->tables[i];
    return 1;
}
int __cdecl destroy_aggregate(AggregateRecvView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<AggregateRecvGraph*>(v->handle);
    if(g->spawn_bound||g->extra_bound||!aggregate_matches(*g,*v))return 0;
    // Preflight every owned object, including nested-only tables. No
    // partial teardown if an engine marker is still set anywhere.
    for(const auto* tab:g->tables)if(!tab||tab->decoder||tab->initialized||tab->in_main_list)return 0;
    for(const auto& rec:g->classes.records)if(rec.class_id!=0)return 0;
    delete g;*v={};return 1;
}
const AggregateRecvApi aggregate_api{version,sizeof(AggregateRecvApi),&create_aggregate,&destroy_aggregate};

unsigned __cdecl creator_size(unsigned slot) noexcept {
    switch(slot){
        case ClPlayer:return 0x1250;
        case ClHand:return 0x480;
        case ClMagnum:case ClAssault:case ClScatter:return 0x830;
        default:return 0;
    }
}
bool creator_writable(void* object,unsigned size) noexcept {
    if(!object||reinterpret_cast<std::uintptr_t>(object)%4||
        !sin_independent::vgui_layout::Readable(object,size))return false;
    MEMORY_BASIC_INFORMATION region{};
    if(VirtualQuery(object,&region,sizeof(region))!=sizeof(region))return false;
    const auto prot=region.Protect&0xffu;
    return prot==PAGE_READWRITE||prot==PAGE_WRITECOPY||
           prot==PAGE_EXECUTE_READWRITE||prot==PAGE_EXECUTE_WRITECOPY;
}
void creator_word(void* object,unsigned offset,std::uint32_t word) noexcept {
    std::memcpy(static_cast<unsigned char*>(object)+offset,&word,4);
}
int __cdecl hl2local_defaults(void* player,unsigned bytes) noexcept {
    if(bytes<0x1250||!creator_writable(player,0x1250))return 0;
    auto* local=static_cast<unsigned char*>(player)+0x11c8;
    for(unsigned off:{4u,8u,0x18u,0x1cu})creator_word(local,off,0);
    for(unsigned off:{0xcu,0x24u,0x28u})creator_word(local,off,0x3f800000u);
    for(unsigned off:{0x2cu,0x54u,0x58u})creator_word(local,off,0xffffffffu);
    for(unsigned off:{0x10u,0x11u,0x20u,0x21u})local[off]=0;
    return 1;
}
int __cdecl player_tail_defaults(void* player,unsigned bytes) noexcept {
    if(bytes<0x1250||!creator_writable(player,0x1250))return 0;
    for(unsigned off:{0x1228u,0x122cu,0x1230u,0x1234u,0x123cu,0x1240u,0x1244u,0x1248u})
        creator_word(player,off,0);
    return 1;
}
int __cdecl weapon_tail_defaults(void* weapon,unsigned bytes) noexcept {
    if(bytes<0x830||!creator_writable(weapon,0x830))return 0;
    auto* raw=static_cast<unsigned char*>(weapon);
    for(unsigned off:{0x788u,0x80cu,0x7e0u,0x7e4u,0x7d8u,0x7dcu})creator_word(raw,off,0xffffffffu);
    for(unsigned off:{0x7ecu,0x7f0u})creator_word(raw,off,0x42820000u);
    for(unsigned off:{0x7f4u,0x7f8u})creator_word(raw,off,0x44800000u);
    for(unsigned off:{0x78cu,0x81cu,0x7d0u,0x79cu})creator_word(raw,off,0);
    raw[0x7fc]=0;
    // Original helper at 0x241e6ed0 returns low AX=0xffff.
    const std::uint16_t invalid=0xffff;
    std::memcpy(raw+0x810,&invalid,2);
    return 1;
}
void* __cdecl create_client_entity(const ClientCreatorBindings* b,unsigned slot,int index,int serial) noexcept {
    const auto bytes=creator_size(slot);
    if(!b||!bytes||!b->allocate||!b->construct||!b->cleanup)return nullptr;
    // Take a copy so a host callback cannot invalidate the binding
    // record while a create operation is in progress.
    const auto host=*b;
    void* object=host.allocate(host.context,slot,bytes);
    if(!object)return nullptr;
    if(!creator_writable(object,bytes)){
        host.cleanup(host.context,slot,object,bytes,CreatorCleanup::Raw);return nullptr;
    }
    std::memset(object,0,bytes);
    if(!host.construct(host.context,slot,object,bytes)){
        host.cleanup(host.context,slot,object,bytes,CreatorCleanup::Raw);return nullptr;
    }
    auto* networkable=static_cast<unsigned char*>(object)+8;
    namespace methods=sin_independent::known_game_system;
    auto refused=methods::CallResult::InvalidObject;
    void* method=methods::VerifiedMethod(object,37,refused);
    if(!method||!methods::VerifiedMethod(networkable,0,refused)){
        host.cleanup(host.context,slot,object,bytes,CreatorCleanup::Constructed);return nullptr;
    }
    using Init=bool (__thiscall *)(void*,int,int);
    // Retail wrappers ignore AL; all five observed original methods
    // return true. Explicitly reject a compatible host's false result.
    if(!reinterpret_cast<Init>(method)(object,index,serial)){
        host.cleanup(host.context,slot,object,bytes,CreatorCleanup::InitAttempted);return nullptr;
    }
    return networkable;
}
const ClientCreatorApi creator_api{version,sizeof(ClientCreatorApi),&creator_size,&create_client_entity,
    &hl2local_defaults,&player_tail_defaults,&weapon_tail_defaults};

namespace registry_methods=sin_independent::known_game_system;
struct ClientRegistry {
    ClientRegistryEntry entries[client_network_limit]{};
    ClientRegistryEntry* head=nullptr;
    ClientRegistryEntry* tail=nullptr;
    unsigned count=0;
    bool busy=false,spawn_bound=false;
    void* context=nullptr;
    RegistryNotify added=nullptr,removed=nullptr;
};
ClientRegistry* registry_get(const ClientRegistryView* v) noexcept {
    if(!v||!v->handle)return nullptr;
    auto* g=static_cast<ClientRegistry*>(v->handle);
    return v->entries==g->entries?g:nullptr;
}
int __cdecl registry_create(const ClientRegistryBindings* b,ClientRegistryView* out) noexcept {
    if(!b||!out||out->handle||out->entries||!b->added||!b->removed||
       !sin_independent::vgui_layout::Readable(b->initial_serials,client_network_limit*sizeof(std::uint16_t)))return 0;
    auto* g=new(std::nothrow) ClientRegistry;
    if(!g)return 0;
    for(unsigned i=0;i<client_network_limit;++i){
        if(b->initial_serials[i]>0x7fff){delete g;return 0;}
        auto& e=g->entries[i];e.serial=b->initial_serials[i];e.prev=&e;e.next=&e;
    }
    g->context=b->context;g->added=b->added;g->removed=b->removed;
    *out={g,g->entries};return 1;
}
int __cdecl registry_destroy(ClientRegistryView* v) noexcept {
    auto* g=registry_get(v);
    if(!g||g->busy||g->spawn_bound||g->count||g->head||g->tail)return 0;
    delete g;*v={};return 1;
}
ClientRegistryEntry* registry_entry(ClientRegistry* g,std::uint32_t handle) noexcept {
    if(!g||handle==0xffffffffu)return nullptr;
    const unsigned index=handle&0xfffu;
    if(index>=client_network_limit)return nullptr;
    auto& e=g->entries[index];
    return e.entity&&e.serial==(handle>>12)?&e:nullptr;
}
void* __cdecl registry_resolve(const ClientRegistryView* v,std::uint32_t handle) noexcept {
    const auto* e=registry_entry(registry_get(v),handle);return e?e->entity:nullptr;
}
int __cdecl registry_state(const ClientRegistryView* v,ClientRegistryState* out) noexcept {
    const auto* g=registry_get(v);
    if(!g||!out)return 0;
    *out={g->head,g->tail,g->count};return 1;
}
using RegistrySetHandle=void (__thiscall *)(void*,const std::uint32_t*);
RegistrySetHandle registry_setter(void* entity) noexcept {
    auto refused=registry_methods::CallResult::InvalidObject;
    return reinterpret_cast<RegistrySetHandle>(registry_methods::VerifiedMethod(entity,1,refused));
}
int __cdecl registry_insert(ClientRegistryView* v,void* entity,int index,int serial,std::uint32_t* handle) noexcept {
    auto* g=registry_get(v);
    if(!g||g->busy||!entity||!handle||index<0||index>=static_cast<int>(client_network_limit)||
       serial< -1||serial>0x7fff)return 0;
    auto& e=g->entries[index];
    if(e.entity||e.prev!=&e||e.next!=&e)return 0;
    // Original overwrites without guard. Native independent callers
    // must not orphan a previously registered object or double-link it.
    for(const auto& other:g->entries)if(other.entity==entity)return 0;
    auto setter=registry_setter(entity);if(!setter)return 0;
    g->busy=true;
    e.entity=entity;if(serial!=-1)e.serial=static_cast<unsigned>(serial);
    e.prev=g->tail;e.next=nullptr;
    if(g->tail)g->tail->next=&e;else g->head=&e;
    g->tail=&e;++g->count;
    const std::uint32_t packed=(e.serial<<12)|static_cast<unsigned>(index);
    setter(entity,&packed);
    g->added(g->context,entity,packed);
    *handle=packed;g->busy=false;return 1;
}
int __cdecl registry_remove(ClientRegistryView* v,std::uint32_t handle) noexcept {
    auto* g=registry_get(v);
    auto* e=registry_entry(g,handle);
    if(!e||g->busy)return 0;
    auto setter=registry_setter(e->entity);if(!setter)return 0;
    g->busy=true;
    const std::uint32_t invalid=0xffffffffu;
    setter(e->entity,&invalid);
    g->removed(g->context,e->entity,handle);
    e->entity=nullptr;e->serial=(e->serial+1)&0x7fffu;
    if(e->prev)e->prev->next=e->next;else g->head=e->next;
    if(e->next)e->next->prev=e->prev;else g->tail=e->prev;
    e->prev=e;e->next=e;--g->count;
    g->busy=false;return 1;
}
const ClientRegistryApi registry_api{version,sizeof(ClientRegistryApi),&registry_create,&registry_destroy,
    &registry_insert,&registry_remove,&registry_resolve,&registry_state};

using GetPrimaryEntity=void* (__thiscall *)(void*);
using GetInterpMap=const ClientInterpMap* (__thiscall *)(void*);
using InterpSetup=void (__thiscall *)(void*,void*,int);
using InterpFlags=int (__thiscall *)(void*);
using InterpSetDuration=void (__thiscall *)(void*,float);
struct WatcherInit {
    ClientInterpEntry entry;
    InterpSetup setup;
    InterpFlags flags;
    InterpSetDuration set_duration;
};
// The original walks a derived-to-base linked chain of maps. Bound the
// independent path and preflight it before invoking any watcher methods.
// The supplied maps and watcher vtables must stay live until completion.
bool prepare_watchers(const ClientInterpMap* map,WatcherInit (&work)[256],unsigned& count) noexcept {
    const ClientInterpMap* seen[32]{};
    unsigned maps=0;
    namespace memory=sin_independent::vgui_layout;
    while(map){
        if(maps==32||!memory::Readable(map,sizeof(*map)))return false;
        for(unsigned i=0;i<maps;++i)if(seen[i]==map)return false;
        seen[maps++]=map;
        ClientInterpMap value{};std::memcpy(&value,map,sizeof(value));
        if(value.count<0||value.capacity<value.count||
           static_cast<unsigned>(value.count)>256-count)return false;
        if(value.count&&!memory::Readable(value.entries,sizeof(ClientInterpEntry)*static_cast<unsigned>(value.count)))return false;
        for(int i=0;i<value.count;++i){
            auto& w=work[count++];
            std::memcpy(&w.entry,&value.entries[i],sizeof(w.entry));
            if(!memory::Readable(w.entry.data,1))return false;
            auto refused=registry_methods::CallResult::InvalidObject;
            w.setup=reinterpret_cast<InterpSetup>(registry_methods::VerifiedMethod(w.entry.watcher,0,refused));
            w.flags=reinterpret_cast<InterpFlags>(registry_methods::VerifiedMethod(w.entry.watcher,5,refused));
            w.set_duration=reinterpret_cast<InterpSetDuration>(registry_methods::VerifiedMethod(w.entry.watcher,1,refused));
            if(!w.setup||!w.flags||!w.set_duration)return false;
        }
        map=value.parent;
    }
    return true;
}
int __cdecl initialize_client(const ClientInitBindings* b,unsigned slot,void* object,
        int index,int serial,std::uint32_t* handle) noexcept {
    const auto bytes=creator_size(slot);
    if(!b||!bytes||!handle||!b->attach_collision||!b->duration||!b->rollback||
       !creator_writable(object,bytes))return 0;
    const auto host=*b;
    auto* g=registry_get(host.registry);
    if(!g||g->busy)return 0;
    auto* raw=static_cast<unsigned char*>(object);
    auto refused=registry_methods::CallResult::InvalidObject;
    auto primary=reinterpret_cast<GetPrimaryEntity>(registry_methods::VerifiedMethod(raw+4,0,refused));
    auto map_method=reinterpret_cast<GetInterpMap>(registry_methods::VerifiedMethod(object,17,refused));
    if(!primary||!map_method||primary(raw+4)!=object)return 0;
    std::uint32_t packed=0;
    if(!registry_insert(host.registry,object,index,serial,&packed))return 0;
    // Registry lookup is allowed during the remaining host operations,
    // but mutation/reentrant initialization is not.
    g->busy=true;
    const auto fail=[&](ClientInitStage stage){
        host.rollback(host.context,object,stage);
        g->busy=false;
        if(!registry_remove(host.registry,packed)){*handle=packed;return -1;}
        return 0;
    };
    if(!host.attach_collision(host.context,raw+0x16c))
        return fail(ClientInitStage::CollisionAttempted);
    // Original vslot17 is evaluated only after registration + collision.
    WatcherInit work[256]{};unsigned count=0;
    if(!prepare_watchers(map_method(object),work,count))
        return fail(ClientInitStage::InterpolationAttempted);
    for(unsigned i=0;i<count;++i){
        auto& w=work[i];
        w.setup(w.entry.watcher,w.entry.data,static_cast<int>(w.entry.kind));
        const int flags=w.flags(w.entry.watcher);
        float duration=0;
        if(!host.duration(host.context,object,flags,&duration)||!std::isfinite(duration)||duration<0)
            return fail(ClientInitStage::InterpolationAttempted);
        w.set_duration(w.entry.watcher,duration);
    }
    std::memcpy(raw+0x50,&index,4);
    *handle=packed;g->busy=false;return 1;
}
const ClientInitApi client_init_api{version,sizeof(ClientInitApi),&initialize_client};

constexpr unsigned spawn_slots[]={ClPlayer,ClHand,ClMagnum,ClAssault,ClScatter};
struct ClientSpawnSession {
    AggregateRecvGraph* graph=nullptr;
    ClientRegistryView registry{};
    ClientCreatorBindings creator{};
    ClientInitBindings init{};
    CreateClientEntity prior[5]{};
    ClientSpawnRecord records[client_network_limit]{};
    unsigned count=0;
    bool busy=false;
};
ClientSpawnSession* active_spawn=nullptr;
bool aggregate_detached(const AggregateRecvGraph& g) noexcept {
    for(const auto* tab:g.tables)if(!tab||tab->decoder||tab->initialized||tab->in_main_list)return false;
    for(const auto& rec:g.classes.records)if(rec.class_id!=0)return false;
    return true;
}
ClientSpawnSession* spawn_get(const ClientSpawnView* v) noexcept {
    return v&&v->handle&&v->handle==active_spawn?active_spawn:nullptr;
}
void* spawn_entity(unsigned slot,int index,int serial) noexcept {
    auto* session=active_spawn;
    const unsigned bytes=creator_size(slot);
    if(!session||session->busy||!bytes||index<0||index>=static_cast<int>(client_network_limit)||
       serial< -1||serial>0x7fff)return nullptr;
    auto* g=registry_get(&session->registry);
    auto& record=session->records[index];
    if(!g||g->busy||g->entries[index].entity||record.primary)return nullptr;
    session->busy=true;
    const auto& host=session->creator;
    void* object=host.allocate(host.context,slot,bytes);
    if(!object){session->busy=false;return nullptr;}
    const auto fail=[&](CreatorCleanup stage)->void* {
        host.cleanup(host.context,slot,object,bytes,stage);
        session->busy=false;return nullptr;
    };
    if(!creator_writable(object,bytes))return fail(CreatorCleanup::Raw);
    std::memset(object,0,bytes);
    if(!host.construct(host.context,slot,object,bytes))return fail(CreatorCleanup::Raw);
    auto* networkable=static_cast<unsigned char*>(object)+8;
    auto refused=registry_methods::CallResult::InvalidObject;
    if(!registry_methods::VerifiedMethod(networkable,0,refused))return fail(CreatorCleanup::Constructed);
    std::uint32_t packed=0;
    // Execute our recovered initializer, not a synthetic slot37 stub.
    const int result=initialize_client(&session->init,slot,object,index,serial,&packed);
    if(result==0)return fail(CreatorCleanup::Constructed);
    record={object,networkable,packed,slot,result==1?1u:0u};
    ++session->count;session->busy=false;
    // Retained rollback failures are accessible through record/release,
    // but never published as successfully initialized networkables.
    return result==1?networkable:nullptr;
}
template<unsigned Slot> void* __cdecl spawn_thunk(int index,int serial){
    return spawn_entity(Slot,index,serial);
}
constexpr CreateClientEntity spawn_creators[]={
    &spawn_thunk<ClPlayer>,&spawn_thunk<ClHand>,&spawn_thunk<ClMagnum>,&spawn_thunk<ClAssault>,&spawn_thunk<ClScatter>
};
int __cdecl spawn_bind(const ClientSpawnBindings* b,ClientSpawnView* out) noexcept {
    if(!b||!out||out->handle||active_spawn||!b->aggregate||!b->aggregate->handle||
       !b->creator.allocate||!b->creator.construct||!b->creator.cleanup||
       !b->init.attach_collision||!b->init.duration||!b->init.rollback)return 0;
    auto* graph=static_cast<AggregateRecvGraph*>(b->aggregate->handle);
    auto* registry=registry_get(b->init.registry);
    if(!aggregate_matches(*graph,*b->aggregate)||graph->spawn_bound||!aggregate_detached(*graph)||
       !registry||registry->busy||registry->spawn_bound)return 0;
    auto* session=new(std::nothrow) ClientSpawnSession;
    if(!session)return 0;
    session->graph=graph;session->registry=*b->init.registry;session->creator=b->creator;
    session->init=b->init;session->init.registry=&session->registry;
    for(unsigned i=0;i<5;++i){
        auto& rec=graph->classes.records[spawn_slots[i]];
        session->prior[i]=rec.create;rec.create=spawn_creators[i];
    }
    graph->spawn_bound=true;registry->spawn_bound=true;
    active_spawn=session;out->handle=session;return 1;
}
int __cdecl spawn_record(const ClientSpawnView* v,unsigned index,ClientSpawnRecord* out) noexcept {
    const auto* session=spawn_get(v);
    if(!session||!out||index>=client_network_limit||!session->records[index].primary)return 0;
    *out=session->records[index];return 1;
}
int __cdecl spawn_release(ClientSpawnView* v,std::uint32_t handle) noexcept {
    auto* session=spawn_get(v);
    const unsigned index=handle&0xfffu;
    if(!session||session->busy||index>=client_network_limit)return 0;
    auto& record=session->records[index];
    if(!record.primary||record.handle!=handle||
       registry_resolve(&session->registry,handle)!=record.primary)return 0;
    session->busy=true;
    if(!registry_remove(&session->registry,handle)){session->busy=false;return 0;}
    session->creator.cleanup(session->creator.context,record.slot,record.primary,creator_size(record.slot),
        record.initialized?CreatorCleanup::InitAttempted:CreatorCleanup::Constructed);
    record={};--session->count;session->busy=false;return 1;
}
int __cdecl spawn_unbind(ClientSpawnView* v) noexcept {
    auto* session=spawn_get(v);
    if(!session||session->busy||session->count||!aggregate_detached(*session->graph))return 0;
    auto* registry=registry_get(&session->registry);
    if(!registry||registry->busy)return 0;
    for(unsigned i=0;i<5;++i)
        if(session->graph->classes.records[spawn_slots[i]].create!=spawn_creators[i])return 0;
    for(unsigned i=0;i<5;++i)session->graph->classes.records[spawn_slots[i]].create=session->prior[i];
    session->graph->spawn_bound=false;registry->spawn_bound=false;
    active_spawn=nullptr;delete session;*v={};return 1;
}
const ClientSpawnApi spawn_api{version,sizeof(ClientSpawnApi),&spawn_bind,&spawn_release,&spawn_record,&spawn_unbind};

struct ExtraRecvGraph {
    AggregateRecvGraph* base=nullptr;
    RecvTable tables[ExtraEntityCount];
    RecvProp props[ExtraEntityCount][7];
    ClientClassRecord records[ExtraEntityCount]{};
    ClientClassRecord* head=nullptr;
    bool build(AggregateRecvGraph& core,const ExtraRecvBindings& b) noexcept {
        for(auto creator:b.creators)if(!creator)return false;
        RecvTable* parents[]={&core.inventory.combat_base,
            &core.weapons.tables[BaseHL],
            &tables[ExGrunt],
            &tables[ExAI],
            &tables[ExAI],
            &core.weapons.tables[SelectFire],
            &core.inventory.combat_base,
            &core.weapons.tables[BaseHL],
            &core.weapons.tables[SelectFire],
            &core.weapons.tables[BaseHL],
            &tables[ExBludgeon]};
        for(unsigned i=0;i<ExtraEntityCount;++i){
            tables[i]={props[i],extra_table_counts[i],nullptr,extra_table_names[i]};
            nested(props[i][0],"baseclass",parents[i]);
            auto& rec=records[i];rec.name=extra_class_names[i];rec.table=&tables[i];
            rec.create=b.creators[i];rec.next=head;head=&rec;
        }
        auto* ai=props[ExAI];
        anim_recv(ai[1],"m_lifeState",0x94);
        anim_recv(ai[2],"m_bPerformAvoidance",0xd48,0,&byte_recv);
        anim_recv(ai[3],"m_bIsMoving",0xd49,0,&byte_recv);
        anim_recv(ai[4],"m_bFadeCorpse",0xd4a,0,&byte_recv);
        anim_recv(ai[5],"m_iDeathPose",0xd4c);
        anim_recv(ai[6],"m_iDeathFrame",0xd50);
        auto* grunt=props[ExGrunt];
        anim_recv(grunt[1],"m_receivedDecapitatingBlow",0xd74,0,&byte_recv);
        anim_recv(grunt[2],"m_decapitateVelocity",0xd78,2,&vector_recv);
        auto* holo=props[ExHologram];
        anim_recv(holo[1],"m_state",0xd7c);
        anim_recv(holo[2],"m_timeEffectStops",0xd80,1);
        anim_recv(props[ExShockwave][1],"m_vecTargetPos",0xd64,2,&vector_recv);
        return true;
    }
};

int __cdecl create_extra(const ExtraRecvBindings* b,ExtraRecvView* out) noexcept {
    if(!b||!b->base||!b->base->handle||!out||out->handle||out->head||out->records)return 0;
    for(auto* table:out->tables)if(table)return 0;
    auto* core=static_cast<AggregateRecvGraph*>(b->base->handle);
    if(core->extra_bound||!aggregate_matches(*core,*b->base))return 0;
    auto* g=new(std::nothrow) ExtraRecvGraph;
    if(!g)return 0;
    if(!g->build(*core,*b)){delete g;return 0;}
    g->base=core;core->extra_bound=true;
    out->handle=g;out->head=g->head;out->records=g->records;
    for(unsigned i=0;i<ExtraEntityCount;++i)out->tables[i]=&g->tables[i];
    return 1;
}
int __cdecl destroy_extra(ExtraRecvView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<ExtraRecvGraph*>(v->handle);
    if(v->head!=g->head||v->records!=g->records)return 0;
    for(unsigned i=0;i<ExtraEntityCount;++i){
        const auto& tab=g->tables[i];const auto& rec=g->records[i];
        if(v->tables[i]!=&tab||rec.class_id!=0||tab.decoder||tab.initialized||tab.in_main_list)return 0;
    }
    g->base->extra_bound=false;
    delete g;*v={};return 1;
}
const ExtraRecvApi extra_api{version,sizeof(ExtraRecvApi),&create_extra,&destroy_extra};

int __cdecl hologram_visibility(void* object,unsigned bytes,float now,HologramFlicker* shared,unsigned* visible) noexcept {
    if(bytes<0xd88||!hologram_writable(object,0xd88)||!shared||shared->visible>1||!visible||!std::isfinite(now))return 0;
    const int state=hologram_read<int>(object,0xd7c);
    if(state!=1&&state!=3){*visible=state!=0?1u:0u;return 1;}
    const int cursor=hologram_read<int>(object,0xd74);
    if(cursor<0)return 0;
    constexpr float thresholds[]={0.1f,0.15f,0.25f,0.26f,0.27f,0.28f,0.3f,0.39f,0.4f,
        0.45f,0.46f,0.5f,0.51f,0.6f,0.7f,0.85f,0.9f,0.95f,0.99f};
    const double remaining=static_cast<double>(hologram_read<float>(object,0xd80))-now;
    const double progress=1.0-remaining*(state==1?0.5:1.0);
    if(cursor<19&&progress>=thresholds[cursor]){
        hologram_write(object,0xd74,cursor+1);
        shared->visible=1u-shared->visible;
    }
    *visible=shared->visible;return 1;
}
const HologramClientApi hologram_api{version,sizeof(HologramClientApi),&hologram_visibility};

int __cdecl grunt_death(HologramControl* ctrl,const GruntDeathBindings* b,int argument,GruntDeathResult* out) noexcept {
    namespace memory=sin_independent::vgui_layout;
    if(!ctrl||!b||!out||ctrl->busy||ctrl->blocked||ctrl->bytes<0xd88||
       !memory::Readable(ctrl->object,0xd88)||!b->base||!b->suppressed||
       !b->find_group||!b->group_count||!b->set_group||!b->random_float||!b->random_int||
       !b->orientation||!b->position||!b->emit)return 0;
    ctrl->busy=1;
    const auto host=*b;
    auto* source=ctrl->object;
    GruntDeathResult result{};
    const auto finish=[&](bool ok){
        ctrl->busy=0;*out=result;
        if(!ok){ctrl->blocked=1;return -1;}
        return 1;
    };
    if(!host.base(host.context,source,argument,&result.ragdoll))return finish(false);
    if(!result.ragdoll||!hologram_read<unsigned char>(source,0xd74))return finish(true);
    unsigned suppressed=0;
    if(!host.suppressed(host.context,&suppressed)||suppressed>1)return finish(false);
    if(suppressed)return finish(true);
    int group=0,count=0;
    if(!host.find_group(host.context,result.ragdoll,"heads",&group)||
       !host.group_count(host.context,result.ragdoll,group,&count))return finish(false);
    // The original subtracts one unconditionally from the group count.
    // Use modulo DWORD subtraction to avoid signed overflow.
    const auto last=static_cast<std::uint32_t>(count)-1u;
    int value=0;std::memcpy(&value,&last,4);
    if(!host.set_group(host.context,result.ragdoll,group,value))return finish(false);
    float sample=0;
    if(!host.random_float(host.context,0,1,&sample)||!std::isfinite(sample)||sample<0||sample>1)return finish(false);
    const auto path=sample>0.1f?GruntHeadPath::EffectRecord:GruntHeadPath::TimedModel;
    GruntHeadEffect effect{};
    effect.model="models/characters/mobgrunt/decaphead.mdl";effect.source=source;
    for(auto& angle:effect.angles){
        int random=0;
        if(!host.random_int(host.context,0,32767,&random)||random<0||random>32767)return finish(false);
        // Original vector helper: randomInt * float(1/32767) * 360 - 180.
        angle=static_cast<float>(static_cast<double>(random)*0.000030518509447574615478515625*360.0-180.0);
    }
    if(path==GruntHeadPath::EffectRecord){
        if(!host.orientation(host.context,source,effect.orientation))return finish(false);
        effect.entity_index=-1;
    }else effect.lifetime=4;
    if(!host.position(host.context,source,effect.position))return finish(false);
    std::memcpy(effect.velocity,static_cast<const unsigned char*>(source)+0xd78,12);
    effect.group=0; // original effect record tail zeroes / timed model defaults
    // The effect API is semantic; original opaque pointer record and
    // model lookup are host work, not fabricated binary stack structures.
    if(!host.emit(host.context,path,&effect))return finish(false);
    result.path=path;
    return finish(true);
}
int __cdecl grunt_death_ack(HologramControl* ctrl) noexcept {
    if(!ctrl||ctrl->busy||!ctrl->blocked||ctrl->bytes<0xd88||
       !sin_independent::vgui_layout::Readable(ctrl->object,0xd88))return 0;
    ctrl->blocked=0;return 1;
}
const GruntDeathApi grunt_death_api{version,sizeof(GruntDeathApi),&grunt_death,&grunt_death_ack};
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
#pragma comment(linker,"/EXPORT:SiN_GetBasePlayerSendGraphApi=_SiN_GetBasePlayerSendGraphApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::BasePlayerSendApi* __cdecl
SiN_GetBasePlayerSendGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(BasePlayerSendApi)?&base_player_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetLocalDataSendGraphApi=_SiN_GetLocalDataSendGraphApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::LocalDataSendApi* __cdecl
SiN_GetLocalDataSendGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(LocalDataSendApi)?&local_data_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetBaseEntitySendGraphApi=_SiN_GetBaseEntitySendGraphApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::BaseEntitySendApi* __cdecl
SiN_GetBaseEntitySendGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(BaseEntitySendApi)?&entity_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetDerivedPlayerSendGraphApi=_SiN_GetDerivedPlayerSendGraphApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::DerivedPlayerSendApi* __cdecl
SiN_GetDerivedPlayerSendGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(DerivedPlayerSendApi)?&derived_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetServerClassSubsetApi=_SiN_GetServerClassSubsetApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerClassApi* __cdecl
SiN_GetServerClassSubsetApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ServerClassApi)?&class_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetNetworkSendAggregateApi=_SiN_GetNetworkSendAggregateApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::AggregateSendApi* __cdecl
SiN_GetNetworkSendAggregateApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(AggregateSendApi)?&aggregate_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetExtraEntitySendApi=_SiN_GetExtraEntitySendApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ExtraSendApi* __cdecl
SiN_GetExtraEntitySendApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ExtraSendApi)?&extra_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetHologramServerApi=_SiN_GetHologramServerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::HologramServerApi* __cdecl
SiN_GetHologramServerApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(HologramServerApi)?&hologram_api:nullptr;
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
#pragma comment(linker,"/EXPORT:SiN_GetBasePlayerRecvGraphApi=_SiN_GetBasePlayerRecvGraphApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::BasePlayerRecvApi* __cdecl
SiN_GetBasePlayerRecvGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(BasePlayerRecvApi)?&base_player_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetLocalDataRecvGraphApi=_SiN_GetLocalDataRecvGraphApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::LocalDataRecvApi* __cdecl
SiN_GetLocalDataRecvGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(LocalDataRecvApi)?&local_data_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetBaseEntityRecvGraphApi=_SiN_GetBaseEntityRecvGraphApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::BaseEntityRecvApi* __cdecl
SiN_GetBaseEntityRecvGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(BaseEntityRecvApi)?&entity_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetDerivedPlayerRecvGraphApi=_SiN_GetDerivedPlayerRecvGraphApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::DerivedPlayerRecvApi* __cdecl
SiN_GetDerivedPlayerRecvGraphApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(DerivedPlayerRecvApi)?&derived_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetClientClassSubsetApi=_SiN_GetClientClassSubsetApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientClassApi* __cdecl
SiN_GetClientClassSubsetApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ClientClassApi)?&class_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetNetworkRecvAggregateApi=_SiN_GetNetworkRecvAggregateApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::AggregateRecvApi* __cdecl
SiN_GetNetworkRecvAggregateApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(AggregateRecvApi)?&aggregate_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetClientEntityCreatorApi=_SiN_GetClientEntityCreatorApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientCreatorApi* __cdecl
SiN_GetClientEntityCreatorApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ClientCreatorApi)?&creator_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetClientEntityRegistryApi=_SiN_GetClientEntityRegistryApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientRegistryApi* __cdecl
SiN_GetClientEntityRegistryApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ClientRegistryApi)?&registry_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetClientEntityInitApi=_SiN_GetClientEntityInitApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientInitApi* __cdecl
SiN_GetClientEntityInitApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ClientInitApi)?&client_init_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetClientSpawnSessionApi=_SiN_GetClientSpawnSessionApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientSpawnApi* __cdecl
SiN_GetClientSpawnSessionApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ClientSpawnApi)?&spawn_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetExtraEntityRecvApi=_SiN_GetExtraEntityRecvApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ExtraRecvApi* __cdecl
SiN_GetExtraEntityRecvApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ExtraRecvApi)?&extra_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetHologramClientApi=_SiN_GetHologramClientApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::HologramClientApi* __cdecl
SiN_GetHologramClientApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(HologramClientApi)?&hologram_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetGruntDeathApi=_SiN_GetGruntDeathApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::GruntDeathApi* __cdecl
SiN_GetGruntDeathApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(GruntDeathApi)?&grunt_death_api:nullptr;
}
#endif
