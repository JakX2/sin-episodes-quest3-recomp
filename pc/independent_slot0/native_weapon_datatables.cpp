#include "native_jessica_script.h"
#include "native_count_interrogation.h"
#include "native_character_response.h"
#include "native_action_monitor.h"
#include <cstdio>
#include "native_radio_message.h"
#include "native_action_listener.h"
#include "native_radio_hud.h"
#include "native_radio_entity.h"
#include "native_radio_transport.h"
#include "native_generic_actor.h"
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
template<unsigned N> bool prop_key_read(const char* source,char (&dest)[N]) noexcept {
    if(!source)return false;
    for(unsigned i=0;i<N;++i){
        if(!sin_independent::vgui_layout::Readable(source+i,1))return false;
        if(!(dest[i]=source[i]))return true;
    }
    return false;
}
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
    bool extra_bound=false,keypad_bound=false,phone_bound=false;
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
    if(g->extra_bound||g->keypad_bound||g->phone_bound||!aggregate_matches(*g,*v))return 0;
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

void __cdecl keypad_combination_send(const s::SendProp*,const void*,const void* data,s::DVariant* out,int,int){
    if(!out)return;
    const char* value=nullptr;
    if(data)std::memcpy(&value,data,4);
    if(!value)value="";
    std::memcpy(out->words,&value,4);
}
struct KeypadSendGraph {
    AggregateSendGraph* base=nullptr;
    s::SendTable tables[KeyTableCount];
    s::SendProp props[KeyTableCount][7];
    ServerClassRecord records[KeyTableCount]{};
    ServerClassRecord* head=nullptr;
    bool build(AggregateSendGraph& core,const KeypadSendBindings&) noexcept {
        for(unsigned i=0;i<KeyTableCount;++i)
            if(!tables[i].Construct(props[i],keypad_counts[i],keypad_tables[i]))return false;
        for(unsigned i=0;i<KeyTableCount;++i){
            if(!s::MakeDataTableProp(props[i][0],"baseclass",0,i==KeyScreen?&core.entity.entity:&tables[KeyScreen],&s::DirectTable))return false;
            auto& rec=records[i];rec.name=keypad_classes[i];rec.table=&tables[i];
            auto** link=&head;
            while(*link&&ascii_class_compare((*link)->name,rec.name)<=0)link=&(*link)->next;
            rec.next=*link;*link=&rec;
        }
        auto* sc=props[KeyScreen];
        anim_float(sc[1],"m_flWidth",0x39c,0,4,0,0,0);
        anim_float(sc[2],"m_flHeight",0x3a0,0,4,0,0,0);
        anim_int(sc[3],"m_nAttachmentIndex",0x3a8,5,1);
        anim_int(sc[4],"m_nPanelName",0x3a4,8,1);
        anim_int(sc[5],"m_fScreenFlags",0x3b0,3,1);
        anim_int(sc[6],"m_nOverlayMaterial",0x3ac,10,1);
        for(unsigned i:{KeyCode,KeyProp})anim_int(props[i][1],"m_bDisabled",0x3c8,1,1,true);
        auto& combo=props[KeyCode][2];
        combo.name="m_combination";combo.type=3;combo.offset=0x3cc;combo.flags=0x200;combo.scalarProxy=&keypad_combination_send;
        auto& input=props[KeyProp][2];
        input.name="m_sInput";input.type=3;input.offset=0x3c9;input.scalarProxy=&player_place_send;
        return true;
    }
};

int __cdecl create_keypad(const KeypadSendBindings* b,KeypadSendView* out) noexcept {
    if(!b||!b->base||!b->base->handle||!out||out->handle||out->head||out->records)return 0;
    for(auto* tab:out->tables)if(tab)return 0;
    auto* base=static_cast<AggregateSendGraph*>(b->base->handle);
    if(base->keypad_bound||!aggregate_matches(*base,*b->base))return 0;
    auto* g=new(std::nothrow) KeypadSendGraph;
    if(!g)return 0;
    if(!g->build(*base,*b)){delete g;return 0;}
    g->base=base;base->keypad_bound=true;
    out->handle=g;out->head=g->head;out->records=g->records;
    for(unsigned i=0;i<KeyTableCount;++i)out->tables[i]=&g->tables[i];
    return 1;
}
int __cdecl destroy_keypad(KeypadSendView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<KeypadSendGraph*>(v->handle);
    if(v->head!=g->head||v->records!=g->records)return 0;
    for(unsigned i=0;i<KeyTableCount;++i){
        const auto& tab=g->tables[i];const auto& rec=g->records[i];
        if(v->tables[i]!=&tab||rec.class_id!=0||rec.instance_baseline!=0xffff||tab.precalc||tab.initialized||tab.written)return 0;
    }
    g->base->keypad_bound=false;delete g;*v={};return 1;
}
const KeypadSendApi keypad_api{version,sizeof(KeypadSendApi),&create_keypad,&destroy_keypad};

int __cdecl keypad_enable(HologramControl* ctrl,const KeypadServerBindings* b,unsigned enabled) noexcept {
    if(!ctrl||!b||!b->dirty||ctrl->busy||ctrl->blocked||enabled>1||ctrl->bytes<0x400||
       !hologram_writable(ctrl->object,0x400))return 0;
    const unsigned char old=hologram_read<unsigned char>(ctrl->object,0x3c8);
    if((old==0)==(enabled!=0))return 1;
    ctrl->busy=1;
    if(!b->dirty(b->context,ctrl->object,0x3c8)){
        ctrl->busy=0;ctrl->blocked=1;return -1;
    }
    hologram_write(ctrl->object,0x3c8,static_cast<unsigned char>(1-enabled));
    ctrl->busy=0;return 1;
}
int __cdecl keypad_event(HologramControl* ctrl,const KeypadServerBindings* b,const char* event) noexcept {
    if(!ctrl||!b||!b->output||ctrl->busy||ctrl->blocked||ctrl->bytes<0x400||
       !hologram_writable(ctrl->object,0x400)||!event)return 0;
    // Bounded C string, do not read a corrupt caller's arbitrary span.
    char name[64]{};unsigned i=0;
    for(;i<sizeof(name);++i){
        if(!sin_independent::vgui_layout::Readable(event+i,1))return 0;
        if(!(name[i]=event[i]))break;
    }
    if(i==sizeof(name))return 0;
    unsigned offset=0;
    if(!std::strcmp(name,"keypad_combo_entered"))offset=0x3d0;
    else if(!std::strcmp(name,"bad_keypad_combo_entered"))offset=0x3e8;
    else return 1;
    ctrl->busy=1;
    const int ok=b->output(b->context,ctrl->object,offset,ctrl->object,ctrl->object,0.0f);
    ctrl->busy=0;if(!ok){ctrl->blocked=1;return -1;}return 1;
}
int __cdecl keypad_ack(HologramControl* ctrl) noexcept {
    if(!ctrl||ctrl->busy||!ctrl->blocked||ctrl->bytes<0x400||!hologram_writable(ctrl->object,0x400))return 0;
    ctrl->blocked=0;return 1;
}
const KeypadServerApi keypad_behavior{version,sizeof(KeypadServerApi),&keypad_enable,&keypad_event,&keypad_ack};

bool prop_key_valid(const HologramControl* ctrl) noexcept {
    return ctrl&&!ctrl->busy&&!ctrl->blocked&&ctrl->bytes>=0x41c&&hologram_writable(ctrl->object,0x41c);
}
int prop_key_finish(HologramControl* ctrl,bool ok) noexcept {
    ctrl->busy=0;if(!ok){ctrl->blocked=1;return -1;}return 1;
}
int __cdecl prop_key_enable(HologramControl* ctrl,const PropKeypadServerBindings* b,unsigned enabled) noexcept {
    if(!prop_key_valid(ctrl)||!b||!b->dirty||enabled>1)return 0;
    const auto value=static_cast<unsigned char>(1-enabled);
    if(hologram_read<unsigned char>(ctrl->object,0x3c8)==value)return 1;
    ctrl->busy=1;
    if(!b->dirty(b->context,ctrl->object,0x3c8))return prop_key_finish(ctrl,false);
    hologram_write(ctrl->object,0x3c8,value);return prop_key_finish(ctrl,true);
}
bool prop_key_output(HologramControl* ctrl,const PropKeypadServerBindings& host) noexcept {
    auto* text=static_cast<char*>(ctrl->object)+0x3c9;
    return host.output(host.context,ctrl->object,0x3ec,*text?text:nullptr,ctrl->object,ctrl->object,0)!=0;
}
int __cdecl prop_key_set(HologramControl* ctrl,const PropKeypadServerBindings* b,const char* value) noexcept {
    if(!prop_key_valid(ctrl)||!b||!b->dirty||!b->output)return 0;
    // Q_strncpy(...,31): at most 30 chars plus NUL.
    char text[31]{};
    if(value)for(unsigned i=0;i<30;++i){
        if(!sin_independent::vgui_layout::Readable(value+i,1))return 0;
        if(!(text[i]=value[i]))break;
    }
    ctrl->busy=1;const auto host=*b;
    if(!host.dirty(host.context,ctrl->object,0x3c9))return prop_key_finish(ctrl,false);
    std::memcpy(static_cast<char*>(ctrl->object)+0x3c9,text,std::strlen(text)+1);
    return prop_key_finish(ctrl,prop_key_output(ctrl,host));
}
int __cdecl prop_key_event(HologramControl* ctrl,const PropKeypadServerBindings* b,const char* name,void* event) noexcept {
    if(!prop_key_valid(ctrl)||!b||!event||!b->dirty||!b->entity_index||!b->event_index||
       !b->event_command||!b->output||!b->sound||!b->base_event)return 0;
    char event_name[64]{};
    if(!prop_key_read(name,event_name))return 0;
    ctrl->busy=1;const auto host=*b;
    if(std::strcmp(event_name,"prop_keypad_input"))
        return prop_key_finish(ctrl,host.base_event(host.context,ctrl->object,event)!=0);
    int index=0,received=0;
    if(!host.entity_index(host.context,ctrl->object,&index)||
       !host.event_index(host.context,event,&received))return prop_key_finish(ctrl,false);
    if(index!=received)return prop_key_finish(ctrl,true);
    char command[256]{};
    if(!host.event_command(host.context,event,command,sizeof(command))||
       !std::memchr(command,0,sizeof(command)))return prop_key_finish(ctrl,false);
    auto* text=static_cast<char*>(ctrl->object)+0x3c9;
    const auto* end=static_cast<const char*>(std::memchr(text,0,32));
    if(!end)return prop_key_finish(ctrl,false);
    const auto length=static_cast<unsigned>(end-text);
    // Q_strncat(...,32,1): at most one character; dirty/output/sound
    // still execute for a full buffer or an empty command.
    if(!host.dirty(host.context,ctrl->object,0x3c9))return prop_key_finish(ctrl,false);
    if(length<31&&command[0]){text[length]=command[0];text[length+1]=0;}
    if(!prop_key_output(ctrl,host))return prop_key_finish(ctrl,false);
    char sound[266]="telephone.";
    std::memcpy(sound+10,command,std::strlen(command)+1);
    return prop_key_finish(ctrl,host.sound(host.context,ctrl->object,sound)!=0);
}
int __cdecl prop_key_ack(HologramControl* ctrl) noexcept {
    if(!ctrl||ctrl->busy||!ctrl->blocked||ctrl->bytes<0x41c||!hologram_writable(ctrl->object,0x41c))return 0;
    ctrl->blocked=0;return 1;
}
const PropKeypadServerApi prop_key_api{version,sizeof(PropKeypadServerApi),&prop_key_enable,&prop_key_set,&prop_key_event,&prop_key_ack};

struct PhoneSendGraph {
    AggregateSendGraph* base=nullptr;
    s::SendTable table{};
    s::SendProp prop{};
    ServerClassRecord record{};
    bool build(AggregateSendGraph& core,const PhoneSendBindings& ) noexcept {
        if(!table.Construct(&prop,1,"DT_CPropPhone")||
            !s::MakeDataTableProp(prop,"baseclass",0,&core.anim.base,&s::DirectTable))return false;
        record.name="CPropPhone";record.table=&table;return true;
    }
};
int __cdecl create_phone(const PhoneSendBindings* b,PhoneSendView* out) noexcept {
    if(!b||!b->base||!b->base->handle||!out||out->handle||out->table||out->record)return 0;
    auto* base=static_cast<AggregateSendGraph*>(b->base->handle);
    if(base->phone_bound||!aggregate_matches(*base,*b->base))return 0;
    auto* g=new(std::nothrow) PhoneSendGraph;
    if(!g)return 0;
    if(!g->build(*base,*b)){delete g;return 0;}
    g->base=base;base->phone_bound=true;*out={g,&g->table,&g->record};return 1;
}
int __cdecl destroy_phone(PhoneSendView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<PhoneSendGraph*>(v->handle);
    if(v->table!=&g->table||v->record!=&g->record||g->table.precalc||g->table.initialized||g->table.written||g->record.class_id||g->record.instance_baseline!=0xffff)return 0;
    g->base->phone_bound=false;delete g;*v={};return 1;
}
const PhoneSendApi phone_api{version,sizeof(PhoneSendApi),&create_phone,&destroy_phone};

constexpr std::uint32_t phone_invalid=0xffffffffu;
constexpr unsigned phone_disabled=0x494,phone_fail=0x498,phone_matched=0x4a4,
    phone_keypad=0x4a8,phone_panel=0x4ac,phone_book=0x4b0;
bool phone_valid(const HologramControl* ctrl) noexcept {
    return ctrl&&!ctrl->busy&&!ctrl->blocked&&ctrl->bytes>=0x4b4&&hologram_writable(ctrl->object,0x4b4);
}
int phone_finish(HologramControl* ctrl,bool ok) noexcept {
    ctrl->busy=0;if(!ok){ctrl->blocked=1;return -1;}return 1;
}
int __cdecl phone_defaults(void* phone,unsigned bytes) noexcept {
    if(bytes<0x4b4||!hologram_writable(phone,0x4b4))return 0;
    hologram_write(phone,phone_disabled,static_cast<unsigned char>(0));
    for(unsigned off:{0x498u,0x49cu,0x4a0u,0x4a4u,0x4acu,0x4b0u})hologram_write(phone,off,0u);
    hologram_write(phone,phone_keypad,phone_invalid);return 1;
}
bool phone_schedule(void* phone,const PhoneBindings& host) noexcept {
    float now=0;
    if(!host.clock(host.context,&now)||!std::isfinite(now))return false;
    const float when=static_cast<float>(static_cast<double>(now)+0.25);
    return std::isfinite(when)&&host.schedule(host.context,phone,when)!=0;
}
int __cdecl phone_enable(HologramControl* ctrl,const PhoneBindings* b,unsigned enabled) noexcept {
    if(!phone_valid(ctrl)||!b||enabled>1||(enabled&&(!b->clock||!b->schedule)))return 0;
    ctrl->busy=1;const auto host=*b;
    hologram_write(ctrl->object,phone_disabled,static_cast<unsigned char>(1-enabled));
    return phone_finish(ctrl,!enabled||phone_schedule(ctrl->object,host));
}
bool phone_resolve(const PhoneBindings& host,std::uint32_t handle,HologramControl*& screen) noexcept {
    screen=nullptr;return host.resolve_screen(host.context,handle,&screen)!=0;
}
int __cdecl phone_think(HologramControl* ctrl,const PhoneBindings* b) noexcept {
    if(!phone_valid(ctrl)||!b||!b->clock||!b->schedule||!b->create_screen||!b->resolve_screen||
       !b->configure_screen||!b->remove_screen||!b->lookup||!b->sound||!b->keypad.dirty||!b->keypad.output)return 0;
    ctrl->busy=1;const auto host=*b;auto* phone=ctrl->object;
    auto handle=hologram_read<std::uint32_t>(phone,phone_keypad);
    const auto* book=hologram_read<const void*>(phone,phone_book);
    HologramControl* screen=nullptr;
    const auto fail=[&](){return phone_finish(ctrl,false);};
    if(hologram_read<unsigned char>(phone,phone_disabled)||!book){
        if(handle!=phone_invalid){
            if(!phone_resolve(host,handle,screen)||
                !host.remove_screen(host.context,handle,screen?screen->object:nullptr))return fail();
            hologram_write(phone,phone_keypad,phone_invalid);
        }
        return phone_finish(ctrl,true);
    }
    if(handle==phone_invalid){
        if(!host.create_screen(host.context,phone,"prop_keypad",&handle))return fail();
        hologram_write(phone,phone_keypad,handle);
        if(handle==phone_invalid)return phone_finish(ctrl,true);
        const char* panel=hologram_read<const char*>(phone,phone_panel);
        if(!panel){
            panel="prop_keypad";hologram_write(phone,phone_panel,panel);
        }
        char name[256]{};
        if(!prop_key_read(panel,name)||!phone_resolve(host,handle,screen)||!prop_key_valid(screen)||
           !host.configure_screen(host.context,phone,screen->object,"keypad",name,16,16))return fail();
    }
    // Original re-resolves the packed handle for the read and clear;
    // host must maintain a live, stable, type-checked CPropKeypad.
    if(!phone_resolve(host,handle,screen)||!prop_key_valid(screen))return fail();
    auto* text=static_cast<const char*>(screen->object)+0x3c9;
    const auto* end=static_cast<const char*>(std::memchr(text,0,32));
    if(!end)return fail();
    const int length=static_cast<int>(end-text);
    if(length>=4&&length>hologram_read<int>(phone,phone_matched)){
        unsigned found=0;const char* sound=nullptr;
        if(!host.lookup(host.context,book,text,&found,&sound)||found>1)return fail();
        if(!found)sound=hologram_read<const char*>(phone,phone_fail);
        char sample[256]{};
        if(sound&&!prop_key_read(sound,sample))return fail();
        if(found&&sample[0]=='*'){
            // The original preserves the input and stores the matched
            // length before emitting the prefix sound (without '*').
            hologram_write(phone,phone_matched,length);
            if(!host.sound(host.context,phone,sample+1))return fail();
        }else{
            if((found||sound)&&!host.sound(host.context,phone,sample))return fail();
            if(!phone_resolve(host,handle,screen)||!prop_key_valid(screen)||
               prop_key_set(screen,&host.keypad,nullptr)!=1)return fail();
            hologram_write(phone,phone_matched,0);
        }
    }
    return phone_finish(ctrl,phone_schedule(phone,host));
}
int __cdecl phone_dial(HologramControl* ctrl,const PhoneBindings* b,const char* number) noexcept {
    if(!phone_valid(ctrl)||!b||!b->resolve_screen||!b->keypad.dirty||!b->keypad.output)return 0;
    const auto handle=hologram_read<std::uint32_t>(ctrl->object,phone_keypad);
    if(handle==phone_invalid)return 1;
    ctrl->busy=1;const auto host=*b;HologramControl* screen=nullptr;
    return phone_finish(ctrl,phone_resolve(host,handle,screen)&&prop_key_valid(screen)&&
        prop_key_set(screen,&host.keypad,number)==1);
}
int __cdecl phone_ack(HologramControl* ctrl) noexcept {
    if(!ctrl||ctrl->busy||!ctrl->blocked||ctrl->bytes<0x4b4||!hologram_writable(ctrl->object,0x4b4))return 0;
    ctrl->blocked=0;return 1;
}
const PhoneServerApi phone_behavior{version,sizeof(PhoneServerApi),&phone_defaults,&phone_enable,&phone_think,&phone_dial,&phone_ack};

constexpr unsigned disp_loaded=0x498,disp_antigen=0x49c,disp_last=0x4a0,disp_fraction=0x4a4,
    disp_reject=0x4a8,disp_user=0x4ac,disp_steam=0x4b0,disp_screen=0x4b4;
