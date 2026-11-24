#include "native_jessica_script.h"
#include "native_count_interrogation.h"
#include "native_npc_dialogue.h"
#include "native_npc_scene_playback.h"
#include "choreography_indexed_rb.h"
#include "native_character_response.h"
#include "native_action_monitor.h"
#include <cstdio>
#include <cstdlib>
#include "native_radio_message.h"
#include "native_action_listener.h"
#include "native_radio_hud.h"
#include "native_radio_entity.h"
#include "native_radio_transport.h"
#include "native_generic_actor.h"
#include "native_companion_reactions.h"
#include "native_count_scheduler.h"
#include "native_use_observation.h"
#include "native_vehicle_use.h"
#include "native_vehicle_actions.h"
#include "native_music_radio.h"
#include "native_music_radio_map.h"
#include <algorithm>
#include <memory>
#include <new>
#include <cmath>
#include "native_vehicle_sound.h"
#include "native_vehicle_hint.h"
#include "native_vehicle_views.h"
#include "native_vehicle_config.h"
#include "native_vehicle_passengers.h"
#include "native_vehicle_map.h"
#include "native_vehicle_lifecycle.h"
#include "native_vehicle_think.h"
#include "native_vehicle_seat_events.h"
#include "native_camera_link.h"
#include "native_vehicle_part_events.h"
#include "native_vehicle_nested.h"
#include "native_npc_map.h"
#include "native_npc_spawn.h"
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
void jessica_ammo_defaults(void* npc) noexcept {
    hologram_write(npc,0x1454,-1);
    hologram_write(npc,0x1450,-1);
}
int __cdecl jessica_spawn_state(HologramControl* ctrl) noexcept {
    if(!health_control(ctrl,jessica_size))return 0;
    jessica_ammo_defaults(ctrl->object);return 1;
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

int __cdecl jessica_interaction(HologramControl* ctrl,const JessicaInteractionBindings* b,unsigned* result) noexcept {
    if(!health_control(ctrl,jessica_size)||!b||!result||!hologram_writable(result,sizeof(*result))||
       !b->speech_manager||!b->resolve||!b->expeditor||!b->eligible||!b->timing||
       !b->player||!b->within_range||!b->suppressed||!b->speak||!b->has_concept||!b->reset_manager)return 0;
    ctrl->busy=1;auto* npc=ctrl->object;const auto h=*b;
    const auto complete=[&](unsigned value){*result=value!=0;return disp_finish(ctrl,true);};
    const auto fail=[&](){return disp_finish(ctrl,false);};
    // 2208ae50(1,1): initialize the manager even if a scalar rejects speech.
    if(!h.speech_manager(h.context))return fail();
    if(hologram_read<unsigned>(npc,0x8c)||(hologram_read<unsigned char>(npc,0xb4)&2)||
       hologram_read<unsigned>(npc,0x850)==4||(hologram_read<unsigned>(npc,0xb8)&0x100000u)||
       hologram_read<unsigned char>(npc,0x968))return complete(0);
    unsigned value=0;
    if(!h.resolve(h.context,hologram_read<std::uint32_t>(npc,0x16c),&value))return fail();
    if(!value)return complete(0);
    void* exp=nullptr;
    if(!h.expeditor(h.context,npc,&exp)||!exp||!h.eligible(h.context,exp,npc,&value))return fail();
    if(!value)return complete(0);
    exp=nullptr;
    if(!h.expeditor(h.context,npc,&exp)||!exp||!h.timing(h.context,exp,&value))return fail();
    if(!value)return complete(0);
    void* player=nullptr;
    if(!h.player(h.context,&player))return fail();
    if(player){
        if(!h.within_range(h.context,npc,player,&value))return fail();
        if(!value)return complete(0);
    }
    if(!h.suppressed(h.context,npc,0,&value))return fail();
    if(value)return complete(0);
    // 2208bee0: successful TLK_USE resets the shared manager but still
    // returns false. An unsuccessful attempt falls through to HELLO/IDLE.
    if(!h.speak(h.context,npc,"TLK_USE",0,&value))return fail();
    if(value){
        if(!h.reset_manager(h.context,-1.0f,0))return fail();
        return complete(0);
    }
    exp=nullptr;
    if(!h.expeditor(h.context,npc,&exp)||!exp||!h.has_concept(h.context,exp,"TLK_HELLO",&value))return fail();
    const char* concept_name=value?"TLK_IDLE":"TLK_HELLO";
    if(!h.speak(h.context,npc,concept_name,0,&value))return fail();
    return complete(value);
}
const JessicaInteractionApi jessica_interaction_api{version,sizeof(JessicaInteractionApi),&jessica_interaction,&health_ack};
int __cdecl response_timing(const void* exp,unsigned bytes,ResponseTiming mode,float now,unsigned* result) noexcept {
    if(!exp||bytes<0x7c||!sin_independent::vgui_layout::Readable(exp,0x7c)||
       !result||!hologram_writable(result,sizeof(*result))||
       (mode!=ResponseTiming::Normal&&mode!=ResponseTiming::Urgent))return 0;
    // 220a3a60 / 220a3aa0. Same-frame equality alone rejects the first
    // predicate (unordered does not). The second compare rejects unordered.
    if(hologram_read<float>(exp,0x78)==now){*result=0;return 1;}
    const float a=hologram_read<float>(exp,mode==ResponseTiming::Urgent?0x6c:0x68);
    const float b=hologram_read<float>(exp,0x70);
    *result=(a>b?a:b)<=now;
    return 1;
}
int __cdecl response_has_concept(void* exp,unsigned bytes,const ResponseHistoryBindings* b,const char* name,unsigned* result) noexcept {
    if(!exp||bytes<0x7c||!sin_independent::vgui_layout::Readable(exp,0x7c)||
       !b||!b->lookup||!b->entry||!name||!result||!hologram_writable(result,sizeof(*result)))return 0;
    const auto h=*b;int index=-1;
    if(!h.lookup(h.context,exp,name,&index)||index<-1)return -1;
    if(index==-1){*result=0;return 1;}
    void* row=nullptr;unsigned n=0;
    if(!h.entry(h.context,exp,index,&row,&n)||!row||n<0x1c||!sin_independent::vgui_layout::Readable(row,0x1c))return -1;
    // 220a6ee0 compares the row+14 timestamp to -1.0f, not zero.
    *result=hologram_read<float>(row,0x14)!=-1.0f;
    return 1;
}
int response_seed_impl(void* exp,unsigned bytes,const ResponseHistoryBindings* b,const char* name,bool notify) noexcept {
    if(!exp||bytes<0x7c||!sin_independent::vgui_layout::Readable(exp,0x7c)||
       !b||!b->lookup||!b->insert||!b->entry||!b->clock||(notify&&!b->notify)||!name)return 0;
    const auto h=*b;int index=-1;
    if(!h.lookup(h.context,exp,name,&index)||index<-1)return -1;
    if(index==-1){
        float initial=0;
        if(!h.clock(h.context,&initial)||!h.insert(h.context,exp,name,initial,&index)||index<0)return -1;
    }
    void* row=nullptr;unsigned n=0;float now=0;
    if(!h.entry(h.context,exp,index,&row,&n)||n<0x1c||!hologram_writable(row,0x1c)||!h.clock(h.context,&now))return -1;
    hologram_write(row,0x14,now);
    // Fixed second argument zero: leave existing response pointer at +18.
    // Third argument one: invoke the owner notification after committing time.
    return !notify||h.notify(h.context,exp,name)?1:-1;
}
int __cdecl response_seed(void* exp,unsigned bytes,const ResponseHistoryBindings* b,const char* name) noexcept {
    return response_seed_impl(exp,bytes,b,name,true);
}
int __cdecl response_seed_silent(void* exp,unsigned bytes,const ResponseHistoryBindings* b,const char* name) noexcept {
    return response_seed_impl(exp,bytes,b,name,false);
}
const ResponsePolicyApi response_policy_api{version,sizeof(ResponsePolicyApi),&response_timing,&response_has_concept,&response_seed};

int response_chance(const void* response) noexcept {
    return (hologram_read<unsigned char>(response,0x88)&4)?hologram_read<int>(response,0x8c):100;
}
int __cdecl response_select(void* exp,unsigned bytes,const ResponseSelectionBindings* b,const char* name,const char* mods,void** result,unsigned* extent) noexcept {
    if(!exp||bytes<0x7c||!sin_independent::vgui_layout::Readable(exp,0x7c)||!name||
       !result||!extent||!hologram_writable(result,sizeof(*result))||!hologram_writable(extent,sizeof(*extent))||
       !b||!b->system||!b->begin_criteria||!b->add||!b->modifiers||!b->npc_criteria||
       !b->player_criteria||!b->allocate||!b->find||!b->draw||!b->destroy||!b->end_criteria)return 0;
    const auto h=*b;void* system=nullptr;
    if(!h.system(h.context,exp,&system))return -1;
    if(!system){*result=nullptr;*extent=0;return 1;}
    void* criteria=nullptr;
    if(!h.begin_criteria(h.context,&criteria)||!criteria)return -1;
    void* response=nullptr;unsigned n=0;
    // All acquired resources on transport failure belong to this call
    // until successfully handed off. Cleanup callbacks are attempted once.
    const auto finish=[&](bool ok,bool keep,bool dispose){
        bool clean=true;
        if(dispose&&response&&!h.destroy(h.context,response))clean=false;
        if(!h.end_criteria(h.context,criteria))clean=false;
        if(!ok||!clean){
            if(keep&&response)(void)h.destroy(h.context,response);
            return -1;
        }
        *result=keep?response:nullptr;*extent=keep?n:0;return 1;
    };
    if(!h.add(h.context,criteria,"concept",name,5.f)||
       (mods&&!h.modifiers(h.context,criteria,mods,5.f))||
       !h.npc_criteria(h.context,exp,criteria)||!h.player_criteria(h.context,criteria,1))
        return finish(false,false,false);
    if(!h.allocate(h.context,0x124,&response,&n)||!response||n<0x124||
       !hologram_writable(response,0x124))return finish(false,false,response!=nullptr);
    // 2205b3a0 constructor scalar fields; embedded opaque data is host-owned.
    hologram_write(response,0,0u);hologram_write(response,4,static_cast<unsigned char>(0));
    hologram_write(response,0x84,0u);hologram_write(response,0x88,0u);hologram_write(response,0x8c,100);
    for(unsigned off=0x90;off<=0xa0;off+=4)hologram_write(response,off,0u);
    hologram_write(response,0xa4,static_cast<unsigned char>(0));
    unsigned found=0;
    if(!h.find(h.context,system,criteria,response,exp,&found))return finish(false,false,true);
    if(!found)return finish(true,false,false); // original finder-false branch
    if(!hologram_read<unsigned char>(response,4))return finish(true,false,true);
    if(response_chance(response)<100){
        int draw=0;
        if(!h.draw(h.context,1,100,&draw)||draw<1||draw>100)return finish(false,false,true);
        // Original re-reads chance AFTER RNG, and rejects equality.
        if(draw<=response_chance(response))return finish(true,false,true);
    }
    return finish(true,true,false);
}
const ResponseSelectApi response_select_api{version,sizeof(ResponseSelectApi),&response_select};

int __cdecl npc_scene_play_scene(void*,const char*,float,const NpcScenePlaybackBindings*,float*) noexcept;
int __cdecl npc_scene_play_sound(void*,const char*,const NpcScenePlaybackBindings*,float*) noexcept;
int __cdecl scene_manager_runtime_query(SceneManagerSession*,const SceneManagerRuntimeBindings*,void*,SceneQueryKind,unsigned,unsigned*) noexcept;
int __cdecl integrated_scene(void*,const char*,float,const NpcScenePlaybackBindings*,const SceneEntityLifecycleBindings*,float*) noexcept;
int __cdecl integrated_sound(void*,const char*,const NpcScenePlaybackBindings*,const SceneEntityLifecycleBindings*,float*) noexcept;
bool response_choreography_ready(const ResponseChoreographyBindings* c) noexcept {
    return c&&c->owner&&c->retain_sound&&c->manager;
}
bool response_choreography_owner(const ResponseChoreographyBindings& c,void* exp,void*& owner) noexcept {
    owner=nullptr;
    return c.owner(c.context,exp,&owner)&&owner&&
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(owner))==hologram_read<std::uint32_t>(exp,4);
}
int __cdecl native_generated_sound(void*,const char*,const NativeGeneratedLifecycleBindings*,float*) noexcept;
int response_dispatch_impl(void* exp,unsigned bytes,const ResponseDispatchBindings* b,const ResponseChoreographyBindings* c,const char* name,
                              void* response,unsigned extent,unsigned* played,const SceneEntityLifecycleBindings* life=nullptr,
                              const NativeGeneratedLifecycleBindings* generated=nullptr) noexcept {
    if(!exp||bytes<0x7c||!hologram_writable(exp,0x7c)||!name||!response||extent<0x124||
       !sin_independent::vgui_layout::Readable(response,0x124)||!played||!hologram_writable(played,sizeof(*played))||
       !b||!b->sample||!b->speaking||!b->force_log||!b->interrupt||
       (c?!response_choreography_ready(c):(!b->stop_scene||!b->scene_blocked||!b->sound||!b->scene))||
       !b->refused_log||!b->sound_duration||!b->emit_raw||!b->note_duration||
       !b->sentence||!b->print_enabled||!b->print||!b->clock||!b->debug_success||!b->destroy||!b->record)return 0;
    const auto h=*b;const auto* text=static_cast<const char*>(response)+4;
    const auto choreography=c?*c:ResponseChoreographyBindings{};
    const auto lifecycle=life?*life:SceneEntityLifecycleBindings{};
    const auto query=[&](SceneQueryKind kind,unsigned* result){
        void* npc=nullptr;
        return response_choreography_owner(choreography,exp,npc)&&
            scene_manager_runtime_query(choreography.manager,&choreography.scenes,npc,kind,1,result)==1;
    };
    // Original inline char buffer ends at +84. Reject a malformed host
    // response without forwarding an unterminated string.
    const auto finish=[&](bool ok){if(!h.destroy(h.context,response)||!ok)return -1;*played=0;return 1;};
    if(!std::memchr(text,0,0x80))return finish(false);
    float delay=0;
    if((hologram_read<unsigned char>(response,0x88)&1)&&
       !h.sample(h.context,static_cast<const unsigned char*>(response)+0x90,&delay))return finish(false);
    const int level=(hologram_read<unsigned char>(response,0x88)&0x10)?hologram_read<int>(response,0xa0):80;
    unsigned busy=0;
    if(!h.speaking(h.context,exp,&busy))return finish(false);
    if(busy&&*name){
        unsigned blocked=0,cancelled=0;
        if(!h.force_log(h.context,exp,name)||
           !(c?query(SceneQueryKind::Cancel,&cancelled):h.stop_scene(h.context,exp,1)!=0)||
           !h.interrupt(h.context,exp)||
           !(c?query(SceneQueryKind::Active,&blocked):h.scene_blocked(h.context,exp,1,&blocked)!=0))return finish(false);
        if(blocked){if(!h.refused_log(h.context,exp,name))return finish(false);return finish(true);}
    }
    const auto type=static_cast<ResponseKind>(hologram_read<unsigned>(response,0));
    float duration=0;bool success=false;
    switch(type){
        case ResponseKind::Speak:
            if(hologram_read<unsigned char>(response,0x88)&0x20){
                if(!h.sound_duration(h.context,exp,text,&duration)||!h.emit_raw(h.context,exp,text,0,0)||
                   !h.note_duration(h.context,exp,duration,delay))return finish(false);
                success=true; // raw path does not test duration
            }else{
                if(c){
                    void* npc=nullptr;const char* retained=nullptr;
                    if(!response_choreography_owner(choreography,exp,npc)||
                       !choreography.retain_sound(choreography.context,text,&retained)||!retained||
                       (generated?native_generated_sound(npc,retained,generated,&duration):
                           life?integrated_sound(npc,retained,&choreography.playback,&lifecycle,&duration):
                           npc_scene_play_sound(npc,retained,&choreography.playback,&duration))!=1)return finish(false);
                }else if(!h.sound(h.context,exp,text,&duration))return finish(false);
                success=duration>0;
                if(success&&!h.note_duration(h.context,exp,duration,delay))return finish(false);
            }
            break;
        case ResponseKind::Sentence:{
            int index=-1;
            if(!h.sentence(h.context,exp,text,delay,1.f,level,0,&index))return finish(false);
            success=index!=-1;
            break;
        }
        case ResponseKind::Scene:
            if(c){
                void* npc=nullptr;
                if(!response_choreography_owner(choreography,exp,npc)||
                   (life?integrated_scene(npc,text,delay,&choreography.playback,&lifecycle,&duration):
                    npc_scene_play_scene(npc,text,delay,&choreography.playback,&duration))!=1)return finish(false);
            }else if(!h.scene(h.context,exp,text,delay,&duration))return finish(false);
            success=duration>0;
            if(success&&!h.note_duration(h.context,exp,duration,delay))return finish(false);
            break;
        case ResponseKind::Print:{
            unsigned enabled=0;
            if(!h.print_enabled(h.context,&enabled))return finish(false);
            if(enabled){if(!h.print(h.context,exp,text))return finish(false);success=true;}
            break;
        }
        default:break; // None, nested Response and out-of-range
    }
    if(!success)return finish(true);
    float now=0;
    if(!h.clock(h.context,&now))return finish(false);
    hologram_write(exp,0x78,now);
    if(type!=ResponseKind::Print&&!h.debug_success(h.context,exp,name,text))return finish(false);
    // Record callback now owns response. No duplicate free on failure.
    if(!h.record(h.context,exp,name,response))return -1;
    *played=1;return 1;
}
int __cdecl response_dispatch(void* exp,unsigned bytes,const ResponseDispatchBindings* b,const char* name,
                              void* response,unsigned extent,unsigned* played) noexcept {
    return response_dispatch_impl(exp,bytes,b,nullptr,name,response,extent,played);
}
int __cdecl response_choreography_dispatch(void* exp,unsigned bytes,const ResponseDispatchBindings* b,const ResponseChoreographyBindings* c,
                                           const char* name,void* response,unsigned extent,unsigned* played) noexcept {
    if(!response_choreography_ready(c))return 0;
    return response_dispatch_impl(exp,bytes,b,c,name,response,extent,played);
}
const ResponseDispatchApi response_dispatch_api{version,sizeof(ResponseDispatchApi),&response_dispatch};
int __cdecl response_speaking(const void* exp,unsigned bytes,float now,unsigned* result) noexcept {
    if(!exp||bytes<0x7c||!sin_independent::vgui_layout::Readable(exp,0x7c)||
       !result||!hologram_writable(result,sizeof(*result)))return 0;
    *result=hologram_read<float>(exp,0x78)==now||hologram_read<float>(exp,0x68)>now;
    return 1;
}
int __cdecl response_note(void* exp,unsigned bytes,SpeechSemaphorePair* state,const ResponseNoteBindings* b,float duration,float delay) noexcept {
    if(!exp||bytes<0x7c||!hologram_writable(exp,0x7c)||!state||!hologram_writable(state,sizeof(*state))||
       !b||!b->owner_start||!b->clock||!b->diagnostic||!b->uses_semaphore||!b->classify||!b->owner_handle)return 0;
    const auto h=*b;
    float total=static_cast<float>(static_cast<double>(duration)+delay);
    if(!h.owner_start(h.context,exp))return -1;
    float now=0;if(!h.clock(h.context,&now))return -1;
    // 220a4590: <=0 replaces total with 0 for logging and uses now+3.
    // Unordered uses normal arithmetic, preserving original NaN propagation.
    float until=0;
    if(total<=0){total=0;until=static_cast<float>(static_cast<double>(now)+3.0);}
    else until=static_cast<float>(static_cast<double>(now)+total);
    hologram_write(exp,0x68,until);
    hologram_write(exp,0x6c,static_cast<float>(static_cast<double>(until)-delay));
    if(!h.diagnostic(h.context,exp,total,delay,until))return -1;
    unsigned uses=0;if(!h.uses_semaphore(h.context,exp,&uses))return -1;
    if(!uses)return 1;
    unsigned classified=0;if(!h.classify(h.context,exp,0,&classified)||!h.clock(h.context,&now))return -1;
    auto& s=classified?state->classified:state->other;
    // Store release before resolving owner, matching original mutation order.
    s.release=static_cast<float>(static_cast<double>(now)+delay);
    std::uint32_t owner=0xffffffffu;if(!h.owner_handle(h.context,exp,&owner))return -1;
    s.owner=owner;return 1;
}
int __cdecl response_interval_sample(const void* interval,unsigned bytes,const ResponseIntervalBindings* b,float* result) noexcept {
    if(!interval||bytes<8||!sin_independent::vgui_layout::Readable(interval,8)||
       !b||!b->random||!result||!hologram_writable(result,sizeof(*result)))return 0;
    const auto base=hologram_read<float>(interval,0),width=hologram_read<float>(interval,4);
    // 22263cf0: zero or unordered width bypasses RNG.
    if(width==0||std::isnan(width)){*result=base;return 1;}
    float offset=0;if(!b->random(b->context,0,width,&offset))return -1;
    *result=static_cast<float>(static_cast<double>(offset)+base);return 1;
}
const ResponseTimingUpdateApi response_timing_update_api{version,sizeof(ResponseTimingUpdateApi),&response_speaking,&response_note,&response_interval_sample};
int __cdecl response_record(void* exp,unsigned bytes,const ResponseRecordBindings* b,const char* name,
                            void* response,unsigned extent,unsigned notify) noexcept {
    if(!exp||bytes<0x7c||!sin_independent::vgui_layout::Readable(exp,0x7c)||!name||
       !response||extent<0x124||!sin_independent::vgui_layout::Readable(response,0x124)||
       !b||!b->destroy||!b->history.lookup||!b->history.insert||!b->history.entry||
       !b->history.clock||!b->history.notify)return 0;
    const auto host=*b;const auto h=host.history;
    const auto ptr=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(response));
    const auto drop=[&](){(void)host.destroy(host.context,response);return -1;};
    int index=-1;
    if(!h.lookup(h.context,exp,name,&index)||index<-1)return drop();
    if(index==-1){
        float initial=0;
        if(!h.clock(h.context,&initial)||!h.insert(h.context,exp,name,initial,&index)||index<0)return drop();
    }
    void* row=nullptr;unsigned n=0;float now=0;
    if(!h.entry(h.context,exp,index,&row,&n)||n<0x1c||!hologram_writable(row,0x1c)||!h.clock(h.context,&now))return drop();
    const auto previous=hologram_read<std::uint32_t>(row,0x18);
    // Original caller selects a fresh object. Refuse an aliased existing
    // history object rather than destroying and retaining a dangling pointer.
    if(previous==ptr)return -1;
    hologram_write(row,0x14,now);
    if(previous){
        const bool ok=host.destroy(host.context,reinterpret_cast<void*>(static_cast<std::uintptr_t>(previous)))!=0;
        if(!ok){
            // Host transport failure has no original equivalent. Destroy
            // consumes on invocation, so invalidate the old word and drop new.
            hologram_write(row,0x18,0u);return drop();
        }
    }
    hologram_write(row,0x18,ptr);
    // Original tests the low byte of the notification argument.
    if((notify&0xff)&&!h.notify(h.context,exp,name))return -1;
    return 1;
}
const ResponseRecordApi response_record_api{version,sizeof(ResponseRecordApi),&response_record};
int response_speak_impl(void* exp,unsigned bytes,const ResponseSpeakBindings* b,const ResponseChoreographyBindings* c,const char* name,const char* mods,unsigned* played,const SceneEntityLifecycleBindings* life=nullptr,
    const NativeGeneratedLifecycleBindings* generated=nullptr) noexcept {
    if(!exp||bytes<0x7c||!hologram_writable(exp,0x7c)||!b||!b->trace_selected||
       !name||!played||!hologram_writable(played,sizeof(*played)))return 0;
    const auto h=*b;void* response=nullptr;unsigned extent=0;
    const auto selected=response_select(exp,bytes,&h.selection,name,mods,&response,&extent);
    if(selected!=1)return selected;
    if(!response){*played=0;return 1;}
    const auto drop=[&](){(void)h.selection.destroy(h.selection.context,response);return -1;};
    if(!h.trace_selected(h.context,exp,name))return drop();
    const auto dispatched=response_dispatch_impl(exp,bytes,&h.dispatch,c,name,response,extent,played,life,generated);
    // A rejected dispatch did not take ownership. Other statuses consumed it.
    return dispatched==0?drop():dispatched;
}
int __cdecl response_speak(void* exp,unsigned bytes,const ResponseSpeakBindings* b,const char* name,const char* mods,unsigned* played) noexcept {
    return response_speak_impl(exp,bytes,b,nullptr,name,mods,played);
}
int __cdecl response_choreography_speak(void* exp,unsigned bytes,const ResponseSpeakBindings* b,const ResponseChoreographyBindings* c,
                                       const char* name,const char* mods,unsigned* played) noexcept {
    if(!response_choreography_ready(c))return 0;
    return response_speak_impl(exp,bytes,b,c,name,mods,played);
}
const ResponseSpeakApi response_speak_api{version,sizeof(ResponseSpeakApi),&response_speak};
const ResponseChoreographyApi response_choreography_api{version,sizeof(ResponseChoreographyApi),&response_choreography_dispatch,&response_choreography_speak};
bool integrated_response_ready(const ResponseChoreographyBindings* c,const SceneEntityLifecycleBindings* life) noexcept {
    return response_choreography_ready(c)&&life&&c->manager==life->manager;
}
int __cdecl integrated_response_dispatch(void* exp,unsigned bytes,const ResponseDispatchBindings* b,const ResponseChoreographyBindings* c,
    const SceneEntityLifecycleBindings* life,const char* name,void* response,unsigned extent,unsigned* played) noexcept {
    if(!integrated_response_ready(c,life))return 0;
    return response_dispatch_impl(exp,bytes,b,c,name,response,extent,played,life);
}
int __cdecl integrated_response_speak(void* exp,unsigned bytes,const ResponseSpeakBindings* b,const ResponseChoreographyBindings* c,
    const SceneEntityLifecycleBindings* life,const char* name,const char* mods,unsigned* played) noexcept {
    if(!integrated_response_ready(c,life))return 0;
    const auto choreography=*c;const auto lifecycle=*life;
    return response_speak_impl(exp,bytes,b,&choreography,name,mods,played,&lifecycle);
}
const IntegratedResponseApi integrated_response_api{version,sizeof(IntegratedResponseApi),&integrated_response_dispatch,&integrated_response_speak};
bool native_generated_response_ready(const NativeGeneratedResponseBindings* b) noexcept {
    return b&&integrated_response_ready(&b->response,&b->generated.lifecycle);
}
int __cdecl native_generated_response_dispatch(void* exp,unsigned bytes,const ResponseDispatchBindings* b,const NativeGeneratedResponseBindings* generated,
    const char* name,void* response,unsigned extent,unsigned* played) noexcept {
    if(!native_generated_response_ready(generated))return 0;
    const auto h=*generated;
    return response_dispatch_impl(exp,bytes,b,&h.response,name,response,extent,played,&h.generated.lifecycle,&h.generated);
}
int __cdecl native_generated_response_speak(void* exp,unsigned bytes,const ResponseSpeakBindings* b,const NativeGeneratedResponseBindings* generated,
    const char* name,const char* mods,unsigned* played) noexcept {
    if(!native_generated_response_ready(generated))return 0;
    const auto h=*generated;
    return response_speak_impl(exp,bytes,b,&h.response,name,mods,played,&h.generated.lifecycle,&h.generated);
}
const NativeGeneratedResponseApi native_generated_response_api{version,sizeof(NativeGeneratedResponseApi),&native_generated_response_dispatch,&native_generated_response_speak};
int response_repeat_row(void* exp,const ResponseRepeatBindings& b,const char* name,void*& row) noexcept {
    int index=-1;
    if(!b.lookup(b.context,exp,name,&index)||index<-1)return -1;
    if(index==-1){row=nullptr;return 1;}
    unsigned extent=0;
    if(!b.entry(b.context,exp,index,&row,&extent)||!row||extent<0x1c||
       !sin_independent::vgui_layout::Readable(row,0x1c))return -1;
    return 1;
}
int __cdecl response_can_repeat(void* exp,unsigned bytes,const ResponseRepeatBindings* b,const char* name,unsigned* out) noexcept {
    if(!exp||bytes<0x7c||!sin_independent::vgui_layout::Readable(exp,0x7c)||!name||!b||
       !b->lookup||!b->entry||!b->resolve||!b->interval.random||!b->clock||
       !out||!hologram_writable(out,sizeof(*out)))return 0;
    const auto h=*b;void* row=nullptr;
    if(response_repeat_row(exp,h,name,row)!=1)return -1;
    if(!row){*out=1;return 1;}
    const auto address=hologram_read<std::uint32_t>(row,0x18);
    if(!address){*out=1;return 1;}
    void* response=nullptr;unsigned extent=0;
    if(!h.resolve(h.context,address,&response,&extent)||!response||extent<0x124||
       !sin_independent::vgui_layout::Readable(response,0x124))return -1;
    const auto flags=hologram_read<unsigned char>(response,0x88);
    if(flags&2){*out=0;return 1;}
    float delay=0;
    if((flags&8)&&response_interval_sample(static_cast<unsigned char*>(response)+0x98,8,&h.interval,&delay)!=1)return -1;
    // Sample BEFORE checking the unspoken sentinel, as in 220a6d00.
    const auto last=hologram_read<float>(row,0x14);
    if(delay==0||last==-1.f){*out=1;return 1;}
    float now=0;if(!h.clock(h.context,&now))return -1;
    // x87 keeps the addition extended until the comparison.
    *out=!(static_cast<double>(last)+static_cast<double>(delay)>static_cast<double>(now));
    return 1;
}
int __cdecl response_last_time(void* exp,unsigned bytes,const ResponseRepeatBindings* b,const char* name,float* out) noexcept {
    if(!exp||bytes<0x7c||!sin_independent::vgui_layout::Readable(exp,0x7c)||!name||!b||
       !b->lookup||!b->entry||!out||!hologram_writable(out,sizeof(*out)))return 0;
    void* row=nullptr;const auto h=*b;
    if(response_repeat_row(exp,h,name,row)!=1)return -1;
    *out=row?hologram_read<float>(row,0x14):-1.f;return 1;
}
int __cdecl response_semaphore_available(void* exp,unsigned bytes,void* npc,const SpeechSemaphorePair* state,
                                          const SpeechGateBindings* b,unsigned* out) noexcept {
    if(!exp||bytes<0x7c||!sin_independent::vgui_layout::Readable(exp,0x7c)||!npc||!state||
       !sin_independent::vgui_layout::Readable(state,sizeof(*state))||!b||!b->uses||!b->classify||!b->clock||!b->resolve||
       !out||!hologram_writable(out,sizeof(*out)))return 0;
    const auto h=*b;unsigned uses=0,classified=0;
    if(!h.uses(h.context,exp,&uses))return -1;
    if(!uses){*out=1;return 1;}
    if(!h.classify(h.context,npc,0,&classified))return -1;
    const auto& sem=classified?state->classified:state->other;
    float now=0;if(!h.clock(h.context,&now))return -1;
    // Strictly later than release: original equality still requires ownership.
    if(now>sem.release){*out=1;return 1;}
    void* owner=nullptr;
    if(sem.owner!=0xffffffffu&&!h.resolve(h.context,sem.owner,&owner))return -1;
    *out=owner==npc;return 1;
}
const ResponseRepeatApi response_repeat_api{version,sizeof(ResponseRepeatApi),&response_can_repeat,&response_last_time,&response_semaphore_available};
bool expeditor_control(const ExpeditorControl* c,ExpeditorStage stage) noexcept {
    return c&&c->stage==stage&&c->bytes>=0x7c&&hologram_writable(c->object,0x7c);
}
bool expeditor_tables(const ExpeditorTables* t) noexcept {
    return t&&t->primary&&t->filter&&t->base&&t->compare;
}
int expeditor_finish(ExpeditorControl* c,bool ok,ExpeditorStage stage) noexcept {
    c->stage=ok?stage:ExpeditorStage::Failed;return ok?1:-1;
}
int __cdecl expeditor_construct(ExpeditorControl* c,void* npc,const ExpeditorTables* t,const ExpeditorLifecycleBindings* b) noexcept {
    if(!expeditor_control(c,ExpeditorStage::Fresh)||!npc||!expeditor_tables(t)||!b||!b->symbol_construct)return 0;
    const auto tables=*t;const auto h=*b;auto* e=c->object;c->stage=ExpeditorStage::Constructing;
    std::memset(e,0,0x7c);
    hologram_write(e,4,static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(npc)));
    hologram_write(e,0,tables.primary);hologram_write(e,8,tables.filter);
    hologram_write(e,0x10,tables.compare);hologram_write(e,0x20,0xffffffffu);hologram_write(e,0x28,0xffffffffu);
    if(!h.symbol_construct(h.context,static_cast<unsigned char*>(e)+0x34,0,0,1))return expeditor_finish(c,false,ExpeditorStage::Live);
    hologram_write(e,0x68,0u);hologram_write(e,0x6c,0u);hologram_write(e,0x70,0u);
    hologram_write(e,0x74,100u);
    return expeditor_finish(c,true,ExpeditorStage::Live);
}
bool expeditor_clear_rows(void* e,const ExpeditorLifecycleBindings& h) noexcept {
    const auto count=hologram_read<int>(e,0x2c);
    // Negative or unrealistically large node extents are not valid tree state.
    if(count<0||count>0x100000)return false;
    std::uint32_t next=0xffffffffu;
    for(int i=count-1;i>=0;--i){
        void* row=nullptr;unsigned extent=0;
        if(!h.row(h.context,e,static_cast<unsigned>(i),&row,&extent)||extent<0x1c||!hologram_writable(row,0x1c))return false;
        const auto index=static_cast<std::uint32_t>(i);
        if(hologram_read<std::uint32_t>(row,0)!=index){
            const auto response=hologram_read<std::uint32_t>(row,0x18);
            if(response){
                const bool ok=h.destroy_response(h.context,response)!=0;
                // Transport consumes on invocation; invalidate pointer on failure.
                hologram_write(row,0x18,0u);if(!ok)return false;
            }else hologram_write(row,0x18,0u);
        }
        hologram_write(row,4,next);hologram_write(row,0,index);next=index;
    }
    hologram_write(e,0x28,count?0u:0xffffffffu);
    hologram_write(e,0x20,0xffffffffu);hologram_write(e,0x24,0u);
    return true;
}
int __cdecl expeditor_clear(ExpeditorControl* c,const ExpeditorLifecycleBindings* b) noexcept {
    if(!expeditor_control(c,ExpeditorStage::Live)||!b||!b->row||!b->destroy_response)return 0;
    const auto h=*b;c->stage=ExpeditorStage::Destroying;
    return expeditor_finish(c,expeditor_clear_rows(c->object,h),ExpeditorStage::Live);
}
int __cdecl expeditor_destroy(ExpeditorControl* c,void* npc,const ExpeditorTables* t,SpeechSemaphorePair* state,const ExpeditorLifecycleBindings* b) noexcept {
    if(!expeditor_control(c,ExpeditorStage::Live)||!npc||!expeditor_tables(t)||!state||!hologram_writable(state,sizeof(*state))||
       !b||!b->row||!b->destroy_response||!b->classify||!b->resolve||!b->symbol_destroy||!b->free_rows)return 0;
    auto* e=c->object;
    if(hologram_read<std::uint32_t>(e,4)!=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(npc)))return 0;
    const auto tables=*t;const auto h=*b;c->stage=ExpeditorStage::Destroying;
    const auto fail=[&](){return expeditor_finish(c,false,ExpeditorStage::Retired);};
    hologram_write(e,0,tables.primary);hologram_write(e,8,tables.filter);
    if(!expeditor_clear_rows(e,h))return fail();
    unsigned classified=0;
    if(!h.classify(h.context,npc,0,&classified))return fail();
    const auto& checked=classified?state->classified:state->other;
    void* owner=nullptr;
    if(checked.owner!=0xffffffffu&&!h.resolve(h.context,checked.owner,&owner))return fail();
    if(owner==npc){
        // Original reclassifies, rather than retaining the first selector.
        if(!h.classify(h.context,npc,0,&classified))return fail();
        auto& release=classified?state->classified:state->other;
        release.release=0;release.owner=0xffffffffu;
    }
    if(!h.symbol_destroy(h.context,static_cast<unsigned char*>(e)+0x34))return fail();
    if(hologram_read<int>(e,0x1c)!=-1){
        const auto rows=hologram_read<std::uint32_t>(e,0x14);
        if(rows){
            const bool ok=h.free_rows(h.context,rows)!=0;
            hologram_write(e,0x14,0u);if(!ok)return fail();
        }
        hologram_write(e,0x18,0u);
    }
    hologram_write(e,0,tables.base);
    return expeditor_finish(c,true,ExpeditorStage::Retired);
}
const ExpeditorLifecycleApi expeditor_lifecycle_api{version,sizeof(ExpeditorLifecycleApi),&expeditor_construct,&expeditor_clear,&expeditor_destroy};
constexpr TalkConceptDescriptor talk_concepts[]={
    {"TLK_ANSWER",0,-1,-1,-1,-1,-1,-1,0},
    {"TLK_QUESTION",0,-1,-1,-1,-1,-1,-1,0},
    {"TLK_IDLE",0,-1,-1,-1,-1,-1,-1,4},
    {"TLK_STARE",0,-1,-1,-1,-1,180,0,4},
    {"TLK_HELLO",0,5,10,-1,-1,-1,-1,7},
    {"TLK_PHELLO",0,-1,-1,-1,-1,-1,-1,7},
    {"TLK_PIDLE",0,-1,-1,-1,-1,-1,-1,0},
    {"TLK_PQUESTION",0,-1,-1,-1,-1,-1,-1,0},
    {"TLK_SMELL",0,-1,-1,-1,-1,-1,-1,0},
    {"TLK_QUESTION_VORT",0,-1,-1,-1,-1,-1,-1,0},
    {"TLK_QUESTION_CIT",0,-1,-1,-1,-1,-1,-1,0},
    {"TLK_ANSWER_VORT",0,-1,-1,-1,-1,-1,-1,0},
    {"TLK_ANSWER_CIT",0,-1,-1,-1,-1,-1,-1,0},
    {"TLK_PLLOOKING",0,-1,-1,-1,-1,-1,-1,0},
    {"TLK_ENEMY_DEAD",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_USE",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_STARTFOLLOW",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_STOPFOLLOW",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_JOINPLAYER",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_STOP",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_NOSHOOT",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_COWER",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_PLHURT1",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_PLHURT2",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_PLHURT3",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_PLPUSH",1,-1,-1,-1,-1,15,30,0},
    {"TLK_PLRELOAD",1,-1,-1,-1,-1,60,0,0},
    {"TLK_SHOT",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_WOUND",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_INPOISONCLOUD",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_MORTAL",1,-1,-1,-1,-1,-1,-1,1},
    {"TLK_SEE_COMBINE",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_ENEMY_DEAD",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_SELECTED",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_COMMANDED",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_COMMAND_FAILED",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_BETRAYED",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_ALLY_KILLED",1,-1,-1,-1,-1,15,30,0},
    {"TLK_ATTACKING",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_HEAL",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_GIVEAMMO",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_PLYR_PHYSATK",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_NEWWEAPON",1,-1,-1,-1,-1,-1,-1,0},
    {"TLK_STARTCOMBAT",1,-1,-1,-1,-1,30,0,0},
    {"TLK_WATCHOUT",1,-1,-1,-1,-1,30,45,0},
    {"TLK_DANGER",2,-1,-1,-1,-1,5,7,0},
    {"TLK_PLDEAD",2,-1,-1,-1,-1,100,0,0},
    {"TLK_HIDEANDRELOAD",2,-1,-1,-1,-1,45,60,0},
};
static_assert(sizeof(talk_concepts)/sizeof(talk_concepts[0])==48);
const TalkConceptDescriptor* __cdecl talk_at(unsigned index) noexcept{return index<48?&talk_concepts[index]:nullptr;}
unsigned char talk_fold(unsigned char c) noexcept{return c>='A'&&c<='Z'?static_cast<unsigned char>(c+32):c;}
const TalkConceptDescriptor* __cdecl talk_lookup(const char* name) noexcept {
    char bounded[128]{};
    if(!prop_key_read(name,bounded))return nullptr;
    for(const auto& d:talk_concepts){
        const char* a=bounded;const char* b=d.name;
        while(*a&&*b&&talk_fold(static_cast<unsigned char>(*a))==talk_fold(static_cast<unsigned char>(*b))){++a;++b;}
        if(!*a&&!*b)return &d;
    }
    return nullptr;
}
bool talk_cooldown_elapsed(float now,float deadline) noexcept {
    return static_cast<double>(now)-deadline>-.001;
}
int talk_group_ready_impl(const void* npc,unsigned bytes,const void* global,const TalkCooldownBindings* b,unsigned priority,unsigned* result) noexcept {
    if(!npc||bytes<0x115c||!sin_independent::vgui_layout::Readable(npc,0x115c)||
       !global||!sin_independent::vgui_layout::Readable(global,12)||!b||!b->clock||priority>2||
       !result||!hologram_writable(result,sizeof(*result)))return 0;
    const auto h=*b;float now=0;
    if(priority!=2){
        if(!h.clock(h.context,&now))return -1;
        if(!talk_cooldown_elapsed(now,hologram_read<float>(global,priority*4))){*result=0;return 1;}
    }
    if(!h.clock(h.context,&now))return -1;
    *result=talk_cooldown_elapsed(now,hologram_read<float>(npc,0x1150+priority*4));return 1;
}
int __cdecl talk_group_ready(const void* npc,unsigned bytes,const TalkCooldownState* state,const TalkCooldownBindings* b,unsigned priority,unsigned* result) noexcept {
    return talk_group_ready_impl(npc,bytes,state?state->global:nullptr,b,priority,result);
}
int __cdecl talk_concept_ready(const TalkCooldownBindings* b,const char* name,unsigned* result) noexcept {
    if(!b||!b->concept_deadline||!b->clock||!name||!result||!hologram_writable(result,sizeof(*result)))return 0;
    const auto h=*b;float* deadline=nullptr;
    if(!h.concept_deadline(h.context,*name?name:nullptr,&deadline))return -1;
    if(!deadline){*result=1;return 1;}
    if(!sin_independent::vgui_layout::Readable(deadline,4))return -1;
    float now=0;if(!h.clock(h.context,&now))return -1;
    *result=talk_cooldown_elapsed(now,*deadline);return 1;
}
bool talk_set_deadline(float& deadline,const TalkCooldownBindings& h,float low,float high) noexcept {
    float delta=low,now=0;
    if(high>0&&!h.random(h.context,low,high,&delta))return false;
    if(!h.clock(h.context,&now))return false;
    deadline=static_cast<float>(static_cast<double>(delta)+now);return true;
}
int talk_note_impl(void* npc,unsigned bytes,void* global,const TalkCooldownBindings* b,const char* name) noexcept {
    if(!npc||bytes<0x115c||!hologram_writable(npc,0x115c)||!global||!hologram_writable(global,12)||
       !b||!b->clock||!b->random||!b->concept_deadline||!b->propagate||!b->cooldown_disabled||!name)return 0;
    const auto h=*b;const auto* d=talk_lookup(name);const unsigned priority=d?d->priority:0;
    if(d&&(d->flags&2)&&!h.propagate(h.context,npc,name))return -1;
    unsigned disabled=0;if(!h.cooldown_disabled(h.context,&disabled))return -1;
    if(disabled)return 1;
    // Original group default row 224e1288: (10,20,0,0),
    // remaining two groups are all zeros.
    if(priority!=2){
        float deadline=0;
        if(d&&d->global_min!=-1){
            if(!talk_set_deadline(deadline,h,d->global_min,d->global_max))return -1;
            hologram_write(global,priority*4,deadline);
        }else if(priority==0){
            if(!talk_set_deadline(deadline,h,10,20))return -1;
            hologram_write(global,0,deadline);
        }
    }
    if(d&&d->npc_min!=-1){
        float deadline=0;
        if(!talk_set_deadline(deadline,h,d->npc_min,d->npc_max))return -1;
        hologram_write(npc,0x1150+priority*4,deadline);
    }
    if(d&&d->concept_min!=-1){
        float* deadline=nullptr;
        if(!h.concept_deadline(h.context,name,&deadline))return -1;
        // Original uses +18 twice, even when +1c differs.
        if(deadline&&(!hologram_writable(deadline,4)||!talk_set_deadline(*deadline,h,d->concept_min,d->concept_min)))return -1;
    }
    return 1;
}
int __cdecl talk_note(void* npc,unsigned bytes,TalkCooldownState* state,const TalkCooldownBindings* b,const char* name) noexcept {
    return talk_note_impl(npc,bytes,state?state->global:nullptr,b,name);
}
int __cdecl talk_reset_interaction(TalkCooldownState* state,const TalkCooldownBindings* b) noexcept {
    if(!state||!hologram_writable(state,sizeof(*state))||!b||!b->clock||!b->random)return 0;
    const auto h=*b;return talk_set_deadline(state->global[0],h,10,20)?1:-1;
}
const TalkCatalogApi talk_catalog_api{version,sizeof(TalkCatalogApi),48,&talk_at,&talk_lookup,&talk_group_ready,&talk_concept_ready,&talk_note,&talk_reset_interaction};
bool companion_talk_preflight(void* npc,unsigned bytes,const CompanionTalkBindings* b,unsigned* out) noexcept {
    return npc&&bytes>=0x1160&&sin_independent::vgui_layout::Readable(npc,0x1160)&&b&&
        b->resolve&&b->expeditor&&b->player&&b->within_range&&b->suppressed&&
        out&&hologram_writable(out,sizeof(*out));
}
bool companion_resolve(const CompanionTalkBindings& h,std::uint32_t packed,void*& p,unsigned& extent) noexcept {
    p=nullptr;extent=0;
    return packed==0xffffffffu||h.resolve(h.context,packed,&p,&extent)!=0;
}
bool companion_expeditor(const CompanionTalkBindings& h,void* npc,void*& exp,unsigned& extent) noexcept {
    exp=nullptr;extent=0;
    return h.expeditor(h.context,npc,&exp,&extent)&&exp&&extent>=0x7c&&sin_independent::vgui_layout::Readable(exp,0x7c);
}
void* managed_talk_globals(const TalkManagerRuntimeBindings&) noexcept;
struct CompanionDeadlineSource {
    const void* legacy;
    const TalkManagerRuntimeBindings* managed;
};
bool companion_deadline_source(const CompanionDeadlineSource& source,const CompanionTalkBindings& h) noexcept {
    return source.managed||(source.legacy&&h.manager);
}
const void* companion_ensure(const CompanionDeadlineSource& source,const CompanionTalkBindings& h) noexcept {
    if(source.managed)return managed_talk_globals(*source.managed);
    return h.manager(h.context)?source.legacy:nullptr;
}
const TalkCooldownBindings& companion_cooldown(const CompanionDeadlineSource& source,const CompanionTalkBindings& h) noexcept {
    return source.managed?source.managed->cooldown:h.cooldown;
}
int companion_group_gate_impl(void* npc,unsigned bytes,const CompanionDeadlineSource& source,const SpeechSemaphorePair* sem,
                                const CompanionTalkBindings* b,unsigned priority,unsigned urgent,unsigned* out) noexcept {
    if(!companion_talk_preflight(npc,bytes,b,out)||priority>2||!sem||
       !companion_deadline_source(source,*b)||!companion_cooldown(source,*b).clock)return 0;
    const auto h=*b;const bool fast=(urgent&0xff)!=0;
    const auto complete=[&](unsigned allowed){*out=allowed!=0;return 1;};
    const void* globals=companion_ensure(source,h);if(!globals)return -1;
    const auto cooldown=companion_cooldown(source,h);
    if(hologram_read<unsigned>(npc,0x8c)||(hologram_read<unsigned char>(npc,0xb4)&2))return complete(0);
    const auto state_id=hologram_read<int>(npc,0x850);
    if(state_id==4||(hologram_read<unsigned>(npc,0xb8)&0x100000)||hologram_read<unsigned char>(npc,0x968))return complete(0);
    unsigned value=0;
    if(!fast){
        if(talk_group_ready_impl(npc,bytes,globals,&cooldown,priority,&value)!=1)return -1;
        if(!value)return complete(0);
    }
    void* associated=nullptr;unsigned extent=0;
    if(!priority){
        if(state_id!=1&&state_id!=2)return complete(0);
        if(!companion_resolve(h,hologram_read<std::uint32_t>(npc,0x115c),associated,extent))return -1;
        if(associated){
            // Original calls 2208abd0 again for the live field read.
            if(!companion_resolve(h,hologram_read<std::uint32_t>(npc,0x115c),associated,extent)||!associated||
               extent<0x3a0||!sin_independent::vgui_layout::Readable(associated,0x3a0))return -1;
            if(hologram_read<float>(associated,0x39c)<-.001)return complete(0);
        }
    }
    if(!companion_resolve(h,hologram_read<std::uint32_t>(npc,0x16c),associated,extent))return -1;
    if(!associated)return complete(0);
    if(priority!=2){
        void* exp=nullptr;unsigned n=0;
        if(!companion_expeditor(h,npc,exp,n)||response_semaphore_available(exp,n,npc,sem,&h.semaphore,&value)!=1)return -1;
        if(!value)return complete(0);
        if(!companion_expeditor(h,npc,exp,n))return -1;
        float now=0;if(!cooldown.clock(cooldown.context,&now)||
            response_timing(exp,n,fast?ResponseTiming::Urgent:ResponseTiming::Normal,now,&value)!=1)return -1;
        if(!value)return complete(0);
        void* player=nullptr;
        if(!h.player(h.context,&player))return -1;
        if(player){if(!h.within_range(h.context,npc,player,&value))return -1;if(!value)return complete(0);}
    }
    if(!h.suppressed(h.context,npc,fast?0:1,&value))return -1;
    return complete(!value);
}
int companion_concept_gate_impl(void* npc,unsigned bytes,const CompanionDeadlineSource& source,const SpeechSemaphorePair* sem,
                                  const CompanionTalkBindings* b,const char* name,unsigned urgent,unsigned* out) noexcept {
    if(!companion_talk_preflight(npc,bytes,b,out)||!name||!companion_deadline_source(source,*b))return 0;
    const auto h=*b;
    if(!companion_ensure(source,h))return -1;
    const auto cooldown=companion_cooldown(source,h);
    const auto* d=talk_lookup(name);unsigned allowed=0;
    const auto grouped=companion_group_gate_impl(npc,bytes,source,sem,&h,d?d->priority:0,urgent,&allowed);
    if(grouped!=1)return grouped;
    if(!allowed){*out=0;return 1;}
    // Original resolves +115c before checking TLK_HELLO, then re-resolves
    // to read +3a0 only when the concept matches case-insensitively.
    void* associated=nullptr;unsigned extent=0;
    if(!companion_resolve(h,hologram_read<std::uint32_t>(npc,0x115c),associated,extent))return -1;
    if(associated&&d==&talk_concepts[4]){
        if(!companion_resolve(h,hologram_read<std::uint32_t>(npc,0x115c),associated,extent)||!associated||
           extent<0x3a1||!sin_independent::vgui_layout::Readable(associated,0x3a1))return -1;
        if(hologram_read<unsigned char>(associated,0x3a0)){*out=0;return 1;}
    }
    if(talk_concept_ready(&cooldown,name,&allowed)!=1)return -1;
    if(!allowed){*out=0;return 1;}
    void* exp=nullptr;unsigned n=0;
    if(d&&(d->flags&1)){
        if(!companion_expeditor(h,npc,exp,n))return -1;
        float last=0;if(response_last_time(exp,n,&h.repeat,name,&last)!=1)return -1;
        if(last!=-1.f){*out=0;return 1;}
    }
    if(!companion_expeditor(h,npc,exp,n)||response_can_repeat(exp,n,&h.repeat,name,&allowed)!=1)return -1;
    *out=allowed!=0;return 1;
}
int companion_request_impl(void* npc,unsigned bytes,const CompanionDeadlineSource& source,const SpeechSemaphorePair* sem,
                              const CompanionTalkBindings* b,const char* name,const char* mods,unsigned urgent,unsigned* out,
                              const ResponseChoreographyBindings* choreography=nullptr,const SceneEntityLifecycleBindings* lifecycle=nullptr,
                              const NativeGeneratedResponseBindings* generated=nullptr) noexcept {
    if(!companion_talk_preflight(npc,bytes,b,out)||!name)return 0;
    const auto h=*b;unsigned allowed=0;
    const auto gate=companion_concept_gate_impl(npc,bytes,source,sem,&h,name,urgent,&allowed);
    if(gate!=1)return gate;
    if(!allowed){*out=0;return 1;}
    void* exp=nullptr;unsigned n=0;
    if(!companion_expeditor(h,npc,exp,n))return -1;
    if((choreography||generated)&&hologram_read<std::uint32_t>(exp,4)!=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(npc)))return -1;
    const auto status=generated?native_generated_response_speak(exp,n,&h.speak,generated,name,mods,out):
        choreography?integrated_response_speak(exp,n,&h.speak,choreography,lifecycle,name,mods,out):
        response_speak(exp,n,&h.speak,name,mods,out);
    return status==0?-1:status;
}
int __cdecl companion_group_gate(void* npc,unsigned bytes,const TalkCooldownState* state,const SpeechSemaphorePair* sem,
                                const CompanionTalkBindings* b,unsigned priority,unsigned urgent,unsigned* out) noexcept {
    return companion_group_gate_impl(npc,bytes,{state?state->global:nullptr,nullptr},sem,b,priority,urgent,out);
}
int __cdecl companion_concept_gate(void* npc,unsigned bytes,const TalkCooldownState* state,const SpeechSemaphorePair* sem,
                                  const CompanionTalkBindings* b,const char* name,unsigned urgent,unsigned* out) noexcept {
    return companion_concept_gate_impl(npc,bytes,{state?state->global:nullptr,nullptr},sem,b,name,urgent,out);
}
int __cdecl companion_request(void* npc,unsigned bytes,const TalkCooldownState* state,const SpeechSemaphorePair* sem,
                              const CompanionTalkBindings* b,const char* name,const char* mods,unsigned urgent,unsigned* out) noexcept {
    return companion_request_impl(npc,bytes,{state?state->global:nullptr,nullptr},sem,b,name,mods,urgent,out);
}
int __cdecl managed_companion_group(void* npc,unsigned bytes,const TalkManagerRuntimeBindings* m,const SpeechSemaphorePair* sem,
                                   const CompanionTalkBindings* b,unsigned priority,unsigned urgent,unsigned* out) noexcept {
    return companion_group_gate_impl(npc,bytes,{nullptr,m},sem,b,priority,urgent,out);
}
int __cdecl managed_companion_concept(void* npc,unsigned bytes,const TalkManagerRuntimeBindings* m,const SpeechSemaphorePair* sem,
                                     const CompanionTalkBindings* b,const char* name,unsigned urgent,unsigned* out) noexcept {
    return companion_concept_gate_impl(npc,bytes,{nullptr,m},sem,b,name,urgent,out);
}
int __cdecl managed_companion_request(void* npc,unsigned bytes,const TalkManagerRuntimeBindings* m,const SpeechSemaphorePair* sem,
                                     const CompanionTalkBindings* b,const char* name,const char* mods,unsigned urgent,unsigned* out) noexcept {
    return companion_request_impl(npc,bytes,{nullptr,m},sem,b,name,mods,urgent,out);
}
const CompanionTalkApi companion_talk_api{version,sizeof(CompanionTalkApi),&companion_group_gate,&companion_concept_gate,&companion_request};
const ManagedCompanionApi managed_companion_api{version,sizeof(ManagedCompanionApi),&managed_companion_group,&managed_companion_concept,&managed_companion_request};
int __cdecl talk_propagate(void* npc,unsigned bytes,const TalkPropagationBindings* b,const char* name) noexcept {
    if(!npc||bytes<0x2c8||!sin_independent::vgui_layout::Readable(npc,0x2c8)||
       !b||!b->list||!b->count||!b->candidate||!b->refresh||!b->visible||!b->expeditor||!name)return 0;
    const auto h=*b;void* token=nullptr;int count=0;
    if(!h.list(h.context,&token)||!h.count(h.context,&count))return -1;
    // Bounded enumeration is a native-host safety adaptation. The original
    // re-reads the list length after every candidate, including skipped ones.
    for(unsigned i=0;count>0&&i<static_cast<unsigned>(count);++i){
        if(!token||count>0x100000||i>=0x100000)return -1;
        void* candidate=nullptr;unsigned extent=0;
        if(!h.candidate(h.context,token,i,&candidate,&extent))return -1;
        if(candidate&&candidate!=npc){
            if(extent<0x2c8||!sin_independent::vgui_layout::Readable(candidate,0x2c8))return -1;
            if((hologram_read<unsigned>(npc,0xb8)&0x800)&&!h.refresh(h.context,npc))return -1;
            if((hologram_read<unsigned>(candidate,0xb8)&0x800)&&!h.refresh(h.context,candidate))return -1;
            // 2208f0d0 stores each subtraction to float before the x87
            // squared-distance comparison (z*z+y*y+x*x).
            const float x=hologram_read<float>(candidate,0x2bc)-hologram_read<float>(npc,0x2bc);
            const float y=hologram_read<float>(candidate,0x2c0)-hologram_read<float>(npc,0x2c0);
            const float z=hologram_read<float>(candidate,0x2c4)-hologram_read<float>(npc,0x2c4);
            const double distance=static_cast<double>(z)*z+static_cast<double>(y)*y+static_cast<double>(x)*x;
            if(distance<1000000.0){
                unsigned visible=0;
                if(!h.visible(h.context,npc,candidate,0x4081,nullptr,&visible))return -1;
                if(visible){
                    void* exp=nullptr;unsigned n=0;const ResponseHistoryBindings* history=nullptr;
                    if(!h.expeditor(h.context,candidate,&exp,&n,&history)||
                       response_seed_impl(exp,n,history,name,false)!=1)return -1;
                }
            }
        }
        if(!h.count(h.context,&count))return -1;
    }
    return 1;
}
const TalkPropagationApi talk_propagation_api{version,sizeof(TalkPropagationApi),&talk_propagate,&response_seed_silent};
bool talk_manager_control(const TalkManagerControl* c,TalkManagerStage stage) noexcept {
    return c&&c->stage==stage&&c->bytes>=talk_manager_bytes&&hologram_writable(c->object,talk_manager_bytes);
}
bool talk_manager_tables(const TalkManagerTables* t) noexcept {
    return t&&t->entity&&t->tree_thunk&&t->compare;
}
bool talk_manager_session(TalkManagerSession* s) noexcept {
    return s&&hologram_writable(s,sizeof(*s));
}
int talk_manager_finish(TalkManagerSession* s,TalkManagerControl* c,bool ok,TalkManagerStage stage) noexcept {
    c->stage=ok?stage:TalkManagerStage::Failed;
    if(!ok)s->blocked=1;
    return ok?1:-1;
}
int __cdecl talk_manager_construct(TalkManagerSession* s,TalkManagerControl* c,const TalkManagerTables* t,
                                  const TalkManagerBindings* b) noexcept {
    if(!talk_manager_session(s)||s->blocked||s->instance||!talk_manager_control(c,TalkManagerStage::Fresh)||
       !talk_manager_tables(t)||!b||!b->base_construct)return 0;
    const auto h=*b;const auto tables=*t;auto* e=c->object;
    c->stage=TalkManagerStage::Constructing;
    if(!h.base_construct(h.context,e,1))return talk_manager_finish(s,c,false,TalkManagerStage::Live);
    // Host must not replace the original singleton during construction.
    if(s->instance)return talk_manager_finish(s,c,false,TalkManagerStage::Live);
    hologram_write(e,0,tables.entity);
    for(unsigned off=0x394;off<=0x39c;off+=4)hologram_write(e,off,-1.f);
    hologram_write(e,0x3a0,tables.tree_thunk);
    for(unsigned off=0x3a4;off<=0x3ac;off+=4)hologram_write(e,off,0u);
    hologram_write(e,0x3b0,0x00ff00ffu);hologram_write(e,0x3b4,0u);hologram_write(e,0x3b8,tables.compare);
    c->stage=TalkManagerStage::Live;s->instance=c;
    return 1;
}
int __cdecl talk_manager_ensure(TalkManagerSession* s,const TalkManagerBindings* b,void** out) noexcept {
    if(!talk_manager_session(s)||s->blocked||s->creating||!b||!b->create||!b->register_created||
       !out||!hologram_writable(out,sizeof(*out)))return 0;
    if(s->instance){
        if(!talk_manager_control(s->instance,TalkManagerStage::Live))return 0;
        *out=s->instance->object;return 1;
    }
    const auto h=*b;s->creating=1;
    const auto fail=[&](){s->creating=0;s->blocked=1;return -1;};
    if(!h.create(h.context,s,"ai_ally_speech_manager",-1)||s->blocked)return fail();
    // Re-read the published singleton, not an unrelated factory return value.
    if(s->instance){
        if(!talk_manager_control(s->instance,TalkManagerStage::Live)||
           !h.register_created(h.context,s->instance->object)||s->blocked)return fail();
    }
    if(s->instance&&!talk_manager_control(s->instance,TalkManagerStage::Live))return fail();
    *out=s->instance?s->instance->object:nullptr;
    s->creating=0;return 1;
}
int __cdecl talk_manager_populate(TalkManagerSession* s,TalkManagerControl* c,const TalkManagerBindings* b) noexcept {
    if(!talk_manager_session(s)||s->blocked||s->instance!=c||!talk_manager_control(c,TalkManagerStage::Live)||
       !b||!b->insert_deadline)return 0;
    const auto h=*b;c->stage=TalkManagerStage::Populating;
    auto* tree=static_cast<unsigned char*>(c->object)+0x3a0;
    for(const auto& d:talk_concepts)
        if(!h.insert_deadline(h.context,tree,d.name,-1.f))return talk_manager_finish(s,c,false,TalkManagerStage::Live);
    return talk_manager_finish(s,c,true,TalkManagerStage::Live);
}
int __cdecl talk_manager_read(const TalkManagerSession* s,TalkCooldownState* out) noexcept {
    if(!s||!sin_independent::vgui_layout::Readable(s,sizeof(*s))||s->blocked||
       !talk_manager_control(s->instance,TalkManagerStage::Live)||!out||!hologram_writable(out,sizeof(*out)))return 0;
    std::memcpy(out->global,static_cast<const unsigned char*>(s->instance->object)+0x394,sizeof(out->global));return 1;
}
int __cdecl talk_manager_write(TalkManagerSession* s,const TalkCooldownState* in) noexcept {
    if(!talk_manager_session(s)||s->blocked||!talk_manager_control(s->instance,TalkManagerStage::Live)||
       !in||!sin_independent::vgui_layout::Readable(in,sizeof(*in)))return 0;
    std::memcpy(static_cast<unsigned char*>(s->instance->object)+0x394,in->global,sizeof(in->global));return 1;
}
int __cdecl talk_manager_destroy(TalkManagerSession* s,TalkManagerControl* c,const TalkManagerTables* t,
                               const TalkManagerBindings* b) noexcept {
    if(!talk_manager_session(s)||s->blocked||s->instance!=c||!talk_manager_control(c,TalkManagerStage::Live)||
       !talk_manager_tables(t)||!b||!b->free_tree||!b->base_destroy)return 0;
    const auto h=*b;auto* e=c->object;
    c->stage=TalkManagerStage::Destroying;hologram_write(e,0,t->entity);s->instance=nullptr;
    if(hologram_read<std::uint32_t>(e,0x3ac)!=0xffffffffu){
        const auto rows=hologram_read<std::uint32_t>(e,0x3a4);
        if(rows){
            const bool ok=h.free_tree(h.context,rows)!=0;
            hologram_write(e,0x3a4,0u);
            if(!ok)return talk_manager_finish(s,c,false,TalkManagerStage::Retired);
        }
        hologram_write(e,0x3a8,0u);
    }
    return talk_manager_finish(s,c,h.base_destroy(h.context,e)!=0,TalkManagerStage::Retired);
}
const TalkManagerApi talk_manager_api{version,sizeof(TalkManagerApi),&talk_manager_construct,&talk_manager_ensure,
    &talk_manager_populate,&talk_manager_read,&talk_manager_write,&talk_manager_destroy};
void* managed_talk_globals(const TalkManagerRuntimeBindings& h) noexcept {
    void* manager=nullptr;
    if(talk_manager_ensure(h.session,&h.manager,&manager)!=1||!manager)return nullptr;
    return static_cast<unsigned char*>(manager)+0x394;
}
int __cdecl managed_talk_notify(const TalkManagerRuntimeBindings* b,void* npc,unsigned bytes,
                                 void* exp,unsigned exp_bytes,const char* name) noexcept {
    if(!b||!name||!npc||bytes<0x115c||!hologram_writable(npc,0x115c)||!exp||
       exp_bytes<0x7c||!sin_independent::vgui_layout::Readable(exp,0x7c)||
       !b->cooldown.clock||!b->cooldown.random||!b->cooldown.concept_deadline||
       !b->cooldown.propagate||!b->cooldown.cooldown_disabled)return 0;
    const auto owner=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(npc));
    if(hologram_read<std::uint32_t>(exp,4)!=owner||hologram_read<std::uint32_t>(exp,0xc)!=owner+0xdfc||
       hologram_read<std::uint32_t>(npc,0xffc)!=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(exp)))return 0;
    const auto h=*b;void* globals=managed_talk_globals(h);
    if(!globals)return -1;
    // The host serializes manager lifetime and the NPC throughout callbacks.
    // Commit each field at its original point, not as a final snapshot copy.
    return talk_note_impl(npc,bytes,globals,&h.cooldown,name)==1?1:-1;
}
int __cdecl managed_talk_reset(const TalkManagerRuntimeBindings* b) noexcept {
    if(!b||!b->cooldown.clock||!b->cooldown.random)return 0;
    const auto h=*b;void* globals=managed_talk_globals(h);
    if(!globals)return -1;
    float deadline=0;if(!talk_set_deadline(deadline,h.cooldown,10,20))return -1;
    hologram_write(globals,0,deadline);return 1;
}
int __cdecl managed_talk_group(const TalkManagerRuntimeBindings* b,const void* npc,unsigned bytes,unsigned priority,unsigned* out) noexcept {
    if(!b||!b->cooldown.clock||!npc||bytes<0x115c||priority>2||!out||!hologram_writable(out,sizeof(*out)))return 0;
    const auto h=*b;void* globals=managed_talk_globals(h);
    if(!globals)return -1;
    return talk_group_ready_impl(npc,bytes,globals,&h.cooldown,priority,out);
}
const TalkManagerRuntimeApi talk_manager_runtime_api{version,sizeof(TalkManagerRuntimeApi),&managed_talk_notify,&managed_talk_reset,&managed_talk_group};
















int __cdecl npc_scene_duration(void* parsed,unsigned bytes,const SceneDurationBindings* b,float* out) noexcept {
    if(!parsed||bytes<0x10||!sin_independent::vgui_layout::Readable(parsed,0x10)||!b||!b->event||
       !out||!hologram_writable(out,sizeof(*out)))return 0;
    const auto h=*b;float maximum=0;
    // Original re-reads vector count after each event; do not cache it.
    for(unsigned i=0;;++i){
        const auto count=hologram_read<int>(parsed,0xc);
        if(count<=0||i>=static_cast<unsigned>(count))break;
        if(count>0x100000)return -1;
        void* e=nullptr;unsigned n=0;
        if(!h.event(h.context,parsed,i,&e,&n)||!e||n<0x18c||
           !sin_independent::vgui_layout::Readable(e,0x18c))return -1;
        const auto time=hologram_read<float>(e,hologram_read<std::uint32_t>(e,0x188)==0xbf800000u?0x184:0x188);
        if(time>maximum)maximum=time;
    }
    *out=maximum;return 1;
}
std::uint32_t scene_word(const void* p) noexcept {
    return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(p));
}
int __cdecl scene_entity_activate(void*,unsigned,const SceneEntityLifecycleBindings*) noexcept;
int scene_entity_activate_impl(void*,unsigned,const SceneEntityLifecycleBindings*,const GeneratedSceneBindings*,const GeneratedSceneAllocationBindings*,const OwnedChoreographyBindings* owned=nullptr) noexcept;
int native_scene_start_impl(void*,unsigned,const SceneStartBindings*,SceneStartResult*,const SceneStopBindings*) noexcept;
int __cdecl scene_actor_attach(void*,unsigned,std::uint32_t,const SceneActorEventBindings*) noexcept;
int __cdecl scene_actor_detach(void*,unsigned,std::uint32_t,const SceneActorEventBindings*) noexcept;
int __cdecl scene_stop_events(void*,unsigned,std::uint32_t,int,const SceneStopBindings*) noexcept;
bool scene_stop_ready(const SceneStopBindings* b) noexcept {
    return b&&b->parsed&&b->actor&&b->events.event&&b->records.storage&&b->records.stop_record;
}
bool scene_start_ready(const SceneStartBindings* b,const SceneStopBindings* stop) noexcept {
    return b&&b->parsed&&b->actor&&b->is_npc&&b->speaking&&b->refresh&&b->output&&b->startup_events&&
        b->reset.events.event&&b->reset.storage&&b->reset.grow&&
        (stop?(scene_stop_ready(stop)&&stop->records.grow):(b->attach&&b->stop_events));
}
int named_scene_register_and_spawn(void*,unsigned,const NativeNamedPlaybackBindings*) noexcept;
int named_scene_activate(void*,unsigned,const SceneEntityLifecycleBindings*) noexcept;
int npc_scene_play(void* npc,const char* name,float delay,const NpcScenePlaybackBindings* b,float* out,bool generated,
                   const SceneEntityLifecycleBindings* lifecycle=nullptr,const GeneratedSceneBindings* generation=nullptr,
                   const GeneratedSceneAllocationBindings* allocation=nullptr,const SceneStartBindings* startup=nullptr,
                   const SceneStopBindings* stop=nullptr,const OwnedChoreographyBindings* owned=nullptr,const NativeNamedPlaybackBindings* named=nullptr) noexcept {
    if(!npc||!name||!b||!b->create||!b->actor_handle||(!lifecycle&&!b->activate)||
       (startup?!scene_start_ready(startup,stop):!b->start)||!b->missing||
       !b->resolve_parsed||!b->duration.event||!out||!hologram_writable(out,sizeof(*out))||
       (generated?!b->spawn_sound:(!b->resolve_name||!b->set_delay||(!named&&(!b->register_scene||!b->set_parsed_flag)))))return 0;
    const auto h=*b;void* entity=nullptr;unsigned bytes=0;
    if(!h.create(h.context,"instanced_scripted_scene",&entity,&bytes)||!entity||
       bytes<0x5e9||!hologram_writable(entity,0x5e9))return -1;
    const auto activate=[&](){return named?named_scene_activate(entity,bytes,lifecycle)==1:lifecycle?scene_entity_activate_impl(entity,bytes,lifecycle,generation,allocation,owned)==1:h.activate(h.context,entity)!=0;};
    auto* buffer=reinterpret_cast<char*>(static_cast<unsigned char*>(entity)+0x568);
    if(generated){
        // Original formats AutoGenerated(%s), then copies at most 128 bytes.
        // Host retains the borrowed name for the scene's lifetime.
        std::snprintf(buffer,128,"AutoGenerated(%s)",name);
    }else if(!h.resolve_name(h.context,npc,name,buffer,128))return -1;
    if(!std::memchr(buffer,0,128))return -1;
    hologram_write(entity,0x394,*buffer?scene_word(buffer):0u);
    std::uint32_t handle=0xffffffffu;
    if(!h.actor_handle(h.context,npc,&handle))return -1;
    hologram_write(entity,0x55c,handle);hologram_write(entity,0x560,static_cast<unsigned char>(1));
    if(generated){
        hologram_write(entity,0x549,static_cast<unsigned char>(1));
        hologram_write(entity,0x54c,*name?scene_word(name):0u);
        if(!h.actor_handle(h.context,npc,&handle))return -1;
        hologram_write(entity,0x550,handle);
        if(!h.spawn_sound(h.context,entity)||!activate())return -1;
    }else{
        if(!h.set_delay(h.context,entity,delay)||
           !(named?named_scene_register_and_spawn(entity,bytes,named)==1:h.register_scene(h.context,entity)!=0)||!activate())return -1;
        const auto parsed=hologram_read<std::uint32_t>(entity,0x40c);
        hologram_write(entity,0x5e8,static_cast<unsigned char>(0));
        if(parsed){
            if(named){
                void* object=nullptr;unsigned n=0;
                if(!h.resolve_parsed(h.context,parsed,&object,&n)||n<0x219||!hologram_writable(object,0x219))return -1;
                hologram_write(object,0x218,static_cast<unsigned char>(0));
            }else if(!h.set_parsed_flag(h.context,parsed,0))return -1;
        }
    }
    if(startup){
        SceneStartResult result{};
        if(native_scene_start_impl(entity,bytes,startup,&result,stop)!=1)return -1;
    }else if(!h.start(h.context,entity))return -1;
    const auto parsed=hologram_read<std::uint32_t>(entity,0x40c);
    if(!parsed){
        if(!h.missing(h.context,generated?name:buffer,generated?1u:0u))return -1;
        *out=0;return 1;
    }
    void* object=nullptr;unsigned n=0;
    if(!h.resolve_parsed(h.context,parsed,&object,&n))return -1;
    return npc_scene_duration(object,n,&h.duration,out)==1?1:-1;
}
int __cdecl npc_scene_play_scene(void* npc,const char* name,float delay,const NpcScenePlaybackBindings* b,float* out) noexcept {
    return npc_scene_play(npc,name,delay,b,out,false);
}
int __cdecl npc_scene_play_sound(void* npc,const char* name,const NpcScenePlaybackBindings* b,float* out) noexcept {
    return npc_scene_play(npc,name,0,b,out,true);
}
const NpcScenePlaybackApi npc_scene_playback_api{version,sizeof(NpcScenePlaybackApi),&npc_scene_duration,&npc_scene_play_scene,&npc_scene_play_sound};
int __cdecl scene_future_speech(void* parsed,unsigned bytes,const SceneDurationBindings* b,unsigned* out) noexcept {
    if(!parsed||bytes<0x7c||!sin_independent::vgui_layout::Readable(parsed,0x7c)||!b||!b->event||
       !out||!hologram_writable(out,sizeof(*out)))return 0;
    const auto h=*b;
    for(unsigned i=0;;++i){
        const auto count=hologram_read<int>(parsed,0xc);
        if(count<=0||i>=static_cast<unsigned>(count))break;
        if(count>0x100000)return -1;
        void* e=nullptr;unsigned n=0;
        if(!h.event(h.context,parsed,i,&e,&n)||!e||n<0x188||!sin_independent::vgui_layout::Readable(e,0x188))return -1;
        if(hologram_read<int>(e,0)==5&&hologram_read<float>(e,0x184)>hologram_read<float>(parsed,0x78)){
            *out=1;return 1;
        }
    }
    *out=0;return 1;
}
int __cdecl scene_contains_actor(void* scene,unsigned bytes,void* npc,const SceneManagerQueryBindings* b,unsigned* out) noexcept {
    if(!scene||bytes<0x410||!sin_independent::vgui_layout::Readable(scene,0x410)||!npc||
       !b||!b->parsed||!b->actor||!out||!hologram_writable(out,sizeof(*out)))return 0;
    const auto h=*b;
    for(unsigned i=0;;++i){
        const auto word=hologram_read<std::uint32_t>(scene,0x40c);
        if(!word){if(i)return -1;break;}
        void* parsed=nullptr;unsigned n=0;
        if(!h.parsed(h.context,word,&parsed,&n)||!parsed||n<0x24||
           !sin_independent::vgui_layout::Readable(parsed,0x24))return -1;
        const int count=hologram_read<int>(parsed,0x20);
        if(count<=0||i>=static_cast<unsigned>(count))break;
        if(count>0x100000)return -1;
        void* actor=nullptr;
        if(!h.actor(h.context,scene,i,&actor))return -1;
        if(actor==npc){*out=1;return 1;}
    }
    *out=0;return 1;
}
int __cdecl scene_manager_query(void* manager,unsigned bytes,void* npc,SceneQueryKind kind,unsigned filter,
                                const SceneManagerQueryBindings* b,unsigned* out) noexcept {
    if(!manager||bytes<0x3a4||!sin_independent::vgui_layout::Readable(manager,0x3a4)||!npc||
       static_cast<unsigned>(kind)>2||!b||!b->entry||!b->resolve||!b->instanced||!b->parsed||!b->actor||
       (kind==SceneQueryKind::Cancel&&!b->cancel)||(kind==SceneQueryKind::FutureSpeech&&!b->events.event)||
       !out||!hologram_writable(out,sizeof(*out)))return 0;
    const auto h=*b;const int count=hologram_read<int>(manager,0x3a0);
    if(count>0x100000)return -1;
    unsigned cancelled=0;
    // Original snapshots count, but reloads the vector base each iteration.
    for(int i=0;i<count;++i){
        std::uint32_t handle=0xffffffffu;
        if(!h.entry(h.context,manager,static_cast<unsigned>(i),&handle))return -1;
        if(handle==0xffffffffu)continue;
        void* scene=nullptr;unsigned n=0;
        if(!h.resolve(h.context,handle,&scene,&n))return -1;
        if(!scene)continue;
        if(n<0x410||!sin_independent::vgui_layout::Readable(scene,0x410))return -1;
        if(kind!=SceneQueryKind::Cancel&&!hologram_read<unsigned char>(scene,0x3d8))continue;
        if(filter&0xff){
            unsigned instance=0;if(!h.instanced(h.context,scene,&instance))return -1;
            if(kind==SceneQueryKind::Cancel?!instance:instance)continue;
        }
        unsigned match=0;
        if(scene_contains_actor(scene,n,npc,&h,&match)!=1)return -1;
        if(!match)continue;
        if(kind==SceneQueryKind::Cancel){
            if(!h.cancel(h.context,scene))return -1;
            ++cancelled;continue;
        }
        if(kind==SceneQueryKind::FutureSpeech){
            const auto word=hologram_read<std::uint32_t>(scene,0x40c);
            if(!word)continue;
            void* parsed=nullptr;unsigned extent=0;
            if(!h.parsed(h.context,word,&parsed,&extent)||scene_future_speech(parsed,extent,&h.events,&match)!=1)return -1;
            if(!match)continue;
        }
        *out=1;return 1;
    }
    *out=cancelled;return 1;
}
const SceneManagerQueryApi scene_manager_query_api{version,sizeof(SceneManagerQueryApi),&scene_contains_actor,&scene_future_speech,&scene_manager_query};
bool scene_manager_resolve(const SceneManagerRuntimeBindings& h,std::uint32_t word,void*& p,unsigned& extent) noexcept {
    p=nullptr;extent=0;
    if(word==0xffffffffu)return true;
    if(!h.resolve(h.context,word,&p,&extent))return false;
    return !p||(extent>=0x3a4&&sin_independent::vgui_layout::Readable(p,0x3a4));
}
int __cdecl scene_manager_ensure(SceneManagerSession* s,const SceneManagerRuntimeBindings* b,void** result,unsigned* bytes) noexcept {
    if(!s||!hologram_writable(s,sizeof(*s))||s->busy||s->blocked||!b||!b->resolve||!b->create||
       !b->packed_handle||!b->spawn||!result||!hologram_writable(result,sizeof(*result))||
       !bytes||!hologram_writable(bytes,sizeof(*bytes)))return 0;
    const auto h=*b;s->busy=1;
    const auto fail=[&](){s->busy=0;s->blocked=1;return -1;};
    if(!(s->initialized&1)){s->initialized|=1;s->handle=0xffffffffu;}
    void* manager=nullptr;unsigned extent=0;
    if(!scene_manager_resolve(h,s->handle,manager,extent))return fail();
    if(!manager){
        void* created=nullptr;
        if(!h.create(h.context,"scene_manager",-1,&created))return fail();
        if(!created)s->handle=0xffffffffu;
        else{
            std::uint32_t word=0xffffffffu;
            if(!h.packed_handle(h.context,created,&word))return fail();
            s->handle=word;
            if(!scene_manager_resolve(h,s->handle,manager,extent))return fail();
            if(manager&&!h.spawn(h.context,manager))return fail();
        }
    }
    // Original re-resolves the cached handle after possible spawn.
    if(!scene_manager_resolve(h,s->handle,manager,extent))return fail();
    *result=manager;*bytes=manager?extent:0;s->busy=0;return 1;
}
int __cdecl scene_manager_runtime_query(SceneManagerSession* s,const SceneManagerRuntimeBindings* b,void* npc,
                                        SceneQueryKind kind,unsigned filter,unsigned* out) noexcept {
    if(!npc||!b||static_cast<unsigned>(kind)>2||!out||!hologram_writable(out,sizeof(*out)))return 0;
    const auto h=*b;void* manager=nullptr;unsigned extent=0;
    const auto status=scene_manager_ensure(s,&h,&manager,&extent);
    if(status!=1)return status;
    if(!manager)return -1; // original wrappers assume nonnull manager
    s->busy=1;
    const auto queried=scene_manager_query(manager,extent,npc,kind,filter,&h.query,out);
    s->busy=0;
    if(queried<0)s->blocked=1;
    return queried;
}
const SceneManagerRuntimeApi scene_manager_runtime_api{version,sizeof(SceneManagerRuntimeApi),&scene_manager_ensure,&scene_manager_runtime_query};
int __cdecl scene_state_pause(void* scene,unsigned bytes) noexcept {
    if(bytes<0x3da||!hologram_writable(scene,0x3da))return 0;
    if(hologram_read<unsigned char>(scene,0x3d8)&&!hologram_read<unsigned char>(scene,0x3d9))
        hologram_write(scene,0x3d9,static_cast<unsigned char>(1));
    return 1;
}
int __cdecl scene_state_resume(void* scene,unsigned bytes,const SceneStateBindings* b) noexcept {
    if(bytes<0x410||!hologram_writable(scene,0x410)||!b||!b->resume_parsed)return 0;
    if(!hologram_read<unsigned char>(scene,0x3d8)||!hologram_read<unsigned char>(scene,0x3d9))return 1;
    const auto parsed=hologram_read<std::uint32_t>(scene,0x40c);
    if(!parsed||!b->resume_parsed(b->context,parsed))return -1;
    hologram_write(scene,0x3d9,static_cast<unsigned char>(0));hologram_write(scene,0x3f4,static_cast<unsigned char>(0));
    return 1;
}
int __cdecl scene_state_complete(void* scene,unsigned bytes,const SceneStateBindings* b) noexcept {
    if(bytes<0x450||!hologram_writable(scene,0x450)||!b||!b->completion||!b->finish)return 0;
    if(!hologram_read<unsigned char>(scene,0x3d8))return 1;
    const auto h=*b;
    hologram_write(scene,0x3d8,static_cast<unsigned char>(0));hologram_write(scene,0x3d9,static_cast<unsigned char>(0));
    if(!h.completion(h.context,static_cast<unsigned char*>(scene)+0x44c,scene,scene,0.f))return -1;
    return h.finish(h.context,scene,1,0)?1:-1;
}
int __cdecl scene_state_wait_tick(void* scene,unsigned bytes,const SceneStateBindings* b) noexcept {
    if(bytes<0x532||!hologram_writable(scene,0x532)||!b||!b->resolve||!b->resume_wait)return 0;
    if(!hologram_read<unsigned char>(scene,0x528))return 1;
    const auto h=*b;const auto handle=hologram_read<std::uint32_t>(scene,0x52c);
    void* target=nullptr;
    if(handle!=0xffffffffu&&!h.resolve(h.context,handle,&target))return -1;
    if(target&&!hologram_read<unsigned char>(scene,0x531))return 1;
    hologram_write(scene,0x528,static_cast<unsigned char>(0));hologram_write(scene,0x52c,0xffffffffu);
    return h.resume_wait(h.context,scene)?1:-1;
}
int __cdecl scene_state_reset_wait(void* scene,unsigned bytes) noexcept {
    if(bytes<0x530||!hologram_writable(scene,0x530))return 0;
    hologram_write(scene,0x524,0u);hologram_write(scene,0x528,static_cast<unsigned char>(0));
    hologram_write(scene,0x52c,0xffffffffu);return 1;
}
const SceneStateApi scene_state_api{version,sizeof(SceneStateApi),&scene_state_pause,&scene_state_resume,&scene_state_complete,&scene_state_wait_tick,&scene_state_reset_wait};
bool scene_manager_control_ready(const SceneManagerControl* c,SceneManagerStage stage) noexcept {
    return c&&c->stage==stage&&!c->ticking&&c->bytes>=scene_manager_bytes&&hologram_writable(c->object,scene_manager_bytes);
}
int __cdecl scene_manager_construct(SceneManagerControl* c,std::uint32_t table,const SceneManagerLifecycleBindings* b) noexcept {
    if(!scene_manager_control_ready(c,SceneManagerStage::Fresh)||!table||!b||!b->base_construct)return 0;
    const auto h=*b;c->stage=SceneManagerStage::Constructing;
    if(!h.base_construct(h.context,c->object,1)){c->stage=SceneManagerStage::Failed;return -1;}
    hologram_write(c->object,0,table);
    for(unsigned off=0x394;off<=0x3b8;off+=4)hologram_write(c->object,off,0u);
    c->stage=SceneManagerStage::Live;return 1;
}
int __cdecl scene_manager_spawn(SceneManagerControl* c,const SceneManagerLifecycleBindings* b) noexcept {
    if(!scene_manager_control_ready(c,SceneManagerStage::Live)||!b||!b->base_spawn||!b->clock||!b->schedule)return 0;
    const auto h=*b;c->ticking=1;float now=0;
    const bool ok=h.base_spawn(h.context,c->object)&&h.clock(h.context,&now)&&h.schedule(h.context,c->object,now,0);
    c->ticking=0;if(!ok)c->stage=SceneManagerStage::Failed;return ok?1:-1;
}
int __cdecl scene_manager_tick(SceneManagerControl* c,const SceneManagerTickBindings* b) noexcept {
    if(!scene_manager_control_ready(c,SceneManagerStage::Live)||!b||!b->clock||!b->schedule||!b->last_think||
       !b->disabled||!b->entry||!b->resolve||!b->advance||!b->shift)return 0;
    const auto h=*b;auto* manager=c->object;c->ticking=1;
    const auto finish=[&](bool ok){c->ticking=0;if(!ok)c->stage=SceneManagerStage::Failed;return ok?1:-1;};
    float now=0,last=0;
    if(!h.clock(h.context,&now)||!h.schedule(h.context,manager,static_cast<float>(static_cast<double>(now)+.001),0)||
       !h.last_think(h.context,manager,0,&last)||!h.clock(h.context,&now))return finish(false);
    const double elapsed=static_cast<double>(now)-last;
    const float dt=static_cast<float>(elapsed>.1?.1:elapsed);
    unsigned disabled=0;if(!h.disabled(h.context,&disabled))return finish(false);
    if(disabled&1)return finish(true);
    int count=hologram_read<int>(manager,0x3a0);
    if(count<0||count>0x100000)return finish(false);
    bool stale=false;
    const auto resolve_at=[&](unsigned i,void*& p){
        std::uint32_t word=0xffffffffu;p=nullptr;
        return h.entry(h.context,manager,i,&word)&&(word==0xffffffffu||h.resolve(h.context,word,&p));
    };
    for(int i=0;i<count;){
        void* scene=nullptr;
        if(!resolve_at(static_cast<unsigned>(i),scene))return finish(false);
        if(!scene){stale=true;++i;continue;}
        if(!h.advance(h.context,scene,dt))return finish(false);
        const int live=hologram_read<int>(manager,0x3a0);
        if(live<0||live>0x100000)return finish(false);
        if(live<count)count=live; // original re-visits the same index
        else ++i; // additions wait for next tick
    }
    if(stale)for(int i=count-1;i>=0;--i){
        void* scene=nullptr;if(!resolve_at(static_cast<unsigned>(i),scene))return finish(false);
        if(scene)continue;
        const int live=hologram_read<int>(manager,0x3a0);
        if(live<=i||live>0x100000)return finish(false);
        const auto trailing=static_cast<unsigned>(live-i-1);
        if(trailing&&!h.shift(h.context,manager,static_cast<unsigned>(i),trailing))return finish(false);
        hologram_write(manager,0x3a0,live-1);
    }
    return finish(true);
}
int __cdecl scene_manager_clear(SceneManagerControl* c) noexcept {
    if(!scene_manager_control_ready(c,SceneManagerStage::Live))return 0;
    hologram_write(c->object,0x3a0,0u);return 1;
}
int __cdecl scene_manager_destroy(SceneManagerControl* c,const SceneManagerLifecycleBindings* b) noexcept {
    if(!scene_manager_control_ready(c,SceneManagerStage::Live)||!b||!b->free_vector||!b->base_destroy)return 0;
    const auto h=*b;auto* m=c->object;c->stage=SceneManagerStage::Destroying;
    for(unsigned base:{0x3a8u,0x394u}){
        hologram_write(m,base+0xc,0u);
        if(hologram_read<std::uint32_t>(m,base+8)!=0xffffffffu){
            const auto allocation=hologram_read<std::uint32_t>(m,base);
            if(allocation){
                const bool ok=h.free_vector(h.context,allocation)!=0;
                hologram_write(m,base,0u);
                if(!ok){c->stage=SceneManagerStage::Failed;return -1;}
            }
            hologram_write(m,base+4,0u);
        }
        hologram_write(m,base+0x10,hologram_read<std::uint32_t>(m,base));
    }
    const bool ok=h.base_destroy(h.context,m)!=0;c->stage=ok?SceneManagerStage::Retired:SceneManagerStage::Failed;return ok?1:-1;
}
const SceneManagerTickApi scene_manager_tick_api{version,sizeof(SceneManagerTickApi),&scene_manager_construct,&scene_manager_spawn,&scene_manager_tick,&scene_manager_clear,&scene_manager_destroy};
bool scene_registration_storage(void* m,const SceneRegistrationBindings& h,unsigned required,void*& storage) noexcept {
    storage=nullptr;
    const auto word=hologram_read<std::uint32_t>(m,0x394);
    unsigned bytes=0;
    return word&&h.storage(h.context,word,&storage,&bytes)&&storage&&bytes>=required*4&&
        hologram_writable(storage,required*4);
}
bool scene_registration_resolve(const SceneRegistrationBindings& h,std::uint32_t word,void*& p) noexcept {
    p=nullptr;return word==0xffffffffu||h.resolve(h.context,word,&p);
}
int scene_registration(void* m,unsigned bytes,const SceneRegistrationBindings* b,void* scene,unsigned* changed,bool add) noexcept {
    if(!m||bytes<0x3a8||!hologram_writable(m,0x3a8)||!b||!b->packed_handle||!b->resolve||!b->storage||
       (add&&!b->grow)||!changed||!hologram_writable(changed,sizeof(*changed)))return 0;
    const auto h=*b;std::uint32_t target=0xffffffffu;
    if(scene&&!h.packed_handle(h.context,scene,&target))return -1;
    const int count=hologram_read<int>(m,0x3a0),capacity=hologram_read<int>(m,0x398);
    if(count<0||capacity<count||count>0x100000||capacity>0x100000)return -1;
    void* storage=nullptr;int found=-1;
    if(count&&!scene_registration_storage(m,h,static_cast<unsigned>(count),storage))return -1;
    for(int i=0;i<count;++i){
        void* wanted=nullptr;void* candidate=nullptr;
        // The original re-resolves target for each entry, even if stale.
        if(!scene_registration_resolve(h,target,wanted)||
           !scene_registration_resolve(h,hologram_read<std::uint32_t>(storage,static_cast<unsigned>(i)*4),candidate))return -1;
        if(wanted==candidate){found=i;break;}
    }
    if(add){
        if(found>=0){*changed=0;return 1;}
        if(count==0x100000)return -1;
        if(count==capacity){
            if(!h.grow(h.context,m,1)||hologram_read<int>(m,0x3a0)!=count||
               hologram_read<int>(m,0x398)<=count||hologram_read<int>(m,0x398)>0x100000)return -1;
        }
        if(!scene_registration_storage(m,h,static_cast<unsigned>(count+1),storage))return -1;
        hologram_write(m,0x3a0,count+1);
        hologram_write(m,0x3a4,hologram_read<std::uint32_t>(m,0x394));
        hologram_write(storage,static_cast<unsigned>(count)*4,target);
    }else{
        if(found<0){*changed=0;return 1;}
        auto* p=static_cast<unsigned char*>(storage)+found*4;
        if(found<count-1)std::memmove(p,p+4,static_cast<unsigned>(count-found-1)*4);
        hologram_write(m,0x3a0,count-1);
    }
    *changed=1;return 1;
}
int __cdecl scene_registration_add(void* m,unsigned bytes,const SceneRegistrationBindings* b,void* s,unsigned* out) noexcept {
    return scene_registration(m,bytes,b,s,out,true);
}
int __cdecl scene_registration_remove(void* m,unsigned bytes,const SceneRegistrationBindings* b,void* s,unsigned* out) noexcept {
    return scene_registration(m,bytes,b,s,out,false);
}
const SceneRegistrationApi scene_registration_api{version,sizeof(SceneRegistrationApi),&scene_registration_add,&scene_registration_remove};
int scene_entity_registration(void* scene,const SceneEntityLifecycleBindings& h,bool add) noexcept {
    void* manager=nullptr;unsigned n=0;
    if(scene_manager_ensure(h.manager,&h.runtime,&manager,&n)!=1)return -1;
    if(!manager)return 1; // original skips the registration on factory absence
    // Original obtains the singleton a second time after nonnull test.
    if(scene_manager_ensure(h.manager,&h.runtime,&manager,&n)!=1||!manager)return -1;
    unsigned changed=0;
    return scene_registration(manager,n,&h.registration,scene,&changed,add)==1?1:-1;
}
// Fresh choreography construction, verified original PE32 field writes.
int __cdecl choreography_parsed_construct(void* object,unsigned bytes,void* callback,const ParsedConstructBindings* b) noexcept {
    if(bytes<0x220||!hologram_writable(object,0x220)||!b||!b->symbol_construct||!b->compare_word)return 0;
    const auto h=*b;
    for(unsigned i=0;i<=0x74;i+=4)hologram_write(object,i,0u);
    for(unsigned i=0x12c;i<=0x13c;i+=4)hologram_write(object,i,0u);
    hologram_write(object,0x140,h.compare_word);
    for(unsigned i=0x144;i<=0x160;i+=4)hologram_write(object,i,i==0x150||i==0x158?0xffffffffu:0u);
    // Constructor invokes symbol table before resetting the simulation state.
    if(!h.symbol_construct(h.context,static_cast<unsigned char*>(object)+0x164,0,0,1))return -1;
    for(unsigned i=0x78;i<=0xa0;i+=4)hologram_write(object,i,i==0x9c?scene_word(callback):0u);
    for(unsigned i:{0xa4u,0x124u,0x125u,0x198u,0x218u})hologram_write(object,i,static_cast<unsigned char>(0));
    hologram_write(object,0x128,60u);hologram_write(object,0x21c,0xffffffffu);
    return 1;
}
int __cdecl choreography_actor_construct(void* object,unsigned bytes) noexcept {
    if(bytes<0x118||!hologram_writable(object,0x118))return 0;
    for(unsigned i=0x100;i<=0x110;i+=4)hologram_write(object,i,0u);
    hologram_write(object,0,static_cast<unsigned char>(0));hologram_write(object,0x80,static_cast<unsigned char>(0));
    hologram_write(object,0x114,static_cast<unsigned char>(1));return 1;
}
int __cdecl choreography_channel_construct(void* object,unsigned bytes) noexcept {
    if(bytes<0x9c||!hologram_writable(object,0x9c))return 0;
    for(unsigned i=0x84;i<=0x94;i+=4)hologram_write(object,i,0u);
    hologram_write(object,0,0u);hologram_write(object,4,static_cast<unsigned char>(0));
    hologram_write(object,0x98,static_cast<unsigned char>(1));return 1;
}
int __cdecl choreography_event_construct(void* object,unsigned bytes,void* parsed,std::uint32_t* counter) noexcept {
    if(bytes<0x3c0||!hologram_writable(object,0x3c0)||!counter||!hologram_writable(counter,4))return 0;
    const auto address=reinterpret_cast<std::uintptr_t>(object),id_address=reinterpret_cast<std::uintptr_t>(counter);
    if(id_address>=address&&id_address-address<0x3c0)return 0;
    for(unsigned i=0x19c;i<=0x1ac;i+=4)hologram_write(object,i,0u);
    for(unsigned i=0x2bc;i<=0x31c;i+=4)hologram_write(object,i,0u);
    for(unsigned i=0x3a4;i<=0x3b4;i+=4)hologram_write(object,i,0u);
    hologram_write(object,0x348,*counter);++*counter;
    for(unsigned i:{0u,0x184u,0x18cu,0x194u,0x2b4u,0x2b8u,0x324u,0x32cu,0x334u,0x338u,0x340u,0x34cu,0x394u,0x398u,0x39cu})
        hologram_write(object,i,0u);
    hologram_write(object,0x188,0xbf800000u);hologram_write(object,0x190,0xffffffffu);
    hologram_write(object,0x330,scene_word(parsed));
    for(unsigned i:{4u,0x84u,0x104u,0x198u,0x1b0u,0x1b1u,0x320u,0x328u,0x33cu,0x390u,0x3a0u,0x3a1u,0x3a2u})
        hologram_write(object,i,static_cast<unsigned char>(0));
    auto* p=static_cast<unsigned char*>(object);
    std::memset(p+0x1b2,0,128);std::memset(p+0x232,0,128);std::memset(p+0x350,0,64);
    return 1;
}
const ChoreographyConstructApi choreography_construct_api{version,sizeof(ChoreographyConstructApi),&choreography_parsed_construct,&choreography_actor_construct,&choreography_channel_construct,&choreography_event_construct};
// Original CUtlVector purge: count first, free only owned memory, update
// mirror, and repeat the allocator purge (normally now a no-op).
bool choreography_purge(void* obj,unsigned off,const ChoreographyDestructionBindings& h) noexcept {
    hologram_write(obj,off+0xc,0u);
    for(unsigned pass=0;pass<2;++pass){
        const auto word=hologram_read<std::uint32_t>(obj,off);
        if(pass)hologram_write(obj,off+0x10,word);
        if(hologram_read<int>(obj,off+8)==-1)continue;
        const bool ok=!word||h.release(h.context,word,ChoreographyRelease::Vector);
        hologram_write(obj,off,0u);hologram_write(obj,off+4,0u);
        if(!ok){hologram_write(obj,off+0x10,0u);return false;}
    }
    return true;
}
int __cdecl choreography_symbol_destroy(void*,unsigned,const ChoreographySymbolBindings*) noexcept;
bool choreography_destroy_ready(const ChoreographyDestructionBindings* h,const ChoreographySymbolBindings* symbols=nullptr) noexcept {
    return h&&h->storage&&h->release&&h->delete_aux&&
        (symbols?(symbols->storage&&symbols->release&&hologram_writable(symbols->sentinel,sizeof(ChoreographySymbolSentinel))):h->symbol_destroy!=nullptr);
}
bool owned_choreography_ready(const OwnedChoreographyBindings* owned) noexcept {
    return owned&&owned->insensitive_comparator&&choreography_destroy_ready(&owned->destruction,&owned->symbols);
}
struct ChoreographyDestroyFrame {const ChoreographyDestroyFrame* parent;std::uint32_t word;unsigned depth;};
bool choreography_destroy_enter(const ChoreographyDestroyFrame* parent,std::uint32_t word) noexcept {
    if(!word||(parent&&parent->depth>=64))return false;
    for(auto* p=parent;p;p=p->parent)if(p->word==word)return false;
    return true;
}
bool choreography_storage(const ChoreographyDestructionBindings& h,std::uint32_t word,unsigned need,void*& ptr) noexcept {
    unsigned extent=0;ptr=nullptr;
    return word&&h.storage(h.context,word,&ptr,&extent)&&extent>=need&&hologram_writable(ptr,need);
}
bool choreography_destroy_parsed(void*,std::uint32_t,const ChoreographyDestructionBindings&,const ChoreographyDestroyFrame*,const ChoreographySymbolBindings* symbols=nullptr) noexcept;
bool choreography_destroy_event(void* event,std::uint32_t word,const ChoreographyDestructionBindings& h,const ChoreographyDestroyFrame* parent,const ChoreographySymbolBindings* symbols=nullptr) noexcept {
    if(!choreography_destroy_enter(parent,word))return false;
    const ChoreographyDestroyFrame frame{parent,word,parent?parent->depth+1:1};
    // 2210e040 repeatedly removes the first auxiliary pointer before
    // invoking its virtual deleting destructor. Null first is malformed.
    for(unsigned iteration=0;;++iteration){
        const auto count=hologram_read<int>(event,0x318);
        if(count<=0)break;
        if(count>0x100000||iteration>=0x100000)return false;
        void* rows=nullptr;
        if(!choreography_storage(h,hologram_read<std::uint32_t>(event,0x30c),static_cast<unsigned>(count)*4,rows))return false;
        const auto aux=hologram_read<std::uint32_t>(rows,0);
        if(!aux)return false;
        if(count>1)std::memmove(rows,static_cast<unsigned char*>(rows)+4,static_cast<unsigned>(count-1)*4);
        hologram_write(event,0x318,count-1);
        if(!h.delete_aux(h.context,aux,1))return false;
    }
    hologram_write(event,0x3b0,0u);
    const auto nested=hologram_read<std::uint32_t>(event,0x324);
    if(nested){
        void* p=nullptr;
        if(!choreography_storage(h,nested,0x220,p)||!choreography_destroy_parsed(p,nested,h,&frame,symbols))return false;
    }
    // Original destructor order, including vectors without active entries.
    for(unsigned off:{0x3a4u,0x30cu,0x2f8u,0x2e4u,0x2d0u,0x2bcu,0x19cu})
        if(!choreography_purge(event,off,h))return false;
    return h.release(h.context,word,ChoreographyRelease::Event)!=0;
}
bool choreography_destroy_parsed(void* parsed,std::uint32_t word,const ChoreographyDestructionBindings& h,const ChoreographyDestroyFrame* parent,const ChoreographySymbolBindings* symbols) noexcept {
    if(!choreography_destroy_enter(parent,word))return false;
    const ChoreographyDestroyFrame frame{parent,word,parent?parent->depth+1:1};
    const unsigned offs[]={0x14,0,0x28},extents[]={0x118,0x3c0,0x9c};
    const ChoreographyRelease kinds[]={ChoreographyRelease::Actor,ChoreographyRelease::Event,ChoreographyRelease::Channel};
    for(unsigned k=0;k<3;++k){
        const auto off=offs[k];
        for(unsigned i=0;;++i){
            const auto count=hologram_read<int>(parsed,off+0xc);
            if(count<=0||i>=static_cast<unsigned>(count))break;
            if(count>0x100000)return false;
            void* rows=nullptr;
            if(!choreography_storage(h,hologram_read<std::uint32_t>(parsed,off),(i+1)*4,rows))return false;
            const auto child=hologram_read<std::uint32_t>(rows,i*4);
            if(!child)continue;
            void* object=nullptr;
            if(!choreography_destroy_enter(&frame,child)||!choreography_storage(h,child,extents[k],object))return false;
            if(k==1){if(!choreography_destroy_event(object,child,h,&frame,symbols))return false;}
            else if(!choreography_purge(object,k==0?0x100:0x84,h)||!h.release(h.context,child,kinds[k]))return false;
        }
        hologram_write(parsed,off+0xc,0u);
    }
    auto* symbol=static_cast<unsigned char*>(parsed)+0x164;
    if(!(symbols?choreography_symbol_destroy(symbol,0x34,symbols)==1:h.symbol_destroy(h.context,symbol)!=0))return false;
    // +144 is CUtlMemory (no count or mirror); remaining are vectors.
    if(hologram_read<int>(parsed,0x14c)!=-1){
        const auto buffer=hologram_read<std::uint32_t>(parsed,0x144);
        const bool ok=!buffer||h.release(h.context,buffer,ChoreographyRelease::Vector);
        hologram_write(parsed,0x144,0u);hologram_write(parsed,0x148,0u);
        if(!ok)return false;
    }
    for(unsigned off:{0x12cu,0x64u,0x50u,0x3cu,0x28u,0x14u,0u})
        if(!choreography_purge(parsed,off,h))return false;
    return h.release(h.context,word,ChoreographyRelease::Parsed)!=0;
}
int __cdecl choreography_delete_parsed(void* parsed,unsigned bytes,const ChoreographyDestructionBindings* h) noexcept {
    if(bytes<0x220||!hologram_writable(parsed,0x220)||!choreography_destroy_ready(h))return 0;
    const auto b=*h;return choreography_destroy_parsed(parsed,scene_word(parsed),b,nullptr)?1:-1;
}
int __cdecl choreography_delete_event(void* event,unsigned bytes,const ChoreographyDestructionBindings* h) noexcept {
    if(bytes<0x3c0||!hologram_writable(event,0x3c0)||!choreography_destroy_ready(h))return 0;
    const auto b=*h;return choreography_destroy_event(event,scene_word(event),b,nullptr)?1:-1;
}
const ChoreographyDestructionApi choreography_destruction_api{version,sizeof(ChoreographyDestructionApi),&choreography_delete_parsed,&choreography_delete_event};
int __cdecl choreography_symbol_construct_empty(void* symbol,unsigned bytes,int grow,std::uint32_t comparator) noexcept {
    if(bytes<0x34||!hologram_writable(symbol,0x34)||!comparator)return 0;
    // Original 2228eab0 with initial=0; +30 is not initialized.
    hologram_write(symbol,0,comparator);
    hologram_write(symbol,4,0u);hologram_write(symbol,8,0u);hologram_write(symbol,0xc,grow);
    hologram_write(symbol,0x10,static_cast<std::uint16_t>(0xffff));
    hologram_write(symbol,0x12,static_cast<std::uint16_t>(0));
    hologram_write(symbol,0x14,static_cast<std::uint16_t>(0xffff));
    hologram_write(symbol,0x16,static_cast<std::uint16_t>(0));
    hologram_write(symbol,0x18,0u);
    for(unsigned off:{0x1cu,0x20u,0x28u,0x2cu})hologram_write(symbol,off,0u);
    hologram_write(symbol,0x24,8u);
    return 1;
}
int __cdecl choreography_symbol_destroy(void* symbol,unsigned bytes,const ChoreographySymbolBindings* b) noexcept {
    if(bytes<0x34||!hologram_writable(symbol,0x34)||!b||!b->storage||!b->release||
       !hologram_writable(b->sentinel,sizeof(ChoreographySymbolSentinel)))return 0;
    const auto h=*b;
    const auto high=hologram_read<std::uint16_t>(symbol,0x16);
    if(high){
        const auto word=hologram_read<std::uint32_t>(symbol,4);
        void* nodes=nullptr;unsigned extent=0;const unsigned need=static_cast<unsigned>(high)*12;
        if(!word||!h.storage(h.context,word,&nodes,&extent)||extent<need||!hologram_writable(nodes,need))return -1;
        if(!(h.sentinel->flags&1)){
            h.sentinel->flags|=1;
            h.sentinel->words[0]=h.sentinel->words[1]=h.sentinel->words[2]=0xffff;
            h.sentinel->words[3]=1;
        }
        // 2228ec50 traverses ALL high-water slots backwards, not live count.
        std::uint16_t next=0xffff;
        for(unsigned i=high;i;){
            --i;hologram_write(nodes,i*12+2,next);hologram_write(nodes,i*12,static_cast<std::uint16_t>(i));next=static_cast<std::uint16_t>(i);
        }
    }
    hologram_write(symbol,0x10,static_cast<std::uint16_t>(0xffff));
    hologram_write(symbol,0x12,static_cast<std::uint16_t>(0));
    hologram_write(symbol,0x14,static_cast<std::uint16_t>(high?0:0xffff));
    // Destructor releases each string before purging its pointer vector.
    for(unsigned i=0;;++i){
        const int count=hologram_read<int>(symbol,0x28);
        if(count<=0||i>=static_cast<unsigned>(count))break;
        if(count>0x100000)return -1;
        const auto word=hologram_read<std::uint32_t>(symbol,0x1c);
        void* data=nullptr;unsigned extent=0;const auto need=(i+1)*4;
        if(!word||!h.storage(h.context,word,&data,&extent)||extent<need||!sin_independent::vgui_layout::Readable(data,need))return -1;
        if(!h.release(h.context,hologram_read<std::uint32_t>(data,i*4),ChoreographySymbolRelease::String))return -1;
    }
    hologram_write(symbol,0x28,0u);
    for(unsigned pass=0;pass<2;++pass){
        const auto word=hologram_read<std::uint32_t>(symbol,0x1c);
        if(pass)hologram_write(symbol,0x2c,word);
        if(hologram_read<int>(symbol,0x24)==-1)continue;
        const bool ok=!word||h.release(h.context,word,ChoreographySymbolRelease::StringVector);
        hologram_write(symbol,0x1c,0u);hologram_write(symbol,0x20,0u);
        if(!ok){hologram_write(symbol,0x2c,0u);return -1;}
    }
    if(hologram_read<int>(symbol,0xc)!=-1){
        const auto word=hologram_read<std::uint32_t>(symbol,4);
        const bool ok=!word||h.release(h.context,word,ChoreographySymbolRelease::Tree);
        hologram_write(symbol,4,0u);hologram_write(symbol,8,0u);
        if(!ok)return -1;
    }
    // Original leaves high-water and tree mirror intact in destructed object.
    return 1;
}
const ChoreographySymbolApi choreography_symbol_api{version,sizeof(ChoreographySymbolApi),&choreography_symbol_construct_empty,&choreography_symbol_destroy};
// 2228f7a0: original first-fit pooled symbol strings and 16-bit tree.
// Allocation and CUtlMemory resizing are external leaves.
int __cdecl choreography_symbol_intern(void* p,void* object,const char* name,std::uint16_t* out) noexcept {
    const auto* b=static_cast<const ChoreographySymbolInternBindings*>(p);
    if(!out)return 0;
    if(!name){*out=0xffff;return 1;}
    if(!b||!b->storage||!b->grow_memory||!b->allocate||!b->release_unpublished||
       !hologram_writable(object,0x34)||!hologram_writable(b->sentinel,sizeof(ChoreographySymbolSentinel)))return 0;
    const auto h=*b;
    using T=sin_port::choreography_index::Tree<std::uint16_t,12>;
    using I=std::uint16_t;
    constexpr unsigned limit=4096;
    unsigned length=0;
    for(;length<65535;++length){
        if(!sin_independent::vgui_layout::Readable(name+length,1))return 0;
        if(!name[length])break;
    }
    if(length==65535)return 0;
    ++length;
    const unsigned high=hologram_read<I>(object,0x16),live=hologram_read<I>(object,0x12);
    const unsigned cap=hologram_read<unsigned>(object,8);
    const unsigned pools=hologram_read<unsigned>(object,0x28),pool_cap=hologram_read<unsigned>(object,0x20);
    if(high>cap||cap>limit||live>high||pools>pool_cap||pool_cap>limit)return 0;
    auto resolve=[&](unsigned off,unsigned need,void*& data) noexcept {
        data=nullptr;if(!need)return true;
        const auto word=hologram_read<std::uint32_t>(object,off);unsigned extent=0;
        return word&&h.storage(h.context,word,&data,&extent)&&extent>=need&&hologram_writable(data,need);
    };
    void *nodes=nullptr,*pool_rows=nullptr;
    if(!resolve(4,high*12,nodes)||!resolve(0x1c,pools*4,pool_rows))return 0;
    struct Pool {unsigned char* data;unsigned capacity,used;};
    Pool blocks[limit];
    unsigned chosen=limit;
    for(unsigned i=0;i<pools;++i){
        void* block=nullptr;unsigned extent=0;
        const auto w=hologram_read<std::uint32_t>(pool_rows,i*4);
        if(!w||!h.storage(h.context,w,&block,&extent)||extent<8||!hologram_writable(block,extent))return 0;
        const auto size=hologram_read<unsigned>(block,0),used=hologram_read<unsigned>(block,4);
        if(size>65535||used>size||extent-8<size)return 0;
        blocks[i]={static_cast<unsigned char*>(block),size,used};
        if(chosen==limit&&size-used>=length)chosen=i;
    }
    T t{static_cast<unsigned char*>(nodes),high,hologram_read<I>(object,0x10)};
    const char* strings[limit]{};
    unsigned occupied=0;
    for(unsigned i=0;i<high;++i){
        if(t.get(static_cast<I>(i),0)==i)continue;
        const auto pool=hologram_read<I>(t.rows,i*12+8),offset=hologram_read<I>(t.rows,i*12+10);
        if(pool>=pools||offset>=blocks[pool].used)return 0;
        const auto* str=reinterpret_cast<const char*>(blocks[pool].data+8+offset);
        if(!std::memchr(str,0,blocks[pool].used-offset))return 0;
        strings[i]=str;++occupied;
    }
    if(occupied!=live)return 0;
    auto compare=[&](const char* a,const char* z) noexcept {
        for(;;++a,++z){
            unsigned char x=static_cast<unsigned char>(*a),y=static_cast<unsigned char>(*z);
            if(h.insensitive){if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;}
            if(x!=y)return x<y?-1:1;
            if(!x)return 0;
        }
    };
    if(!t.valid(live,[&](I a,I z) noexcept {return strings[a]&&strings[z]&&compare(strings[a],strings[z])<0;}))return 0;
    const I free_head=hologram_read<I>(object,0x14);
    unsigned free_count=0;
    for(I f=free_head;f!=T::nil;f=t.get(f,1)){
        if(!t.index(f)||t.get(f,0)!=f||++free_count>high-live)return 0;
    }
    if(free_count!=high-live)return 0;
    I parent=T::nil;unsigned side=0;
    for(I i=t.root;i!=T::nil;i=t.get(i,side)){
        if(!strings[i])return 0;
        const int cmp=compare(name,strings[i]);
        if(!cmp){hologram_write(object,0x30,scene_word(name));*out=i;return 1;}
        parent=i;side=cmp<0?0:1;
    }
    // Reserve tree and pool-pointer capacity before publishing new strings.
    I slot=free_head;
    if(slot==T::nil){
        if(high>=limit)return 0;
        if(high==cap&&(hologram_read<int>(object,0xc)==-1||
           !h.grow_memory(h.context,static_cast<unsigned char*>(object)+4,1,12)||
           hologram_read<unsigned>(object,8)<=high||hologram_read<unsigned>(object,8)>limit))return 0;
        if(!resolve(4,(high+1)*12,nodes))return 0;
        t.rows=static_cast<unsigned char*>(nodes);t.count=high+1;slot=static_cast<I>(high);
    }
    const bool new_pool=chosen==limit;
    if(new_pool){
        if(pools>=limit)return 0;
        if(pools==pool_cap&&(hologram_read<int>(object,0x24)==-1||
           !h.grow_memory(h.context,static_cast<unsigned char*>(object)+0x1c,1,4)||
           hologram_read<unsigned>(object,0x20)<=pools||hologram_read<unsigned>(object,0x20)>limit))return 0;
        if(!resolve(0x1c,(pools+1)*4,pool_rows))return 0;
        const unsigned capacity=length>2048?length:2048;
        void* block=nullptr;unsigned extent=0;
        if(!h.allocate(h.context,capacity+11,&block,&extent)||!block)return 0;
        if(extent<capacity+11||!hologram_writable(block,capacity+11)){
            (void)h.release_unpublished(h.context,scene_word(block));return 0;
        }
        chosen=pools;blocks[chosen]={static_cast<unsigned char*>(block),capacity,0};
        hologram_write(block,0,capacity);hologram_write(block,4,0u);
        hologram_write(pool_rows,pools*4,scene_word(block));
        hologram_write(object,0x28,pools+1);hologram_write(object,0x2c,hologram_read<std::uint32_t>(object,0x1c));
    }
    auto& block=blocks[chosen];
    std::memcpy(block.data+8+block.used,name,length);
    const auto offset=static_cast<I>(block.used);
    hologram_write(block.data,4,block.used+length);
    if(slot==free_head)hologram_write(object,0x14,t.get(slot,1));
    else hologram_write(object,0x16,static_cast<I>(high+1));
    if(parent!=T::nil||free_head!=T::nil){
        if(!(h.sentinel->flags&1)){
            h.sentinel->flags|=1;h.sentinel->words[0]=h.sentinel->words[1]=h.sentinel->words[2]=T::nil;h.sentinel->words[3]=1;
        }
    }
    t.link(slot,parent,side);
    hologram_write(object,0x10,t.root);hologram_write(object,0x12,static_cast<I>(live+1));
    hologram_write(object,0x18,hologram_read<std::uint32_t>(object,4));
    hologram_write(t.rows,slot*12+8,static_cast<I>(chosen));hologram_write(t.rows,slot*12+10,offset);
    // Scratch pointer mirrors original +30; never dereferenced after call.
    hologram_write(object,0x30,scene_word(name));*out=slot;return 1;
}
const ChoreographySymbolInternApi choreography_symbol_intern_api{version,sizeof(ChoreographySymbolInternApi),&choreography_symbol_intern};
// 221a3c30 and its fresh-object setters. Engine allocations, vector
// growth and the original-compatible constructors/destructors remain hosted.
bool generated_scene_text(const char* source,char (&dest)[128]) noexcept {
    if(!source)return false;
    for(unsigned i=0;i<127;++i){
        if(!sin_independent::vgui_layout::Readable(source+i,1))return false;
        dest[i]=source[i];if(!dest[i])return true;
    }
    dest[127]=0;return true;
}
bool generated_scene_ready(const GeneratedSceneBindings* b,const GeneratedSceneAllocationBindings* a,const OwnedChoreographyBindings* owned=nullptr) noexcept {
    if(!b||!b->model_name||!b->duration||!b->warning||!b->scene_name||!b->append||!b->diagnostic_word||
       (owned?(!a||!owned_choreography_ready(owned)):!b->destroy))return false;
    return a?(a->allocate&&a->release_unpublished&&a->append_parsed&&(owned||a->parsed.symbol_construct)&&a->parsed.compare_word&&
        a->event_counter&&hologram_writable(a->event_counter,4)):(b->create_parsed&&b->create_child);
}
bool allocate_generated_parsed(const GeneratedSceneAllocationBindings& a,void* callback,void** out,unsigned* bytes,const OwnedChoreographyBindings* owned) noexcept {
    if(!a.allocate(a.context,0x220,out,bytes))return false;
    if(!*out)return true;
    auto construct=a.parsed;
    if(owned){
        construct.context=const_cast<OwnedChoreographyBindings*>(owned);
        construct.symbol_construct=[](void* p,void* s,int grow,int initial,unsigned insensitive) noexcept -> int{
            const auto& o=*static_cast<const OwnedChoreographyBindings*>(p);
            return !initial&&insensitive==1&&choreography_symbol_construct_empty(s,0x34,grow,o.insensitive_comparator)==1;};
    }
    if(choreography_parsed_construct(*out,*bytes,callback,&construct)==1)return true;
    (void)a.release_unpublished(a.context,*out);*out=nullptr;return false;
}
// Original CUtlMemory growth policy and choreography pointer-vector insert.
// 22107c90/22106f80/22112ae0/2210cf70: initial 8 words, fixed
// growth step or doubling. 22107d00/2210c1d0: initial 3 samples.
// 2210c260/2210c2f0/2210c380: initial 1 tag; 22112c60: 2 scale rows.
// 220f6b20: initial 1 actor event record of stride 0x40.
bool choreography_vector_ready(const ChoreographyVectorBindings* h) noexcept {
    return h&&h->storage&&h->resize;
}
int __cdecl choreography_grow_memory(void* p,void* mem,unsigned extra,unsigned stride) noexcept {
    const auto* b=static_cast<const ChoreographyVectorBindings*>(p);
    if(!choreography_vector_ready(b)||!hologram_writable(mem,12)||!extra||
       (stride!=4&&stride!=12&&stride!=24&&stride!=0x40&&stride!=0x88&&stride!=0x8c))return 0;
    const auto h=*b;
    const unsigned cap=hologram_read<unsigned>(mem,4);
    const auto old=hologram_read<std::uint32_t>(mem,0);
    const int step=hologram_read<int>(mem,8);
    if(cap>4096||extra>4096-cap||step<0||step>4096||(!old&&cap))return 0;
    if(old){
        void* data=nullptr;unsigned extent=0;
        if(!h.storage(h.context,old,&data,&extent)||extent<cap*stride||
           !hologram_writable(data,cap?cap*stride:4))return 0;
    }
    unsigned next=cap;
    while(next<cap+extra){
        if(!next)next=(31+stride)/stride;
        else{
            const unsigned add=step?static_cast<unsigned>(step):next;
            if(add>4096-next)return 0;
            next+=add;
        }
    }
    void* data=nullptr;unsigned extent=0;
    if(!h.resize(h.context,old,next*stride,&data,&extent))return 0;
    // The successful allocator owns the old block and guarantees a valid
    // writable extent. Publish its new pointer even if that contract fails,
    // so the owner can still retire it; never retain a freed old word.
    hologram_write(mem,0,scene_word(data));hologram_write(mem,4,next);
    return extent>=next*stride&&hologram_writable(data,next*stride);
}
int __cdecl choreography_grow_vector(void* p,void* v,unsigned extra,unsigned stride) noexcept {
    if(!hologram_writable(v,20))return 0;
    const int n=hologram_read<int>(v,0xc),cap=hologram_read<int>(v,4);
    if(n<0||cap<n)return 0;
    return choreography_grow_memory(p,v,extra,stride);
}
int __cdecl choreography_insert_word(void* p,void* v,unsigned index,std::uint32_t value) noexcept {
    const auto* b=static_cast<const ChoreographyVectorBindings*>(p);
    if(!choreography_vector_ready(b)||!hologram_writable(v,20))return 0;
    const int n=hologram_read<int>(v,0xc),cap=hologram_read<int>(v,4);
    if(n<0||cap<n||cap>4096||n>=4096||index>static_cast<unsigned>(n))return 0;
    if(n==cap&&!choreography_grow_vector(p,v,1,4))return 0;
    const auto h=*b;void* rows=nullptr;unsigned extent=0;
    const auto word=hologram_read<std::uint32_t>(v,0);
    if(!word||!h.storage(h.context,word,&rows,&extent)||extent<static_cast<unsigned>(n+1)*4||
       !hologram_writable(rows,static_cast<unsigned>(n+1)*4))return 0;
    auto* row=static_cast<unsigned char*>(rows)+index*4;
    if(index<static_cast<unsigned>(n))std::memmove(row+4,row,(static_cast<unsigned>(n)-index)*4);
    hologram_write(row,0,value);hologram_write(v,0x10,word);hologram_write(v,0xc,n+1);
    return 1;
}
int choreography_append_word(void* p,void* obj,unsigned bytes,unsigned off,void* child) noexcept {
    if(!hologram_writable(obj,bytes))return 0;
    auto* v=static_cast<unsigned char*>(obj)+off;
    return choreography_insert_word(p,v,hologram_read<unsigned>(v,0xc),scene_word(child));
}
int __cdecl choreography_append_parsed(void* p,void* parsed,GeneratedSceneChild kind,void* child) noexcept {
    constexpr unsigned offsets[]={0x14,0x28,0};
    const auto i=static_cast<unsigned>(kind);
    return i<3&&choreography_append_word(p,parsed,0x220,offsets[i],child);
}
int __cdecl choreography_append_channel(void* p,void* actor,void* channel) noexcept {
    return choreography_append_word(p,actor,0x118,0x100,channel);
}
int __cdecl choreography_append_event(void* p,void* channel,void* event) noexcept {
    return choreography_append_word(p,channel,0x9c,0x84,event);
}
const ChoreographyVectorApi choreography_vector_api{version,sizeof(ChoreographyVectorApi),
    &choreography_grow_memory,&choreography_grow_vector,&choreography_insert_word,
    &choreography_append_parsed,&choreography_append_channel,&choreography_append_event};
bool allocate_generated_child(const GeneratedSceneAllocationBindings& a,void* parsed,GeneratedSceneChild kind,void** out,unsigned* bytes) noexcept {
    const auto i=static_cast<unsigned>(kind);constexpr unsigned sizes[]={0x118,0x9c,0x3c0};
    if(i>=3||!a.allocate(a.context,sizes[i],out,bytes))return false;
    if(*out){
        const int status=i==0?choreography_actor_construct(*out,*bytes):i==1?choreography_channel_construct(*out,*bytes):
            choreography_event_construct(*out,*bytes,parsed,a.event_counter);
        if(status!=1){(void)a.release_unpublished(a.context,*out);*out=nullptr;return false;}
    }
    if(a.append_parsed(a.context,parsed,kind,*out))return true;
    if(*out)(void)a.release_unpublished(a.context,*out);
    *out=nullptr;return false;
}
int generate_sound_scene_impl(void* scene,unsigned bytes,void* actor,const char* sound,const GeneratedSceneBindings* b,std::uint32_t* out,
    const GeneratedSceneAllocationBindings* allocation,const OwnedChoreographyBindings* owned=nullptr) noexcept {
    if(bytes<0x414||!scene||!sin_independent::vgui_layout::Readable(scene,0x414)||!sound||!out||!hologram_writable(out,4)||
       !generated_scene_ready(b,allocation,owned))return 0;
    const auto allocator=allocation?*allocation:GeneratedSceneAllocationBindings{};
    const auto h=*b;const char* model=nullptr;
    if(actor){if(!h.model_name(h.context,actor,&model))return -1;if(!model)model="";}
    float duration=0;
    if(!h.duration(h.context,sound,model,&duration))return -1;
    // Original x87 parity test rejects ordered <=0, but admits unordered.
    if(duration<=0){
        if(!h.warning(h.context,GeneratedSceneWarning::Duration,sound))return -1;
        *out=0;return 1;
    }
    void* parsed=nullptr;unsigned parsed_bytes=0;
    void* callback=static_cast<unsigned char*>(scene)+0x410;
    if(!(allocation?allocate_generated_parsed(allocator,callback,&parsed,&parsed_bytes,owned):
        h.create_parsed(h.context,callback,&parsed,&parsed_bytes)!=0))return -1;
    if(!parsed){
        if(!h.warning(h.context,GeneratedSceneWarning::SceneAllocation,sound))return -1;
        *out=0;return 1;
    }
    const auto destroy=[&](){return owned?choreography_destroy_parsed(parsed,scene_word(parsed),owned->destruction,nullptr,&owned->symbols):h.destroy(h.context,parsed)!=0;};
    const auto fail=[&](){(void)destroy();return -1;};
    if(parsed_bytes<0x220||!hologram_writable(parsed,0x220)||
       hologram_read<std::uint32_t>(parsed,0x9c)!=scene_word(static_cast<unsigned char*>(scene)+0x410)||
       hologram_read<unsigned char>(parsed,0x125))return fail();
    hologram_write(parsed,0x98,h.diagnostic_word);
    void* children[3]{};unsigned extents[3]{};
    constexpr unsigned sizes[]={0x118,0x9c,0x3c0};
    for(unsigned i=0;i<3;++i)
        if(!(allocation?allocate_generated_child(allocator,parsed,static_cast<GeneratedSceneChild>(i),&children[i],&extents[i]):
            h.create_child(h.context,parsed,static_cast<GeneratedSceneChild>(i),&children[i],&extents[i])!=0))return fail();
    if(!children[0]||!children[1]||!children[2]){
        const bool logged=h.warning(h.context,GeneratedSceneWarning::ChildAllocation,sound)!=0;
        const bool destroyed=destroy();
        if(!logged||!destroyed)return -1;
        *out=0;return 1;
    }
    for(unsigned i=0;i<3;++i)
        if(extents[i]<sizes[i]||!hologram_writable(children[i],sizes[i]))return fail();
    auto* a=static_cast<unsigned char*>(children[0]);auto* c=static_cast<unsigned char*>(children[1]);auto* e=static_cast<unsigned char*>(children[2]);
    // Fresh event constructor binds owning scene, start=0/end=-1, no tags.
    if(hologram_read<std::uint32_t>(e,0x330)!=scene_word(parsed)||hologram_read<std::uint32_t>(e,0x184)||
       hologram_read<std::uint32_t>(e,0x188)!=0xbf800000u||hologram_read<unsigned>(e,0x318))return fail();
    char actor_name[128]="!self",channel_name[128]{},event_name[128]{};
    const auto word=hologram_read<std::uint32_t>(scene,0x394);const char* name="";
    if(word&&(!h.scene_name(h.context,word,&name)||!name))return fail();
    if(!generated_scene_text(name,channel_name)||!generated_scene_text(sound,event_name))return fail();
    std::memcpy(a,actor_name,128);hologram_write(a,0x114,static_cast<unsigned char>(1));
    std::memcpy(c+4,channel_name,128);hologram_write(c,0,scene_word(a));
    if(!h.append(h.context,GeneratedSceneChild::Channel,a,c))return fail();
    hologram_write(e,0,5u);hologram_write(e,0x198,static_cast<unsigned char>(1));
    std::memcpy(e+4,event_name,128);std::memcpy(e+0x84,event_name,128);
    hologram_write(e,0x184,0.f);hologram_write(e,0x1b1,static_cast<unsigned char>(0));
    std::memset(e+0x1b2,0,128);std::memset(e+0x232,0,128);
    // 22110f20 sets duration; 2210cfe0 snaps to frame only when parsed+125
    // is set, which the original fresh scene constructor clears.
    hologram_write(e,0x188,duration);
    if(!h.append(h.context,GeneratedSceneChild::Event,c,e))return fail();
    hologram_write(e,0x2b8,scene_word(c));hologram_write(e,0x2b4,scene_word(a));
    *out=scene_word(parsed);return 1;
}
int __cdecl generate_sound_scene(void* scene,unsigned bytes,void* actor,const char* sound,const GeneratedSceneBindings* b,std::uint32_t* out) noexcept {
    return generate_sound_scene_impl(scene,bytes,actor,sound,b,out,nullptr);
}
int __cdecl allocated_generate(void* scene,unsigned bytes,void* actor,const char* sound,const GeneratedSceneBindings* b,
    const GeneratedSceneAllocationBindings* a,std::uint32_t* out) noexcept {
    return a?generate_sound_scene_impl(scene,bytes,actor,sound,b,out,a):0;
}
const GeneratedSceneApi generated_scene_api{version,sizeof(GeneratedSceneApi),&generate_sound_scene};
int scene_entity_activate_impl(void* scene,unsigned bytes,const SceneEntityLifecycleBindings* b,const GeneratedSceneBindings* generation,
    const GeneratedSceneAllocationBindings* allocation,const OwnedChoreographyBindings* owned) noexcept {
    if(bytes<0x554||!hologram_writable(scene,0x554)||!b||!b->resolve_actor||!b->sound_name||
       (generation?(!allocation||!generated_scene_ready(generation,allocation,owned)):!b->generate)||!b->base_activate||!b->manager)return 0;
    const auto h=*b;
    if(hologram_read<unsigned char>(scene,0x549)&&!hologram_read<std::uint32_t>(scene,0x40c)){
        const auto word=hologram_read<std::uint32_t>(scene,0x54c);
        const char* name="";
        if(word&&(!h.sound_name(h.context,word,&name)||!name))return -1;
        const auto handle=hologram_read<std::uint32_t>(scene,0x550);void* actor=nullptr;
        if(handle!=0xffffffffu&&!h.resolve_actor(h.context,handle,&actor))return -1;
        std::uint32_t parsed=0;
        if(!(generation?generate_sound_scene_impl(scene,bytes,actor,name,generation,&parsed,allocation,owned)==1:
            h.generate(h.context,scene,actor,name,&parsed)!=0))return -1;
        hologram_write(scene,0x40c,parsed);
    }
    if(!h.base_activate(h.context,scene))return -1;
    return scene_entity_registration(scene,h,true);
}
int __cdecl scene_entity_activate(void* scene,unsigned bytes,const SceneEntityLifecycleBindings* b) noexcept {
    return scene_entity_activate_impl(scene,bytes,b,nullptr,nullptr);
}
int __cdecl allocated_activate(void* scene,unsigned bytes,const SceneEntityLifecycleBindings* b,const GeneratedSceneBindings* generation,
    const GeneratedSceneAllocationBindings* allocation) noexcept {
    if(!generation||!allocation)return 0;
    const auto g=*generation;const auto a=*allocation;
    return scene_entity_activate_impl(scene,bytes,b,&g,&a);
}
int __cdecl allocated_sound(void* npc,const char* name,const NpcScenePlaybackBindings* b,const SceneEntityLifecycleBindings* life,
    const GeneratedSceneBindings* generation,const GeneratedSceneAllocationBindings* allocation,float* out) noexcept {
    if(!life||!allocation||!generated_scene_ready(generation,allocation))return 0;
    const auto l=*life;const auto g=*generation;const auto a=*allocation;
    return npc_scene_play(npc,name,0,b,out,true,&l,&g,&a);
}
const AllocatedGeneratedSceneApi allocated_generated_scene_api{version,sizeof(AllocatedGeneratedSceneApi),&allocated_generate,&allocated_activate,&allocated_sound};
int scene_entity_cleanup_impl(void* scene,unsigned bytes,const SceneEntityLifecycleBindings* b,const SceneStopBindings* stop,const ChoreographyDestructionBindings* destruction=nullptr,const ChoreographySymbolBindings* symbols=nullptr) noexcept {
    if(bytes<0x410||!hologram_writable(scene,0x410)||!b||!b->parsed||
       (destruction?!choreography_destroy_ready(destruction,symbols):!b->destroy_parsed)||
       (stop?!scene_stop_ready(stop):(!b->stop_events||!b->actor||!b->detach)))return 0;
    const auto h=*b;auto parsed=hologram_read<std::uint32_t>(scene,0x40c);
    if(parsed){
        if(!(stop?scene_stop_events(scene,bytes,parsed,0,stop)==1:h.stop_events(h.context,scene,parsed,0)!=0))return -1;
        for(unsigned i=0;;++i){
            parsed=hologram_read<std::uint32_t>(scene,0x40c);
            if(!parsed)return -1;
            void* p=nullptr;unsigned n=0;
            if(!h.parsed(h.context,parsed,&p,&n)||!p||n<0x24||!sin_independent::vgui_layout::Readable(p,0x24))return -1;
            const int count=hologram_read<int>(p,0x20);
            if(count<=0||i>=static_cast<unsigned>(count))break;
            if(count>0x100000)return -1;
            void* actor=nullptr;unsigned actor_bytes=0;
            if(!(stop?stop->actor(stop->context,scene,i,&actor,&actor_bytes)!=0:h.actor(h.context,scene,i,&actor)!=0))return -1;
            if(actor){
                const auto word=hologram_read<std::uint32_t>(scene,0x40c);
                if(!(stop?scene_actor_detach(actor,actor_bytes,word,&stop->records)==1:h.detach(h.context,actor,word)!=0))return -1;
            }
        }
    }
    parsed=hologram_read<std::uint32_t>(scene,0x40c);
    if(parsed){
        bool ok=false;
        if(destruction){
            void* object=nullptr;unsigned extent=0;
            if(!h.parsed(h.context,parsed,&object,&extent)||extent<0x220||!hologram_writable(object,0x220))return -1;
            const auto d=*destruction;
            ok=choreography_destroy_parsed(object,parsed,d,nullptr,symbols);
        }else ok=h.destroy_parsed(h.context,parsed)!=0;
        // Ownership is consumed on destruction attempt, not on lookup failure.
        hologram_write(scene,0x40c,0u);
        if(!ok)return -1;
    }else hologram_write(scene,0x40c,0u);
    return 1;
}
int __cdecl scene_entity_cleanup(void* scene,unsigned bytes,const SceneEntityLifecycleBindings* b) noexcept {
    return scene_entity_cleanup_impl(scene,bytes,b,nullptr);
}
int scene_entity_remove_impl(void* scene,unsigned bytes,const SceneEntityLifecycleBindings* b,const SceneStopBindings* stop,const ChoreographyDestructionBindings* destruction=nullptr,const ChoreographySymbolBindings* symbols=nullptr) noexcept {
    if(!b||!b->base_remove||!b->manager)return 0;
    const auto h=*b;const auto clean=scene_entity_cleanup_impl(scene,bytes,&h,stop,destruction,symbols);
    if(clean!=1)return clean;
    if(!h.base_remove(h.context,scene))return -1;
    return scene_entity_registration(scene,h,false);
}
int __cdecl scene_entity_remove(void* scene,unsigned bytes,const SceneEntityLifecycleBindings* b) noexcept {
    return scene_entity_remove_impl(scene,bytes,b,nullptr);
}
const SceneEntityLifecycleApi scene_entity_lifecycle_api{version,sizeof(SceneEntityLifecycleApi),&scene_entity_activate,&scene_entity_cleanup,&scene_entity_remove};
int __cdecl parsed_scene_resume(void* parsed,unsigned bytes,const ParsedSceneResumeBindings* b) noexcept {
    if(!parsed||bytes<0x220||!hologram_writable(parsed,0x220)||!b||!b->event||!b->words||
       !b->allocate||!b->release||!b->start)return 0;
    const auto h=*b;const auto reset=[&](){hologram_write(parsed,0x21c,0xffffffffu);hologram_write(parsed,0x5c,0u);return 1;};
    const int index=hologram_read<int>(parsed,0x21c),count=hologram_read<int>(parsed,0x70);
    if(index<0||index>=count)return reset();
    if(count>0x100000)return -1;
    const void* vector=nullptr;unsigned n=0;
    if(!h.words(h.context,hologram_read<std::uint32_t>(parsed,0x64),&vector,&n)||!vector||
       n<static_cast<unsigned>(count)*4||!sin_independent::vgui_layout::Readable(vector,static_cast<unsigned>(count)*4))return -1;
    void* parent=nullptr;unsigned extent=0;
    if(!h.event(h.context,hologram_read<std::uint32_t>(vector,static_cast<unsigned>(index)*4),&parent,&extent)||
       !parent||extent<0x3b4||!sin_independent::vgui_layout::Readable(parent,0x3b4))return -1;
    const int children=hologram_read<int>(parent,0x3b0);
    if(children<=0)return reset();
    if(children>0x100000)return -1;
    const unsigned needed=static_cast<unsigned>(children)*4;
    const void* source=nullptr;
    if(!h.words(h.context,hologram_read<std::uint32_t>(parent,0x3a4),&source,&n)||!source||
       n<needed||!sin_independent::vgui_layout::Readable(source,needed))return -1;
    void* snapshot=nullptr;unsigned size=0;
    const auto fail=[&](){if(snapshot)(void)h.release(h.context,snapshot);return -1;};
    if(!h.allocate(h.context,needed,&snapshot,&size)||!snapshot||size<needed||
       !hologram_writable(snapshot,needed))return fail();
    // Original copies the child vector before any event callback.
    // Host must keep this temporary storage and source disjoint.
    const auto src=reinterpret_cast<std::uintptr_t>(source),dst=reinterpret_cast<std::uintptr_t>(snapshot);
    if(src<=dst?dst-src<needed:src-dst<needed)return fail();
    std::memcpy(snapshot,source,needed);
    for(int i=0;i<children;++i){
        void* event=nullptr;
        if(!h.event(h.context,hologram_read<std::uint32_t>(snapshot,static_cast<unsigned>(i)*4),&event,&extent)||
           !event||extent<0x329||!hologram_writable(event,0x329))return fail();
        const auto callback=hologram_read<std::uint32_t>(parsed,0x9c);
        const auto time=hologram_read<float>(parsed,0x78);
        hologram_write(event,0x328,static_cast<unsigned char>(1));
        if(callback&&!h.start(h.context,callback,time,parsed,event))return fail();
    }
    if(!h.release(h.context,snapshot))return -1;
    return reset();
}
const ParsedSceneResumeApi parsed_scene_resume_api{version,sizeof(ParsedSceneResumeApi),&parsed_scene_resume};
int __cdecl parsed_event_classify(void* parsed,unsigned bytes,void* event,unsigned extent,float previous,float target,unsigned forward,
                                  const ParsedEventClassifyBindings* b,ParsedEventDecision* out) noexcept {
    if(!parsed||bytes<0x125||!sin_independent::vgui_layout::Readable(parsed,0x125)||!event||extent<0x329||
       !sin_independent::vgui_layout::Readable(event,0x329)||!b||!b->pending||!b->attach||
       !out||!hologram_writable(out,sizeof(*out)))return 0;
    const auto h=*b;const bool ahead=(forward&0xff)!=0;
    const float start=hologram_read<float>(event,0x184);
    const bool has_end=hologram_read<std::uint32_t>(event,0x188)!=0xbf800000u;
    const float end=has_end?hologram_read<float>(event,0x188):start;
    const float lower=ahead?previous:target,upper=ahead?target:previous;
    const auto type=hologram_read<int>(event,0);
    bool suppressed=type==11&&hologram_read<unsigned char>(parsed,0x124);
    if(type==5&&ahead){
        const float latency=hologram_read<float>(parsed,0x90);
        const double delta=static_cast<double>(start)-latency;
        const float window_start=static_cast<float>(delta);
        const float window_end=static_cast<float>(delta+latency);
        for(unsigned i=0;;++i){
            const int count=hologram_read<int>(parsed,0x70);
            if(count<=0||i>=static_cast<unsigned>(count))break;
            if(count>0x100000)return -1;
            void* parent=nullptr;unsigned n=0;
            if(!h.pending(h.context,parsed,i,&parent,&n))return -1;
            if(!parent)continue;
            if(n<0x188||!sin_independent::vgui_layout::Readable(parent,0x188))return -1;
            const float time=hologram_read<float>(parent,0x184);
            if(time>window_end||time<window_start)continue;
            // 22113e00 returns first matching vector entry. Caller then
            // independently compares its start against the window start.
            if(time>=window_start){
                if(!h.attach(h.context,parent,event))return -1;
                suppressed=true;
            }
            break;
        }
    }
    const bool active=hologram_read<unsigned char>(event,0x328)!=0;
    using A=ParsedEventAction;
    if(suppressed){*out={active?A::Stop:A::None,0};return 1;}
    // Original x87 branches reject only ordered greater/less; unordered
    // comparisons take the inclusive interval path.
    const bool inside=!(lower>end)&&!(lower<start);
    if(active){*out={inside?A::Update:A::Stop,inside?1u:0u};return 1;}
    if(inside){*out={hologram_read<unsigned char>(event,0x1b0)?A::StartTracked:A::Start,1};return 1;}
    if(!has_end&&!(start>upper)&&!(start<lower)){*out={A::Start,1};return 1;}
    *out={A::None,0};return 1;
}
const ParsedEventClassifyApi parsed_event_classify_api{version,sizeof(ParsedEventClassifyApi),&parsed_event_classify};
int __cdecl parsed_event_less(const void* a,unsigned na,const void* b,unsigned nb,const ParsedSimulationBindings* h,unsigned* out) noexcept {
    if(!a||!b||na<0x2bc||nb<0x2bc||!sin_independent::vgui_layout::Readable(a,0x2bc)||
       !sin_independent::vgui_layout::Readable(b,0x2bc)||!h||!h->channel_index||
       !out||!hologram_writable(out,sizeof(*out)))return 0;
    const float x=hologram_read<float>(a,0x184),y=hologram_read<float>(b,0x184);
    if(x<y){*out=1;return 1;}if(x>y){*out=0;return 1;}
    if(hologram_read<std::uint32_t>(a,0x188)!=0xbf800000u&&hologram_read<std::uint32_t>(b,0x188)!=0xbf800000u){
        const float e=hologram_read<float>(a,0x188),f=hologram_read<float>(b,0x188);
        if(e>f){*out=1;return 1;}if(e<f){*out=0;return 1;}
    }
    const auto actor=hologram_read<std::uint32_t>(a,0x2b4),other=hologram_read<std::uint32_t>(b,0x2b4);
    const auto c=hologram_read<std::uint32_t>(a,0x2b8),d=hologram_read<std::uint32_t>(b,0x2b8);
    if(actor&&actor==other&&c&&d&&c!=d){
        int i=0,j=0;
        if(!h->channel_index(h->context,actor,c,&i)||!h->channel_index(h->context,actor,d,&j))return -1;
        *out=i<j;return 1;
    }
    const auto* lhs=static_cast<const char*>(a)+4;const auto* rhs=static_cast<const char*>(b)+4;
    if(!std::memchr(lhs,0,128)||!std::memchr(rhs,0,128))return -1;
    *out=std::strcmp(lhs,rhs)<0;return 1;
}
struct ParsedQueueRow {void* event;unsigned extent;ParsedEventAction action;};
struct ParsedQueue {
    ParsedQueueRow* rows=nullptr;unsigned count=0,capacity=0;
    ~ParsedQueue(){delete[] rows;}
    bool insert(ParsedQueueRow value,const ParsedSimulationBindings& h) noexcept {
        if(count==capacity){
            const unsigned next=capacity?capacity*2:16;
            if(next>0x100000)return false;
            auto* p=new(std::nothrow) ParsedQueueRow[next];if(!p)return false;
            if(count)std::memcpy(p,rows,count*sizeof(*rows));delete[] rows;rows=p;capacity=next;
        }
        // Equivalent keys insert after existing keys, as the original
        // tree descends right when less() is false. Valid ordered data only.
        unsigned lo=0,hi=count;
        while(lo<hi){
            const unsigned mid=lo+(hi-lo)/2;unsigned less=0;
            if(parsed_event_less(value.event,value.extent,rows[mid].event,rows[mid].extent,&h,&less)!=1)return false;
            if(less)hi=mid;else lo=mid+1;
        }
        if(lo<count)std::memmove(rows+lo+1,rows+lo,(count-lo)*sizeof(*rows));
        rows[lo]=value;++count;return true;
    }
};
bool parsed_sim_pending(const ParsedSimulationBindings& h,void* parsed,unsigned i,void*& e,unsigned& n) noexcept {
    e=nullptr;n=0;return h.classifier.pending(h.classifier.context,parsed,i,&e,&n)!=0;
}
int __cdecl parsed_scene_simulate(void* parsed,unsigned bytes,float target,const ParsedSimulationBindings* b) noexcept {
    if(!parsed||bytes<0x220||!hologram_writable(parsed,0x220)||!b||!b->classifier.pending||!b->classifier.attach||
       !b->events.event||!b->channel_index||!b->track||!b->callback)return 0;
    const auto h=*b;ParsedQueue queue;
    const double delta=static_cast<double>(target)-hologram_read<float>(parsed,0x78);
    const unsigned forward=delta>=0; // x87 unordered is reverse
    hologram_write(parsed,0x8c,0u);
    const int pending=hologram_read<int>(parsed,0x70);
    if(pending>0x100000)return -1;
    for(int i=0;i<pending;++i){
        void* e=nullptr;unsigned n=0;
        if(!parsed_sim_pending(h,parsed,static_cast<unsigned>(i),e,n)||!e||n<0x3b4||!hologram_writable(e,0x3b4))return -1;
        hologram_write(e,0x3b0,0u);
    }
    for(unsigned i=0;;++i){
        const int count=hologram_read<int>(parsed,0xc);
        if(count<=0||i>=static_cast<unsigned>(count))break;
        if(count>0x100000)return -1;
        void* e=nullptr;unsigned n=0;
        if(!h.events.event(h.events.context,parsed,i,&e,&n))return -1;
        if(!e)continue;
        ParsedEventDecision decision{};
        if(parsed_event_classify(parsed,bytes,e,n,hologram_read<float>(parsed,0x78),target,forward,&h.classifier,&decision)!=1)return -1;
        hologram_write(parsed,0x8c,hologram_read<unsigned>(parsed,0x8c)+decision.active_count);
        if(decision.action!=ParsedEventAction::None&&!queue.insert({e,n,decision.action},h))return -1;
    }
    for(unsigned i=0;i<queue.count;++i){
        const auto row=queue.rows[i];
        if(row.extent<0x329||!hologram_writable(row.event,0x329))return -1;
        const auto cb=hologram_read<std::uint32_t>(parsed,0x9c);
        const auto time=hologram_read<float>(parsed,0x78);
        using A=ParsedEventAction;
        switch(row.action){
        case A::Start:case A::StartTracked:{
            hologram_write(row.event,0x328,static_cast<unsigned char>(1));
            if(cb&&!h.callback(h.context,cb,0,time,parsed,row.event))return -1;
            if(row.action==A::StartTracked&&!h.track(h.context,parsed,row.event))return -1;
            if(hologram_read<int>(row.event,0)==1){
                int index=-1;const int n=hologram_read<int>(parsed,0x70);
                if(n>0x100000)return -1;
                for(int j=0;j<n;++j){
                    void* e=nullptr;unsigned extent=0;
                    if(!parsed_sim_pending(h,parsed,static_cast<unsigned>(j),e,extent))return -1;
                    if(e==row.event){index=j;break;}
                }
                hologram_write(parsed,0x21c,index);
            }
            break;
        }
        case A::Update:if(cb&&!h.callback(h.context,cb,8,time,parsed,row.event))return -1;break;
        case A::Stop:
            if(cb&&!h.callback(h.context,cb,4,time,parsed,row.event))return -1;
            hologram_write(row.event,0x328,static_cast<unsigned char>(0));break;
        default:break;
        }
    }
    hologram_write(parsed,0x78,target);
    if(hologram_read<unsigned>(parsed,0x8c))hologram_write(parsed,0x94,target);
    return 1;
}
const ParsedSceneSimulationApi parsed_scene_simulation_api{version,sizeof(ParsedSceneSimulationApi),&parsed_scene_simulate,&parsed_event_less};
int __cdecl choreography_event_reset(void* event,unsigned bytes) noexcept {
    if(bytes<0x329||!hologram_writable(event,0x329))return 0;
    hologram_write(event,0x328,static_cast<unsigned char>(0));
    if(hologram_read<int>(event,0)==12)hologram_write(event,0x194,hologram_read<std::uint32_t>(event,0x190));
    return 1;
}
// Original timing scans are independent, zero-anchored and re-read count.
// Compare extended intermediate before rounding to float; NaN is skipped.
bool choreography_bound(void* parsed,const SceneDurationBindings& h,bool end,float& result) noexcept {
    result=0;
    for(unsigned i=0;;++i){
        const int count=hologram_read<int>(parsed,0xc);
        if(count<=0||i>=static_cast<unsigned>(count))break;
        if(count>0x100000)return false;
        void* event=nullptr;unsigned bytes=0;
        if(!h.event(h.context,parsed,i,&event,&bytes)||!event||bytes<0x18c||
           !sin_independent::vgui_layout::Readable(event,0x18c))return false;
        const unsigned off=end&&hologram_read<std::uint32_t>(event,0x188)!=0xbf800000u?0x188:0x184;
        double time=hologram_read<float>(event,off);
        if(hologram_read<int>(event,0)==5){
            const double latency=hologram_read<float>(parsed,0x90);
            time=end?time+latency:time-latency;
        }
        if(end?time>result:time<result)result=static_cast<float>(time);
    }
    return true;
}
int __cdecl choreography_bounds(void* parsed,unsigned bytes,const SceneDurationBindings* b,float* first,float* last) noexcept {
    if(bytes<0x94||!parsed||!sin_independent::vgui_layout::Readable(parsed,0x94)||!b||!b->event||
       !hologram_writable(first,4)||!hologram_writable(last,4)||first==last)return 0;
    const auto h=*b;float start=0,end=0;
    if(!choreography_bound(parsed,h,false,start)||!choreography_bound(parsed,h,true,end))return -1;
    *first=start;*last=end;return 1;
}
bool choreography_reset_append(void* vector,const ParsedSceneResetBindings& h,void* event) noexcept {
    const int count=hologram_read<int>(vector,0xc),capacity=hologram_read<int>(vector,4);
    if(count<0||capacity<count||capacity>0x100000||count>=0x100000)return false;
    if(count==capacity){
        if(!h.grow(h.context,vector,1)||hologram_read<int>(vector,0xc)!=count||
           hologram_read<int>(vector,4)<=count||hologram_read<int>(vector,4)>0x100000)return false;
    }
    const auto word=hologram_read<std::uint32_t>(vector,0);
    void* storage=nullptr;unsigned extent=0;
    if(!word||!h.storage(h.context,word,&storage,&extent)||extent<static_cast<unsigned>(count+1)*4||
       !hologram_writable(storage,static_cast<unsigned>(count+1)*4))return false;
    hologram_write(vector,0xc,count+1);
    hologram_write(vector,0x10,word);
    hologram_write(storage,static_cast<unsigned>(count)*4,scene_word(event));
    return true;
}
int __cdecl choreography_reset(void* parsed,unsigned bytes,unsigned forward,std::uint32_t a,std::uint32_t z,const ParsedSceneResetBindings* b) noexcept {
    if(bytes<0x220||!hologram_writable(parsed,0x220)||!b||!b->events.event||!b->storage||!b->grow)return 0;
    const auto h=*b;auto* p=static_cast<unsigned char*>(parsed);
    hologram_write(p,0x5c,0u);hologram_write(p,0x48,0u);hologram_write(p,0x70,0u);
    for(unsigned i=0;;++i){
        const int count=hologram_read<int>(p,0xc);
        if(count<=0||i>=static_cast<unsigned>(count))break;
        if(count>0x100000)return -1;
        void* e=nullptr;unsigned extent=0;
        if(!h.events.event(h.events.context,p,i,&e,&extent)||choreography_event_reset(e,extent)!=1)return -1;
        if(hologram_read<int>(e,0)==1){
            if(!choreography_reset_append(p+0x64,h,e))return -1;
        }else if(hologram_read<unsigned char>(e,0x1b0)){
            if(!choreography_reset_append(p+0x3c,h,e))return -1;
        }
    }
    float first=0,last=0;
    if(!choreography_bound(p,h.events,false,first))return -1;
    hologram_write(p,0x84,first);
    if(!choreography_bound(p,h.events,true,last))return -1;
    hologram_write(p,0x88,last);hologram_write(p,0x78,(forward&0xff)?first:last);
    hologram_write(p,0x94,0u);hologram_write(p,0x8c,hologram_read<std::uint32_t>(p,0xc));
    hologram_write(p,0x7c,a);hologram_write(p,0x80,z);
    return 1;
}
const ParsedSceneResetApi parsed_scene_reset_api{version,sizeof(ParsedSceneResetApi),&choreography_event_reset,&choreography_bounds,&choreography_reset};
// 221adcd0. Virtual leaves remain hosted, but native code owns the
// admission, deferred flag, mutation sequence and original reset.
int native_scene_start_impl(void* scene,unsigned bytes,const SceneStartBindings* b,SceneStartResult* out,const SceneStopBindings* stop) noexcept {
    if(bytes<0x55c||!hologram_writable(scene,0x55c)||!out||!hologram_writable(out,sizeof(*out))||
       !scene_start_ready(b,stop))return 0;
    const auto h=*b;
    const auto complete=[&](SceneStartResult r){*out=r;return 1;};
    if(!hologram_read<std::uint32_t>(scene,0x40c)||hologram_read<unsigned char>(scene,0x3d8))
        return complete(SceneStartResult::NoAction);
    auto parsed=[&](void*& p,unsigned& extent){
        const auto word=hologram_read<std::uint32_t>(scene,0x40c);
        p=nullptr;extent=0;return word&&h.parsed(h.context,word,&p,&extent)&&extent>=0x220&&hologram_writable(p,0x220);
    };
    void* p=nullptr;unsigned extent=0;
    for(unsigned i=0;;++i){
        if(!parsed(p,extent))return -1;
        const int count=hologram_read<int>(p,0x20);
        if(count<=0||i>=static_cast<unsigned>(count))break;
        if(count>0x100000)return -1;
        void* actor=nullptr;unsigned n=0,eligible=0;
        if(!h.actor(h.context,scene,i,&actor,&n))return -1;
        if(!actor)continue;
        if(!h.is_npc(h.context,actor,&eligible))return -1;
        if(!(eligible&0xff))continue;
        if(!h.is_npc(h.context,actor,&eligible)||!(eligible&0xff)||n<0x90||
           !sin_independent::vgui_layout::Readable(actor,0x90))return -1;
        if(hologram_read<std::uint32_t>(actor,0x8c))return complete(SceneStartResult::ActorBlocked);
        if(hologram_read<int>(scene,0x558)==1){
            if(!h.is_npc(h.context,actor,&eligible))return -1;
            if(eligible&0xff){
                unsigned speaking=0;if(!h.speaking(h.context,actor,&speaking))return -1;
                if(speaking&0xff){hologram_write(scene,0x3f5,static_cast<unsigned char>(1));return complete(SceneStartResult::Deferred);}
            }
        }
        const auto word=hologram_read<std::uint32_t>(scene,0x40c);
        if(!(stop?scene_actor_attach(actor,n,word,&stop->records)==1:h.attach(h.context,actor,word)!=0))return -1;
    }
    hologram_write(scene,0x3f5,static_cast<unsigned char>(0));
    hologram_write(scene,0x3d8,static_cast<unsigned char>(1));
    hologram_write(scene,0x3d9,static_cast<unsigned char>(0));
    hologram_write(scene,0x3dc,0u);
    if(choreography_reset(p,extent,1,0,0,&h.reset)!=1)return -1;
    if(!h.refresh(h.context,scene)||
       !(stop?scene_stop_events(scene,bytes,hologram_read<std::uint32_t>(scene,0x40c),0,stop)==1:
           h.stop_events(h.context,scene,hologram_read<std::uint32_t>(scene,0x40c),0)!=0)||
       !h.output(h.context,static_cast<unsigned char*>(scene)+0x41c,scene,scene,0.f)||
       !h.startup_events(h.context,scene,hologram_read<std::uint32_t>(scene,0x40c)))return -1;
    return complete(SceneStartResult::Started);
}
int __cdecl native_scene_start(void* scene,unsigned bytes,const SceneStartBindings* b,SceneStartResult* out) noexcept {
    return native_scene_start_impl(scene,bytes,b,out,nullptr);
}
const SceneStartApi scene_start_api{version,sizeof(SceneStartApi),&native_scene_start};
bool scene_actor_storage(const SceneActorEventBindings& h,void* v,unsigned count,unsigned stride,void*& rows) noexcept {
    rows=nullptr;unsigned bytes=0;const auto word=hologram_read<std::uint32_t>(v,0);
    return word&&h.storage(h.context,word,&rows,&bytes)&&bytes>=count*stride&&hologram_writable(rows,count*stride);
}
int scene_actor_associate(void* actor,unsigned bytes,std::uint32_t parsed,const SceneActorEventBindings* b,bool add) noexcept {
    if(bytes<0x601||!hologram_writable(actor,0x601)||!b||!b->storage||(add&&!b->grow))return 0;
    const auto h=*b;auto* v=static_cast<unsigned char*>(actor)+0x5ec;
    const int count=hologram_read<int>(v,0xc),capacity=hologram_read<int>(v,4);
    if(count<0||count>0x100000||capacity<count||capacity>0x100000)return -1;
    void* rows=nullptr;
    if(count&&!scene_actor_storage(h,v,static_cast<unsigned>(count),4,rows))return -1;
    int index=-1;
    for(int i=0;i<count;++i)if(hologram_read<std::uint32_t>(rows,i*4)==parsed){index=i;break;}
    if(add){
        if(index>=0)return 1; // original does not dirty on duplicate
        if(count>=0x100000)return -1;
        if(count==capacity&&(!h.grow(h.context,v,1)||hologram_read<int>(v,0xc)!=count||
            hologram_read<int>(v,4)<=count||hologram_read<int>(v,4)>0x100000))return -1;
        if(!scene_actor_storage(h,v,static_cast<unsigned>(count+1),4,rows))return -1;
        hologram_write(v,0xc,count+1);hologram_write(v,0x10,hologram_read<std::uint32_t>(v,0));
        hologram_write(rows,count*4,parsed);
    }else if(index>=0){
        auto* p=static_cast<unsigned char*>(rows)+index*4;
        if(index<count-1)std::memmove(p,p+4,static_cast<unsigned>(count-index-1)*4);
        hologram_write(v,0xc,count-1);
    }
    hologram_write(actor,0x600,static_cast<unsigned char>(1));
    return 1;
}
int __cdecl scene_actor_attach(void* actor,unsigned bytes,std::uint32_t parsed,const SceneActorEventBindings* b) noexcept {
    return scene_actor_associate(actor,bytes,parsed,b,true);
}
int __cdecl scene_actor_detach(void* actor,unsigned bytes,std::uint32_t parsed,const SceneActorEventBindings* b) noexcept {
    return scene_actor_associate(actor,bytes,parsed,b,false);
}
// 220f7d00 walks backward over 64-byte active actor event records.
int __cdecl scene_actor_stop(void* actor,unsigned bytes,std::uint32_t parsed,int argument,const SceneActorEventBindings* b) noexcept {
    if(bytes<0x5c8||!hologram_writable(actor,0x5c8)||!b||!b->storage||!b->stop_record)return 0;
    if(!parsed){hologram_write(actor,0x5c4,0u);return 1;}
    const auto h=*b;auto* v=static_cast<unsigned char*>(actor)+0x5b8;
    int count=hologram_read<int>(v,0xc);
    if(count<0||count>0x100000)return -1;
    if(!count)return 1;
    void* rows=nullptr;if(!scene_actor_storage(h,v,static_cast<unsigned>(count),0x40,rows))return -1;
    for(int i=count-1;i>=0;--i){
        auto* row=static_cast<unsigned char*>(rows)+i*0x40;
        if(hologram_read<std::uint32_t>(row,4)!=parsed)continue;
        const auto word=hologram_read<std::uint32_t>(v,0);
        if(!h.stop_record(h.context,actor,row,0,argument)||hologram_read<int>(v,0xc)!=count||
           hologram_read<std::uint32_t>(v,0)!=word)return -1;
        hologram_write(row,0,0u);hologram_write(row,4,0u);hologram_write(row,0xc,static_cast<unsigned char>(0));
        if(i<count-1)std::memmove(row,row+0x40,static_cast<unsigned>(count-i-1)*0x40);
        hologram_write(v,0xc,--count);
    }
    return 1;
}
// 221a2c90 uses root scene actor count for each nested parsed argument.
// Bound recursion and reject cycles as host safety adaptations.
bool scene_stop_visit(void* scene,std::uint32_t word,int argument,const SceneStopBindings& h,
    std::uint32_t (&path)[64],unsigned depth,unsigned& budget) noexcept {
    if(!hologram_read<std::uint32_t>(scene,0x40c))return true;
    if(!word||depth>=64||!budget)return false;
    --budget;
    for(unsigned i=0;i<depth;++i)if(path[i]==word)return false;
    path[depth]=word;
    for(unsigned i=0;;++i){
        const auto root=hologram_read<std::uint32_t>(scene,0x40c);
        void* p=nullptr;unsigned n=0;
        if(!root||!h.parsed(h.context,root,&p,&n)||!p||n<0x24||!sin_independent::vgui_layout::Readable(p,0x24))return false;
        const int count=hologram_read<int>(p,0x20);
        if(count<=0||i>=static_cast<unsigned>(count))break;
        if(count>0x100000||!budget)return false;
        --budget;void* actor=nullptr;unsigned extent=0;
        if(!h.actor(h.context,scene,i,&actor,&extent)||
           (actor&&scene_actor_stop(actor,extent,word,argument,&h.records)!=1))return false;
    }
    void* p=nullptr;unsigned n=0;
    if(!h.parsed(h.context,word,&p,&n)||!p||n<0x125||!sin_independent::vgui_layout::Readable(p,0x125))return false;
    for(unsigned i=0;;++i){
        const int count=hologram_read<int>(p,0xc);
        if(count<=0||i>=static_cast<unsigned>(count))break;
        if(count>0x100000||!budget)return false;
        --budget;void* e=nullptr;unsigned extent=0;
        if(!h.events.event(h.events.context,p,i,&e,&extent))return false;
        if(!e)continue;
        if(extent<4||!sin_independent::vgui_layout::Readable(e,4))return false;
        if(hologram_read<int>(e,0)!=11||hologram_read<unsigned char>(p,0x124))continue;
        if(extent<0x328||!sin_independent::vgui_layout::Readable(e,0x328))return false;
        const auto nested=hologram_read<std::uint32_t>(e,0x324);
        if(nested&&!scene_stop_visit(scene,nested,argument,h,path,depth+1,budget))return false;
    }
    return true;
}
int __cdecl scene_stop_events(void* scene,unsigned bytes,std::uint32_t parsed,int argument,const SceneStopBindings* b) noexcept {
    if(bytes<0x410||!scene||!sin_independent::vgui_layout::Readable(scene,0x410)||!b||!b->parsed||!b->actor||
       !b->events.event||!b->records.storage||!b->records.stop_record)return 0;
    const auto h=*b;std::uint32_t path[64]{};unsigned budget=0x100000;
    return scene_stop_visit(scene,parsed,argument,h,path,0,budget)?1:-1;
}
const SceneActorEventsApi scene_actor_events_api{version,sizeof(SceneActorEventsApi),&scene_actor_attach,&scene_actor_detach,&scene_actor_stop,&scene_stop_events};
int __cdecl native_generated_sound(void* npc,const char* name,const NativeGeneratedLifecycleBindings* b,float* out) noexcept {
    if(!b||!generated_scene_ready(&b->generation,&b->allocation)||!scene_start_ready(&b->startup,&b->stop))return 0;
    const auto h=*b;
    return npc_scene_play(npc,name,0,&h.playback,out,true,&h.lifecycle,&h.generation,&h.allocation,&h.startup,&h.stop);
}
int __cdecl native_generated_start(void* scene,unsigned bytes,const SceneStartBindings* b,const SceneStopBindings* stop,SceneStartResult* out) noexcept {
    if(!stop)return 0;
    const auto h=*stop;
    return native_scene_start_impl(scene,bytes,b,out,&h);
}
int __cdecl native_generated_remove(void* scene,unsigned bytes,const SceneEntityLifecycleBindings* b,const SceneStopBindings* stop) noexcept {
    if(!stop)return 0;
    const auto h=*stop;
    return scene_entity_remove_impl(scene,bytes,b,&h);
}
const NativeGeneratedLifecycleApi native_generated_lifecycle_api{version,sizeof(NativeGeneratedLifecycleApi),&native_generated_sound,&native_generated_start,&native_generated_remove};
int __cdecl native_scene_cleanup_owned(void* scene,unsigned bytes,const SceneEntityLifecycleBindings* b,const SceneStopBindings* stop,const ChoreographyDestructionBindings* destruction) noexcept {
    if(!scene_stop_ready(stop)||!choreography_destroy_ready(destruction))return 0;
    const auto s=*stop;const auto d=*destruction;
    return scene_entity_cleanup_impl(scene,bytes,b,&s,&d);
}
int __cdecl native_scene_remove_owned(void* scene,unsigned bytes,const SceneEntityLifecycleBindings* b,const SceneStopBindings* stop,const ChoreographyDestructionBindings* destruction) noexcept {
    if(!scene_stop_ready(stop)||!choreography_destroy_ready(destruction))return 0;
    const auto s=*stop;const auto d=*destruction;
    return scene_entity_remove_impl(scene,bytes,b,&s,&d);
}
const NativeSceneRetirementApi native_scene_retirement_api{version,sizeof(NativeSceneRetirementApi),&native_scene_cleanup_owned,&native_scene_remove_owned};
int __cdecl owned_choreography_generate(void* scene,unsigned bytes,void* actor,const char* sound,const GeneratedSceneBindings* g,
    const GeneratedSceneAllocationBindings* a,const OwnedChoreographyBindings* owned,std::uint32_t* out) noexcept {
    if(!owned_choreography_ready(owned)||!a)return 0;
    const auto o=*owned;return generate_sound_scene_impl(scene,bytes,actor,sound,g,out,a,&o);
}
int __cdecl owned_choreography_sound(void* npc,const char* name,const NativeGeneratedLifecycleBindings* b,const OwnedChoreographyBindings* owned,float* out) noexcept {
    if(!owned_choreography_ready(owned)||!b||!generated_scene_ready(&b->generation,&b->allocation,owned)||!scene_start_ready(&b->startup,&b->stop))return 0;
    const auto h=*b;const auto o=*owned;
    return npc_scene_play(npc,name,0,&h.playback,out,true,&h.lifecycle,&h.generation,&h.allocation,&h.startup,&h.stop,&o);
}
int __cdecl owned_choreography_remove(void* scene,unsigned bytes,const SceneEntityLifecycleBindings* b,const SceneStopBindings* stop,const OwnedChoreographyBindings* owned) noexcept {
    if(!owned_choreography_ready(owned)||!scene_stop_ready(stop))return 0;
    const auto o=*owned;const auto s=*stop;
    return scene_entity_remove_impl(scene,bytes,b,&s,&o.destruction,&o.symbols);
}
const OwnedChoreographyApi owned_choreography_api{version,sizeof(OwnedChoreographyApi),&owned_choreography_generate,&owned_choreography_sound,&owned_choreography_remove};
// 221ad670: extension replacement on 512-byte path; 221ad1b0 uses a
// separate 256-byte slash-fixed lowercase cache key. ASCII game paths.
bool named_scene_copy(char* out,unsigned cap,const char* in,bool lower) noexcept {
    if(!out||!cap||!in)return false;
    for(unsigned i=0;i+1<cap;++i){
        if(!sin_independent::vgui_layout::Readable(in+i,1))return false;
        unsigned char ch=static_cast<unsigned char>(in[i]);
        if(ch=='/')ch='\\';
        if(lower&&ch>='A'&&ch<='Z')ch=static_cast<unsigned char>(ch+32);
        out[i]=static_cast<char>(ch);if(!ch)return true;
    }
    out[cap-1]=0;return true;
}
bool choreography_text_ready(const ChoreographyTextParserBindings* p) noexcept {
    return p&&p->tokenizer_word&&p->next&&p->token&&p->block&&p->unexpected&&p->finalize;
}
bool named_scene_ready(const NamedSceneLoadBindings* b,const ChoreographyTextParserBindings* parser=nullptr) noexcept {
    return b&&b->set_extension&&b->allocate&&b->release&&b->missing&&
        (parser?(choreography_text_ready(parser)&&parser->begin):b->parse!=nullptr)&&b->compare_word&&b->diagnostic_word&&
        b->cache.ensure&&b->cache.find&&b->cache.lock&&b->cache.reload&&b->cache.unlock&&owned_choreography_ready(&b->owned);
}
int __cdecl choreography_text_parse(void*,unsigned,const char*,const ChoreographyTextParserBindings*) noexcept;
int choreography_text_parse_impl(void*,unsigned,const char*,const ChoreographyTextParserBindings*,const ChoreographyActorParserBindings*,const ChoreographyEventParserBindings* events=nullptr) noexcept;
int named_scene_load_impl(void* scene,unsigned bytes,const char* name,const NamedSceneLoadBindings* b,std::uint32_t* out,const ChoreographyTextParserBindings* parser,const ChoreographyActorParserBindings* actors=nullptr,const ChoreographyEventParserBindings* events=nullptr) noexcept {
    if(!scene||bytes<0x414||!sin_independent::vgui_layout::Readable(scene,0x414)||!name||!hologram_writable(out,4)||!named_scene_ready(b,parser))return 0;
    const auto h=*b;const auto& cache=h.cache;
    char path[512]{},key[256]{};
    // Copy before Q_SetExtension; FixSlashes occurs after it in the original.
    for(unsigned i=0;i<sizeof(path)-1;++i){
        if(!sin_independent::vgui_layout::Readable(name+i,1))return 0;
        if(!(path[i]=name[i]))break;
    }
    if(!h.set_extension(h.context,path,".vcd",sizeof(path))||!std::memchr(path,0,sizeof(path)))return -1;
    for(auto* p=path;*p;++p)if(*p=='/')*p='\\';
    if(!named_scene_copy(key,sizeof(key),path,true))return -1;
    auto missing=[&](){if(!h.missing(h.context,path))return -1;*out=0;return 1;};
    std::uint32_t index=0xffffffffu;
    if(!cache.ensure(cache.context,key)||!cache.find(cache.context,key,&index))return -1;
    if(index==0xffffffffu)return missing();
    const void* data=nullptr;unsigned n=0;
    if(!cache.lock(cache.context,index,&data,&n))return -1;
    if(!data){
        if(n)return -1;
        if(!cache.reload(cache.context,key,index,&index))return -1;
        if(index==0xffffffffu)return missing();
        if(!cache.lock(cache.context,index,&data,&n))return -1;
        if(!data)return n?-1:missing();
    }
    if(!n){if(!cache.unlock(cache.context,index))return -1;return missing();}
    // Original copies size+1, appends NUL, then unlocks before parsing.
    // Reject overflow and malformed cache regions, never pass borrowed data.
    void* copy=nullptr;unsigned copy_bytes=0;
    bool copied=n<0x7fffffffu&&sin_independent::vgui_layout::Readable(data,n)&&
        h.allocate(h.context,n+1,&copy,&copy_bytes)&&copy&&copy_bytes>=n+1&&hologram_writable(copy,n+1);
    if(copied){std::memcpy(copy,data,n);static_cast<char*>(copy)[n]=0;}
    const bool unlocked=cache.unlock(cache.context,index)!=0;
    if(!copied||!unlocked){if(copy)(void)h.release(h.context,copy);return -1;}
    GeneratedSceneAllocationBindings a{};
    a.context=h.context;a.allocate=h.allocate;a.release_unpublished=h.release;a.parsed.compare_word=h.compare_word;
    void* parsed=nullptr;unsigned extent=0;
    if(!allocate_generated_parsed(a,static_cast<unsigned char*>(scene)+0x410,&parsed,&extent,&h.owned)||!parsed){
        (void)h.release(h.context,copy);return -1;
    }
    const bool parsed_ok=parser?(parser->begin(parser->context,static_cast<const char*>(copy),n)&&
        choreography_text_parse_impl(parsed,extent,path,parser,actors,events)==1):h.parse(h.context,parsed,path,static_cast<const char*>(copy),n)!=0;
    // Original 22119b20 sets diagnostics after the parser returns.
    if(parsed_ok)hologram_write(parsed,0x98,h.diagnostic_word);
    const bool released=h.release(h.context,copy)!=0;
    if(!parsed_ok||!released){
        (void)choreography_destroy_parsed(parsed,scene_word(parsed),h.owned.destruction,nullptr,&h.owned.symbols);return -1;
    }
    *out=scene_word(parsed);return 1;
}
int __cdecl named_scene_load(void* scene,unsigned bytes,const char* name,const NamedSceneLoadBindings* b,std::uint32_t* out) noexcept {
    return named_scene_load_impl(scene,bytes,name,b,out,nullptr);
}
const NamedSceneLoadApi named_scene_load_api{version,sizeof(NamedSceneLoadApi),&named_scene_load};
// Original 223a5673 uses 32-bit wraparound arithmetic, then signed clamp.
int choreography_text_integer(const char* p) noexcept {
    while(*p==' '||(*p>='\t'&&*p<='\r'))++p;
    const bool negative=*p=='-';if(*p=='-'||*p=='+')++p;
    std::uint32_t v=0;
    while(*p>='0'&&*p<='9'){v=v*10+static_cast<unsigned>(*p-'0');++p;}
    if(negative)v=0u-v;
    int result=0;std::memcpy(&result,&v,4);return result;
}
bool choreography_text_equal(const char* a,const char* b) noexcept {
    for(;;++a,++b){unsigned char c=static_cast<unsigned char>(*a),d=static_cast<unsigned char>(*b);
        if(c>='A'&&c<='Z')c=static_cast<unsigned char>(c+32);
        if(d>='A'&&d<='Z')d=static_cast<unsigned char>(d+32);
        if(c!=d)return false;if(!c)return true;}
}
bool choreography_text_copy(char* dest,unsigned cap,const char* source) noexcept {
    if(!source)return false;
    for(unsigned i=0;i+1<cap;++i){
        if(!sin_independent::vgui_layout::Readable(source+i,1))return false;
        dest[i]=source[i];if(!dest[i]){std::memset(dest+i+1,0,cap-i-1);return true;}
    }
    dest[cap-1]=0;return true;
}
bool choreography_text_token(const ChoreographyTextParserBindings& h,char (&out)[1024]) noexcept {
    const char* p=nullptr;if(!h.token(h.context,&p)||!p)return false;
    for(unsigned i=0;i<sizeof(out);++i){
        if(!sin_independent::vgui_layout::Readable(p+i,1))return false;
        out[i]=p[i];if(!out[i])return true;
    }
    return false;
}
bool choreography_actor_parser_ready(const ChoreographyActorParserBindings* b,bool native_event=false) noexcept {
    return b&&choreography_text_ready(&b->text)&&b->allocation.allocate&&b->allocation.release_unpublished&&
        b->allocation.append_parsed&&b->append_channel&&(native_event||b->event);
}
// Original actor/channel grammar expects name on the same line (next(0)),
// then braces and field values with next(1). EOF within a body is rejected
// rather than looping forever on stale tokens.
bool choreography_actor_next(const ChoreographyTextParserBindings& h,unsigned crossline,char (&token)[1024]) noexcept {
    unsigned more=0;return h.next(h.context,crossline,&more)&&more&&choreography_text_token(h,token);
}
int __cdecl choreography_event_parse(void*,unsigned,void*,void*,const ChoreographyEventParserBindings*) noexcept;
bool choreography_event_parser_ready(const ChoreographyEventParserBindings* b) noexcept {
    return b&&choreography_actor_parser_ready(&b->actors,true)&&b->actors.allocation.event_counter&&
        hologram_writable(b->actors.allocation.event_counter,4)&&b->same_line&&b->number&&b->end_changed&&
        b->append_event&&b->block&&b->legacy_ramp;
}
bool choreography_actor_body(void* parsed,void* actor,const ChoreographyActorParserBindings& h,bool channel,const ChoreographyEventParserBindings* events=nullptr) noexcept {
    void* object=actor;unsigned extent=0;
    if(channel){
        if(!allocate_generated_child(h.allocation,parsed,GeneratedSceneChild::Channel,&object,&extent)||!object)return false;
    }
    const auto& text=h.text;char token[1024]{};
    const unsigned nameoff=channel?4:0,activeoff=channel?0x98:0x114;
    if(!choreography_actor_next(text,0,token)||!choreography_text_copy(static_cast<char*>(object)+nameoff,128,token))return false;
    if(!choreography_actor_next(text,1,token))return false;
    if(!choreography_text_equal(token,"{")&&!text.unexpected(text.context,token))return false;
    for(unsigned i=0;i<0x100000;++i){
        if(!choreography_actor_next(text,1,token))return false;
        if(choreography_text_equal(token,"}")){
            if(channel){
                if(!h.append_channel(h.context,actor,object))return false;
                hologram_write(object,0,scene_word(actor));
            }
            return true;
        }
        if(choreography_text_equal(token,"active")){
            if(!choreography_actor_next(text,1,token))return false;
            hologram_write(object,activeoff,static_cast<unsigned char>(choreography_text_integer(token)!=0));
        }else if(!channel&&choreography_text_equal(token,"faceposermodel")){
            if(!choreography_actor_next(text,1,token)||!choreography_text_copy(static_cast<char*>(object)+0x80,128,token))return false;
        }else if(!channel&&choreography_text_equal(token,"channel")){
            if(!choreography_actor_body(parsed,actor,h,true,events))return false;
        }else if(channel&&choreography_text_equal(token,"event")){
            if(!(events?choreography_event_parse(parsed,0x220,actor,object,events)==1:h.event(h.context,parsed,actor,object)!=0))return false;
        }else if(!text.unexpected(text.context,token))return false;
    }
    return false;
}
int choreography_actor_parse_impl(void* parsed,unsigned bytes,const ChoreographyActorParserBindings* b,const ChoreographyEventParserBindings* events) noexcept {
    if(bytes<0x220||!hologram_writable(parsed,0x220)||!choreography_actor_parser_ready(b,events!=nullptr))return 0;
    const auto h=*b;void* actor=nullptr;unsigned extent=0;
    if(!allocate_generated_child(h.allocation,parsed,GeneratedSceneChild::Actor,&actor,&extent)||!actor)return -1;
    return choreography_actor_body(parsed,actor,h,false,events)?1:-1;
}
int __cdecl choreography_actor_parse(void* parsed,unsigned bytes,const ChoreographyActorParserBindings* b) noexcept {
    return choreography_actor_parse_impl(parsed,bytes,b,nullptr);
}
int choreography_text_parse_impl(void* parsed,unsigned bytes,const char* filename,const ChoreographyTextParserBindings* b,const ChoreographyActorParserBindings* actors,const ChoreographyEventParserBindings* events) noexcept {
    if(bytes<0x220||!hologram_writable(parsed,0x220)||!filename||!choreography_text_ready(b))return 0;
    const auto h=*b;
    char path[128]{};
    if(!choreography_text_copy(path,sizeof(path),filename))return 0;
    std::memcpy(static_cast<unsigned char*>(parsed)+0x198,path,sizeof(path));
    hologram_write(parsed,0xa0,h.tokenizer_word);
    for(unsigned iteration=0;;++iteration){
        if(iteration>=0x100000)return -1;
        unsigned more=0;if(!h.next(h.context,1,&more))return -1;
        if(!more)break;
        char token[1024]{};
        if(!choreography_text_token(h,token))return -1;
        if(!*token)break;
        ChoreographyTextBlock block{};
        bool nested=true;
        if(choreography_text_equal(token,"event"))block=ChoreographyTextBlock::Event;
        else if(choreography_text_equal(token,"actor"))block=ChoreographyTextBlock::Actor;
        else if(choreography_text_equal(token,"scene_ramp"))block=ChoreographyTextBlock::SceneRamp;
        else if(choreography_text_equal(token,"scalesettings"))block=ChoreographyTextBlock::ScaleSettings;
        else nested=false;
        if(nested){
            if(actors&&block==ChoreographyTextBlock::Actor){if(choreography_actor_parse_impl(parsed,bytes,actors,events)!=1)return -1;}
            else if(actors&&block==ChoreographyTextBlock::Event){if(!(events?choreography_event_parse(parsed,bytes,nullptr,nullptr,events)==1:actors->event(actors->context,parsed,nullptr,nullptr)!=0))return -1;}
            else if(!h.block(h.context,parsed,block))return -1;
            continue;
        }
        const bool map=choreography_text_equal(token,"mapname"),fps=choreography_text_equal(token,"fps"),snap=choreography_text_equal(token,"snap");
        if(!map&&!fps&&!snap){if(!h.unexpected(h.context,token))return -1;break;}
        // Original scalar handlers ignore next's boolean, but still read
        // the current token. Transport failure remains fatal.
        if(!h.next(h.context,1,&more)||!choreography_text_token(h,token))return -1;
        if(map){if(!choreography_text_copy(reinterpret_cast<char*>(static_cast<unsigned char*>(parsed)+0xa4),128,token))return -1;}
        else if(fps){const int v=choreography_text_integer(token);hologram_write(parsed,0x128,v>240?240:v<10?10:v);}
        else hologram_write(parsed,0x125,static_cast<unsigned char>(choreography_text_equal(token,"on")));
    }
    for(auto stage:{ChoreographyTextFinalize::RelativeTags,ChoreographyTextFinalize::GestureReconcile,ChoreographyTextFinalize::CaptionReconcile})
        if(!h.finalize(h.context,parsed,stage))return -1;
    return 1;
}
int __cdecl choreography_text_parse(void* parsed,unsigned bytes,const char* name,const ChoreographyTextParserBindings* b) noexcept {
    return choreography_text_parse_impl(parsed,bytes,name,b,nullptr);
}
int __cdecl choreography_actor_parse_root(void* parsed,unsigned bytes,const char* name,const ChoreographyActorParserBindings* b) noexcept {
    if(!choreography_actor_parser_ready(b))return 0;
    const auto h=*b;return choreography_text_parse_impl(parsed,bytes,name,&h.text,&h);
}
int __cdecl choreography_actor_load(void* scene,unsigned bytes,const char* name,const NamedSceneLoadBindings* b,const ChoreographyActorParserBindings* actors,std::uint32_t* out) noexcept {
    if(!choreography_actor_parser_ready(actors)||!actors->text.begin)return 0;
    const auto h=*actors;return named_scene_load_impl(scene,bytes,name,b,out,&h.text,&h);
}
const ChoreographyActorParserApi choreography_actor_parser_api{version,sizeof(ChoreographyActorParserApi),&choreography_actor_parse,&choreography_actor_parse_root,&choreography_actor_load};
// Original type table at 224e9e50 (17 entries) and caption table at 224e9ed8.
unsigned choreography_event_type(const char* token) noexcept {
    constexpr const char* names[]={"unspecified","section","expression","lookat","moveto","speak","gesture","sequence","face","firetrigger","flexanimation","subscene","loop","interrupt","stoppoint","permitresponses","generic"};
    for(unsigned i=0;i<17;++i)if(choreography_text_equal(token,names[i]))return i;
    return 0;
}
int __cdecl choreography_event_parse(void* parsed,unsigned bytes,void* actor,void* channel,const ChoreographyEventParserBindings* b) noexcept {
    if(bytes<0x220||!hologram_writable(parsed,0x220)||!choreography_event_parser_ready(b)||
       (actor&&!hologram_writable(actor,0x118))||(channel&&!hologram_writable(channel,0x9c)))return 0;
    const auto h=*b;const auto& text=h.actors.text;
    void* event=nullptr;unsigned extent=0;
    if(!allocate_generated_child(h.actors.allocation,parsed,GeneratedSceneChild::Event,&event,&extent)||!event)return -1;
    char token[1024]{};
    auto next=[&](unsigned cross){return choreography_actor_next(text,cross,token);};
    auto number=[&](unsigned cross,float& out){return next(cross)&&h.number(h.context,token,&out);};
    auto string=[&](unsigned cross,unsigned off,unsigned cap){return next(cross)&&choreography_text_copy(static_cast<char*>(event)+off,cap,token);};
    if(!next(0))return -1;
    const unsigned type=choreography_event_type(token);
    hologram_write(event,0,type);hologram_write(event,0x198,static_cast<unsigned char>(type==5||type==11));
    if(!string(0,4,128)||!next(1))return -1;
    if(!choreography_text_equal(token,"{")&&!text.unexpected(text.context,token))return -1;
    float ramp[3]{1,1,1};bool legacy=false;
    for(unsigned i=0;i<0x100000;++i){
        if(!next(1)||!*token)return -1;
        if(choreography_text_equal(token,"}")){
            if(channel&&!h.append_event(h.context,channel,event))return -1;
            hologram_write(event,0x2b4,scene_word(actor));hologram_write(event,0x2b8,scene_word(channel));
            // 221178f2..221179b0: only legacy nontrivial ramp with no samples.
            if(legacy&&!hologram_read<unsigned>(event,0x1a8)&&(ramp[0]!=1||ramp[1]!=1||ramp[2]!=1)&&
               !h.legacy_ramp(h.context,event,ramp[0],ramp[1],ramp[2]))return -1;
            return 1;
        }
        if(choreography_text_equal(token,"time")){
            float start=0,end=1;unsigned more=0;
            if(!number(0,start)||!h.same_line(h.context,&more)||(more&&!number(0,end)))return -1;
            const float old=hologram_read<float>(event,0x188);
            hologram_write(event,0x184,start);
            if(old!=-1&&old<start)hologram_write(event,0x188,start);
            const bool changed=hologram_read<float>(event,0x188)!=end;
            hologram_write(event,0x188,end);
            if(end!=-1){
                if(end<start)hologram_write(event,0x188,start);
                if(changed&&hologram_read<int>(event,0x318)>0&&!h.end_changed(h.context,event))return -1;
            }
        }else if(choreography_text_equal(token,"ramp")){
            legacy=true;if(!number(0,ramp[0]))return -1;
            for(unsigned j=1;j<3;++j){unsigned more=0;if(!h.same_line(h.context,&more))return -1;if(more&&!number(0,ramp[j]))return -1;}
        }else if(choreography_text_equal(token,"param")){if(!string(0,0x84,128))return -1;}
        else if(choreography_text_equal(token,"param2")){
            if(!string(0,0x104,128))return -1;
            const auto len=std::strlen(token);
            // Original trims one trailing space using source length.
            // Bound the write to the destination for oversized/empty tokens.
            if(len&&len<128&&token[len-1]==' ')static_cast<char*>(event)[0x104+len-1]=0;
        }else if(choreography_text_equal(token,"pitch")||choreography_text_equal(token,"yaw")||choreography_text_equal(token,"loopcount")){
            const unsigned off=choreography_text_equal(token,"pitch")?0x334:choreography_text_equal(token,"yaw")?0x338:0x190;
            if(!next(0))return -1;int v=choreography_text_integer(token);
            if(off==0x190&&v< -1)v=-1;hologram_write(event,off,v);
        }else if(choreography_text_equal(token,"distancetotarget")||choreography_text_equal(token,"sequenceduration")){
            const unsigned off=choreography_text_equal(token,"distancetotarget")?0x340:0x18c;
            float v=0;if(!number(0,v))return -1;hologram_write(event,off,v);
        }else if(choreography_text_equal(token,"resumecondition"))hologram_write(event,0x1b0,static_cast<unsigned char>(1));
        else if(choreography_text_equal(token,"fixedlength"))hologram_write(event,0x198,static_cast<unsigned char>(1));
        else if(choreography_text_equal(token,"lockbodyfacing"))hologram_write(event,0x33c,static_cast<unsigned char>(1));
        else if(choreography_text_equal(token,"relativetag")){
            // Original 2210b740 copies first token (sound reference) to +232,
            // second token (tag name) to +1b2.
            if(!string(0,0x232,128)||!string(0,0x1b2,128))return -1;
            hologram_write(event,0x1b1,static_cast<unsigned char>(1));
        }else if(choreography_text_equal(token,"cctype")){
            if(!next(0))return -1;
            hologram_write(event,0x34c,choreography_text_equal(token,"cc_slave")?1u:choreography_text_equal(token,"cc_disabled")?2u:0u);
        }else if(choreography_text_equal(token,"cctoken")){if(!string(0,0x350,64))return -1;}
        else if(choreography_text_equal(token,"cc_usingcombinedfile"))hologram_write(event,0x390,static_cast<unsigned char>(1));
        else if(choreography_text_equal(token,"cc_combinedusesgender"))hologram_write(event,0x3a1,static_cast<unsigned char>(1));
        else if(choreography_text_equal(token,"cc_noattenuate"))hologram_write(event,0x3a2,static_cast<unsigned char>(1));
        else{
            ChoreographyEventBlock block{};
            if(choreography_text_equal(token,"tags"))block=ChoreographyEventBlock::Tags;
            else if(choreography_text_equal(token,"absolutetags"))block=ChoreographyEventBlock::AbsoluteTags;
            else if(choreography_text_equal(token,"flextimingtags"))block=ChoreographyEventBlock::FlexTimingTags;
            else if(choreography_text_equal(token,"flexanimations"))block=ChoreographyEventBlock::FlexAnimations;
            else if(choreography_text_equal(token,"event_ramp"))block=ChoreographyEventBlock::EventRamp;
            else continue; // original unrecognized event field is ignored
            if(!h.block(h.context,event,block))return -1;
        }
    }
    return -1;
}
int __cdecl choreography_event_parse_root(void* parsed,unsigned bytes,const char* name,const ChoreographyEventParserBindings* b) noexcept {
    if(!choreography_event_parser_ready(b))return 0;
    const auto h=*b;return choreography_text_parse_impl(parsed,bytes,name,&h.actors.text,&h.actors,&h);
}
int __cdecl choreography_event_load(void* scene,unsigned bytes,const char* name,const NamedSceneLoadBindings* b,const ChoreographyEventParserBindings* events,std::uint32_t* out) noexcept {
    if(!choreography_event_parser_ready(events)||!events->actors.text.begin)return 0;
    const auto h=*events;return named_scene_load_impl(scene,bytes,name,b,out,&h.actors.text,&h.actors,&h);
}
const ChoreographyEventParserApi choreography_event_parser_api{version,sizeof(ChoreographyEventParserApi),&choreography_event_parse,&choreography_event_parse_root,&choreography_event_load};
int __cdecl choreography_text_load(void* scene,unsigned bytes,const char* name,const NamedSceneLoadBindings* b,const ChoreographyTextParserBindings* parser,std::uint32_t* out) noexcept {
    if(!choreography_text_ready(parser)||!parser->begin)return 0;
    const auto p=*parser;return named_scene_load_impl(scene,bytes,name,b,out,&p);
}
const ChoreographyTextParserApi choreography_text_parser_api{version,sizeof(ChoreographyTextParserApi),&choreography_text_parse,&choreography_text_load};
bool named_scene_precache_ready(const NamedScenePrecacheBindings* b,const ChoreographyEventParserBindings* events=nullptr) noexcept {
    return b&&b->precache_sound&&(!events||choreography_event_parser_ready(events))&&
        named_scene_ready(&b->load,events?&events->actors.text:b->parser);
}
int named_scene_precache_load(void* scene,unsigned bytes,const char* name,const NamedScenePrecacheBindings& h,
    const ChoreographyEventParserBindings* events,std::uint32_t* out) noexcept {
    return named_scene_load_impl(scene,bytes,name,&h.load,out,events?&events->actors.text:h.parser,
        events?&events->actors:nullptr,events);
}
// 221ad950 traverses the live event count, skips null events, and only
// expands subscenes of a non-subscene root. Nested parsed +124 is set
// before linking event +324 and visiting the child's speech events.
int named_scene_precache_events_impl(void* scene,unsigned bytes,void* parsed,unsigned extent,const NamedScenePrecacheBindings& h,unsigned depth,const ChoreographyEventParserBindings* events=nullptr) noexcept {
    if(depth>1||extent<0x220||!hologram_writable(parsed,0x220))return -1;
    const auto& d=h.load.owned.destruction;
    for(unsigned i=0;;++i){
        const int count=hologram_read<int>(parsed,0xc);
        if(count<=0||i>=static_cast<unsigned>(count))break;
        if(count>0x100000)return -1;
        void* rows=nullptr;
        if(!choreography_storage(d,hologram_read<std::uint32_t>(parsed,0),(i+1)*4,rows))return -1;
        const auto word=hologram_read<std::uint32_t>(rows,i*4);
        if(!word)continue;
        void* event=nullptr;if(!choreography_storage(d,word,0x3c0,event))return -1;
        const auto type=hologram_read<unsigned>(event,0);
        const auto* parameter=reinterpret_cast<const char*>(static_cast<unsigned char*>(event)+0x84);
        if(type==5){
            if(!std::memchr(parameter,0,128)||!h.precache_sound(h.context,parameter))return -1;
            if(hologram_read<unsigned>(event,0x34c)==0&&hologram_read<int>(event,0x398)>0){
                const auto* token=reinterpret_cast<const char*>(static_cast<unsigned char*>(event)+0x350);
                if(!std::memchr(token,0,64))return -1;
                char caption[64]{};
                if(!choreography_text_copy(caption,sizeof(caption),*token?token:parameter)||!h.precache_sound(h.context,caption))return -1;
            }
        }else if(type==11&&!hologram_read<unsigned char>(parsed,0x124)&&!hologram_read<unsigned>(event,0x324)){
            if(!std::memchr(parameter,0,128)||depth)return -1;
            std::uint32_t child=0;
            if(named_scene_precache_load(scene,bytes,parameter,h,events,&child)!=1||!child)return -1;
            // Failed resolution retains no safe writable pointer. Host owns
            // diagnosis and reclamation; never continue on malformed words.
            void* nested=nullptr;if(!choreography_storage(d,child,0x220,nested))return -1;
            hologram_write(nested,0x124,static_cast<unsigned char>(1));
            hologram_write(event,0x324,child);
            if(named_scene_precache_events_impl(scene,bytes,nested,0x220,h,depth+1,events)!=1)return -1;
        }
    }
    return 1;
}
int __cdecl named_scene_precache_events(void* scene,unsigned bytes,void* parsed,unsigned extent,const NamedScenePrecacheBindings* b) noexcept {
    if(!scene||bytes<0x414||!sin_independent::vgui_layout::Readable(scene,0x414)||extent<0x220||
       !hologram_writable(parsed,0x220)||!named_scene_precache_ready(b))return 0;
    const auto h=*b;return named_scene_precache_events_impl(scene,bytes,parsed,extent,h,0);
}
int named_scene_precache_spawn_impl(void* scene,unsigned bytes,const SceneEntityLifecycleBindings* life,const SceneStopBindings* stop,const NamedScenePrecacheBindings* b,const ChoreographyEventParserBindings* events) noexcept {
    if(bytes<0x54a||!hologram_writable(scene,0x54a))return 0;
    if(hologram_read<unsigned char>(scene,0x549))return 1;
    if(!life||!scene_stop_ready(stop)||!named_scene_precache_ready(b,events)||!b->text)return 0;
    const auto h=*b;const auto l=*life;const auto s=*stop;
    const auto word=hologram_read<std::uint32_t>(scene,0x394);
    const char* name="";
    if(word&&(!h.text(h.context,word,&name)||!name))return -1;
    // Snapshot the name before cleanup so no borrowed string is invalidated.
    char path[512]{};
    if(!choreography_text_copy(path,sizeof(path),name))return -1;
    const auto clean=scene_entity_cleanup_impl(scene,bytes,&l,&s,&h.load.owned.destruction,&h.load.owned.symbols);
    if(clean!=1)return clean;
    std::uint32_t parsed=0;
    if(named_scene_precache_load(scene,bytes,path,h,events,&parsed)!=1)return -1;
    hologram_write(scene,0x40c,parsed);
    if(!parsed)return 1;
    void* object=nullptr;
    if(!choreography_storage(h.load.owned.destruction,parsed,0x220,object))return -1;
    return named_scene_precache_events_impl(scene,bytes,object,0x220,h,0,events);
}
int __cdecl named_scene_precache_spawn(void* scene,unsigned bytes,const SceneEntityLifecycleBindings* life,const SceneStopBindings* stop,const NamedScenePrecacheBindings* b) noexcept {
    return named_scene_precache_spawn_impl(scene,bytes,life,stop,b,nullptr);
}
int named_scene_activate(void* scene,unsigned bytes,const SceneEntityLifecycleBindings* life) noexcept {
    if(bytes<0x554||!hologram_writable(scene,0x554)||!life||!life->base_activate||!life->manager||
       hologram_read<unsigned char>(scene,0x549))return 0;
    const auto h=*life;
    if(!h.base_activate(h.context,scene))return -1;
    return scene_entity_registration(scene,h,true);
}
struct NamedSpawnState {
    void* scene;unsigned bytes;const NativeNamedPlaybackBindings* bindings;
    unsigned calls=0;int status=0;
};
int __cdecl named_spawn_once(void* context,void* scene) noexcept {
    auto& state=*static_cast<NamedSpawnState*>(context);
    if(state.calls++||scene!=state.scene){state.status=-1;return 0;}
    const auto& h=*state.bindings;
    state.status=named_scene_precache_spawn_impl(scene,state.bytes,&h.lifecycle,&h.stop,&h.precache,h.events);
    return state.status==1;
}
int named_scene_register_and_spawn(void* scene,unsigned bytes,const NativeNamedPlaybackBindings* b) noexcept {
    if(hologram_read<unsigned char>(scene,0x549))return -1;
    NamedSpawnState state{scene,bytes,b};
    const bool ok=b->register_with_spawn(b->context,scene,&state,&named_spawn_once)!=0;
    return ok&&state.calls==1&&state.status==1?1:-1;
}
int __cdecl native_named_playback(void* npc,const char* name,float delay,const NativeNamedPlaybackBindings* b,float* out) noexcept {
    if(!b||!b->register_with_spawn||!b->lifecycle.parsed||!b->lifecycle.base_activate||
       !b->lifecycle.manager||!b->precache.text||!named_scene_precache_ready(&b->precache,b->events)||
       !scene_start_ready(&b->startup,&b->stop))return 0;
    const auto h=*b;
    return npc_scene_play(npc,name,delay,&h.playback,out,false,&h.lifecycle,nullptr,nullptr,&h.startup,&h.stop,nullptr,&h);
}
const NativeNamedPlaybackApi native_named_playback_api{version,sizeof(NativeNamedPlaybackApi),&native_named_playback};
bool choreography_tags_ready(const ChoreographyTimingTagBindings* b) noexcept {
    return b&&b->storage&&b->grow;
}
int __cdecl choreography_tag_append(void* event,unsigned bytes,ChoreographyTagKind kind,const char* name,float time,unsigned locked,const ChoreographyTimingTagBindings* b) noexcept {
    const unsigned k=static_cast<unsigned>(kind);
    if(bytes<0x3c0||!hologram_writable(event,0x3c0)||k>3||!name||!choreography_tags_ready(b))return 0;
    const auto h=*b;const unsigned off=0x2bc+k*0x14,stride=k?0x8c:0x88;
    auto* v=static_cast<unsigned char*>(event)+off;
    const int count=hologram_read<int>(v,0xc),capacity=hologram_read<int>(v,4);
    // Original performs a quadratic selection-style exchange after every
    // absolute/flex insertion. Bound malformed hostile counts defensively.
    if(count<0||capacity<0||count>capacity||count>=4096)return -1;
    char label[128]{};
    if(!choreography_text_copy(label,sizeof(label),name))return 0;
    if(count==capacity&&(!h.grow(h.context,v,1,stride)||hologram_read<int>(v,0xc)!=count||hologram_read<int>(v,4)<=count))return -1;
    const auto word=hologram_read<std::uint32_t>(v,0);
    void* data=nullptr;unsigned extent=0;
    const unsigned need=(static_cast<unsigned>(count)+1)*stride;
    if(!word||!h.storage(h.context,word,&data,&extent)||extent<need||!hologram_writable(data,need))return -1;
    auto* rows=static_cast<unsigned char*>(data);
    auto* row=rows+static_cast<unsigned>(count)*stride;
    std::memcpy(row,label,128);hologram_write(row,0x80,time);
    if(k>=2){
        // Original temporary high nibble is indeterminate; choose zero for
        // deterministic reconstructed storage. Low nibble is cleared.
        hologram_write(row,0x84,static_cast<unsigned char>(0));hologram_write(row,0x88,scene_word(event));
    }else{
        hologram_write(row,0x84,scene_word(event));
        if(k==1)hologram_write(row,0x88,static_cast<unsigned char>(locked!=0));
    }
    hologram_write(v,0xc,count+1);hologram_write(v,0x10,word);
    if(k){
        // Exact nested exchange order from 22110440 / 221105e0, not
        // std::sort (ties and unordered floats must retain original behavior).
        unsigned char scratch[0x8c];
        for(int i=0;i<=count;++i)for(int j=i+1;j<=count;++j){
            auto* left=rows+i*stride;auto* right=rows+j*stride;
            if(hologram_read<float>(left,0x80)>hologram_read<float>(right,0x80)){
                std::memcpy(scratch,left,stride);std::memcpy(left,right,stride);std::memcpy(right,scratch,stride);
            }
        }
    }
    return 1;
}
int __cdecl choreography_tag_block(void* event,unsigned bytes,ChoreographyEventBlock block,const ChoreographyTimingTagBindings* b) noexcept {
    if(bytes<0x3c0||!hologram_writable(event,0x3c0)||!choreography_tags_ready(b)||!b->number||
       !b->text.next||!b->text.token||!b->text.unexpected)return 0;
    if(block!=ChoreographyEventBlock::Tags&&block!=ChoreographyEventBlock::AbsoluteTags&&block!=ChoreographyEventBlock::FlexTimingTags)return 0;
    const auto h=*b;char token[1024]{};
    auto next=[&](unsigned cross){return choreography_actor_next(h.text,cross,token);};
    ChoreographyTagKind kind=ChoreographyTagKind::Relative;
    if(block==ChoreographyEventBlock::FlexTimingTags)kind=ChoreographyTagKind::FlexTiming;
    else if(block==ChoreographyEventBlock::AbsoluteTags){
        if(!next(1))return -1;
        if(choreography_text_equal(token,"playback_time"))kind=ChoreographyTagKind::Playback;
        else if(choreography_text_equal(token,"shifted_time"))kind=ChoreographyTagKind::Shifted;
        else{(void)h.text.unexpected(h.text.context,token);return -1;}
    }
    if(!next(1))return -1;
    if(!choreography_text_equal(token,"{")&&!h.text.unexpected(h.text.context,token))return -1;
    for(unsigned i=0;i<4096;++i){
        if(!next(1)||!*token)return -1;
        if(choreography_text_equal(token,"}"))return 1;
        char name[128]{};
        if(!choreography_text_copy(name,sizeof(name),token)||!next(0))return -1;
        float time=0;unsigned locked=0;
        if(!h.number(h.context,token,&time))return -1;
        if(kind==ChoreographyTagKind::FlexTiming){if(!next(0))return -1;locked=choreography_text_integer(token)!=0;}
        if(choreography_tag_append(event,bytes,kind,name,time,locked,&h)!=1)return -1;
    }
    return -1;
}
const ChoreographyTimingTagsApi choreography_timing_tags_api{version,sizeof(ChoreographyTimingTagsApi),&choreography_tag_append,&choreography_tag_block};
bool choreography_ramp_object(void* p,unsigned bytes,ChoreographyRampKind kind) noexcept {
    if(kind!=ChoreographyRampKind::Event&&kind!=ChoreographyRampKind::Scene)return false;
    const unsigned need=kind==ChoreographyRampKind::Event?0x3c0:0x220;
    return bytes>=need&&hologram_writable(p,need);
}
unsigned choreography_ramp_offset(ChoreographyRampKind kind) noexcept {
    return kind==ChoreographyRampKind::Event?0x19c:0x12c;
}
bool choreography_ramp_storage(const ChoreographyTimingTagBindings& h,std::uint32_t word,unsigned need,void*& data) noexcept {
    unsigned n=0;data=nullptr;
    return word&&h.storage(h.context,word,&data,&n)&&n>=need&&hologram_writable(data,need);
}
int __cdecl choreography_ramp_append(void* object,unsigned bytes,ChoreographyRampKind kind,float time,float value,unsigned selected,const ChoreographyTimingTagBindings* b) noexcept {
    if(!choreography_ramp_object(object,bytes,kind)||!choreography_tags_ready(b))return 0;
    const auto h=*b;auto* v=static_cast<unsigned char*>(object)+choreography_ramp_offset(kind);
    const int count=hologram_read<int>(v,0xc),capacity=hologram_read<int>(v,4);
    if(count<0||capacity<count||count>=4096)return -1;
    if(count==capacity&&(!h.grow(h.context,v,1,12)||hologram_read<int>(v,0xc)!=count||hologram_read<int>(v,4)<=count))return -1;
    const auto word=hologram_read<std::uint32_t>(v,0);void* data=nullptr;
    if(!choreography_ramp_storage(h,word,(count+1)*12,data))return -1;
    auto* row=static_cast<unsigned char*>(data)+count*12;
    hologram_write(row,0,value);hologram_write(row,4,time);
    // Original copies uninitialized padding; use deterministic zero padding.
    hologram_write(row,8,static_cast<std::uint32_t>(selected&0xff));
    hologram_write(v,0xc,count+1);hologram_write(v,0x10,word);
    return 1;
}
int __cdecl choreography_ramp_finish(void* object,unsigned bytes,ChoreographyRampKind kind,const ChoreographyTimingTagBindings* b) noexcept {
    if(!choreography_ramp_object(object,bytes,kind)||!choreography_tags_ready(b))return 0;
    const auto h=*b;auto* v=static_cast<unsigned char*>(object)+choreography_ramp_offset(kind);
    int count=hologram_read<int>(v,0xc);
    if(count<0||count>4096||hologram_read<int>(v,4)<count)return -1;
    void* data=nullptr;
    if(count&&!choreography_ramp_storage(h,hologram_read<std::uint32_t>(v,0),count*12,data))return -1;
    auto* rows=static_cast<unsigned char*>(data);
    // Original nested exchange sort, strict ordered greater; equal and NaN
    // comparisons do not exchange. Sort before duration lookup/trim.
    for(int i=0;i<count;++i)for(int j=i+1;j<count;++j){
        auto* l=rows+i*12;auto* r=rows+j*12;
        if(hologram_read<float>(l,4)>hologram_read<float>(r,4)){
            unsigned char scratch[12];std::memcpy(scratch,l,12);std::memcpy(l,r,12);std::memcpy(r,scratch,12);
        }
    }
    float duration=0;
    if(kind==ChoreographyRampKind::Event){
        if(hologram_read<std::uint32_t>(object,0x188)!=0xbf800000u)
            duration=hologram_read<float>(object,0x188)-hologram_read<float>(object,0x184);
    }else{
        // Original 22113c10: zero-anchored max event end or start sentinel.
        for(unsigned i=0;;++i){
            const auto n=hologram_read<int>(object,0xc);
            if(n<=0||i>=static_cast<unsigned>(n))break;
            if(n>0x100000)return -1;
            void* list=nullptr;void* event=nullptr;
            if(!choreography_ramp_storage(h,hologram_read<std::uint32_t>(object,0),(i+1)*4,list)||
               !choreography_ramp_storage(h,hologram_read<std::uint32_t>(list,i*4),0x18c,event))return -1;
            const auto time=hologram_read<float>(event,hologram_read<std::uint32_t>(event,0x188)==0xbf800000u?0x184:0x188);
            if(time>duration)duration=time;
        }
    }
    // 2210ec20/221144e0: remove backwards, retaining unordered samples,
    // and allow 0.01 seconds beyond the original single-precision duration.
    for(int i=count-1;i>=0;--i){
        const float time=hologram_read<float>(rows+i*12,4);
        if(time<0||static_cast<double>(time)>static_cast<double>(duration)+0.01){
            if(i+1<count)std::memmove(rows+i*12,rows+(i+1)*12,(count-i-1)*12);
            hologram_write(v,0xc,--count);
        }
    }
    return 1;
}
int __cdecl choreography_ramp_block(void* object,unsigned bytes,ChoreographyRampKind kind,const ChoreographyTimingTagBindings* b) noexcept {
    if(!choreography_ramp_object(object,bytes,kind)||!choreography_tags_ready(b)||!b->number||
       !b->text.next||!b->text.token||!b->text.unexpected)return 0;
    const auto h=*b;char token[1024]{};
    auto next=[&](unsigned cross){return choreography_actor_next(h.text,cross,token);};
    hologram_write(object,choreography_ramp_offset(kind)+0xc,0u);
    if(!next(1))return -1;
    if(!choreography_text_equal(token,"{")&&!h.text.unexpected(h.text.context,token))return -1;
    for(unsigned i=0;i<=4096;++i){
        if(!next(1)||!*token)return -1;
        if(choreography_text_equal(token,"}"))return choreography_ramp_finish(object,bytes,kind,&h);
        float time=0,value=0;
        if(!h.number(h.context,token,&time)||!next(0)||!h.number(h.context,token,&value)||
           choreography_ramp_append(object,bytes,kind,time,value,0,&h)!=1)return -1;
    }
    return -1;
}
const ChoreographyRampApi choreography_ramp_api{version,sizeof(ChoreographyRampApi),&choreography_ramp_block,&choreography_ramp_append,&choreography_ramp_finish};
bool choreography_scale_ready(const ChoreographyScaleBindings* b) noexcept {
    return b&&b->storage&&hologram_writable(b->sentinel,sizeof(ChoreographyScaleSentinel));
}
bool choreography_scale_row(void* parsed,std::uint32_t index,const ChoreographyScaleBindings& h,void*& row) noexcept {
    const int high=hologram_read<int>(parsed,0x15c),capacity=hologram_read<int>(parsed,0x148);
    if(high<0||capacity<high||index>=static_cast<unsigned>(high)||index>=0x100000)return false;
    const auto word=hologram_read<std::uint32_t>(parsed,0x144);
    void* rows=nullptr;unsigned n=0;const auto need=(index+1)*24;
    if(!word||!h.storage(h.context,word,&rows,&n)||n<need||!hologram_writable(rows,need))return false;
    row=static_cast<unsigned char*>(rows)+index*24;return true;
}
int __cdecl choreography_scale_find(void* parsed,unsigned bytes,std::uint16_t symbol,const ChoreographyScaleBindings* b,std::uint32_t* out) noexcept {
    if(bytes<0x220||!hologram_writable(parsed,0x220)||!choreography_scale_ready(b)||!hologram_writable(out,4))return 0;
    const auto h=*b;auto index=hologram_read<std::uint32_t>(parsed,0x150);
    // 22114c70 calls comparator 22112550 (unsigned 16-bit symbol order).
    // Reproduce lazy shared sentinel on descent, not on equal/root absent.
    for(unsigned depth=0;index!=0xffffffffu;++depth){
        if(depth>=128)return -1;
        void* row=nullptr;if(!choreography_scale_row(parsed,index,h,row))return -1;
        const auto key=hologram_read<std::uint16_t>(row,0x10);
        if(symbol==key)break;
        if(!(h.sentinel->flags&1)){
            h.sentinel->flags|=1;h.sentinel->left=h.sentinel->right=h.sentinel->parent=0xffffffffu;h.sentinel->color=1;
        }
        index=hologram_read<std::uint32_t>(row,symbol<key?0:4);
    }
    *out=index;return 1;
}
// Original scale tree (22119787..221198a2) and rebalance 22115310.
// Preserve indices and the free-list/high-water ABI; allocator stays hosted.
int __cdecl choreography_scale_insert(void* p,void* object,std::uint16_t symbol,std::uint32_t* out) noexcept {
    const auto* b=static_cast<const ChoreographyScaleTreeBindings*>(p);
    if(!b||!b->storage||!b->grow_memory||!hologram_writable(b->sentinel,sizeof(ChoreographyScaleSentinel))||
       !hologram_writable(object,0x24)||!hologram_writable(out,4)||symbol==0xffff)return 0;
    const auto h=*b;
    using T=sin_port::choreography_index::Tree<std::uint32_t,24>;
    const unsigned high=hologram_read<unsigned>(object,0x1c),live=hologram_read<unsigned>(object,0x14);
    const unsigned cap=hologram_read<unsigned>(object,8);
    if(high>cap||cap>4096||live>high)return 0;
    auto resolve=[&](unsigned n,void*& rows) noexcept {
        rows=nullptr;if(!n)return true;
        unsigned extent=0;const auto w=hologram_read<std::uint32_t>(object,4);
        return w&&h.storage(h.context,w,&rows,&extent)&&extent>=n*24&&hologram_writable(rows,n*24);
    };
    void* rows=nullptr;if(!resolve(high,rows))return 0;
    T t{static_cast<unsigned char*>(rows),high,hologram_read<unsigned>(object,0x10)};
    auto key=[&](unsigned i) noexcept {return hologram_read<std::uint16_t>(t.rows,i*24+0x10);};
    if(!t.valid(live,[&](unsigned a,unsigned z) noexcept {return key(a)<key(z);}))return 0;
    const auto free_head=hologram_read<unsigned>(object,0x18);
    unsigned free_count=0;
    for(auto f=free_head;f!=T::nil;f=t.get(f,1)){
        if(!t.index(f)||t.get(f,0)!=f||++free_count>high-live)return 0;
    }
    if(free_count!=high-live)return 0;
    unsigned parent=T::nil,side=0;
    for(auto i=t.root;i!=T::nil;i=t.get(i,side)){
        if(symbol==key(i)){*out=i;return 1;}
        parent=i;side=symbol<key(i)?0:1;
    }
    unsigned slot=free_head;
    if(slot==T::nil){
        if(high>=4096)return 0;
        if(high==cap){
            if(hologram_read<int>(object,0xc)==-1||
               !h.grow_memory(h.context,static_cast<unsigned char*>(object)+4,1,24)||
               hologram_read<unsigned>(object,8)<=high||hologram_read<unsigned>(object,8)>4096||
               hologram_read<unsigned>(object,0x1c)!=high||hologram_read<unsigned>(object,0x14)!=live)return 0;
        }
        if(!resolve(high+1,rows))return 0;
        t.rows=static_cast<unsigned char*>(rows);t.count=high+1;slot=high;
        hologram_write(object,0x1c,high+1);
    }else{
        hologram_write(object,0x18,t.get(slot,1));
    }
    if(parent!=T::nil||free_head!=T::nil){
        if(!(h.sentinel->flags&1)){
            h.sentinel->flags|=1;h.sentinel->left=h.sentinel->right=h.sentinel->parent=T::nil;h.sentinel->color=1;
        }
    }
    // 22119821 writes invalid symbol before linking/rebalancing, then
    // stores key and default scale 100 after the node is live.
    auto* row=t.rows+slot*24;
    hologram_write(row,0x10,static_cast<std::uint16_t>(0xffff));
    t.link(slot,parent,side);
    hologram_write(object,0x10,t.root);hologram_write(object,0x14,live+1);
    hologram_write(object,0x20,hologram_read<std::uint32_t>(object,4));
    hologram_write(row,0x10,symbol);hologram_write(row,0x14,100);
    *out=slot;return 1;
}
const ChoreographyScaleTreeApi choreography_scale_tree_api{version,sizeof(ChoreographyScaleTreeApi),&choreography_scale_insert};
int __cdecl choreography_scale_block(void* parsed,unsigned bytes,const ChoreographyScaleBindings* b) noexcept {
    if(bytes<0x220||!hologram_writable(parsed,0x220)||!choreography_scale_ready(b)||!b->integer||!b->intern||!b->insert||
       !b->text.next||!b->text.token||!b->text.unexpected)return 0;
    const auto h=*b;char token[1024]{};
    auto next=[&](unsigned cross){return choreography_actor_next(h.text,cross,token);};
    if(!next(1))return -1;
    if(!choreography_text_equal(token,"{")&&!h.text.unexpected(h.text.context,token))return -1;
    for(unsigned i=0;i<4096;++i){
        if(!next(1)||!*token)return -1;
        if(choreography_text_equal(token,"}"))return 1;
        char key[256]{};
        if(!choreography_text_copy(key,sizeof(key),token)||!next(0))return -1;
        int value=0;if(!h.integer(h.context,token,&value))return -1;
        if(value<=0)value=100;
        auto* symbol_table=static_cast<unsigned char*>(parsed)+0x164;
        std::uint16_t symbol=0xffff;
        if(!h.intern(h.context,symbol_table,key,&symbol)||symbol==0xffff)return -1;
        std::uint32_t index=0xffffffffu;
        if(choreography_scale_find(parsed,bytes,symbol,&h,&index)!=1)return -1;
        if(index==0xffffffffu){
            // Original interns a second time on insertion path.
            std::uint16_t repeated=0xffff;
            if(!h.intern(h.context,symbol_table,key,&repeated)||repeated!=symbol||
               !h.insert(h.context,static_cast<unsigned char*>(parsed)+0x140,symbol,&index))return -1;
        }
        void* row=nullptr;
        if(!choreography_scale_row(parsed,index,h,row)||hologram_read<std::uint16_t>(row,0x10)!=symbol)return -1;
        hologram_write(row,0x14,value);
    }
    return -1;
}
const ChoreographyScaleApi choreography_scale_api{version,sizeof(ChoreographyScaleApi),&choreography_scale_find,&choreography_scale_block};
// Original 221108b0 / 22110ff0 / 2210f950 / 22110910.
int __cdecl choreography_flex_construct(void* track,unsigned bytes,void* event,std::uint32_t vtable) noexcept {
    if(bytes<0x50||!hologram_writable(track,0x50)||!vtable)return 0;
    hologram_write(track,0,vtable);
    hologram_write(track,4,0u);
    hologram_write(track,8,static_cast<unsigned char>(0));
    hologram_write(track,9,static_cast<unsigned char>(0));
    for(unsigned off=0xc;off<0x3c;off+=4)hologram_write(track,off,0u);
    for(unsigned off=0x3c;off<0x4c;off+=4)hologram_write(track,off,0xffffffffu);
    hologram_write(track,0x4c,scene_word(event));
    return 1;
}
bool choreography_flex_owner_ready(const ChoreographyFlexOwnershipBindings* b) noexcept {
    return b&&b->vtable&&b->allocate&&b->release&&b->storage&&b->grow;
}
bool choreography_flex_owner_storage(const ChoreographyFlexOwnershipBindings& h,std::uint32_t word,unsigned need,void*& out) noexcept {
    unsigned n=0;out=nullptr;
    return word&&h.storage(h.context,word,&out,&n)&&n>=need&&hologram_writable(out,need);
}
int __cdecl choreography_flex_owned_create(void* p,void* event,const char* name,void** out,unsigned* bytes) noexcept {
    const auto* b=static_cast<const ChoreographyFlexOwnershipBindings*>(p);
    if(!choreography_flex_owner_ready(b)||!hologram_writable(event,0x3c0)||!name||!out||!bytes)return 0;
    *out=nullptr;*bytes=0;
    const auto h=*b;
    char label[1024]{};
    // Exact copy for supported input; reject overlong/nonterminated name.
    unsigned length=0;
    for(;length<sizeof(label);++length){
        if(!sin_independent::vgui_layout::Readable(name+length,1))return 0;
        if(!(label[length]=name[length]))break;
    }
    if(length==sizeof(label))return 0;
    ++length;
    auto* v=static_cast<unsigned char*>(event)+0x30c;
    const int count=hologram_read<int>(v,0xc),capacity=hologram_read<int>(v,4);
    if(count<0||capacity<count||count>=4096)return 0;
    void *track=nullptr,*text=nullptr;unsigned extent=0,n=0;
    // No publication until track, name and vector storage are valid.
    if(!h.allocate(h.context,0x50,ChoreographyFlexAllocation::Track,&track,&extent)||!track)return 0;
    auto fail=[&]() noexcept -> int {
        bool ok=true;
        if(text)ok=h.release(h.context,scene_word(text),ChoreographyFlexAllocation::Name)!=0;
        if(!h.release(h.context,scene_word(track),ChoreographyFlexAllocation::Track))ok=false;
        (void)ok;return 0;
    };
    if(choreography_flex_construct(track,extent,event,h.vtable)!=1)return fail();
    if(!h.allocate(h.context,length,ChoreographyFlexAllocation::Name,&text,&n)||!text||
       n<length||!sin_independent::vgui_layout::Readable(text,length))return fail();
    // Original memcpy into allocator-returned writable bytes. Allocator
    // contract guarantees writable name storage (which may be unaligned).
    std::memcpy(text,label,length);hologram_write(track,4,scene_word(text));
    if(count==capacity&&(!h.grow(h.context,v,1,4)||hologram_read<int>(v,0xc)!=count||
        hologram_read<int>(v,4)<=count))return fail();
    void* rows=nullptr;
    const auto word=hologram_read<std::uint32_t>(v,0);
    if(!choreography_flex_owner_storage(h,word,(count+1)*4,rows))return fail();
    hologram_write(rows,count*4,scene_word(track));
    hologram_write(v,0x10,word);hologram_write(v,0xc,count+1);
    *out=track;*bytes=extent;return 1;
}
int __cdecl choreography_flex_owned_destroy(void* p,std::uint32_t word,unsigned flags) noexcept {
    const auto* b=static_cast<const ChoreographyFlexOwnershipBindings*>(p);
    if(!choreography_flex_owner_ready(b)||!word)return 0;
    const auto h=*b;void* track=nullptr;
    if(!choreography_flex_owner_storage(h,word,0x50,track)||hologram_read<std::uint32_t>(track,0)!=h.vtable)return 0;
    // 2210f950 releases name, purges forward, then revisits banks in
    // reverse. Clear consumed ownership after release; preserve borrowed
    // buffers, capacity and mirrors exactly as the original does.
    hologram_write(track,0,h.vtable);
    const auto name=hologram_read<std::uint32_t>(track,4);
    if(name){
        hologram_write(track,4,0u); // defensive: consumed on failure too
        if(!h.release(h.context,name,ChoreographyFlexAllocation::Name))return 0;
    }
    for(unsigned bank=0;bank<2;++bank){
        auto* v=static_cast<unsigned char*>(track)+0x14+bank*0x14;
        hologram_write(v,0xc,0u);
        if(hologram_read<int>(v,8)!=-1){
            const auto data=hologram_read<std::uint32_t>(v,0);
            hologram_write(v,0,0u);hologram_write(v,4,0u);hologram_write(v,0x10,0u);
            if(data&&!h.release(h.context,data,ChoreographyFlexAllocation::SampleVector))return 0;
        }else hologram_write(v,0x10,hologram_read<std::uint32_t>(v,0));
    }
    // Reverse pass has no further releases after the successful forward pass.
    if((flags&1)&&!h.release(h.context,word,ChoreographyFlexAllocation::Track))return 0;
    return 1;
}
const ChoreographyFlexOwnershipApi choreography_flex_ownership_api{
    version,sizeof(ChoreographyFlexOwnershipApi),&choreography_flex_construct,&choreography_flex_owned_create,&choreography_flex_owned_destroy};
bool choreography_flex_ready(const ChoreographyFlexBindings* b) noexcept {
    return b&&b->create&&b->delete_aux&&choreography_tags_ready(&b->samples);
}
bool choreography_flex_clear(void* event,const ChoreographyFlexBindings& h) noexcept {
    for(unsigned i=0;;++i){
        const int count=hologram_read<int>(event,0x318);
        if(!count)break;
        if(count<0||count>4096||i>=4096)return false;
        void* rows=nullptr;
        if(!choreography_ramp_storage(h.samples,hologram_read<std::uint32_t>(event,0x30c),count*4,rows))return false;
        const auto word=hologram_read<std::uint32_t>(rows,0);
        if(!word)return false;
        if(count>1)std::memmove(rows,static_cast<unsigned char*>(rows)+4,(count-1)*4);
        hologram_write(event,0x318,count-1);
        if(!h.delete_aux(h.context,word,1))return false;
    }
    hologram_write(event,0x320,static_cast<unsigned char>(0));return true;
}
bool choreography_flex_append(void* track,unsigned bank,float time,float value,const ChoreographyTimingTagBindings& h) noexcept {
    auto* v=static_cast<unsigned char*>(track)+0x14+bank*0x14;
    const int count=hologram_read<int>(v,0xc),cap=hologram_read<int>(v,4);
    if(count<0||cap<count||count>=4096)return false;
    if(count==cap&&(!h.grow(h.context,v,1,12)||hologram_read<int>(v,0xc)!=count||hologram_read<int>(v,4)<=count))return false;
    const auto word=hologram_read<std::uint32_t>(v,0);void* data=nullptr;
    if(!choreography_ramp_storage(h,word,(count+1)*12,data))return false;
    auto* row=static_cast<unsigned char*>(data)+count*12;
    hologram_write(row,0,value);hologram_write(row,4,time);hologram_write(row,8,0u);
    hologram_write(v,0xc,count+1);hologram_write(v,0x10,word);return true;
}
bool choreography_flex_finish(void* track,float duration,const ChoreographyTimingTagBindings& h) noexcept {
    // 2210fbb0 sorts requested bank, then 2210dbd0 trims BOTH banks.
    // Original invokes it for bank 0 then 1, no ramp's +0.01 tolerance.
    for(unsigned sort=0;sort<2;++sort){
        for(unsigned bank=0;bank<2;++bank){
            auto* v=static_cast<unsigned char*>(track)+0x14+bank*0x14;
            int count=hologram_read<int>(v,0xc);
            if(count<0||count>4096||hologram_read<int>(v,4)<count)return false;
            void* data=nullptr;
            if(count&&!choreography_ramp_storage(h,hologram_read<std::uint32_t>(v,0),count*12,data))return false;
            auto* rows=static_cast<unsigned char*>(data);
            if(bank==sort){
                for(int i=0;i<count;++i)for(int j=i+1;j<count;++j){
                    auto* l=rows+i*12;auto* r=rows+j*12;
                    if(hologram_read<float>(l,4)>hologram_read<float>(r,4)){
                        unsigned char tmp[12];std::memcpy(tmp,l,12);std::memcpy(l,r,12);std::memcpy(r,tmp,12);
                    }
                }
            }
            for(int i=count-1;i>=0;--i){
                const auto time=hologram_read<float>(rows+i*12,4);
                if(time<0||time>duration){
                    if(i+1<count)std::memmove(rows+i*12,rows+(i+1)*12,(count-i-1)*12);
                    hologram_write(v,0xc,--count);
                }
            }
        }
    }
    return true;
}
int __cdecl choreography_flex_block(void* event,unsigned bytes,const ChoreographyFlexBindings* b) noexcept {
    if(bytes<0x3c0||!hologram_writable(event,0x3c0)||!choreography_flex_ready(b)||!b->samples.number||
       !b->samples.text.next||!b->samples.text.token||!b->samples.text.unexpected)return 0;
    const auto h=*b;const auto& s=h.samples;char token[1024]{};
    if(!choreography_flex_clear(event,h))return -1;
    const float span=hologram_read<float>(event,0x188)-hologram_read<float>(event,0x184);
    const float duration=hologram_read<std::uint32_t>(event,0x188)==0xbf800000u?0.f:span;
    auto next=[&](unsigned cross){return choreography_actor_next(s.text,cross,token);};
    auto brace=[&](){return choreography_text_equal(token,"{")||s.text.unexpected(s.text.context,token)!=0;};
    if(!next(1))return -1;
    const bool actual_time=choreography_text_equal(token,"samples_use_time");
    if(actual_time&&!next(1))return -1;
    if(!brace())return -1;
    struct Sample {float time,value;};
    Sample samples[2][4096];
    for(unsigned iteration=0;iteration<4096;++iteration){
        if(!next(1)||!*token)return -1;
        if(choreography_text_equal(token,"}"))return 1;
        char name[128]{};
        if(!choreography_text_copy(name,sizeof(name),token)||!next(1))return -1;
        bool active=true,combo=false;float minimum=0,maximum=1;
        if(choreography_text_equal(token,"disabled")){active=false;if(!next(1))return -1;}
        if(choreography_text_equal(token,"combo")){combo=true;if(!next(1))return -1;}
        if(choreography_text_equal(token,"range")){
            if(!next(0)||!s.number(s.context,token,&minimum)||!next(0)||!s.number(s.context,token,&maximum)||!next(1))return -1;
        }
        unsigned counts[2]{};
        for(unsigned bank=0;bank<(combo?2u:1u);++bank){
            if(bank&&!next(1))return -1;
            if(!brace())return -1;
            for(;;){
                if(!next(1)||!*token)return -1;
                if(choreography_text_equal(token,"}"))break;
                if(counts[bank]>=4096)return -1;
                float time=0,value=0;
                if(!s.number(s.context,token,&time)||!next(0)||!s.number(s.context,token,&value))return -1;
                samples[bank][counts[bank]++]={actual_time?time:time*span,value};
            }
        }
        // Original omits disabled tracks with no primary samples, even
        // if the combo's secondary vector is nonempty.
        if(!active&&!counts[0])continue;
        void* track=nullptr;unsigned extent=0;
        const int old=hologram_read<int>(event,0x318);
        if(old<0||old>=4096||!h.create(h.context,event,name,&track,&extent)||extent<0x50||!hologram_writable(track,0x50)||
           hologram_read<int>(event,0x318)!=old+1||hologram_read<std::uint32_t>(track,0x4c)!=scene_word(event)||
           hologram_read<int>(track,0x20)||hologram_read<int>(track,0x34))return -1;
        void* rows=nullptr;
        if(!choreography_ramp_storage(s,hologram_read<std::uint32_t>(event,0x30c),(old+1)*4,rows)||
           hologram_read<std::uint32_t>(rows,old*4)!=scene_word(track))return -1;
        hologram_write(track,8,static_cast<unsigned char>(active));hologram_write(track,9,static_cast<unsigned char>(combo));
        hologram_write(track,0xc,minimum);hologram_write(track,0x10,maximum);
        for(unsigned bank=0;bank<2;++bank)
            for(unsigned j=0;j<counts[bank];++j)
                if(!choreography_flex_append(track,bank,samples[bank][j].time,samples[bank][j].value,s))return -1;
        if(!choreography_flex_finish(track,duration,s))return -1;
    }
    return -1;
}
const ChoreographyFlexApi choreography_flex_api{version,sizeof(ChoreographyFlexApi),&choreography_flex_block};
bool choreography_finalize_ready(const ChoreographyFinalizeBindings* b) noexcept {
    return b&&b->storage&&b->missing_tag;
}
bool choreography_finalize_storage(const ChoreographyFinalizeBindings& h,std::uint32_t word,unsigned need,void*& object) noexcept {
    unsigned n=0;object=nullptr;
    return word&&h.storage(h.context,word,&object,&n)&&n>=need&&hologram_writable(object,need);
}
bool choreography_finalize_child(const ChoreographyFinalizeBindings& h,void* parent,unsigned offset,unsigned i,unsigned need,void*& object) noexcept {
    void* list=nullptr;object=nullptr;
    if(!choreography_finalize_storage(h,hologram_read<std::uint32_t>(parent,offset),(i+1)*4,list))return false;
    const auto word=hologram_read<std::uint32_t>(list,i*4);
    return !word||choreography_finalize_storage(h,word,need,object);
}
// Original 22115b80/22114230, 221145a0 and 22114600 all traverse
// parsed actors -> actor channels, not the flat event/channel lists.
template<class Visit>
int choreography_visit_channels(void* parsed,const ChoreographyFinalizeBindings& h,Visit visit) noexcept {
    unsigned budget=0x100000;
    for(unsigned a=0;;++a){
        const int count=hologram_read<int>(parsed,0x20);
        if(count<=0||a>=static_cast<unsigned>(count))break;
        if(count>0x100000||!budget--)return -1;
        void* actor=nullptr;if(!choreography_finalize_child(h,parsed,0x14,a,0x118,actor))return -1;
        if(!actor)continue;
        for(unsigned c=0;;++c){
            const int n=hologram_read<int>(actor,0x10c);
            if(n<=0||c>=static_cast<unsigned>(n))break;
            if(n>0x100000||!budget--)return -1;
            void* channel=nullptr;if(!choreography_finalize_child(h,actor,0x100,c,0x9c,channel))return -1;
            if(!channel)continue;
            const int status=visit(channel);
            if(status!=1)return status;
        }
    }
    return 1;
}
template<class Visit>
int choreography_visit_linked_events(void* parsed,const ChoreographyFinalizeBindings& h,Visit visit) noexcept {
    unsigned budget=0x100000;
    return choreography_visit_channels(parsed,h,[&](void* channel){
        for(unsigned e=0;;++e){
            const int count=hologram_read<int>(channel,0x90);
            if(count<=0||e>=static_cast<unsigned>(count))break;
            if(count>0x100000||!budget--)return -1;
            void* event=nullptr;if(!choreography_finalize_child(h,channel,0x84,e,0x3c0,event))return -1;
            if(!event)continue;
            const int status=visit(event);if(status!=1)return status;
        }
        return 1;
    });
}
int choreography_find_relative(void* parsed,const char* reference,const char* tag,const ChoreographyFinalizeBindings& h,void*& found) noexcept {
    found=nullptr;
    return choreography_visit_linked_events(parsed,h,[&](void* event){
        if(hologram_read<unsigned>(event,0)!=5)return 1;
        const auto* sound=static_cast<const char*>(event)+0x84;
        if(!std::memchr(sound,0,128))return -1;
        // 223a5750 is case-sensitive substring search, not equality.
        if(!std::strstr(sound,reference))return 1;
        for(unsigned i=0;;++i){
            const int count=hologram_read<int>(event,0x2c8);
            if(count<=0||i>=static_cast<unsigned>(count))break;
            if(count>4096)return -1;
            void* rows=nullptr;
            if(!choreography_finalize_storage(h,hologram_read<std::uint32_t>(event,0x2bc),(i+1)*0x88,rows))return -1;
            auto* row=static_cast<char*>(rows)+i*0x88;
            if(!std::memchr(row,0,128))return -1;
            if(choreography_text_equal(row,tag)){found=row;return 2;}
        }
        return 1;
    });
}
// Original 2210a330 orders type-6 events by their pre-reconciliation
// start times. Equal keys enter the right branch (22108200/22107650).
// Bounded native storage: reject unordered start keys before mutation;
// the original RB tree's NaN placement is not a strict weak ordering.
constexpr unsigned gesture_limit=4096;
double gesture_span(const void* event) noexcept {
    return hologram_read<std::uint32_t>(event,0x188)==0xbf800000u?0.0:
        static_cast<double>(hologram_read<float>(event,0x188))-hologram_read<float>(event,0x184);
}
bool gesture_rows(void* event,const ChoreographyFinalizeBindings& h,void*& rows,int& count) noexcept {
    count=hologram_read<int>(event,0x2f0);rows=nullptr;
    return count>=0&&count<=static_cast<int>(gesture_limit)&&
        (!count||choreography_finalize_storage(h,hologram_read<std::uint32_t>(event,0x2e4),count*0x8c,rows));
}
void* gesture_tag(void* rows,int count,unsigned char flag) noexcept {
    for(int i=0;i<count;++i){
        auto* row=static_cast<unsigned char*>(rows)+i*0x8c;
        if(hologram_read<unsigned char>(row,0x84)&flag)return row;
    }
    return nullptr;
}
bool gesture_tag_owner(void* tag,const ChoreographyFinalizeBindings& h,void*& owner) noexcept {
    owner=nullptr;const auto w=hologram_read<std::uint32_t>(tag,0x88);
    return !w||choreography_finalize_storage(h,w,0x18c,owner);
}
// 2210f000: walk backwards, constrain each fraction to the running
// ceiling, shrink spacing only after a clamp (min(.01, ceiling/(i+1))).
void gesture_normalize(void* rows,int count) noexcept {
    double ceiling=1.0;float spacing=.01f;
    for(int i=count-1;i>=0;--i){
        auto* row=static_cast<unsigned char*>(rows)+i*0x8c;
        double f=hologram_read<float>(row,0x80);
        if(ceiling<f){
            hologram_write(row,0x80,static_cast<float>(ceiling));f=ceiling;
            const double next=ceiling/(i+1);spacing=static_cast<float>(next<.01?next:.01);
        }
        const double next=f-spacing;ceiling=next>0?next:0;
    }
}
// 221105a0 calls 2210fbb0 for bank zero only; that routine trims
// both banks. Ownerless tracks are sorted but not trimmed (2210dbd0).
bool gesture_refresh(void* event,const ChoreographyFinalizeBindings& h) noexcept {
    const int count=hologram_read<int>(event,0x318);void* list=nullptr;
    if(count<0||count>static_cast<int>(gesture_limit)||
       (count&&!choreography_finalize_storage(h,hologram_read<std::uint32_t>(event,0x30c),count*4,list)))return false;
    for(int i=0;i<count;++i){
        const auto w=hologram_read<std::uint32_t>(list,i*4);if(!w)continue;
        void* track=nullptr;if(!choreography_finalize_storage(h,w,0x50,track))return false;
        const auto ownerword=hologram_read<std::uint32_t>(track,0x4c);void* owner=nullptr;
        if(ownerword&&!choreography_finalize_storage(h,ownerword,0x18c,owner))return false;
        const float duration=owner?static_cast<float>(gesture_span(owner)):0;
        for(unsigned bank=0;bank<2;++bank){
            auto* v=static_cast<unsigned char*>(track)+0x14+bank*0x14;
            int n=hologram_read<int>(v,0xc);void* data=nullptr;
            if(n<0||n>static_cast<int>(gesture_limit)||hologram_read<int>(v,4)<n||
               (n&&!choreography_finalize_storage(h,hologram_read<std::uint32_t>(v,0),n*12,data)))return false;
            auto* rows=static_cast<unsigned char*>(data);
            if(!bank)for(int a=0;a<n;++a)for(int b=a+1;b<n;++b){
                auto* l=rows+a*12;auto* r=rows+b*12;
                if(hologram_read<float>(l,4)>hologram_read<float>(r,4)){
                    unsigned char tmp[12];std::memcpy(tmp,l,12);std::memcpy(l,r,12);std::memcpy(r,tmp,12);
                }
            }
            if(owner)for(int j=n-1;j>=0;--j){
                const auto t=hologram_read<float>(rows+j*12,4);
                if(t<0||t>duration){
                    if(j+1<n)std::memmove(rows+j*12,rows+(j+1)*12,(n-j-1)*12);
                    hologram_write(v,0xc,--n);
                }
            }
        }
    }
    return true;
}
bool choreography_gesture_channel(void* channel,const ChoreographyFinalizeBindings& h) noexcept {
    const int count=hologram_read<int>(channel,0x90);
    if(count<=0)return count==0;
    if(count>static_cast<int>(gesture_limit))return false;
    struct Entry {void* event;float start;unsigned ordinal;};
    Entry entries[gesture_limit];unsigned used=0;
    for(int i=0;i<count;++i){
        void* event=nullptr;if(!choreography_finalize_child(h,channel,0x84,i,0x3c0,event))return false;
        if(!event||hologram_read<unsigned>(event,0)!=6)continue;
        const float start=hologram_read<float>(event,0x184);
        if(std::isnan(start))return false;
        entries[used++]={event,start,static_cast<unsigned>(i)};
    }
    std::sort(entries,entries+used,[](const Entry& a,const Entry& b) noexcept {
        if(a.start<b.start)return true;
        if(b.start<a.start)return false;
        return a.ordinal<b.ordinal;
    });
    if(!used)return true;
    // 2210b390: reset first gesture start, clamp an existing end only
    // when it is below the new start; do not refresh flex tracks here.
    auto* previous=entries[0].event;
    hologram_write(previous,0x184,0.f);
    if(hologram_read<std::uint32_t>(previous,0x188)!=0xbf800000u&&hologram_read<float>(previous,0x188)<0)
        hologram_write(previous,0x188,0.f);
    for(unsigned i=1;i<used;++i){
        auto* current=entries[i].event;
        void *inrows=nullptr,*outrows=nullptr;int in_count=0,out_count=0;
        if(!gesture_rows(current,h,inrows,in_count)||!gesture_rows(previous,h,outrows,out_count))return false;
        auto* entry=gesture_tag(inrows,in_count,4);auto* exit=gesture_tag(outrows,out_count,8);
        if(entry&&exit){
            void* owner=nullptr;if(!gesture_tag_owner(entry,h,owner))return false;
            const float absolute=owner?static_cast<float>(hologram_read<float>(owner,0x184)+gesture_span(owner)*hologram_read<float>(entry,0x80)):0.f;
            const float old_span=static_cast<float>(gesture_span(previous));
            const float end=static_cast<float>(absolute+(1.0-hologram_read<float>(exit,0x80))*old_span);
            const float old_start=hologram_read<float>(previous,0x184),old_end=hologram_read<float>(previous,0x188);
            // 2210ecf0 remaps the old playback fractions before changing
            // the end; finite values clamp to [0,1], unordered survives.
            if(old_end!=end){
                const float new_span=end-old_start;const double reciprocal=1.0/new_span;
                for(int j=0;j<out_count;++j){
                    auto* row=static_cast<unsigned char*>(outrows)+j*0x8c;
                    double f=hologram_read<float>(row,0x80)*gesture_span(previous)*reciprocal;
                    if(f>1)f=1;else if(f<0)f=0;
                    hologram_write(row,0x80,static_cast<float>(f));
                }
            }
            // 22110f20 stores end, clamps unless exact -1, refreshes
            // flex samples if the original requested end differs.
            hologram_write(previous,0x188,end);
            if(end!=-1.f){
                if(end<old_start)hologram_write(previous,0x188,old_start);
                if(old_end!=end&&!gesture_refresh(previous,h))return false;
            }
            // 2210c550 recomputes the prior exit's fraction using its
            // owner, even when the owner span is zero (IEEE division).
            if(!gesture_tag_owner(exit,h,owner))return false;
            if(owner)hologram_write(exit,0x80,static_cast<float>((static_cast<double>(absolute)-hologram_read<float>(owner,0x184))/gesture_span(owner)));
            gesture_normalize(inrows,in_count);gesture_normalize(outrows,out_count);
        }
        previous=current;
    }
    return true;
}
// 2210bb20/70/90/b0/d0: reset type optionally; reset combined flag,
// checksum, follower count and last follower end, without touching token.
void choreography_caption_reset(void* event,bool master) noexcept {
    if(master)hologram_write(event,0x34c,0u);
    hologram_write(event,0x390,static_cast<unsigned char>(0));
    hologram_write(event,0x394,0u);hologram_write(event,0x398,0u);hologram_write(event,0x39c,0.f);
}
bool choreography_caption_channel(void* channel,const ChoreographyFinalizeBindings& h) noexcept {
    const int count=hologram_read<int>(channel,0x90);
    if(count<=0)return count==0;
    if(count>static_cast<int>(gesture_limit))return false;
    // 2210aa90 creates a private case-insensitive symbol table and per-
    // token start-time RB trees. Use first-seen group IDs and stable finite
    // ordering, avoiding a temporary PE32 allocator and symbol callback.
    struct Entry {void* event;float start;unsigned group,ordinal;};
    Entry entries[gesture_limit];const char* keys[gesture_limit];
    unsigned used=0,groups=0;
    for(int i=0;i<count;++i){
        void* event=nullptr;if(!choreography_finalize_child(h,channel,0x84,i,0x3c0,event))return false;
        if(!event||hologram_read<unsigned>(event,0)!=5)continue;
        const unsigned type=hologram_read<unsigned>(event,0x34c);
        if(type==2){choreography_caption_reset(event,false);continue;}
        const auto* token=static_cast<const char*>(event)+0x350;
        if(!std::memchr(token,0,64))return false;
        if(!*token){
            if(type==1)choreography_caption_reset(event,true);
            continue;
        }
        const float start=hologram_read<float>(event,0x184);
        if(std::isnan(start))return false;
        unsigned group=0;
        while(group<groups&&!choreography_text_equal(keys[group],token))++group;
        if(group==groups)keys[groups++]=token;
        entries[used++]={event,start,group,static_cast<unsigned>(i)};
    }
    std::sort(entries,entries+used,[](const Entry& a,const Entry& b) noexcept {
        if(a.group!=b.group)return a.group<b.group;
        if(a.start<b.start)return true;
        if(b.start<a.start)return false;
        return a.ordinal<b.ordinal;
    });
    for(unsigned i=0;i<used;){
        unsigned end=i+1;while(end<used&&entries[end].group==entries[i].group)++end;
        auto* master=entries[i].event;
        if(end-i==1)choreography_caption_reset(master,true);
        else{
            // 2210acc3: preserve the master's combined flag. Each later
            // event inherits it, while the master's end is overwritten in
            // traversal order, NOT max(end). Slave follower count untouched.
            hologram_write(master,0x34c,0u);hologram_write(master,0x394,0u);
            hologram_write(master,0x398,end-i-1);
            hologram_write(master,0x39c,hologram_read<float>(master,0x188));
            for(unsigned j=i+1;j<end;++j){
                auto* slave=entries[j].event;
                hologram_write(master,0x39c,hologram_read<float>(slave,0x188));
                hologram_write(slave,0x34c,1u);
                hologram_write(slave,0x390,hologram_read<unsigned char>(master,0x390));
                hologram_write(slave,0x394,0u);hologram_write(slave,0x39c,0.f);
            }
        }
        i=end;
    }
    return true;
}
int __cdecl choreography_finalize_stage(void* parsed,unsigned bytes,ChoreographyTextFinalize stage,const ChoreographyFinalizeBindings* b) noexcept {
    if(bytes<0x220||!hologram_writable(parsed,0x220)||!choreography_finalize_ready(b)||static_cast<unsigned>(stage)>2)return 0;
    const auto h=*b;
    if(stage!=ChoreographyTextFinalize::RelativeTags){
        return choreography_visit_channels(parsed,h,[&](void* channel){
            if(stage==ChoreographyTextFinalize::GestureReconcile)
                return (h.gesture_channel?h.gesture_channel(h.context,channel)!=0:choreography_gesture_channel(channel,h))?1:-1;
            return (h.caption_channel?h.caption_channel(h.context,channel)!=0:choreography_caption_channel(channel,h))?1:-1;
        });
    }
    return choreography_visit_linked_events(parsed,h,[&](void* event){
        if(!hologram_read<unsigned char>(event,0x1b1))return 1;
        const auto* tag=static_cast<const char*>(event)+0x1b2;
        const auto* reference=static_cast<const char*>(event)+0x232;
        const auto* name=static_cast<const char*>(event)+4;
        if(!std::memchr(tag,0,128)||!std::memchr(reference,0,128)||!std::memchr(name,0,128))return -1;
        void* row=nullptr;
        const int status=choreography_find_relative(parsed,reference,tag,h,row);
        if(status<0)return -1;
        if(!row){
            if(!h.missing_tag(h.context,parsed,name,reference,tag))return -1;
            hologram_write(event,0x1b1,static_cast<unsigned char>(0));
            std::memset(static_cast<char*>(event)+0x1b2,0,128);std::memset(static_cast<char*>(event)+0x232,0,128);
            return 1;
        }
        float absolute=0;
        const auto owner=hologram_read<std::uint32_t>(row,0x84);
        if(owner){
            void* source=nullptr;
            if(!choreography_finalize_storage(h,owner,0x18c,source))return -1;
            const double start=hologram_read<float>(source,0x184);
            const double span=hologram_read<std::uint32_t>(source,0x188)==0xbf800000u?0.0:
                static_cast<double>(hologram_read<float>(source,0x188))-start;
            absolute=static_cast<float>(start+span*hologram_read<float>(row,0x80));
        }
        // 2210c420 returns absolute time; caller rounds, then rounds
        // delta before 2210b4a0 shifts end (unless sentinel) and start.
        const float delta=absolute-hologram_read<float>(event,0x184);
        if(hologram_read<std::uint32_t>(event,0x188)!=0xbf800000u)
            hologram_write(event,0x188,hologram_read<float>(event,0x188)+delta);
        hologram_write(event,0x184,hologram_read<float>(event,0x184)+delta);
        return 1;
    });
}
int __cdecl choreography_finalize_all(void* parsed,unsigned bytes,const ChoreographyFinalizeBindings* b) noexcept {
    for(auto stage:{ChoreographyTextFinalize::RelativeTags,ChoreographyTextFinalize::GestureReconcile,ChoreographyTextFinalize::CaptionReconcile}){
        const int status=choreography_finalize_stage(parsed,bytes,stage,b);if(status!=1)return status;
    }
    return 1;
}
const ChoreographyFinalizeApi choreography_finalize_api{version,sizeof(ChoreographyFinalizeApi),&choreography_finalize_stage,&choreography_finalize_all};
using PortableTokenizer=sin_port::vcd::Tokenizer;
int __cdecl portable_vcd_begin(void* p,const char* data,unsigned bytes) noexcept {
    if(!hologram_writable(p,sizeof(PortableTokenizer))||!data||
       (bytes&&!sin_independent::vgui_layout::Readable(data,bytes)))return 0;
    return static_cast<PortableTokenizer*>(p)->begin(data,bytes);
}
int __cdecl portable_vcd_next(void* p,unsigned cross,unsigned* more) noexcept {
    if(!hologram_writable(p,sizeof(PortableTokenizer))||!hologram_writable(more,4))return 0;
    return static_cast<PortableTokenizer*>(p)->next(cross,*more);
}
int __cdecl portable_vcd_token(void* p,const char** out) noexcept {
    if(!hologram_writable(p,sizeof(PortableTokenizer))||!hologram_writable(out,sizeof(*out)))return 0;
    const auto& t=*static_cast<PortableTokenizer*>(p);if(t.failed)return 0;
    *out=t.current;return 1;
}
int __cdecl portable_vcd_same_line(void* p,unsigned* more) noexcept {
    if(!hologram_writable(p,sizeof(PortableTokenizer))||!hologram_writable(more,4))return 0;
    const auto& t=*static_cast<PortableTokenizer*>(p);if(t.failed)return 0;
    *more=t.same_line();return 1;
}
int __cdecl portable_vcd_clear(void* p) noexcept {
    if(!hologram_writable(p,sizeof(PortableTokenizer)))return 0;
    *static_cast<PortableTokenizer*>(p)=PortableTokenizer{};return 1;
}
const PortableVcdTokenizerApi portable_vcd_tokenizer_api{version,sizeof(PortableVcdTokenizerApi),&portable_vcd_begin,&portable_vcd_next,&portable_vcd_token,&portable_vcd_same_line,&portable_vcd_clear};
// Per-invocation adapter: all native subparsers share one bounded cursor.
// Preserve original engine allocation/finalization callback contexts.
// Original 221178f2..221179b0 converts legacy ramp tokens into three
// equal-valued samples at duration*(1-a), their midpoint, duration*c.
// 2210b710 returns extended duration; round the two endpoints to float.
bool choreography_legacy_ramp(void* event,float a,float value,float c,const ChoreographyTimingTagBindings& tags) noexcept {
    const double duration=gesture_span(event);
    const float first=static_cast<float>(duration*(1.0-a));
    const float last=static_cast<float>(duration*c);
    const float middle=static_cast<float>((static_cast<double>(first)+last)*0.5);
    for(const auto time:{first,middle,last})
        if(choreography_ramp_append(event,0x3c0,ChoreographyRampKind::Event,time,value,0,&tags)!=1)return false;
    return choreography_ramp_finish(event,0x3c0,ChoreographyRampKind::Event,&tags)==1;
}
struct PortableGrammarState {
    PortableTokenizer cursor{};
    PortableVcdGrammarBindings source{};
    ChoreographyEventParserBindings events{};
    ChoreographyTimingTagBindings tags{};
    ChoreographyScaleBindings scales{};ChoreographyFlexBindings flex{};ChoreographyFinalizeBindings finalize{};
    bool native_curves=false,native_flex=false,native_finalize=false;
    explicit PortableGrammarState(const PortableVcdGrammarBindings& b,const ChoreographyScaleBindings* s=nullptr,const ChoreographyFlexBindings* f=nullptr,const ChoreographyFinalizeBindings* z=nullptr) noexcept:source(b),events(b.events),tags(b.tags),scales(s?*s:ChoreographyScaleBindings{}),flex(f?*f:ChoreographyFlexBindings{}),finalize(z?*z:ChoreographyFinalizeBindings{}),native_curves(s!=nullptr),native_flex(f!=nullptr),native_finalize(z!=nullptr){
        auto& text=events.actors.text;text.context=this;events.context=this;
        text.begin=[](void* p,const char* data,unsigned n) noexcept -> int{return portable_vcd_begin(&self(p).cursor,data,n);};
        text.next=[](void* p,unsigned cross,unsigned* more) noexcept -> int{return portable_vcd_next(&self(p).cursor,cross,more);};
        text.token=[](void* p,const char** token) noexcept -> int{return portable_vcd_token(&self(p).cursor,token);};
        text.block=[](void* p,void* parsed,ChoreographyTextBlock block) noexcept -> int{
            auto& s=self(p);
            if(s.native_curves){
                if(block==ChoreographyTextBlock::SceneRamp)return choreography_ramp_block(parsed,0x220,ChoreographyRampKind::Scene,&s.tags)==1;
                if(block==ChoreographyTextBlock::ScaleSettings)return choreography_scale_block(parsed,0x220,&s.scales)==1;
                return 0;
            }
            return s.source.root_block(s.source.context,parsed,block,&s.cursor);};
        text.unexpected=[](void* p,const char* token) noexcept -> int{
            auto& s=self(p);const auto& h=s.source.events.actors.text;return h.unexpected(h.context,token);};
        text.finalize=[](void* p,void* parsed,ChoreographyTextFinalize stage) noexcept -> int{
            auto& s=self(p);
            if(s.native_finalize)return choreography_finalize_stage(parsed,0x220,stage,&s.finalize)==1;
            const auto& h=s.source.events.actors.text;return h.finalize(h.context,parsed,stage);};
        events.same_line=[](void* p,unsigned* more) noexcept -> int{return portable_vcd_same_line(&self(p).cursor,more);};
        events.number=[](void* p,const char* token,float* out) noexcept -> int{
            const auto& h=self(p).source.events;return h.number(h.context,token,out);};
        events.end_changed=[](void* p,void* event) noexcept -> int{
            auto& s=self(p);const auto& h=s.source.events;
            if(h.end_changed)return h.end_changed(h.context,event);
            if(!s.native_flex)return 0;
            ChoreographyFinalizeBindings storage{};
            storage.context=s.flex.samples.context;storage.storage=s.flex.samples.storage;
            return gesture_refresh(event,storage);};
        events.append_event=[](void* p,void* channel,void* event) noexcept -> int{
            const auto& h=self(p).source.events;return h.append_event(h.context,channel,event);};
        events.legacy_ramp=[](void* p,void* event,float a,float b,float c) noexcept -> int{
            auto& s=self(p);const auto& h=s.source.events;
            if(h.legacy_ramp)return h.legacy_ramp(h.context,event,a,b,c);
            return s.native_curves&&choreography_legacy_ramp(event,a,b,c,s.tags);};
        events.block=[](void* p,void* event,ChoreographyEventBlock block) noexcept -> int{
            auto& s=self(p);
            if(block==ChoreographyEventBlock::Tags||block==ChoreographyEventBlock::AbsoluteTags||block==ChoreographyEventBlock::FlexTimingTags)
                return choreography_tag_block(event,0x3c0,block,&s.tags)==1;
            if(s.native_curves&&block==ChoreographyEventBlock::EventRamp)
                return choreography_ramp_block(event,0x3c0,ChoreographyRampKind::Event,&s.tags)==1;
            if(s.native_flex&&block==ChoreographyEventBlock::FlexAnimations)
                return choreography_flex_block(event,0x3c0,&s.flex)==1;
            return s.source.event_block(s.source.context,event,block,&s.cursor);};
        tags.context=this;tags.text=text;tags.number=events.number;
        if(native_curves)scales.text=text;
        if(native_flex){
            flex.samples.text=text;
            // Forward numeric conversion through original event context, but
            // flex storage/growth have their own preserved host context.
            flex.samples.number=f->samples.number;
        }
        tags.storage=[](void* p,std::uint32_t word,void** out,unsigned* bytes) noexcept -> int{
            const auto& h=self(p).source.tags;return h.storage(h.context,word,out,bytes);};
        tags.grow=[](void* p,void* v,unsigned additional,unsigned stride) noexcept -> int{
            const auto& h=self(p).source.tags;return h.grow(h.context,v,additional,stride);};
    }
    static PortableGrammarState& self(void* p) noexcept{return *static_cast<PortableGrammarState*>(p);}
};
bool portable_grammar_ready(const PortableVcdGrammarBindings* b,bool native_curves=false,bool native_flex=false,bool native_finalize=false) noexcept {
    if(!b||(!native_curves&&!b->root_block)||(!native_flex&&!b->event_block)||!choreography_tags_ready(&b->tags))return false;
    const auto& h=b->events;const auto& text=h.actors.text;
    return text.tokenizer_word&&text.unexpected&&(native_finalize||text.finalize)&&h.number&&(native_flex||h.end_changed)&&h.append_event&&(native_curves||h.legacy_ramp);
}
int __cdecl portable_grammar_parse(void* parsed,unsigned extent,const char* name,const char* data,unsigned bytes,const PortableVcdGrammarBindings* b) noexcept {
    if(!portable_grammar_ready(b))return 0;
    PortableGrammarState s(*b);
    if(!choreography_event_parser_ready(&s.events)||!portable_vcd_begin(&s.cursor,data,bytes))return 0;
    return choreography_text_parse_impl(parsed,extent,name,&s.events.actors.text,&s.events.actors,&s.events);
}
int __cdecl portable_grammar_load(void* scene,unsigned extent,const char* name,const NamedSceneLoadBindings* load,const PortableVcdGrammarBindings* b,std::uint32_t* out) noexcept {
    if(!portable_grammar_ready(b))return 0;
    PortableGrammarState s(*b);
    return choreography_event_load(scene,extent,name,load,&s.events,out);
}
int __cdecl portable_grammar_scene(void* npc,const char* name,float delay,const NativeNamedPlaybackBindings* playback,const PortableVcdGrammarBindings* b,float* out) noexcept {
    if(!playback||!portable_grammar_ready(b))return 0;
    PortableGrammarState s(*b);auto h=*playback;h.events=&s.events;
    return native_named_playback(npc,name,delay,&h,out);
}
const PortableVcdGrammarApi portable_vcd_grammar_api{version,sizeof(PortableVcdGrammarApi),&portable_grammar_parse,&portable_grammar_load,&portable_grammar_scene};
bool portable_curve_grammar_ready(const PortableVcdCurveGrammarBindings* b) noexcept {
    return b&&portable_grammar_ready(&b->grammar,true)&&choreography_scale_ready(&b->scales)&&
        b->scales.integer&&b->scales.intern&&b->scales.insert;
}
int __cdecl portable_curve_parse(void* parsed,unsigned extent,const char* name,const char* data,unsigned bytes,const PortableVcdCurveGrammarBindings* b) noexcept {
    if(!portable_curve_grammar_ready(b))return 0;
    PortableGrammarState s(b->grammar,&b->scales);
    if(!choreography_event_parser_ready(&s.events)||!portable_vcd_begin(&s.cursor,data,bytes))return 0;
    return choreography_text_parse_impl(parsed,extent,name,&s.events.actors.text,&s.events.actors,&s.events);
}
int __cdecl portable_curve_load(void* scene,unsigned extent,const char* name,const NamedSceneLoadBindings* load,const PortableVcdCurveGrammarBindings* b,std::uint32_t* out) noexcept {
    if(!portable_curve_grammar_ready(b))return 0;
    PortableGrammarState s(b->grammar,&b->scales);return choreography_event_load(scene,extent,name,load,&s.events,out);
}
int __cdecl portable_curve_scene(void* npc,const char* name,float delay,const NativeNamedPlaybackBindings* playback,const PortableVcdCurveGrammarBindings* b,float* out) noexcept {
    if(!playback||!portable_curve_grammar_ready(b))return 0;
    PortableGrammarState s(b->grammar,&b->scales);auto h=*playback;h.events=&s.events;
    return native_named_playback(npc,name,delay,&h,out);
}
const PortableVcdCurveGrammarApi portable_vcd_curve_grammar_api{version,sizeof(PortableVcdCurveGrammarApi),&portable_curve_parse,&portable_curve_load,&portable_curve_scene};
bool portable_flex_grammar_ready(const PortableVcdFlexGrammarBindings* b,bool native_finalize=false) noexcept {
    return b&&portable_grammar_ready(&b->curves.grammar,true,true,native_finalize)&&choreography_scale_ready(&b->curves.scales)&&
        b->curves.scales.integer&&b->curves.scales.intern&&b->curves.scales.insert&&choreography_flex_ready(&b->flex)&&b->flex.samples.number;
}
int __cdecl portable_flex_parse(void* parsed,unsigned extent,const char* name,const char* data,unsigned bytes,const PortableVcdFlexGrammarBindings* b) noexcept {
    if(!portable_flex_grammar_ready(b))return 0;
    PortableGrammarState s(b->curves.grammar,&b->curves.scales,&b->flex);
    if(!choreography_event_parser_ready(&s.events)||!portable_vcd_begin(&s.cursor,data,bytes))return 0;
    return choreography_text_parse_impl(parsed,extent,name,&s.events.actors.text,&s.events.actors,&s.events);
}
int __cdecl portable_flex_load(void* scene,unsigned extent,const char* name,const NamedSceneLoadBindings* load,const PortableVcdFlexGrammarBindings* b,std::uint32_t* out) noexcept {
    if(!portable_flex_grammar_ready(b))return 0;
    PortableGrammarState s(b->curves.grammar,&b->curves.scales,&b->flex);
    return choreography_event_load(scene,extent,name,load,&s.events,out);
}
int __cdecl portable_flex_scene(void* npc,const char* name,float delay,const NativeNamedPlaybackBindings* playback,const PortableVcdFlexGrammarBindings* b,float* out) noexcept {
    if(!playback||!portable_flex_grammar_ready(b))return 0;
    PortableGrammarState s(b->curves.grammar,&b->curves.scales,&b->flex);auto h=*playback;h.events=&s.events;
    return native_named_playback(npc,name,delay,&h,out);
}
const PortableVcdFlexGrammarApi portable_vcd_flex_grammar_api{version,sizeof(PortableVcdFlexGrammarApi),&portable_flex_parse,&portable_flex_load,&portable_flex_scene};
bool portable_finalized_ready(const PortableVcdFinalizedGrammarBindings* b) noexcept {
    return b&&portable_flex_grammar_ready(&b->grammar,true)&&choreography_finalize_ready(&b->finalize);
}
int __cdecl portable_finalized_parse(void* parsed,unsigned extent,const char* name,const char* data,unsigned bytes,const PortableVcdFinalizedGrammarBindings* b) noexcept {
    if(!portable_finalized_ready(b))return 0;
    const auto& g=b->grammar;PortableGrammarState s(g.curves.grammar,&g.curves.scales,&g.flex,&b->finalize);
    if(!choreography_event_parser_ready(&s.events)||!portable_vcd_begin(&s.cursor,data,bytes))return 0;
    return choreography_text_parse_impl(parsed,extent,name,&s.events.actors.text,&s.events.actors,&s.events);
}
int __cdecl portable_finalized_load(void* scene,unsigned extent,const char* name,const NamedSceneLoadBindings* load,const PortableVcdFinalizedGrammarBindings* b,std::uint32_t* out) noexcept {
    if(!portable_finalized_ready(b))return 0;
    const auto& g=b->grammar;PortableGrammarState s(g.curves.grammar,&g.curves.scales,&g.flex,&b->finalize);
    return choreography_event_load(scene,extent,name,load,&s.events,out);
}
int __cdecl portable_finalized_scene(void* npc,const char* name,float delay,const NativeNamedPlaybackBindings* playback,const PortableVcdFinalizedGrammarBindings* b,float* out) noexcept {
    if(!playback||!portable_finalized_ready(b))return 0;
    const auto& g=b->grammar;PortableGrammarState s(g.curves.grammar,&g.curves.scales,&g.flex,&b->finalize);auto h=*playback;h.events=&s.events;
    return native_named_playback(npc,name,delay,&h,out);
}
const PortableVcdFinalizedGrammarApi portable_vcd_finalized_grammar_api{version,sizeof(PortableVcdFinalizedGrammarApi),&portable_finalized_parse,&portable_finalized_load,&portable_finalized_scene};
// Compose original-derived grammar, graph, vector, symbol, scale and flex
// lifecycles with only engine-facing leaves. Adapters are synchronous and
// never stored in parsed objects; only host-supplied ABI words are stored.
bool native_vcd_runtime_ready(const NativeVcdRuntimeBindings* b) noexcept {
    return b&&b->storage&&b->allocate_object&&b->resize_memory&&b->release_object&&b->release_memory&&
        b->number&&b->integer&&b->unexpected&&b->missing_tag&&b->scale_comparator&&b->symbol_comparator&&
        b->flex_vtable&&b->tokenizer_word&&b->diagnostic_word&&hologram_writable(b->event_counter,4)&&
        hologram_writable(b->scale_sentinel,sizeof(ChoreographyScaleSentinel))&&
        hologram_writable(b->symbol_sentinel,sizeof(ChoreographySymbolSentinel));
}
struct NativeVcdRuntimeState {
    NativeVcdRuntimeBindings host;
    ChoreographyVectorBindings vectors;
    ChoreographyFlexOwnershipBindings tracks{};
    ChoreographyScaleTreeBindings scale{};
    ChoreographySymbolInternBindings intern{};
    PortableVcdFinalizedGrammarBindings grammar{};
    OwnedChoreographyBindings owned{};
    static NativeVcdRuntimeState& self(void* p) noexcept {return *static_cast<NativeVcdRuntimeState*>(p);}
    static int __cdecl storage(void* p,std::uint32_t w,void** out,unsigned* n) noexcept {
        auto& h=self(p).host;return h.storage(h.context,w,out,n);
    }
    static int __cdecl object_allocate(void* p,unsigned n,void** out,unsigned* extent) noexcept {
        auto& h=self(p).host;return h.allocate_object(h.context,n,out,extent);
    }
    static int __cdecl object_release(void* p,void* obj) noexcept {
        auto& h=self(p).host;return h.release_object(h.context,scene_word(obj));
    }
    static int __cdecl number(void* p,const char* s,float* out) noexcept {
        auto& h=self(p).host;return h.number(h.context,s,out);
    }
    static int __cdecl grow_vector(void* p,void* v,unsigned n,unsigned stride) noexcept {
        return choreography_grow_vector(&self(p).vectors,v,n,stride);
    }
    static int __cdecl grow_memory(void* p,void* v,unsigned n,unsigned stride) noexcept {
        return choreography_grow_memory(&self(p).vectors,v,n,stride);
    }
    explicit NativeVcdRuntimeState(const NativeVcdRuntimeBindings& h) noexcept:host(h),vectors{h.context,h.storage,h.resize_memory}{
        tracks.context=this;tracks.vtable=h.flex_vtable;
        tracks.allocate=[](void* p,unsigned n,ChoreographyFlexAllocation k,void** out,unsigned* bytes) noexcept -> int{
            auto& h=self(p).host;
            return k==ChoreographyFlexAllocation::SampleVector?h.resize_memory(h.context,0,n,out,bytes):h.allocate_object(h.context,n,out,bytes);
        };
        tracks.release=[](void* p,std::uint32_t w,ChoreographyFlexAllocation k) noexcept -> int{
            auto& h=self(p).host;return k==ChoreographyFlexAllocation::SampleVector?h.release_memory(h.context,w):h.release_object(h.context,w);
        };
        tracks.storage=&storage;tracks.grow=&grow_vector;
        scale.context=this;scale.sentinel=h.scale_sentinel;scale.storage=&storage;scale.grow_memory=&grow_memory;
        intern.context=this;intern.sentinel=h.symbol_sentinel;intern.insensitive=1;intern.storage=&storage;intern.grow_memory=&grow_memory;
        intern.allocate=[](void* p,unsigned n,void** out,unsigned* bytes) noexcept -> int{
            auto& h=self(p).host;return h.resize_memory(h.context,0,n,out,bytes);
        };
        intern.release_unpublished=[](void* p,std::uint32_t w) noexcept -> int{
            auto& h=self(p).host;return h.release_memory(h.context,w);
        };
        owned.insensitive_comparator=h.symbol_comparator;
        auto& d=owned.destruction;d.context=this;d.storage=&storage;
        d.release=[](void* p,std::uint32_t w,ChoreographyRelease k) noexcept -> int{
            auto& h=self(p).host;return k==ChoreographyRelease::Vector?h.release_memory(h.context,w):h.release_object(h.context,w);
        };
        d.delete_aux=[](void* p,std::uint32_t w,unsigned flags) noexcept -> int{
            return choreography_flex_owned_destroy(&self(p).tracks,w,flags);
        };
        auto& sy=owned.symbols;sy.context=this;sy.sentinel=h.symbol_sentinel;sy.storage=&storage;
        sy.release=[](void* p,std::uint32_t w,ChoreographySymbolRelease) noexcept -> int{
            auto& h=self(p).host;return h.release_memory(h.context,w);
        };
        auto& g=grammar.grammar.curves.grammar;auto& e=g.events;auto& a=e.actors;auto& t=a.text;
        e.context=a.context=a.allocation.context=t.context=this;t.tokenizer_word=h.tokenizer_word;
        t.unexpected=[](void* p,const char* token) noexcept -> int{
            auto& h=self(p).host;return h.unexpected(h.context,token);
        };
        auto& al=a.allocation;al.allocate=&object_allocate;al.release_unpublished=&object_release;
        al.event_counter=h.event_counter;al.parsed.compare_word=h.scale_comparator;
        al.append_parsed=[](void* p,void* root,GeneratedSceneChild k,void* obj) noexcept -> int{
            return choreography_append_parsed(&self(p).vectors,root,k,obj);
        };
        a.append_channel=[](void* p,void* actor,void* channel) noexcept -> int{
            return choreography_append_channel(&self(p).vectors,actor,channel);
        };
        e.append_event=[](void* p,void* channel,void* event) noexcept -> int{
            return choreography_append_event(&self(p).vectors,channel,event);
        };
        e.number=&number;
        g.tags.context=this;g.tags.storage=&storage;g.tags.grow=&grow_vector;
        auto& sc=grammar.grammar.curves.scales;sc.context=this;sc.sentinel=h.scale_sentinel;sc.storage=&storage;
        sc.integer=[](void* p,const char* text,int* out) noexcept -> int{
            auto& h=self(p).host;return h.integer(h.context,text,out);
        };
        sc.intern=[](void* p,void* table,const char* text,std::uint16_t* out) noexcept -> int{
            return choreography_symbol_intern(&self(p).intern,table,text,out);
        };
        sc.insert=[](void* p,void* table,std::uint16_t sym,std::uint32_t* out) noexcept -> int{
            return choreography_scale_insert(&self(p).scale,table,sym,out);
        };
        auto& f=grammar.grammar.flex;f.context=&tracks;f.create=&choreography_flex_owned_create;f.delete_aux=&choreography_flex_owned_destroy;
        f.samples.context=this;f.samples.storage=&storage;f.samples.grow=&grow_vector;f.samples.number=&number;
        grammar.finalize.context=this;grammar.finalize.storage=&storage;
        grammar.finalize.missing_tag=[](void* p,void* root,const char* e,const char* ref,const char* tag) noexcept -> int{
            auto& h=self(p).host;return h.missing_tag(h.context,root,e,ref,tag);
        };
    }
    NativeVcdRuntimeState(const NativeVcdRuntimeState&)=delete;
    NativeVcdRuntimeState& operator=(const NativeVcdRuntimeState&)=delete;
    bool destroy(void* parsed,std::uint32_t word) noexcept {
        return choreography_destroy_parsed(parsed,word,owned.destruction,nullptr,&owned.symbols);
    }
};
struct NativeVcdLoadState {
    NativeVcdRuntimeState& runtime;
    NamedSceneLoadBindings source;
    NamedSceneLoadBindings load;
    explicit NativeVcdLoadState(NativeVcdRuntimeState& r,const NamedSceneLoadBindings& b) noexcept:runtime(r),source(b),load(b){
        load.context=this;load.owned=r.owned;
        load.compare_word=r.host.scale_comparator;load.diagnostic_word=r.host.diagnostic_word;load.parse=nullptr;
        load.set_extension=[](void* p,char* path,const char* ext,unsigned n) noexcept -> int{
            const auto& b=static_cast<NativeVcdLoadState*>(p)->source;return b.set_extension(b.context,path,ext,n);
        };
        load.missing=[](void* p,const char* path) noexcept -> int{
            const auto& b=static_cast<NativeVcdLoadState*>(p)->source;return b.missing(b.context,path);
        };
        load.allocate=[](void* p,unsigned n,void** out,unsigned* extent) noexcept -> int{
            auto& r=static_cast<NativeVcdLoadState*>(p)->runtime;return r.object_allocate(&r,n,out,extent);
        };
        load.release=[](void* p,void* object) noexcept -> int{
            auto& r=static_cast<NativeVcdLoadState*>(p)->runtime;return r.object_release(&r,object);
        };
    }
};
int __cdecl native_vcd_runtime_parse(void* callback,const char* name,const char* text,unsigned n,const NativeVcdRuntimeBindings* b,std::uint32_t* out) noexcept {
    if(!native_vcd_runtime_ready(b)||!name||!text||!hologram_writable(out,4))return 0;
    *out=0;NativeVcdRuntimeState s(*b);void* root=nullptr;unsigned extent=0;
    const auto& a=s.grammar.grammar.curves.grammar.events.actors.allocation;
    if(!allocate_generated_parsed(a,callback,&root,&extent,&s.owned)||!root)return -1;
    if(portable_finalized_parse(root,extent,name,text,n,&s.grammar)!=1){
        (void)s.destroy(root,scene_word(root));return -1;
    }
    hologram_write(root,0x98,b->diagnostic_word);*out=scene_word(root);return 1;
}
int __cdecl native_vcd_runtime_load(void* scene,unsigned extent,const char* name,const NamedSceneLoadBindings* load,const NativeVcdRuntimeBindings* b,std::uint32_t* out) noexcept {
    if(!native_vcd_runtime_ready(b)||!load||!load->set_extension||!load->missing)return 0;
    NativeVcdRuntimeState s(*b);NativeVcdLoadState l(s,*load);
    return portable_finalized_load(scene,extent,name,&l.load,&s.grammar,out);
}
int __cdecl native_vcd_runtime_scene(void* npc,const char* name,float delay,const NativeNamedPlaybackBindings* playback,const NativeVcdRuntimeBindings* b,float* out) noexcept {
    if(!native_vcd_runtime_ready(b)||!playback||!playback->precache.load.set_extension||!playback->precache.load.missing)return 0;
    NativeVcdRuntimeState s(*b);NativeVcdLoadState l(s,playback->precache.load);
    auto h=*playback;h.precache.load=l.load;h.precache.parser=nullptr;
    return portable_finalized_scene(npc,name,delay,&h,&s.grammar,out);
}
int __cdecl native_vcd_runtime_destroy(std::uint32_t word,const NativeVcdRuntimeBindings* b) noexcept {
    if(!native_vcd_runtime_ready(b)||!word)return 0;
    NativeVcdRuntimeState s(*b);void* parsed=nullptr;
    if(!choreography_storage(s.owned.destruction,word,0x220,parsed))return 0;
    return s.destroy(parsed,word)?1:-1;
}
int __cdecl native_vcd_runtime_remove(void* scene,unsigned extent,const SceneEntityLifecycleBindings* life,const SceneStopBindings* stop,const NativeVcdRuntimeBindings* b) noexcept {
    if(!native_vcd_runtime_ready(b))return 0;
    NativeVcdRuntimeState s(*b);return owned_choreography_remove(scene,extent,life,stop,&s.owned);
}
const NativeVcdRuntimeApi native_vcd_runtime_api{version,sizeof(NativeVcdRuntimeApi),
    &native_vcd_runtime_parse,&native_vcd_runtime_load,&native_vcd_runtime_scene,&native_vcd_runtime_destroy,&native_vcd_runtime_remove};
// 221adaa0 collects symbol/time keys; 221a9ba0 warms in-order.
// Stable sorting preserves right-on-equal insertion order for finite times.
struct ChoreographyStartupEntry { float time;char sound[64]; };
struct ChoreographyStartupCollector {
    const ChoreographyStartupSoundBindings& h;
    void* scene;
    ChoreographyStartupEntry* entries;
    unsigned count=0,budget=0x100000;
    static constexpr unsigned capacity=4096;
    bool storage(std::uint32_t word,unsigned need,void*& ptr) noexcept {
        unsigned extent=0;ptr=nullptr;
        return word&&h.storage(h.context,word,&ptr,&extent)&&extent>=need&&hologram_writable(ptr,need);
    }
    bool collect(std::uint32_t word,float offset,unsigned depth) noexcept {
        void* root=nullptr;
        if(depth>1||!storage(word,0x220,root))return false;
        for(unsigned i=0;;++i){
            const auto n=hologram_read<int>(root,0xc);
            if(n<0||n>0x100000)return false;
            if(i>=static_cast<unsigned>(n))return true;
            if(!budget--)return false;
            void* rows=nullptr;
            if(!storage(hologram_read<std::uint32_t>(root,0),(i+1)*4,rows))return false;
            auto ew=hologram_read<std::uint32_t>(rows,i*4);
            if(!ew)continue;
            void* event=nullptr;
            if(!storage(ew,0x3c0,event))return false;
            const auto type=hologram_read<int>(event,0);
            const char* param=static_cast<char*>(event)+0x84;
            if(type==11){
                // Startup ignores already linked children, unlike initial precache.
                if(depth||hologram_read<unsigned char>(root,0x124)||hologram_read<std::uint32_t>(event,0x324))continue;
                std::uint32_t childword=0;void* child=nullptr;
                if(!h.load_subscene||!std::memchr(param,0,128)||
                   !h.load_subscene(h.context,scene,param,&childword)||childword==word||
                   !storage(childword,0x220,child))return false;
                hologram_write(child,0x124,static_cast<unsigned char>(1));
                hologram_write(event,0x324,childword);
                if(!collect(childword,hologram_read<float>(event,0x184),depth+1))return false;
                continue;
            }
            if(type!=5)continue;
            ChoreographyStartupEntry e{};
            const char* token=static_cast<char*>(event)+0x350;
            const char* selected=hologram_read<int>(event,0x34c)==0&&*token?token:param;
            if(!choreography_text_copy(e.sound,sizeof(e.sound),selected))return false;
            unsigned include=1;
            if(h.select_sound&&!h.select_sound(h.context,scene,event,e.sound,sizeof(e.sound),&include))return false;
            if(!include)continue;
            if(!std::memchr(e.sound,0,sizeof(e.sound))||count==capacity)return false;
            e.time=hologram_read<float>(event,0x184)+offset;
            if(!std::isfinite(e.time))return false;
            unsigned pos=count++;
            while(pos&&e.time<entries[pos-1].time){entries[pos]=entries[pos-1];--pos;}
            entries[pos]=e;
        }
    }
};
int __cdecl choreography_startup_sound_warm(void* context,void* scene,std::uint32_t word) noexcept {
    auto* b=static_cast<const ChoreographyStartupSoundBindings*>(context);
    if(!b||!b->storage||!b->warm_sound||!word)return 0;
    std::unique_ptr<ChoreographyStartupEntry[]> rows(new(std::nothrow) ChoreographyStartupEntry[ChoreographyStartupCollector::capacity]);
    if(!rows)return 0;
    ChoreographyStartupCollector c{*b,scene,rows.get()};
    if(!c.collect(word,0.f,0))return 0;
    for(unsigned i=0;i<c.count;++i)if(!b->warm_sound(b->context,rows[i].sound))return 0;
    return 1;
}
const ChoreographyStartupSoundApi choreography_startup_sound_api{version,sizeof(ChoreographyStartupSoundApi),&choreography_startup_sound_warm};
const NamedScenePrecacheApi named_scene_precache_api{version,sizeof(NamedScenePrecacheApi),&named_scene_precache_events,&named_scene_precache_spawn};
bool scene_finish_ready(const SceneFinishBindings* b) noexcept {
    return b&&b->entry&&b->resolve&&b->notify&&b->refresh&&b->output&&b->reset.events.event&&
        b->reset.storage&&b->reset.grow&&scene_stop_ready(&b->stop);
}
int __cdecl native_scene_finish(void* scene,unsigned bytes,int argument,unsigned fire_output,const SceneFinishBindings* b) noexcept {
    if(bytes<0x544||!hologram_writable(scene,0x544)||!scene_finish_ready(b))return 0;
    if(!hologram_read<std::uint32_t>(scene,0x40c))return 1;
    const auto h=*b;const int count=hologram_read<int>(scene,0x540);
    if(count>0x100000)return -1;
    // Original snapshots count but re-reads handle vector per index.
    for(int i=0;i<count;++i){
        std::uint32_t word=0xffffffffu;
        if(!h.entry(h.context,scene,static_cast<unsigned>(i),&word))return -1;
        if(word==0xffffffffu)continue;
        void* target=nullptr;if(!h.resolve(h.context,word,&target))return -1;
        if(target&&target!=scene&&!h.notify(h.context,target,scene))return -1;
    }
    hologram_write(scene,0x540,0u);
    const auto resolve=[&](void*& parsed,unsigned& n){
        const auto word=hologram_read<std::uint32_t>(scene,0x40c);
        parsed=nullptr;n=0;return word&&h.stop.parsed(h.stop.context,word,&parsed,&n)&&parsed&&n>=0x220&&hologram_writable(parsed,0x220);
    };
    void* parsed=nullptr;unsigned n=0;
    if(!resolve(parsed,n)||choreography_reset(parsed,n,1,0,0,&h.reset)!=1)return -1;
    hologram_write(scene,0x3d8,static_cast<unsigned char>(0));
    hologram_write(scene,0x3d9,static_cast<unsigned char>(0));hologram_write(scene,0x3dc,0u);
    if(!h.refresh(h.context,scene))return -1;
    if((fire_output&0xff)&&!hologram_read<unsigned char>(scene,0x530)&&
       !h.output(h.context,static_cast<unsigned char*>(scene)+0x434,scene,scene,0.f))return -1;
    if(scene_stop_events(scene,bytes,hologram_read<std::uint32_t>(scene,0x40c),argument,&h.stop)!=1)return -1;
    for(unsigned i=0;;++i){
        if(!resolve(parsed,n))return -1;
        const int actors=hologram_read<int>(parsed,0x20);
        if(actors<=0||i>=static_cast<unsigned>(actors))break;
        if(actors>0x100000)return -1;
        void* actor=nullptr;unsigned extent=0;
        if(!h.stop.actor(h.stop.context,scene,i,&actor,&extent)||
           (actor&&scene_actor_detach(actor,extent,hologram_read<std::uint32_t>(scene,0x40c),&h.stop.records)!=1))return -1;
    }
    return 1;
}
int __cdecl parsed_scene_finished(const void* parsed,unsigned bytes,unsigned* out) noexcept {
    if(!parsed||bytes<0x90||!sin_independent::vgui_layout::Readable(parsed,0x90)||
       !out||!hologram_writable(out,sizeof(*out)))return 0;
    const float time=hologram_read<float>(parsed,0x78);
    *out=time>hologram_read<float>(parsed,0x88)?hologram_read<unsigned>(parsed,0x8c)==0:time<hologram_read<float>(parsed,0x84);
    return 1;
}
bool scene_advance_parsed(void* scene,const SceneAdvanceBindings& h,void*& p,unsigned& bytes) noexcept {
    p=nullptr;bytes=0;const auto word=hologram_read<std::uint32_t>(scene,0x40c);
    return word&&h.parsed(h.context,word,&p,&bytes)&&p&&bytes>=0x220&&hologram_writable(p,0x220);
}
int scene_advance(void* scene,unsigned bytes,float dt,const SceneAdvanceBindings* b,bool instanced,const SceneFinishBindings* finish=nullptr) noexcept {
    const unsigned required=instanced?0x561:(finish?0x544:0x41c);
    if(bytes<required||!hologram_writable(scene,required)||!b||!b->pre_tick||!b->paused_tick||
       !b->parsed||!b->latency||(finish?!scene_finish_ready(finish):!b->finish)||
       (instanced?(!b->resolve_actor||!b->remove):(!b->start||!b->release_actors)))return 0;
    const auto h=*b;
    if(!h.pre_tick(h.context,scene))return -1;
    if(!instanced&&hologram_read<unsigned char>(scene,0x3f5)&&!h.start(h.context,scene))return -1;
    const auto retire=[&](){return h.remove(h.context,scene)?1:-1;};
    if(!hologram_read<std::uint32_t>(scene,0x40c)||!hologram_read<unsigned char>(scene,0x3d8))
        return instanced?retire():1;
    if(instanced&&hologram_read<unsigned char>(scene,0x560)){
        const auto word=hologram_read<std::uint32_t>(scene,0x55c);void* actor=nullptr;
        if(word!=0xffffffffu&&!h.resolve_actor(h.context,word,&actor))return -1;
        if(!actor)return retire();
    }
    if(hologram_read<unsigned char>(scene,0x3d9))return h.paused_tick(h.context,scene)?1:-1;
    if(!instanced)hologram_write(scene,0x3e0,dt);
    void* parsed=nullptr;unsigned n=0;
    if(!scene_advance_parsed(scene,h,parsed,n))return -1;
    float latency=.1f;const auto nested=hologram_read<std::uint32_t>(scene,0x418);
    if(nested&&!h.latency(h.context,nested,&latency))return -1;
    hologram_write(parsed,0x90,latency);
    if(!scene_advance_parsed(scene,h,parsed,n)||
       parsed_scene_simulate(parsed,n,hologram_read<float>(scene,0x3dc),&h.simulation)!=1)return -1;
    const float increment=instanced?dt:hologram_read<float>(scene,0x3e0);
    hologram_write(scene,0x3dc,static_cast<float>(static_cast<double>(hologram_read<float>(scene,0x3dc))+increment));
    if(instanced||!hologram_read<unsigned char>(scene,0x3d9)){
        unsigned finished=0;
        if(!scene_advance_parsed(scene,h,parsed,n)||parsed_scene_finished(parsed,n,&finished)!=1)return -1;
        if(finished){
            if(!(finish?native_scene_finish(scene,bytes,0,instanced?0u:1u,finish)==1:
                h.finish(h.context,scene,0,instanced?0:1)!=0))return -1;
            if(instanced)return retire();
            if(!h.release_actors(h.context,scene,hologram_read<std::uint32_t>(scene,0x40c)))return -1;
        }
    }
    if(!instanced&&hologram_read<unsigned char>(scene,0x3d9)){
        if(!scene_advance_parsed(scene,h,parsed,n))return -1;
        hologram_write(scene,0x3dc,hologram_read<float>(parsed,0x78));
    }
    return 1;
}
int __cdecl scripted_scene_advance(void* scene,unsigned bytes,float dt,const SceneAdvanceBindings* b) noexcept {
    return scene_advance(scene,bytes,dt,b,false);
}
int __cdecl instanced_scene_advance(void* scene,unsigned bytes,float dt,const SceneAdvanceBindings* b) noexcept {
    return scene_advance(scene,bytes,dt,b,true);
}
const SceneAdvanceApi scene_advance_api{version,sizeof(SceneAdvanceApi),&parsed_scene_finished,&scripted_scene_advance,&instanced_scene_advance};
int __cdecl native_finish_advance(void* scene,unsigned bytes,float dt,const SceneAdvanceBindings* b,const SceneFinishBindings* finish,unsigned instanced) noexcept {
    if(!finish||instanced>1)return 0;
    const auto h=*finish;
    return scene_advance(scene,bytes,dt,b,instanced!=0,&h);
}
const SceneFinishApi scene_finish_api{version,sizeof(SceneFinishApi),&native_scene_finish,&native_finish_advance};
// 220f6310 copies named members, not the six padding bytes.
void copy_actor_event_record(void* dest,const void* source) noexcept {
    for(unsigned off:{0u,4u,8u,0x10u,0x14u,0x18u,0x20u,0x24u,0x2cu,0x30u,0x34u,0x38u,0x3cu})
        hologram_write(dest,off,hologram_read<std::uint32_t>(source,off));
    for(unsigned off:{0xcu,0x1cu,0x28u,0x29u})hologram_write(dest,off,hologram_read<unsigned char>(source,off));
}
bool actor_event_rows(void* actor,const ActorEventRecordBindings& h,unsigned count,void*& rows) noexcept {
    auto* v=static_cast<unsigned char*>(actor)+0x5b8;
    unsigned extent=0;const auto word=hologram_read<std::uint32_t>(v,0);
    rows=nullptr;return word&&h.storage(h.context,word,&rows,&extent)&&extent>=count*0x40&&hologram_writable(rows,count*0x40);
}
bool actor_event_ready(void* actor,unsigned bytes,const ActorEventRecordBindings* b,unsigned* out) noexcept {
    return bytes>=0x5cc&&hologram_writable(actor,0x5cc)&&b&&b->storage&&out&&hologram_writable(out,4);
}
int __cdecl actor_event_add(void* actor,unsigned bytes,std::uint32_t parsed,void* event,unsigned extent,void* target,
    const ActorEventRecordBindings* b,unsigned* out) noexcept {
    if(!actor_event_ready(actor,bytes,b,out)||!b->grow||!b->handle||!b->diagnostic||!b->begin)return 0;
    const auto h=*b;
    const auto rejected=[&](ActorEventDiagnostic d){if(!h.diagnostic(h.context,d,parsed))return -1;*out=0;return 1;};
    if(!parsed||!event)return rejected(ActorEventDiagnostic::NullSceneOrEvent);
    if(extent<0x2b8||!sin_independent::vgui_layout::Readable(event,0x2b8))return 0;
    const auto choreo_actor=hologram_read<std::uint32_t>(event,0x2b4);
    if(!choreo_actor)return rejected(ActorEventDiagnostic::NullChoreoActor);
    alignas(4) unsigned char record[0x40]{};
    hologram_write(record,0,scene_word(event));hologram_write(record,4,parsed);
    std::uint32_t handle=0xffffffffu;if(target&&!h.handle(h.context,target,&handle))return -1;
    hologram_write(record,0x24,handle);
    unsigned accepted=0;
    if(!h.begin(h.context,actor,record,parsed,event,choreo_actor,target,&accepted))return -1;
    if(!(accepted&0xff))return rejected(ActorEventDiagnostic::StartRejected);
    auto* v=static_cast<unsigned char*>(actor)+0x5b8;
    const int count=hologram_read<int>(v,0xc),capacity=hologram_read<int>(v,4);
    if(count<0||count>=0x100000||capacity<count||capacity>0x100000)return -1;
    if(count==capacity&&(!h.grow(h.context,v,1)||hologram_read<int>(v,0xc)!=count||
        hologram_read<int>(v,4)<=count||hologram_read<int>(v,4)>0x100000))return -1;
    void* rows=nullptr;if(!actor_event_rows(actor,h,static_cast<unsigned>(count+1),rows))return -1;
    hologram_write(v,0xc,count+1);hologram_write(v,0x10,hologram_read<std::uint32_t>(v,0));
    copy_actor_event_record(static_cast<unsigned char*>(rows)+count*0x40,record);
    *out=1;return 1;
}
int __cdecl actor_event_stop_one(void* actor,unsigned bytes,std::uint32_t parsed,void* event,int arg,
    const ActorEventRecordBindings* b,unsigned* out) noexcept {
    if(!actor_event_ready(actor,bytes,b,out)||!b->stop)return 0;
    const auto h=*b;auto* v=static_cast<unsigned char*>(actor)+0x5b8;
    for(unsigned i=0;;++i){
        const int count=hologram_read<int>(v,0xc);
        if(count<0||count>0x100000)return -1;
        if(i>=static_cast<unsigned>(count)){*out=0;return 1;}
        void* rows=nullptr;if(!actor_event_rows(actor,h,static_cast<unsigned>(count),rows))return -1;
        auto* row=static_cast<unsigned char*>(rows)+i*0x40;
        if(hologram_read<std::uint32_t>(row,4)!=parsed||hologram_read<std::uint32_t>(row,0)!=scene_word(event))continue;
        const auto word=hologram_read<std::uint32_t>(v,0);unsigned accepted=0;
        if(!h.stop(h.context,actor,row,arg,0,&accepted)||hologram_read<int>(v,0xc)!=count||
            hologram_read<std::uint32_t>(v,0)!=word)return -1;
        if(!(accepted&0xff))continue;
        hologram_write(row,0,0u);hologram_write(row,4,0u);hologram_write(row,0xc,static_cast<unsigned char>(0));
        if(i+1<static_cast<unsigned>(count))std::memmove(row,row+0x40,(static_cast<unsigned>(count)-i-1)*0x40);
        hologram_write(v,0xc,count-1);*out=1;return 1;
    }
}
int __cdecl actor_event_check_one(void* actor,unsigned bytes,float time,std::uint32_t parsed,void* event,
    const ActorEventRecordBindings* b,unsigned* out) noexcept {
    if(!actor_event_ready(actor,bytes,b,out)||!b->check)return 0;
    const auto h=*b;const int count=hologram_read<int>(actor,0x5c4);
    if(count<0||count>0x100000)return -1;
    void* rows=nullptr;if(count&&!actor_event_rows(actor,h,static_cast<unsigned>(count),rows))return -1;
    for(int i=0;i<count;++i){
        auto* row=static_cast<unsigned char*>(rows)+i*0x40;
        if(hologram_read<std::uint32_t>(row,4)!=parsed||hologram_read<std::uint32_t>(row,0)!=scene_word(event))continue;
        unsigned accepted=0;if(!h.check(h.context,actor,row,time,parsed,event,&accepted))return -1;
        *out=(accepted&0xff)!=0;return 1;
    }
    *out=1;return 1;
}
const ActorEventRecordApi actor_event_record_api{version,sizeof(ActorEventRecordApi),&actor_event_add,&actor_event_stop_one,&actor_event_check_one};
bool speech_text_valid(const void* event,unsigned bytes) noexcept {
    return event&&bytes>=0x391&&sin_independent::vgui_layout::Readable(event,0x391)&&
        std::memchr(static_cast<const unsigned char*>(event)+0x84,0,128)&&
        std::memchr(static_cast<const unsigned char*>(event)+0x350,0,64);
}
// Caller-provided destination is not required to be dword aligned.
bool speech_output_ready(char* dest,unsigned capacity,unsigned* available) noexcept {
    if(!dest||!capacity||capacity>0x100000||!available||!hologram_writable(available,4))return false;
    if(!sin_independent::vgui_layout::Readable(dest,capacity))return false;
    MEMORY_BASIC_INFORMATION region{};
    if(VirtualQuery(dest,&region,sizeof(region))!=sizeof(region))return false;
    const auto protection=region.Protect&0xffu;
    return protection==PAGE_READWRITE||protection==PAGE_WRITECOPY||protection==PAGE_EXECUTE_READWRITE||protection==PAGE_EXECUTE_WRITECOPY;
}
void speech_copy(char* dest,unsigned capacity,const char* source) noexcept {
    const auto count=std::strlen(source);
    const auto n=count<capacity-1?count:capacity-1;
    std::memmove(dest,source,n);dest[n]=0;
}
const char* speech_caption_token(const void* event) noexcept {
    const int mode=hologram_read<int>(event,0x34c);
    if(mode!=0&&(mode!=1||hologram_read<unsigned char>(event,0x390)))return nullptr;
    const auto* p=static_cast<const char*>(event);
    return p[0x350]?p+0x350:p+0x84;
}
int __cdecl speech_token(const void* event,unsigned bytes,char* dest,unsigned capacity,unsigned* available) noexcept {
    if(!speech_text_valid(event,bytes)||!speech_output_ready(dest,capacity,available))return 0;
    const auto* token=speech_caption_token(event);speech_copy(dest,capacity,token?token:"");*available=token!=nullptr;return 1;
}
int __cdecl speech_sound(const void* event,unsigned bytes,unsigned has_language,int language,unsigned force,char* dest,unsigned capacity,unsigned* available) noexcept {
    if(!speech_text_valid(event,bytes)||!speech_output_ready(dest,capacity,available))return 0;
    const int mode=hologram_read<int>(event,0x34c);
    const auto* primary=static_cast<const char*>(event)+0x84;
    const auto* selected=primary;unsigned accepted=1;
    if(force||(has_language&&language!=1)){
        if(mode==1||mode==2)accepted=0;
        else{
            // Original helper retrieves into a 64-byte local token.
            char token[64]{};
            const auto* alternate=speech_caption_token(event);
            if(alternate){speech_copy(token,sizeof(token),alternate);speech_copy(dest,capacity,token);*available=1;return 1;}
        }
    }
    speech_copy(dest,capacity,selected);*available=accepted;return 1;
}
int __cdecl speech_timing(const void* scene,unsigned bytes,const void* event,unsigned extent,float now,SceneSpeechTiming* out) noexcept {
    if(!scene||bytes<0x3e0||!sin_independent::vgui_layout::Readable(scene,0x3e0)||!event||extent<0x18c||
       !sin_independent::vgui_layout::Readable(event,0x18c)||!out||!hologram_writable(out,sizeof(*out)))return 0;
    const double start=hologram_read<float>(event,0x184);
    const float offset=static_cast<float>(static_cast<double>(hologram_read<float>(scene,0x3dc))-start);
    const float absolute=static_cast<float>(static_cast<double>(now)-offset);
    const double duration=hologram_read<std::uint32_t>(event,0x188)==0xbf800000u?0.0:
        static_cast<double>(hologram_read<float>(event,0x188))-start;
    *out={offset,absolute,static_cast<float>(duration-offset)};return 1;
}
int __cdecl speech_queue(void* manager,unsigned bytes,void* actor,const char* sound,int level,float offset,const SceneSpeechQueueBindings* b) noexcept {
    if(bytes<0x3bc||!hologram_writable(manager,0x3bc)||!sound||!b||!b->handle||!b->storage||!b->grow)return 0;
    const auto h=*b;alignas(4) unsigned char record[0x8c]{};
    std::uint32_t handle=0xffffffffu;if(actor&&!h.handle(h.context,actor,&handle))return -1;
    hologram_write(record,0,handle);
    char text[128]{};if(!generated_scene_text(sound,text))return -1;
    std::memcpy(record+4,text,128);hologram_write(record,0x84,level);hologram_write(record,0x88,offset);
    auto* v=static_cast<unsigned char*>(manager)+0x3a8;
    const int count=hologram_read<int>(v,0xc),capacity=hologram_read<int>(v,4);
    if(count<0||count>=0x100000||capacity<count||capacity>0x100000)return -1;
    if(count==capacity&&(!h.grow(h.context,v,1)||hologram_read<int>(v,0xc)!=count||
       hologram_read<int>(v,4)<=count||hologram_read<int>(v,4)>0x100000))return -1;
    const auto word=hologram_read<std::uint32_t>(v,0);void* rows=nullptr;unsigned n=0;
    const auto needed=static_cast<unsigned>(count+1)*0x8c;
    if(!word||!h.storage(h.context,word,&rows,&n)||n<needed||!hologram_writable(rows,needed))return -1;
    hologram_write(v,0xc,count+1);hologram_write(v,0x10,word);
    std::memcpy(static_cast<unsigned char*>(rows)+count*0x8c,record,sizeof(record));return 1;
}
const SceneSpeechPolicyApi scene_speech_policy_api{version,sizeof(SceneSpeechPolicyApi),&speech_token,&speech_sound,&speech_timing,&speech_queue};
std::uint32_t speech_caption_crc(char* token) noexcept {
    std::uint32_t crc=0xffffffffu;
    // Original CRT C-locale lowercase then 2225a6e0/710/6f0 CRC32.
    for(auto* p=token;*p;++p){
        if(*p>='A'&&*p<='Z')*p=static_cast<char>(*p+('a'-'A'));
        crc^=static_cast<unsigned char>(*p);
        for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1)?0xedb88320u:0u);
    }
    return ~crc;
}
bool speech_execution_ready(const SceneSpeechExecutionBindings* b,bool native_preparation=false) noexcept {
    return b&&b->open_filter&&b->close_filter&&b->clock&&(native_preparation||b->prepare_actor)&&b->recipient_count&&b->recipient&&
       b->language&&b->emit&&b->caption_register&&b->caption_preferences&&b->caption_far&&b->remove_recipient&&b->caption_send;
}
int __cdecl scene_speech_prepare(void* scene,unsigned bytes,void* actor,float remaining,const SceneSpeechPreparationBindings* b) noexcept {
    if(!scene||!b||b->instanced>1||!actor||!b->cast_base_actor||!b->actor_notify||!b->is_npc||
       !b->classify||!b->clock||!b->handle||!hologram_writable(b->state,sizeof(SpeechSemaphorePair)))return 0;
    if(b->instanced&&(bytes<0x568||!sin_independent::vgui_layout::Readable(scene,0x568)))return 0;
    const auto h=*b;void* base=nullptr;
    if(!h.cast_base_actor(h.context,actor,&base))return -1;
    if(base){
        const float margin=h.instanced?hologram_read<float>(scene,0x564):1.f;
        return h.actor_notify(h.context,base,remaining,margin)?1:-1;
    }
    unsigned npc=0;
    if(!h.is_npc(h.context,actor,&npc))return -1;
    if(!(npc&0xff))return 1;
    if(!h.is_npc(h.context,actor,&npc)||!(npc&0xff))return -1;
    unsigned classified=0;
    if(!h.classify(h.context,actor,0,&classified))return -1;
    const float margin=h.instanced?hologram_read<float>(scene,0x564):1.f;
    const float duration=static_cast<float>(static_cast<double>(remaining)+margin);
    float now=0;if(!h.clock(h.context,&now))return -1;
    auto& sem=(classified&0xff)?h.state->classified:h.state->other;
    // Original 220a3d00 stores release before obtaining actor's EHANDLE.
    sem.release=static_cast<float>(static_cast<double>(now)+duration);
    std::uint32_t owner=0xffffffffu;if(!h.handle(h.context,actor,&owner))return -1;
    sem.owner=owner;return 1;
}
int speech_execute_impl(void* scene,unsigned bytes,void* actor,unsigned actor_bytes,std::uint32_t parsed,void* event,unsigned extent,int level,
    const SceneSpeechExecutionBindings* b,const SceneSpeechPreparationBindings* preparation) noexcept {
    if(!scene||bytes<0x549||!sin_independent::vgui_layout::Readable(scene,0x549)||!b)return 0;
    if(!actor)return 1;
    if(!speech_execution_ready(b,preparation!=nullptr)||!speech_text_valid(event,extent)||extent<0x3a3||
       !sin_independent::vgui_layout::Readable(event,0x3a3))return 0;
    const auto h=*b;void* filter=nullptr;
    if(!h.open_filter(h.context,actor,.8f,&filter)||!filter)return -1;
    const auto close=[&](bool ok){const bool closed=h.close_filter(h.context,filter)!=0;return ok&&closed?1:-1;};
    float now=0;SceneSpeechTiming timing{};
    if(!h.clock(h.context,&now)||speech_timing(scene,bytes,event,extent,now,&timing)!=1)return close(false);
    if(hologram_read<unsigned char>(scene,0x548)){
        void* manager=nullptr;unsigned n=0;
        if(scene_manager_ensure(h.manager,&h.runtime,&manager,&n)!=1||!manager)return close(false);
        return close(speech_queue(manager,n,actor,static_cast<const char*>(event)+0x84,level,timing.offset,&h.queue)==1);
    }
    if(!(preparation?scene_speech_prepare(scene,bytes,actor,timing.remaining,preparation)==1:
        h.prepare_actor(h.context,scene,actor,timing.remaining)!=0))return close(false);
    int count=0;if(!h.recipient_count(h.context,filter,&count)||count<0||count>0x100000)return close(false);
    // Original snapshots filter count for the emission pass.
    for(int i=0;i<count;++i){
        void* player=nullptr;
        if(!h.recipient(h.context,filter,static_cast<unsigned>(i),&player))return close(false);
        if(!player)continue;
        unsigned present=0,force=0,available=0;int english=0;char sound[512]{};
        if(!h.language(h.context,player,&present,&english,&force)||
           speech_sound(event,extent,present,english,force,sound,sizeof(sound),&available)!=1)return close(false);
        if(!available)continue;
        if(!h.emit(h.context,actor,player,sound,2,1.f,level,timing.absolute_start))return close(false);
        unsigned added=0;
        // AddSceneEvent runs once after each successful recipient emission;
        // an ordinary actor virtual rejection is not an audio failure.
        if(actor_event_add(actor,actor_bytes,parsed,event,extent,nullptr,&h.records,&added)!=1)return close(false);
    }
    // Original only sends caption messages for mode zero.
    if(hologram_read<int>(event,0x34c)!=0)return close(true);
    char token[64]{};unsigned available=0;
    if(speech_token(event,extent,token,sizeof(token),&available)!=1)return close(false);
    if(!available)return close(true);
    const auto crc=speech_caption_crc(token);
    if(!h.caption_register(h.context,crc,token)||!h.caption_preferences(h.context,filter))return close(false);
    if(!hologram_read<unsigned char>(event,0x3a2)){
        if(!h.recipient_count(h.context,filter,&count)||count<0||count>0x100000)return close(false);
        for(int i=count-1;i>=0;--i){
            void* player=nullptr;unsigned distant=0;
            if(!h.recipient(h.context,filter,static_cast<unsigned>(i),&player))return close(false);
            if(player&&(!h.caption_far(h.context,player,&distant)||(distant&&!h.remove_recipient(h.context,filter,player))))return close(false);
        }
    }
    if(!h.recipient_count(h.context,filter,&count)||count<0||count>0x100000)return close(false);
    if(count>0){
        const double start=hologram_read<float>(event,0x184);
        const float duration=static_cast<float>(hologram_read<std::uint32_t>(event,0x188)==0xbf800000u?0.0:
            static_cast<double>(hologram_read<float>(event,0x188))-start);
        const double caption=static_cast<double>(hologram_read<float>(event,0x39c))-start;
        const float span=duration>caption?duration:static_cast<float>(caption);
        const double deciseconds=static_cast<double>(span)*10;
        // Original upper-clamps integer to 255; preserve low byte for
        // negative finite durations, reject malformed nonfinite/overflow.
        if(!(deciseconds>=-2147483648.0&&deciseconds<2147483648.0))return close(false);
        int ticks=static_cast<int>(deciseconds);if(ticks>255)ticks=255;
        if(!h.caption_send(h.context,filter,crc,static_cast<unsigned>(ticks)&0xff,1))return close(false);
    }
    return close(true);
}
int __cdecl speech_execute(void* scene,unsigned bytes,void* actor,unsigned actor_bytes,std::uint32_t parsed,void* event,unsigned extent,int level,
    const SceneSpeechExecutionBindings* b) noexcept {
    return speech_execute_impl(scene,bytes,actor,actor_bytes,parsed,event,extent,level,b,nullptr);
}
const SceneSpeechExecutionApi scene_speech_execution_api{version,sizeof(SceneSpeechExecutionApi),&speech_execute};
bool scene_event_ignored(const char* name) noexcept {
    // Original 223a5574/223a8410 is case-insensitive (C-locale path).
    constexpr char ignored[]="null";
    for(unsigned i=0;i<4;++i){auto c=name[i];if(c>='A'&&c<='Z')c=static_cast<char>(c+32);if(c!=ignored[i])return false;}
    return name[4]==0;
}
int __cdecl resolved_scene_actor(void*,unsigned,std::uint32_t,const ResolvedSceneActorBindings*,void**,unsigned*) noexcept;
int speech_callback_invoke_impl(const void* callback,unsigned callback_bytes,void* scene,unsigned bytes,
    unsigned slot,float time,void* parsed,unsigned parsed_bytes,void* event,unsigned extent,
    const SceneSpeechCallbackBindings* b,const ResolvedSceneActorBindings* resolved,unsigned* out,const SceneSpeechPreparationBindings* preparation=nullptr) noexcept {
    if(!callback||callback_bytes<8||!sin_independent::vgui_layout::Readable(callback,8)||!out||!hologram_writable(out,4)||
       (slot!=0&&slot!=4&&slot!=8&&slot!=12))return 0;
    const auto owner=hologram_read<std::uint32_t>(callback,4);
    if(!owner){*out=1;return 1;}
    if(!scene||bytes<0x549||!sin_independent::vgui_layout::Readable(scene,0x549)||
       owner!=scene_word(scene)||callback!=static_cast<unsigned char*>(scene)+0x410||
       !parsed||parsed_bytes<0xa0||!sin_independent::vgui_layout::Readable(parsed,0xa0)||
       !event||extent<0x2b8||!sin_independent::vgui_layout::Readable(event,0x2b8)||hologram_read<int>(event,0)!=5||
       !b||(!resolved&&!b->actor)||!b->trace)return 0;
    if(slot==8){*out=1;return 1;} // only nested type11 updates in original
    const auto h=*b;
    const auto* name=static_cast<const char*>(event)+4;
    if(slot!=12){
        if(!std::memchr(name,0,128))return 0;
        if(scene_event_ignored(name)){
            if(slot==0&&!h.trace(h.context,scene,event,time,SceneSpeechTrace::Ignored))return -1;
            *out=1;return 1;
        }
    }
    const auto choreo_actor=hologram_read<std::uint32_t>(event,0x2b4);
    void* actor=nullptr;unsigned actor_bytes=0;
    if(choreo_actor&&!(resolved?resolved_scene_actor(scene,bytes,choreo_actor,resolved,&actor,&actor_bytes)==1:
        h.actor(h.context,scene,choreo_actor,&actor,&actor_bytes)!=0))return -1;
    if(slot==0&&choreo_actor&&!actor){
        if(!h.trace(h.context,scene,event,time,SceneSpeechTrace::MissingActor))return -1;
        *out=1;return 1;
    }
    if(slot==12){
        if(choreo_actor&&!actor&&!h.trace(h.context,scene,event,time,SceneSpeechTrace::MissingActor))return -1;
        if(!actor){*out=1;return 1;}
        return actor_event_check_one(actor,actor_bytes,time,scene_word(parsed),event,&h.speech.records,out);
    }
    if(!h.trace(h.context,scene,event,time,slot==0?SceneSpeechTrace::Start:SceneSpeechTrace::Stop))return -1;
    if(!actor){*out=1;return 1;}
    if(slot==0){
        const auto* level_text=static_cast<const char*>(event)+0x104;
        if(!std::memchr(level_text,0,128))return 0;
        // CRT atoi path: zero (including invalid or empty text) -> 80.
        const long value=std::strtol(level_text,nullptr,10);
        if(value<(std::numeric_limits<int>::min)()||value>(std::numeric_limits<int>::max)())return -1;
        if(speech_execute_impl(scene,bytes,actor,actor_bytes,scene_word(parsed),event,extent,value?static_cast<int>(value):80,&h.speech,preparation)!=1)return -1;
    }else{
        unsigned removed=0;
        if(actor_event_stop_one(actor,actor_bytes,scene_word(parsed),event,hologram_read<unsigned char>(scene,0x548),&h.speech.records,&removed)!=1)return -1;
    }
    *out=1;return 1;
}
int __cdecl speech_callback_invoke(const void* callback,unsigned callback_bytes,void* scene,unsigned bytes,
    unsigned slot,float time,void* parsed,unsigned parsed_bytes,void* event,unsigned extent,
    const SceneSpeechCallbackBindings* b,unsigned* out) noexcept {
    return speech_callback_invoke_impl(callback,callback_bytes,scene,bytes,slot,time,parsed,parsed_bytes,event,extent,b,nullptr,out);
}
const SceneSpeechCallbackApi scene_speech_callback_api{version,sizeof(SceneSpeechCallbackApi),&speech_callback_invoke};

bool scene_actor_cache_ready(void* scene,unsigned bytes,const SceneActorCacheBindings* b,void** out,unsigned* extent,bool native_names=false) noexcept {
    return scene&&bytes>=0x410&&hologram_writable(scene,0x410)&&b&&b->parsed&&b->storage&&b->grow&&b->resolve&&
        (native_names||b->lookup)&&b->handle&&b->invalid_index&&out&&extent&&hologram_writable(out,sizeof(*out))&&hologram_writable(extent,4);
}
bool scene_actor_cache_parsed(void* scene,const SceneActorCacheBindings& h,void** parsed,unsigned* count) noexcept {
    unsigned bytes=0;*parsed=nullptr;
    const auto word=hologram_read<std::uint32_t>(scene,0x40c);
    if(!word||!h.parsed(h.context,word,parsed,&bytes)||!*parsed||bytes<0x24||
       !sin_independent::vgui_layout::Readable(*parsed,0x24))return false;
    const int n=hologram_read<int>(*parsed,0x20);if(n<0||n>0x100000)return false;
    *count=static_cast<unsigned>(n);return true;
}
bool scene_actor_cache_array(const SceneActorCacheBindings& h,std::uint32_t word,unsigned count,void** p,bool write) noexcept {
    unsigned bytes=0;*p=nullptr;
    return count&&word&&h.storage(h.context,word,p,&bytes)&&*p&&bytes>=count*4&&
        (write?hologram_writable(*p,count*4):sin_independent::vgui_layout::Readable(*p,count*4));
}
int __cdecl scene_name_actor(void*,unsigned,const char*,unsigned,const SceneActorNameBindings*,void**,unsigned*) noexcept;
int scene_actor_cache_index_impl(void* scene,unsigned bytes,int index,const SceneActorCacheBindings* b,void** out,unsigned* extent,
    const ResolvedSceneActorBindings* native_names) noexcept {
    if(!scene_actor_cache_ready(scene,bytes,b,out,extent,native_names!=nullptr))return 0;
    const auto h=*b;void* parsed=nullptr;unsigned count=0;
    if(!scene_actor_cache_parsed(scene,h,&parsed,&count))return -1;
    auto* vector=static_cast<unsigned char*>(scene)+0x3f8;
    int cached=hologram_read<int>(vector,0xc);
    if(cached<0||cached>0x100000)return -1;
    if(!cached&&count){
        const int capacity=hologram_read<int>(vector,4);
        if(capacity<0||capacity>0x100000)return -1;
        if(count>static_cast<unsigned>(capacity)&&!h.grow(h.context,vector,count-static_cast<unsigned>(capacity)))return -1;
        const int new_capacity=hologram_read<int>(vector,4);
        if(new_capacity<0||new_capacity>0x100000||count>static_cast<unsigned>(new_capacity))return -1;
        void* slots=nullptr;
        const auto word=hologram_read<std::uint32_t>(vector,0);
        if(!scene_actor_cache_array(h,word,count,&slots,true))return -1;
        hologram_write(vector,0xc,count);hologram_write(vector,0x10,word);
        for(unsigned i=0;i<count;++i)hologram_write(slots,i*4,0xffffffffu);
        cached=static_cast<int>(count);
    }
    if(index<0||index>=cached){
        if(!h.invalid_index(h.context,scene,index,static_cast<unsigned>(cached)))return -1;
        *out=nullptr;*extent=0;return 1;
    }
    void* slots=nullptr;
    if(!scene_actor_cache_array(h,hologram_read<std::uint32_t>(vector,0),static_cast<unsigned>(cached),&slots,true))return -1;
    const unsigned offset=static_cast<unsigned>(index)*4;
    const auto handle=hologram_read<std::uint32_t>(slots,offset);
    void* actor=nullptr;unsigned actor_bytes=0;
    if(handle!=0xffffffffu&&!h.resolve(h.context,handle,&actor,&actor_bytes))return -1;
    if(actor){
        if(actor_bytes<0x90||!sin_independent::vgui_layout::Readable(actor,0x90))return -1;
        if(!hologram_read<std::uint32_t>(actor,0x8c)){*out=actor;*extent=actor_bytes;return 1;}
    }
    *out=nullptr;*extent=0;
    if(static_cast<unsigned>(index)>=count)return 1;
    void* actors=nullptr;
    if(!scene_actor_cache_array(h,hologram_read<std::uint32_t>(parsed,0x14),count,&actors,false))return -1;
    const auto choreo=hologram_read<std::uint32_t>(actors,offset);
    if(!choreo)return 1;
    actor=nullptr;actor_bytes=0;
    if(native_names){
        const auto& n=*native_names;const void* object=nullptr;unsigned object_bytes=0;
        if(!n.choreo_object||!n.choreo_object(n.context,choreo,&object,&object_bytes)||!object||object_bytes<128||
           !sin_independent::vgui_layout::Readable(object,128)||!std::memchr(object,0,128))return -1;
        if(scene_name_actor(scene,bytes,static_cast<const char*>(object),n.instanced,&n.names,&actor,&actor_bytes)!=1)return -1;
    }else if(!h.lookup(h.context,scene,choreo,&actor,&actor_bytes))return -1;
    if(!actor)return 1; // original does not clear stale cache on failed lookup
    std::uint32_t replacement=0xffffffffu;
    if(!h.handle(h.context,actor,&replacement))return -1;
    // Name lookup may relocate the vector. Reload the actual backing word.
    if(hologram_read<int>(vector,0xc)!=cached||
       !scene_actor_cache_array(h,hologram_read<std::uint32_t>(vector,0),static_cast<unsigned>(cached),&slots,true))return -1;
    hologram_write(slots,offset,replacement);*out=actor;*extent=actor_bytes;return 1;
}
int __cdecl scene_actor_cache_index(void* scene,unsigned bytes,int index,const SceneActorCacheBindings* b,void** out,unsigned* extent) noexcept {
    return scene_actor_cache_index_impl(scene,bytes,index,b,out,extent,nullptr);
}
int scene_actor_cache_actor_impl(void* scene,unsigned bytes,std::uint32_t choreo,const SceneActorCacheBindings* b,void** out,unsigned* extent,
    const ResolvedSceneActorBindings* native_names) noexcept {
    if(!scene_actor_cache_ready(scene,bytes,b,out,extent,native_names!=nullptr))return 0;
    const auto h=*b;void* parsed=nullptr;unsigned count=0;
    if(!scene_actor_cache_parsed(scene,h,&parsed,&count))return -1;
    if(count){
        void* actors=nullptr;
        if(!scene_actor_cache_array(h,hologram_read<std::uint32_t>(parsed,0x14),count,&actors,false))return -1;
        for(unsigned i=0;i<count;++i)
            if(hologram_read<std::uint32_t>(actors,i*4)==choreo)
                return scene_actor_cache_index_impl(scene,bytes,static_cast<int>(i),&h,out,extent,native_names);
    }
    *out=nullptr;*extent=0;return 1;
}
int __cdecl scene_actor_cache_actor(void* scene,unsigned bytes,std::uint32_t choreo,const SceneActorCacheBindings* b,void** out,unsigned* extent) noexcept {
    return scene_actor_cache_actor_impl(scene,bytes,choreo,b,out,extent,nullptr);
}
const SceneActorCacheApi scene_actor_cache_api{version,sizeof(SceneActorCacheApi),&scene_actor_cache_index,&scene_actor_cache_actor};

bool scene_name_equal(const char* a,const char* b) noexcept {
    for(;*a&&*b;++a,++b){
        auto x=static_cast<unsigned char>(*a),y=static_cast<unsigned char>(*b);
        if(x>='A'&&x<='Z')x=static_cast<unsigned char>(x+32);
        if(y>='A'&&y<='Z')y=static_cast<unsigned char>(y+32);
        if(x!=y)return false;
    }
    return *a==*b;
}
bool scene_names_ready(void* scene,unsigned bytes,const char* name,const SceneActorNameBindings* b,void** out,unsigned* n) noexcept {
    if(bytes<0x558||!hologram_writable(scene,0x558)||!name||!b||!b->resolve||!b->handle||!b->text||
       !b->max_clients||!b->local_player||!b->find||!b->is_npc||!b->cast_actor||!b->npc_lookup||
       !hologram_writable(out,sizeof(*out))||!hologram_writable(n,4))return false;
    for(unsigned i=0;i<4096;++i){
        if(!sin_independent::vgui_layout::Readable(name+i,1))return false;
        if(!name[i])return true;
    }
    return false;
}
bool scene_names_resolve(const SceneActorNameBindings& h,std::uint32_t word,void** out,unsigned* n) noexcept {
    *out=nullptr;*n=0;
    if(word==0xffffffffu)return true;
    if(!h.resolve(h.context,word,out,n))return false;
    if(!*out)*n=0;
    return true;
}
// 221a3fe0: resolve a target alias's configured name.
bool scene_target_name(void* scene,const char* name,unsigned require_actor,const SceneActorNameBindings& h,void** out,unsigned* n) noexcept {
    if(!name)name="";
    if(scene_name_equal(name,"!activator"))
        return scene_names_resolve(h,hologram_read<std::uint32_t>(scene,0x554),out,n);
    const bool wildcard=std::strchr(name,'*')!=nullptr;
    void* previous=nullptr;
    for(unsigned attempts=0;attempts<0x100000;++attempts){
        void* candidate=nullptr;unsigned extent=0;
        if(!h.find(h.context,name,previous,nullptr,&candidate,&extent))return false;
        if(!candidate){*out=nullptr;*n=0;return true;}
        if(!wildcard||!(require_actor&0xff)){*out=candidate;*n=extent;return true;}
        void* cast=nullptr;unsigned cast_bytes=0;
        if(!h.cast_actor(h.context,candidate,&cast,&cast_bytes))return false;
        if(cast){*out=candidate;*n=extent;return true;}
        if(candidate==previous)return false;
        previous=candidate;
    }
    return false;
}
int __cdecl scene_name_entity(void* scene,unsigned bytes,const char* name,void* search_context,unsigned require_actor,
    const SceneActorNameBindings* b,void** out,unsigned* n) noexcept {
    if(!scene_names_ready(scene,bytes,name,b,out,n))return 0;
    const auto h=*b;void* result=nullptr;unsigned extent=0;
    if(scene_name_equal(name,"Player")||scene_name_equal(name,"!player")){
        int clients=0;if(!h.max_clients(h.context,&clients))return -1;
        if(clients==1&&!h.local_player(h.context,&result,&extent))return -1;
    }else{
        constexpr const char* targets[]={"!target1","!target2","!target3","!target4","!target5","!target6","!target7","!target8"};
        int target=-1;for(int i=0;i<8;++i)if(scene_name_equal(name,targets[i])){target=i;break;}
        if(target>=0){
            const unsigned handle_off=0x3b8+4*static_cast<unsigned>(target);
            const auto word=hologram_read<std::uint32_t>(scene,handle_off);
            if(!scene_names_resolve(h,word,&result,&extent))return -1;
            if(!result){
                const auto str_word=hologram_read<std::uint32_t>(scene,0x398+4*static_cast<unsigned>(target));
                const char* configured=nullptr;
                if(str_word&&!h.text(h.context,str_word,&configured))return -1;
                // Host text is borrowed; require bounded terminated storage.
                if(configured){
                    bool valid=false;for(unsigned i=0;i<4096;++i){
                        if(!sin_independent::vgui_layout::Readable(configured+i,1))return -1;
                        if(!configured[i]){valid=true;break;}
                    }if(!valid)return -1;
                }
                if(!scene_target_name(scene,configured,require_actor,h,&result,&extent))return -1;
                std::uint32_t packed=0xffffffffu;
                if(result&&!h.handle(h.context,result,&packed))return -1;
                hologram_write(scene,handle_off,packed);
                // Original 22028fb0 resolves the newly stored handle.
                if(!scene_names_resolve(h,packed,&result,&extent))return -1;
            }
        }else{
            unsigned npc=0;
            if(search_context&&!h.is_npc(h.context,search_context,&npc))return -1;
            if(search_context&&(npc&0xff)){
                if(!h.is_npc(h.context,search_context,&npc)||!(npc&0xff))return -1;
                if(!h.npc_lookup(h.context,search_context,name,&result,&extent))return -1;
            }else if(!h.find(h.context,name,nullptr,search_context,&result,&extent))return -1;
        }
    }
    *out=result;*n=result?extent:0;return 1;
}
int __cdecl scene_name_actor(void* scene,unsigned bytes,const char* name,unsigned instanced,const SceneActorNameBindings* b,void** out,unsigned* n) noexcept {
    if(instanced>1||!scene_names_ready(scene,bytes,name,b,out,n)||(instanced&&(bytes<0x560||!sin_independent::vgui_layout::Readable(scene,0x560))))return 0;
    const auto h=*b;void* actor=nullptr;unsigned extent=0;
    if(instanced){
        if(!scene_names_resolve(h,hologram_read<std::uint32_t>(scene,0x55c),&actor,&extent))return -1;
        if(actor){
            unsigned npc=0;
            if(!scene_names_resolve(h,hologram_read<std::uint32_t>(scene,0x55c),&actor,&extent)||!actor||
               !h.is_npc(h.context,actor,&npc))return -1;
            if(npc&0xff){*out=actor;*n=extent;return 1;}
        }
    }
    if(scene_name_entity(scene,bytes,name,nullptr,1,&h,&actor,&extent)!=1)return -1;
    if(actor){
        void* cast=nullptr;unsigned cast_bytes=0;
        if(!h.cast_actor(h.context,actor,&cast,&cast_bytes))return -1;
        actor=cast;extent=cast_bytes;
    }
    *out=actor;*n=actor?extent:0;return 1;
}
const SceneActorNamesApi scene_actor_names_api{version,sizeof(SceneActorNamesApi),&scene_name_entity,&scene_name_actor};
int __cdecl resolved_scene_index(void* scene,unsigned bytes,int index,const ResolvedSceneActorBindings* b,void** out,unsigned* n) noexcept {
    if(!b||b->instanced>1||!b->choreo_object)return 0;
    const auto h=*b;return scene_actor_cache_index_impl(scene,bytes,index,&h.cache,out,n,&h);
}
int __cdecl resolved_scene_actor(void* scene,unsigned bytes,std::uint32_t actor,const ResolvedSceneActorBindings* b,void** out,unsigned* n) noexcept {
    if(!b||b->instanced>1||!b->choreo_object)return 0;
    const auto h=*b;return scene_actor_cache_actor_impl(scene,bytes,actor,&h.cache,out,n,&h);
}
int __cdecl resolved_scene_speech(const void* callback,unsigned cb_bytes,void* scene,unsigned bytes,unsigned slot,float time,
    void* parsed,unsigned parsed_bytes,void* event,unsigned extent,const SceneSpeechCallbackBindings* b,
    const ResolvedSceneActorBindings* names,unsigned* out) noexcept {
    if(!names)return 0;
    const auto h=*names;
    return speech_callback_invoke_impl(callback,cb_bytes,scene,bytes,slot,time,parsed,parsed_bytes,event,extent,b,&h,out);
}
const ResolvedSceneActorApi resolved_scene_actor_api{version,sizeof(ResolvedSceneActorApi),&resolved_scene_index,&resolved_scene_actor,&resolved_scene_speech};
int __cdecl prepared_speech_start(void* scene,unsigned bytes,void* actor,unsigned actor_bytes,std::uint32_t parsed,void* event,unsigned extent,int level,
    const SceneSpeechExecutionBindings* b,const SceneSpeechPreparationBindings* prep) noexcept {
    if(!prep)return 0;
    const auto h=*prep;return speech_execute_impl(scene,bytes,actor,actor_bytes,parsed,event,extent,level,b,&h);
}
int __cdecl prepared_speech_callback(const void* cb,unsigned cb_bytes,void* scene,unsigned bytes,unsigned slot,float time,
    void* parsed,unsigned parsed_bytes,void* event,unsigned extent,const SceneSpeechCallbackBindings* b,
    const ResolvedSceneActorBindings* names,const SceneSpeechPreparationBindings* prep,unsigned* out) noexcept {
    if(!prep)return 0;
    const auto h=*prep;return speech_callback_invoke_impl(cb,cb_bytes,scene,bytes,slot,time,parsed,parsed_bytes,event,extent,b,names,out,&h);
}
const SceneSpeechPreparationApi scene_speech_preparation_api{version,sizeof(SceneSpeechPreparationApi),&scene_speech_prepare,&prepared_speech_start,&prepared_speech_callback};
// 22113bb0 flat events, 22113e00 pending lookup, 22111080 unique
// pause-child attachment, 22107170 channel index, 22118c99 tracked append.
struct NativeChoreographyClockState {
    NativeChoreographyClockBindings h;
    ParsedSimulationBindings sim{};
    ParsedSceneResetBindings reset{};
    ParsedSceneResumeBindings resume{};
    static NativeChoreographyClockState& self(void* p) noexcept{return *static_cast<NativeChoreographyClockState*>(p);}
    static int __cdecl storage(void* p,std::uint32_t w,void** out,unsigned* bytes) noexcept {
        const auto& v=self(p).h.vectors;return v.storage(v.context,w,out,bytes);
    }
    static bool list(void* p,void* v,void*& rows,unsigned& count) noexcept {
        if(!hologram_writable(v,20))return false;
        const int n=hologram_read<int>(v,0xc),cap=hologram_read<int>(v,4);
        rows=nullptr;count=0;
        if(n<0||cap<n||cap>4096)return false;
        count=static_cast<unsigned>(n);
        if(!count)return true;
        unsigned bytes=0;
        return storage(p,hologram_read<std::uint32_t>(v,0),&rows,&bytes)&&bytes>=count*4&&hologram_writable(rows,count*4);
    }
    static int indexed(void* p,void* root,unsigned off,unsigned i,void** out,unsigned* bytes) noexcept {
        *out=nullptr;*bytes=0;
        if(!hologram_writable(root,0x220))return 0;
        void* rows=nullptr;unsigned n=0;
        if(!list(p,static_cast<unsigned char*>(root)+off,rows,n))return 0;
        if(i>=n)return 1; // original indexed accessor returns null
        const auto w=hologram_read<std::uint32_t>(rows,i*4);
        return !w||storage(p,w,out,bytes);
    }
    static int __cdecl event(void* p,void* root,unsigned i,void** out,unsigned* bytes) noexcept {
        return indexed(p,root,0,i,out,bytes);
    }
    static int __cdecl pending(void* p,void* root,unsigned i,void** out,unsigned* bytes) noexcept {
        return indexed(p,root,0x64,i,out,bytes);
    }
    static int append(void* p,void* v,void* obj,bool unique) noexcept {
        void* rows=nullptr;unsigned n=0;
        if(!list(p,v,rows,n)||!obj)return 0;
        const auto w=scene_word(obj);
        if(unique)for(unsigned i=0;i<n;++i)if(hologram_read<std::uint32_t>(rows,i*4)==w)return 1;
        return choreography_insert_word(&self(p).h.vectors,v,n,w);
    }
    static int __cdecl attach(void* p,void* parent,void* child) noexcept {
        if(!hologram_writable(parent,0x3c0)||!hologram_writable(child,0x3c0))return 0;
        return append(p,static_cast<unsigned char*>(parent)+0x3a4,child,true);
    }
    static int __cdecl track(void* p,void* root,void* event) noexcept {
        if(!hologram_writable(root,0x220)||!hologram_writable(event,0x3c0))return 0;
        return append(p,static_cast<unsigned char*>(root)+0x50,event,false);
    }
    static int __cdecl channel(void* p,std::uint32_t actor,std::uint32_t channel,int* out) noexcept {
        void *a=nullptr,*rows=nullptr;unsigned bytes=0,n=0;
        if(!storage(p,actor,&a,&bytes)||bytes<0x118||!hologram_writable(a,0x118)||
           !list(p,static_cast<unsigned char*>(a)+0x100,rows,n))return 0;
        *out=-1;for(unsigned i=0;i<n;++i)if(hologram_read<std::uint32_t>(rows,i*4)==channel){*out=static_cast<int>(i);break;}
        return 1;
    }
    static int __cdecl callback(void* p,std::uint32_t word,unsigned slot,float time,void* root,void* event) noexcept {
        const auto& h=self(p).h;
        if(!hologram_writable(root,0x220)||!hologram_writable(event,0x3c0))return 0;
        if(hologram_read<int>(event,0)==5&&h.speech){
            if(!h.scene||h.scene_bytes<0x549||!sin_independent::vgui_layout::Readable(h.scene,0x549))return 0;
            auto* cb=static_cast<unsigned char*>(h.scene)+0x410;
            if(word!=scene_word(cb))return 0;
            unsigned result=0;
            return speech_callback_invoke_impl(cb,8,h.scene,h.scene_bytes,slot,time,root,0x220,event,0x3c0,
                h.speech,h.actors,&result,h.preparation)==1;
        }
        return h.other_event&&h.other_event(h.context,word,slot,time,root,event);
    }
    explicit NativeChoreographyClockState(const NativeChoreographyClockBindings& b) noexcept:h(b){
        sim.context=this;sim.classifier.context=this;sim.classifier.pending=&pending;sim.classifier.attach=&attach;
        sim.events.context=this;sim.events.event=&event;sim.channel_index=&channel;sim.track=&track;sim.callback=&callback;
        reset.context=this;reset.events=sim.events;reset.storage=&storage;
        reset.grow=[](void* p,void* v,unsigned extra) noexcept -> int{return choreography_grow_vector(&self(p).h.vectors,v,extra,4);};
        resume.context=this;resume.event=&storage;
        resume.words=[](void* p,std::uint32_t w,const void** out,unsigned* bytes) noexcept -> int{
            void* data=nullptr;const int status=storage(p,w,&data,bytes);*out=data;return status;
        };
        resume.allocate=[](void* p,unsigned n,void** out,unsigned* bytes) noexcept -> int{
            const auto& v=self(p).h.vectors;return v.resize(v.context,0,n,out,bytes);
        };
        resume.release=[](void* p,void* data) noexcept -> int{
            const auto& h=self(p).h;return h.release_memory(h.vectors.context,scene_word(data));
        };
        resume.start=[](void* p,std::uint32_t w,float time,void* root,void* event) noexcept -> int{
            return callback(p,w,0,time,root,event);
        };
    }
};
bool native_choreography_clock_ready(const NativeChoreographyClockBindings* b) noexcept {
    return b&&choreography_vector_ready(&b->vectors)&&b->release_memory;
}
int __cdecl native_choreography_clock_reset(void* root,unsigned bytes,unsigned forward,std::uint32_t a,std::uint32_t z,const NativeChoreographyClockBindings* b) noexcept {
    if(!native_choreography_clock_ready(b))return 0;
    NativeChoreographyClockState s(*b);return choreography_reset(root,bytes,forward,a,z,&s.reset);
}
int __cdecl native_choreography_clock_simulate(void* root,unsigned bytes,float target,const NativeChoreographyClockBindings* b) noexcept {
    if(!native_choreography_clock_ready(b))return 0;
    NativeChoreographyClockState s(*b);return parsed_scene_simulate(root,bytes,target,&s.sim);
}
int __cdecl native_choreography_clock_resume(void* root,unsigned bytes,const NativeChoreographyClockBindings* b) noexcept {
    if(!native_choreography_clock_ready(b))return 0;
    NativeChoreographyClockState s(*b);return parsed_scene_resume(root,bytes,&s.resume);
}
const NativeChoreographyClockApi native_choreography_clock_api{version,sizeof(NativeChoreographyClockApi),
    &native_choreography_clock_reset,&native_choreography_clock_simulate,&native_choreography_clock_resume};
// Original jump tables 221a27c8 (start types 1..16), 221a2a58
// (stop types 2..16). Zero denotes a special or no-operation branch.
int __cdecl native_choreography_dispatch(std::uint32_t word,unsigned slot,float time,void* parsed,void* event,
    const NativeChoreographyDispatchBindings* b,unsigned* result) noexcept {
    if(!b||!hologram_writable(result,4)||!b->scene||b->scene_bytes<0x549||
       !hologram_writable(b->scene,0x549)||!hologram_writable(parsed,0x220)||
       !hologram_writable(event,0x3c0)||(slot!=0&&slot!=4&&slot!=8&&slot!=12))return 0;
    const auto h=*b;auto* scene=static_cast<unsigned char*>(h.scene);
    if(word!=scene_word(scene+0x410))return 0;
    const auto owner=hologram_read<std::uint32_t>(scene,0x414);
    if(!owner){*result=1;return 1;}
    if(owner!=scene_word(scene))return 0;
    const unsigned type=hologram_read<unsigned>(event,0);
    if(type==5)return 0; // existing native speech route has separate leaves
    const auto complete=[&](){*result=1;return 1;};
    const auto nested=[&](void*& child,unsigned& extent){
        child=nullptr;extent=0;const auto w=hologram_read<std::uint32_t>(event,0x324);
        return !w||(w!=scene_word(parsed)&&h.storage&&h.storage(h.context,w,&child,&extent)&&child&&extent>=0x220&&hologram_writable(child,0x220));
    };
    if(slot==8){
        if(type!=11)return complete();
        void* child=nullptr;unsigned extent=0,finished=0;
        if(!nested(child,extent))return -1;
        if(!child)return complete();
        if(parsed_scene_finished(child,extent,&finished)!=1)return -1;
        if(finished)return complete();
        // 221a2b60 passes scene+3e0 directly as the simulation target.
        return h.clock&&native_choreography_clock_simulate(child,extent,hologram_read<float>(scene,0x3e0),h.clock)==1?complete():-1;
    }
    if(slot==12&&type==11)return complete();
    const auto* name=static_cast<const char*>(event)+4;
    const auto* param=static_cast<const char*>(event)+0x84;
    if(!h.trace||!std::memchr(name,0,128)||!std::memchr(param,0,128))return 0;
    auto trace=[&](ChoreographyDispatchTrace kind){return h.trace(h.context,scene,event,time,kind)!=0;};
    if(slot!=12&&scene_event_ignored(name)){
        return slot==0&&!trace(ChoreographyDispatchTrace::Ignored)?-1:complete();
    }
    void* actor=nullptr;unsigned actor_bytes=0;
    const auto choreo=hologram_read<std::uint32_t>(event,0x2b4);
    if(choreo&&!(h.actors?resolved_scene_actor(scene,h.scene_bytes,choreo,h.actors,&actor,&actor_bytes)==1:
        h.actor&&h.actor(h.context,scene,choreo,&actor,&actor_bytes)))return -1;
    if(choreo&&!actor&&(slot==0||slot==12)){
        return trace(ChoreographyDispatchTrace::MissingActor)?complete():-1;
    }
    if(slot==12){
        if(!actor)return complete();
        return h.records?actor_event_check_one(actor,actor_bytes,time,scene_word(parsed),event,h.records,result):0;
    }
    if(!trace(slot==0?ChoreographyDispatchTrace::Start:ChoreographyDispatchTrace::Stop))return -1;
    if(!type||type>16)return complete();
    const auto dispatch=[&](unsigned offset,void* target=nullptr){
        return h.virtual_event&&h.virtual_event(h.context,scene,offset,parsed,event,actor,target,param)?complete():-1;
    };
    if(slot==4){
        if(type==11){
            void* child=nullptr;unsigned extent=0;if(!nested(child,extent))return -1;
            return !child?complete():h.clock&&native_choreography_clock_reset(child,extent,1,0,0,h.clock)==1?complete():-1;
        }
        if(type==16&&!std::strncmp(param,"debugtext",9))return complete();
        constexpr unsigned stop_slots[]={0,0,0x2d0,0x2e8,0x2f0,0x2f8,0x2e0,0x308,0x300,0,0x2d8,0,0,0x314,0,0x324,0x31c};
        const unsigned offset=stop_slots[type];
        return !offset||(!actor&&type!=13&&type!=15)?complete():dispatch(offset);
    }
    if(type==4&&hologram_read<std::uint32_t>(event,0x188)==0xbf800000u){
        const auto start=hologram_read<float>(event,0x184);
        const auto end=static_cast<float>(static_cast<double>(start)+1.0);
        hologram_write(event,0x188,end<start?start:end);
        ChoreographyFinalizeBindings storage{};storage.context=h.context;storage.storage=h.storage;
        if(!gesture_refresh(event,storage))return -1;
    }
    if(type==9){
        if(hologram_read<unsigned char>(scene,0x548))return complete();
        const auto i=std::strtol(param,nullptr,10);
        if(i<1||i>8)return complete();
        return h.output&&h.output(h.context,scene+0x464+24*(i-1),actor?actor:scene,scene,0.f)?complete():-1;
    }
    if(type==16&&!std::strncmp(param,"debugtext",9)){
        // Original passes param+10. Handle a bare 9-character token
        // without reading past its terminator.
        return h.debug_text&&h.debug_text(h.context,std::strlen(param)>9?param+10:"")?complete():-1;
    }
    constexpr unsigned start_slots[]={0,0x32c,0x2cc,0x2e4,0x2ec,0x2f4,0x2dc,0x304,0x2fc,0,0x2d4,0x30c,0x328,0x310,0x330,0x320,0x318};
    const bool needs_actor=type>=2&&type<=11||type==16;
    if(needs_actor&&!actor)return complete();
    if(type==3||type==4||type==8){
        void* target=nullptr;
        if(!h.target||!h.target(h.context,scene,param,actor,0,&target))return -1;
        if(!target)return trace(ChoreographyDispatchTrace::MissingTarget)?complete():-1;
        return dispatch(start_slots[type],target);
    }
    return dispatch(start_slots[type]);
}
int __cdecl native_choreography_other_event(void* p,std::uint32_t word,unsigned slot,float time,void* parsed,void* event) noexcept {
    unsigned result=0;return native_choreography_dispatch(word,slot,time,parsed,event,
        static_cast<const NativeChoreographyDispatchBindings*>(p),&result)==1;
}
const NativeChoreographyDispatchApi native_choreography_dispatch_api{version,sizeof(NativeChoreographyDispatchApi),&native_choreography_dispatch,&native_choreography_other_event};
bool speech_drain_ready(const SceneSpeechDrainBindings* b,unsigned* out) noexcept {
    return b&&b->storage&&b->resolve&&b->clock&&b->open_filter&&b->emit&&b->close_filter&&hologram_writable(out,4);
}
int __cdecl speech_drain(void* manager,unsigned bytes,const SceneSpeechDrainBindings* b,unsigned* out) noexcept {
    if(bytes<0x3bc||!hologram_writable(manager,0x3bc)||!speech_drain_ready(b,out))return 0;
    const auto h=*b;const int count=hologram_read<int>(manager,0x3b4);
    if(count>0x100000)return -1;
    unsigned emitted=0;
    // The original snapshots count, reloads vector backing each iteration.
    // Newly appended records are not visited and count is reset at the end.
    for(int i=0;i<count;++i){
        if(hologram_read<int>(manager,0x3b4)<count)return -1;
        const auto word=hologram_read<std::uint32_t>(manager,0x3a8);
        void* storage=nullptr;unsigned extent=0;
        const auto needed=static_cast<unsigned>(i+1)*0x8c;
        if(!word||!h.storage(h.context,word,&storage,&extent)||extent<needed||
           !sin_independent::vgui_layout::Readable(storage,needed))return -1;
        alignas(4) unsigned char row[0x8c];std::memcpy(row,static_cast<unsigned char*>(storage)+i*0x8c,sizeof(row));
        const auto handle=hologram_read<std::uint32_t>(row,0);
        if(handle==0xffffffffu)continue;
        void* actor=nullptr;
        if(!h.resolve(h.context,handle,&actor))return -1;
        if(!actor)continue;
        const auto offset=hologram_read<float>(row,0x88);
        // x87 status mask 0x41: <=10 or unordered enters emission path.
        if(offset>10.f)continue;
        const auto* sound=reinterpret_cast<const char*>(row+4);
        if(!std::memchr(sound,0,128))return -1;
        if(!h.resolve(h.context,handle,&actor)||!actor)return -1;
        void* filter=nullptr;
        if(!h.open_filter(h.context,actor,.8f,&filter)||!filter)return -1;
        const auto close=[&](bool ok){const bool closed=h.close_filter(h.context,filter)!=0;return ok&&closed;};
        float now=0;
        if(!h.clock(h.context,&now)||!h.resolve(h.context,handle,&actor)||!actor){(void)close(false);return -1;}
        const float start=static_cast<float>(static_cast<double>(now)-offset);
        if(!close(h.emit(h.context,filter,actor,sound,2,1.f,hologram_read<int>(row,0x84),start)!=0))return -1;
        ++emitted;
    }
    hologram_write(manager,0x3b4,0u);*out=emitted;return 1;
}
int __cdecl speech_flush(SceneManagerSession* session,const SceneManagerRuntimeBindings* runtime,const SceneSpeechDrainBindings* b,unsigned* out) noexcept {
    if(!speech_drain_ready(b,out))return 0;
    const auto h=*b;void* manager=nullptr;unsigned n=0;
    auto status=scene_manager_ensure(session,runtime,&manager,&n);
    if(status!=1)return status;
    if(!manager){*out=0;return 1;}
    status=scene_manager_ensure(session,runtime,&manager,&n);
    if(status!=1)return status;
    if(!manager)return -1;
    session->busy=1;
    status=speech_drain(manager,n,&h,out);
    session->busy=0;
    if(status!=1)session->blocked=1; // do not replay partially emitted records
    return status;
}
const SceneSpeechDrainApi scene_speech_drain_api{version,sizeof(SceneSpeechDrainApi),&speech_drain,&speech_flush};


int __cdecl integrated_scene(void* npc,const char* name,float delay,const NpcScenePlaybackBindings* b,const SceneEntityLifecycleBindings* life,float* out) noexcept {
    if(!life)return 0;const auto h=*life;
    return npc_scene_play(npc,name,delay,b,out,false,&h);
}
int __cdecl integrated_sound(void* npc,const char* name,const NpcScenePlaybackBindings* b,const SceneEntityLifecycleBindings* life,float* out) noexcept {
    if(!life)return 0;const auto h=*life;
    return npc_scene_play(npc,name,0,b,out,true,&h);
}
const IntegratedScenePlaybackApi integrated_scene_playback_api{version,sizeof(IntegratedScenePlaybackApi),&integrated_scene,&integrated_sound};











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
struct JessicaDialogueCall {
    HologramControl* ctrl;JessicaDialogueBindings host;
    const TalkManagerRuntimeBindings* manager=nullptr;
    const ResponseChoreographyBindings* choreography=nullptr;
    const SceneEntityLifecycleBindings* lifecycle=nullptr;
    const NativeGeneratedResponseBindings* generated=nullptr;
};
int __cdecl dialogue_jessica_clock(void* p,float* out) noexcept {
    auto& x=*static_cast<JessicaDialogueCall*>(p);return x.host.clock(x.host.context,out);
}
int __cdecl dialogue_jessica_criteria(void* p,void* npc,const void* record,float now,const char* name) noexcept {
    auto& x=*static_cast<JessicaDialogueCall*>(p);const auto& h=x.host;
    if(!x.ctrl->busy||npc!=x.ctrl->object)return 0;
    CountCriteria criteria{};
    if(!h.criteria(h.context,record,now,&criteria)||!std::memchr(criteria.bytes,0,sizeof(criteria.bytes)))return 0;
    unsigned played=0;
    // 22362154 / 22362236 -> 2208f500(name,modifier,0).
    // Original ignores whether it spoke; only transport failure is fatal.
    const auto* mods=reinterpret_cast<const char*>(criteria.bytes);
    if(x.choreography||x.generated){
        const CompanionDeadlineSource source{x.manager?nullptr:h.cooldown->global,x.manager};
        return companion_request_impl(npc,x.ctrl->bytes,source,h.semaphore,&h.talk,name,mods,0,&played,x.choreography,x.lifecycle,x.generated)==1;
    }
    return (x.manager?managed_companion_request(npc,x.ctrl->bytes,x.manager,h.semaphore,&h.talk,name,mods,0,&played):
        companion_request(npc,x.ctrl->bytes,h.cooldown,h.semaphore,&h.talk,name,mods,0,&played))==1;
}
int __cdecl dialogue_jessica(HologramControl* ctrl,JessicaAttentionState* state,const JessicaDialogueBindings* b,
                             const void* obs,unsigned bytes) noexcept {
    if(!b||!b->clock||!b->criteria||!b->cooldown||!b->semaphore)return 0;
    JessicaDialogueCall call{ctrl,*b};
    const JessicaAttentionBindings host{&call,&dialogue_jessica_clock,&dialogue_jessica_criteria};
    return jessica_attention(ctrl,state,&host,obs,bytes);
}
int __cdecl dialogue_jessica_managed(HologramControl* ctrl,JessicaAttentionState* state,const JessicaDialogueBindings* b,
                                     const TalkManagerRuntimeBindings* manager,const void* obs,unsigned bytes) noexcept {
    if(!b||!b->clock||!b->criteria||!b->semaphore||!manager)return 0;
    JessicaDialogueCall call{ctrl,*b,manager};
    const JessicaAttentionBindings host{&call,&dialogue_jessica_clock,&dialogue_jessica_criteria};
    return jessica_attention(ctrl,state,&host,obs,bytes);
}
const ManagedJessicaObservationApi managed_jessica_observation_api{version,sizeof(ManagedJessicaObservationApi),&dialogue_jessica_managed};
struct CountDialogueCall {
    HologramControl* ctrl;CountDialogueBindings host;
    void* exp=nullptr;unsigned extent=0;
    const ResponseChoreographyBindings* choreography=nullptr;
    const SceneEntityLifecycleBindings* lifecycle=nullptr;
    const NativeGeneratedResponseBindings* generated=nullptr;
};
int __cdecl dialogue_count_resolve(void* p,std::uint32_t handle,void** out) noexcept {
    auto& x=*static_cast<CountDialogueCall*>(p);return x.host.resolve(x.host.context,handle,out);
}
int __cdecl dialogue_count_clock(void* p,float* out) noexcept {
    auto& x=*static_cast<CountDialogueCall*>(p);return x.host.clock(x.host.context,out);
}
int __cdecl dialogue_count_expeditor(void* p,void* npc,void** out) noexcept {
    auto& x=*static_cast<CountDialogueCall*>(p);x.exp=nullptr;x.extent=0;
    if(!x.host.expeditor(x.host.context,npc,&x.exp,&x.extent))return 0;
    if(x.exp&&(x.extent<0x7c||!sin_independent::vgui_layout::Readable(x.exp,0x7c)))return 0;
    *out=x.exp;return 1;
}
int __cdecl dialogue_count_speaking(void* p,void* exp,unsigned* out) noexcept {
    auto& x=*static_cast<CountDialogueCall*>(p);float now=0;
    return x.ctrl->busy&&exp==x.exp&&x.host.clock(x.host.context,&now)&&response_speaking(exp,x.extent,now,out)==1;
}
int __cdecl dialogue_count_criteria(void* p,const void* record,float now,CountCriteria* out) noexcept {
    auto& x=*static_cast<CountDialogueCall*>(p);
    return x.ctrl->busy&&x.host.criteria(x.host.context,record,now,out);
}
int __cdecl dialogue_count_speak(void* p,void* exp,const char* name,const CountCriteria* criteria,unsigned* played) noexcept {
    auto& x=*static_cast<CountDialogueCall*>(p);
    if(!x.ctrl->busy||exp!=x.exp||!std::memchr(criteria->bytes,0,sizeof(criteria->bytes)))return 0;
    // 22394440 bypasses the companion gate. Its phase advances on normal
    // no-match too, so return transport success, not the played boolean.
    const auto* mods=reinterpret_cast<const char*>(criteria->bytes);
    if(x.choreography||x.generated){
        if(hologram_read<std::uint32_t>(exp,4)!=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(x.ctrl->object)))return 0;
        return (x.generated?native_generated_response_speak(exp,x.extent,&x.host.speak,x.generated,name,mods,played):
            integrated_response_speak(exp,x.extent,&x.host.speak,x.choreography,x.lifecycle,name,mods,played))==1;
    }
    return response_speak(exp,x.extent,&x.host.speak,name,mods,played)==1;
}
bool dialogue_count_ready(const CountDialogueBindings* b) noexcept {
    return b&&b->resolve&&b->clock&&b->expeditor&&b->criteria;
}
CountBindings dialogue_count_bindings(CountDialogueCall& call) noexcept {
    CountBindings host{};
    host.context=&call;host.resolve=&dialogue_count_resolve;host.clock=&dialogue_count_clock;
    host.expeditor=&dialogue_count_expeditor;host.speaking=&dialogue_count_speaking;
    host.criteria=&dialogue_count_criteria;host.speak=&dialogue_count_speak;return host;
}
int __cdecl dialogue_count(HologramControl* ctrl,const CountDialogueBindings* b,const void* obs,unsigned bytes) noexcept {
    if(!dialogue_count_ready(b))return 0;
    CountDialogueCall call{ctrl,*b};const auto host=dialogue_count_bindings(call);
    return count_observe(ctrl,&host,obs,bytes);
}
const NpcDialogueApi npc_dialogue_api{version,sizeof(NpcDialogueApi),&dialogue_jessica,&dialogue_count};
int __cdecl integrated_jessica_observe(HologramControl* ctrl,JessicaAttentionState* state,const JessicaDialogueBindings* b,
    const TalkManagerRuntimeBindings* manager,const ResponseChoreographyBindings* c,const SceneEntityLifecycleBindings* life,const void* obs,unsigned bytes) noexcept {
    if(!b||!b->clock||!b->criteria||!b->semaphore||(!manager&&!b->cooldown)||!integrated_response_ready(c,life))return 0;
    const auto choreography=*c;const auto lifecycle=*life;
    JessicaDialogueCall call{ctrl,*b,manager,&choreography,&lifecycle};
    const JessicaAttentionBindings host{&call,&dialogue_jessica_clock,&dialogue_jessica_criteria};
    return jessica_attention(ctrl,state,&host,obs,bytes);
}
int __cdecl integrated_count_observe(HologramControl* ctrl,const CountDialogueBindings* b,const ResponseChoreographyBindings* c,
    const SceneEntityLifecycleBindings* life,const void* obs,unsigned bytes) noexcept {
    if(!dialogue_count_ready(b)||!integrated_response_ready(c,life))return 0;
    const auto choreography=*c;const auto lifecycle=*life;
    CountDialogueCall call{ctrl,*b,nullptr,0,&choreography,&lifecycle};const auto host=dialogue_count_bindings(call);
    return count_observe(ctrl,&host,obs,bytes);
}
const IntegratedNpcDialogueApi integrated_npc_dialogue_api{version,sizeof(IntegratedNpcDialogueApi),&integrated_jessica_observe,&integrated_count_observe};


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

bool humanoid_profile_leaf(void* npc,const JcProfileBindings& h) noexcept {
    hologram_write(npc,0x614,247);
    if(hologram_read<int>(npc,0x9c)!=8){
        if(!h.health_dirty(h.context,npc))return false;
        hologram_write(npc,0x9c,8);
    }
    hologram_write(npc,0x618,0.5f);
    hologram_write(npc,0x850,0);
    hologram_write(npc,0x8ec,hologram_read<unsigned>(npc,0x8ec)|0x801801u);
    hologram_write(npc,0x8ec,hologram_read<unsigned>(npc,0x8ec)|0x02000000u);
    hologram_write(npc,0xbb,static_cast<unsigned char>(hologram_read<unsigned char>(npc,0xbb)|0x58u));
    return h.npc_initialize(h.context,npc)!=0;
}
int __cdecl jc_profile(HologramControl* c,const JcProfileBindings* b) noexcept {
    if(!health_control(c,jc_bytes)||!b||!b->health_dirty||!b->npc_initialize)return 0;
    c->busy=1;const auto h=*b;return disp_finish(c,humanoid_profile_leaf(c->object,h));
}
int __cdecl jessica_reaction_update(HologramControl* c,const JessicaReactionBindings* b) noexcept {
    if(!health_control(c,jessica_size)||!b||!b->base_update||!b->resolve||!b->restore_unparented||!b->post_restore||!b->condition||!b->speak)return 0;
    c->busy=1;const auto h=*b;auto* npc=c->object;
    if(!h.base_update(h.context,npc))return disp_finish(c,false);
    if(hologram_read<int>(npc,0x204)==0){
        const auto parent=hologram_read<std::uint32_t>(npc,0xf0);void* target=nullptr;
        if(parent!=0xffffffffu&&!h.resolve(h.context,parent,&target))return disp_finish(c,false);
        if(!target&&(!h.restore_unparented(h.context,npc)||!h.post_restore(h.context,npc)))return disp_finish(c,false);
    }
    unsigned active=0;
    if(!h.condition(h.context,npc,0x43,&active))return disp_finish(c,false);
    return disp_finish(c,!active||h.speak(h.context,npc,"TLK_PLDEAD",0,0)!=0);
}
int __cdecl jessica_reaction_event(HologramControl* c,const JessicaReactionBindings* b,int registered_id,int id,std::uint32_t arg1,std::uint32_t arg2,unsigned* handled) noexcept {
    if(!health_control(c,jessica_size)||!b||!b->speech_ready||!b->speak||!b->base_event||!handled)return 0;
    c->busy=1;const auto h=*b;auto* npc=c->object;
    if(id!=registered_id)return disp_finish(c,h.base_event(h.context,npc,id,arg1,arg2,handled)!=0);
    unsigned ready=0;
    if(!h.speech_ready(h.context,npc,&ready))return disp_finish(c,false);
    if(ready&&!h.speak(h.context,npc,"TLK_PLYR_PHYSATK",0,0))return disp_finish(c,false);
    *handled=1;return disp_finish(c,true);
}
int __cdecl jessica_reaction_schedule(HologramControl* c,const JessicaReactionBindings* b,int requested,int* result) noexcept {
    if(!health_control(c,jessica_size)||!b||!b->base_schedule||!b->clock||!b->secondary_timestamp||!result)return 0;
    c->busy=1;const auto h=*b;auto* npc=c->object;
    int selected=0;
    if(!h.base_schedule(h.context,npc,requested,&selected))return disp_finish(c,false);
    *result=selected;
    const auto state=hologram_read<int>(npc,0x850);
    if(selected!=1||(state!=2&&state!=3))return disp_finish(c,true);
    float now=0;
    if(!h.clock(h.context,&now))return disp_finish(c,false);
    // x87 unordered comparisons do not take the less-than branch.
    if(static_cast<double>(now)-hologram_read<float>(npc,0x99c)<3.0){*result=0x48;return disp_finish(c,true);}
    float stamp=0;
    if(!h.secondary_timestamp(h.context,npc,&stamp)||!h.clock(h.context,&now))return disp_finish(c,false);
    if(static_cast<double>(now)-stamp<8.0)*result=0x48;
    return disp_finish(c,true);
}
const CompanionReactionApi companion_reaction_api{version,sizeof(CompanionReactionApi),&jc_profile,&jessica_reaction_update,&jessica_reaction_event,&jessica_reaction_schedule,&health_ack};

std::uint32_t count_pointer(const void* p) noexcept {
    return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(p));
}
std::uint32_t count_embedded(void* npc) noexcept {
    return count_pointer(static_cast<unsigned char*>(npc)+0x1000);
}
bool count_entry(void* npc,int index,std::uint32_t& entry) noexcept {
    if(index<0||index>=4096)return false;
    const auto vector=hologram_read<std::uint32_t>(npc,0xc1c);
    if(!vector||vector>0xffffffffu-static_cast<unsigned>(index)*4u)return false;
    const auto* address=reinterpret_cast<const void*>(static_cast<std::uintptr_t>(vector)+static_cast<unsigned>(index)*4u);
    if(!sin_independent::vgui_layout::Readable(address,4))return false;
    entry=hologram_read<std::uint32_t>(address,0);
    return entry!=0;
}
bool count_register_leaf(void* npc,const CountSchedulerBindings& h) noexcept {
    const auto embedded=count_embedded(npc);
    const int before=hologram_read<int>(npc,0xc28);
    if(before<0||before>=4096)return false;
    if(!h.append(h.context,npc,embedded))return false;
    std::uint32_t inserted=0;
    if(hologram_read<int>(npc,0xc28)!=before+1||!count_entry(npc,before,inserted)||inserted!=embedded)return false;
    // Embedded behavior vslot +4 is 220cdbd0, a plain owner write.
    hologram_write(npc,0x1004,count_pointer(npc));
    hologram_write(npc,0x100c,count_pointer(static_cast<unsigned char*>(npc)+0xc14));
    return h.register_base(h.context,npc)!=0;
}
int __cdecl count_register_interrogation(HologramControl* c,const CountSchedulerBindings* b) noexcept {
    if(!health_control(c,count_size)||!b||!b->append||!b->register_base)return 0;
    const int count=hologram_read<int>(c->object,0xc28);
    if(count<0||count>=4096)return 0;
    c->busy=1;const auto h=*b;return disp_finish(c,count_register_leaf(c->object,h));
}
int __cdecl count_select(HologramControl* c,CountSchedulerState* state,const CountSchedulerBindings* b,int* result) noexcept {
    if(!health_control(c,count_size)||!state||!hologram_writable(state,sizeof(*state))||state->busy||
       !b||!b->eligible||!b->begin||!b->gather||!b->end||!b->reset||!b->changed||!b->schedule||
       !b->is_local||!b->translate||!b->base_schedule||!result)return 0;
    c->busy=1;state->busy=1;const auto h=*b;auto* npc=c->object;
    auto finish=[&](bool ok){state->busy=0;return disp_finish(c,ok);};
    const auto embedded=count_embedded(npc);
    std::uint32_t selected=0;
    for(int i=0;i<hologram_read<int>(npc,0xc28);++i){
        std::uint32_t behavior=0;
        if(!count_entry(npc,i,behavior))return finish(false);
        unsigned eligible=0;
        if(behavior==embedded)eligible=hologram_read<unsigned char>(npc,0x1010);
        else if(!h.eligible(h.context,behavior,&eligible))return finish(false);
        if(!eligible)continue;
        // Concrete Count's approval virtual +77c (2216dde0) is true.
        // The original reloads vector entries before approval and install.
        if(!count_entry(npc,i,selected))return finish(false);
        break;
    }
    const auto prior=hologram_read<std::uint32_t>(npc,0xc18);
    hologram_write(npc,0xc18,selected);
    if(prior!=selected){
        if(selected){
            if(!h.begin(h.context,selected))return finish(false);
            state->gathering=1;
            const auto active=hologram_read<std::uint32_t>(npc,0xc18);
            const bool ok=active&&h.gather(h.context,active);
            state->gathering=0;
            if(!ok)return finish(false);
        }
        if(prior&&(!h.end(h.context,prior)||!h.reset(h.context,npc)))return finish(false);
        if(!h.changed(h.context,npc,prior,selected))return finish(false);
    }
    const auto active=hologram_read<std::uint32_t>(npc,0xc18);
    hologram_write(npc,0xc30,static_cast<unsigned char>(1));
    if(!active)return finish(h.base_schedule(h.context,npc,result)!=0);
    int raw=0;if(!h.schedule(h.context,active,&raw))return finish(false);
    unsigned local=0;
    // Embedded vslot +d4 is the range predicate at 220ce130.
    if(active==embedded)local=raw>=100000&&raw<110000;
    else if(!h.is_local(h.context,active,raw,&local))return finish(false);
    if(local)return finish(h.translate(h.context,active,raw,result)!=0);
    *result=raw;return finish(true);
}
int __cdecl count_active_observe(HologramControl* c,const CountSchedulerBindings* b,const CountBindings* cb,const void* obs,unsigned bytes) noexcept {
    if(!health_control(c,count_size)||!b||!obs)return 0;
    const auto active=hologram_read<std::uint32_t>(c->object,0xc18);
    if(!active)return 1;
    if(active==count_embedded(c->object))return count_observe(c,cb,obs,bytes);
    if(!b->observe_other)return 0;
    c->busy=1;const auto h=*b;
    return disp_finish(c,h.observe_other(h.context,active,obs,bytes)!=0);
}
const CountSchedulerApi count_scheduler_api{version,sizeof(CountSchedulerApi),&count_register_interrogation,&count_select,&count_active_observe};

int __cdecl action_capture_use(HologramControl* c,const UseObservationBindings* b,int descriptor,void* subject) noexcept {
    if(!health_control(c,0x5e9)||!b||!b->monitor.resolve||!b->monitor.clock||!b->monitor.packed_handle||
       !b->monitor.sample||!b->event)return 0;
    c->busy=1;const auto h=*b;const auto& m=h.monitor;auto* obj=static_cast<unsigned char*>(c->object);
    auto finish=[&](bool ok){return disp_finish(c,ok);};
    std::uint32_t handle=0xffffffffu;
    if(subject&&!m.packed_handle(m.context,subject,&handle))return finish(false);
    hologram_write(obj,0x304,handle);
    float now=0;if(!m.clock(m.context,&now))return finish(false);
    hologram_write(obj,0x5c4,now);
    auto resolve_owner=[&](void*& out){
        out=nullptr;const auto owner_handle=hologram_read<std::uint32_t>(obj,4);
        return owner_handle==0xffffffffu||m.resolve(m.context,owner_handle,&out)!=0;
    };
    void* owner=nullptr;
    if(!resolve_owner(owner))return finish(false);
    ActionEntitySample owner_sample{},subject_sample{};
    if(!action_snapshot(m,owner,owner_sample)||!action_snapshot(m,subject,subject_sample))return finish(false);
    hologram_write(obj,0x5c0,descriptor);
    // 2215c480 copies subject before owner and leaves subobject intact.
    std::memcpy(obj+0x468,&subject_sample,sizeof(subject_sample));
    std::memcpy(obj+0x310,&owner_sample,sizeof(owner_sample));
    if(!hologram_read<int>(obj,0x5c0))return finish(true);
    if(!resolve_owner(owner))return finish(false);
    const auto* part=reinterpret_cast<const char*>(obj+0x5c8);
    if(!std::memchr(part,0,32))return finish(false);
    return finish(h.event(m.context,hologram_read<int>(obj,0x5c0),owner,subject,part)!=0);
}
int __cdecl npc_observation_tick(void* npc,const NpcObservationBindings* b) noexcept {
    if(!npc||!b||!b->max_clients||!b->single_player||!b->observe)return 0;
    const auto h=*b;int clients=0;
    if(!h.max_clients(h.context,&clients))return -1;
    // Original outer NPC gate accepts <=1; helper returns no player
    // unless exactly one. Do not invoke the host lookup for zero.
    if(clients!=1)return 1;
    void* player=nullptr;unsigned bytes=0;
    if(!h.single_player(h.context,&player,&bytes))return -1;
    if(!player)return 1;
    constexpr unsigned offset=0xc40,extent=0x5e9;
    if(bytes<offset+extent)return -1;
    const auto* monitor=static_cast<const unsigned char*>(player)+offset;
    if(!sin_independent::vgui_layout::Readable(monitor,extent))return -1;
    if(!monitor[0x300]&&!monitor[0x5e8])return 1;
    return h.observe(h.context,npc,monitor,extent)?1:-1;
}
const UseObservationApi use_observation_api{version,sizeof(UseObservationApi),&action_capture_use,&npc_observation_tick,&health_ack};

const char* __cdecl vehicle_part_name(int part) noexcept {
    constexpr const char* names[]={"Door","AirConditioning","Radio","GloveBox","RightFrontWindow","RearLeftWindow","RearRightWindow","DomeLight","GPS","EmergencyButton","Wipers","GearShift","Sunroof","RearViewMirror","Trunk","Hood"};
    return part>=0&&part<16?names[part]:nullptr;
}
bool vehicle_use_valid(const HologramControl* c,const VehicleUseBindings* b,void* actor,unsigned bytes) noexcept {
    return health_control(c,0x74c)&&b&&b->pick_part&&b->resolve&&b->participant&&b->outside_seat&&actor&&bytes>=0x9b5&&sin_independent::vgui_layout::Readable(actor,0x9b5);
}
bool vehicle_admission(void* vehicle,const VehicleUseBindings& h,void* actor,unsigned& allowed) noexcept {
    allowed=0;
    if(!hologram_read<unsigned char>(vehicle,0x74a))return true;
    int part=-1;
    if(!h.pick_part(h.context,vehicle,actor,&part))return false;
    if(part<0)return true;
    const auto handle=hologram_read<std::uint32_t>(actor,0xe8);
    void* current=nullptr;
    if(handle!=0xffffffffu&&!h.resolve(h.context,handle,&current))return false;
    unsigned player=0;
    if(!h.participant(h.context,actor,&player))return false;
    // The original assumes the participant cast succeeds.
    if(!player)return true;
    if(current==vehicle){
        if(hologram_read<unsigned char>(actor,0x9b4)||part>=16)return true;
        allowed=part!=8&&part!=14&&part!=15;return true;
    }
    if(!(hologram_read<unsigned char>(actor,0xbc)&1)||part>=16)return true;
    if(part==0){
        int seat=-1;
        if(!h.outside_seat(h.context,vehicle,actor,&seat))return false;
        allowed=seat>=0;
    }else allowed=part==4||part==14||part==15;
    return true;
}
int __cdecl vehicle_can_use(HologramControl* c,const VehicleUseBindings* b,void* actor,unsigned bytes,unsigned* allowed) noexcept {
    if(!allowed||!vehicle_use_valid(c,b,actor,bytes))return 0;
    c->busy=1;const auto h=*b;
    return disp_finish(c,vehicle_admission(c->object,h,actor,*allowed));
}
int __cdecl vehicle_use(HologramControl* c,const VehicleUseBindings* b,void* actor,unsigned bytes,int type,void* record,unsigned record_bytes,unsigned* executed) noexcept {
    if(!executed||!vehicle_use_valid(c,b,actor,bytes)||!b->apply_part||!hologram_writable(actor,0x9b5)||
       (record&&(record_bytes<0x2e0||!hologram_writable(record,0x2e0))))return 0;
    c->busy=1;const auto h=*b;*executed=0;
    unsigned allowed=0;
    if(!vehicle_admission(c->object,h,actor,allowed))return disp_finish(c,false);
    if(!allowed)return disp_finish(c,true);
    unsigned player=0;
    if(!h.participant(h.context,actor,&player))return disp_finish(c,false);
    if(!player)return disp_finish(c,true);
    hologram_write(actor,0x93c,hologram_read<std::uint32_t>(actor,0x93c)&~0x20u);
    if(type!=3)return disp_finish(c,true);
    int part=-1;
    if(!h.pick_part(h.context,c->object,actor,&part))return disp_finish(c,false);
    if(part<0)return disp_finish(c,true);
    const char* name=vehicle_part_name(part);
    if(!name)return disp_finish(c,false); // original table assumes valid index
    if(record){
        auto* dest=static_cast<char*>(record)+0x2c0;
        std::memset(dest,0,32);std::memcpy(dest,name,std::strlen(name));
    }
    if(!h.apply_part(h.context,c->object,actor,part))return disp_finish(c,false);
    *executed=1;return disp_finish(c,true);
}
const VehicleUseApi vehicle_use_api{version,sizeof(VehicleUseApi),&vehicle_part_name,&vehicle_can_use,&vehicle_use,&health_ack};

constexpr unsigned vehicle_action_extent=0x834;
bool vehicle_action_valid(void* v,unsigned bytes,const VehicleActionBindings* b,void* actor,unsigned actor_bytes) noexcept {
    return bytes>=vehicle_action_extent&&hologram_writable(v,vehicle_action_extent)&&
        actor&&actor_bytes>=0xec&&sin_independent::vgui_layout::Readable(actor,0xec)&&
        b&&b->output&&b->resolve&&b->participant&&b->occupied_seat&&b->candidate_seat&&
        b->seat_type&&b->passenger_enter&&b->passenger_exit&&b->sound&&b->sequence&&b->radio_next&&b->unimplemented;
}
bool vehicle_toggle_record(void* v,const VehicleActionBindings& h,bool window,int index) noexcept {
    const unsigned vector_off=window?0x824:0x810,count_off=window?0x830:0x81c;
    if(index<0||index>=hologram_read<int>(v,count_off))return true;
    auto record=[&]() -> unsigned char* {
        const auto base=hologram_read<std::uint32_t>(v,vector_off);
        const auto offset=static_cast<unsigned>(index)*76u+0x44u;
        if(!base||base>0xffffffffu-offset)return nullptr;
        auto* p=reinterpret_cast<unsigned char*>(static_cast<std::uintptr_t>(base)+offset);
        // Byte field at aligned +44 of a 76-byte record.
        return hologram_writable(p,1)?p:nullptr;
    };
    auto* flag=record();if(!flag)return false;
    if(!h.sound(h.context,v,window?"window":"door",*flag?"close":"open",index))return false;
    flag=record();if(!flag)return false; // original reloads vector and byte
    *flag=*flag?0:1;return true;
}
bool vehicle_action_leaf(void* v,const VehicleActionBindings& h,void* actor,int part) noexcept {
    if(!h.output(h.context,v,actor,0x7cc,part))return false;
    auto resolve=[&](unsigned off,void* obj,void*& out){
        out=nullptr;const auto handle=hologram_read<std::uint32_t>(obj,off);
        return handle==0xffffffffu||h.resolve(h.context,handle,&out)!=0;
    };
    switch(part){
        case 4:{
            void* current=nullptr;
            if(!resolve(0xe8,actor,current))return false;
            if(current==v)return vehicle_toggle_record(v,h,true,1);
            // Otherwise falls through to passenger entry/exit.
        } [[fallthrough]];
        case 0:{
            unsigned player=0;if(!h.participant(h.context,actor,&player))return false;
            if(!player)return true;
            int seat=-1;if(!h.occupied_seat(h.context,v,actor,&seat))return false;
            if(seat>=0)return h.passenger_exit(h.context,v,actor,0)!=0;
            if(!h.candidate_seat(h.context,v,actor,&seat))return false;
            if(seat<0)return true;
            int type=0;if(!h.seat_type(h.context,v,seat,&type))return false;
            if(type==0&&hologram_read<unsigned char>(v,0x751))return true;
            return h.passenger_enter(h.context,v,actor,-1)!=0;
        }
        case 1:case 7:case 9:return h.unimplemented(h.context,part)!=0;
        case 2:{
            void* radio=nullptr;
            if(!resolve(0x768,v,radio))return false;
            if(!radio)return true;
            // Original existence check and separate resolved fetch.
            if(!resolve(0x768,v,radio)||!radio)return false;
            return h.radio_next(h.context,radio)!=0;
        }
        case 3:return vehicle_toggle_record(v,h,false,4);
        case 5:return vehicle_toggle_record(v,h,true,3);
        case 6:return vehicle_toggle_record(v,h,true,2);
        case 8:{
            const auto* action=hologram_read<unsigned char>(v,0x758)?"close":"open";
            if(!h.sound(h.context,v,"monitor",action,0))return false;
            hologram_write(v,0x758,static_cast<unsigned char>(!hologram_read<unsigned char>(v,0x758)));
            return true;
        }
        case 10:hologram_write(v,0x74e,static_cast<unsigned char>(!hologram_read<unsigned char>(v,0x74e)));return true;
        case 11:return h.sound(h.context,v,"gears","grind",0)&&h.sequence(h.context,v,"gear_shift");
        case 12:return vehicle_toggle_record(v,h,true,4);
        case 13:return h.sound(h.context,v,"mirror","adjust",0)&&h.sequence(h.context,v,"rear_view_mirror");
        case 14:return vehicle_toggle_record(v,h,false,2);
        case 15:return vehicle_toggle_record(v,h,false,3);
        default:return true;
    }
}
int __cdecl vehicle_apply_leaf(void* v,unsigned bytes,const VehicleActionBindings* b,void* actor,unsigned actor_bytes,int part) noexcept {
    if(!vehicle_action_valid(v,bytes,b,actor,actor_bytes))return 0;
    const auto h=*b;return vehicle_action_leaf(v,h,actor,part)?1:-1;
}
int __cdecl vehicle_apply_action(HologramControl* c,const VehicleActionBindings* b,void* actor,unsigned actor_bytes,int part) noexcept {
    if(!health_control(c,vehicle_action_extent)||!vehicle_action_valid(c->object,c->bytes,b,actor,actor_bytes))return 0;
    c->busy=1;const auto h=*b;
    return disp_finish(c,vehicle_action_leaf(c->object,h,actor,part));
}
const VehicleActionsApi vehicle_actions_api{version,sizeof(VehicleActionsApi),&vehicle_apply_action,&vehicle_apply_leaf,&health_ack};

bool music_radio_ready(const HologramControl* c,const MusicRadioBindings* b) noexcept {
    return health_control(c,music_radio_bytes)&&b&&b->clock&&b->schedule&&b->resolve&&b->handle&&b->emit&&b->stop;
}
int __cdecl music_radio_init(HologramControl* c,std::uint32_t script) noexcept {
    if(!health_control(c,music_radio_bytes))return 0;
    hologram_write(c->object,0x394,0xffffffffu);
    for(unsigned off:{0x398u,0x3a0u,0x3a4u,0x3a8u,0x3acu,0x3b0u,0x3b4u,0x3b8u})hologram_write(c->object,off,0u);
    hologram_write(c->object,0x39c,script);return 1;
}
int __cdecl music_radio_ack(HologramControl* c) noexcept {
    if(!c||c->busy||!c->blocked||c->bytes<music_radio_bytes||!hologram_writable(c->object,music_radio_bytes))return 0;
    c->blocked=0;return 1;
}
// Original station vector +3a8, count +3b4, stride56; per-station
// selected track +20, track vector +24, count +30, stride68.
unsigned char* music_record(std::uint32_t base,int index,unsigned stride,unsigned extent) noexcept {
    if(!base||index<0||index>65535)return nullptr;
    const auto offset=static_cast<unsigned>(index)*stride;
    if(base>0xffffffffu-offset)return nullptr;
    auto* p=reinterpret_cast<unsigned char*>(static_cast<std::uintptr_t>(base)+offset);
    return hologram_writable(p,extent)?p:nullptr;
}
bool music_speaker(void* radio,const MusicRadioBindings& h,void*& speaker) noexcept {
    auto resolve=[&](){
        speaker=nullptr;const auto word=hologram_read<std::uint32_t>(radio,0x394);
        return word==0xffffffffu||h.resolve(h.context,word,&speaker)!=0;
    };
    if(!resolve())return false;
    if(!speaker){
        std::uint32_t word=0xffffffffu;
        if(!h.handle(h.context,radio,&word))return false;
        hologram_write(radio,0x394,word);
    }
    return resolve()&&speaker;
}
bool music_play(void* radio,const MusicRadioBindings& h,bool advance,bool volume_change,bool stop) noexcept {
    const int station=hologram_read<int>(radio,0x3a4),count=hologram_read<int>(radio,0x3b4);
    if(station<0||station>=count)return true;
    auto* s=music_record(hologram_read<std::uint32_t>(radio,0x3a8),station,56,56);
    if(!s)return false;
    int track=hologram_read<int>(s,0x20),tracks=hologram_read<int>(s,0x30);
    if(advance){
        const auto next=static_cast<std::int64_t>(track)+1;
        track=next>=tracks?0:next<0?tracks-1:static_cast<int>(next);
        hologram_write(s,0x20,track);
    }
    if(track<0||track>=tracks)return true;
    auto* t=music_record(hologram_read<std::uint32_t>(s,0x24),track,68,68);
    if(!t)return false;
    char name[64]{};
    if(!prop_key_read(reinterpret_cast<const char*>(t+4),name))return false;
    void* speaker=nullptr;if(!music_speaker(radio,h,speaker))return false;
    if(stop)return h.stop(h.context,speaker,5,name)!=0;
    const double scaled=static_cast<double>(hologram_read<float>(radio,0x398))*0.01f;
    const float volume=scaled>1?1.f:scaled<0?0.f:static_cast<float>(scaled);
    float duration=0;
    if(!h.emit(h.context,radio,speaker,name,volume,5,60,volume_change?1u:0u,&duration))return false;
    if(advance){
        float now=0;if(!h.clock(h.context,&now))return false;
        hologram_write(radio,0x3a0,static_cast<float>(static_cast<double>(now)+duration));
    }
    return true;
}
bool music_on(void* radio,const MusicRadioBindings& h) noexcept {
    if(hologram_read<float>(radio,0x398)!=0.0f)return true;
    hologram_write(radio,0x398,100.0f);
    float now=0;if(!h.clock(h.context,&now)||!h.schedule(h.context,radio,now))return false;
    return music_play(radio,h,true,false,false);
}
bool music_off(void* radio,const MusicRadioBindings& h) noexcept {
    if(!(hologram_read<float>(radio,0x398)>0.0f))return true;
    hologram_write(radio,0x398,0.0f);
    return music_play(radio,h,false,false,true);
}
bool music_station(void* radio,const MusicRadioBindings& h,int direction,int requested) noexcept {
    const int count=hologram_read<int>(radio,0x3b4);
    if(count<0||count>65536)return false;
    const int last=count-1;int station=0;
    if(direction==0)station=requested>last?last:requested<0?0:requested;
    else{
        const auto next=static_cast<std::int64_t>(hologram_read<int>(radio,0x3a4))+direction;
        station=next>last?0:next<0?last:static_cast<int>(next);
    }
    hologram_write(radio,0x3a4,station);return music_play(radio,h,true,false,false);
}
int __cdecl music_radio_input(HologramControl* c,const MusicRadioBindings* b,MusicRadioInput input,int value) noexcept {
    if(!music_radio_ready(c,b)||static_cast<unsigned>(input)>static_cast<unsigned>(MusicRadioInput::Cycle))return 0;
    c->busy=1;const auto h=*b;auto* radio=c->object;bool ok=false;
    switch(input){
        case MusicRadioInput::On:ok=music_on(radio,h);break;
        case MusicRadioInput::Off:ok=music_off(radio,h);break;
        case MusicRadioInput::Next:ok=music_station(radio,h,1,0);break;
        case MusicRadioInput::Previous:ok=music_station(radio,h,-1,0);break;
        case MusicRadioInput::Station:ok=music_station(radio,h,0,value);break;
        case MusicRadioInput::Volume:hologram_write(radio,0x398,static_cast<float>(value));ok=music_play(radio,h,false,true,false);break;
        case MusicRadioInput::Cycle:
            if(hologram_read<float>(radio,0x398)>0.0f){
                const auto last=static_cast<std::int64_t>(hologram_read<int>(radio,0x3b4))-1;
                ok=hologram_read<int>(radio,0x3a4)<last?music_station(radio,h,1,0):music_off(radio,h);
            }else{hologram_write(radio,0x3a4,0);ok=music_on(radio,h);}
            break;
    }
    return disp_finish(c,ok);
}
int __cdecl music_radio_tick(HologramControl* c,const MusicRadioBindings* b) noexcept {
    if(!music_radio_ready(c,b))return 0;
    c->busy=1;const auto h=*b;auto* radio=c->object;
    const int station=hologram_read<int>(radio,0x3a4);
    float now=0;
    if(station>=0&&station<hologram_read<int>(radio,0x3b4)){
        if(!h.clock(h.context,&now))return disp_finish(c,false);
        if(now>=hologram_read<float>(radio,0x3a0)&&!music_play(radio,h,true,false,false))return disp_finish(c,false);
    }
    if(hologram_read<float>(radio,0x398)>0.f){
        if(!h.clock(h.context,&now)||!h.schedule(h.context,radio,now))return disp_finish(c,false);
    }
    return disp_finish(c,true);
}
const MusicRadioApi music_radio_api{version,sizeof(MusicRadioApi),&music_radio_init,&music_radio_input,&music_radio_tick,&music_radio_ack};

constexpr MusicRadioField music_fields[]={
    {"m_hSpeaker",MusicFieldKind::Saved,0xd,0x394},
    {"Volume",MusicFieldKind::Key,1,0x398},
    {"ScriptFile",MusicFieldKind::Key,2,0x39c},
    {"m_fSoundEnd",MusicFieldKind::Saved,0x10,0x3a0},
    {"m_nStation",MusicFieldKind::Saved,5,0x3a4},
    {"RadioThink",MusicFieldKind::Think,0,0},
    {"TurnOn",MusicFieldKind::Input,0,0},
    {"TurnOff",MusicFieldKind::Input,0,0},
    {"NextStation",MusicFieldKind::Input,0,0},
    {"PrevStation",MusicFieldKind::Input,0,0},
    {"SetStation",MusicFieldKind::Input,5,0},
    {"SetVolume",MusicFieldKind::Input,5,0}
};
const MusicRadioSchema music_schema{"env_radio",music_radio_bytes,12,music_fields};
const MusicRadioSchema* __cdecl music_map_schema() noexcept{return &music_schema;}
int __cdecl music_map_key(HologramControl* c,const char* name,unsigned type,std::uint32_t word) noexcept {
    if(!health_control(c,music_radio_bytes))return 0;
    char key[64]{};
    if(!prop_key_read(name,key))return 0;
    for(const auto& field:music_fields)if(field.kind==MusicFieldKind::Key&&!std::strcmp(key,field.name)){
        if(field.type!=type)return 0;
        hologram_write(c->object,field.offset,word);return 1;
    }
    return 0;
}
bool music_script_ready(const HologramControl* c,const MusicRadioMapBindings* b) noexcept {
    return health_control(c,music_radio_bytes)&&b&&b->open_script&&b->entry&&b->close_script&&
        b->append_station&&b->append_track&&b->precache;
}
unsigned char* music_station_record(void* radio,int index) noexcept {
    return music_record(hologram_read<std::uint32_t>(radio,0x3a8),index,56,56);
}
unsigned char music_lower(unsigned char c) noexcept {
    return c>='A'&&c<='Z'?static_cast<unsigned char>(c+('a'-'A')):c;
}
bool music_equal(const char* a,const char* b) noexcept {
    for(;;++a,++b){
        if(music_lower(static_cast<unsigned char>(*a))!=music_lower(static_cast<unsigned char>(*b)))return false;
        if(!*a)return true;
    }
}
bool music_load_entry(void* radio,const MusicRadioMapBindings& h,const char* station_name,const char* sound_name) noexcept {
    const auto context=h.playback.context;
    char name[1024]{},sound[1024]{};
    if(!prop_key_read(station_name,name)||!prop_key_read(sound_name,sound))return false;
    int count=hologram_read<int>(radio,0x3b4);
    if(count<0||count>=65536)return false;
    int station=0;
    for(;station<count;++station){
        auto* s=music_station_record(radio,station);
        if(!s||!std::memchr(s,0,32))return false;
        if(music_equal(reinterpret_cast<const char*>(s),name))break;
    }
    if(station==count){
        if(!h.append_station(context,radio)||hologram_read<int>(radio,0x3b4)!=count+1)return false;
        auto* s=music_station_record(radio,station);
        if(!s)return false;
        std::memset(s,0,32);
        const auto length=(std::min)(std::strlen(name),static_cast<std::size_t>(31));
        std::memcpy(s,name,length);
        hologram_write(s,0x20,-1);
        // Constructor 222e2b50 clears nested vector, not just its count.
        for(unsigned off:{0x24u,0x28u,0x2cu,0x30u,0x34u})hologram_write(s,off,0u);
    }
    MusicRadioTrack track{};track.station=station;
    const auto length=(std::min)(std::strlen(sound),static_cast<std::size_t>(63));
    std::memcpy(track.sound,sound,length);
    if(!h.precache(context,track.sound))return false;
    auto* s=music_station_record(radio,station);
    if(!s)return false;
    const int before=hologram_read<int>(s,0x30);
    if(before<0||before>=65536||!h.append_track(context,radio,station,&track))return false;
    s=music_station_record(radio,station);
    if(!s||hologram_read<int>(s,0x30)!=before+1)return false;
    auto* t=music_record(hologram_read<std::uint32_t>(s,0x24),before,68,68);
    return t&&hologram_read<int>(t,0)==station&&std::memchr(t+4,0,64)&&!std::strcmp(reinterpret_cast<const char*>(t+4),track.sound);
}
bool music_load_script(void* radio,const MusicRadioMapBindings& h) noexcept {
    const auto context=h.playback.context;
    void* token=nullptr;
    if(!h.open_script(context,hologram_read<std::uint32_t>(radio,0x39c),&token))return false;
    if(!token)return true;
    bool ok=false;
    // Bound the host-controlled enumeration; preserve partial mutation
    // on failure, close the token exactly once.
    for(unsigned i=0;i<=65536;++i){
        unsigned present=0;const char* station=nullptr;const char* sound=nullptr;
        if(!h.entry(context,token,i,&present,&station,&sound))break;
        if(!present){ok=true;break;}
        if(i==65536||!music_load_entry(radio,h,station,sound))break;
    }
    const bool closed=h.close_script(context,token)!=0;
    return ok&&closed;
}
int __cdecl music_map_precache(HologramControl* c,const MusicRadioMapBindings* b) noexcept {
    if(!music_script_ready(c,b))return 0;
    c->busy=1;const auto h=*b;
    return disp_finish(c,music_load_script(c->object,h));
}
int music_map_start(HologramControl* c,const MusicRadioMapBindings* b,bool activate) noexcept {
    if(!music_script_ready(c,b)||!b->base_spawn||!b->base_activate||!b->install_think||
       !b->playback.clock||!b->playback.schedule)return 0;
    c->busy=1;const auto h=*b;const auto context=h.playback.context;auto* radio=c->object;
    if(!(activate?h.base_activate(context,radio):h.base_spawn(context,radio)))return disp_finish(c,false);
    if(!music_load_script(radio,h)||!h.install_think(context,radio,"RadioThink"))return disp_finish(c,false);
    if(hologram_read<float>(radio,0x398)>0){
        float now=0;if(!h.playback.clock(context,&now)||!h.playback.schedule(context,radio,static_cast<float>(static_cast<double>(now)+0.2f)))return disp_finish(c,false);
        if(activate)hologram_write(radio,0x3a0,0.0f);
    }
    return disp_finish(c,true);
}
int __cdecl music_map_spawn(HologramControl* c,const MusicRadioMapBindings* b) noexcept{return music_map_start(c,b,false);}
int __cdecl music_map_activate(HologramControl* c,const MusicRadioMapBindings* b) noexcept{return music_map_start(c,b,true);}
int __cdecl music_map_input(HologramControl* c,const MusicRadioMapBindings* b,const char* name,unsigned type,std::uint32_t word) noexcept {
    if(!b)return 0;
    char key[64]{};if(!prop_key_read(name,key))return 0;
    constexpr MusicRadioInput ops[]={MusicRadioInput::On,MusicRadioInput::Off,MusicRadioInput::Next,MusicRadioInput::Previous,MusicRadioInput::Station,MusicRadioInput::Volume};
    for(unsigned i=0;i<6;++i)if(!std::strcmp(key,music_fields[i+6].name)){
        // Original integer input handlers default to zero on wrong tag.
        int value=0;if(type==5)std::memcpy(&value,&word,4);
        return music_radio_input(c,&b->playback,ops[i],value);
    }
    return 0;
}
const MusicRadioMapApi music_map_api{version,sizeof(MusicRadioMapApi),&music_map_schema,&music_map_key,&music_map_precache,&music_map_spawn,&music_map_activate,&music_map_input};

constexpr unsigned vehicle_sound_extent=0x848;
const unsigned char* vehicle_sound_record(const void* vehicle,int index) noexcept {
    const auto base=hologram_read<std::uint32_t>(vehicle,0x838);
    if(!base||index<0||index>=65536||base>0xffffffffu-static_cast<unsigned>(index)*128u)return nullptr;
    auto* record=reinterpret_cast<const unsigned char*>(static_cast<std::uintptr_t>(base)+static_cast<unsigned>(index)*128u);
    return sin_independent::vgui_layout::Readable(record,128)?record:nullptr;
}
int __cdecl vehicle_sound_find(const void* vehicle,unsigned bytes,const char* group,const char* action,int index,int* found) noexcept {
    if(!vehicle||bytes<vehicle_sound_extent||!sin_independent::vgui_layout::Readable(vehicle,vehicle_sound_extent)||!found)return 0;
    char g[1024]{},a[1024]{},key[128]{};
    if(!prop_key_read(group,g)||!prop_key_read(action,a))return 0;
    std::snprintf(key,sizeof(key),"%s:%s:%d",g,a,index);
    *found=-1;
    const int count=hologram_read<int>(vehicle,0x844);
    if(count>65536)return -1;
    for(int i=0;i<count;++i){
        const auto* record=vehicle_sound_record(vehicle,i);
        if(!record||!std::memchr(record,0,64))return -1;
        if(music_equal(reinterpret_cast<const char*>(record),key)){*found=i;break;}
    }
    return 1;
}
int __cdecl vehicle_sound_play(void* vehicle,unsigned bytes,const VehicleSoundBindings* b,const char* group,const char* action,int index) noexcept {
    if(!vehicle||bytes<vehicle_sound_extent||!sin_independent::vgui_layout::Readable(vehicle,vehicle_sound_extent)||!b||!b->controller_scalar||!b->emit)return 0;
    const auto h=*b;float scalar=0;
    if(!h.controller_scalar(h.context,vehicle,&scalar))return -1;
    // Original x87 branch accepts <=0.5 AND unordered.
    if(scalar>0.5f)return 1;
    int found=-1;
    const auto status=vehicle_sound_find(vehicle,bytes,group,action,index,&found);
    if(status!=1||found<0)return status;
    const auto* record=vehicle_sound_record(vehicle,found);
    if(!record||!std::memchr(record+64,0,64))return -1;
    return h.emit(h.context,vehicle,reinterpret_cast<const char*>(record+64),0,0)?1:-1;
}
int __cdecl vehicle_sequence_play(void* vehicle,unsigned bytes,const VehicleSoundBindings* b,const char* name) noexcept {
    if(!vehicle||!bytes||!b||!b->lookup_sequence||!b->set_sequence||!b->reset_sequence)return 0;
    char key[128]{};
    if(!prop_key_read(name,key))return 0;
    const auto h=*b;int index=-1;
    if(!h.lookup_sequence(h.context,vehicle,key,&index))return -1;
    if(index<0)return 1;
    return h.set_sequence(h.context,vehicle,index)&&h.reset_sequence(h.context,vehicle)?1:-1;
}
const VehicleSoundApi vehicle_sound_api{version,sizeof(VehicleSoundApi),&vehicle_sound_find,&vehicle_sound_play,&vehicle_sequence_play};

int __cdecl vehicle_hint_enable(HologramControl* c,unsigned type,std::uint32_t word) noexcept {
    if(!health_control(c,0x765))return 0;
    hologram_write(c->object,0x764,static_cast<unsigned char>(type==5&&word!=0));return 1;
}
int __cdecl hint_encode(const char* text,const float* duration,unsigned char* out,unsigned capacity,unsigned* used) noexcept {
    if(!out||!used)return 0;*used=0;
    char token[2048]{};
    if(!prop_key_read(text,token))return 0;
    const auto length=static_cast<unsigned>(std::strlen(token))+1;
    const unsigned total=1+length+(duration?4:0);
    if(capacity<total)return 0;
    out[0]=1;std::memcpy(out+1,token,length);
    if(duration)std::memcpy(out+1+length,duration,4);
    *used=total;return 1;
}
int __cdecl vehicle_hint_notify(void* v,unsigned bytes,const VehicleHintBindings* b,int seat,unsigned entering) noexcept {
    if(!v||bytes<0x765||!sin_independent::vgui_layout::Readable(v,0x765)||!b||
       !b->resolve||!b->participant||!b->send||seat<0||entering>1)return 0;
    if(!hologram_read<unsigned char>(v,0x764))return 1;
    const auto off=0x6a4ull+static_cast<unsigned long long>(seat)*4;
    if(off+4>bytes||off+4>0x764)return 0;
    const auto h=*b;void* player=nullptr;
    const auto handle=hologram_read<std::uint32_t>(v,static_cast<unsigned>(off));
    if(handle!=0xffffffffu&&!h.resolve(h.context,handle,&player))return -1;
    if(!player)return 1;
    unsigned eligible=0;
    if(!h.participant(h.context,player,&eligible))return -1;
    if(!eligible)return 1;
    const char* token=!entering?"":seat==0?"#SE1_Hint_Driver":"#SE1_Hint_Vehicle";
    unsigned char payload[64]{};unsigned count=0;
    if(hint_encode(token,nullptr,payload,sizeof(payload),&count)!=1)return -1;
    return h.send(h.context,player,1,hint_message_name,payload,count)?1:-1;
}
int __cdecl hint_server_register(const VehicleHintBindings* b) noexcept {
    if(!b||!b->register_message)return 0;
    return b->register_message(b->context,hint_message_name,-1)?1:-1;
}
const VehicleHintApi vehicle_hint_api{version,sizeof(VehicleHintApi),&vehicle_hint_enable,&vehicle_hint_notify,&hint_encode,&hint_server_register};

constexpr unsigned view_vehicle_extent=0x80c,view_player_extent=0x9b5;
bool view_ready(const HologramControl* v,const VehicleViewBindings* b) noexcept {
    return health_control(v,view_vehicle_extent)&&b&&b->dirty_edict&&b->output&&b->attachment&&b->set_position;
}
const unsigned char* view_record(void* vehicle,int index) noexcept {
    const auto base=hologram_read<std::uint32_t>(vehicle,0x7fc);
    const auto count=hologram_read<int>(vehicle,0x808);
    if(!base||index<0||index>=count||index>=65536||base>0xffffffffu-static_cast<unsigned>(index)*100u)return nullptr;
    auto* p=reinterpret_cast<const unsigned char*>(static_cast<std::uintptr_t>(base)+static_cast<unsigned>(index)*100u);
    return sin_independent::vgui_layout::Readable(p,100)?p:nullptr;
}
bool view_player_set(void* player,const VehicleViewBindings& h,int index,unsigned char permission) noexcept {
    if(hologram_read<int>(player,0x9b0)!=index){
        if(hologram_read<unsigned char>(player,0x1f0))hologram_write(player,0x200,static_cast<unsigned char>(1));
        else{
            const auto edict=hologram_read<std::uint32_t>(player,0x16c);
            if(edict&&!h.dirty_edict(h.context,player,edict))return false;
        }
        hologram_write(player,0x9b0,index);
    }
    hologram_write(player,0x9b4,permission);
    return true;
}
int view_finish(HologramControl* v,HologramControl* p,bool ok) noexcept {
    if(p)p->busy=0;
    if(!ok&&p)p->blocked=1;
    return disp_finish(v,ok);
}
int __cdecl vehicle_set_view(HologramControl* v,const VehicleViewBindings* b,void* activator,unsigned type,int value) noexcept {
    if(!view_ready(v,b)||!b->cast_player)return 0;
    v->busy=1;const auto h=*b;
    HologramControl* p=nullptr;
    if(!h.cast_player(h.context,activator,&p))return disp_finish(v,false);
    if(!p)return disp_finish(v,true);
    if(type!=5)value=0;
    if(value<0||value>=hologram_read<int>(v->object,0x808))return disp_finish(v,true);
    if(p==v||!health_control(p,view_player_extent))return disp_finish(v,false);
    const auto* rec=view_record(v->object,value);
    if(!rec)return disp_finish(v,false);
    p->busy=1;
    if(!view_player_set(p->object,h,value,rec[0x55])||!h.output(h.context,v->object,p->object,0x7e4,value))return view_finish(v,p,false);
    // Original retains selected record across output. Engine must not
    // destroy or relocate that table during the synchronous callback.
    if(!sin_independent::vgui_layout::Readable(rec,100))return view_finish(v,p,false);
    float pos[3]{};unsigned found=0;
    if(!h.attachment(h.context,v->object,hologram_read<int>(rec,0x48),pos,&found))return view_finish(v,p,false);
    return view_finish(v,p,!found||h.set_position(h.context,p->object,pos)!=0);
}
bool vehicle_seat_default_leaf(void* v,void* player,const VehicleViewBindings& h,int seat) noexcept {
    const int count=hologram_read<int>(v,0x808);
    for(int i=0;i<count;++i){
        const auto* rec=view_record(v,i);
        if(!rec)return false;
        if(hologram_read<int>(rec,0x44)==seat&&hologram_read<int>(rec,0x40)<0)
            return view_player_set(player,h,i,rec[0x55]);
    }
    return true;
}
int __cdecl vehicle_seat_default(HologramControl* v,HologramControl* p,const VehicleViewBindings* b,int seat) noexcept {
    if(!view_ready(v,b)||p==v||!health_control(p,view_player_extent)||seat<0)return 0;
    v->busy=1;p->busy=1;const auto h=*b;
    return view_finish(v,p,vehicle_seat_default_leaf(v->object,p->object,h,seat));
}
int __cdecl vehicle_directional(HologramControl* v,HologramControl* p,const VehicleViewBindings* b,int candidate,const float pos[3]) noexcept {
    if(!view_ready(v,b)||p==v||!health_control(p,view_player_extent)||!pos||!sin_independent::vgui_layout::Readable(pos,12))return 0;
    v->busy=1;p->busy=1;const auto h=*b;
    if(candidate<0||candidate==hologram_read<int>(p->object,0x9b0))return view_finish(v,p,true);
    const auto* rec=view_record(v->object,candidate);
    if(!rec)return view_finish(v,p,false);
    return view_finish(v,p,view_player_set(p->object,h,candidate,rec[0x55])&&h.output(h.context,v->object,p->object,0x7e4,candidate)&&h.set_position(h.context,p->object,pos));
}
int __cdecl vehicle_return_view(HologramControl* v,HologramControl* p,const VehicleViewBindings* b) noexcept {
    if(!view_ready(v,b)||p==v||!health_control(p,view_player_extent))return 0;
    v->busy=1;p->busy=1;const auto h=*b;
    const int current=hologram_read<int>(p->object,0x9b0),count=hologram_read<int>(v->object,0x808);
    int index=current;const unsigned char* rec=nullptr;
    // Valid original chains terminate. Bound malformed cyclic chains.
    for(int traversed=0;index>=0;++traversed){
        if(traversed>=count||traversed>=65536)return view_finish(v,p,false);
        rec=view_record(v->object,index);if(!rec)return view_finish(v,p,false);
        if(!rec[0x54])break;
        index=hologram_read<int>(rec,0x40);
    }
    if(index<0||index==current)return view_finish(v,p,true);
    float pos[3]{};unsigned found=0;
    if(!h.attachment(h.context,v->object,hologram_read<int>(rec,0x48),pos,&found))return view_finish(v,p,false);
    if(!found)return view_finish(v,p,true);
    return view_finish(v,p,view_player_set(p->object,h,index,0)&&h.output(h.context,v->object,p->object,0x7e4,index)&&h.set_position(h.context,p->object,pos));
}
const VehicleViewsApi vehicle_views_api{version,sizeof(VehicleViewsApi),&vehicle_set_view,&vehicle_seat_default,&vehicle_directional,&vehicle_return_view};

unsigned char* config_view_record(void* v,int i) noexcept {
    auto* r=const_cast<unsigned char*>(view_record(v,i));
    return r&&hologram_writable(r,100)?r:nullptr;
}
int config_find_view(void* v,const char* name) noexcept {
    const int count=hologram_read<int>(v,0x808);
    if(count<0||count>65536)return -2;
    for(int i=0;i<count;++i){
        const auto* r=view_record(v,i);
        if(!r||!std::memchr(r,0,64))return -2;
        if(music_equal(reinterpret_cast<const char*>(r),name))return i;
    }
    return -1;
}
int config_root_id(const char* name) noexcept {
    char root[64]{};
    for(int seat=0;seat<3;++seat){
        std::snprintf(root,sizeof(root),"passenger%d_view",seat);
        if(music_equal(root,name))return seat;
    }
    return -1;
}
bool config_append_view(void* v,const VehicleConfigBindings& h,const char* name,int parent,int seat,unsigned char temporary,int root) noexcept {
    const int index=hologram_read<int>(v,0x808);
    if(index<0||index>=65536||!h.append_view(h.context,v)||hologram_read<int>(v,0x808)!=index+1)return false;
    auto* r=config_view_record(v,index);if(!r)return false;
    std::memset(r,0,64);
    std::memcpy(r,name,(std::min)(std::strlen(name),static_cast<std::size_t>(63)));
    hologram_write(r,0x40,parent);
    hologram_write(r,0x44,seat);
    int id=-1;
    if(!h.attachment_id(h.context,v,name,&id))return false;
    r=config_view_record(v,index);if(!r)return false;
    hologram_write(r,0x48,id);
    hologram_write(r,0x4c,root);
    hologram_write(r,0x50,0);
    r[0x54]=temporary;r[0x55]=0;
    // Preserve unknown padding +56/+57, just as original does.
    hologram_write(r,0x58,-1);hologram_write(r,0x5c,-1);
    hologram_write(r,0x60,1.f);
    return true;
}
bool config_link_block(void* v,const VehicleConfigBindings& h,const char* block,unsigned char temporary) noexcept {
    for(unsigned i=0;i<=65536;++i){
        VehicleConfigEntry e{};unsigned present=0;
        if(!h.entry(h.context,block,i,&present,&e))return false;
        if(!present)return true;
        char key[1024]{},name[1024]{};
        if(i==65536||!prop_key_read(e.name,key)||!prop_key_read(e.text,name))return false;
        const int parent=config_find_view(v,key);
        if(parent==-2)return false;
        if(parent<0)continue;
        const auto* r=view_record(v,parent);
        const int seat=hologram_read<int>(r,0x44);
        if(!config_append_view(v,h,name,parent,seat,temporary,config_root_id(name)))return false;
    }
    return false;
}
bool config_overrides(void* v,const VehicleConfigBindings& h,const char* block,unsigned offset) noexcept {
    for(unsigned i=0;i<=65536;++i){
        VehicleConfigEntry e{};unsigned present=0;
        if(!h.entry(h.context,block,i,&present,&e))return false;
        if(!present)return true;
        char name[1024]{};
        if(i==65536||!prop_key_read(e.name,name))return false;
        const int index=config_find_view(v,name);
        if(index==-2)return false;
        if(index<0)continue;
        auto* r=config_view_record(v,index);if(!r)return false;
        if(offset==0x60)hologram_write(r,offset,e.real);
        else if(offset==0x55)r[offset]=e.integer!=0;
        else hologram_write(r,offset,e.integer);
    }
    return false;
}
bool config_views_leaf(void* v,const VehicleConfigBindings& h) noexcept {
    for(int seat=0;seat<3;++seat){
        char name[64]{};
        std::snprintf(name,sizeof(name),"passenger%d_view",seat);
        if(!config_append_view(v,h,name,-1,seat,0,-1))return false;
    }
    return config_link_block(v,h,"view_links",0)&&config_link_block(v,h,"view_temp",1)&&
        config_overrides(v,h,"view_damage_mod",0x60)&&config_overrides(v,h,"view_allow_weapons",0x55)&&config_overrides(v,h,"view_key",0x50);
}
bool config_damage_leaf(void* v,const VehicleConfigBindings& h) noexcept {
    unsigned seen=0;
    for(unsigned i=0;i<=65536;++i){
        VehicleConfigEntry e{};unsigned present=0;
        if(!h.entry(h.context,"damage_mod",i,&present,&e))return false;
        if(!present)return true;
        char name[1024]{};
        if(i==65536||!prop_key_read(e.name,name))return false;
        constexpr const char* keys[]={"blast","crush","bullet"};
        for(unsigned j=0;j<3;++j)if(!(seen&(1u<<j))&&music_equal(name,keys[j])){
            hologram_write(v,0x864+j*4,e.real);seen|=1u<<j;
        }
    }
    return false;
}
int __cdecl config_views(HologramControl* c,const VehicleConfigBindings* b) noexcept {
    if(!health_control(c,view_vehicle_extent)||!b||!b->entry||!b->append_view||!b->attachment_id)return 0;
    c->busy=1;const auto h=*b;
    return disp_finish(c,config_views_leaf(c->object,h));
}
int __cdecl config_damage(HologramControl* c,const VehicleConfigBindings* b) noexcept {
    if(!health_control(c,0x870)||!b||!b->entry)return 0;
    c->busy=1;const auto h=*b;
    return disp_finish(c,config_damage_leaf(c->object,h));
}
const VehicleConfigApi vehicle_config_api{version,sizeof(VehicleConfigApi),&config_views,&config_damage,&health_ack};

constexpr unsigned vehicle_passenger_extent=0x890;
int __cdecl passenger_defaults(HologramControl* c) noexcept {
    if(!health_control(c,vehicle_passenger_extent))return 0;
    for(unsigned off:{0x864u,0x868u,0x86cu})hologram_write(c->object,off,1.f);
    hologram_write(c->object,0x74c,static_cast<unsigned char>(1));
    hologram_write(c->object,0x88c,0.f);
    return 1;
}
int __cdecl passenger_damage_factor(HologramControl* c,const VehiclePassengerBindings* b,int seat,unsigned flags,float* out) noexcept {
    if(!health_control(c,vehicle_passenger_extent)||!b||!b->occupant||!b->global_damage_scale||!b->participant||!b->view_index||!out)return 0;
    c->busy=1;const auto h=*b;auto* v=c->object;
    void* occupant=nullptr;
    if(!h.occupant(h.context,v,seat,&occupant))return disp_finish(c,false);
    if(!occupant){*out=0;return disp_finish(c,true);}
    // Original x87 multiplies the category factors and global scale
    // before one float store, then conditionally multiplies by view.
    double factor=(flags&0x40u)?hologram_read<float>(v,0x864):1.;
    if(flags&1u)factor*=hologram_read<float>(v,0x868);
    if(flags&2u)factor*=hologram_read<float>(v,0x86c);
    float global=0;
    if(!h.global_damage_scale(h.context,&global))return disp_finish(c,false);
    const float scaled=static_cast<float>(factor*global);
    unsigned player=0;
    if(!h.participant(h.context,occupant,&player))return disp_finish(c,false);
    if(!player){*out=scaled;return disp_finish(c,true);}
    int view=-1;
    if(!h.view_index(h.context,occupant,&view))return disp_finish(c,false);
    const auto* r=view_record(v,view);
    if(!r)return disp_finish(c,false);
    *out=scaled*hologram_read<float>(r,0x60);
    return disp_finish(c,true);
}
int __cdecl passenger_enable_ammo(HologramControl* c,unsigned type,std::uint32_t word) noexcept {
    if(!health_control(c,vehicle_passenger_extent))return 0;
    hologram_write(c->object,0x74c,static_cast<unsigned char>(type==5&&word!=0));
    return 1;
}
bool passenger_refill_one(const VehiclePassengerBindings& h,void* combat,const char* name) noexcept {
    int index=-1;
    if(!h.ammo_index(h.context,name,&index))return false;
    if(index<0)return true;
    unsigned allowed=0;
    if(!h.ammo_allowed(h.context,combat,index,&allowed))return false;
    if(!allowed)return true;
    int count=0;
    if(!h.ammo_count(h.context,combat,index,&count))return false;
    if(count>0)return true;
    int max=0;
    if(!h.ammo_max(h.context,index,&max))return false;
    return h.give_ammo(h.context,combat,max,index,0)!=0;
}
bool passenger_refill_ready(const VehiclePassengerBindings* b) noexcept {
    return b&&b->clock&&b->occupant&&b->combat_character&&b->base_player_cast&&b->view_index&&
        b->ammo_index&&b->ammo_allowed&&b->ammo_count&&b->ammo_max&&b->give_ammo;
}
bool passenger_refill_leaf(void* v,const VehiclePassengerBindings& h) noexcept {
    if(!hologram_read<unsigned char>(v,0x74c))return true;
    float now=0;
    if(!h.clock(h.context,&now))return false;
    if(!(static_cast<double>(now)-hologram_read<float>(v,0x88c)>1.))return true;
    hologram_write(v,0x88c,now);
    for(int seat=0;seat<3;++seat){
        void* occupant=nullptr;void* combat=nullptr;
        if(!h.occupant(h.context,v,seat,&occupant))return false;
        if(!occupant)continue;
        if(!h.combat_character(h.context,occupant,&combat))return false;
        if(!combat)continue;
        unsigned player=0;
        if(!h.base_player_cast(h.context,combat,&player))return false;
        if(player){
            int view=-1;
            if(!h.view_index(h.context,combat,&view))return false;
            if(view!=1&&view!=3&&view!=4)continue;
        }
        constexpr const char* names[]={"MAG","AR","BUCK","ARNADE","FLAK","GRENADE"};
        for(const char* name:names)if(!passenger_refill_one(h,combat,name))return false;
    }
    return true;
}

int __cdecl passenger_refill(HologramControl* c,const VehiclePassengerBindings* b) noexcept {
    if(!health_control(c,vehicle_passenger_extent)||!passenger_refill_ready(b))return 0;
    c->busy=1;const auto h=*b;
    return disp_finish(c,passenger_refill_leaf(c->object,h));
}
const VehiclePassengersApi vehicle_passengers_api{version,sizeof(VehiclePassengersApi),&passenger_defaults,&passenger_damage_factor,&passenger_enable_ammo,&passenger_refill,&health_ack};

constexpr VehicleField vehicle_map_fields[]={
    {"EnterVehicle",VehicleFieldKind::Input,5,0,0x2232dd00u},
    {"ExitVehicle",VehicleFieldKind::Input,5,0,0x2232dd40u},
    {"EnableUse",VehicleFieldKind::Input,5,0,0x2232dd90u},
    {"SetView",VehicleFieldKind::Input,5,0,0x2232fa70u},
    {"LockView",VehicleFieldKind::Input,5,0,0x2232ddc0u},
    {"OpenDoor",VehicleFieldKind::Input,5,0,0x22330800u},
    {"OpenWindow",VehicleFieldKind::Input,5,0,0x22330870u},
    {"CloseDoor",VehicleFieldKind::Input,5,0,0x223308d0u},
    {"CloseWindow",VehicleFieldKind::Input,5,0,0x22330940u},
    {"OpenDoors",VehicleFieldKind::Input,0,0,0x223309a0u},
    {"CloseDoors",VehicleFieldKind::Input,0,0,0x223309b0u},
    {"OpenWindows",VehicleFieldKind::Input,0,0,0x223309c0u},
    {"CloseWindows",VehicleFieldKind::Input,0,0,0x223309d0u},
    {"HeadlightOn",VehicleFieldKind::Input,0,0,0x223309e0u},
    {"HeadlightOff",VehicleFieldKind::Input,0,0,0x22330a20u},
    {"WipersOn",VehicleFieldKind::Input,0,0,0x2232ddf0u},
    {"WipersOff",VehicleFieldKind::Input,0,0,0x2232de00u},
    {"MonitorOn",VehicleFieldKind::Input,0,0,0x22330a60u},
    {"MonitorOff",VehicleFieldKind::Input,0,0,0x22330a90u},
    {"SetCamera",VehicleFieldKind::Input,2,0,0x2232fb20u},
    {"EnableHints",VehicleFieldKind::Input,5,0,0x2232de10u},
    {"EnableAmmo",VehicleFieldKind::Input,5,0,0x2232de40u},
    {"RadioOn",VehicleFieldKind::Input,0,0,0x2232f500u},
    {"RadioOff",VehicleFieldKind::Input,0,0,0x2232f540u},
    {"SetRadioStation",VehicleFieldKind::Input,5,0,0x2232f580u},
    {"EnableUse",VehicleFieldKind::Key,6,0x74a,0},
    {"ViewsLocked",VehicleFieldKind::Key,6,0x74b,0},
    {"InfiniteAmmo",VehicleFieldKind::Key,6,0x74c,0},
    {"Invunerable",VehicleFieldKind::Key,6,0x74d,0},
    {"DriverDoorLocked",VehicleFieldKind::Key,6,0x751,0},
    {"DamageTolerance",VehicleFieldKind::Key,1,0x754,0},
    {"MonitorActive",VehicleFieldKind::Key,6,0x758,0},
    {"MonitorCamera",VehicleFieldKind::Key,2,0x75c,0},
    {"HintsEnabled",VehicleFieldKind::Key,6,0x764,0},
    {"DoorClosed",VehicleFieldKind::Output,0xb,0x76c,0},
    {"DoorOpen",VehicleFieldKind::Output,0xb,0x784,0},
    {"WindowClosed",VehicleFieldKind::Output,0xb,0x79c,0},
    {"WindowOpen",VehicleFieldKind::Output,0xb,0x7b4,0},
    {"UseBox",VehicleFieldKind::Output,0xb,0x7cc,0},
    {"ViewChanged",VehicleFieldKind::Output,0xb,0x7e4,0}
};
const VehicleMapSchema vehicle_map_schema{"prop_vehicle_sin",40,vehicle_map_fields};
const VehicleMapSchema* __cdecl vehicle_map_get_schema() noexcept {return &vehicle_map_schema;}
int __cdecl vehicle_map_key(HologramControl* c,const char* name,unsigned type,std::uint32_t word) noexcept {
    if(!health_control(c,vehicle_action_extent))return 0;
    char key[128]{};
    if(!prop_key_read(name,key))return 0;
    for(const auto& field:vehicle_map_fields){
        if(field.kind!=VehicleFieldKind::Key||std::strcmp(key,field.name))continue;
        if(type!=field.type)return 0;
        if(type==6)hologram_write(c->object,field.offset,static_cast<unsigned char>(word!=0));
        else hologram_write(c->object,field.offset,word);
        return 1;
    }
    return 0;
}
unsigned char* vehicle_map_record(void* v,bool window,int index) noexcept {
    const auto base=hologram_read<std::uint32_t>(v,window?0x824:0x810);
    if(index<0||index>=65536||!base||base>0xffffffffu-static_cast<unsigned>(index)*76u)return nullptr;
    auto* p=reinterpret_cast<unsigned char*>(static_cast<std::uintptr_t>(base)+static_cast<unsigned>(index)*76u);
    return hologram_writable(p,76)?p:nullptr;
}
bool vehicle_map_set_part(void* v,const VehicleMapBindings& h,bool window,int index,bool open) noexcept {
    const int count=hologram_read<int>(v,window?0x830:0x81c);
    if(index<0||index>=count)return true;
    auto* r=vehicle_map_record(v,window,index);if(!r)return false;
    if((r[0x44]!=0)!=open){
        if(!h.sound||!h.sound(h.context,v,window?"window":"door",open?"open":"close",index))return false;
        r=vehicle_map_record(v,window,index);if(!r)return false;
    }
    r[0x44]=open?1:0;
    if(!window)r[0x45]=0;
    return true;
}
bool vehicle_map_bulk(void* v,const VehicleMapBindings& h,bool window,bool open) noexcept {
    for(int i=0;i<hologram_read<int>(v,window?0x830:0x81c);++i){
        if(i>=65536||!vehicle_map_set_part(v,h,window,i,open))return false;
    }
    return true;
}
bool vehicle_map_unlink(void* v,const VehicleMapBindings& h) noexcept {
    auto resolve=[&](void*& camera){
        camera=nullptr;const auto word=hologram_read<std::uint32_t>(v,0x760);
        return word==0xffffffffu||(h.resolve&&h.resolve(h.context,word,&camera));
    };
    void* camera=nullptr;
    if(!resolve(camera))return false;
    if(!camera)return true; // original leaves stale handle unchanged
    if(!resolve(camera)||!camera||!h.unlink_camera||!h.unlink_camera(h.context,camera))return false;
    hologram_write(v,0x760,0xffffffffu);return true;
}
bool vehicle_map_headlight(void* v,const VehicleMapBindings& h,unsigned char value) noexcept {
    if(hologram_read<unsigned char>(v,0x748)==value)return true;
    if(hologram_read<unsigned char>(v,0x1f0))hologram_write(v,0x200,static_cast<unsigned char>(1));
    else{
        const auto edict=hologram_read<std::uint32_t>(v,0x16c);
        if(edict&&(!h.dirty_edict||!h.dirty_edict(h.context,v,edict)))return false;
    }
    hologram_write(v,0x748,value);return true;
}
bool vehicle_map_radio(void* v,const VehicleMapBindings& h,MusicRadioInput op,int value) noexcept {
    const auto handle=hologram_read<std::uint32_t>(v,0x768);
    void* radio=nullptr;
    if(handle==0xffffffffu||!h.resolve||!h.resolve(h.context,handle,&radio)||!radio)return false;
    HologramControl* rc=nullptr;
    if(!h.radio_control||!h.radio_control(h.context,radio,&rc)||!rc||rc->object!=radio)return false;
    return music_radio_input(rc,&h.radio,op,value)==1;
}
// HologramControl::busy is an unsigned guard, not a boolean ABI field.
 // 1 is ordinary ownership. Values below are one-shot host permits.
enum class VehiclePermit:unsigned {Enter=2,Exit=3,Parts=4};
template<class F> bool vehicle_callback(HologramControl* c,VehiclePermit permit,F invoke) noexcept {
    if(!c||c->busy!=1||c->blocked)return false;
    const unsigned value=static_cast<unsigned>(permit);
    c->busy=value;
    const bool ok=invoke();
    // Legacy engine callbacks may implement their own original path
    // without consuming the permit. The nested path consumes it once.
    const bool valid=c->busy==1||c->busy==value;
    c->busy=1;
    return ok&&valid&&!c->blocked;
}
bool vehicle_consume(HologramControl* c,VehiclePermit permit) noexcept {
    if(!c||c->busy!=static_cast<unsigned>(permit)||c->blocked)return false;
    c->busy=1;
    return true;
}
int vehicle_nested_finish(HologramControl* c,bool ok) noexcept {
    if(!ok)c->blocked=1;
    return ok?1:-1; // outer operation, not leaf, releases ownership
}
int __cdecl vehicle_map_input(HologramControl* c,const VehicleMapBindings* b,const char* name,unsigned type,std::uint32_t word,void* activator) noexcept {
    if(!health_control(c,vehicle_action_extent)||!b)return 0;
    char key[128]{};
    if(!prop_key_read(name,key))return 0;
    unsigned index=25;
    for(unsigned i=0;i<25;++i)if(!std::strcmp(vehicle_map_fields[i].name,key)){index=i;break;}
    if(index==25)return 0;
    int value=0;if(type==5)std::memcpy(&value,&word,4);
    const auto h=*b;
    // Delegate to the actual native functions before taking their locks.
    if(index==3)return vehicle_set_view(c,&h.views,activator,type,value);
    if(index==20)return vehicle_hint_enable(c,type,word);
    if(index==21)return passenger_enable_ammo(c,type,word);
    c->busy=1;auto* v=c->object;bool ok=true;
    switch(index){
        case 0:ok=h.enter&&vehicle_callback(c,VehiclePermit::Enter,[&](){return h.enter(h.context,v,activator,value)!=0;});break;
        case 1:{
            void* occupant=nullptr;
            ok=h.occupant&&h.occupant(h.context,v,value,&occupant);
            if(ok&&occupant)ok=h.exit&&vehicle_callback(c,VehiclePermit::Exit,[&](){return h.exit(h.context,v,occupant,1)!=0;});
            break;
        }
        case 2:hologram_write(v,0x74a,static_cast<unsigned char>(value!=0));break;
        case 4:hologram_write(v,0x74b,static_cast<unsigned char>(value!=0));break;
        case 5:case 6:case 7:case 8:ok=vehicle_map_set_part(v,h,index==6||index==8,value,index==5||index==6);break;
        case 9:case 10:case 11:case 12:ok=vehicle_map_bulk(v,h,index>=11,index==9||index==11);break;
        case 13:case 14:ok=vehicle_map_headlight(v,h,static_cast<unsigned char>(index==13));break;
        case 15:case 16:hologram_write(v,0x74e,static_cast<unsigned char>(index==15));break;
        case 17:case 18:{
            const bool on=index==17;
            if((hologram_read<unsigned char>(v,0x758)!=0)==on)break;
            ok=h.sound&&h.sound(h.context,v,"monitor",on?"open":"close",0);
            if(ok){hologram_write(v,0x758,static_cast<unsigned char>(on));if(!on)ok=vehicle_map_unlink(v,h);}
            break;
        }
        case 19:{
            std::uint32_t camera=0;
            ok=h.camera_value&&h.camera_value(h.context,type,word,&camera);
            if(ok){hologram_write(v,0x75c,camera);ok=vehicle_map_unlink(v,h);}
            break;
        }
        case 22:case 23:case 24:ok=vehicle_map_radio(v,h,index==22?MusicRadioInput::On:index==23?MusicRadioInput::Off:MusicRadioInput::Station,value);break;
        default:ok=false;break;
    }
    return disp_finish(c,ok);
}
const VehicleMapApi vehicle_map_api{version,sizeof(VehicleMapApi),&vehicle_map_get_schema,&vehicle_map_key,&vehicle_map_input,&health_ack};

int __cdecl vehicle_life_defaults(HologramControl* c) noexcept {
    if(!health_control(c,vehicle_object_bytes))return 0;
    auto* v=c->object;
    for(unsigned off:{0x734u,0x738u,0x73cu,0x744u,0x87cu,0x880u,0x88cu,0x75cu})hologram_write(v,off,0u);
    for(unsigned off:{0x740u,0x74bu,0x749u,0x74eu,0x74fu,0x750u,0x758u,0x730u,0x731u})hologram_write(v,off,static_cast<unsigned char>(0));
    for(unsigned off:{0x74au,0x74cu,0x74du,0x764u})hologram_write(v,off,static_cast<unsigned char>(1));
    for(unsigned off:{0x760u,0x768u,0x884u,0x888u})hologram_write(v,off,0xffffffffu);
    hologram_write(v,0x754,20.f);
    for(unsigned off:{0x864u,0x868u,0x86cu})hologram_write(v,off,1.f);
    // Original 2232de70 initializes these four scalar sets.
    for(unsigned i=0;i<4;++i){
        hologram_write(v,0x6e8+i,static_cast<unsigned char>(0));
        hologram_write(v,0x6ec+i,static_cast<unsigned char>(0));
        for(unsigned j=0;j<3;++j)hologram_write(v,0x6f0+i*12+j*4,0u);
        hologram_write(v,0x720+i*4,0u);
    }
    return 1;
}
bool vehicle_load_sounds(void* v,const VehicleLifecycleBindings& h) noexcept {
    for(unsigned i=0;i<=65536;++i){
        VehicleConfigEntry e{};unsigned present=0;
        if(!h.config.entry(h.config.context,"vehicle_sounds",i,&present,&e))return false;
        if(!present)return true;
        char key[64]{},sound[64]{};
        if(i==65536||!prop_key_read(e.name,key)||!prop_key_read(e.text,sound))return false;
        const int n=hologram_read<int>(v,0x844);
        if(n<0||n>=65536||!h.append_sound(h.context,v)||hologram_read<int>(v,0x844)!=n+1)return false;
        auto* record=const_cast<unsigned char*>(vehicle_sound_record(v,n));
        if(!record||!hologram_writable(record,128))return false;
        // Original copies only through NUL, leaving the rest untouched.
        std::memcpy(record,key,std::strlen(key)+1);
        std::memcpy(record+64,sound,std::strlen(sound)+1);
        if(!h.precache(h.context,reinterpret_cast<const char*>(record+64)))return false;
    }
    return false;
}
bool vehicle_config_load(void* v,const VehicleLifecycleBindings& h,VehicleConfigKind kind) noexcept {
    unsigned present=0;
    if(!h.open_config(h.context,v,kind,&present))return false;
    if(!present)return true;
    const bool ok=kind==VehicleConfigKind::Damage?config_damage_leaf(v,h.config):
        kind==VehicleConfigKind::Sounds?vehicle_load_sounds(v,h):config_views_leaf(v,h.config);
    const bool closed=h.close_config(h.context,v,kind)!=0;
    return ok&&closed;
}
bool vehicle_link_radio(void* v,const VehicleLifecycleBindings& h) noexcept {
    void* radio=nullptr;
    if(!h.find_radio(h.context,"env_radio",&radio))return false;
    std::uint32_t word=0xffffffffu;
    if(radio&&!h.handle(h.context,radio,&word))return false;
    hologram_write(v,0x768,word);
    // Retail code assumes env_radio exists; fail closed on missing or
    // stale link instead of dereferencing null.
    HologramControl* rc=nullptr;
    if(word==0xffffffffu||!h.resolve_radio(h.context,word,&rc)||!rc||rc->object!=radio||
       !health_control(rc,music_radio_bytes))return false;
    std::uint32_t speaker=0xffffffffu;
    if(!h.handle(h.context,v,&speaker)||speaker==0xffffffffu)return false;
    hologram_write(rc->object,0x394,speaker);
    // Original explicit virtual Spawn on the radio; do not silently
    // suppress this call because it may also occur in map load order.
    if(!h.resolve_radio(h.context,word,&rc)||!rc||rc->object!=radio||music_map_spawn(rc,&h.music)!=1)return false;
    if(!h.resolve_radio(h.context,word,&rc)||!rc||rc->object!=radio)return false;
    return h.parent_radio(h.context,radio,v,-1)!=0;
}
bool vehicle_life_ready(const HologramControl* c,const VehicleLifecycleBindings* b) noexcept {
    return health_control(c,vehicle_object_bytes)&&b&&b->base_spawn&&b->base_activate&&b->initialize_subsystem&&
        b->open_config&&b->close_config&&b->config.entry&&b->config.append_view&&b->config.attachment_id&&
        b->append_sound&&b->precache&&b->find_radio&&b->handle&&b->resolve_radio&&b->parent_radio&&
        b->initialize_components&&b->clock&&b->health_dirty&&b->spawn_tail&&b->activate_tail;
}
int __cdecl vehicle_life_spawn(HologramControl* c,const VehicleLifecycleBindings* b) noexcept {
    if(!vehicle_life_ready(c,b))return 0;
    c->busy=1;auto* v=c->object;const auto h=*b;
    hologram_write(v,0x5dc,1);hologram_write(v,0x39c,static_cast<unsigned char>(1));
    if(!h.base_spawn(h.context,v)||!h.initialize_subsystem(h.context,v,0x84c)||
       !vehicle_config_load(v,h,VehicleConfigKind::Damage)||!vehicle_config_load(v,h,VehicleConfigKind::Sounds)||
       !vehicle_link_radio(v,h)||!vehicle_config_load(v,h,VehicleConfigKind::Views)||!h.initialize_components(h.context,v))return disp_finish(c,false);
    float now=0;if(!h.clock(h.context,&now))return disp_finish(c,false);
    hologram_write(v,0x740,static_cast<unsigned char>(0));
    hologram_write(v,0x73c,static_cast<float>(static_cast<double>(now)+0.1));
    if(hologram_read<int>(v,0x9c)!=100){
        if(!h.health_dirty(h.context,v))return disp_finish(c,false);
        hologram_write(v,0x9c,100);
    }
    hologram_write(v,0x74d,static_cast<unsigned char>(1));
    hologram_write(v,0x749,static_cast<unsigned char>(0));
    return disp_finish(c,h.spawn_tail(h.context,v,hologram_read<unsigned short>(v,0x120)|0x11u)!=0);
}
int __cdecl vehicle_life_activate(HologramControl* c,const VehicleLifecycleBindings* b) noexcept {
    if(!vehicle_life_ready(c,b))return 0;
    c->busy=1;auto* v=c->object;const auto h=*b;
    if(!h.base_activate(h.context,v)||!h.initialize_subsystem(h.context,v,0x84c)||
       !vehicle_config_load(v,h,VehicleConfigKind::Views)||!vehicle_config_load(v,h,VehicleConfigKind::Damage)||
       !h.initialize_components(h.context,v)||!vehicle_config_load(v,h,VehicleConfigKind::Sounds))return disp_finish(c,false);
    return disp_finish(c,!hologram_read<std::uint32_t>(v,0x218)||h.activate_tail(h.context,v,0x870)!=0);
}
const VehicleLifecycleApi vehicle_lifecycle_api{version,sizeof(VehicleLifecycleApi),&vehicle_life_defaults,&vehicle_life_spawn,&vehicle_life_activate,&health_ack};

bool camera_ready(const VehicleCameraBindings* b) noexcept {
    return b&&b->resolve&&b->find_named&&b->is_point_camera&&b->create&&b->handle&&b->spawn&&b->remove_link&&b->diagnostic;
}
int __cdecl camera_link_defaults(HologramControl* c) noexcept {
    if(!health_control(c,camera_link_bytes))return 0;
    hologram_write(c->object,0x394,0xffffffffu);
    hologram_write(c->object,0x398,0xffffffffu);
    return 1;
}
bool camera_resolve(void* v,const VehicleCameraBindings& h,void*& link) noexcept {
    link=nullptr;const auto word=hologram_read<std::uint32_t>(v,0x760);
    return word==0xffffffffu||h.resolve(h.context,word,&link)!=0;
}
bool vehicle_camera_leaf(void* v,const VehicleCameraBindings& h) noexcept {
    const auto name=hologram_read<std::uint32_t>(v,0x75c);
    if(!name)return true; // original does not unlink when name is zero
    void* link=nullptr;
    if(!camera_resolve(v,h,link))return false;
    if(!hologram_read<unsigned char>(v,0x758)){
        if(!link)return true;
        VehicleMapBindings removal{};removal.context=h.context;removal.resolve=h.resolve;removal.unlink_camera=h.remove_link;
        return vehicle_map_unlink(v,removal);
    }
    if(link)return true;
    void* camera=nullptr;
    if(!h.find_named(h.context,name,&camera))return false;
    unsigned matched=0;
    if(camera&&!h.is_point_camera(h.context,camera,&matched))return false;
    if(camera&&matched){
        HologramControl* c=nullptr;
        if(!h.create(h.context,"info_camera_link",&c))return false;
        std::uint32_t link_word=0xffffffffu;
        if(c){
            if(!health_control(c,camera_link_bytes))return false;
            c->busy=1;
            std::uint32_t camera_word=0xffffffffu,vehicle_word=0xffffffffu;
            if(!h.handle(h.context,camera,&camera_word)||camera_word==0xffffffffu||
               !h.handle(h.context,v,&vehicle_word)||vehicle_word==0xffffffffu){
                disp_finish(c,false);return false;
            }
            hologram_write(c->object,0x394,camera_word);hologram_write(c->object,0x398,vehicle_word);
            if(!h.spawn(h.context,c->object)||!h.handle(h.context,c->object,&link_word)||link_word==0xffffffffu){
                disp_finish(c,false);return false;
            }
            disp_finish(c,true);
        }
        hologram_write(v,0x760,link_word);
    }
    if(!camera_resolve(v,h,link))return false;
    if(!link){
        if(!h.diagnostic(h.context,hologram_read<std::uint32_t>(v,0x75c)))return false;
        hologram_write(v,0x75c,0u);
    }
    return true;
}
int __cdecl vehicle_refresh_camera(HologramControl* c,const VehicleCameraBindings* b) noexcept {
    if(!health_control(c,vehicle_object_bytes)||!camera_ready(b))return 0;
    c->busy=1;const auto h=*b;return disp_finish(c,vehicle_camera_leaf(c->object,h));
}
bool vehicle_think_dirty(void* v,const VehicleThinkBindings& h) noexcept {
    if(hologram_read<unsigned char>(v,0x1f0)){hologram_write(v,0x200,static_cast<unsigned char>(1));return true;}
    const auto word=hologram_read<std::uint32_t>(v,0x16c);
    return !word||h.dirty_edict(h.context,v,word)!=0;
}
int __cdecl vehicle_think(HologramControl* c,const VehicleThinkBindings* b) noexcept {
    if(!health_control(c,vehicle_object_bytes)||!b||!passenger_refill_ready(&b->passengers)||!camera_ready(&b->camera)||
       !b->base_think||!b->wiper_update||!b->driver_update||!b->clock||!b->dirty_edict||!b->schedule||
       !b->startup||!b->active_predicate||!b->frame_time||!b->tail)return 0;
    c->busy=1;const auto h=*b;auto* v=c->object;
    if(!h.base_think(h.context,v)||!vehicle_callback(c,VehiclePermit::Parts,[&](){return h.wiper_update(h.context,v)!=0;})||!h.driver_update(h.context,v))return disp_finish(c,false);
    float now=0;
    if(!h.clock(h.context,&now))return disp_finish(c,false);
    if(hologram_read<float>(v,0x30)!=now){
        if(!vehicle_think_dirty(v,h))return disp_finish(c,false);
        hologram_write(v,0x30,now);
    }
    if(!h.clock(h.context,&now)||!h.schedule(h.context,v,now,0))return disp_finish(c,false);
    if(hologram_read<unsigned char>(v,0x2d9)!=1){
        if(!vehicle_think_dirty(v,h))return disp_finish(c,false);
        hologram_write(v,0x2d9,static_cast<unsigned char>(1));
    }
    if(!hologram_read<unsigned char>(v,0x740)){
        hologram_write(v,0x740,static_cast<unsigned char>(1));
        if(!h.startup(h.context,v,0)||!h.startup(h.context,v,1))return disp_finish(c,false);
    }
    if(!passenger_refill_leaf(v,h.passengers)||!vehicle_camera_leaf(v,h.camera))return disp_finish(c,false);
    unsigned active=0;
    if(!h.active_predicate(h.context,v,&active))return disp_finish(c,false);
    if(!active)hologram_write(v,0x744,0.f);
    else{
        float frame=0;if(!h.frame_time(h.context,&frame))return disp_finish(c,false);
        hologram_write(v,0x744,hologram_read<float>(v,0x744)+frame);
    }
    return disp_finish(c,h.tail(h.context,v)!=0);
}
const VehicleThinkApi vehicle_think_api{version,sizeof(VehicleThinkApi),&camera_link_defaults,&vehicle_refresh_camera,&vehicle_think,&health_ack};

bool seat_event_bindings(const VehicleSeatEventBindings* b,int seat) noexcept {
    return seat>=0&&seat<3&&b&&b->base_enter&&b->base_exit&&
        b->participant&&b->sound&&b->refresh_environment&&b->controller_predicate&&b->controller_followup&&
        b->dirty_edict&&b->hint.resolve&&b->hint.participant&&b->hint.send&&b->views.cast_player&&b->views.dirty_edict;
}
bool seat_event_doors(void* v,const VehicleSeatEventBindings& h,int seat) noexcept {
    for(int i=0;i<hologram_read<int>(v,0x81c);++i){
        auto* rec=vehicle_map_record(v,false,i);if(!rec)return false;
        if(hologram_read<int>(rec,0x48)!=seat)continue;
        if(!rec[0x44]){
            if(!h.sound(h.context,v,"door","open",i))return false;
            rec=vehicle_map_record(v,false,i);if(!rec)return false;
        }
        rec[0x44]=1;rec[0x45]=1;
    }
    return true;
}
bool seat_event_enter_leaf(HologramControl* c,const VehicleSeatEventBindings& h,int seat,void* actor) noexcept {
    auto* v=c->object;
    if(!h.base_enter(h.context,v,seat,actor))return false;
    unsigned player=0;
    if(!h.participant(h.context,actor,&player)||(player&&!seat_event_doors(v,h,seat)))return false;
    if(!h.refresh_environment(h.context,v))return false;
    unsigned pred=0;
    if(!h.controller_predicate(h.context,v,&pred)||(pred&&!h.controller_followup(h.context,v)))return false;
    if(!h.participant(h.context,actor,&player))return false;
    if(player){
        HologramControl* p=nullptr;
        if(!h.views.cast_player(h.views.context,actor,&p)||!p||p==c||p->object!=actor||!health_control(p,view_player_extent))return false;
        p->busy=1;const bool ok=vehicle_seat_default_leaf(v,p->object,h.views,seat);p->busy=0;
        if(!ok){p->blocked=1;return false;}
    }
    return vehicle_hint_notify(v,c->bytes,&h.hint,seat,1)==1;
}
bool seat_event_exit_leaf(HologramControl* c,const VehicleSeatEventBindings& h,int seat) noexcept {
    auto* v=c->object;
    const auto word=hologram_read<std::uint32_t>(v,0x6a4+seat*4);
    void* actor=nullptr;
    if(word==0xffffffffu||!h.hint.resolve(h.hint.context,word,&actor)||!actor)return false;
    unsigned player=0;
    if(!h.participant(h.context,actor,&player)||(player&&!seat_event_doors(v,h,seat)))return false;
    if(vehicle_hint_notify(v,c->bytes,&h.hint,seat,0)!=1)return false;
    if(seat==0&&hologram_read<unsigned char>(v,0x748)){
        if(hologram_read<unsigned char>(v,0x1f0))hologram_write(v,0x200,static_cast<unsigned char>(1));
        else{
            const auto edict=hologram_read<std::uint32_t>(v,0x16c);
            if(edict&&!h.dirty_edict(h.context,v,edict))return false;
        }
        hologram_write(v,0x748,static_cast<unsigned char>(0));
    }
    return h.base_exit(h.context,v,seat)!=0;
}
int __cdecl seat_event_enter(HologramControl* c,const VehicleSeatEventBindings* b,int seat,void* actor) noexcept {
    if(!actor||!health_control(c,vehicle_object_bytes)||!seat_event_bindings(b,seat))return 0;
    c->busy=1;const auto h=*b;return disp_finish(c,seat_event_enter_leaf(c,h,seat,actor));
}
int __cdecl seat_event_exit(HologramControl* c,const VehicleSeatEventBindings* b,int seat) noexcept {
    if(!health_control(c,vehicle_object_bytes)||!seat_event_bindings(b,seat))return 0;
    c->busy=1;const auto h=*b;return disp_finish(c,seat_event_exit_leaf(c,h,seat));
}
bool vehicle_nested_extent(const HologramControl* c) noexcept {
    return c->bytes>=vehicle_object_bytes&&hologram_writable(c->object,vehicle_object_bytes);
}
int __cdecl nested_seat_enter(HologramControl* c,const VehicleSeatEventBindings* b,int seat,void* actor) noexcept {
    if(!vehicle_consume(c,VehiclePermit::Enter))return 0;
    if(!vehicle_nested_extent(c)||!actor||!seat_event_bindings(b,seat))return vehicle_nested_finish(c,false);
    const auto h=*b;return vehicle_nested_finish(c,seat_event_enter_leaf(c,h,seat,actor));
}
int __cdecl nested_seat_exit(HologramControl* c,const VehicleSeatEventBindings* b,int seat) noexcept {
    if(!vehicle_consume(c,VehiclePermit::Exit))return 0;
    if(!vehicle_nested_extent(c)||!seat_event_bindings(b,seat))return vehicle_nested_finish(c,false);
    const auto h=*b;return vehicle_nested_finish(c,seat_event_exit_leaf(c,h,seat));
}
const VehicleSeatEventsApi vehicle_seat_events_api{version,sizeof(VehicleSeatEventsApi),&seat_event_enter,&seat_event_exit,&health_ack};

constexpr CameraLinkField camera_link_fields[]={
    {"PointCamera",CameraLinkFieldKind::Key,2,0x39c},
    {"m_hCamera",CameraLinkFieldKind::Saved,0xd,0x394},
    {"m_hTargetEntity",CameraLinkFieldKind::Saved,0xd,0x398},
    {"SetCamera",CameraLinkFieldKind::Input,2,0}
};
const CameraLinkSchema camera_link_schema{"info_camera_link",camera_link_bytes,4,camera_link_fields};
const CameraLinkSchema* __cdecl camera_link_get_schema() noexcept{return &camera_link_schema;}
int __cdecl camera_link_construct(HologramControl* c,const CameraLinkBindings* b) noexcept {
    if(!health_control(c,camera_link_bytes)||!b||!b->base_construct||!b->register_link)return 0;
    c->busy=1;const auto h=*b;auto* o=c->object;
    if(!h.base_construct(h.context,o,1))return disp_finish(c,false);
    hologram_write(o,0x394,0xffffffffu);hologram_write(o,0x398,0xffffffffu);
    return disp_finish(c,h.register_link(h.context,o)!=0);
}
int __cdecl camera_link_destroy(HologramControl* c,const CameraLinkBindings* b) noexcept {
    // Destruction is also the reconciliation path for failed callbacks.
    // Host owns final deallocation and must retain storage until return.
    if(!c||c->busy||c->bytes<camera_link_bytes||!hologram_writable(c->object,camera_link_bytes)||!b||!b->unregister_link||!b->base_destroy)return 0;
    c->busy=1;const auto h=*b;
    return disp_finish(c,h.unregister_link(h.context,c->object)&&h.base_destroy(h.context,c->object));
}
int __cdecl camera_link_key(HologramControl* c,const char* name,unsigned type,std::uint32_t word) noexcept {
    if(!health_control(c,camera_link_bytes)||type!=2)return 0;
    char key[64]{};
    if(!prop_key_read(name,key)||std::strcmp(key,"PointCamera"))return 0;
    hologram_write(c->object,0x39c,word);return 1;
}
bool camera_link_resolve(void* o,unsigned off,const CameraLinkBindings& h,void*& entity) noexcept {
    const auto word=hologram_read<std::uint32_t>(o,off);entity=nullptr;
    return word==0xffffffffu||h.resolve(h.context,word,&entity)!=0;
}
bool camera_link_set(void* o,const CameraLinkBindings& h,std::uint32_t name) noexcept {
    void* found=nullptr;
    if(!h.find_named(h.context,name,&found))return false;
    if(!found)return true; // original missing name preserves both fields
    unsigned camera=0;
    if(!h.is_point_camera(h.context,found,&camera))return false;
    std::uint32_t word=0xffffffffu;
    if(camera&&!h.handle(h.context,found,&word))return false;
    hologram_write(o,0x394,word);
    // A found entity with the wrong dynamic type invalidates the handle
    // but does not replace the previously stored string.
    void* resolved=nullptr;
    if(!camera_link_resolve(o,0x394,h,resolved))return false;
    if(resolved)hologram_write(o,0x39c,name);
    return true;
}
bool camera_link_names_ready(const CameraLinkBindings* b) noexcept {
    return b&&b->find_named&&b->is_point_camera&&b->handle&&b->resolve;
}
int __cdecl camera_link_input(HologramControl* c,const CameraLinkBindings* b,const char* name,unsigned type,std::uint32_t word) noexcept {
    if(!health_control(c,camera_link_bytes)||!camera_link_names_ready(b))return 0;
    char key[64]{};
    if(!prop_key_read(name,key)||std::strcmp(key,"SetCamera")||(type!=2&&!b->convert))return 0;
    c->busy=1;const auto h=*b;std::uint32_t value=word;
    if(type!=2&&!h.convert(h.context,type,word,&value))return disp_finish(c,false);
    return disp_finish(c,camera_link_set(c->object,h,value));
}
int __cdecl camera_link_resolve_names(HologramControl* c,const CameraLinkBindings* b) noexcept {
    if(!health_control(c,camera_link_bytes)||!camera_link_names_ready(b)||!b->base_resolve_names)return 0;
    c->busy=1;const auto h=*b;auto* o=c->object;
    if(!h.base_resolve_names(h.context,o))return disp_finish(c,false);
    void* resolved=nullptr;
    if(!camera_link_resolve(o,0x394,h,resolved))return disp_finish(c,false);
    if(!resolved&&!camera_link_set(o,h,hologram_read<std::uint32_t>(o,0x39c)))return disp_finish(c,false);
    if(!camera_link_resolve(o,0x398,h,resolved))return disp_finish(c,false);
    if(!resolved){
        // Base target-name word +90, no camera cast for target entity.
        void* target=nullptr;std::uint32_t word=0xffffffffu;
        if(!h.find_named(h.context,hologram_read<std::uint32_t>(o,0x90),&target)||
           (target&&!h.handle(h.context,target,&word)))return disp_finish(c,false);
        hologram_write(o,0x398,word);
    }
    return disp_finish(c,true);
}
int __cdecl camera_link_ack(HologramControl* c) noexcept {
    if(!c||c->busy||!c->blocked||c->bytes<camera_link_bytes||!hologram_writable(c->object,camera_link_bytes))return 0;
    c->blocked=0;return 1;
}
const CameraLinkApi camera_link_api{version,sizeof(CameraLinkApi),&camera_link_get_schema,&camera_link_construct,&camera_link_destroy,&camera_link_key,&camera_link_input,&camera_link_resolve_names,&camera_link_ack};

bool part_pose_read(void* v,unsigned char* rec,const VehiclePartEventBindings& h,float& value) noexcept {
    return h.pose(h.context,v,hologram_read<int>(rec,0x40),&value)!=0;
}
bool part_pose_step(void* v,unsigned char* rec,const VehiclePartEventBindings& h,bool open,double& value) noexcept {
    float pose=0,frame=0;
    if(!part_pose_read(v,rec,h,pose)||!h.frame_time(h.context,&frame))return false;
    value=static_cast<double>(pose)+(open?1.:-1.)*4.*static_cast<double>(frame);
    return true;
}
bool part_events_one(void* v,const VehiclePartEventBindings& h,bool window,int index) noexcept {
    auto* rec=vehicle_map_record(v,window,index);if(!rec)return false;
    // The original tests the requested flag and the live pose before
    // attempting a step. Already completed states do not refire outputs.
    bool open=rec[0x44]!=0;float current=0;
    if(open){
        if(!part_pose_read(v,rec,h,current))return false;
        if(!(current<1.f)){
            if(rec[0x44])return true;
            open=false; // original rechecks desired flag before close path
        }
    }
    if(!open){
        if(!part_pose_read(v,rec,h,current))return false;
        if(!(current>0.f))return true;
    }
    double next=0;
    float stored=0;
    bool reached=false;
    // The original re-reads the pose/frame pair for each comparison.
    // Preserve the order; the host supplies the live engine values.
    if(!part_pose_step(v,rec,h,open,next))return false;
    if(next>1.){stored=1.f;reached=open;}
    else{
        if(!part_pose_step(v,rec,h,open,next))return false;
        if(next<0.){stored=0.f;reached=!open;}
        else{
            if(!part_pose_step(v,rec,h,open,next))return false;
            stored=static_cast<float>(next);
            reached=open?next>=1.:next<=0.;
        }
    }
    if(reached){
        const unsigned off=window?(open?0x7b4:0x79c):(open?0x784:0x76c);
        // Original holds this record across synchronous output. Host
        // must not invalidate the object or record before return.
        if(!h.output(h.context,v,off,index))return false;
        if(!hologram_writable(rec,76))return false;
        if(!window&&open&&rec[0x45]){
            rec[0x44]=0;rec[0x45]=0;
            if(!h.sound(h.context,v,"door","close",index))return false;
        }
    }
    return h.set_pose(h.context,v,hologram_read<int>(rec,0x40),stored)!=0;
}
bool part_events_bindings(const VehiclePartEventBindings* b) noexcept {
    return b&&b->pose&&b->frame_time&&b->set_pose&&b->output&&b->sound;
}
bool vehicle_part_events_leaf(void* v,const VehiclePartEventBindings& h) noexcept {
    for(unsigned window=0;window<2;++window){
        for(int i=0;i<hologram_read<int>(v,window?0x830:0x81c);++i){
            if(i>=65536||!part_events_one(v,h,window!=0,i))return false;
        }
    }
    return true;
}
int __cdecl vehicle_part_events_tick(HologramControl* c,const VehiclePartEventBindings* b) noexcept {
    if(!health_control(c,vehicle_object_bytes)||!part_events_bindings(b))return 0;
    c->busy=1;const auto h=*b;return disp_finish(c,vehicle_part_events_leaf(c->object,h));
}
int __cdecl nested_part_events(HologramControl* c,const VehiclePartEventBindings* b) noexcept {
    if(!vehicle_consume(c,VehiclePermit::Parts))return 0;
    if(!vehicle_nested_extent(c)||!part_events_bindings(b))return vehicle_nested_finish(c,false);
    const auto h=*b;return vehicle_nested_finish(c,vehicle_part_events_leaf(c->object,h));
}
const VehicleNestedApi vehicle_nested_api{version,sizeof(VehicleNestedApi),&nested_seat_enter,&nested_seat_exit,&nested_part_events};
const VehiclePartEventsApi vehicle_part_events_api{version,sizeof(VehiclePartEventsApi),&vehicle_part_events_tick,&health_ack};

constexpr NpcMapField npc_jessica_fields[]={
    {"ThrowItemAtPlayer",NpcMapFieldKind::Input,2,0x223622d0u},
    {"SetPrimaryAmmoCountToThrow",NpcMapFieldKind::Input,5,0x22362320u},
    {"SetSecondaryAmmoCountToThrow",NpcMapFieldKind::Input,5,0x22362350u},
    {"HolsterWeapon",NpcMapFieldKind::Input,5,0x22362300u}
};
constexpr NpcMapField npc_count_fields[]={
    {"InterrogationStart",NpcMapFieldKind::Input,0,0x2234e2a0u},
    {"InterrogationStop",NpcMapFieldKind::Input,0,0x2234e2b0u}
};
constexpr NpcMapField npc_actor_fields[]={
    {"bloodColor",NpcMapFieldKind::Key,5,0},
    {"initialHealth",NpcMapFieldKind::Key,5,0},
    {"heads",NpcMapFieldKind::Key,5,0},
    {"dontHammerBBox",NpcMapFieldKind::Key,5,0},
    {"PlayerCollide",NpcMapFieldKind::Input,6,0x22357950u}
};
constexpr NpcMapSchema npc_map_schemas[]={
    {"npc_jessica",0x1458,0x223c6e10u,4,npc_jessica_fields},
    {"npc_count",0x103c,0x223c66d0u,2,npc_count_fields},
    {"npc_generic_actor",generic_actor_bytes,0x223c6b90u,5,npc_actor_fields},
    {"npc_jc",jc_bytes,0x223c6df0u,0,nullptr}
};
const NpcMapSchema* __cdecl npc_map_schema(NpcMapKind kind) noexcept {
    const auto i=static_cast<unsigned>(kind);return i<4?&npc_map_schemas[i]:nullptr;
}
bool npc_map_ready(NpcMapKind kind,HologramControl* c) noexcept {
    const auto* schema=npc_map_schema(kind);return schema&&health_control(c,schema->bytes);
}
int __cdecl npc_map_key(NpcMapKind kind,HologramControl* c,const char* name,unsigned type,std::uint32_t word) noexcept {
    if(kind!=NpcMapKind::GenericActor||!npc_map_ready(kind,c))return 0;
    const ActorValue value{type,word};return generic_actor_key(c,name,&value);
}
int __cdecl npc_map_input(NpcMapKind kind,HologramControl* c,const NpcMapBindings* b,const char* name,unsigned type,std::uint32_t word) noexcept {
    if(!npc_map_ready(kind,c)||!b)return 0;
    char key[128]{};if(!prop_key_read(name,key))return 0;
    const auto* s=npc_map_schema(kind);
    for(unsigned i=0;i<s->count;++i){
        const auto& field=s->fields[i];if(field.kind!=NpcMapFieldKind::Input||std::strcmp(field.name,key))continue;
        if(kind==NpcMapKind::Jessica){
            const JessicaValue v{type,word};return jessica_input(c,&b->jessica,static_cast<JessicaInput>(i),&v);
        }
        if(kind==NpcMapKind::Count)return count_input(c,i==0?1u:0u);
        if(kind==NpcMapKind::GenericActor){
            // 22357950 copies the actual byte, not normalized boolean.
            hologram_write(c->object,0x100c,static_cast<unsigned char>(type==6?word:0u));return 1;
        }
    }
    return 0;
}
int __cdecl npc_map_observe(NpcMapKind kind,HologramControl* c,NpcMapSession* state,const NpcMapBindings* b,const void* obs,unsigned bytes) noexcept {
    if(!npc_map_ready(kind,c)||!b||!state||!obs||bytes<0x5e9||!sin_independent::vgui_layout::Readable(obs,0x5e9))return 0;
    switch(kind){
        case NpcMapKind::Jessica:return jessica_attention(c,&state->attention,&b->attention,obs,bytes);
        case NpcMapKind::Count:return count_active_observe(c,&b->scheduler,&b->count,obs,bytes);
        case NpcMapKind::GenericActor:return generic_actor_observe(c,&b->actor,obs);
        default:return 0; // JC's inherited route stays with engine host
    }
}
int __cdecl npc_map_select_count(HologramControl* c,NpcMapSession* state,const NpcMapBindings* b,int* result) noexcept {
    return npc_map_ready(NpcMapKind::Count,c)&&state&&b?count_select(c,&state->scheduler,&b->scheduler,result):0;
}
const NpcMapApi npc_map_api{version,sizeof(NpcMapApi),&npc_map_schema,&npc_map_key,&npc_map_input,&npc_map_observe,&npc_map_select_count};
int __cdecl npc_map_dialogue_observe(NpcMapKind kind,HologramControl* c,NpcMapSession* session,const NpcMapBindings* map,
                                    const NpcMapDialogueBindings* dialogue,const void* obs,unsigned bytes) noexcept {
    if(!npc_map_ready(kind,c)||!session||!map||!dialogue||!obs||bytes<0x5e9||
       !sin_independent::vgui_layout::Readable(obs,0x5e9))return 0;
    if(kind==NpcMapKind::Jessica)return dialogue_jessica(c,&session->attention,&dialogue->jessica,obs,bytes);
    if(kind==NpcMapKind::Count){
        if(!dialogue_count_ready(&dialogue->count))return 0;
        CountDialogueCall call{c,dialogue->count};const auto host=dialogue_count_bindings(call);
        // Do NOT call dialogue_count directly: active non-interrogation
        // behaviors must still receive the original scheduler callback.
        return count_active_observe(c,&map->scheduler,&host,obs,bytes);
    }
    return 0;
}
const NpcMapDialogueApi npc_map_dialogue_api{version,sizeof(NpcMapDialogueApi),&npc_map_dialogue_observe};
int __cdecl integrated_npc_map_observe(NpcMapKind kind,HologramControl* c,NpcMapSession* session,const NpcMapBindings* map,
    const NpcMapDialogueBindings* dialogue,const TalkManagerRuntimeBindings* manager,const IntegratedNpcMapDialogueBindings* scene,
    const void* obs,unsigned bytes) noexcept {
    if(!npc_map_ready(kind,c)||!session||!map||!dialogue||!scene||!obs||bytes<0x5e9||
       !sin_independent::vgui_layout::Readable(obs,0x5e9)||!integrated_response_ready(&scene->response,&scene->lifecycle))return 0;
    const auto h=*scene;
    if(kind==NpcMapKind::Jessica)return integrated_jessica_observe(c,&session->attention,&dialogue->jessica,manager,&h.response,&h.lifecycle,obs,bytes);
    if(kind==NpcMapKind::Count){
        if(!dialogue_count_ready(&dialogue->count))return 0;
        CountDialogueCall call{c,dialogue->count,nullptr,0,&h.response,&h.lifecycle};const auto callbacks=dialogue_count_bindings(call);
        return count_active_observe(c,&map->scheduler,&callbacks,obs,bytes);
    }
    return 0;
}
const IntegratedNpcMapDialogueApi integrated_npc_map_dialogue_api{version,sizeof(IntegratedNpcMapDialogueApi),&integrated_npc_map_observe};
int __cdecl native_generated_map_observe(NpcMapKind kind,HologramControl* c,NpcMapSession* session,const NpcMapBindings* map,
    const NpcMapDialogueBindings* dialogue,const TalkManagerRuntimeBindings* manager,const NativeGeneratedResponseBindings* scene,
    const void* obs,unsigned bytes) noexcept {
    if(!npc_map_ready(kind,c)||!session||!map||!dialogue||!obs||bytes<0x5e9||
       !sin_independent::vgui_layout::Readable(obs,0x5e9)||!native_generated_response_ready(scene))return 0;
    const auto h=*scene;
    if(kind==NpcMapKind::Jessica){
        const auto& b=dialogue->jessica;
        if(!b.clock||!b.criteria||!b.semaphore||(!manager&&!b.cooldown))return 0;
        JessicaDialogueCall call{c,b,manager,nullptr,nullptr,&h};
        const JessicaAttentionBindings host{&call,&dialogue_jessica_clock,&dialogue_jessica_criteria};
        return jessica_attention(c,&session->attention,&host,obs,bytes);
    }
    if(kind==NpcMapKind::Count){
        if(!dialogue_count_ready(&dialogue->count))return 0;
        CountDialogueCall call{c,dialogue->count,nullptr,0,nullptr,nullptr,&h};
        const auto callbacks=dialogue_count_bindings(call);
        return count_active_observe(c,&map->scheduler,&callbacks,obs,bytes);
    }
    return 0;
}
const NativeGeneratedMapDialogueApi native_generated_map_dialogue_api{version,sizeof(NativeGeneratedMapDialogueApi),&native_generated_map_observe};


int __cdecl jessica_full_spawn(HologramControl* c,const JessicaSpawnBindings* b) noexcept {
    if(!health_control(c,jessica_size)||!b||!b->base_spawn||!b->resolve_parent||!b->parented_setup||
       !b->restore_unparented||!b->health_dirty||!b->npc_initialize||!b->install_interaction||!b->expeditor||!b->seed_concept)return 0;
    c->busy=1;const auto h=*b;auto* n=c->object;
    if(!h.base_spawn(h.context,n))return disp_finish(c,false);
    void* parent=nullptr;const auto word=hologram_read<std::uint32_t>(n,0xf0);
    if(word!=0xffffffffu&&!h.resolve_parent(h.context,word,&parent))return disp_finish(c,false);
    if(parent){
        if(!h.parented_setup(h.context,n,0,0))return disp_finish(c,false);
        // Original clears all capabilities before adding the two masks.
        hologram_write(n,0x8ec,0x00801000u);
        hologram_write(n,0x8ec,hologram_read<unsigned>(n,0x8ec)|0x02000000u);
    }else if(!h.restore_unparented(h.context,n))return disp_finish(c,false);
    hologram_write(n,0xbb,static_cast<unsigned char>(hologram_read<unsigned char>(n,0xbb)|0x58u));
    if(hologram_read<int>(n,0x9c)!=80){
        if(!h.health_dirty(h.context,n))return disp_finish(c,false);
        hologram_write(n,0x9c,80);
    }
    jessica_ammo_defaults(n);
    if(!h.npc_initialize(h.context,n)||!h.install_interaction(h.context,n,0x80,0x22361e40u))return disp_finish(c,false);
    void* expeditor=nullptr;
    if(!h.expeditor(h.context,n,&expeditor)||!expeditor)return disp_finish(c,false);
    return disp_finish(c,h.seed_concept(h.context,expeditor,"TLK_HELLO",0,1)!=0);
}
int __cdecl count_class_init(HologramControl* c,const CountClassInitBindings* b,std::uint32_t classname,unsigned* accepted) noexcept {
    if(!health_control(c,0x103c)||!b||!b->base_postconstruct||!b->base_create||!b->behaviors.append||!b->behaviors.register_base||!b->create_expeditor||!accepted)return 0;
    c->busy=1;const auto h=*b;auto* n=c->object;
    *accepted=0;
    if(!h.base_postconstruct(h.context,n,classname))return disp_finish(c,false);
    unsigned base_ok=0;
    if(!h.base_create(h.context,n,&base_ok))return disp_finish(c,false);
    if(!base_ok)return disp_finish(c,true);
    if(!count_register_leaf(n,h.behaviors))return disp_finish(c,false);
    void* expeditor=nullptr;unsigned bytes=0;
    if(!h.create_expeditor(h.context,n,&expeditor,&bytes))return disp_finish(c,false);
    if(expeditor&&(bytes<0x7c||!hologram_writable(expeditor,0x7c)))return disp_finish(c,false);
    hologram_write(n,0xffc,count_pointer(expeditor));
    if(!expeditor)return disp_finish(c,true);
    hologram_write(expeditor,0xc,count_pointer(static_cast<unsigned char*>(n)+0xdfc));
    *accepted=1;
    return disp_finish(c,true);
}
int __cdecl count_spawn_profile(HologramControl* c,const JcProfileBindings* b) noexcept {
    if(!health_control(c,0x103c)||!b||!b->health_dirty||!b->npc_initialize)return 0;
    c->busy=1;const auto h=*b;return disp_finish(c,humanoid_profile_leaf(c->object,h));
}
const NpcSpawnApi npc_spawn_api{version,sizeof(NpcSpawnApi),&jessica_full_spawn,&count_class_init,&count_spawn_profile,&health_ack};
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

constexpr unsigned hint_hud_bytes=0x178;
int hint_finish(HologramControl* c,bool ok) noexcept {c->busy=0;if(!ok){c->blocked=1;return -1;}return 1;}
bool hint_hud_ready(HologramControl* c,const HintHudBindings* b) noexcept {
    return c&&!c->busy&&!c->blocked&&c->bytes>=hint_hud_bytes&&hologram_writable(c->object,hint_hud_bytes)&&b&&b->rebuild&&b->visible&&b->animation&&b->clock;
}
int __cdecl hint_hud_receive(HologramControl* c,const HintHudBindings* b,const unsigned char* data,unsigned bytes) noexcept {
    if(!hint_hud_ready(c,b)||!data||!bytes||!sin_independent::vgui_layout::Readable(data,bytes))return 0;
    // Original count mismatch logs and returns, without changing HUD.
    if(data[0]!=1)return 1;
    if(bytes<2)return 0;
    const auto* text=reinterpret_cast<const char*>(data+1);
    const auto* end=static_cast<const char*>(std::memchr(text,0,bytes-1));
    if(!end||end-text>=128)return 0; // original raw token occupies d4..153
    const unsigned remaining=bytes-static_cast<unsigned>(end-text)-2;
    // Vehicle sender omits the duration; original bf_read returns 0 on
    // the failed 32-bit read. Do not invent a persistent duration.
    float duration=0;
    if(remaining>=4)std::memcpy(&duration,end+1,4);
    const auto h=*b;c->busy=1;
    unsigned built=0;
    // Rebuild owns clear/localization and the raw token write, before
    // the visible/animation operations in the actual receiver.
    if(!h.rebuild(h.context,c->object,text,&built))return hint_finish(c,false);
    if(!built)return hint_finish(c,h.animation(h.context,"HintMessageHide")!=0);
    if(!h.visible(h.context,c->object,1)||!h.animation(h.context,"HintMessageShow"))return hint_finish(c,false);
    float now=0;if(!h.clock(h.context,&now))return hint_finish(c,false);
    hologram_write(c->object,0x154,static_cast<float>(static_cast<double>(now)+duration));
    return hint_finish(c,true);
}
int __cdecl hint_hud_tick(HologramControl* c,const HintHudBindings* b) noexcept {
    if(!hint_hud_ready(c,b))return 0;
    const float deadline=hologram_read<float>(c->object,0x154);
    if(deadline==0)return 1;
    const auto h=*b;c->busy=1;float now=0;
    if(!h.clock(h.context,&now))return hint_finish(c,false);
    if(deadline<now){
        if(!h.animation(h.context,"HintMessageHide"))return hint_finish(c,false);
        hologram_write(c->object,0x154,0.f);
    }
    return hint_finish(c,true);
}
int __cdecl hint_hud_register(const HintHudBindings* b) noexcept {
    if(!b||!b->register_message)return 0;
    return b->register_message(b->context,hint_message_name,-1)?1:-1;
}
int __cdecl hint_hud_route(const HintHudBindings* b,const unsigned char* data,unsigned bytes) noexcept {
    if(!b||!b->find_hud)return 0;
    const auto h=*b;HologramControl* c=nullptr;
    if(!h.find_hud(h.context,"CHudHintDisplay",&c))return -1;
    return c?hint_hud_receive(c,&h,data,bytes):1;
}
int __cdecl hint_hud_ack(HologramControl* c) noexcept {
    if(!c||c->busy||!c->blocked||c->bytes<hint_hud_bytes||!hologram_writable(c->object,hint_hud_bytes))return 0;
    c->blocked=0;return 1;
}
const HintHudApi hint_hud_api{version,sizeof(HintHudApi),&hint_hud_receive,&hint_hud_tick,&hint_hud_register,&hint_hud_route,&hint_hud_ack};
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

#pragma comment(linker,"/EXPORT:SiN_GetJessicaInteractionApi=_SiN_GetJessicaInteractionApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::JessicaInteractionApi* __cdecl
SiN_GetJessicaInteractionApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(JessicaInteractionApi)?&jessica_interaction_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetResponsePolicyApi=_SiN_GetResponsePolicyApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ResponsePolicyApi* __cdecl
SiN_GetResponsePolicyApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ResponsePolicyApi)?&response_policy_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetResponseSelectApi=_SiN_GetResponseSelectApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ResponseSelectApi* __cdecl
SiN_GetResponseSelectApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ResponseSelectApi)?&response_select_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetResponseDispatchApi=_SiN_GetResponseDispatchApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ResponseDispatchApi* __cdecl
SiN_GetResponseDispatchApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ResponseDispatchApi)?&response_dispatch_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetResponseTimingUpdateApi=_SiN_GetResponseTimingUpdateApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ResponseTimingUpdateApi* __cdecl
SiN_GetResponseTimingUpdateApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ResponseTimingUpdateApi)?&response_timing_update_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetResponseRecordApi=_SiN_GetResponseRecordApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ResponseRecordApi* __cdecl
SiN_GetResponseRecordApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ResponseRecordApi)?&response_record_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetResponseSpeakApi=_SiN_GetResponseSpeakApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ResponseSpeakApi* __cdecl
SiN_GetResponseSpeakApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ResponseSpeakApi)?&response_speak_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetResponseRepeatApi=_SiN_GetResponseRepeatApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ResponseRepeatApi* __cdecl
SiN_GetResponseRepeatApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ResponseRepeatApi)?&response_repeat_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetExpeditorLifecycleApi=_SiN_GetExpeditorLifecycleApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ExpeditorLifecycleApi* __cdecl
SiN_GetExpeditorLifecycleApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ExpeditorLifecycleApi)?&expeditor_lifecycle_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetTalkCatalogApi=_SiN_GetTalkCatalogApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::TalkCatalogApi* __cdecl
SiN_GetTalkCatalogApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(TalkCatalogApi)?&talk_catalog_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetCompanionTalkApi=_SiN_GetCompanionTalkApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::CompanionTalkApi* __cdecl
SiN_GetCompanionTalkApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(CompanionTalkApi)?&companion_talk_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetTalkPropagationApi=_SiN_GetTalkPropagationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::TalkPropagationApi* __cdecl
SiN_GetTalkPropagationApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(TalkPropagationApi)?&talk_propagation_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetNpcMapDialogueApi=_SiN_GetNpcMapDialogueApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::NpcMapDialogueApi* __cdecl
SiN_GetNpcMapDialogueApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(NpcMapDialogueApi)?&npc_map_dialogue_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetManagedJessicaObservationApi=_SiN_GetManagedJessicaObservationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ManagedJessicaObservationApi* __cdecl
SiN_GetManagedJessicaObservationApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ManagedJessicaObservationApi)?&managed_jessica_observation_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetNpcDialogueApi=_SiN_GetNpcDialogueApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::NpcDialogueApi* __cdecl
SiN_GetNpcDialogueApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(NpcDialogueApi)?&npc_dialogue_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetTalkManagerApi=_SiN_GetTalkManagerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::TalkManagerApi* __cdecl
SiN_GetTalkManagerApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(TalkManagerApi)?&talk_manager_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetTalkManagerRuntimeApi=_SiN_GetTalkManagerRuntimeApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::TalkManagerRuntimeApi* __cdecl
SiN_GetTalkManagerRuntimeApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(TalkManagerRuntimeApi)?&talk_manager_runtime_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetManagedCompanionApi=_SiN_GetManagedCompanionApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ManagedCompanionApi* __cdecl
SiN_GetManagedCompanionApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ManagedCompanionApi)?&managed_companion_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetChoreographyFinalizeApi=_SiN_GetChoreographyFinalizeApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ChoreographyFinalizeApi* __cdecl
SiN_GetChoreographyFinalizeApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ChoreographyFinalizeApi)?&choreography_finalize_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetChoreographyStartupSoundApi=_SiN_GetChoreographyStartupSoundApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ChoreographyStartupSoundApi* __cdecl
SiN_GetChoreographyStartupSoundApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ChoreographyStartupSoundApi)?&choreography_startup_sound_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetNativeVcdRuntimeApi=_SiN_GetNativeVcdRuntimeApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::NativeVcdRuntimeApi* __cdecl
SiN_GetNativeVcdRuntimeApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(NativeVcdRuntimeApi)?&native_vcd_runtime_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetPortableVcdFinalizedGrammarApi=_SiN_GetPortableVcdFinalizedGrammarApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PortableVcdFinalizedGrammarApi* __cdecl
SiN_GetPortableVcdFinalizedGrammarApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PortableVcdFinalizedGrammarApi)?&portable_vcd_finalized_grammar_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetChoreographyFlexOwnershipApi=_SiN_GetChoreographyFlexOwnershipApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ChoreographyFlexOwnershipApi* __cdecl
SiN_GetChoreographyFlexOwnershipApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ChoreographyFlexOwnershipApi)?&choreography_flex_ownership_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetChoreographyFlexApi=_SiN_GetChoreographyFlexApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ChoreographyFlexApi* __cdecl
SiN_GetChoreographyFlexApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ChoreographyFlexApi)?&choreography_flex_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetPortableVcdFlexGrammarApi=_SiN_GetPortableVcdFlexGrammarApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PortableVcdFlexGrammarApi* __cdecl
SiN_GetPortableVcdFlexGrammarApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PortableVcdFlexGrammarApi)?&portable_vcd_flex_grammar_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetChoreographyScaleTreeApi=_SiN_GetChoreographyScaleTreeApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ChoreographyScaleTreeApi* __cdecl
SiN_GetChoreographyScaleTreeApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ChoreographyScaleTreeApi)?&choreography_scale_tree_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetChoreographyScaleApi=_SiN_GetChoreographyScaleApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ChoreographyScaleApi* __cdecl
SiN_GetChoreographyScaleApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ChoreographyScaleApi)?&choreography_scale_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetPortableVcdCurveGrammarApi=_SiN_GetPortableVcdCurveGrammarApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PortableVcdCurveGrammarApi* __cdecl
SiN_GetPortableVcdCurveGrammarApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PortableVcdCurveGrammarApi)?&portable_vcd_curve_grammar_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetChoreographyRampApi=_SiN_GetChoreographyRampApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ChoreographyRampApi* __cdecl
SiN_GetChoreographyRampApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ChoreographyRampApi)?&choreography_ramp_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetPortableVcdGrammarApi=_SiN_GetPortableVcdGrammarApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PortableVcdGrammarApi* __cdecl
SiN_GetPortableVcdGrammarApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PortableVcdGrammarApi)?&portable_vcd_grammar_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetPortableVcdTokenizerApi=_SiN_GetPortableVcdTokenizerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PortableVcdTokenizerApi* __cdecl
SiN_GetPortableVcdTokenizerApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PortableVcdTokenizerApi)?&portable_vcd_tokenizer_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetChoreographyTimingTagsApi=_SiN_GetChoreographyTimingTagsApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ChoreographyTimingTagsApi* __cdecl
SiN_GetChoreographyTimingTagsApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ChoreographyTimingTagsApi)?&choreography_timing_tags_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetNativeNamedPlaybackApi=_SiN_GetNativeNamedPlaybackApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::NativeNamedPlaybackApi* __cdecl
SiN_GetNativeNamedPlaybackApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(NativeNamedPlaybackApi)?&native_named_playback_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetChoreographyEventParserApi=_SiN_GetChoreographyEventParserApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ChoreographyEventParserApi* __cdecl
SiN_GetChoreographyEventParserApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ChoreographyEventParserApi)?&choreography_event_parser_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetChoreographyActorParserApi=_SiN_GetChoreographyActorParserApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ChoreographyActorParserApi* __cdecl
SiN_GetChoreographyActorParserApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ChoreographyActorParserApi)?&choreography_actor_parser_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetNamedScenePrecacheApi=_SiN_GetNamedScenePrecacheApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::NamedScenePrecacheApi* __cdecl
SiN_GetNamedScenePrecacheApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(NamedScenePrecacheApi)?&named_scene_precache_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetChoreographyTextParserApi=_SiN_GetChoreographyTextParserApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ChoreographyTextParserApi* __cdecl
SiN_GetChoreographyTextParserApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ChoreographyTextParserApi)?&choreography_text_parser_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetNamedSceneLoadApi=_SiN_GetNamedSceneLoadApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::NamedSceneLoadApi* __cdecl
SiN_GetNamedSceneLoadApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(NamedSceneLoadApi)?&named_scene_load_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetOwnedChoreographyApi=_SiN_GetOwnedChoreographyApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::OwnedChoreographyApi* __cdecl
SiN_GetOwnedChoreographyApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(OwnedChoreographyApi)?&owned_choreography_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetChoreographySymbolInternApi=_SiN_GetChoreographySymbolInternApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ChoreographySymbolInternApi* __cdecl
SiN_GetChoreographySymbolInternApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ChoreographySymbolInternApi)?&choreography_symbol_intern_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetChoreographySymbolApi=_SiN_GetChoreographySymbolApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ChoreographySymbolApi* __cdecl
SiN_GetChoreographySymbolApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ChoreographySymbolApi)?&choreography_symbol_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetNativeSceneRetirementApi=_SiN_GetNativeSceneRetirementApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::NativeSceneRetirementApi* __cdecl
SiN_GetNativeSceneRetirementApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(NativeSceneRetirementApi)?&native_scene_retirement_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetChoreographyDestructionApi=_SiN_GetChoreographyDestructionApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ChoreographyDestructionApi* __cdecl
SiN_GetChoreographyDestructionApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ChoreographyDestructionApi)?&choreography_destruction_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetNativeGeneratedResponseApi=_SiN_GetNativeGeneratedResponseApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::NativeGeneratedResponseApi* __cdecl
SiN_GetNativeGeneratedResponseApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(NativeGeneratedResponseApi)?&native_generated_response_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetSceneSpeechDrainApi=_SiN_GetSceneSpeechDrainApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::SceneSpeechDrainApi* __cdecl
SiN_GetSceneSpeechDrainApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(SceneSpeechDrainApi)?&scene_speech_drain_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetNativeChoreographyDispatchApi=_SiN_GetNativeChoreographyDispatchApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::NativeChoreographyDispatchApi* __cdecl
SiN_GetNativeChoreographyDispatchApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(NativeChoreographyDispatchApi)?&native_choreography_dispatch_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetNativeChoreographyClockApi=_SiN_GetNativeChoreographyClockApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::NativeChoreographyClockApi* __cdecl
SiN_GetNativeChoreographyClockApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(NativeChoreographyClockApi)?&native_choreography_clock_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetSceneSpeechPreparationApi=_SiN_GetSceneSpeechPreparationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::SceneSpeechPreparationApi* __cdecl
SiN_GetSceneSpeechPreparationApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(SceneSpeechPreparationApi)?&scene_speech_preparation_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetResolvedSceneActorApi=_SiN_GetResolvedSceneActorApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ResolvedSceneActorApi* __cdecl
SiN_GetResolvedSceneActorApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ResolvedSceneActorApi)?&resolved_scene_actor_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetSceneActorNamesApi=_SiN_GetSceneActorNamesApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::SceneActorNamesApi* __cdecl
SiN_GetSceneActorNamesApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(SceneActorNamesApi)?&scene_actor_names_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetSceneActorCacheApi=_SiN_GetSceneActorCacheApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::SceneActorCacheApi* __cdecl
SiN_GetSceneActorCacheApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(SceneActorCacheApi)?&scene_actor_cache_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetSceneSpeechCallbackApi=_SiN_GetSceneSpeechCallbackApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::SceneSpeechCallbackApi* __cdecl
SiN_GetSceneSpeechCallbackApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(SceneSpeechCallbackApi)?&scene_speech_callback_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetSceneSpeechExecutionApi=_SiN_GetSceneSpeechExecutionApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::SceneSpeechExecutionApi* __cdecl
SiN_GetSceneSpeechExecutionApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(SceneSpeechExecutionApi)?&scene_speech_execution_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetSceneSpeechPolicyApi=_SiN_GetSceneSpeechPolicyApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::SceneSpeechPolicyApi* __cdecl
SiN_GetSceneSpeechPolicyApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(SceneSpeechPolicyApi)?&scene_speech_policy_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetActorEventRecordApi=_SiN_GetActorEventRecordApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ActorEventRecordApi* __cdecl
SiN_GetActorEventRecordApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ActorEventRecordApi)?&actor_event_record_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetSceneFinishApi=_SiN_GetSceneFinishApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::SceneFinishApi* __cdecl
SiN_GetSceneFinishApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(SceneFinishApi)?&scene_finish_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetNativeGeneratedLifecycleApi=_SiN_GetNativeGeneratedLifecycleApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::NativeGeneratedLifecycleApi* __cdecl
SiN_GetNativeGeneratedLifecycleApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(NativeGeneratedLifecycleApi)?&native_generated_lifecycle_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetSceneActorEventsApi=_SiN_GetSceneActorEventsApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::SceneActorEventsApi* __cdecl
SiN_GetSceneActorEventsApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(SceneActorEventsApi)?&scene_actor_events_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetSceneStartApi=_SiN_GetSceneStartApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::SceneStartApi* __cdecl
SiN_GetSceneStartApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(SceneStartApi)?&scene_start_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetParsedSceneResetApi=_SiN_GetParsedSceneResetApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ParsedSceneResetApi* __cdecl
SiN_GetParsedSceneResetApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ParsedSceneResetApi)?&parsed_scene_reset_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetChoreographyVectorApi=_SiN_GetChoreographyVectorApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ChoreographyVectorApi* __cdecl
SiN_GetChoreographyVectorApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ChoreographyVectorApi)?&choreography_vector_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetAllocatedGeneratedSceneApi=_SiN_GetAllocatedGeneratedSceneApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::AllocatedGeneratedSceneApi* __cdecl
SiN_GetAllocatedGeneratedSceneApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(AllocatedGeneratedSceneApi)?&allocated_generated_scene_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetChoreographyConstructApi=_SiN_GetChoreographyConstructApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ChoreographyConstructApi* __cdecl
SiN_GetChoreographyConstructApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ChoreographyConstructApi)?&choreography_construct_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetGeneratedSceneApi=_SiN_GetGeneratedSceneApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::GeneratedSceneApi* __cdecl
SiN_GetGeneratedSceneApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(GeneratedSceneApi)?&generated_scene_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetNativeGeneratedMapDialogueApi=_SiN_GetNativeGeneratedMapDialogueApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::NativeGeneratedMapDialogueApi* __cdecl
SiN_GetNativeGeneratedMapDialogueApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(NativeGeneratedMapDialogueApi)?&native_generated_map_dialogue_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetIntegratedNpcMapDialogueApi=_SiN_GetIntegratedNpcMapDialogueApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::IntegratedNpcMapDialogueApi* __cdecl
SiN_GetIntegratedNpcMapDialogueApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(IntegratedNpcMapDialogueApi)?&integrated_npc_map_dialogue_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetIntegratedNpcDialogueApi=_SiN_GetIntegratedNpcDialogueApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::IntegratedNpcDialogueApi* __cdecl
SiN_GetIntegratedNpcDialogueApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(IntegratedNpcDialogueApi)?&integrated_npc_dialogue_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetIntegratedResponseApi=_SiN_GetIntegratedResponseApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::IntegratedResponseApi* __cdecl
SiN_GetIntegratedResponseApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(IntegratedResponseApi)?&integrated_response_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetIntegratedScenePlaybackApi=_SiN_GetIntegratedScenePlaybackApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::IntegratedScenePlaybackApi* __cdecl
SiN_GetIntegratedScenePlaybackApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(IntegratedScenePlaybackApi)?&integrated_scene_playback_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetSceneAdvanceApi=_SiN_GetSceneAdvanceApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::SceneAdvanceApi* __cdecl
SiN_GetSceneAdvanceApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(SceneAdvanceApi)?&scene_advance_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetParsedSceneSimulationApi=_SiN_GetParsedSceneSimulationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ParsedSceneSimulationApi* __cdecl
SiN_GetParsedSceneSimulationApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ParsedSceneSimulationApi)?&parsed_scene_simulation_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetParsedEventClassifyApi=_SiN_GetParsedEventClassifyApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ParsedEventClassifyApi* __cdecl
SiN_GetParsedEventClassifyApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ParsedEventClassifyApi)?&parsed_event_classify_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetParsedSceneResumeApi=_SiN_GetParsedSceneResumeApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ParsedSceneResumeApi* __cdecl
SiN_GetParsedSceneResumeApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ParsedSceneResumeApi)?&parsed_scene_resume_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetSceneEntityLifecycleApi=_SiN_GetSceneEntityLifecycleApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::SceneEntityLifecycleApi* __cdecl
SiN_GetSceneEntityLifecycleApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(SceneEntityLifecycleApi)?&scene_entity_lifecycle_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetSceneRegistrationApi=_SiN_GetSceneRegistrationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::SceneRegistrationApi* __cdecl
SiN_GetSceneRegistrationApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(SceneRegistrationApi)?&scene_registration_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetSceneManagerTickApi=_SiN_GetSceneManagerTickApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::SceneManagerTickApi* __cdecl
SiN_GetSceneManagerTickApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(SceneManagerTickApi)?&scene_manager_tick_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetSceneStateApi=_SiN_GetSceneStateApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::SceneStateApi* __cdecl
SiN_GetSceneStateApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(SceneStateApi)?&scene_state_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetResponseChoreographyApi=_SiN_GetResponseChoreographyApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ResponseChoreographyApi* __cdecl
SiN_GetResponseChoreographyApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(ResponseChoreographyApi)?&response_choreography_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetSceneManagerRuntimeApi=_SiN_GetSceneManagerRuntimeApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::SceneManagerRuntimeApi* __cdecl
SiN_GetSceneManagerRuntimeApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(SceneManagerRuntimeApi)?&scene_manager_runtime_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetSceneManagerQueryApi=_SiN_GetSceneManagerQueryApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::SceneManagerQueryApi* __cdecl
SiN_GetSceneManagerQueryApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(SceneManagerQueryApi)?&scene_manager_query_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetNpcScenePlaybackApi=_SiN_GetNpcScenePlaybackApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::NpcScenePlaybackApi* __cdecl
SiN_GetNpcScenePlaybackApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(NpcScenePlaybackApi)?&npc_scene_playback_api:nullptr;
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

#pragma comment(linker,"/EXPORT:SiN_GetCompanionReactionApi=_SiN_GetCompanionReactionApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::CompanionReactionApi* __cdecl
SiN_GetCompanionReactionApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(CompanionReactionApi)?&companion_reaction_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetCountSchedulerApi=_SiN_GetCountSchedulerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::CountSchedulerApi* __cdecl
SiN_GetCountSchedulerApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(CountSchedulerApi)?&count_scheduler_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetUseObservationApi=_SiN_GetUseObservationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::UseObservationApi* __cdecl
SiN_GetUseObservationApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(UseObservationApi)?&use_observation_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetVehicleUseApi=_SiN_GetVehicleUseApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::VehicleUseApi* __cdecl
SiN_GetVehicleUseApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(VehicleUseApi)?&vehicle_use_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetVehicleActionsApi=_SiN_GetVehicleActionsApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::VehicleActionsApi* __cdecl
SiN_GetVehicleActionsApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(VehicleActionsApi)?&vehicle_actions_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetMusicRadioApi=_SiN_GetMusicRadioApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::MusicRadioApi* __cdecl
SiN_GetMusicRadioApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(MusicRadioApi)?&music_radio_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetMusicRadioMapApi=_SiN_GetMusicRadioMapApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::MusicRadioMapApi* __cdecl
SiN_GetMusicRadioMapApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(MusicRadioMapApi)?&music_map_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetVehicleSoundApi=_SiN_GetVehicleSoundApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::VehicleSoundApi* __cdecl
SiN_GetVehicleSoundApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(VehicleSoundApi)?&vehicle_sound_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetVehicleHintApi=_SiN_GetVehicleHintApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::VehicleHintApi* __cdecl
SiN_GetVehicleHintApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(VehicleHintApi)?&vehicle_hint_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetVehicleViewsApi=_SiN_GetVehicleViewsApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::VehicleViewsApi* __cdecl
SiN_GetVehicleViewsApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(VehicleViewsApi)?&vehicle_views_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetVehicleConfigApi=_SiN_GetVehicleConfigApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::VehicleConfigApi* __cdecl
SiN_GetVehicleConfigApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(VehicleConfigApi)?&vehicle_config_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetVehiclePassengersApi=_SiN_GetVehiclePassengersApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::VehiclePassengersApi* __cdecl
SiN_GetVehiclePassengersApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(VehiclePassengersApi)?&vehicle_passengers_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetVehicleMapApi=_SiN_GetVehicleMapApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::VehicleMapApi* __cdecl
SiN_GetVehicleMapApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(VehicleMapApi)?&vehicle_map_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetVehicleLifecycleApi=_SiN_GetVehicleLifecycleApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::VehicleLifecycleApi* __cdecl
SiN_GetVehicleLifecycleApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(VehicleLifecycleApi)?&vehicle_lifecycle_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetVehicleThinkApi=_SiN_GetVehicleThinkApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::VehicleThinkApi* __cdecl
SiN_GetVehicleThinkApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(VehicleThinkApi)?&vehicle_think_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetVehicleSeatEventsApi=_SiN_GetVehicleSeatEventsApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::VehicleSeatEventsApi* __cdecl
SiN_GetVehicleSeatEventsApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(VehicleSeatEventsApi)?&vehicle_seat_events_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetCameraLinkApi=_SiN_GetCameraLinkApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::CameraLinkApi* __cdecl
SiN_GetCameraLinkApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(CameraLinkApi)?&camera_link_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetVehiclePartEventsApi=_SiN_GetVehiclePartEventsApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::VehiclePartEventsApi* __cdecl
SiN_GetVehiclePartEventsApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(VehiclePartEventsApi)?&vehicle_part_events_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetVehicleNestedApi=_SiN_GetVehicleNestedApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::VehicleNestedApi* __cdecl
SiN_GetVehicleNestedApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(VehicleNestedApi)?&vehicle_nested_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetNpcMapApi=_SiN_GetNpcMapApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::NpcMapApi* __cdecl
SiN_GetNpcMapApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(NpcMapApi)?&npc_map_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetNpcSpawnApi=_SiN_GetNpcSpawnApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::NpcSpawnApi* __cdecl
SiN_GetNpcSpawnApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(NpcSpawnApi)?&npc_spawn_api:nullptr;
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

#pragma comment(linker,"/EXPORT:SiN_GetHintHudApi=_SiN_GetHintHudApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::HintHudApi* __cdecl
SiN_GetHintHudApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(HintHudApi)?&hint_hud_api:nullptr;
}
#pragma comment(linker,"/EXPORT:SiN_GetPoisonHudAnimationApi=_SiN_GetPoisonHudAnimationApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::PoisonHudAnimationApi* __cdecl
SiN_GetPoisonHudAnimationApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==sin_port::native_weapon_tables::version&&size==sizeof(PoisonHudAnimationApi)?&poison_hud_animation_api:nullptr;
}
#endif