constexpr std::uint32_t disp_invalid=0xffffffffu;
bool disp_valid(const HologramControl* ctrl) noexcept {
    return ctrl&&!ctrl->busy&&!ctrl->blocked&&ctrl->bytes>=0x4e8&&hologram_writable(ctrl->object,0x4e8);
}
int disp_finish(HologramControl* ctrl,bool ok) noexcept {
    ctrl->busy=0;if(!ok){ctrl->blocked=1;return -1;}return 1;
}
int __cdecl disp_defaults(void* object,unsigned bytes) noexcept {
    if(bytes<0x4e8||!hologram_writable(object,0x4e8))return 0;
    hologram_write(object,disp_loaded,static_cast<unsigned char>(1));
    hologram_write(object,disp_antigen,100.0f);
    for(unsigned off:{disp_last,disp_fraction,disp_reject})hologram_write(object,off,0.0f);
    for(unsigned off:{disp_user,disp_steam,disp_screen})hologram_write(object,off,disp_invalid);
    return 1;
}
bool disp_clock(const DispenserBindings& b,float& now) noexcept {
    return b.clock(b.context,&now)&&std::isfinite(now);
}
bool disp_pose(void* object,const DispenserBindings& b) noexcept {
    const float amount=hologram_read<float>(object,disp_antigen);
    if(!std::isfinite(amount))return false;
    const float fraction=static_cast<float>(static_cast<double>(amount)*0.01f);
    const float value=fraction>1?1.0f:fraction<0?0.0f:fraction;
    return b.pose(b.context,object,"percentFull",value)&&b.advance_animation(b.context,object);
}
bool disp_start(void* object,const DispenserBindings& b) noexcept {
    if(!b.sound(b.context,object,DispenserSound::UseLoop))return false;
    auto handle=hologram_read<std::uint32_t>(object,disp_steam);
    void* steam=nullptr;
    if(handle!=disp_invalid&&!b.resolve(b.context,handle,&steam))return false;
    if(steam)return true;
    handle=disp_invalid;
    if(!b.create_steam(b.context,object,&handle))return false;
    hologram_write(object,disp_steam,handle);return true;
}
bool disp_stop(void* object,const DispenserBindings& b) noexcept {
    auto handle=hologram_read<std::uint32_t>(object,disp_steam);
    if(handle==disp_invalid)return true;
    void* steam=nullptr;
    if(!b.resolve(b.context,handle,&steam))return false;
    if(!steam)return true; // original leaves a stale handle unchanged
    // Original resolves twice; never pass a stale object to removal.
    handle=hologram_read<std::uint32_t>(object,disp_steam);
    steam=nullptr;
    if(handle!=disp_invalid&&!b.resolve(b.context,handle,&steam))return false;
    if(!b.remove(b.context,steam))return false;
    hologram_write(object,disp_steam,disp_invalid);
    return b.sound(b.context,object,DispenserSound::StopLoop)&&b.sound(b.context,object,DispenserSound::Stop);
}
bool disp_use_ready(const DispenserBindings* b) noexcept {
    return b&&b->observe_use&&b->clock&&b->resolve&&b->sound&&b->create_steam&&b->install_use_think&&b->schedule;
}
int disp_use_body(HologramControl* ctrl,const DispenserBindings& host,void* activator) noexcept {
    auto* object=ctrl->object;
    if(!hologram_read<unsigned char>(object,disp_loaded)||!activator)return disp_finish(ctrl,true);
    DispenserUseObservation o{};
    if(!host.observe_use(host.context,object,activator,&o)||o.is_player>1||o.within_use_predicate>1)
        return disp_finish(ctrl,false);
    if(!o.is_player||o.health>=o.max_health||!o.within_use_predicate)return disp_finish(ctrl,true);
    const float amount=hologram_read<float>(object,disp_antigen);
    if(!std::isfinite(amount))return disp_finish(ctrl,false);
    if(amount<=0)return disp_finish(ctrl,host.sound(host.context,object,DispenserSound::Empty)!=0);
    float now=0;
    if(o.handle==disp_invalid||!disp_clock(host,now))return disp_finish(ctrl,false);
    hologram_write(object,disp_last,now);hologram_write(object,disp_user,o.handle);
    if(!disp_start(object,host)||!host.install_use_think(host.context,object)||!disp_clock(host,now))
        return disp_finish(ctrl,false);
    return disp_finish(ctrl,host.schedule(host.context,object,now)!=0);
}
int __cdecl disp_use(HologramControl* ctrl,const DispenserBindings* b,void* activator) noexcept {
    if(!disp_valid(ctrl)||!disp_use_ready(b))return 0;
    ctrl->busy=1;const auto host=*b;return disp_use_body(ctrl,host,activator);
}
int __cdecl disp_think(HologramControl* ctrl,const DispenserBindings* b) noexcept {
    if(!disp_valid(ctrl)||!b||!b->resolve||!b->position||!b->clock||!b->heal||!b->remove||
       !b->sound||!b->schedule||!b->pose||!b->advance_animation)return 0;
    ctrl->busy=1;const auto host=*b;auto* object=ctrl->object;
    const auto finish=[&](bool ok){return disp_finish(ctrl,ok);};
    const auto stop=[&](){
        if(!disp_stop(object,host))return finish(false);
        hologram_write(object,disp_last,0.0f);hologram_write(object,disp_user,disp_invalid);
        return finish(disp_pose(object,host));
    };
    auto handle=hologram_read<std::uint32_t>(object,disp_user);
    if(handle==disp_invalid)return stop();
    void* user=nullptr;
    if(!host.resolve(host.context,handle,&user))return finish(false);
    if(!user)return stop();
    float origin[3]{},position[3]{};
    if(!host.position(host.context,object,origin))return finish(false);
    handle=hologram_read<std::uint32_t>(object,disp_user);user=nullptr;
    if(handle==disp_invalid)return stop();
    if(!host.resolve(host.context,handle,&user))return finish(false);
    if(!user)return stop();
    if(!host.position(host.context,user,position))return finish(false);
    double d2=0;
    for(unsigned i=0;i<3;++i){
        const float delta=position[i]-origin[i];
        d2+=static_cast<double>(delta)*delta;
    }
    if(!(d2<6400.0))return stop();
    float now=0;
    const auto last=hologram_read<float>(object,disp_last);
    const auto amount=hologram_read<float>(object,disp_antigen);
    const auto fraction=hologram_read<float>(object,disp_fraction);
    if(!disp_clock(host,now)||!std::isfinite(last)||!std::isfinite(amount)||!std::isfinite(fraction))
        return finish(false);
    // Original caps positive elapsed time at .1f; it does not clamp
    // negative elapsed time. Subtract dose even if healing is refused.
    double elapsed=static_cast<double>(now)-last;
    if(elapsed>0.1f)elapsed=0.1f;
    const double dose=elapsed*7.0;
    const double remaining=static_cast<double>(amount)-dose;
    const double accumulated=static_cast<double>(fraction)+dose;
    constexpr auto max_float=(std::numeric_limits<float>::max)();
    if(std::abs(remaining)>max_float||std::abs(accumulated)>max_float)return finish(false);
    hologram_write(object,disp_last,now);
    hologram_write(object,disp_antigen,remaining<0?0.0f:static_cast<float>(remaining));
    const auto stored=static_cast<float>(accumulated);
    hologram_write(object,disp_fraction,stored);
    if(accumulated>=1.0){
        const float whole=static_cast<float>(std::floor(accumulated));
        if(static_cast<double>(whole)>(std::numeric_limits<int>::max)())return finish(false);
        hologram_write(object,disp_fraction,static_cast<float>(static_cast<double>(stored)-whole));
        if(hologram_read<float>(object,disp_antigen)<=0)return stop();
        handle=hologram_read<std::uint32_t>(object,disp_user);user=nullptr;
        if(handle==disp_invalid)return stop();
        if(!host.resolve(host.context,handle,&user))return finish(false);
        if(!user)return stop();
        int accepted=0;
        if(!host.heal(host.context,user,static_cast<int>(whole),0x00400000u,&accepted))return finish(false);
        if(!accepted)return stop();
    }
    // A fractional final dose with zero remaining still schedules until
    // the next integer threshold, matching the original branch.
    if(!disp_clock(host,now))return finish(false);
    const float when=static_cast<float>(static_cast<double>(now)+0.1f);
    if(!std::isfinite(when)||!host.schedule(host.context,object,when))return finish(false);
    return finish(disp_pose(object,host));
}
int __cdecl disp_stop_api(HologramControl* ctrl,const DispenserBindings* b) noexcept {
    if(!disp_valid(ctrl)||!b||!b->resolve||!b->remove||!b->sound)return 0;
    ctrl->busy=1;return disp_finish(ctrl,disp_stop(ctrl->object,*b));
}
int __cdecl disp_pose_api(HologramControl* ctrl,const DispenserBindings* b) noexcept {
    if(!disp_valid(ctrl)||!b||!b->pose||!b->advance_animation)return 0;
    ctrl->busy=1;return disp_finish(ctrl,disp_pose(ctrl->object,*b));
}
int __cdecl disp_ack(HologramControl* ctrl) noexcept {
    if(!ctrl||ctrl->busy||!ctrl->blocked||ctrl->bytes<0x4e8||!hologram_writable(ctrl->object,0x4e8))return 0;
    ctrl->blocked=0;return 1;
}
const DispenserApi dispenser_api{version,sizeof(DispenserApi),&disp_defaults,&disp_use,&disp_think,
    &disp_stop_api,&disp_pose_api,&disp_ack};

int __cdecl container_spawn_state(HologramControl* ctrl,const ContainerBindings* b) noexcept {
    if(!disp_valid(ctrl)||!b||!b->bodygroup)return 0;
    const float amount=hologram_read<float>(ctrl->object,disp_antigen);
    if(!std::isfinite(amount))return 0;
    ctrl->busy=1;
    const float clamped=amount>100?100.0f:amount<0?0.0f:amount;
    const auto loaded=static_cast<unsigned char>(clamped>0);
    hologram_write(ctrl->object,disp_antigen,clamped);
    hologram_write(ctrl->object,disp_loaded,loaded);
    return disp_finish(ctrl,b->bodygroup(b->dispenser.context,ctrl->object,1,loaded)!=0);
}
int __cdecl container_touch(HologramControl* ctrl,const ContainerBindings* b,void* entity) noexcept {
    if(!disp_valid(ctrl)||!b||!b->cast_container||!b->bodygroup||!b->notice||!b->sound||
       !b->output||!b->dispenser.clock||!b->dispenser.pose||
       !b->dispenser.advance_animation||!b->dispenser.remove)return 0;
    if(!entity)return 1;
    ctrl->busy=1;const auto host=*b;const auto& d=host.dispenser;auto* object=ctrl->object;
    ContainerObservation o{};
    if(!host.cast_container(d.context,entity,&o))return disp_finish(ctrl,false);
    if(!o.container||!(o.flags&8u)||hologram_read<unsigned char>(object,disp_loaded))
        return disp_finish(ctrl,true);
    float now=0;
    const float reject=hologram_read<float>(object,disp_reject);
    if(!disp_clock(d,now)||!std::isfinite(reject)||!std::isfinite(o.amount))return disp_finish(ctrl,false);
    // Original rejected touch refreshes the rejection timestamp.
    // A continuous contact therefore cannot expire this window.
    if(!(static_cast<double>(reject)+0.5<now)){
        hologram_write(object,disp_reject,now);return disp_finish(ctrl,true);
    }
    // Preserve both original multiplications, not a direct copy.
    const auto amount=static_cast<float>(static_cast<double>(o.amount)*100.0f*0.01f);
    hologram_write(object,disp_antigen,amount);
    if(!host.bodygroup(d.context,object,1,1)||!disp_pose(object,d))return disp_finish(ctrl,false);
    // Original marks loaded after pose, even if inserted amount is zero.
    hologram_write(object,disp_loaded,static_cast<unsigned char>(1));
    if(!host.notice(d.context,object,o.container,ContainerNotice::Inserted)||
       !d.remove(d.context,o.container)||!host.sound(d.context,object,ContainerSound::Load))
        return disp_finish(ctrl,false);
    return disp_finish(ctrl,host.output(d.context,object,0x4b8,o.container,object,0.0f)!=0);
}
int __cdecl container_eject(HologramControl* ctrl,const ContainerBindings* b) noexcept {
    if(!disp_valid(ctrl)||!b||!b->create_ejected||!b->container_amount||!b->notice||!b->bodygroup||
       !b->eject_message||!b->sound||!b->clear_think||!b->output||
       !b->dispenser.clock||!b->dispenser.resolve||!b->dispenser.remove||!b->dispenser.sound)return 0;
    auto* object=ctrl->object;
    if(!hologram_read<unsigned char>(object,disp_loaded))return 1;
    ctrl->busy=1;const auto host=*b;const auto& d=host.dispenser;
    void* container=nullptr;
    if(!host.create_ejected(d.context,object,"prop_antigen_explosive_barrel",&container)||!container)
        return disp_finish(ctrl,false);
    const float original=hologram_read<float>(object,disp_antigen);
    if(!std::isfinite(original))return disp_finish(ctrl,false);
    const float amount=static_cast<float>(static_cast<double>(original)*100.0f*0.01f);
    const float pose=static_cast<float>(static_cast<double>(amount)*0.01f);
    if(!host.container_amount(d.context,container,amount,pose)||
       !host.notice(d.context,object,container,ContainerNotice::Ejected))return disp_finish(ctrl,false);
    float now=0;
    if(!disp_clock(d,now))return disp_finish(ctrl,false);
    hologram_write(object,disp_last,now);
    hologram_write(object,disp_loaded,static_cast<unsigned char>(0));
    hologram_write(object,disp_antigen,0.0f);
    if(!disp_stop(object,d)||!host.eject_message(d.context,object,1)||
       !host.bodygroup(d.context,object,1,0)||!host.sound(d.context,object,ContainerSound::Eject)||
       !host.clear_think(d.context,object)||!host.output(d.context,object,0x4d0,container,object,0.0f)||
       !disp_clock(d,now))return disp_finish(ctrl,false);
    hologram_write(object,disp_reject,now);
    // Original does not reset user handle or fractional health here.
    return disp_finish(ctrl,true);
}
const ContainerApi container_api{version,sizeof(ContainerApi),&container_spawn_state,&container_touch,&container_eject};

constexpr unsigned dna_raw=0x494,dna_caps=0x498,dna_deadline=0x49c,dna_output=0x4a0;
bool dna_valid(const HologramControl* ctrl) noexcept {
    return ctrl&&!ctrl->busy&&!ctrl->blocked&&ctrl->bytes>=0x4b8&&hologram_writable(ctrl->object,0x4b8);
}
int __cdecl dna_spawn_state(HologramControl* ctrl) noexcept {
    if(!dna_valid(ctrl))return 0;
    hologram_write(ctrl->object,dna_raw,0u);hologram_write(ctrl->object,dna_caps,0x20u);
    return 1;
}
int __cdecl dna_capabilities(const void* object,unsigned bytes,std::uint32_t base,std::uint32_t* out) noexcept {
    if(!out||bytes<0x4b8||!sin_independent::vgui_layout::Readable(object,0x4b8))return 0;
    *out=base|hologram_read<std::uint32_t>(object,dna_caps);return 1;
}
bool dna_deny(void* scanner,const DnaScannerBindings& b) noexcept {
    float now=0;
    if(!b.clock(b.context,&now)||!std::isfinite(now))return false;
    // FCOM + TEST AH,41 / JP: ordered less-than OR equal.
    // NaN deadline skips; do not turn equality into a strict gate.
    if(!(hologram_read<float>(scanner,dna_deadline)<=now))return true;
    const float deadline=static_cast<float>(static_cast<double>(now)+0.62);
    if(!std::isfinite(deadline))return false;
    hologram_write(scanner,dna_deadline,deadline);
    return b.sound(b.context,scanner,"SuitRecharge.Deny")!=0;
}
int __cdecl dna_touch(HologramControl* ctrl,const DnaScannerBindings* b,void* other) noexcept {
    if(!dna_valid(ctrl)||!b||!b->classname||!b->clock||!b->sound||!b->output)return 0;
    if(!other)return 1;
    ctrl->busy=1;const auto host=*b;const char* name=nullptr;
    if(!host.classname(host.context,other,&name))return disp_finish(ctrl,false);
    // Original reads entity classname and compares exact literals.
    // Host supplies stable NUL-terminated classname for the call.
    if(!name)return disp_finish(ctrl,true);
    char text[128]{};
    if(!prop_key_read(name,text))return disp_finish(ctrl,false);
    if(!std::strcmp(text,"prop_ragdoll"))
        return disp_finish(ctrl,host.output(host.context,ctrl->object,dna_output,ctrl->object,ctrl->object,0.0f)!=0);
    if(!std::strcmp(text,"player"))return disp_finish(ctrl,dna_deny(ctrl->object,host));
    return disp_finish(ctrl,true);
}
int __cdecl dna_use(HologramControl* ctrl,const DnaScannerBindings* b,void* activator) noexcept {
    if(!dna_valid(ctrl)||!b||!b->is_player||!b->clock||!b->sound)return 0;
    if(!activator)return 1;
    ctrl->busy=1;const auto host=*b;unsigned player=0;
    if(!host.is_player(host.context,activator,&player)||player>1)return disp_finish(ctrl,false);
    return disp_finish(ctrl,!player||dna_deny(ctrl->object,host));
}
int __cdecl dna_ack(HologramControl* ctrl) noexcept {
    if(!ctrl||ctrl->busy||!ctrl->blocked||ctrl->bytes<0x4b8||!hologram_writable(ctrl->object,0x4b8))return 0;
    ctrl->blocked=0;return 1;
}
const DnaScannerApi dna_api{version,sizeof(DnaScannerApi),&dna_spawn_state,&dna_capabilities,
    &dna_touch,&dna_use,&dna_ack};

constexpr unsigned health_max=0x98,health_over=0x12a1;
bool health_control(const HologramControl* ctrl,unsigned bytes) noexcept {
    return ctrl&&!ctrl->busy&&!ctrl->blocked&&ctrl->bytes>=bytes&&hologram_writable(ctrl->object,bytes);
}
bool health_set_max(void* player,const HealthPickupBindings& host,int maximum) noexcept {
    if(hologram_read<int>(player,health_max)!=maximum){
        if(!host.dirty_player(host.context,player))return false;
        hologram_write(player,health_max,maximum);
    }
    // Preserve the original TWO writes and notifications for >200,
    // rather than clamping before the first tracked setter.
    if(hologram_read<int>(player,health_max)>200){
        if(!host.dirty_player(host.context,player))return false;
        hologram_write(player,health_max,200);
    }
    hologram_write(player,health_over,static_cast<unsigned char>(hologram_read<int>(player,health_max)>100));
    return true;
}
int __cdecl health_set_api(HologramControl* player,const HealthPickupBindings* host,int maximum) noexcept {
    if(!health_control(player,0x12a4)||!host||!host->dirty_player)return 0;
    player->busy=1;
    return disp_finish(player,health_set_max(player->object,*host,maximum));
}

bool heal_bindings(const PlayerHealBindings* b) noexcept {
    return b&&b->dirty_max&&b->dirty_health&&b->is_player&&b->health_event&&b->fade;
}
int health_int_sum(int a,int b) noexcept {
    const auto raw=static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b);
    int result=0;std::memcpy(&result,&raw,4);return result;
}
int health_int_delta(int a,int b) noexcept {
    const auto raw=static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b);
    int result=0;std::memcpy(&result,&raw,4);return result;
}
bool tracked_health(void* player,const PlayerHealBindings& b,int value) noexcept {
    if(hologram_read<int>(player,0x9c)==value)return true;
    if(!b.dirty_health(b.context,player))return false;
    hologram_write(player,0x9c,value);return true;
}
bool native_take_health(void* player,const PlayerHealBindings& b,float amount,unsigned flags,int& accepted) noexcept {
    accepted=0;
    const double trunc=std::trunc(static_cast<double>(amount));
    if(!std::isfinite(amount)||trunc<(std::numeric_limits<int>::min)()||trunc>(std::numeric_limits<int>::max)())return false;
    // CBasePlayer::TakeHealth 22246860 clears the eligible damage bits
    // even if the later base health addition refuses the request.
    if(hologram_read<int>(player,0x94)!=0)
        hologram_write(player,0x9d0,hologram_read<std::uint32_t>(player,0x9d0)&~(flags&0xffc07fffu));
    if(!hologram_read<unsigned char>(player,health_over)&&hologram_read<int>(player,health_max)>100){
        HealthPickupBindings max_binding{};max_binding.context=b.context;max_binding.dirty_player=b.dirty_max;
        if(!health_set_max(player,max_binding,100))return false;
    }
    // Combat-character gate followed by CBaseEntity::TakeHealth.
    if(!hologram_read<std::uint32_t>(player,0x16c)||hologram_read<int>(player,0x94)<2)return true;
    const int previous=hologram_read<int>(player,0x9c);
    if(previous>=hologram_read<int>(player,health_max))return true;
    if(!tracked_health(player,b,health_int_sum(previous,static_cast<int>(trunc))))return false;
    unsigned is_player=0;
    if(!b.is_player(b.context,player,&is_player)||is_player>1)return false;
    if(is_player){
        const int current=hologram_read<int>(player,0x9c);
        if(!b.health_event(b.context,player,current,health_int_delta(current,previous),flags))return false;
    }
    const int maximum=hologram_read<int>(player,health_max);
    if(hologram_read<int>(player,0x9c)>maximum&&!tracked_health(player,b,maximum))return false;
    if(health_int_delta(hologram_read<int>(player,0x9c),previous)==0)return true;
    constexpr unsigned char rgba[]={0,0,128,64};
    if(!b.fade(b.context,player,rgba,1.0f,0.5f,1))return false;
    if(hologram_read<int>(player,0x9c)>100)
        hologram_write(player,health_over,static_cast<unsigned char>(0));
    accepted=1;return true;
}
int __cdecl player_take(HologramControl* player,const PlayerHealBindings* b,float amount,unsigned flags,int* accepted) noexcept {
    const double trunc=std::trunc(static_cast<double>(amount));
    if(!health_control(player,0x12a4)||!heal_bindings(b)||!accepted||!std::isfinite(amount)||
       trunc<(std::numeric_limits<int>::min)()||trunc>(std::numeric_limits<int>::max)())return 0;
    player->busy=1;const auto host=*b;int result=0;
    const bool ok=native_take_health(player->object,host,amount,flags,result);
    *accepted=result;return disp_finish(player,ok);
}
int health_pickup_impl(HologramControl* item,HologramControl* player,const HealthPickupBindings* b,
        const PlayerHealBindings* native,HealthPickupKind kind,unsigned* accepted) noexcept {
    if(!health_control(item,0x4cc)||!health_control(player,0x12a4)||item==player||item->object==player->object||
        !b||!b->skill_amount||!b->dirty_player||(!native&&!b->heal)||(native&&!heal_bindings(native))||!b->pickup_message||!b->pickup_sound||
        !b->should_respawn||!b->respawn||!b->remove||!accepted||static_cast<unsigned>(kind)>2)return 0;
    item->busy=player->busy=1;
    const auto host=*b;
    const auto finish=[&](bool ok,unsigned consumed){
        item->busy=player->busy=0;*accepted=consumed;
        if(!ok){item->blocked=player->blocked=1;return -1;}
        return 1;
    };
    float amount=0;
    if(!host.skill_amount(host.context,kind,&amount)||!std::isfinite(amount))return finish(false,0);
    if(kind==HealthPickupKind::Mega){
        // Original FISTP with truncation control, then DWORD addition.
        const double truncated=std::trunc(static_cast<double>(amount));
        if(truncated<(std::numeric_limits<int>::min)()||truncated>(std::numeric_limits<int>::max)())
            return finish(false,0);
        const auto increase=static_cast<int>(truncated);
        const auto sum=static_cast<std::uint32_t>(hologram_read<int>(player->object,health_max))+
            static_cast<std::uint32_t>(increase);
        int maximum=0;std::memcpy(&maximum,&sum,4);
        if(!health_set_max(player->object,host,maximum))return finish(false,0);
        amount=static_cast<float>(increase);
    }
    int healed=0;
    if(native){const auto services=*native;if(!native_take_health(player->object,services,amount,0,healed))return finish(false,0);}
    else if(!host.heal(host.context,player->object,amount,0,&healed))return finish(false,0);
    // Mega's max change is NOT rolled back if heal returns zero.
    if(!healed)return finish(true,0);
    const char* classname=hologram_read<const char*>(item->object,0xc);
    char name[128]{};
    if(classname&&!prop_key_read(classname,name))return finish(false,1);
    if(!host.pickup_message(host.context,player->object,"ItemPickup",name))return finish(false,1);
    const char* sound=kind==HealthPickupKind::Kit?"HealthKit.Touch":"HealthVial.Touch";
    if(!host.pickup_sound(host.context,item->object,player->object,sound))return finish(false,1);
    unsigned respawn=0;
    if(!host.should_respawn(host.context,item->object,&respawn)||respawn>1)return finish(false,1);
    return finish(respawn?host.respawn(host.context,item->object)!=0:host.remove(host.context,item->object)!=0,1);
}
int __cdecl health_pickup(HologramControl* item,HologramControl* player,const HealthPickupBindings* b,
        HealthPickupKind kind,unsigned* accepted) noexcept {
    return health_pickup_impl(item,player,b,nullptr,kind,accepted);
}
int __cdecl player_pickup(HologramControl* item,HologramControl* player,const HealthPickupBindings* b,
        const PlayerHealBindings* native,HealthPickupKind kind,unsigned* accepted) noexcept {
    if(!heal_bindings(native))return 0;
    return health_pickup_impl(item,player,b,native,kind,accepted);
}
const PlayerHealApi player_heal_api{version,sizeof(PlayerHealApi),&player_take,&player_pickup};
int __cdecl health_ack(HologramControl* ctrl) noexcept {
    if(!ctrl||ctrl->busy||!ctrl->blocked||ctrl->bytes<0x4cc||!hologram_writable(ctrl->object,ctrl->bytes>=0x12a4?0x12a4:0x4cc))return 0;
    ctrl->blocked=0;return 1;
}
const HealthPickupApi health_pickup_api{version,sizeof(HealthPickupApi),&health_set_api,&health_pickup,&health_ack};

bool screen_schedule(void* object,const DispenserBindings& d) noexcept {
    float now=0;if(!disp_clock(d,now))return false;
    const float when=static_cast<float>(static_cast<double>(now)+0.1f);
    return std::isfinite(when)&&d.schedule(d.context,object,when)!=0;
}
bool screen_resolve(void* object,const DispenserBindings& d,void*& screen) noexcept {
    screen=nullptr;const auto handle=hologram_read<std::uint32_t>(object,disp_screen);
    return handle==disp_invalid||d.resolve(d.context,handle,&screen)!=0;
}
bool screen_live(void* object,const DispenserBindings& d,void*& screen) noexcept {
    return screen_resolve(object,d,screen)&&screen!=nullptr;
}
bool screen_flags(void* object,const DispenserScreenBindings& b,bool set_high) noexcept {
    void* screen=nullptr;unsigned bytes=0;const auto& d=b.dispenser;
    if(!screen_live(object,d,screen)||!b.screen_storage(d.context,screen,&bytes)||
       bytes<0x3b8||!hologram_writable(screen,0x3b8))return false;
    // Original raw flag writes, distinct from m_fScreenFlags at +3b0.
    if(set_high)hologram_write(screen,0x3b7,static_cast<unsigned char>(hologram_read<unsigned char>(screen,0x3b7)|0x80u));
    else hologram_write(screen,0x3b4,hologram_read<std::uint32_t>(screen,0x3b4)&~2u);
    return true;
}
bool screen_build(void* object,const DispenserScreenBindings& b) noexcept {
    const auto& d=b.dispenser;void* screen=nullptr;
    if(!screen_resolve(object,d,screen))return false;
    if(screen)return true; // existing screen is not reconfigured
    auto handle=disp_invalid;
    if(!b.create_screen(d.context,"vgui_screen",&handle))return false;
    hologram_write(object,disp_screen,handle);
    if(handle==disp_invalid)return true; // original still updates pose and schedules
    float origin[3]{},angles[3]{},basis[3]{},forward[3]{},right[3]{},up[3]{},position[3]{};
    if(!b.sample_origin(d.context,object,origin)||!b.sample_angles(d.context,object,angles,basis)||
       !b.angle_vectors(d.context,basis,forward,right,up)||!b.normalize(d.context,forward))return false;
    for(unsigned i=0;i<3;++i){
        if(!std::isfinite(origin[i])||!std::isfinite(angles[i])||!std::isfinite(basis[i])||!std::isfinite(forward[i])||
           !std::isfinite(right[i])||!std::isfinite(up[i]))return false;
        // Match observed rounded product and intermediate stores:
        // forward * -4, right * 16.5, up * 3.6.
        const float f=static_cast<float>(static_cast<double>(forward[i])*-4.0);
        const float r=static_cast<float>(static_cast<double>(right[i])*16.5);
        const float u=static_cast<float>(static_cast<double>(up[i])*3.6f);
        double partial=static_cast<double>(origin[i])+f;
        if(i==2)partial=static_cast<float>(partial);
        partial+=r;if(i!=1)partial=static_cast<float>(partial);
        position[i]=static_cast<float>(partial+u);
        if(!std::isfinite(position[i]))return false;
    }
    angles[1]=static_cast<float>(static_cast<double>(angles[1])+90);
    angles[2]=static_cast<float>(static_cast<double>(angles[2])+90);
    if(!std::isfinite(angles[1])||!std::isfinite(angles[2]))return false;
    return screen_live(object,d,screen)&&b.set_origin(d.context,screen,position)&&
        screen_live(object,d,screen)&&b.set_angles(d.context,screen,angles)&&
        screen_live(object,d,screen)&&b.set_size(d.context,screen,7.1f,2.0f)&&
        screen_live(object,d,screen)&&b.keyvalue(d.context,screen,"panelname","vgui_dispenser_panel")&&
        screen_flags(object,b,true)&&screen_flags(object,b,false)&&
        screen_live(object,d,screen)&&b.spawn_screen(d.context,screen);
}
int __cdecl screen_idle(HologramControl* ctrl,const DispenserScreenBindings* b) noexcept {
    if(!disp_valid(ctrl)||!b)return 0;
    const auto& d=b->dispenser;
    if(!d.resolve||!d.clock||!d.schedule||!d.pose||!d.advance_animation||!b->create_screen||
       !b->sample_origin||!b->sample_angles||!b->angle_vectors||!b->normalize||!b->set_origin||
       !b->set_angles||!b->set_size||!b->keyvalue||!b->screen_storage||!b->spawn_screen)return 0;
    ctrl->busy=1;const auto host=*b;auto* object=ctrl->object;
    return disp_finish(ctrl,screen_build(object,host)&&disp_pose(object,host.dispenser)&&
        screen_schedule(object,host.dispenser));
}
int __cdecl screen_restore(HologramControl* ctrl,const DispenserScreenBindings* b) noexcept {
    if(!disp_valid(ctrl)||!b)return 0;
    const auto& d=b->dispenser;
    if(!d.resolve||!d.clock||!d.schedule||!d.pose||!d.advance_animation||!d.sound||!d.create_steam||
       !d.remove||!b->base_restore||!b->listen||!b->think_kind||!b->next_think||!b->install_idle)return 0;
    ctrl->busy=1;const auto host=*b;const auto& hd=host.dispenser;auto* object=ctrl->object;
    const auto fail=[&](){return disp_finish(ctrl,false);};
    if(!host.base_restore(hd.context,object)||!host.listen(hd.context,object,"button_press")||
       !disp_pose(object,hd))return fail();
    DispenserThinkKind kind=DispenserThinkKind::Other;
    if(!host.think_kind(hd.context,object,&kind)||static_cast<unsigned>(kind)>2)return fail();
    bool start=false;float next=0,now=0;
    if(kind==DispenserThinkKind::Use){
        if(!host.next_think(hd.context,object,&next)||!disp_clock(hd,now))return fail();
        start=next>=now;
    }
    if(!(start?disp_start(object,hd):disp_stop(object,hd)))return fail();
    if(!host.next_think(hd.context,object,&next)||!disp_clock(hd,now))return fail();
    if(next<now&&(!host.install_idle(hd.context,object)||!screen_schedule(object,hd)))return fail();
    return disp_finish(ctrl,true);
}
const DispenserScreenApi dispenser_screen_api{version,sizeof(DispenserScreenApi),&screen_idle,&screen_restore};

constexpr unsigned can_amount=0x628,can_flags=0x62c,can_bytes=0x6a0;
constexpr char can_model[]="models/items/antigen/antigentank.mdl";
bool can_valid(const HologramControl* ctrl) noexcept {
    return health_control(ctrl,can_bytes);
}
int __cdecl can_defaults(void* object,unsigned bytes) noexcept {
    if(bytes<can_bytes||!hologram_writable(object,can_bytes))return 0;
    // Original CAntigenCanister constructor after base CPropContainer
    // initialization; leave output objects and vtables host-owned.
    hologram_write(object,can_amount,100.0f);
    hologram_write(object,can_flags,9u);
    hologram_write(object,0x630,static_cast<const char*>(can_model));
    hologram_write(object,0x634,0.0f);hologram_write(object,0x638,1u);
    for(unsigned off:{0x63cu,0x640u,0x644u,0x648u,0x64cu})hologram_write(object,off,0u);
    hologram_write(object,0x668,disp_invalid);hologram_write(object,0x69c,disp_invalid);
    return 1;
}
int __cdecl can_describe(void* object,unsigned bytes,ContainerObservation* out) noexcept {
    if(!out||bytes<can_bytes||!sin_independent::vgui_layout::Readable(object,can_bytes))return 0;
    *out={object,hologram_read<std::uint32_t>(object,can_flags),hologram_read<float>(object,can_amount)};
    return 1;
}
bool can_pose(void* object,const CanisterBindings& b) noexcept {
    const float amount=hologram_read<float>(object,can_amount);
    if(!std::isfinite(amount))return false;
    return b.pose(b.context,object,"percentFull",static_cast<float>(static_cast<double>(amount)*0.01f))!=0;
}
int __cdecl can_set(HologramControl* ctrl,const CanisterBindings* b,float amount) noexcept {
    if(!can_valid(ctrl)||!b||!b->pose||!std::isfinite(amount))return 0;
    ctrl->busy=1;
    hologram_write(ctrl->object,can_amount,amount);
    return disp_finish(ctrl,can_pose(ctrl->object,*b));
}
bool can_schedule(void* object,const CanisterBindings& b,double delay) noexcept {
    float now=0;
    if(!b.clock(b.context,&now)||!std::isfinite(now))return false;
    const float when=static_cast<float>(static_cast<double>(now)+delay);
    return std::isfinite(when)&&b.schedule(b.context,object,when)!=0;
}
int __cdecl can_spawn(HologramControl* ctrl,const CanisterBindings* b) noexcept {
    if(!can_valid(ctrl)||!b||!b->base_spawn||!b->pose||!b->install_idle||!b->clock||!b->schedule)return 0;
    ctrl->busy=1;const auto host=*b;auto* object=ctrl->object;
    return disp_finish(ctrl,host.base_spawn(host.context,object)&&can_pose(object,host)&&
        host.install_idle(host.context,object)&&can_schedule(object,host,0.25));
}
int __cdecl can_idle(HologramControl* ctrl,const CanisterBindings* b) noexcept {
    if(!can_valid(ctrl)||!b||!b->advance_animation||!b->random_float||!b->clock||!b->schedule)return 0;
    ctrl->busy=1;const auto host=*b;float sample=0;
    if(!host.advance_animation(host.context,ctrl->object)||!host.random_float(host.context,0.5f,1.5f,&sample)||
        !std::isfinite(sample)||sample<0.5f||sample>1.5f)return disp_finish(ctrl,false);
    return disp_finish(ctrl,can_schedule(ctrl->object,host,static_cast<double>(sample)+1.0));
}
int __cdecl can_caps(HologramControl* ctrl,const CanisterBindings* b,unsigned* out) noexcept {
    if(!can_valid(ctrl)||!b||!b->base_capabilities||!b->carry_eligible||!out)return 0;
    ctrl->busy=1;const auto host=*b;unsigned base=0,carry=0;
    if(!host.base_capabilities(host.context,ctrl->object,&base)||
        !host.carry_eligible(host.context,ctrl->object,&carry)||carry>1)return disp_finish(ctrl,false);
    *out=base|0x40000000u|(carry?0x10u:0u);return disp_finish(ctrl,true);
}
int __cdecl can_use(HologramControl* ctrl,const CanisterBindings* b,void* activator,unsigned type,const void* payload) noexcept {
    if(!can_valid(ctrl)||!b||!b->is_player||!b->carry)return 0;
    if(!activator)return 1;
    ctrl->busy=1;const auto host=*b;unsigned player=0;
    if(!host.is_player(host.context,activator,&player)||player>1)return disp_finish(ctrl,false);
    if(!player||type!=3)return disp_finish(ctrl,true);
    return disp_finish(ctrl,payload&&host.carry(host.context,activator,ctrl->object,payload));
}
int __cdecl can_break(HologramControl* ctrl,const CanisterBindings* b) noexcept {
    if(!can_valid(ctrl)||!b||!b->base_break||!b->fragment)return 0;
    ctrl->busy=1;const auto host=*b;
    if(!host.base_break(host.context,ctrl->object))return disp_finish(ctrl,false);
    constexpr const char* models[]={"models/items/antigen/antigen_break01.mdl",
        "models/items/antigen/antigen_break02.mdl","models/items/antigen/antigen_break03.mdl",
        "models/items/antigen/antigen_break04.mdl"};
    for(auto name:models)
        if(!host.fragment(host.context,ctrl->object,1,name,200.0f,-200.0f,25.0f))return disp_finish(ctrl,false);
    return disp_finish(ctrl,true);
}
int __cdecl can_ack(HologramControl* ctrl) noexcept {
    if(!ctrl||ctrl->busy||!ctrl->blocked||ctrl->bytes<can_bytes||!hologram_writable(ctrl->object,can_bytes))return 0;
    ctrl->blocked=0;return 1;
}
const CanisterApi canister_api{version,sizeof(CanisterApi),&can_defaults,&can_describe,&can_set,&can_spawn,
    &can_idle,&can_caps,&can_use,&can_break,&can_ack};

int __cdecl disp_use_variant(HologramControl* ctrl,const DispenserInputBindings* b,void* activator,unsigned type) noexcept {
    if(!disp_valid(ctrl)||!b)return 0;
    if(type!=0&&type!=1)return 1;
    if(!b->use_event||(type==1&&!disp_use_ready(&b->dispenser)))return 0;
    ctrl->busy=1;const auto host=*b;const auto& d=host.dispenser;
    if(!host.use_event(d.context,activator,ctrl->object,type==1?2u:3u))return disp_finish(ctrl,false);
    if(type==1)return disp_use_body(ctrl,d,activator);
    // Original release only invalidates user. Next use think owns
    // stopping steam, clearing time and updating the fill pose.
    hologram_write(ctrl->object,disp_user,disp_invalid);
    return disp_finish(ctrl,true);
}
int __cdecl disp_button_event(HologramControl* ctrl,const DispenserInputBindings* b,const char* name,void* event) noexcept {
    if(!disp_valid(ctrl)||!b||!event||!b->dispenser.resolve||!b->dispenser.clock||
       !b->dispenser.schedule||!b->screen_index||!b->event_index||!b->event_command||!b->install_eject)return 0;
    char event_name[64]{};
    if(!prop_key_read(name,event_name))return 0;
    if(ascii_class_compare(event_name,"button_press"))return 1;
    ctrl->busy=1;const auto host=*b;const auto& d=host.dispenser;
    auto* object=ctrl->object;
    void* screen=nullptr;
    if(!screen_resolve(object,d,screen))return disp_finish(ctrl,false);
    if(!screen)return disp_finish(ctrl,true);
    // Original re-resolves screen handle before the index query.
    if(!screen_live(object,d,screen))return disp_finish(ctrl,false);
    int index=0,received=0;
    if(!host.screen_index(d.context,screen,&index)||
       !host.event_index(d.context,event,"entindex",0,&received))return disp_finish(ctrl,false);
    if(index!=received)return disp_finish(ctrl,true);
    const char* command=nullptr;char text[128]{};
    if(!host.event_command(d.context,event,"command","",&command)||!prop_key_read(command,text))
        return disp_finish(ctrl,false);
    if(ascii_class_compare(text,"eject"))return disp_finish(ctrl,true);
    if(!host.install_eject(d.context,object))return disp_finish(ctrl,false);
    float now=0;
    if(!disp_clock(d,now))return disp_finish(ctrl,false);
    return disp_finish(ctrl,d.schedule(d.context,object,now)!=0);
}
const DispenserInputApi dispenser_input_api{version,sizeof(DispenserInputApi),&disp_use_variant,&disp_button_event};

int __cdecl prop_container_defaults(HologramControl* ctrl,const PropContainerBindings* b,PropContainerKind kind) noexcept {
    if(!can_valid(ctrl)||static_cast<unsigned>(kind)>6)return 0;
    if((kind==PropContainerKind::Acetylene||kind==PropContainerKind::Barrel)&&(!b||!b->random_float))return 0;
    ctrl->busy=1;
    unsigned flags=3,raw=100;int count=1;float duration=10,parameter=0;
    const char* model="models/containers/acetylene_tank.mdl";
    const char* loop=nullptr;
    switch(kind){
    case PropContainerKind::Generic:break;
    case PropContainerKind::Mutagen:
        flags=4;duration=12;count=5;parameter=1;model="models/containers/mutagen_barrel.mdl";loop="gas.poison_loop";break;
    case PropContainerKind::Antigen:
        flags=9;raw=0;duration=0;model=can_model;break;
    case PropContainerKind::OxygenLarge:
        flags=0x92;parameter=1000;model="models/containers/oxygen_tank_large.mdl";
        loop="container.leak_pressurized_loop";break;
    case PropContainerKind::OxygenSmall:
        flags=0x92;duration=20;parameter=3000;model="models/containers/oxygen_tank_small.mdl";
        loop="container.leak_pressurized_loop";break;
    case PropContainerKind::Acetylene:
    case PropContainerKind::Barrel:{
        const bool barrel=kind==PropContainerKind::Barrel;
        flags=barrel?0xe0u:0xa2u;count=0;parameter=barrel?0.0f:3000.0f;
        model=barrel?"models/containers/exploding_barrel.mdl":"models/containers/acetyline_tank.mdl";
        loop=barrel?nullptr:"container.leak_pressurized_loop";
        const float low=barrel?1.0f:3.0f,high=barrel?2.0f:4.0f;
        if(!b->random_float(b->context,low,high,&duration)||!std::isfinite(duration)||duration<low||duration>high)
            return disp_finish(ctrl,false);
        break;
    }}
    auto* object=ctrl->object;
    hologram_write(object,can_amount,100.0f);hologram_write(object,can_flags,flags);
    hologram_write(object,0x630,model);hologram_write(object,0x634,duration);
    hologram_write(object,0x638,count);hologram_write(object,0x63c,parameter);
    hologram_write(object,0x640,loop);hologram_write(object,0x644,static_cast<const char*>(nullptr));
    hologram_write(object,0x648,raw);hologram_write(object,0x64c,0.0f);
    hologram_write(object,0x668,disp_invalid);hologram_write(object,0x69c,disp_invalid);
    return disp_finish(ctrl,true);
}
bool prop_container_stop_body(void* object,const PropContainerBindings& b) noexcept {
    unsigned flags=hologram_read<unsigned>(object,can_flags);
    if(!(flags&0x100u))return true;
    hologram_write(object,can_flags,flags&~0x102u);
    const char* loop=hologram_read<const char*>(object,0x640);
    if(loop){
        char text[128]{};
        if(!prop_key_read(loop,text)||!b.stop_sound(b.context,object,text))return false;
    }
    auto handle=hologram_read<std::uint32_t>(object,0x668);
    if(handle!=disp_invalid){
        void* steam=nullptr;
        if(!b.resolve(b.context,handle,&steam))return false;
        if(steam){
            handle=hologram_read<std::uint32_t>(object,0x668);steam=nullptr;
            if(handle!=disp_invalid&&!b.resolve(b.context,handle,&steam))return false;
            if(!b.remove(b.context,steam))return false;
            hologram_write(object,0x668,disp_invalid);
        }
    }
    const int count=health_int_delta(hologram_read<int>(object,0x638),1);
    hologram_write(object,0x638,count);
    if(count<=0){hologram_write(object,0x63c,0.0f);hologram_write(object,can_amount,0.0f);}
    return !(hologram_read<unsigned>(object,can_flags)&0x20u)||b.break_container(b.context,object)!=0;
}
int __cdecl prop_container_stop(HologramControl* ctrl,const PropContainerBindings* b) noexcept {
    if(!can_valid(ctrl)||!b||!b->stop_sound||!b->resolve||!b->remove||!b->break_container)return 0;
    ctrl->busy=1;const auto host=*b;
    return disp_finish(ctrl,prop_container_stop_body(ctrl->object,host));
}
int __cdecl prop_container_radius(const void* object,unsigned bytes,float* out) noexcept {
    if(!out||bytes<can_bytes||!sin_independent::vgui_layout::Readable(object,can_bytes))return 0;
    const auto flags=hologram_read<unsigned>(object,can_flags);
    *out=(flags&0x60u)?256.0f:((flags&0x0cu)?125.0f:0.0f);
    return 1;
}
const PropContainerApi prop_container_api{version,sizeof(PropContainerApi),&prop_container_defaults,
    &prop_container_stop,&prop_container_radius};

constexpr unsigned gas_bytes=0x3bc,gas_on=0x394,gas_kind=0x398,gas_amount=0x39c,
    gas_radius=0x3a0,gas_duration=0x3a4,gas_rate=0x3a8,gas_last=0x3ac,
    gas_time=0x3b0,gas_child=0x3b4,gas_audio=0x3b8;
constexpr GasVariant gas_variants[]={
    {0x10000,0xb43cea32,0xff00ff00,"gas.poison_spawn","gas.poison_loop"},
    {0x400000,0xffe58619,0xffe54f19,"gas.antigen_spawn","gas.antigen_loop"},
    {2,0xff11a4ed,0xff2864ef,"gas.poison_spawn","gas.poison_loop"},
    {3,0xff808080,0xff808080,"gas.inert_spawn","gas.inert_loop"}
};
bool gas_valid(const HologramControl* ctrl) noexcept {return health_control(ctrl,gas_bytes);}
bool gas_type_valid(int kind) noexcept {return kind>=0&&kind<4;}
bool gas_bound(const GasBindings* b) noexcept {
    return b&&b->clock&&b->random_float&&b->install_think&&b->schedule&&b->visual&&
        b->remove&&b->sound&&b->radius_effect;
}
int __cdecl gas_defaults(void* object,unsigned bytes) noexcept {
    if(bytes<gas_bytes||!hologram_writable(object,gas_bytes))return 0;
    hologram_write(object,gas_on,static_cast<unsigned char>(1));
    hologram_write(object,gas_audio,static_cast<unsigned char>(1));
    for(unsigned off:{gas_kind,gas_last,gas_time,gas_child})hologram_write(object,off,0u);
    hologram_write(object,gas_amount,5.0f);hologram_write(object,gas_radius,300.0f);
    hologram_write(object,gas_duration,20.0f);hologram_write(object,gas_rate,15.0f);return 1;
}
int __cdecl gas_configure(HologramControl* ctrl,int kind,float duration,float radius,float rate,float amount) noexcept {
    if(!gas_valid(ctrl)||!std::isfinite(duration)||!std::isfinite(radius)||!std::isfinite(rate)||!std::isfinite(amount))return 0;
    auto* obj=ctrl->object;
    if(hologram_read<void*>(obj,gas_child))return 0;
    hologram_write(obj,gas_kind,kind);
    hologram_write(obj,gas_duration,duration<0?20.0f:duration);
    hologram_write(obj,gas_radius,radius<0?300.0f:radius);
    hologram_write(obj,gas_rate,rate<0?15.0f:rate);
    hologram_write(obj,gas_amount,amount<0?5.0f:amount);return 1;
}
bool gas_clock(const GasBindings& b,float& now) noexcept {return b.clock(b.context,&now)&&std::isfinite(now);}
bool gas_schedule(void* obj,const GasBindings& b,double delta) noexcept {
    float now=0;if(!gas_clock(b,now))return false;
    const float when=static_cast<float>(static_cast<double>(now)+delta);
    return std::isfinite(when)&&b.schedule(b.context,obj,when)!=0;
}
bool gas_install(void* obj,const GasBindings& b) noexcept {
    float delay=0;
    return b.install_think(b.context,obj,1)&&b.random_float(b.context,0,0.5f,&delay)&&
        std::isfinite(delay)&&delay>=0&&delay<=0.5f&&gas_schedule(obj,b,delay);
}
bool gas_start_body(void* obj,const GasBindings& b) noexcept {
    if(hologram_read<void*>(obj,gas_child))return true;
    const int kind=hologram_read<int>(obj,gas_kind);
    if(!gas_type_valid(kind))return false;
    const float radius=hologram_read<float>(obj,gas_radius),duration=hologram_read<float>(obj,gas_duration),
        rate=hologram_read<float>(obj,gas_rate);
    if(!std::isfinite(radius)||!std::isfinite(duration)||!std::isfinite(rate))return false;
    if(!gas_install(obj,b))return false;
    float now=0;if(!gas_clock(b,now))return false;
    hologram_write(obj,gas_time,static_cast<float>(static_cast<double>(now)+1.0));
    void* child=nullptr;
    if(!b.visual(b.context,obj,&gas_variants[kind],kind,radius,duration,rate,&child))return false;
    hologram_write(obj,gas_child,child);
    return !hologram_read<unsigned char>(obj,gas_audio)||b.sound(b.context,obj,gas_variants[kind].spawn_sound,0);
}
bool gas_stop_body(void* obj,const GasBindings& b) noexcept {
    void* child=hologram_read<void*>(obj,gas_child);
    if(!child)return true;
    const int kind=hologram_read<int>(obj,gas_kind);
    if(!gas_type_valid(kind)||!b.remove(b.context,child))return false;
    hologram_write(obj,gas_child,static_cast<void*>(nullptr));
    return b.install_think(b.context,obj,0)&&b.schedule(b.context,obj,-1.0f)&&
        b.sound(b.context,obj,gas_variants[kind].loop_sound,1);
}
int __cdecl gas_spawn(HologramControl* ctrl,const GasBindings* b) noexcept {
    if(!gas_valid(ctrl)||!gas_bound(b))return 0;
    ctrl->busy=1;const auto host=*b;auto* obj=ctrl->object;
    if(!gas_type_valid(hologram_read<int>(obj,gas_kind)))hologram_write(obj,gas_kind,0);
    hologram_write(obj,gas_amount,5.0f);
    return disp_finish(ctrl,!hologram_read<unsigned char>(obj,gas_on)||gas_start_body(obj,host));
}
int __cdecl gas_enable(HologramControl* ctrl,const GasBindings* b,unsigned enabled) noexcept {
    if(!gas_valid(ctrl)||!gas_bound(b)||enabled>1)return 0;
    auto* obj=ctrl->object;
    if((hologram_read<unsigned char>(obj,gas_on)!=0)==(enabled!=0))return 1;
    ctrl->busy=1;const auto host=*b;hologram_write(obj,gas_on,static_cast<unsigned char>(enabled));
    return disp_finish(ctrl,enabled?gas_start_body(obj,host):gas_stop_body(obj,host));
}
int __cdecl gas_think(HologramControl* ctrl,const GasBindings* b) noexcept {
    if(!gas_valid(ctrl)||!gas_bound(b))return 0;
    auto* obj=ctrl->object;const int kind=hologram_read<int>(obj,gas_kind);
    if(!gas_type_valid(kind))return 0;
    if(kind==3)return 1;
    ctrl->busy=1;const auto host=*b;float now=0;
    if(!gas_clock(host,now))return disp_finish(ctrl,false);
    float last=hologram_read<float>(obj,gas_last);
    if(last==0){last=now;hologram_write(obj,gas_last,now);}
    const double elapsed=static_cast<double>(now)-last;
    if(elapsed>0.25){
        const float delta=static_cast<float>(elapsed);
        const float damage=static_cast<float>(static_cast<double>(delta)*hologram_read<float>(obj,gas_amount));
        const float radius=hologram_read<float>(obj,gas_radius);
        float duration=hologram_read<float>(obj,gas_duration);
        if(!std::isfinite(damage)||!std::isfinite(radius)||!std::isfinite(duration)||
           !host.radius_effect(host.context,obj,damage,radius,gas_variants[kind].damage_flags))return disp_finish(ctrl,false);
        if(duration>0){
            const double remaining=static_cast<double>(duration)-delta;
            hologram_write(obj,gas_duration,static_cast<float>(remaining));
            if(remaining<=0)return disp_finish(ctrl,gas_stop_body(obj,host)&&host.remove(host.context,obj));
            if(kind==1&&hologram_read<void*>(obj,gas_child)){
                if(!gas_stop_body(obj,host)||!gas_install(obj,host))return disp_finish(ctrl,false);
            }
        }else if(hologram_read<unsigned char>(obj,gas_audio)&&
            !host.sound(host.context,obj,gas_variants[kind].loop_sound,0))return disp_finish(ctrl,false);
        hologram_write(obj,gas_last,now);
    }
    return disp_finish(ctrl,gas_schedule(obj,host,0.25));
}
int __cdecl gas_kill(HologramControl* ctrl,const GasBindings* b) noexcept {
    if(!gas_valid(ctrl)||!gas_bound(b))return 0;
    ctrl->busy=1;const auto host=*b;
    return disp_finish(ctrl,gas_stop_body(ctrl->object,host)&&host.remove(host.context,ctrl->object));
}
int __cdecl gas_ack(HologramControl* ctrl) noexcept {
    if(!ctrl||ctrl->busy||!ctrl->blocked||ctrl->bytes<gas_bytes||!hologram_writable(ctrl->object,gas_bytes))return 0;
    ctrl->blocked=0;return 1;
}
const PoisonGasApi poison_gas_api{version,sizeof(PoisonGasApi),&gas_defaults,&gas_configure,&gas_spawn,
    &gas_enable,&gas_think,&gas_kill,&gas_ack};

constexpr unsigned poison_value=0x14d4,poison_source=0x14d8,poison_onset=0x14dc,poison_intensity=0x14e0,poison_wire=0x13a0;
bool poison_valid(const HologramControl* ctrl) noexcept {return health_control(ctrl,0x14e4);}
bool poison_resolve(void* player,const PlayerPoisonBindings& b,void*& gas) noexcept {
    gas=nullptr;unsigned bytes=0;
    const auto handle=hologram_read<std::uint32_t>(player,poison_source);
    if(handle==disp_invalid)return true;
    if(!b.resolve(b.context,handle,&gas,&bytes))return false;
    return !gas||(bytes>=gas_bytes&&sin_independent::vgui_layout::Readable(gas,gas_bytes));
}
bool poison_inside_body(void* player,const PlayerPoisonBindings& b,unsigned& inside) noexcept {
    inside=0;void* gas=nullptr;
    if(!poison_resolve(player,b,gas))return false;
    if(!gas)return true; // stale/invalid and disabled leave handle untouched
    if(!poison_resolve(player,b,gas)||!gas)return false;
    if(!hologram_read<unsigned char>(gas,gas_on))return true;
    if(!poison_resolve(player,b,gas)||!gas)return false;
    float origin[3]{},pos[3]{};
    if(!b.position(b.context,gas,origin)||!b.position(b.context,player,pos))return false;
    float delta[3]{};
    for(unsigned i=0;i<3;++i)delta[i]=pos[i]-origin[i];
    const float d2=static_cast<float>(static_cast<double>(delta[2])*delta[2]+
        static_cast<double>(delta[1])*delta[1]+static_cast<double>(delta[0])*delta[0]);
    const float distance=std::sqrt(d2);
    if(!poison_resolve(player,b,gas)||!gas)return false;
    if(static_cast<double>(distance)<static_cast<double>(hologram_read<float>(gas,gas_radius))+10.0){
        inside=1;return true;
    }
    hologram_write(player,poison_source,disp_invalid);return true;
}
int __cdecl poison_spawn(HologramControl* ctrl) noexcept {
    if(!poison_valid(ctrl))return 0;
    hologram_write(ctrl->object,poison_value,1.0f);
    hologram_write(ctrl->object,poison_source,disp_invalid);
    hologram_write(ctrl->object,poison_onset,0.0f);return 1;
}
int __cdecl poison_attach(HologramControl* ctrl,std::uint32_t handle) noexcept {
    if(!poison_valid(ctrl))return 0;
    hologram_write(ctrl->object,poison_source,handle);return 1;
}
int __cdecl poison_inside(HologramControl* ctrl,const PlayerPoisonBindings* b,unsigned* out) noexcept {
    if(!poison_valid(ctrl)||!b||!b->resolve||!b->position||!out)return 0;
    ctrl->busy=1;unsigned result=0;
    const bool ok=poison_inside_body(ctrl->object,*b,result);if(ok)*out=result;
    return disp_finish(ctrl,ok);
}
int __cdecl poison_tick(HologramControl* ctrl,PoisonTiming* timing,const PlayerPoisonBindings* b) noexcept {
    if(!poison_valid(ctrl)||!timing||timing->busy||timing->blocked||!b||!b->resolve||!b->position||
       !b->clock||!b->damage||!b->effect||!b->intensity||!b->dirty)return 0;
    ctrl->busy=timing->busy=1;const auto host=*b;auto* player=ctrl->object;
    const auto finish=[&](bool ok){
        timing->busy=0;if(!ok)timing->blocked=1;
        return disp_finish(ctrl,ok);
    };
    const auto clock=[&](float& now){return host.clock(host.context,&now)&&std::isfinite(now);};
    unsigned inside=0;float now=0;
    if(!poison_inside_body(player,host,inside)||!clock(now))return finish(false);
    if(timing->last>now)timing->last=now;
    if(static_cast<double>(now)+0.6f<timing->next_damage)timing->next_damage=now;
    const double delta=(static_cast<double>(now)-timing->last)*0.15;
    timing->last=now;
    const double value=static_cast<double>(hologram_read<float>(player,poison_value))+(inside?-delta:delta);
    hologram_write(player,poison_value,static_cast<float>(value));
    if(value>1.0)hologram_write(player,poison_value,1.0f);
    else if(value<=0){
        hologram_write(player,poison_value,0.0f);
        if(inside&&timing->next_damage<now){
            void* gas=nullptr;
            if(!poison_resolve(player,host,gas)||!host.damage(host.context,player,gas,10.0f,0x40000u)||!clock(now))
                return finish(false);
            timing->next_damage=static_cast<float>(static_cast<double>(now)+0.6f);
        }
    }
    const float stored=hologram_read<float>(player,poison_value);
    if(stored<1.0f){
        if(hologram_read<float>(player,poison_onset)==0){
            if(!clock(now))return finish(false);
            hologram_write(player,poison_onset,now);
        }
        const unsigned id=stored>0.75f?26u:stored>0.5f?27u:stored>0.25f?28u:29u;
        float intensity=0;
        if(!host.effect(host.context,player,id)||!host.intensity(host.context,stored,&intensity))return finish(false);
        hologram_write(player,poison_intensity,intensity);
    }else if(hologram_read<float>(player,poison_onset)>0){
        if(!host.effect(host.context,player,0))return finish(false);
        hologram_write(player,poison_onset,0.0f);hologram_write(player,poison_intensity,1.0f);
    }
    // Original reads current value again after dirty callback.
    if(hologram_read<float>(player,poison_value)!=hologram_read<float>(player,poison_wire)){
        if(!host.dirty(host.context,player,0x137c))return finish(false);
        hologram_write(player,poison_wire,hologram_read<float>(player,poison_value));
    }
    return finish(true);
}
int __cdecl poison_ack(HologramControl* ctrl,PoisonTiming* timing) noexcept {
    if(!ctrl||ctrl->busy||!ctrl->blocked||ctrl->bytes<0x14e4||!hologram_writable(ctrl->object,0x14e4)||
       (timing&&timing->busy))return 0;
    ctrl->blocked=0;if(timing)timing->blocked=0;return 1;
}
const PlayerPoisonApi player_poison_api{version,sizeof(PlayerPoisonApi),&poison_spawn,&poison_attach,&poison_inside,&poison_tick,&poison_ack};

int __cdecl gas_input_apply(HologramControl* ctrl,const GasInputBindings* b,GasInput input,const GasInputValue* value) noexcept {
    if(!gas_valid(ctrl)||!b||!value||static_cast<unsigned>(input)>5)return 0;
    const auto host=*b;const auto v=*value;auto* gas=ctrl->object;
    auto* child=hologram_read<void*>(gas,gas_child);
    // Duration has no child dependency. SoundOn/color handlers do
    // nothing without a visual child. Radius/rate always store their
    // parent values first, even if no child exists.
    const bool vector_input=input==GasInput::SetStartColor||input==GasInput::SetEndColor;
    if(input==GasInput::SetDuration){
        hologram_write(gas,gas_duration,v.type==1?v.scalar:0.0f);return 1;
    }
    if(input==GasInput::SoundOn){
        if(child)hologram_write(gas,gas_audio,static_cast<unsigned char>(v.type==6?v.boolean_value:0));
        return 1;
    }
    if(vector_input&&!child)return 1;
    if(child&&(!host.visual_storage||!host.dirty||
       (vector_input&&v.type!=3&&v.type!=15&&!host.fallback_vector)))return 0;
    ctrl->busy=1;
    const auto finish=[&](bool ok){return disp_finish(ctrl,ok);};
    if(!vector_input){
        const float scalar=v.type==1?v.scalar:0.0f;
        const bool radius=input==GasInput::SetRadius;
        hologram_write(gas,radius?gas_radius:gas_rate,scalar);
        if(!child)return finish(true);
        unsigned bytes=0;
        if(!host.visual_storage(host.context,child,&bytes)||bytes<0x3d0||
           !hologram_writable(child,0x3d0))return finish(false);
        const unsigned offset=radius?0x3ccu:0x3b0u;
        const double next=radius?static_cast<double>(scalar)*0.75:static_cast<double>(scalar);
        if(static_cast<double>(hologram_read<float>(child,offset))!=next){
            if(!host.dirty(host.context,child,offset))return finish(false);
            hologram_write(child,offset,static_cast<float>(next));
        }
        return finish(true);
    }
    unsigned bytes=0;
    if(!host.visual_storage(host.context,child,&bytes)||bytes<0x3d0||
       !hologram_writable(child,0x3d0))return finish(false);
    float rgb[3]{};
    if(v.type==3||v.type==15)std::memcpy(rgb,v.vector,12);
    else if(!host.fallback_vector(host.context,rgb))return finish(false);
    constexpr float scale=1.0f/255.0f;
    for(auto& channel:rgb)channel=static_cast<float>(static_cast<double>(channel)*scale);
    const unsigned offset=input==GasInput::SetStartColor?0x394u:0x3a0u;
    bool changed=false;
    for(unsigned i=0;i<3;++i)
        changed=changed||hologram_read<float>(child,offset+i*4)!=rgb[i];
    if(changed){
        if(!host.dirty(host.context,child,offset))return finish(false);
        std::memcpy(static_cast<unsigned char*>(child)+offset,rgb,12);
    }
    return finish(true);
}
const GasInputApi gas_input_api{version,sizeof(GasInputApi),&gas_input_apply};

int __cdecl player_damage_take(HologramControl* ctrl,const PlayerDamageBindings* b,const PlayerDamageInfo* info,int* result) noexcept {
    if(!health_control(ctrl,0x14f8)||!b||!info||!result||!b->global_state||!b->clock||
       !b->resolve||!b->entity_group||!b->notify_source||!b->sound||!b->poison_cast||
       !b->adjust_damage||!b->base_damage||!sin_independent::vgui_layout::Readable(info,sizeof(*info)))return 0;
    ctrl->busy=1;const auto host=*b;auto* player=ctrl->object;
    *result=0;
    const auto finish=[&](bool ok){return disp_finish(ctrl,ok);};
    int state=0;
    if(!host.global_state(host.context,"gordon_invulnerable",&state))return finish(false);
    if(state==1||hologram_read<int>(player,0x8c)!=0)return finish(true);
    const auto flags=hologram_read<unsigned>(info,0x38);
    const auto clock=[&](float& now){return host.clock(host.context,&now)&&std::isfinite(now);};
    const auto resolve=[&](const PlayerDamageInfo& src,unsigned offset,void*& entity){
        entity=nullptr;const auto handle=hologram_read<std::uint32_t>(&src,offset);
        return handle==disp_invalid||host.resolve(host.context,handle,&entity)!=0;
    };
    float now=0;
    if(flags&0x20u){
        if(!clock(now))return finish(false);
        if(hologram_read<float>(player,0x14b8)>now){
            hologram_write(player,0x14b8,0.0f);return finish(true);
        }
    }
    if((flags&0x10000000u)&&hologram_read<int>(player,0x298)>2){
        void* actor=nullptr;int group=0;
        if(!resolve(*info,0x24,actor)||!actor||!host.entity_group(host.context,actor,&group))return finish(false);
        if(group==0)return finish(true);
    }
    if(hologram_read<float>(info,0x2c)>0){
        if(!clock(now))return finish(false);
        hologram_write(player,0x14f4,now);
        void* source=nullptr;
        if(!resolve(*info,0x28,source))return finish(false);
        if(source){
            if(!resolve(*info,0x28,source)||!host.notify_source(host.context,player,source))return finish(false);
        }
    }
    if(flags&0x4000u){
        const bool start=hologram_read<int>(player,0x9f4)==hologram_read<int>(player,0x9f8);
        if(!host.sound(host.context,player,start?"Player.DrownStart":"Player.DrownContinue"))return finish(false);
    }
    if((flags&8u)&&!host.sound(host.context,player,"HL2Player.BurnPain"))return finish(false);
    PlayerDamageInfo copy=*info;
    if(hologram_read<unsigned>(&copy,0x38)&0x30000u){
        void* source=nullptr;unsigned matched=0;std::uint32_t handle=disp_invalid;
        if(!resolve(copy,0x28,source)||!host.poison_cast(host.context,source,&matched,&handle)||matched>1)return finish(false);
        if(matched)hologram_write(player,poison_source,handle);
        return finish(true); // even a failed cast suppresses base damage
    }
    void* source=nullptr;
    if(!resolve(*info,0x28,source))return finish(false);
    if(source!=player){
        void* actor=nullptr;
        if(!resolve(*info,0x24,actor))return finish(false);
        if(actor!=player){
            hologram_write(&copy,0x34,hologram_read<float>(&copy,0x2c));
            if(!host.adjust_damage(host.context,&copy))return finish(false);
        }
    }
    return finish(host.base_damage(host.context,player,&copy,result)!=0);
}
const PlayerDamageApi player_damage_api{version,sizeof(PlayerDamageApi),&player_damage_take};

constexpr unsigned jessica_size=0x1458, jessica_condition=0x47;
int __cdecl jessica_spawn_state(HologramControl* ctrl) noexcept {
    if(!health_control(ctrl,jessica_size))return 0;
    hologram_write(ctrl->object,0x1454,-1);
    hologram_write(ctrl->object,0x1450,-1);
    return 1;
}
int __cdecl jessica_input(HologramControl* ctrl,const JessicaBindings* b,JessicaInput op,const JessicaValue* v) noexcept {
    if(!health_control(ctrl,jessica_size)||!b)return 0;
    if(op==JessicaInput::Holster){
        if(!b->resolve_target||!b->holster)return 0;
        ctrl->busy=1;void* target=nullptr;
        const auto handle=hologram_read<std::uint32_t>(ctrl->object,0x78c);
        if(handle!=0xffffffffu&&!b->resolve_target(b->context,handle,&target))return disp_finish(ctrl,false);
        return disp_finish(ctrl,!target||b->holster(b->context,target,0)!=0);
    }
    if(!v||!sin_independent::vgui_layout::Readable(v,sizeof(*v)))return 0;
    if(op==JessicaInput::ThrowItem){
        if(!b->condition)return 0;
        ctrl->busy=1;
        hologram_write(ctrl->object,0x144c,v->type==2?v->word:0u);
        unsigned ignored=0;
        return disp_finish(ctrl,b->condition(b->context,ctrl->object,JessicaCondition::Set,jessica_condition,&ignored)!=0);
    }
    if(op!=JessicaInput::PrimaryAmmo&&op!=JessicaInput::SecondaryAmmo)return 0;
    hologram_write(ctrl->object,op==JessicaInput::PrimaryAmmo?0x1450:0x1454,v->type==5?v->word:0u);
    return 1;
}
int __cdecl jessica_schedule(HologramControl* ctrl,const JessicaBindings* b,int* result) noexcept {
    if(!health_control(ctrl,jessica_size)||!b||!b->condition||!b->base_schedule||!result)return 0;
    ctrl->busy=1;unsigned present=0;
    if(!b->condition(b->context,ctrl->object,JessicaCondition::Test,jessica_condition,&present))return disp_finish(ctrl,false);
    if(present){*result=0x59;return disp_finish(ctrl,true);}
    return disp_finish(ctrl,b->base_schedule(b->context,ctrl->object,result)!=0);
}
int __cdecl jessica_interrupts(HologramControl* ctrl,const JessicaBindings* b) noexcept {
    if(!health_control(ctrl,jessica_size)||!b||!b->condition||!b->base_interrupts)return 0;
    ctrl->busy=1;unsigned ignored=0;
    if(!b->condition(b->context,ctrl->object,JessicaCondition::Interrupt,jessica_condition,&ignored))return disp_finish(ctrl,false);
    return disp_finish(ctrl,b->base_interrupts(b->context,ctrl->object)!=0);
}
int __cdecl jessica_animation(HologramControl* ctrl,const JessicaBindings* b,const void* event,int id,int registered,JessicaEventResult* result) noexcept {
    if(!health_control(ctrl,jessica_size)||!b||!event||!result)return 0;
    if(id!=registered){
        if(!b->base_event)return 0;
        ctrl->busy=1;*result=JessicaEventResult::Forwarded;
        return disp_finish(ctrl,b->base_event(b->context,ctrl->object,event)!=0);
    }
    if(!b->condition||!b->max_clients||!b->single_player||!b->attachment||!b->spawn||!b->weapon_cast||!b->post_spawn||!b->toss)return 0;
    ctrl->busy=1;auto* npc=ctrl->object;const auto host=*b;
    *result=JessicaEventResult::NoSpawn;
    const auto finish=[&](bool ok){return disp_finish(ctrl,ok);};
    unsigned ignored=0;
    if(!host.condition(host.context,npc,JessicaCondition::Clear,jessica_condition,&ignored))return finish(false);
    int maxclients=0;void* player=nullptr;
    if(!host.max_clients(host.context,&maxclients))return finish(false);
    if(maxclients<=1&&!host.single_player(host.context,&player))return finish(false);
    JessicaAttachment attachment{};
    if(!host.attachment(host.context,npc,"anim_attachment_RH",&attachment))return finish(false);
    void* item=nullptr;
    if(!host.spawn(host.context,npc,hologram_read<std::uint32_t>(npc,0x144c),&attachment,&item))return finish(false);
    if(!item)return finish(true);
    void* weapon=nullptr;unsigned bytes=0;
    if(!host.weapon_cast(host.context,item,&weapon,&bytes))return finish(false);
    if(weapon){
        if(bytes<0x4dc||!hologram_writable(weapon,0x4dc))return finish(false);
        for(unsigned off:{0x1450u,0x1454u}){
            const auto count=hologram_read<int>(npc,off);
            if(count>=0){hologram_write(weapon,off-0xf7c,count);hologram_write(npc,off,-1);}
        }
    }
    if(!host.post_spawn(host.context,item))return finish(false);
    // The original dereferences the single-player target here. Refuse
    // the unsafe call after preserving preceding effects if it is absent.
    if(!player)return finish(false);
    if(!host.toss(host.context,item,player,attachment.origin,15.0f))return finish(false);
    *result=JessicaEventResult::Thrown;
    return finish(true);
}
const JessicaApi jessica_api{version,sizeof(JessicaApi),&jessica_spawn_state,&jessica_input,&jessica_schedule,&jessica_interrupts,&jessica_animation,&health_ack};

int __cdecl jessica_take_damage(HologramControl* ctrl,const JessicaCombatBindings* b,const PlayerDamageInfo* info,int* result) noexcept {
    if(!health_control(ctrl,jessica_size)||!b||!b->speak||!b->base_damage||!info||!result||
       !sin_independent::vgui_layout::Readable(info,sizeof(*info)))return 0;
    ctrl->busy=1;auto* npc=ctrl->object;const auto host=*b;PlayerDamageInfo copy=*info;
    if(hologram_read<unsigned>( &copy,0x38)&0x10000u){
        if(!host.speak(host.context,npc,"TLK_INPOISONCLOUD",0,0))return disp_finish(ctrl,false);
    }
    if(hologram_read<unsigned>(npc,0xb4)&0x10000u)hologram_write(&copy,0x2c,0.0f);
    return disp_finish(ctrl,host.base_damage(host.context,npc,&copy,result)!=0);
}
bool jessica_active_weapon(void* npc,const JessicaCombatBindings& b,void*& weapon) noexcept {
    weapon=nullptr;const auto handle=hologram_read<std::uint32_t>(npc,0x78c);
    return handle==0xffffffffu||b.resolve_weapon(b.context,handle,&weapon)!=0;
}
int __cdecl jessica_scene_enter(HologramControl* ctrl,const JessicaCombatBindings* b,void* scene,std::uint32_t argument) noexcept {
    if(!health_control(ctrl,jessica_size)||!b||!scene||!b->scene_predicate||!b->base_scene||!b->resolve_weapon||!b->holster)return 0;
    ctrl->busy=1;auto* npc=ctrl->object;const auto host=*b;unsigned skip=0;
    if(!host.scene_predicate(host.context,scene,argument,&skip))return disp_finish(ctrl,false);
    if(!skip){
        void* weapon=nullptr;
        if(!jessica_active_weapon(npc,host,weapon))return disp_finish(ctrl,false);
        if(weapon&&!host.holster(host.context,weapon,0))return disp_finish(ctrl,false);
    }
    return disp_finish(ctrl,host.base_scene(host.context,npc,scene,argument)!=0);
}
int __cdecl jessica_scene_exit(HologramControl* ctrl,const JessicaCombatBindings* b) noexcept {
    if(!health_control(ctrl,jessica_size)||!b||!b->resolve_weapon||!b->weapon_query||!b->weapon_restore)return 0;
    ctrl->busy=1;auto* npc=ctrl->object;const auto host=*b;
    if(hologram_read<int>(npc,0x8c)!=0)return disp_finish(ctrl,true);
    void* weapon=nullptr;
    if(!jessica_active_weapon(npc,host,weapon))return disp_finish(ctrl,false);
    if(!weapon)return disp_finish(ctrl,true);
    // Re-resolve at the same points as the original. A host that
    // observes disappearance refuses the unsafe original dereference.
    if(!jessica_active_weapon(npc,host,weapon)||!weapon)return disp_finish(ctrl,false);
    unsigned query=0;
    if(!host.weapon_query(host.context,weapon,&query))return disp_finish(ctrl,false);
    if(query)return disp_finish(ctrl,true);
    if(!jessica_active_weapon(npc,host,weapon)||!weapon)return disp_finish(ctrl,false);
    return disp_finish(ctrl,host.weapon_restore(host.context,weapon)!=0);
}
const JessicaCombatApi jessica_combat_api{version,sizeof(JessicaCombatApi),&jessica_take_damage,&jessica_scene_enter,&jessica_scene_exit,&health_ack};

bool jessica_attention_record(void* npc,const void* observation,unsigned base,unsigned enable,float& last,
                              bool looking,const JessicaAttentionBindings& host) noexcept {
    const auto* record=static_cast<const unsigned char*>(observation)+base;
    if(!hologram_read<unsigned char>(observation,enable)||
       (!hologram_read<unsigned char>(record,0x108)&&!hologram_read<unsigned char>(record,0x260))){
        last=0.0f;return true;
    }
    float now=0;
    if(!host.clock(host.context,&now))return false;
    const unsigned off=looking&&hologram_read<unsigned char>(record,0x2c0)?0x2e4:0x2bc;
    // x87 retains the subtraction in an extended precision register
    // until storing the shared float timestamp. Do not round it early.
    const double elapsed=static_cast<double>(now)-hologram_read<float>(record,off);
    if(looking&&elapsed<last)last=static_cast<float>(elapsed);
    if(last!=0.0f&&!(elapsed-static_cast<double>(last)>3.0))return true;
    last=static_cast<float>(elapsed);
    // Original rereads current time just before the criteria producer.
    if(!host.clock(host.context,&now))return false;
    return host.speak_criteria(host.context,npc,record,now,looking?"TLK_PLLOOKING":"TLK_PLUSING")!=0;
}
int __cdecl jessica_attention(HologramControl* ctrl,JessicaAttentionState* state,const JessicaAttentionBindings* b,const void* obs,unsigned bytes) noexcept {
    if(!health_control(ctrl,jessica_size)||!state||!hologram_writable(state,sizeof(*state))||
       !b||!b->clock||!b->speak_criteria||!obs||bytes<0x5e9||
       !sin_independent::vgui_layout::Readable(obs,0x5e9))return 0;
    // The second record only accesses +0x2bc and +0x260, so its extent
    // is covered by the wrapper's final enable byte at +0x5e8.
    ctrl->busy=1;const auto host=*b;
    if(!jessica_attention_record(ctrl->object,obs,0x18,0x300,state->looking,true,host))return disp_finish(ctrl,false);
    if(!jessica_attention_record(ctrl->object,obs,0x308,0x5e8,state->using_part,false,host))return disp_finish(ctrl,false);
    return disp_finish(ctrl,true);
}
const JessicaAttentionApi jessica_attention_api{version,sizeof(JessicaAttentionApi),&jessica_attention,&health_ack};

constexpr unsigned count_size=0x1038;
int __cdecl count_initialize(HologramControl* ctrl) noexcept {
    if(!health_control(ctrl,count_size))return 0;
    hologram_write(ctrl->object,0x1010,static_cast<unsigned char>(0));
    hologram_write(ctrl->object,0x1014,0);
    hologram_write(ctrl->object,0x1018,static_cast<unsigned char>(0));
    return 1;
}
int __cdecl count_input(HologramControl* ctrl,unsigned start) noexcept {
    if(!health_control(ctrl,count_size)||start>1)return 0;
    hologram_write(ctrl->object,0x1010,static_cast<unsigned char>(start));
    return 1;
}
int __cdecl count_enabled(HologramControl* ctrl,unsigned* result) noexcept {
    if(!health_control(ctrl,count_size)||!result)return 0;
    *result=hologram_read<unsigned char>(ctrl->object,0x1010);
    return 1;
}
int __cdecl count_observe(HologramControl* ctrl,const CountBindings* b,const void* obs,unsigned bytes) noexcept {
    if(!health_control(ctrl,count_size)||!b||!b->resolve||!b->clock||!b->expeditor||!b->speaking||!b->criteria||!b->speak||
       !obs||bytes<0x2f8||!sin_independent::vgui_layout::Readable(obs,0x2f8))return 0;
    ctrl->busy=1;auto* npc=ctrl->object;const auto host=*b;
    const auto finish=[&](bool ok){return disp_finish(ctrl,ok);};
    void* target=nullptr;const auto handle=hologram_read<std::uint32_t>(obs,0x14);
    if(handle!=0xffffffffu&&!host.resolve(host.context,handle,&target))return finish(false);
    if(target!=npc)return finish(true);
    float now=0;if(!host.clock(host.context,&now))return finish(false);
    if(!(static_cast<double>(now)-hologram_read<float>(obs,0x2d4)>1.0))return finish(true);
    void* expeditor=nullptr;
    if(!host.expeditor(host.context,npc,&expeditor))return finish(false);
    if(!expeditor)return finish(true);
    if(!host.expeditor(host.context,npc,&expeditor)||!expeditor)return finish(false);
    unsigned busy=0;if(!host.speaking(host.context,expeditor,&busy))return finish(false);
    if(busy)return finish(true);
    const auto* part=static_cast<const char*>(obs)+0x2d8;
    auto* prior=static_cast<char*>(npc)+0x1018;
    // Original strcmp/strncpy assume terminated strings. Refuse malformed
    // host observations instead of reading outside the supplied record.
    if(!std::memchr(part,0,32)||!std::memchr(prior,0,32))return finish(false);
    if(!std::strcmp(prior,part))return finish(true);
    std::strncpy(prior,part,32);
    CountCriteria criteria{};
    if(!host.clock(host.context,&now)||!host.criteria(host.context,static_cast<const char*>(obs)+0x18,now,&criteria))return finish(false);
    const int state=hologram_read<int>(npc,0x1014);
    constexpr const char* concepts[]={"TLK_INTERROGATE_COCKY","TLK_INTERROGATE_NERVOUS","TLK_INTERROGATE_TALKING"};
    if(state<0||state>2)return finish(true);
    if(!host.expeditor(host.context,npc,&expeditor)||!expeditor)return finish(false);
    unsigned played=0;
    if(!host.speak(host.context,expeditor,concepts[state],&criteria,&played))return finish(false);
    // Original advances regardless of whether a response matched/played.
    if(state==2){*prior=0;hologram_write(npc,0x1014,0);}
    else hologram_write(npc,0x1014,state+1);
    return finish(true);
}
int __cdecl count_damage(HologramControl* ctrl,const CountBindings* b,const PlayerDamageInfo* info,int* result) noexcept {
    if(!health_control(ctrl,count_size)||!b||!b->behavior_damage||!b->base_damage||!info||!result||
       !sin_independent::vgui_layout::Readable(info,sizeof(*info)))return 0;
    ctrl->busy=1;const auto host=*b;auto* npc=ctrl->object;PlayerDamageInfo copy=*info;
    hologram_write(&copy,0x2c,0.0f);
    const auto behavior=hologram_read<std::uint32_t>(npc,0xc18);
    if(behavior)return disp_finish(ctrl,host.behavior_damage(host.context,behavior,&copy,result)!=0);
    return disp_finish(ctrl,host.base_damage(host.context,npc,&copy,result)!=0);
}
const CountInterrogationApi count_api{version,sizeof(CountInterrogationApi),&count_initialize,&count_input,&count_enabled,&count_observe,&count_damage,&health_ack};

int character_region_common(HologramControl* ctrl,const CountRegionBindings* b,const CountRegionQuery* query,const char** result,bool count) noexcept {
    if(!health_control(ctrl,count?count_size:jessica_size)||!b||!b->forward_xy||!b->hit_identity||!query||!result||
       !sin_independent::vgui_layout::Readable(query,sizeof(*query)))return 0;
    *result=nullptr;
    // Original x87 ordered greater-than; exactly 180 is accepted.
    if(query->distance>180.0f)return 1;
    ctrl->busy=1;auto* npc=ctrl->object;const auto host=*b;const auto q=*query;
    const auto finish=[&](bool ok){return disp_finish(ctrl,ok);};
    float xy[2]{};
    if(!host.forward_xy(host.context,npc,xy))return finish(false);
    const bool positive=static_cast<double>(q.direction[0])*xy[0]+static_cast<double>(q.direction[1])*xy[1]>0.0;
    int group=0;const char* bone=nullptr;
    if(!host.hit_identity(host.context,npc,q.hitbox,&group,&bone))return finish(false);
    char name[128]{};
    if(!prop_key_read(bone,name))return finish(false);
    // These case-sensitive bone substrings precede the hitgroup table.
    if(std::strstr(name,"Bip01_Pelvis"))*result=positive?"butt":"crotch";
    else if(count&&std::strstr(name,"Hand"))*result="hand";
    else if(count&&std::strstr(name,"Calf"))*result="knee";
    else {
        static constexpr const char* groups[]={"generic stuff","head","chest","stomach","left arm","right arm","left leg","right leg"};
        if(group>=0&&group<8)*result=group==2&&positive?"back":groups[group];
    }
    return finish(true);
}
int __cdecl count_region(HologramControl* ctrl,const CountRegionBindings* b,const CountRegionQuery* query,const char** result) noexcept {
    return character_region_common(ctrl,b,query,result,true);
}
int __cdecl character_semaphore(HologramControl* ctrl,unsigned* uses) noexcept {
    if(!health_control(ctrl,0xb8)||!uses)return 0;
    *uses=(hologram_read<unsigned char>(ctrl->object,0xb6)&1u)?0u:1u;
    return 1;
}
int __cdecl jessica_damage_filter(HologramControl* ctrl,const JessicaFilterBindings* b,const PlayerDamageInfo* info,unsigned* result) noexcept {
    if(!health_control(ctrl,jessica_size)||!b||!b->base_filter||!info||!result||
       !sin_independent::vgui_layout::Readable(info,sizeof(*info)))return 0;
    if(hologram_read<unsigned>(info,0x38)&0x10000u){*result=1;return 1;}
    ctrl->busy=1;const auto host=*b;
    return disp_finish(ctrl,host.base_filter(host.context,ctrl->object,info,result)!=0);
}
const CharacterResponseApi character_response_api{version,sizeof(CharacterResponseApi),&count_region,&character_semaphore,&jessica_damage_filter,&health_ack};

bool action_string(const char* text,unsigned capacity) noexcept {
    return std::memchr(text,0,capacity)!=nullptr;
}
bool action_snapshot(const ActionMonitorBindings& b,void* entity,ActionEntitySample& sample) noexcept {
    if(!entity){sample={};return true;}
    return b.sample(b.context,entity,&sample)&&action_string(sample.name,sizeof(sample.name))&&
        action_string(sample.classname,sizeof(sample.classname));
}
int __cdecl action_update_look(HologramControl* ctrl,const ActionMonitorBindings* b,const ActionLookSample* input) noexcept {
    if(!health_control(ctrl,0x5e9)||!b||!b->resolve||!b->clock||!b->packed_handle||!b->sample||
       !input||!sin_independent::vgui_layout::Readable(input,sizeof(*input)))return 0;
    char part[32]{};
    if(input->part&&!prop_key_read(input->part,part))return 0;
    ctrl->busy=1;auto* monitor=static_cast<unsigned char*>(ctrl->object);const auto host=*b;const auto q=*input;
    auto finish=[&](bool ok){return disp_finish(ctrl,ok);};
    void* previous=nullptr;const auto previous_handle=hologram_read<std::uint32_t>(monitor,0x14);
    if(previous_handle!=0xffffffffu&&!host.resolve(host.context,previous_handle,&previous))return finish(false);
    auto* prior=reinterpret_cast<char*>(monitor+0x2d8);
    if(previous!=q.target){
        float now=0;if(!host.clock(host.context,&now))return finish(false);
        hologram_write(monitor,0x2d4,now);
        if(!host.clock(host.context,&now))return finish(false);
        hologram_write(monitor,0x2fc,now);
    }else if(std::strncmp(prior,part,32)!=0){
        float now=0;if(!host.clock(host.context,&now))return finish(false);
        hologram_write(monitor,0x2fc,now);
    }
    std::uint32_t target_handle=0xffffffffu;
    if(q.target&&!host.packed_handle(host.context,q.target,&target_handle))return finish(false);
    hologram_write(monitor,0x14,target_handle);
    void* owner=nullptr;const auto owner_handle=hologram_read<std::uint32_t>(monitor,4);
    if(owner_handle!=0xffffffffu&&!host.resolve(host.context,owner_handle,&owner))return finish(false);
    ActionEntitySample owner_sample{},target_sample{};
    if(!action_snapshot(host,owner,owner_sample)||!action_snapshot(host,q.target,target_sample))return finish(false);
    hologram_write(monitor,0x2d0,1); // LOOK_STARTED
    std::memcpy(monitor+0x20,&owner_sample,sizeof(owner_sample));
    std::memcpy(monitor+0x178,&target_sample,sizeof(target_sample));
    hologram_write(monitor,0x2f8,q.detail);
    if(!q.part)*prior=0;
    else if(std::strncmp(prior,part,32)!=0)std::memcpy(prior,part,32);
    return finish(true);
}
bool action_record_valid(const void* record,unsigned bytes,unsigned minimum) noexcept {
    return record&&bytes>=minimum&&sin_independent::vgui_layout::Readable(record,minimum);
}
template<class... Args> void action_append(ActionCriteria& out,const char* format,Args... args) noexcept {
    // The original Q_snprintf uses a 512-byte destination and its own
    // preceding contents as a format argument. Use a separate scratch
    // for defined overlapping-buffer semantics and bounded termination.
    char scratch[512]{};
    std::snprintf(scratch,sizeof(scratch),format,args...);
    std::memcpy(out.text,scratch,sizeof(scratch));
}
int __cdecl action_criteria(const void* record,unsigned bytes,float now,ActionCriteria* out,bool looking) noexcept {
    if(!out||!action_record_valid(record,bytes,looking?0x2e8:0x2e0))return 0;
    const auto* r=static_cast<const char*>(record);
    const auto* owner=r+0x108;const auto* subject=r+0x260;
    const auto* subject_name=r+0x160;const auto* part=r+0x2c0;
    if(!action_string(owner,64)||!action_string(subject,64)||!action_string(subject_name,256)||!action_string(part,32))return 0;
    out->text[0]=0;
    if(!*owner&&!*subject)return 1;
    const auto elapsed=static_cast<double>(now)-hologram_read<float>(record,0x2bc);
    if(looking){
        if(*subject_name)action_append(*out,"%s:%s,","PlayerLookingAt",subject_name);
        action_append(*out,"%s%s:%s,",out->text,"PlayerLookingAtEnt",subject);
        action_append(*out,"%s%s:%.2f",out->text,"PlayerLookingAtForSeconds",elapsed);
        if(*part){
            action_append(*out,"%s,%s:%s",out->text,"PlayerLookingAtPart",part);
            action_append(*out,"%s,%s:%.2f",out->text,"PlayerLookingAtPartForSeconds",static_cast<double>(now)-hologram_read<float>(record,0x2e4));
        }
    }else{
        static constexpr const char* actions[]={"USE_ACTION_NONE","USE_ACTION_USE","USE_ACTION_ON","USE_ACTION_OFF","USE_ACTION_LOCKED","USE_ACTION_PICKED_UP","USE_ACTION_DROPPED","USE_ACTION_TOSSED","USE_ACTION_OPENED","USE_ACTION_CLOSED","USE_ACTION_CONTINUOUS_START","USE_ACTION_CONTINUOUS_STOP","USE_ACTION_DRAGGED"};
        const auto id=hologram_read<unsigned>(record,0x2b8);
        if(id>=sizeof(actions)/sizeof(actions[0]))return 0;
        if(*subject_name)action_append(*out,"%s:%s,","PlayerUsing",subject_name);
        action_append(*out,"%s%s:%s",out->text,"PlayerUsingEnt",subject);
        action_append(*out,"%s,%s:%s",out->text,"PlayerUsingPart",part);
        action_append(*out,"%s,%s:%s",out->text,"PlayerUsingAction",actions[id]);
    }
    return 1;
}
int __cdecl action_look_criteria(const void* r,unsigned bytes,float now,ActionCriteria* out) noexcept {return action_criteria(r,bytes,now,out,true);}
int __cdecl action_use_criteria(const void* r,unsigned bytes,ActionCriteria* out) noexcept {return action_criteria(r,bytes,0,out,false);}
const ActionMonitorApi action_monitor_api{version,sizeof(ActionMonitorApi),&action_update_look,&action_look_criteria,&action_use_criteria,&health_ack};

constexpr unsigned radio_size=0x3d4;
bool radio_resolve(const RadioBindings& b,std::uint32_t h,void*& entity) noexcept {
    entity=nullptr;if(h==0xffffffffu)return true;
    unsigned bytes=0;
    return b.resolve(b.context,h,&entity,&bytes)&&(!entity||(bytes>=radio_size&&sin_independent::vgui_layout::Readable(entity,radio_size)));
}
bool radio_text(const RadioBindings& b,std::uint32_t word,const char*& out) noexcept {
    out="";if(!word)return true;
    const char* value=nullptr;
    if(!b.string(b.context,word,&value)||!value)return false;
    out=value;return true;
}
bool radio_set_message(void* object,const RadioBindings& b,std::uint32_t word) noexcept {
    const char* name=nullptr;
    if(!radio_text(b,word,name))return false;
    if(!*name)word=0;
    hologram_write(object,0x3c8,word);
    return b.precache(b.context,name)!=0;
}
bool radio_output(void* object,const RadioBindings& b,unsigned off) noexcept {
    int clients=0;if(!b.max_clients(b.context,&clients))return false;
    if(clients>1)return true;
    void* player=nullptr;
    if(!b.single_player(b.context,&player))return false;
    return !player||b.output(b.context,object,player,off)!=0;
}
int __cdecl radio_initialize(HologramControl* ctrl) noexcept {
    if(!health_control(ctrl,radio_size))return 0;
    for(unsigned off:{0x394u,0x3a4u,0x3acu,0x3bcu,0x3c4u,0x3c8u,0x3ccu})hologram_write(ctrl->object,off,0u);
    for(unsigned off:{0x3a0u,0x3b8u})hologram_write(ctrl->object,off,0xffffffffu);
    for(unsigned off:{0x3d0u,0x3d1u,0x3d2u})hologram_write(ctrl->object,off,static_cast<unsigned char>(0));
    return 1;
}
int __cdecl radio_input(HologramControl* ctrl,RadioDispatcher* state,const RadioBindings* b,RadioInput op,const RadioValue* val) noexcept {
    if(!health_control(ctrl,radio_size)||!b)return 0;
    if(op!=RadioInput::Remove&&(!val||!sin_independent::vgui_layout::Readable(val,sizeof(*val))))return 0;
    if(op==RadioInput::Speaker){hologram_write(ctrl->object,0x3cc,val->type==2?val->word:0u);return 1;}
    if(op==RadioInput::Priority){hologram_write(ctrl->object,0x3c4,val->type==5&&val->word==1?1u:0u);return 1;}
    if(op!=RadioInput::Message&&op!=RadioInput::Dispatch&&op!=RadioInput::Remove)return 0;
    if((op==RadioInput::Message||op==RadioInput::Dispatch)&&(!b->string||!b->precache))return 0;
    if(op==RadioInput::Message&&val->type!=2&&!b->convert_variant)return 0;
    if(op!=RadioInput::Message&&(!state||!hologram_writable(state,sizeof(*state))||!b->resolve))return 0;
    if(op==RadioInput::Dispatch&&(!b->handle||!b->max_clients||!b->single_player||!b->output))return 0;
    if(op==RadioInput::Remove&&!b->clear_delivery)return 0;
    ctrl->busy=1;auto* object=ctrl->object;const auto host=*b;
    auto finish=[&](bool ok){return disp_finish(ctrl,ok);};
    if(op==RadioInput::Message){
        std::uint32_t word=val->word;
        if(val->type!=2&&!host.convert_variant(host.context,val,&word))return finish(false);
        return finish(radio_set_message(object,host,word));
    }
    if(op==RadioInput::Dispatch){
        if(val->type==2&&val->word&&!radio_set_message(object,host,val->word))return finish(false);
        hologram_write(object,0x3d1,static_cast<unsigned char>(0));
        const auto priority=hologram_read<unsigned>(object,0x3c4);
        void* previous=nullptr;
        if(!radio_resolve(host,state->pending,previous))return finish(false);
        // Only reject a nonzero priority when a live objective is pending.
        if(!previous||!priority||hologram_read<unsigned>(previous,0x3c4)!=0){
            std::uint32_t handle=0xffffffffu;
            if(!host.handle(host.context,object,&handle))return finish(false);
            state->pending=handle;state->kind=priority?2:1;
        }
        // Map output fires even when priority prevented replacement.
        return finish(radio_output(object,host,0x394));
    }
    hologram_write(object,0x3d1,static_cast<unsigned char>(1));
    for(auto* slot:{&state->pending,&state->current,&state->other}){
        void* entity=nullptr;
        if(!radio_resolve(host,*slot,entity))return finish(false);
        if(entity==object){*slot=0xffffffffu;return finish(slot==&state->other||host.clear_delivery(host.context)!=0);}
    }
    return finish(true);
}
int __cdecl radio_heard(HologramControl* ctrl,const RadioBindings* b,unsigned value) noexcept {
    if(!health_control(ctrl,radio_size)||!b||!b->max_clients||!b->single_player||!b->output||value>255)return 0;
    ctrl->busy=1;
    hologram_write(ctrl->object,0x3d0,static_cast<unsigned char>(value));
    return disp_finish(ctrl,radio_output(ctrl->object,*b,0x3ac));
}
int __cdecl radio_promote(RadioDispatcher* s,const RadioBindings* b) noexcept {
    if(!s||!hologram_writable(s,sizeof(*s))||!b||!b->resolve)return 0;
    void* entity=nullptr;
    if(!radio_resolve(*b,s->pending,entity))return -1;
    s->current=entity?s->pending:s->other;s->pending=0xffffffffu;
    if(!radio_resolve(*b,s->current,entity))return -1;
    s->stage=6;
    if(entity){
        if(s->kind==1)s->stage=4;
        else if(s->kind==3&&hologram_read<unsigned>(entity,0x3c4)==1)s->stage=3;
    }
    return 1;
}
int __cdecl radio_sound(const RadioDispatcher* s,const RadioBindings* b,unsigned stage,RadioSound* out) noexcept {
    if(!s||!b||!b->resolve||!b->string||!out)return 0;
    *out={};if(stage>4)return 1;
    if(stage==2){std::memcpy(out->text,"..",3);out->present=1;return 1;}
    void* entity=nullptr;
    if(!radio_resolve(*b,stage<3?s->pending:s->current,entity))return -1;
    if(!entity)return 1;
    const auto priority=hologram_read<unsigned>(entity,0x3c4)?"Extraneous":"Objective";
    const auto speaker=hologram_read<std::uint32_t>(entity,0x3cc);
    const char* name="";
    if(stage==0)std::snprintf(out->text,255,"policeradio.Alert.%s",priority);
    else if(stage==1){
        if(!radio_text(*b,speaker,name))return -1;
        std::snprintf(out->text,255,"%s.Notification.%s",name,priority);
    }else if(stage==3){
        if(!radio_text(*b,speaker,name))return -1;
        std::snprintf(out->text,255,"npc_Blade.Acknowledgement.%s%s%s",priority,*name?".":"",name);
    }else{
        if(!radio_text(*b,hologram_read<std::uint32_t>(entity,0x3c8),name))return -1;
        out->speaker_word=speaker;std::snprintf(out->text,255,"%s",name);
    }
    out->present=1;return 1;
}
int __cdecl radio_message_ack(HologramControl*) noexcept;
const RadioMessageApi radio_message_api{version,sizeof(RadioMessageApi),&radio_initialize,&radio_input,&radio_heard,&radio_promote,&radio_sound,&radio_message_ack};

int __cdecl look_trace_miss(HologramControl* ctrl) noexcept {
    if(!health_control(ctrl,0x5e9))return 0;
    auto* record=static_cast<unsigned char*>(ctrl->object)+0x18;
    // 2215b540 -> 2215b0d0. Preserve padding, vtable, category.
    for(unsigned off:{8u,0x108u,0x160u,0x260u,0x2c0u})record[off]=0;
    for(unsigned off:{0x148u,0x14cu,0x150u,0x154u,0x158u,0x15cu,0x2a0u,0x2a4u,0x2a8u,0x2acu,0x2b0u,0x2b4u,0x2b8u,0x2bcu,0x2e0u,0x2e4u})hologram_write(record,off,0u);
    hologram_write(ctrl->object,0x14,0xffffffffu);
    return 1;
}
bool look_window(void* monitor,unsigned base,unsigned enable,float min,float max,const LookEventBindings& b,bool& changed) noexcept {
    auto* record=static_cast<unsigned char*>(monitor)+base;
    const auto prior=hologram_read<unsigned char>(monitor,enable);
    unsigned char next=0;
    if(record[0x108]||record[0x260]){
        float now=0;if(!b.clock(b.context,&now))return false;
        const double elapsed=static_cast<double>(now)-hologram_read<float>(record,0x2bc);
        next=elapsed>=min&&elapsed<max?1:0;
    }
    changed=prior!=next;hologram_write(monitor,enable,next);
    return true;
}
int __cdecl look_event_tick(HologramControl* ctrl,LookEventState* shared,const LookEventBindings* b) noexcept {
    if(!health_control(ctrl,0x5e9)||!shared||!hologram_writable(shared,sizeof(*shared))||!b||!b->clock||!b->resolve||!b->look_event)return 0;
    ctrl->busy=1;auto* monitor=ctrl->object;const auto host=*b;
    auto finish=[&](bool ok){return disp_finish(ctrl,ok);};
    bool changed=false;
    if(!look_window(monitor,0x18,0x300,1.5f,(std::numeric_limits<float>::max)(),host,changed))return finish(false);
    const auto* part=static_cast<const char*>(monitor)+0x2d8;
    bool emit=changed;
    if(hologram_read<unsigned char>(monitor,0x300)){
        if(*part){
            const float start=hologram_read<float>(monitor,0x2fc);
            if(changed||shared->last_part==0.0f)shared->last_part=start;
            float now=0;if(!host.clock(host.context,&now))return finish(false);
            if(static_cast<double>(now)-start>=1.5&&start>shared->last_part){
                shared->last_part=start;emit=true;
            }
        }else shared->last_part=0.0f;
    }
    if(emit){
        if(!std::memchr(part,0,32))return finish(false);
        int descriptor=hologram_read<int>(monitor,0x2d0);if(!descriptor)descriptor=2;
        const auto distance=hologram_read<float>(monitor,0x2f8);
        void* target=nullptr;void* owner=nullptr;
        const auto target_handle=hologram_read<std::uint32_t>(monitor,0x14);
        const auto owner_handle=hologram_read<std::uint32_t>(monitor,4);
        if(target_handle!=0xffffffffu&&!host.resolve(host.context,target_handle,&target))return finish(false);
        if(owner_handle!=0xffffffffu&&!host.resolve(host.context,owner_handle,&owner))return finish(false);
        if(!host.look_event(host.context,descriptor,owner,target,distance,part))return finish(false);
        if(descriptor==2)hologram_write(monitor,0x14,0xffffffffu);
    }
    if(!look_window(monitor,0x308,0x5e8,0.0f,1.0f,host,changed))return finish(false);
    return finish(true);
}
const LookEventApi look_event_api{version,sizeof(LookEventApi),&look_trace_miss,&look_event_tick,&health_ack};

int __cdecl action_listener_subscribe(void* listener,const ActionListenerBindings* b) noexcept {
    if(!listener||!b||!b->prepare||!b->subscribe)return 0;
    const auto host=*b;
    if(!host.prepare(host.context))return -1;
    for(const char* name:{"look","use","weapon","move","health","map","game","dynamic_difficulty","player_activate"})
        if(!host.subscribe(host.context,listener,name,1))return -1;
    return 1;
}
int __cdecl action_listener_unsubscribe(void* listener,const ActionListenerBindings* b) noexcept {
    if(!listener||!b||!b->unsubscribe)return 0;
    return b->unsubscribe(b->context,listener)?1:-1;
}
bool action_prefix(const char* name,const char* prefix) noexcept {
    return std::strncmp(name,prefix,std::strlen(prefix))==0;
}
int action_listener_capture(const ActionListenerBindings& h,const void* event,ActionRoute route,int descriptor,void* owner,void* subject) noexcept {
    alignas(4) unsigned char record[0x2e8]{};
    float now=0,distance=0;
    char part[32]{};
    const bool looking=route==ActionRoute::Look;
    if(looking){
        const char* sub=nullptr;
        if(!h.get_string(h.context,event,"subobject","",&sub)||!sub)return -1;
        // The original copies 32 bytes with strncpy; a bounded prefix
        // is sufficient here, retaining a terminator for safe consumers.
        for(unsigned i=0;i<31;++i){
            if(!sin_independent::vgui_layout::Readable(sub+i,1))return -1;
            part[i]=sub[i];if(!part[i])break;
        }
        if(!h.get_float(h.context,event,"distance",0,&distance))return -1;
    }
    if(!h.clock(h.context,&now))return -1;
    ActionEntitySample a{},s{};
    const auto sample=[&](void* entity,ActionEntitySample& out){
        return !entity||(h.sample(h.context,entity,&out)&&action_string(out.name,sizeof(out.name))&&action_string(out.classname,sizeof(out.classname)));
    };
    if(!sample(owner,a)||!sample(subject,s))return -1;
    hologram_write(record,4,looking?0:1);
    // The original use setter copies subject before owner; both source
    // samples have already been taken in owner/subject order.
    std::memcpy(record+8,&a,sizeof(a));std::memcpy(record+0x160,&s,sizeof(s));
    hologram_write(record,0x2b8,descriptor);hologram_write(record,0x2bc,now);
    if(looking){std::memcpy(record+0x2c0,part,sizeof(part));hologram_write(record,0x2e0,distance);}
    return h.record(h.context,route,record,looking?0x2e8:0x2e0)?1:-1;
}
int __cdecl action_listener_dispatch(const ActionListenerBindings* b,const void* event) noexcept {
    if(!event||!b||!b->get_int||!b->resolve||!b->participant||!b->name||!b->route||
       !b->get_string||!b->get_float||!b->clock||!b->sample||!b->record)return 0;
    const auto h=*b;
    int descriptor=0,oh=0,sh=0;
    if(!h.get_int(h.context,event,"descriptor",0,&descriptor)||!h.get_int(h.context,event,"owner",0,&oh))return -1;
    void* owner=nullptr;void* subject=nullptr;
    if(static_cast<unsigned>(oh)!=0xffffffffu&&!h.resolve(h.context,static_cast<unsigned>(oh),&owner))return -1;
    if(!h.get_int(h.context,event,"subject",0,&sh))return -1;
    if(static_cast<unsigned>(sh)!=0xffffffffu&&!h.resolve(h.context,static_cast<unsigned>(sh),&subject))return -1;
    const char* raw=nullptr;
    if(!h.name(h.context,event,&raw))return -1;
    char name[128]{};
    if(!prop_key_read(raw,name))return -1;
    const auto route=[&](ActionRoute kind,void* a,void* s){return h.route(h.context,kind,event,descriptor,a,s)?1:-1;};
    // Map/game/difficulty/activation bypass the participant gate.
    if(action_prefix(name,"map"))return route(ActionRoute::Map,nullptr,nullptr);
    if(action_prefix(name,"game"))return route(ActionRoute::Game,owner,nullptr);
    if(action_prefix(name,"dynamic_difficulty"))return route(ActionRoute::Difficulty,nullptr,nullptr);
    if(action_prefix(name,"player_activate"))return route(ActionRoute::Activate,nullptr,nullptr);
    unsigned eligible=0;
    if(owner&&!h.participant(h.context,owner,&eligible))return -1;
    if(!eligible&&subject&&!h.participant(h.context,subject,&eligible))return -1;
    if(!eligible)return 1;
    if(action_prefix(name,"look"))return action_listener_capture(h,event,ActionRoute::Look,descriptor,owner,subject);
    if(action_prefix(name,"use"))return action_listener_capture(h,event,ActionRoute::Use,descriptor,owner,subject);
    if(action_prefix(name,"weapon"))return route(ActionRoute::Weapon,owner,subject);
    if(action_prefix(name,"move"))return route(ActionRoute::Move,owner,subject);
    if(action_prefix(name,"health"))return route(ActionRoute::Health,owner,subject);
    return 1;
}
const ActionListenerApi action_listener_api{version,sizeof(ActionListenerApi),&action_listener_subscribe,&action_listener_unsubscribe,&action_listener_dispatch};

int __cdecl jessica_region(HologramControl* ctrl,const CountRegionBindings* b,const CountRegionQuery* query,const char** result) noexcept {
    return character_region_common(ctrl,b,query,result,false);
}
int __cdecl player_look_frame(void* player,unsigned bytes,HologramControl* monitor,LookEventState* shared,const PlayerLookFrameBindings* b,const PlayerLookObservation* observation) noexcept {
    constexpr unsigned monitor_offset=0xc40,monitor_extent=0x5e9;
    if(!player||bytes<monitor_offset+monitor_extent||!health_control(monitor,monitor_extent)||
       monitor->object!=static_cast<unsigned char*>(player)+monitor_offset||
       !b||!observation||!sin_independent::vgui_layout::Readable(observation,sizeof(*observation))||
       !shared||!hologram_writable(shared,sizeof(*shared))||
       !b->event.clock||!b->event.resolve||!b->event.look_event)return 0;
    const auto host=*b;const auto hit=*observation;
    if(hit.hit>1)return 0;
    if(hit.hit&&(!hit.target||!host.classify||!host.monitor.resolve||!host.monitor.clock||!host.monitor.packed_handle||!host.monitor.sample))return 0;
    int status=0;
    if(hit.hit){
        // Original subtracts endpoints into float locals then sums
        // squares using x87 and rounds the sum before sqrt.
        float delta[3]{};
        for(unsigned i=0;i<3;++i)delta[i]=hit.end[i]-hit.start[i];
        const double sum=static_cast<double>(delta[0])*delta[0]+static_cast<double>(delta[1])*delta[1]+static_cast<double>(delta[2])*delta[2];
        const float distance=std::sqrt(static_cast<float>(sum));
        CountRegionQuery query{{hit.direction[0],hit.direction[1],hit.direction[2]},hit.hitbox,distance};
        const char* part=nullptr;
        // Guard the persistent monitor against reentry from the classifier.
        monitor->busy=1;
        if(!host.classify(host.context,hit.target,&query,&part))return disp_finish(monitor,false);
        monitor->busy=0;
        ActionLookSample sample{hit.target,0,part};
        std::memcpy(&sample.detail,&distance,4);
        status=action_update_look(monitor,&host.monitor,&sample);
    }else status=look_trace_miss(monitor);
    if(status!=1)return status;
    return look_event_tick(monitor,shared,&host.event);
}
const PlayerLookFrameApi player_look_frame_api{version,sizeof(PlayerLookFrameApi),&jessica_region,&player_look_frame};

bool radio_playback_ready(const RadioPlaybackControl* c) noexcept {
    return c&&!c->busy&&!c->blocked&&c->state&&hologram_writable(c->state,sizeof(*c->state));
}
int __cdecl radio_request(RadioPlaybackControl* c) noexcept {
    if(!radio_playback_ready(c))return 0;
    if(c->state->stage==6)c->state->kind=3;
    return 1;
}
int __cdecl radio_player_input(RadioPlaybackControl* c,unsigned a,unsigned pressed,unsigned released) noexcept {
    if(!radio_playback_ready(c))return 0;
    constexpr unsigned mask=0x08000000;
    if(!c->input_latch&&((a|pressed)&mask)){
        c->input_latch=1;
        if(c->state->stage==6)c->state->kind=3;
    }else if((a|released)&mask)c->input_latch=0;
    return 1;
}
bool radio_play(RadioDispatcher& s,const RadioPlaybackBindings& h,const char* sound) noexcept {
    if(!sound||!*sound)return true; // original logs empty sound, no emit
    const auto& b=h.message;int clients=0;
    if(!b.max_clients(b.context,&clients))return false;
    if(clients>1)return true;
    void* player=nullptr;if(!b.single_player(b.context,&player))return false;
    if(!player)return true;
    unsigned eligible=0;if(!h.participant(b.context,player,&eligible))return false;
    if(!eligible)return true;
    float duration=0,now=0;
    if(!h.emit(b.context,player,sound,4,1.0f,0.8f,&duration)||!h.clock(b.context,&now))return false;
    s.deadline=static_cast<float>(static_cast<double>(now)+duration);
    return true;
}
int __cdecl radio_playback_tick(RadioPlaybackControl* c,const RadioPlaybackBindings* b) noexcept {
    if(!radio_playback_ready(c)||!b||!b->clock||!b->participant||!b->emit||!b->notify||
       !b->message.resolve||!b->message.string||!b->message.max_clients||
       !b->message.single_player||!b->message.output||!b->message.clear_delivery)return 0;
    c->busy=1;auto& s=*c->state;const auto h=*b;const auto& m=h.message;
    auto finish=[&](bool ok){c->busy=0;if(!ok){c->blocked=1;return -1;}return 1;};
    float now=0;if(!h.clock(m.context,&now))return finish(false);
    if(s.deadline>now)return finish(true); // unordered also enters original
    s.deadline=-1.0f;
    switch(s.stage){
        case 6:
            if(s.kind==1||s.kind==2)s.stage=0;else if(s.kind==3)s.stage=2;
            return finish(true);
        case 2:
            return finish(radio_promote(&s,&m)==1);
        case 5:
            if(!h.clock(m.context,&now))return finish(false);
            if(now>s.deadline){
                if(!m.clear_delivery(m.context))return finish(false);
                s.stage=6;
            }
            return finish(true);
        case 0: case 1: case 3: case 4:break;
        default:return finish(true);
    }
    const int stage=s.stage;
    RadioSound sound{};
    if(radio_sound(&s,&m,static_cast<unsigned>(stage),&sound)!=1)return finish(false);
    if(!sound.present){s.stage=6;return finish(true);}
    if(!radio_play(s,h,sound.text))return finish(false);
    void* message=nullptr;
    if(stage==0){
        s.stage=s.kind==1?2:1;
        if(!radio_resolve(m,s.pending,message)||!message)return finish(false);
        const char* speaker=nullptr;
        if(!radio_text(m,hologram_read<std::uint32_t>(message,0x3cc),speaker))return finish(false);
        const int priority=static_cast<std::int16_t>(hologram_read<unsigned>(message,0x3c4)&0xffffu);
        return finish(h.notify(m.context,1,priority,speaker)!=0);
    }
    if(stage==1){
        s.stage=6;s.kind=0;
        if(!radio_resolve(m,s.pending,message)||!message)return finish(false);
        if(hologram_read<unsigned char>(message,0x3d1)){
            s.pending=0xffffffffu;
            return finish(m.clear_delivery(m.context)!=0);
        }
        return finish(true);
    }
    if(stage==3){s.stage=4;return finish(true);}
    s.stage=6;
    if(!radio_resolve(m,s.current,message))return finish(false);
    if(message){
        if(!radio_resolve(m,s.current,message)||!message||!hologram_writable(message,radio_size))return finish(false);
        hologram_write(message,0x3d0,static_cast<unsigned char>(1));
        if(!radio_output(message,m,0x3ac))return finish(false);
        if(!radio_resolve(m,s.current,message)||!message)return finish(false);
        if(hologram_read<unsigned>(message,0x3c4)==0)s.other=s.current;
        void* other=nullptr;if(!radio_resolve(m,s.other,other))return finish(false);
        if(other&&hologram_read<unsigned char>(other,0x3d1))s.other=0xffffffffu;
        s.current=0xffffffffu;
    }
    s.stage=5;s.kind=0;
    return finish(true);
}
int __cdecl radio_playback_ack(RadioPlaybackControl* c) noexcept {
    if(!c||c->busy||!c->blocked)return 0;
    c->blocked=0;return 1;
}
const RadioPlaybackApi radio_playback_api{version,sizeof(RadioPlaybackApi),&radio_request,&radio_player_input,&radio_playback_tick,&radio_playback_ack};

int __cdecl radio_message_ack(HologramControl* c) noexcept {
    if(!c||c->busy||!c->blocked||c->bytes<radio_size||!hologram_writable(c->object,radio_size))return 0;
    c->blocked=0;return 1;
}
int __cdecl radio_wire_encode(unsigned active,int priority,const char* speaker,unsigned char* payload,unsigned capacity,unsigned* used) noexcept {
    if(!used||!payload||capacity<5)return 0;
    *used=0;
    char name[2048]{};
    if(speaker&&!prop_key_read(speaker,name))return 0;
    const auto count=static_cast<unsigned>(std::strlen(name))+1;
    if(capacity<4+count)return 0;
    const auto p=static_cast<std::uint16_t>(priority);
    payload[0]=static_cast<unsigned char>(active);payload[1]=0;
    payload[2]=static_cast<unsigned char>(p);payload[3]=static_cast<unsigned char>(p>>8);
    std::memcpy(payload+4,name,count);*used=4+count;return 1;
}
const RadioWireApi radio_wire_api{version,sizeof(RadioWireApi),&radio_wire_encode};

constexpr RadioField radio_fields[]={
    {"message",RadioFieldKind::Key,0x13,0x3c8},
    {"speaker",RadioFieldKind::Key,2,0x3cc},
    {"priority",RadioFieldKind::Key,5,0x3c4},
    {"SetMessage",RadioFieldKind::Input,2,0},
    {"SetSpeaker",RadioFieldKind::Input,2,0},
    {"SetPriority",RadioFieldKind::Input,5,0},
    {"Dispatch",RadioFieldKind::Input,2,0},
    {"Remove",RadioFieldKind::Input,0,0},
    {"OnDispatch",RadioFieldKind::Output,0xb,0x394},
    {"OnHeard",RadioFieldKind::Output,0xb,0x3ac}
};
const RadioEntitySchema radio_entity_schema{"env_radio_message","CRadioMessage",radio_entity_bytes,10,radio_fields};
const RadioEntitySchema* __cdecl radio_schema() noexcept {return &radio_entity_schema;}
bool radio_entity_valid(const HologramControl* c) noexcept {return health_control(c,radio_entity_bytes);}
int __cdecl radio_construct(HologramControl* c,const RadioEntityBindings* b) noexcept {
    if(!radio_entity_valid(c)||!b||!b->base_construct||!b->class_init)return 0;
    c->busy=1;const auto h=*b;auto* entity=c->object;
    if(!h.base_construct(h.message.context,entity,radio_entity_bytes))return disp_finish(c,false);
    // Native leaf construction; no absolute original vtable installed.
    for(unsigned off:{0x394u,0x3a4u,0x3acu,0x3bcu,0x3c4u,0x3c8u,0x3ccu})hologram_write(entity,off,0u);
    for(unsigned off:{0x3a0u,0x3b8u})hologram_write(entity,off,0xffffffffu);
    for(unsigned off:{0x3d0u,0x3d1u,0x3d2u})hologram_write(entity,off,static_cast<unsigned char>(0));
    return disp_finish(c,h.class_init(h.message.context,entity,radio_entity_schema.classname)!=0);
}
int __cdecl radio_key(HologramControl* c,const char* name,const RadioValue* value) noexcept {
    if(!radio_entity_valid(c)||!value||!sin_independent::vgui_layout::Readable(value,sizeof(*value)))return 0;
    char key[64]{};
    if(!prop_key_read(name,key))return 0;
    // FIELD_SOUNDNAME (0x13) is the key descriptor. Host may supply
    // the pooled string as FIELD_STRING (2), but not an integer.
    for(unsigned i=0;i<3;++i){
        const auto& field=radio_fields[i];
        if(std::strcmp(key,field.name))continue;
        if(value->type!=field.field_type&&!(i==0&&value->type==2))return 0;
        hologram_write(c->object,field.offset,value->word);return 1;
    }
    return 0;
}
int __cdecl radio_named_input(HologramControl* c,RadioDispatcher* s,const RadioEntityBindings* b,const char* name,const RadioValue* value) noexcept {
    if(!radio_entity_valid(c)||!b)return 0;
    char key[64]{};if(!prop_key_read(name,key))return 0;
    constexpr RadioInput operations[]={RadioInput::Message,RadioInput::Speaker,RadioInput::Priority,RadioInput::Dispatch,RadioInput::Remove};
    for(unsigned i=0;i<5;++i)
        if(!std::strcmp(key,radio_fields[i+3].name))return radio_input(c,s,&b->message,operations[i],value);
    return 0;
}
bool radio_precache_leaves(void* entity,const RadioBindings& b) noexcept {
    static constexpr const char* sounds[]={
        "policeradio.Alert.Objective","policeradio.Alert.Extraneous",
        "npc_jessica.Notification.Objective","npc_jessica.Notification.Extraneous",
        "npc_jc.Notification.Objective","npc_jc.Notification.Extraneous",
        "npc_Blade.Acknowledgement.Objective.npc_jessica",
        "npc_Blade.Acknowledgement.Extraneous.npc_jessica",
        "npc_Blade.Acknowledgement.Objective.npc_jc",
        "npc_Blade.Acknowledgement.Extraneous.npc_jc"
    };
    for(const char* name:sounds)if(!b.precache(b.context,name))return false;
    const auto word=hologram_read<std::uint32_t>(entity,0x3c8);
    if(word){
        const char* name=nullptr;
        if(!radio_text(b,word,name)||!b.precache(b.context,name))return false;
    }
    return true;
}
int __cdecl radio_entity_precache(HologramControl* c,const RadioEntityBindings* b) noexcept {
    if(!radio_entity_valid(c)||!b||!b->message.precache||!b->message.string)return 0;
    c->busy=1;const auto h=*b;
    return disp_finish(c,radio_precache_leaves(c->object,h.message));
}
int __cdecl radio_entity_spawn(HologramControl* c,const RadioEntityBindings* b) noexcept {
    if(!radio_entity_valid(c)||!b||!b->message.precache||!b->message.string||!b->spawn_helper)return 0;
    c->busy=1;const auto h=*b;auto* entity=c->object;
    if(!radio_precache_leaves(entity,h.message))return disp_finish(c,false);
    if(!h.spawn_helper(h.message.context,entity,0x22201ac0u,0,0))return disp_finish(c,false);
    return disp_finish(c,h.spawn_helper(h.message.context,entity,0x22143610u,0,0)!=0);
}
int __cdecl radio_entity_activate(HologramControl* c,const RadioEntityBindings* b) noexcept {
    if(!radio_entity_valid(c)||!b||!b->base_activate||!b->message.precache||!b->message.string)return 0;
    c->busy=1;const auto h=*b;auto* entity=c->object;
    if(!h.base_activate(h.message.context,entity))return disp_finish(c,false);
    return disp_finish(c,radio_set_message(entity,h.message,hologram_read<std::uint32_t>(entity,0x3c8)));
}
int __cdecl radio_entity_destroy(HologramControl* c,const RadioEntityBindings* b) noexcept {
    if(!radio_entity_valid(c)||!b||!b->destroy_outputs||!b->base_destroy)return 0;
    c->busy=1;const auto h=*b;auto* entity=c->object;
    if(!h.destroy_outputs(h.message.context,entity,0x3ac)||!h.destroy_outputs(h.message.context,entity,0x394))return disp_finish(c,false);
    if(!h.base_destroy(h.message.context,entity))return disp_finish(c,false);
    // No use of possibly destroyed memory after base destructor.
    c->object=nullptr;c->bytes=0;c->busy=0;return 1;
}
const RadioEntityApi radio_entity_api{version,sizeof(RadioEntityApi),&radio_schema,&radio_construct,&radio_key,&radio_named_input,&radio_entity_precache,&radio_entity_spawn,&radio_entity_activate,&radio_entity_destroy};

int __cdecl radio_delivery_register(const RadioDeliveryBindings* b) noexcept {
    if(!b||!b->register_message)return 0;
    return b->register_message(b->context,radio_message_name,-1)?1:-1;
}
int __cdecl radio_delivery_notify(const RadioDeliveryBindings* b,unsigned active,int priority,const char* speaker) noexcept {
    if(!b||!b->max_clients||!b->edict||!b->player_from_unknown||!b->send)return 0;
    const auto h=*b;int clients=0;
    if(!h.max_clients(h.context,&clients))return -1;
    if(clients!=1)return 1;
    RadioEdictObservation edict{};
    if(!h.edict(h.context,1,&edict))return -1;
    if(!edict.present||(edict.flags&2u)||!edict.unknown)return 1;
    void* player=nullptr;
    if(!h.player_from_unknown(h.context,edict.unknown,&player))return -1;
    if(!player)return 1;
    unsigned char payload[2052]{};unsigned bytes=0;
    if(!radio_wire_encode(active,priority,speaker,payload,sizeof(payload),&bytes))return -1;
    return h.send(h.context,player,1,radio_message_name,payload,bytes)?1:-1;
}
const RadioDeliveryApi radio_delivery_api{version,sizeof(RadioDeliveryApi),&radio_delivery_register,&radio_delivery_notify};

constexpr ActorField generic_actor_fields[]={
    {"bloodColor",ActorFieldKind::Key,5,0x1000},
    {"initialHealth",ActorFieldKind::Key,5,0x1004},
    {"heads",ActorFieldKind::Key,5,0x1008},
    {"dontHammerBBox",ActorFieldKind::Key,5,0x1010},
    {"m_shouldCollideWithPlayer",ActorFieldKind::Save,6,0x100c},
    {"PlayerCollide",ActorFieldKind::Input,6,0x100c}
};
const ActorSchema generic_actor_schema{"npc_generic_actor",generic_actor_bytes,6,generic_actor_fields};
const ActorSchema* __cdecl generic_actor_get_schema() noexcept{return &generic_actor_schema;}
int __cdecl generic_actor_key(HologramControl* c,const char* name,const ActorValue* v) noexcept {
    if(!health_control(c,generic_actor_bytes)||!v||!sin_independent::vgui_layout::Readable(v,sizeof(*v))||v->type!=5)return 0;
    char key[64]{};if(!prop_key_read(name,key))return 0;
    for(const auto& field:generic_actor_fields)if(field.kind==ActorFieldKind::Key&&!std::strcmp(field.name,key)){
        hologram_write(c->object,field.offset,v->word);return 1;
    }
    return 0;
}
int __cdecl generic_actor_spawn_profile(HologramControl* c,const ActorBindings* b) noexcept {
    if(!health_control(c,generic_actor_bytes)||!b||!b->health_dirty)return 0;
    c->busy=1;auto* npc=c->object;const auto h=*b;
    // 22357a2f..22357a9b; original blood setter is a single dword write.
    hologram_write(npc,0x614,hologram_read<int>(npc,0x1000));
    if(hologram_read<int>(npc,0x9c)!=hologram_read<int>(npc,0x1004)){
        if(!h.health_dirty(h.context,npc))return disp_finish(c,false);
        hologram_write(npc,0x9c,hologram_read<int>(npc,0x1004));
    }
    hologram_write(npc,0x618,0.5f);
    hologram_write(npc,0x850,0);
    hologram_write(npc,0x100c,static_cast<unsigned char>(1));
    // 22030390 is a plain OR of capabilities at +8ec, twice in original.
    hologram_write(npc,0x8ec,hologram_read<unsigned>(npc,0x8ec)|0x800801u);
    hologram_write(npc,0x8ec,hologram_read<unsigned>(npc,0x8ec)|0x02000000u);
    hologram_write(npc,0xbb,static_cast<unsigned char>(hologram_read<unsigned char>(npc,0xbb)|0x58u));
    return disp_finish(c,true);
}
int __cdecl generic_actor_damage(HologramControl* c,const ActorBindings* b,PlayerDamageInfo* info,int* result) noexcept {
    if(!health_control(c,generic_actor_bytes)||!b||!b->behavior_damage||!b->base_damage||!info||!result||
       !hologram_writable(info,sizeof(*info)))return 0;
    c->busy=1;const auto h=*b;
    const auto behavior=hologram_read<std::uint32_t>(c->object,0xc18);
    return disp_finish(c,behavior?h.behavior_damage(h.context,behavior,info,result)!=0:h.base_damage(h.context,c->object,info,result)!=0);
}
int __cdecl generic_actor_observe(HologramControl* c,const ActorBindings* b,const void* obs) noexcept {
    if(!health_control(c,generic_actor_bytes)||!b||!b->behavior_observe||!obs)return 0;
    const auto behavior=hologram_read<std::uint32_t>(c->object,0xc18);
    if(!behavior)return 1;
    c->busy=1;const auto h=*b;
    return disp_finish(c,h.behavior_observe(h.context,behavior,obs)!=0);
}
int __cdecl generic_actor_semaphore(HologramControl* c,unsigned* uses) noexcept {
    if(!health_control(c,generic_actor_bytes)||!uses)return 0;
    return character_semaphore(c,uses);
}
const GenericActorApi generic_actor_api{version,sizeof(GenericActorApi),&generic_actor_get_schema,&generic_actor_key,&generic_actor_spawn_profile,&generic_actor_damage,&generic_actor_observe,&generic_actor_semaphore,&health_ack};
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
    bool spawn_bound=false,extra_bound=false,keypad_bound=false,phone_bound=false;
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
    if(g->spawn_bound||g->extra_bound||g->keypad_bound||g->phone_bound||!aggregate_matches(*g,*v))return 0;
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

void __cdecl keypad_string_recv(const RecvData* d,void*,void* out){
    if(!d||!d->property||!out)return;
    const int size=d->property->string_size;
    if(size!=255&&size!=32)return;
    const char* src=nullptr;std::memcpy(&src,d->value.words,4);
    if(!src)return;
    char buffer[255]{};unsigned n=0;
    for(;n<static_cast<unsigned>(size);++n){
        if(!sin_independent::vgui_layout::Readable(src+n,1))return;
        buffer[n]=src[n];if(!buffer[n]){++n;break;}
    }
    std::memcpy(out,buffer,n);static_cast<char*>(out)[size-1]=0;
}
struct KeypadRecvGraph {
    AggregateRecvGraph* base=nullptr;
    RecvTable tables[KeyTableCount];
    RecvProp props[KeyTableCount][7];
    ClientClassRecord records[KeyTableCount]{};
    ClientClassRecord* head=nullptr;
    bool build(AggregateRecvGraph& core,const KeypadRecvBindings& b) noexcept {
        for(auto creator:b.creators)if(!creator)return false;
        for(unsigned i=0;i<KeyTableCount;++i){
            tables[i]={props[i],keypad_counts[i],nullptr,keypad_tables[i]};
            nested(props[i][0],"baseclass",i==KeyScreen?&core.entity.entity:&tables[KeyScreen]);
            auto& rec=records[i];rec.name=keypad_classes[i];rec.table=&tables[i];rec.create=b.creators[i];
            rec.next=head;head=&rec;
        }
        auto* sc=props[KeyScreen];
        anim_recv(sc[1],"m_flWidth",0x48c,1);anim_recv(sc[2],"m_flHeight",0x490,1);
        anim_recv(sc[3],"m_nAttachmentIndex",0x4ac);anim_recv(sc[4],"m_nPanelName",0x494);
        anim_recv(sc[5],"m_fScreenFlags",0x4b4);anim_recv(sc[6],"m_nOverlayMaterial",0x4b0);
        for(unsigned i:{KeyCode,KeyProp})anim_recv(props[i][1],"m_bDisabled",0x510,0,&byte_recv);
        anim_recv(props[KeyCode][2],"m_combination",0x511,3,&keypad_string_recv);
        props[KeyCode][2].string_size=255;
        anim_recv(props[KeyProp][2],"m_sInput",0x511,3,&keypad_string_recv);
        props[KeyProp][2].string_size=32;
        return true;
    }
};

int __cdecl create_keypad(const KeypadRecvBindings* b,KeypadRecvView* out) noexcept {
    if(!b||!b->base||!b->base->handle||!out||out->handle||out->head||out->records)return 0;
    for(auto* tab:out->tables)if(tab)return 0;
    auto* base=static_cast<AggregateRecvGraph*>(b->base->handle);
    if(base->keypad_bound||!aggregate_matches(*base,*b->base))return 0;
    auto* g=new(std::nothrow) KeypadRecvGraph;
    if(!g)return 0;
    if(!g->build(*base,*b)){delete g;return 0;}
    g->base=base;base->keypad_bound=true;
    out->handle=g;out->head=g->head;out->records=g->records;
    for(unsigned i=0;i<KeyTableCount;++i)out->tables[i]=&g->tables[i];
    return 1;
}
int __cdecl destroy_keypad(KeypadRecvView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<KeypadRecvGraph*>(v->handle);
    if(v->head!=g->head||v->records!=g->records)return 0;
    for(unsigned i=0;i<KeyTableCount;++i){
        const auto& tab=g->tables[i];const auto& rec=g->records[i];
        if(v->tables[i]!=&tab||rec.class_id!=0||tab.decoder||tab.initialized||tab.in_main_list)return 0;
    }
    g->base->keypad_bound=false;delete g;*v={};return 1;
}
const KeypadRecvApi keypad_api{version,sizeof(KeypadRecvApi),&create_keypad,&destroy_keypad};

int __cdecl keypad_command(void* object,unsigned bytes,const KeypadClientBindings* b,int command) noexcept {
    if(!b||!b->append_command||!b->read_text||!b->create_event||!b->fire_event||!b->clear_text||!b->base_command)return 0;
    const auto host=*b;
    if(!object)return host.base_command(host.context,command)?1:-1;
    if(bytes<0x610||!sin_independent::vgui_layout::Readable(object,0x610))return 0;
    if(hologram_read<unsigned char>(object,0x510))return host.base_command(host.context,command)?1:-1;
    if(!host.append_command(host.context,command))return -1;
    char text[255]{};
    if(!host.read_text(host.context,text,sizeof(text)))return -1;
    const auto* combo=static_cast<const char*>(object)+0x511;
    const auto* text_end=static_cast<const char*>(std::memchr(text,0,sizeof(text)));
    const auto* combo_end=static_cast<const char*>(std::memchr(combo,0,255));
    if(!text_end||!combo_end)return -1;
    const bool equal=std::strcmp(text,combo)==0;
    if(equal||text_end-text>=combo_end-combo){
        void* event=nullptr;
        if(!host.create_event(host.context,equal?"keypad_combo_entered":"bad_keypad_combo_entered",1,&event))return -1;
        if(event&&!host.fire_event(host.context,event,0))return -1;
        if(!host.clear_text(host.context))return -1;
    }
    return host.base_command(host.context,command)?1:-1;
}
const KeypadClientApi keypad_behavior{version,sizeof(KeypadClientApi),&keypad_command};

int __cdecl prop_key_command(void* object,unsigned bytes,const PropKeypadClientBindings* b,const char* command) noexcept {
    if(!b||!b->create_event||!b->entity_index||!b->set_int||!b->set_string||!b->fire_event||!b->base_command)return 0;
    char text[256]{};
    if(!prop_key_read(command,text))return 0;
    const auto host=*b;
    if(object){
        if(bytes<0x538||!sin_independent::vgui_layout::Readable(object,0x538))return 0;
        if(!hologram_read<unsigned char>(object,0x510)){
            void* event=nullptr;
            if(!host.create_event(host.context,"prop_keypad_input",1,&event))return -1;
            if(event){
                int index=0;
                if(!host.entity_index(host.context,object,&index)||
                   !host.set_int(host.context,event,"entindex",index)||
                   !host.set_string(host.context,event,"Command",text)||
                   !host.fire_event(host.context,event,0))return -1;
            }
        }
    }
    return host.base_command(host.context,text)?1:-1;
}
int __cdecl prop_key_tick(void* object,unsigned bytes,const PropKeypadClientBindings* b) noexcept {
    if(!b||!b->display||!b->base_tick)return 0;
    const auto host=*b;
    if(object){
        if(bytes<0x538||!sin_independent::vgui_layout::Readable(object,0x538))return 0;
        if(!hologram_read<unsigned char>(object,0x510)){
            const auto* text=static_cast<const char*>(object)+0x511;
            if(!std::memchr(text,0,32))return 0;
            if(!host.display(host.context,text))return -1;
        }
    }
    return host.base_tick(host.context)?1:-1;
}
const PropKeypadClientApi prop_key_api{version,sizeof(PropKeypadClientApi),&prop_key_command,&prop_key_tick};

struct PhoneRecvGraph {
    AggregateRecvGraph* base=nullptr;
    RecvTable table{};
    RecvProp prop{};
    ClientClassRecord record{};
    bool build(AggregateRecvGraph& core,const PhoneRecvBindings& b) noexcept {
        if(!b.creator)return false;
        table={&prop,1,nullptr,"DT_CPropPhone"};nested(prop,"baseclass",&core.anim.base);record.create=b.creator;
        record.name="CPropPhone";record.table=&table;return true;
    }
};
int __cdecl create_phone(const PhoneRecvBindings* b,PhoneRecvView* out) noexcept {
    if(!b||!b->base||!b->base->handle||!out||out->handle||out->table||out->record)return 0;
    auto* base=static_cast<AggregateRecvGraph*>(b->base->handle);
    if(base->phone_bound||!aggregate_matches(*base,*b->base))return 0;
    auto* g=new(std::nothrow) PhoneRecvGraph;
    if(!g)return 0;
    if(!g->build(*base,*b)){delete g;return 0;}
    g->base=base;base->phone_bound=true;*out={g,&g->table,&g->record};return 1;
}
int __cdecl destroy_phone(PhoneRecvView* v) noexcept {
    if(!v||!v->handle)return 0;
    auto* g=static_cast<PhoneRecvGraph*>(v->handle);
    if(v->table!=&g->table||v->record!=&g->record||g->table.decoder||g->table.initialized||g->table.in_main_list||g->record.class_id)return 0;
    g->base->phone_bound=false;delete g;*v={};return 1;
}
const PhoneRecvApi phone_api{version,sizeof(PhoneRecvApi),&create_phone,&destroy_phone};

constexpr unsigned phud_bytes=0x130,phud_color=0xb7,phud_fraction=0x120,
    phud_foreground=0x124,phud_background=0x128,phud_inactive=0x12c;
bool phud_valid(const HologramControl* ctrl) noexcept {
    return ctrl&&!ctrl->busy&&!ctrl->blocked&&ctrl->bytes>=phud_bytes&&hologram_writable(ctrl->object,phud_bytes);
}
int phud_finish(HologramControl* ctrl,bool ok) noexcept {
    ctrl->busy=0;if(!ok){ctrl->blocked=1;return -1;}return 1;
}
int __cdecl phud_reset(HologramControl* ctrl) noexcept {
    if(!phud_valid(ctrl))return 0;
    hologram_write(ctrl->object,phud_fraction,0.0f);return 1;
}
int __cdecl phud_bind(HologramControl* ctrl,const PoisonHudBindings* b) noexcept {
    if(!phud_valid(ctrl)||!b||!b->resource)return 0;
    ctrl->busy=1;const auto host=*b;
    constexpr unsigned names[]={0xbc,0xdd,0xfe},slots[]={phud_foreground,phud_background,phud_inactive};
    for(unsigned i=0;i<3;++i){
        const auto* name=static_cast<const char*>(ctrl->object)+names[i];
        if(!std::memchr(name,0,33))return phud_finish(ctrl,false);
        void* resource=nullptr;
        if(!host.resource(host.context,name,&resource))return phud_finish(ctrl,false);
        hologram_write(ctrl->object,slots[i],resource);
    }
    return phud_finish(ctrl,true);
}
template<class Dispatch>
int phud_tick_body(HologramControl* ctrl,const void* player,unsigned bytes,Dispatch dispatch) noexcept {
    if(!player)return 1;
    if(bytes<0x11f0||!sin_independent::vgui_layout::Readable(player,0x11f0))return 0;
    ctrl->busy=1;auto* obj=ctrl->object;
    const float incoming=hologram_read<float>(player,0x11ec);
    if(incoming==hologram_read<float>(obj,phud_fraction))return phud_finish(ctrl,true);
    const auto alpha=hologram_read<unsigned char>(obj,phud_color+3);
    const char* event=nullptr;
    if(incoming<1.0f&&alpha==0)event="PoisonTimeFadeIn";
    else if(incoming>=1.0f&&alpha==255)event="PoisonTimeFadeOut";
    if(event&&!dispatch(event))return phud_finish(ctrl,false);
    // The original re-reads player after animation-controller dispatch.
    // Scheme animation owns color; this method never writes alpha.
    hologram_write(obj,phud_fraction,static_cast<float>(1.0-static_cast<double>(hologram_read<float>(player,0x11ec))));
    return phud_finish(ctrl,true);
}
int __cdecl phud_tick(HologramControl* ctrl,const PoisonHudBindings* b,const void* player,unsigned bytes) noexcept {
    if(!phud_valid(ctrl)||!b||!b->sound)return 0;
    const auto host=*b;
    return phud_tick_body(ctrl,player,bytes,[&](const char* event){return host.sound(host.context,event)!=0;});
}
int __cdecl phud_tick_animation(HologramControl* ctrl,const PoisonHudAnimationBindings* b,const void* player,unsigned bytes) noexcept {
    if(!phud_valid(ctrl)||!b||!b->controller||!b->start_sequence)return 0;
    const auto host=*b;
    return phud_tick_body(ctrl,player,bytes,[&](const char* event){
        void* controller=nullptr;unsigned started=0;
        if(!host.controller(host.context,&controller)||!controller)return false;
        // The original HUD ignores the sequence-start return boolean.
        return host.start_sequence(host.context,controller,event,&started)!=0;
    });
}
const PoisonHudAnimationApi poison_hud_animation_api{version,sizeof(PoisonHudAnimationApi),&phud_tick_animation};
int __cdecl phud_paint(HologramControl* ctrl,const PoisonHudBindings* b) noexcept {
    if(!phud_valid(ctrl)||!b||!b->base_paint||!b->draw)return 0;
    ctrl->busy=1;const auto host=*b;auto* obj=ctrl->object;
    if(!host.base_paint(host.context,static_cast<unsigned char*>(obj)+0x14))return phud_finish(ctrl,false);
    PoisonHudDraw draw{};
    draw.fraction=hologram_read<float>(obj,phud_fraction);
    draw.foreground=hologram_read<void*>(obj,phud_foreground);
    draw.background=hologram_read<void*>(obj,draw.fraction>0?phud_background:phud_inactive);
    if(!draw.foreground||!draw.background)return phud_finish(ctrl,true);
    std::memcpy(draw.rgba,static_cast<unsigned char*>(obj)+phud_color,4);
    draw.design_width=1600;draw.design_height=1200;draw.x=draw.y=0;draw.mode=3;
    return phud_finish(ctrl,host.draw(host.context,&draw)!=0);
}
int __cdecl phud_ack(HologramControl* ctrl) noexcept {
    if(!ctrl||ctrl->busy||!ctrl->blocked||ctrl->bytes<phud_bytes||!hologram_writable(ctrl->object,phud_bytes))return 0;
    ctrl->blocked=0;return 1;
}
const PoisonHudApi poison_hud_api{version,sizeof(PoisonHudApi),&phud_reset,&phud_bind,&phud_tick,&phud_paint,&phud_ack};


constexpr unsigned radio_hud_size=0xc0;
bool radio_hud_valid(const HologramControl* c) noexcept {
    return c&&!c->busy&&!c->blocked&&c->bytes>=radio_hud_size&&hologram_writable(c->object,radio_hud_size);
}
int __cdecl radio_hud_reset(HologramControl* c) noexcept {
    if(!radio_hud_valid(c))return 0;
    hologram_write(c->object,0xb6,static_cast<unsigned char>(0));
    hologram_write(c->object,0xb8,static_cast<std::uint16_t>(0));return 1;
}
int __cdecl radio_hud_init(HologramControl* c) noexcept {
    if(!radio_hud_reset(c))return 0;
    hologram_write(c->object,0xbc,static_cast<const char*>(nullptr));return 1;
}
int __cdecl radio_hud_receive(HologramControl* c,const RadioHudBindings* b,const unsigned char* payload,unsigned bytes) noexcept {
    if(!radio_hud_valid(c)||!b||!b->store_speaker||!b->panel_flag||!b->animation||!payload||bytes<5||bytes>2052||
       !sin_independent::vgui_layout::Readable(payload,bytes))return 0;
    const auto* name=reinterpret_cast<const char*>(payload+4);
    if(!std::memchr(name,0,bytes-4))return 0;
    const unsigned active=(payload[0]||payload[1])?1:0;
    const auto priority=static_cast<std::uint16_t>(payload[2]|(static_cast<unsigned>(payload[3])<<8));
    c->busy=1;const auto h=*b;auto* object=c->object;
    auto finish=[&](bool ok){c->busy=0;if(!ok){c->blocked=1;return -1;}return 1;};
    hologram_write(object,0xb6,static_cast<unsigned char>(active));
    hologram_write(object,0xb8,priority);
    const char* stored=nullptr;
    if(!h.store_speaker(h.context,name,&stored)||!stored)return finish(false);
    hologram_write(object,0xbc,stored);
    auto* panel=static_cast<unsigned char*>(object)+0x14;
    if(!h.panel_flag(h.context,panel,0x78,active)||!h.panel_flag(h.context,panel,0xfc,active))return finish(false);
    const char* sequence=active?(priority?"PoliceRadioShowExtraneous":"PoliceRadioShowObjective"):"PoliceRadioHide";
    return finish(h.animation(h.context,sequence)!=0);
}
int __cdecl radio_hud_icon(HologramControl* c,RadioIconRequest* out) noexcept {
    if(!radio_hud_valid(c)||!out)return 0;
    *out={};
    if(!hologram_read<unsigned char>(c->object,0xb6))return 1;
    if(!hologram_read<std::uint16_t>(c->object,0xb8))std::memcpy(out->name,"police_radio_obj",17);
    else {
        const auto* speaker=hologram_read<const char*>(c->object,0xbc);
        constexpr char prefix[]="police_radio_";
        std::memcpy(out->name,prefix,sizeof(prefix));
        if(speaker){
            // Original bounded concatenation into 64-byte icon key.
            for(unsigned i=0;i<sizeof(out->name)-sizeof(prefix);++i){
                if(!sin_independent::vgui_layout::Readable(speaker+i,1))return 0;
                out->name[sizeof(prefix)-1+i]=speaker[i];
                if(!speaker[i])break;
            }
        }
    }
    out->present=1;out->width=1600;out->height=1200;return 1;
}
int __cdecl radio_hud_ack(HologramControl* c) noexcept {
    if(!c||c->busy||!c->blocked||c->bytes<radio_hud_size||!hologram_writable(c->object,radio_hud_size))return 0;
    c->blocked=0;return 1;
}
const RadioHudApi radio_hud_api{version,sizeof(RadioHudApi),&radio_hud_init,&radio_hud_reset,&radio_hud_receive,&radio_hud_icon,&radio_hud_ack};

int __cdecl radio_client_route_register(const RadioClientRouteBindings* b) noexcept {
    if(!b||!b->register_message)return 0;
    return b->register_message(b->context,radio_message_name,-1)?1:-1;
}
int __cdecl radio_client_route_receive(const RadioClientRouteBindings* b,const unsigned char* payload,unsigned bytes) noexcept {
    if(!b||!b->find_hud)return 0;
    const auto h=*b;HologramControl* ctrl=nullptr;
    if(!h.find_hud(h.context,"CHudPoliceRadio",&ctrl))return -1;
    if(!ctrl)return 1;
    return radio_hud_receive(ctrl,&h.hud,payload,bytes);
}
const RadioClientRouteApi radio_client_route_api{version,sizeof(RadioClientRouteApi),&radio_client_route_register,&radio_client_route_receive};
bool screen_command_valid(const HologramControl* ctrl) noexcept {
    return ctrl&&!ctrl->busy&&!ctrl->blocked&&ctrl->bytes>=0xdc&&
        sin_independent::vgui_layout::Readable(ctrl->object,0xdc);
}
int __cdecl generic_screen_command(HologramControl* ctrl,const ScreenCommandBindings* b,const char* command) noexcept {
    if(!screen_command_valid(ctrl)||!b||!b->create_event||!b->resolve||!b->entity_index||
       !b->set_int||!b->set_string||!b->fire||!b->discard||!b->base_command)return 0;
    char text[256]{};
    if(!prop_key_read(command,text))return 0;
    ctrl->busy=1;const auto host=*b;
    const auto finish=[&](bool ok){ctrl->busy=0;if(!ok){ctrl->blocked=1;return -1;}return 1;};
    void* event=nullptr;
    if(!host.create_event(host.context,"button_press",1,&event)){
        if(event)host.discard(host.context,event);
        return finish(false);
    }
    if(event){
        const auto handle=hologram_read<std::uint32_t>(ctrl->object,0xd8);
        void* screen=nullptr;int index=0;
        if(handle==0xffffffffu||!host.resolve(host.context,handle,&screen)||!screen||
           !host.entity_index(host.context,screen,&index)||
           !host.set_int(host.context,event,"entindex",index)||
           !host.set_string(host.context,event,"command",text)){
            host.discard(host.context,event);return finish(false);
        }
        if(!host.fire(host.context,event,0))return finish(false);
    }
    return finish(host.base_command(host.context,ctrl->object,text)!=0);
}
int __cdecl generic_screen_ack(HologramControl* ctrl) noexcept {
    if(!ctrl||ctrl->busy||!ctrl->blocked||ctrl->bytes<0xdc||
       !sin_independent::vgui_layout::Readable(ctrl->object,0xdc))return 0;
    ctrl->blocked=0;return 1;
}
const ScreenCommandApi screen_command_api{version,sizeof(ScreenCommandApi),&generic_screen_command,&generic_screen_ack};
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
#pragma comment(linker,"/EXPORT:SiN_GetKeypadSendApi=_SiN_GetKeypadSendApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::KeypadSendApi* __cdecl
SiN_GetKeypadSendApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(KeypadSendApi)?&keypad_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetKeypadServerApi=_SiN_GetKeypadServerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::KeypadServerApi* __cdecl
SiN_GetKeypadServerApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(KeypadServerApi)?&keypad_behavior:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetPropKeypadServerApi=_SiN_GetPropKeypadServerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PropKeypadServerApi* __cdecl
SiN_GetPropKeypadServerApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PropKeypadServerApi)?&prop_key_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetPropPhoneSendApi=_SiN_GetPropPhoneSendApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PhoneSendApi* __cdecl
SiN_GetPropPhoneSendApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PhoneSendApi)?&phone_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetPropPhoneServerApi=_SiN_GetPropPhoneServerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PhoneServerApi* __cdecl
SiN_GetPropPhoneServerApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PhoneServerApi)?&phone_behavior:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetAntigenDispenserApi=_SiN_GetAntigenDispenserApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::DispenserApi* __cdecl
SiN_GetAntigenDispenserApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(DispenserApi)?&dispenser_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetAntigenContainerApi=_SiN_GetAntigenContainerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ContainerApi* __cdecl
SiN_GetAntigenContainerApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ContainerApi)?&container_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetDnaScannerApi=_SiN_GetDnaScannerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::DnaScannerApi* __cdecl
SiN_GetDnaScannerApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(DnaScannerApi)?&dna_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetHealthPickupApi=_SiN_GetHealthPickupApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::HealthPickupApi* __cdecl
SiN_GetHealthPickupApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(HealthPickupApi)?&health_pickup_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetDispenserScreenApi=_SiN_GetDispenserScreenApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::DispenserScreenApi* __cdecl
SiN_GetDispenserScreenApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(DispenserScreenApi)?&dispenser_screen_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetPlayerHealApi=_SiN_GetPlayerHealApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PlayerHealApi* __cdecl
SiN_GetPlayerHealApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PlayerHealApi)?&player_heal_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetAntigenCanisterApi=_SiN_GetAntigenCanisterApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::CanisterApi* __cdecl
SiN_GetAntigenCanisterApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(CanisterApi)?&canister_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetDispenserInputApi=_SiN_GetDispenserInputApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::DispenserInputApi* __cdecl
SiN_GetDispenserInputApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(DispenserInputApi)?&dispenser_input_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetPropContainerApi=_SiN_GetPropContainerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PropContainerApi* __cdecl
SiN_GetPropContainerApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PropContainerApi)?&prop_container_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetPoisonGasApi=_SiN_GetPoisonGasApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PoisonGasApi* __cdecl
SiN_GetPoisonGasApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PoisonGasApi)?&poison_gas_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetPlayerPoisonApi=_SiN_GetPlayerPoisonApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PlayerPoisonApi* __cdecl
SiN_GetPlayerPoisonApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PlayerPoisonApi)?&player_poison_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetGasInputApi=_SiN_GetGasInputApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::GasInputApi* __cdecl
SiN_GetGasInputApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(GasInputApi)?&gas_input_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetPlayerDamageApi=_SiN_GetPlayerDamageApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PlayerDamageApi* __cdecl
SiN_GetPlayerDamageApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PlayerDamageApi)?&player_damage_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetJessicaScriptApi=_SiN_GetJessicaScriptApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::JessicaApi* __cdecl
SiN_GetJessicaScriptApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(JessicaApi)?&jessica_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetJessicaCombatApi=_SiN_GetJessicaCombatApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::JessicaCombatApi* __cdecl
SiN_GetJessicaCombatApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(JessicaCombatApi)?&jessica_combat_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetJessicaAttentionApi=_SiN_GetJessicaAttentionApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::JessicaAttentionApi* __cdecl
SiN_GetJessicaAttentionApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(JessicaAttentionApi)?&jessica_attention_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetCountInterrogationApi=_SiN_GetCountInterrogationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::CountInterrogationApi* __cdecl
SiN_GetCountInterrogationApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(CountInterrogationApi)?&count_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetCharacterResponseApi=_SiN_GetCharacterResponseApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::CharacterResponseApi* __cdecl
SiN_GetCharacterResponseApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(CharacterResponseApi)?&character_response_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetActionMonitorApi=_SiN_GetActionMonitorApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ActionMonitorApi* __cdecl
SiN_GetActionMonitorApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ActionMonitorApi)?&action_monitor_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetRadioMessageApi=_SiN_GetRadioMessageApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::RadioMessageApi* __cdecl
SiN_GetRadioMessageApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(RadioMessageApi)?&radio_message_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetLookEventApi=_SiN_GetLookEventApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::LookEventApi* __cdecl
SiN_GetLookEventApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(LookEventApi)?&look_event_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetActionListenerApi=_SiN_GetActionListenerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ActionListenerApi* __cdecl
SiN_GetActionListenerApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ActionListenerApi)?&action_listener_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetPlayerLookFrameApi=_SiN_GetPlayerLookFrameApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PlayerLookFrameApi* __cdecl
SiN_GetPlayerLookFrameApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PlayerLookFrameApi)?&player_look_frame_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetRadioPlaybackApi=_SiN_GetRadioPlaybackApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::RadioPlaybackApi* __cdecl
SiN_GetRadioPlaybackApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(RadioPlaybackApi)?&radio_playback_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetRadioWireApi=_SiN_GetRadioWireApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::RadioWireApi* __cdecl
SiN_GetRadioWireApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(RadioWireApi)?&radio_wire_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetRadioEntityApi=_SiN_GetRadioEntityApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::RadioEntityApi* __cdecl
SiN_GetRadioEntityApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(RadioEntityApi)?&radio_entity_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetRadioDeliveryApi=_SiN_GetRadioDeliveryApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::RadioDeliveryApi* __cdecl
SiN_GetRadioDeliveryApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(RadioDeliveryApi)?&radio_delivery_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetGenericActorApi=_SiN_GetGenericActorApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::GenericActorApi* __cdecl
SiN_GetGenericActorApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(GenericActorApi)?&generic_actor_api:nullptr;
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
#pragma comment(linker,"/EXPORT:SiN_GetKeypadRecvApi=_SiN_GetKeypadRecvApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::KeypadRecvApi* __cdecl
SiN_GetKeypadRecvApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(KeypadRecvApi)?&keypad_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetKeypadClientApi=_SiN_GetKeypadClientApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::KeypadClientApi* __cdecl
SiN_GetKeypadClientApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(KeypadClientApi)?&keypad_behavior:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetPropKeypadClientApi=_SiN_GetPropKeypadClientApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PropKeypadClientApi* __cdecl
SiN_GetPropKeypadClientApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PropKeypadClientApi)?&prop_key_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetPropPhoneRecvApi=_SiN_GetPropPhoneRecvApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PhoneRecvApi* __cdecl
SiN_GetPropPhoneRecvApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PhoneRecvApi)?&phone_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetPoisonHudApi=_SiN_GetPoisonHudApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PoisonHudApi* __cdecl
SiN_GetPoisonHudApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PoisonHudApi)?&poison_hud_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetScreenCommandApi=_SiN_GetScreenCommandApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ScreenCommandApi* __cdecl
SiN_GetScreenCommandApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ScreenCommandApi)?&screen_command_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetRadioHudApi=_SiN_GetRadioHudApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::RadioHudApi* __cdecl
SiN_GetRadioHudApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(RadioHudApi)?&radio_hud_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetRadioClientRouteApi=_SiN_GetRadioClientRouteApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::RadioClientRouteApi* __cdecl
SiN_GetRadioClientRouteApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(RadioClientRouteApi)?&radio_client_route_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetPoisonHudAnimationApi=_SiN_GetPoisonHudAnimationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PoisonHudAnimationApi* __cdecl
SiN_GetPoisonHudAnimationApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PoisonHudAnimationApi)?&poison_hud_animation_api:nullptr;
}
#endif
