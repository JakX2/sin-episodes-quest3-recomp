#include "server_fixed_difficulty_service.h"
#include "server_attack_spread_rules.h"
#include "sin_compat/fixed_difficulty_phase663.hpp"
#include "sin_compat/info_challenge_phase665.hpp"
#include "sin_compat/fixed_difficulty_trigger_phase663.hpp"
#include <memory>
#include <utility>
#include <cmath>
#include <optional>
#include <vector>
#include <string>
#include <cstring>
#include <map>
#include <charconv>
#include <limits>
#if defined(_MSC_VER) && defined(_M_IX86)
#include "client_known_gamesystem_init.h"
#include "server_single_cvar_probe.h"
#endif

namespace abi=sin_port::fixed_service;
namespace diff=sin_compat::difficulty;
namespace fixed=diff::fixed_phase663;
namespace challenge=diff::info_challenge_phase665;
namespace triggers=sin_compat::map_entities;
namespace {
fixed::FixedDifficultyRuntime make_policy(int skill) noexcept {
    fixed::FixedDifficultyRuntime policy;
    (void)policy.set_skill(skill);
    return policy;
}
struct Campaign {
    explicit Campaign(int skill) noexcept:
        policy(make_policy(skill)), initial_skill(skill), values(policy) {}
    Campaign(const Campaign&)=delete;
    Campaign& operator=(const Campaign&)=delete;
    fixed::FixedDifficultyRuntime policy;
    int initial_skill;
    challenge::FixedChallengeValues values;
};
// Module-owned lifetime, not an address in an original PE image.
std::optional<Campaign> campaign;
// Refuse lifecycle/skill mutations reentered by a synchronous engine lookup.
bool reading_engine_skill=false;
#if defined(_MSC_VER) && defined(_M_IX86)
bool trigger_publish_active=false;
bool native_event_active=false;
struct TriggerPublishScope {
    TriggerPublishScope() noexcept {trigger_publish_active=true;}
    ~TriggerPublishScope(){trigger_publish_active=false;}
};
struct NativeEventScope {
    bool prior=reading_engine_skill;
    NativeEventScope() noexcept {reading_engine_skill=true;native_event_active=true;}
    ~NativeEventScope(){native_event_active=false;reading_engine_skill=prior;}
};
#endif
// Map-local behavior objects, not actual CBaseEntity allocations.
enum class EntityKind { EnvGlobal, InfoChallenge, DynamicTrigger, SecretTrigger };
struct MapEntity {
    std::uint32_t generation=0;
    EntityKind kind=EntityKind::EnvGlobal;
    bool alive=true,spawned=false,remove_requested=false;
    std::optional<std::string> global_name;
    std::uint32_t flags=0;
    std::int32_t initial_state=0;
    float snapshot=0.f;
};
struct MapLevel {
    std::uint32_t epoch=0;
    std::string name;
    std::map<std::uint32_t,MapEntity> entities;
};
std::optional<MapLevel> level;
std::uint32_t last_epoch=0;
constexpr std::size_t max_map_entities=8192;
// Original CGlobalState globalentity_t has two 16-bit symbol indices
// and a 32-bit state. Preserve observable name/level/state semantics,
// using native owned strings instead of original allocator/tree storage.
struct GlobalRecord {
    std::string name,level;
    std::int32_t value;
};
std::vector<GlobalRecord> globals;
bool global_updates_disabled=false;
constexpr std::size_t max_global_string=4096, max_global_records=65535;
bool bounded_string(const char* s) noexcept {
    if(!s)return false;
    for(std::size_t i=0;i<max_global_string;++i)if(s[i]=='\0')return true;
    return false;
}
int SIN_FIXED_CALL global_count() noexcept {return static_cast<int>(globals.size());}
int SIN_FIXED_CALL global_find(const char* name) noexcept {
    if(!bounded_string(name))return -1;
    for(std::size_t i=0;i<globals.size();++i)
        if(globals[i].name==name)return static_cast<int>(i);
    return -1;
}
bool valid_global_index(int index) noexcept {
    return index>=0&&static_cast<std::size_t>(index)<globals.size();
}
int SIN_FIXED_CALL global_add(const char* name,const char* level,std::int32_t value) noexcept {
    if(reading_engine_skill||!bounded_string(name)||!bounded_string(level))return -1;
    const int prior=global_find(name);
    if(prior>=0)return prior; // original duplicate returns prior index, unchanged
    if(globals.size()>=max_global_records)return -1;
    try {globals.push_back({name,level,value});}
    catch(...) {return -1;}
    return global_count()-1;
}
int SIN_FIXED_CALL global_read(int index,char* name,std::uint32_t name_bytes,
    char* level,std::uint32_t level_bytes,std::int32_t* value) noexcept {
    if(!valid_global_index(index)||!name||!level||!value)return 0;
    const auto& item=globals[static_cast<std::size_t>(index)];
    if(name_bytes<=item.name.size()||level_bytes<=item.level.size())return 0;
    std::memcpy(name,item.name.c_str(),item.name.size()+1);
    std::memcpy(level,item.level.c_str(),item.level.size()+1);
    *value=item.value;
    return 1;
}
int SIN_FIXED_CALL global_set_state(int index,std::int32_t value) noexcept {
    if(reading_engine_skill||global_updates_disabled||!valid_global_index(index))return 0;
    globals[static_cast<std::size_t>(index)].value=value;
    return 1;
}
int SIN_FIXED_CALL global_set_level(int index,const char* level) noexcept {
    if(reading_engine_skill||!valid_global_index(index)||!bounded_string(level?level:""))return 0;
    try {globals[static_cast<std::size_t>(index)].level=level?level:"";}
    catch(...) {return 0;}
    return 1;
}
int SIN_FIXED_CALL global_disable(int disable) noexcept {
    if(reading_engine_skill||(disable!=0&&disable!=1))return 0;
    global_updates_disabled=disable!=0;
    return 1;
}
void clear_global_storage() noexcept {
    // Original reset does not touch the state-update disable byte.
    // Independently owned records and strings can be released together.
    std::vector<GlobalRecord>().swap(globals);
}
int SIN_FIXED_CALL global_clear() noexcept {
    if(reading_engine_skill)return 0;
    clear_global_storage();
    return 1;
}
const abi::GlobalStateApi global_api{abi::global_state_version,sizeof(abi::GlobalStateApi),
    &global_count,&global_find,&global_add,&global_read,&global_set_state,
    &global_set_level,&global_disable,&global_clear};
int SIN_FIXED_CALL begin(int skill) noexcept {
    if(reading_engine_skill || campaign || !fixed::valid_skill(skill))return 0;
    campaign.emplace(skill);
    return 1;
}
int SIN_FIXED_CALL end() noexcept {
    if(reading_engine_skill || !campaign || level)return 0;
    campaign.reset();
    return 1;
}
int SIN_FIXED_CALL select(int skill) noexcept {
    if(reading_engine_skill || !campaign)return 0;
    return campaign->policy.set_skill(skill)!=fixed::Selection::InvalidSkill;
}
int SIN_FIXED_CALL state(abi::State* out) noexcept {
    if(!campaign||!out)return 0;
    const auto& p=campaign->policy;
    *out={static_cast<std::uint32_t>(p.skill()),
          static_cast<std::uint32_t>(campaign->initial_skill),p.revision(),
          p.trigger_count(),p.playthrough_completed()?1u:0u,
          p.adaptive_updates_enabled()?1u:0u};
    return 1;
}
int SIN_FIXED_CALL metric(std::uint32_t index,float* out) noexcept {
    if(!campaign||!out||index>=abi::metric_count)return 0;
    *out=campaign->policy.metric_at(index);
    return 1;
}
int SIN_FIXED_CALL damage_taken(float amount,std::uint32_t type,float* out) noexcept {
    if(!campaign||!out||!std::isfinite(amount))return 0;
    diff::OriginalDamageFields damage{amount,type};
    diff::apply_damage_taken_metric(damage,campaign->policy.metrics());
    if(!std::isfinite(damage.damage))return 0;
    *out=damage.damage;
    return 1;
}
int SIN_FIXED_CALL damage_done(float amount,float* out) noexcept {
    if(!campaign||!out||!std::isfinite(amount))return 0;
    const float scaled=diff::scaled_damage_done(amount,campaign->policy.metrics());
    if(!std::isfinite(scaled))return 0;
    *out=scaled;
    return 1;
}
int SIN_FIXED_CALL shoot_delay(float base,float* out) noexcept {
    if(!campaign||!out||!std::isfinite(base))return 0;
    const float delay=diff::npc_random_wait_before_shoot(base,campaign->policy.metrics());
    if(!std::isfinite(delay))return 0;
    *out=delay;
    return 1;
}
int SIN_FIXED_CALL trigger(std::int32_t difficulty,float time,std::int32_t eog) noexcept {
    if(reading_engine_skill||!campaign||!std::isfinite(time))return 0;
    campaign->policy.observe_dynamic_trigger({difficulty,time,eog});
    return 1;
}
int SIN_FIXED_CALL capture(float* out) noexcept {
    if(!campaign||!out)return 0;
    return campaign->values.read_spawn_value(*out)?1:0;
}
class Output final:public challenge::IChallengeOutput {
public:
    bool fire_float(std::uint32_t offset,std::string_view,float value,
                    std::uint32_t activator,std::uint32_t caller) override {
        record={offset,activator,caller,value};
        return true;
    }
    abi::ChallengeOutput record{};
};
int SIN_FIXED_CALL query(std::uint32_t input,float snapshot,std::uint32_t self,
                         abi::ChallengeOutput* out) noexcept {
    if(!campaign||!out||input>abi::Difference)return 0;
    Output sink;
    challenge::InfoChallenge entity(self,campaign->values,sink);
    if(input==abi::Difference){
        if(snapshot!=0.f&&snapshot!=0.5f&&snapshot!=1.f)return 0;
        if(!entity.restore_snapshot({snapshot,true}))return 0;
    }
    const auto result=entity.accept(static_cast<challenge::Input>(input));
    if(result.code!=challenge::Code::Completed)return 0;
    *out=sink.record;
    return 1;
}

// Original 0x2239fa00 checks the persistent global before the life state.
// Missing names resolve to state 0. Dead state 2 is not invulnerability.
int SIN_FIXED_CALL player_gate(std::uint32_t life_state) noexcept {
    if(!campaign)return abi::PlayerDamageRejected;
    const int index=global_find("gordon_invulnerable");
    if(index>=0&&globals[static_cast<std::size_t>(index)].value==1)
        return abi::PlayerDamageInvulnerable;
    if(life_state!=0)return abi::PlayerDamageNotAlive;
    return abi::PlayerDamageReady;
}
int SIN_FIXED_CALL prepare_player_copy(const abi::PlayerDamageRecord* input,
        const abi::PlayerDamageObservation* observed,abi::PlayerDamageRecord* output) noexcept {
    if(!input||!observed||!output||observed->attacker_is_self>1||observed->inflictor_is_self>1)
        return abi::PlayerDamageRejected;
    const int gate=player_gate(observed->life_state);
    if(gate!=abi::PlayerDamageReady)return gate;
    if(!std::isfinite(input->amount))return abi::PlayerDamageRejected;
    auto copy=*input;
    // The original tests the resolved attacker and inflictor entity
    // pointers against self, not just raw EHANDLE values. In either self
    // case it skips the entire original 0x221f8770 helper.
    if(!observed->attacker_is_self&&!observed->inflictor_is_self){
        copy.base_damage=copy.amount;
        if(!damage_taken(copy.amount,copy.damage_type,&copy.amount))
            return abi::PlayerDamageRejected;
    }
    *output=copy;
    return abi::PlayerDamageReady;
}
// Original 0x222a7c51..0x222a7d13. The f32 fractional remainder is
// added to the prior per-character carry; an x87 comparison against
// 1.0 precedes the carry subtraction. Use double intermediates for
// the normal finite nonnegative input domain, then f32 stores.
int SIN_FIXED_CALL common_health_stage(const abi::CommonDamageInput* input,
        abi::CommonDamageOutput* out) noexcept {
    if(!input||!out)return abi::CommonDamageRejected;
    const auto in=*input;
    if(!std::isfinite(in.amount)||in.amount<0.f||
       !std::isfinite(in.fraction)||in.fraction<0.f||in.fraction>1.f)
        return abi::CommonDamageRejected;
    abi::CommonDamageOutput result{in.health,0,in.fraction,0,0};
    if(in.take_damage==1){*out=result;return abi::CommonDamageEventsOnly;}
    const double amount=static_cast<double>(in.amount);
    const float remainder=static_cast<float>(amount-std::floor(amount));
    double whole=amount-static_cast<double>(remainder);
    const double combined=static_cast<double>(in.fraction)+static_cast<double>(remainder);
    double carry=combined;
    if(combined>=1.0){whole+=1.0;carry-=1.0;}
    if(whole<0.0||whole>static_cast<double>((std::numeric_limits<std::int32_t>::max)()))
        return abi::CommonDamageRejected;
    const auto reduction=static_cast<std::int32_t>(whole);
    const auto health=static_cast<std::int64_t>(in.health)-reduction;
    if(health<(std::numeric_limits<std::int32_t>::min)())return abi::CommonDamageRejected;
    result.health=static_cast<std::int32_t>(health);
    result.integer_damage=reduction;
    result.fraction=static_cast<float>(carry);
    result.fraction_write=1;
    result.health_changed=reduction!=0?1u:0u;
    *out=result;
    return reduction?abi::CommonDamageReduced:abi::CommonDamageFractionOnly;
}
const abi::CombatDamageApi combat_api{abi::combat_version,sizeof(abi::CombatDamageApi),
    &player_gate,&prepare_player_copy,&common_health_stage};


struct NpcRandomScope {
    NpcRandomScope() noexcept {reading_engine_skill=true;}
    ~NpcRandomScope(){reading_engine_skill=false;}
};
int SIN_FIXED_CALL npc_decide(std::uint32_t kind,float factor,abi::NpcRandomFloat random,
        void* context,abi::NpcDecisionOutput* out) noexcept {
    if(reading_engine_skill||!campaign||!out)return 0;
    const auto& m=campaign->policy.metrics();
    float threshold=0.f;
    switch(kind){
    case abi::NpcSecondaryFire:threshold=m[5];break;
    case abi::NpcWearsHelmet:threshold=m[8];break;
    case abi::NpcHelmetShotOff:threshold=m[9];break;
    case abi::NpcTakesCover:threshold=m[10];break;
    case abi::NpcMobile:
        if(!std::isfinite(factor))return 0;
        threshold=static_cast<float>(static_cast<double>(m[13])*factor*static_cast<double>(0.2f));
        break;
    case abi::NpcSpawnCountDelta:threshold=m[7];break;
    case abi::NpcHealthKitDrop:
        // Fixed profiles keep forced health kits at zero. The original
        // slot97's nonzero forced-counter mutation is outside this ABI.
        if(m[3]!=0.f)return 0;
        threshold=m[4];break;
    default:return 0;
    }
    if(!std::isfinite(threshold))return 0;
    abi::NpcDecisionOutput result{std::fabs(threshold),0.f,0,0};
    if(threshold==0.f){*out=result;return 1;}
    if(!random)return 0;
    float sample=0.f;
    {
        NpcRandomScope scope;
        try {if(!random(context,0.f,1.f,&sample))return 0;}
        catch(...){return 0;}
    }
    if(!std::isfinite(sample)||sample<0.f||sample>1.f)return 0;
    result.sample=sample;result.drew_random=1;
    // Original FCOMPP compares abs(metric) to random; C0 is the
    // failure branch. Equality succeeds, and >1 is not clamped.
    if(sample<=result.threshold)
        result.decision=(kind==abi::NpcSpawnCountDelta&&threshold<0.f)?-1:1;
    *out=result;
    return 1;
}
int SIN_FIXED_CALL npc_spread(float base,abi::NpcSpread* out) noexcept {
    if(!campaign||!out||!std::isfinite(base)||base<0.f)return 0;
    const float spread=base*campaign->policy.metric_at(12);
    if(!std::isfinite(spread))return 0;
    *out={spread,spread,spread};
    return 1;
}
const abi::NpcRulesApi npc_api{abi::npc_rules_version,sizeof(abi::NpcRulesApi),&npc_decide,&npc_spread};

sin_independent::attack_spread::Rules spread_rules;
int SIN_FIXED_CALL load_spread_rules(const char* text,std::uint32_t size) noexcept {
    if(reading_engine_skill||!text||!size||size>sin_independent::attack_spread::max_bytes)return 0;
    try {return spread_rules.load(std::string_view(text,size))?1:0;}catch(...){return 0;}
}
int SIN_FIXED_CALL clear_spread_rules() noexcept {
    if(reading_engine_skill)return 0;
    spread_rules.clear();return 1;
}
int SIN_FIXED_CALL query_spread_rules(const char* npc,const char* weapon,const char* target,
    abi::AttackSpreadResult* out) noexcept {
    if(reading_engine_skill||!campaign||!out||(npc&&!bounded_string(npc))||
       (weapon&&!bounded_string(weapon))||(target&&!bounded_string(target)))return 0;
    float base=0.f;
    const bool found=spread_rules.resolve(npc?npc:"*",weapon?weapon:"*",target?target:"*",base);
    abi::NpcSpread scaled{};
    if(!npc_spread(base,&scaled))return 0;
    // Recompute on query rather than retaining stale scaled cache entries
    // when this port changes skill at runtime.
    *out={base,scaled.x,scaled.y,scaled.z,found?0u:1u};
    return 1;
}
const abi::AttackSpreadApi attack_spread_api{abi::attack_spread_version,
    sizeof(abi::AttackSpreadApi),&load_spread_rules,&clear_spread_rules,&query_spread_rules};

static_assert(diff::original_metric_count==abi::metric_count);
static_assert(static_cast<std::uint32_t>(challenge::Input::Initial)==abi::Initial);
static_assert(static_cast<std::uint32_t>(challenge::Input::Current)==abi::Current);
static_assert(static_cast<std::uint32_t>(challenge::Input::Difference)==abi::Difference);
const abi::Api api{abi::abi_version,sizeof(abi::Api),abi::metric_count,
    &begin,&end,&select,&state,&metric,&damage_taken,&damage_done,&shoot_delay,
    &trigger,&capture,&query};
}
// MSVC Win32 cdecl symbols have a leading underscore. Explicit alias
// guarantees a stable, undecorated GetProcAddress name on the real DLL.
#if defined(_MSC_VER) && defined(_M_IX86)
#pragma comment(linker,"/EXPORT:SiN_GetFixedDifficultyApi=_SiN_GetFixedDifficultyApi")
#pragma comment(linker,"/EXPORT:SiN_GetCombatDamageApi=_SiN_GetCombatDamageApi")
#pragma comment(linker,"/EXPORT:SiN_GetNpcRulesApi=_SiN_GetNpcRulesApi")
#pragma comment(linker,"/EXPORT:SiN_GetAttackSpreadApi=_SiN_GetAttackSpreadApi")
#pragma comment(linker,"/EXPORT:SiN_GetGlobalStateApi=_SiN_GetGlobalStateApi")
#pragma comment(linker,"/EXPORT:SiN_EnvGlobalDispatch=_SiN_EnvGlobalDispatch")
#pragma comment(linker,"/EXPORT:SiN_GetMapEntityApi=_SiN_GetMapEntityApi")
#pragma comment(linker,"/EXPORT:SiN_GetCampaignTriggerApi=_SiN_GetCampaignTriggerApi")
#define SIN_FIXED_EXPORT
#elif defined(_WIN32)
#define SIN_FIXED_EXPORT __declspec(dllexport)
#else
#define SIN_FIXED_EXPORT __attribute__((visibility("default")))
#endif
extern "C" SIN_FIXED_EXPORT const abi::Api* SIN_FIXED_CALL
SiN_GetFixedDifficultyApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==abi::abi_version&&bytes==sizeof(abi::Api)?&api:nullptr;
}

extern "C" SIN_FIXED_EXPORT const abi::AttackSpreadApi* SIN_FIXED_CALL
SiN_GetAttackSpreadApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==abi::attack_spread_version&&bytes==sizeof(abi::AttackSpreadApi)?&attack_spread_api:nullptr;
}

extern "C" SIN_FIXED_EXPORT const abi::NpcRulesApi* SIN_FIXED_CALL
SiN_GetNpcRulesApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==abi::npc_rules_version&&bytes==sizeof(abi::NpcRulesApi)?&npc_api:nullptr;
}

extern "C" SIN_FIXED_EXPORT const abi::CombatDamageApi* SIN_FIXED_CALL
SiN_GetCombatDamageApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==abi::combat_version&&bytes==sizeof(abi::CombatDamageApi)?&combat_api:nullptr;
}

extern "C" SIN_FIXED_EXPORT const abi::GlobalStateApi* SIN_FIXED_CALL
SiN_GetGlobalStateApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==abi::global_state_version&&bytes==sizeof(abi::GlobalStateApi)?&global_api:nullptr;
}

// Original CEnvGlobal Spawn and four input methods. The host supplies
// resolved string_t values, not original object addresses. Null global
// name on Spawn requests entity removal; on inputs it becomes "".
extern "C" SIN_FIXED_EXPORT int SIN_FIXED_CALL
SiN_EnvGlobalDispatch(std::uint32_t op,const char* name,const char* level,
                     std::uint32_t flags,std::int32_t initial_state) noexcept {
    if(reading_engine_skill||op>abi::EnvToggle)return abi::EnvRejected;
    if(op==abi::EnvSpawn&&!name)return abi::EnvRemoveSelf;
    name=name?name:"";
    level=level?level:"";
    if(!bounded_string(name))return abi::EnvRejected;
    const int index=global_find(name);
    if(op==abi::EnvSpawn) {
        if(!(flags&1u)||index>=0)return abi::EnvUnchanged;
        return global_add(name,level,initial_state)>=0?abi::EnvApplied:abi::EnvRejected;
    }
    int next=0;
    switch(op) {
    case abi::EnvTurnOn:next=1;break;
    case abi::EnvTurnOff:next=0;break;
    case abi::EnvRemove:next=2;break;
    case abi::EnvToggle:{
        const auto current=index>=0?globals[static_cast<std::size_t>(index)].value:0;
        if(current!=0&&current!=1)return abi::EnvUnchanged;
        next=current==0?1:0;
        break;
    }
    default:return abi::EnvRejected;
    }
    // Original missing entries are added even with the update gate set.
    if(index<0)return global_add(name,level,next)>=0?abi::EnvApplied:abi::EnvRejected;
    if(global_updates_disabled)return abi::EnvBlocked;
    return global_set_state(index,next)?abi::EnvApplied:abi::EnvRejected;
}


namespace {
bool active_level(std::uint32_t epoch) noexcept {return level&&epoch&&level->epoch==epoch;}
MapEntity* map_find(abi::MapEntityRef ref) noexcept {
    if(!active_level(ref.epoch))return nullptr;
    const auto it=level->entities.find(ref.index);
    if(it==level->entities.end()||!it->second.alive||it->second.generation!=ref.generation)return nullptr;
    return &it->second;
}

bool is_trigger(EntityKind kind) noexcept {
    return kind==EntityKind::DynamicTrigger||kind==EntityKind::SecretTrigger;
}
sin_compat::EntityRef trigger_ref(abi::MapEntityRef e) noexcept {return {e.index,e.generation};}
class TriggerBridge final:public triggers::IOriginalTriggerHost {
public:
    const abi::NativeTriggerHost* callbacks=nullptr;
    void* context=nullptr;
    abi::MapEntityRef active{};
    abi::MapEntityRef ref(sin_compat::EntityRef e) const noexcept {
        return {level->epoch,e.index,e.generation};
    }
    bool is_live(sin_compat::EntityRef e) const override {
        // Attach/configure use only the DLL registry. Runtime callbacks
        // use the caller's actual host liveness predicate, including actors.
        return callbacks?callbacks->is_live(context,ref(e))!=0:map_find(ref(e))!=nullptr;
    }
    bool initialize_native_base_trigger(sin_compat::EntityRef e) override {
        return callbacks&&callbacks->base_spawn(context,ref(e))!=0;
    }
    void report_original_warning(std::string_view text) override {
        const std::string owned(text);
        callbacks->warning(context,owned.c_str());
    }
    bool is_player(sin_compat::EntityRef e) const override {
        return callbacks&&callbacks->is_player(context,ref(e))!=0;
    }
    int current_difficulty_owner_index() const override {
        return callbacks?callbacks->owner_index(context):-1;
    }
    bool perform_special_trigger_base_action(sin_compat::EntityRef e) override {
        return callbacks&&callbacks->special_action(context,ref(e))!=0;
    }
    bool publish(const triggers::TriggerEvent& e) override {
        const std::string name(e.name);
        const abi::NativeTriggerEvent event{
            e.event==triggers::TriggerEventName::DynamicDifficulty?abi::TriggerDynamicEvent:abi::TriggerGameEvent,
            e.descriptor,e.owner,e.advisor,e.decision,e.value,e.origin.x,e.origin.y,e.origin.z,name.c_str()};
#if defined(_MSC_VER) && defined(_M_IX86)
        TriggerPublishScope scope;
#endif
        return callbacks&&callbacks->publish(context,active,&event)!=0;
    }
    bool mark_difficulty_manager() override {return false;} // fixed wrapper owns this
    bool queue_remove(sin_compat::EntityRef e) override {
        return callbacks&&callbacks->queue_remove(context,ref(e))!=0;
    }
};
struct TriggerRuntime {
    explicit TriggerRuntime(fixed::FixedDifficultyRuntime& policy) noexcept:
        fixed_host(bridge,policy),factory(fixed_host){}
    TriggerBridge bridge;
    fixed::FixedDifficultyTriggerHost fixed_host;
    triggers::OriginalTriggerEntityFactory factory;
};
std::unique_ptr<TriggerRuntime> trigger_runtime;
bool attach_trigger(abi::MapEntityRef ref,const char* classname) noexcept {
    try {
        if(!trigger_runtime)trigger_runtime=std::make_unique<TriggerRuntime>(campaign->policy);
        return trigger_runtime->factory.spawn({classname,trigger_ref(ref)}).ok();
    }catch(...){return false;}
}
bool valid_trigger_host(const abi::NativeTriggerHost* host) noexcept {
    return host&&host->version==abi::trigger_version&&host->size==sizeof(*host)&&
        host->is_live&&host->is_player&&host->base_spawn&&host->warning&&host->owner_index&&
        host->special_action&&host->publish&&host->queue_remove;
}
struct TriggerCallbackScope {
    TriggerBridge& bridge;
    TriggerCallbackScope(TriggerBridge& b,abi::MapEntityRef ref,
                         const abi::NativeTriggerHost* cb,void* context) noexcept:bridge(b) {
        reading_engine_skill=true;
        b.active=ref;b.callbacks=cb;b.context=context;
    }
    ~TriggerCallbackScope(){bridge.callbacks=nullptr;bridge.context=nullptr;bridge.active={};reading_engine_skill=false;}
};
static_assert(static_cast<std::uint32_t>(triggers::TriggerCode::Handled)==abi::TriggerHandled);
static_assert(static_cast<std::uint32_t>(triggers::TriggerCode::NonPlayer)==abi::TriggerNonPlayer);
static_assert(static_cast<std::uint32_t>(triggers::TriggerCode::AlreadyConsumed)==abi::TriggerAlreadyConsumed);
static_assert(static_cast<std::uint32_t>(triggers::TriggerCode::UnknownEntity)==abi::TriggerUnknownEntity);
static_assert(static_cast<std::uint32_t>(triggers::TriggerCode::InvalidActor)==abi::TriggerInvalidActor);
static_assert(static_cast<std::uint32_t>(triggers::TriggerCode::InvalidConfig)==abi::TriggerInvalidConfig);
static_assert(static_cast<std::uint32_t>(triggers::TriggerCode::UnknownKey)==abi::TriggerUnknownKey);
static_assert(static_cast<std::uint32_t>(triggers::TriggerCode::HostNotLive)==abi::TriggerHostNotLive);
static_assert(static_cast<std::uint32_t>(triggers::TriggerCode::HostRefused)==abi::TriggerHostRefused);
static_assert(static_cast<std::uint32_t>(triggers::TriggerCode::EventUnavailable)==abi::TriggerEventUnavailable);
static_assert(static_cast<std::uint32_t>(triggers::TriggerCode::ManagerRefused)==abi::TriggerManagerRefused);
static_assert(static_cast<std::uint32_t>(triggers::TriggerCode::RemovalPending)==abi::TriggerRemovalPending);
static_assert(static_cast<std::uint32_t>(triggers::TriggerCode::WrongClass)==abi::TriggerWrongClass);
static_assert(static_cast<std::uint32_t>(triggers::TriggerCode::Reentrant)==abi::TriggerReentrant);
std::uint32_t trigger_code(triggers::TriggerCode code) noexcept {return static_cast<std::uint32_t>(code);}
int SIN_FIXED_CALL native_trigger_spawn(abi::MapEntityRef ref,const abi::NativeTriggerSpawn* input,
        const abi::NativeTriggerHost* host,void* context,abi::NativeTriggerReport* out) noexcept {
    auto* entity=map_find(ref);
    if(reading_engine_skill||!entity||!is_trigger(entity->kind)||!trigger_runtime||
       !input||input->engine_requests_removal>1||!out||!valid_trigger_host(host))return 0;
    if(entity->spawned||entity->remove_requested){
        *out={abi::TriggerAlreadyConsumed,0,0,0,0,0};return 1;
    }
    TriggerCallbackScope scope(trigger_runtime->bridge,ref,host,context);
    const triggers::TriggerSpawnInput fields{input->engine_requests_removal!=0,input->flags_120,input->spawnflags_b4};
    const auto report=entity->kind==EntityKind::DynamicTrigger?
        trigger_runtime->factory.run_dynamic_spawn(trigger_ref(ref),fields):
        trigger_runtime->factory.run_secret_spawn(trigger_ref(ref),fields);
    if(report.base_initialized)entity->spawned=true;
    if(fields.engine_requests_removal&&(report.code==triggers::TriggerCode::Handled||
        report.code==triggers::TriggerCode::RemovalPending))entity->remove_requested=true;
    *out={trigger_code(report.code),report.base_initialized?1u:0u,report.warning_emitted?1u:0u,
        report.event_published?1u:0u,0,report.removal_queued?1u:0u};
    return 1;
}
int SIN_FIXED_CALL native_trigger_touch(abi::MapEntityRef ref,abi::MapEntityRef actor,
        const abi::NativeTriggerTouch* input,const abi::NativeTriggerHost* host,void* context,
        abi::NativeTriggerReport* out) noexcept {
    auto* entity=map_find(ref);
    if(reading_engine_skill||!entity||!is_trigger(entity->kind)||!trigger_runtime||
       !entity->spawned||entity->remove_requested||!input||!out||!valid_trigger_host(host))return 0;
    if(actor.epoch!=ref.epoch){
        *out={abi::TriggerInvalidActor,0,0,0,0,0};return 1;
    }
    TriggerCallbackScope scope(trigger_runtime->bridge,ref,host,context);
    auto& factory=trigger_runtime->factory;
    const auto refresh=factory.refresh_native_fields(trigger_ref(ref),
        {input->x,input->y,input->z},input->flags_b8);
    if(refresh!=triggers::TriggerCode::Handled){
        *out={trigger_code(refresh),0,0,0,0,0};return 1;
    }
    const auto report=factory.touch(trigger_ref(ref),trigger_ref(actor));
    *out={trigger_code(report.code),0,0,report.event_published?1u:0u,
        report.manager_signaled?1u:0u,report.removal_queued?1u:0u};
    return 1;
}
int SIN_FIXED_CALL native_trigger_retry(abi::MapEntityRef ref,const abi::NativeTriggerHost* host,
        void* context,abi::NativeTriggerReport* out) noexcept {
    auto* entity=map_find(ref);
    if(reading_engine_skill||!entity||!is_trigger(entity->kind)||!trigger_runtime||!out||
       !valid_trigger_host(host))return 0;
    TriggerCallbackScope scope(trigger_runtime->bridge,ref,host,context);
    const auto code=trigger_runtime->factory.retry_pending_remove(trigger_ref(ref));
    *out={trigger_code(code),0,0,0,0,code==triggers::TriggerCode::Handled?1u:0u};
    return 1;
}
const abi::CampaignTriggerApi trigger_api{abi::trigger_version,sizeof(abi::CampaignTriggerApi),
    &native_trigger_spawn,&native_trigger_touch,&native_trigger_retry};

std::uint32_t SIN_FIXED_CALL map_begin(const char* name) noexcept {
    if(reading_engine_skill||!campaign||level||!bounded_string(name)||!*name||
       last_epoch==(std::numeric_limits<std::uint32_t>::max)())return 0;
    try {
        MapLevel next;
        next.epoch=last_epoch+1;
        next.name=name;
        level.emplace(std::move(next));
    }catch(...){return 0;}
    return ++last_epoch;
}
int SIN_FIXED_CALL map_end(std::uint32_t epoch) noexcept {
    if(reading_engine_skill||!active_level(epoch))return 0;
    trigger_runtime.reset();
    level.reset();
    return 1;
}
int SIN_FIXED_CALL map_count(std::uint32_t epoch) noexcept {
    if(!active_level(epoch))return -1;
    int count=0;
    for(const auto& item:level->entities)if(item.second.alive)++count;
    return count;
}
int SIN_FIXED_CALL map_create(abi::MapEntityRef ref,const char* classname) noexcept {
    if(reading_engine_skill||!active_level(ref.epoch)||!bounded_string(classname)||
       ref.index==(std::numeric_limits<std::uint32_t>::max)()||
       ref.generation==(std::numeric_limits<std::uint32_t>::max)())return abi::MapRejected;
    EntityKind kind;
    if(std::strcmp(classname,"env_global")==0)kind=EntityKind::EnvGlobal;
    else if(std::strcmp(classname,"info_challenge")==0)kind=EntityKind::InfoChallenge;
    else if(std::strcmp(classname,"trigger_dynamic_difficulty")==0)kind=EntityKind::DynamicTrigger;
    else if(std::strcmp(classname,"trigger_secret")==0)kind=EntityKind::SecretTrigger;
    else return abi::MapUnknownClass;
    auto it=level->entities.find(ref.index);
    if(it!=level->entities.end()&&(it->second.alive||ref.generation<=it->second.generation))
        return abi::MapRejected;
    if(it==level->entities.end()&&level->entities.size()>=max_map_entities)return abi::MapRejected;
    try {
        MapEntity entity;
        entity.generation=ref.generation;
        entity.kind=kind;
        if(it==level->entities.end())level->entities.emplace(ref.index,std::move(entity));
        else it->second=std::move(entity);
    }catch(...){return abi::MapRejected;}
    if(is_trigger(kind)&&!attach_trigger(ref,classname)){
        // No live behavior on failure; this generation is retired.
        level->entities.find(ref.index)->second.alive=false;
        return abi::MapRejected;
    }
    return abi::MapApplied;
}
template<class Int>
bool parse_map_int(const char* s,Int& out) noexcept {
    // Strict port-side conversion, not a claim of native atoi parity.
    if(!bounded_string(s)||!*s)return false;
    const auto* end=s+std::strlen(s);
    Int parsed{};
    const auto result=std::from_chars(s,end,parsed,10);
    if(result.ec!=std::errc{}||result.ptr!=end)return false;
    out=parsed;
    return true;
}
int SIN_FIXED_CALL map_keyvalue(abi::MapEntityRef ref,const char* key,const char* value) noexcept {
    auto* entity=map_find(ref);
    if(reading_engine_skill||!entity||entity->spawned||entity->remove_requested||
       !bounded_string(key)||!bounded_string(value))return abi::MapRejected;
    if(is_trigger(entity->kind)){
        if(!trigger_runtime)return abi::MapRejected;
        const auto result=trigger_runtime->factory.apply_map_keyvalue(trigger_ref(ref),key,value);
        if(result==triggers::TriggerCode::Handled)return abi::MapApplied;
        return result==triggers::TriggerCode::UnknownKey?abi::MapUnhandledKey:abi::MapRejected;
    }
    if(entity->kind!=EntityKind::EnvGlobal)return abi::MapUnhandledKey;
    if(std::strcmp(key,"globalstate")==0){
        try {entity->global_name=std::string(value);}catch(...){return abi::MapRejected;}
    }else if(std::strcmp(key,"initialstate")==0){
        if(!parse_map_int(value,entity->initial_state))return abi::MapRejected;
    }else if(std::strcmp(key,"spawnflags")==0){
        if(!parse_map_int(value,entity->flags))return abi::MapRejected;
    }else return abi::MapUnhandledKey;
    return abi::MapApplied;
}
int SIN_FIXED_CALL map_spawn(abi::MapEntityRef ref) noexcept {
    auto* entity=map_find(ref);
    if(reading_engine_skill||!entity||entity->spawned||entity->remove_requested)return abi::MapRejected;
    // Trigger spawn needs the explicit native base/flags callbacks.
    if(is_trigger(entity->kind))return abi::MapRejected;
    if(entity->kind==EntityKind::EnvGlobal){
        const int result=SiN_EnvGlobalDispatch(abi::EnvSpawn,
            entity->global_name?entity->global_name->c_str():nullptr,
            level->name.c_str(),entity->flags,entity->initial_state);
        if(result==abi::EnvRejected)return abi::MapRejected;
        if(result==abi::EnvRemoveSelf){entity->remove_requested=true;return abi::MapRemoveSelf;}
    }else if(!capture(&entity->snapshot))return abi::MapRejected;
    entity->spawned=true;
    return abi::MapApplied;
}
struct OutputScope {
    OutputScope() noexcept {reading_engine_skill=true;}
    ~OutputScope(){reading_engine_skill=false;}
};
int SIN_FIXED_CALL map_input(abi::MapEntityRef ref,const char* name,
        abi::MapFloatOutput sink,void* context) noexcept {
    auto* entity=map_find(ref);
    if(reading_engine_skill||!entity||!entity->spawned||entity->remove_requested||
       !bounded_string(name))return abi::MapRejected;
    if(entity->kind==EntityKind::EnvGlobal){
        std::uint32_t op=0;
        if(std::strcmp(name,"TurnOn")==0)op=abi::EnvTurnOn;
        else if(std::strcmp(name,"TurnOff")==0)op=abi::EnvTurnOff;
        else if(std::strcmp(name,"Remove")==0)op=abi::EnvRemove;
        else if(std::strcmp(name,"Toggle")==0)op=abi::EnvToggle;
        else return abi::MapUnhandledInput;
        const int result=SiN_EnvGlobalDispatch(op,
            entity->global_name?entity->global_name->c_str():nullptr,
            level->name.c_str(),entity->flags,entity->initial_state);
        switch(result){
        case abi::EnvApplied:return abi::MapApplied;
        case abi::EnvUnchanged:return abi::MapUnchanged;
        case abi::EnvBlocked:return abi::MapBlocked;
        default:return abi::MapRejected;
        }
    }
    if(is_trigger(entity->kind))return abi::MapUnhandledInput;
    challenge::Input input;
    if(std::strcmp(name,"GetInitialChallenge")==0)input=challenge::Input::Initial;
    else if(std::strcmp(name,"GetCurrentChallenge")==0)input=challenge::Input::Current;
    else if(std::strcmp(name,"GetChallengeDifference")==0)input=challenge::Input::Difference;
    else return abi::MapUnhandledInput;
    if(!sink)return abi::MapRejected;
    abi::ChallengeOutput out{};
    if(!query(static_cast<std::uint32_t>(input),entity->snapshot,ref.index,&out))return abi::MapRejected;
    OutputScope guard;
    try {
        return sink(context,&ref,challenge::output_name(input),out.original_field_offset,out.value)
            ?abi::MapApplied:abi::MapOutputFailed;
    }catch(...){return abi::MapOutputFailed;}
}
int SIN_FIXED_CALL map_remove(abi::MapEntityRef ref) noexcept {
    auto* entity=map_find(ref);
    if(reading_engine_skill||!entity)return 0;
    if(is_trigger(entity->kind)&&(!trigger_runtime||
        trigger_runtime->factory.release(trigger_ref(ref))!=triggers::TriggerCode::Handled))return 0;
    entity->alive=false;
    entity->global_name.reset();
    return 1;
}
const abi::MapEntityApi map_api{abi::map_entity_version,sizeof(abi::MapEntityApi),
    &map_begin,&map_end,&map_create,&map_keyvalue,&map_spawn,&map_input,&map_remove,&map_count};
}
extern "C" SIN_FIXED_EXPORT const abi::MapEntityApi* SIN_FIXED_CALL
SiN_GetMapEntityApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==abi::map_entity_version&&bytes==sizeof(abi::MapEntityApi)?&map_api:nullptr;
}

extern "C" SIN_FIXED_EXPORT const abi::CampaignTriggerApi* SIN_FIXED_CALL
SiN_GetCampaignTriggerApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==abi::trigger_version&&bytes==sizeof(abi::CampaignTriggerApi)?&trigger_api:nullptr;
}

#if defined(_MSC_VER) && defined(_M_IX86)
namespace {
namespace memory=sin_independent::vgui_layout;
namespace methods=sin_independent::known_game_system;
// Phase335: 24-byte ConCommandBase, 68-byte ConVar. Original getters
// resolve a single parent link. Only byte reads of verified fields cross
// the foreign object boundary; no cast to the reconstructed C++ class.
constexpr std::size_t parent_offset=0x18, name_offset=0x0c, int_offset=0x2c;
bool read_engine_skill(void* engine,int& value) noexcept {
    methods::CallResult refused=methods::CallResult::InvalidObject;
    const auto method=methods::VerifiedMethod(engine,3,refused);
    if(!method)return false;
    using Find=sin_independent::console_abi::FindSlot;
    const void* variable=nullptr;
    try {variable=reinterpret_cast<Find>(method)(engine,"skill");}
    catch(...) {return false;}
    if(!memory::Readable(variable,parent_offset+sizeof(void*)))return false;
    const auto* bytes=static_cast<const unsigned char*>(variable);
    const unsigned char* root=nullptr;
    std::memcpy(&root,bytes+parent_offset,sizeof(root));
    if(!memory::Readable(root,int_offset+sizeof(int)))return false;
    const unsigned char* root_parent=nullptr;
    std::memcpy(&root_parent,root+parent_offset,sizeof(root_parent));
    if(root_parent!=root)return false;
    const char* name=nullptr;
    std::memcpy(&name,root+name_offset,sizeof(name));
    if(!memory::Readable(name,6)||std::memcmp(name,"skill",6)!=0)return false;
    int observed=0;
    std::memcpy(&observed,root+int_offset,sizeof(observed));
    if(!fixed::valid_skill(observed))return false;
    value=observed;
    return true;
}
struct EngineReadScope {
    EngineReadScope() noexcept {reading_engine_skill=true;}
    ~EngineReadScope(){reading_engine_skill=false;}
};
}
// Original KeyValues file reader 0x22264200 calls primary VFileSystem012
// slots 2/7/0/3. The CSE1Rules constructor at 0x222d7e30 supplies GAME.
// This adapter reads exactly that resource and publishes only after a
// normal Close and successful independent parser validation.
#pragma comment(linker,"/EXPORT:SiN_LoadAttackSpreadFromEngine=_SiN_LoadAttackSpreadFromEngine")
extern "C" int SIN_FIXED_CALL SiN_LoadAttackSpreadFromEngine(void* fs,
        abi::SpreadFileReport* output) noexcept {
    if(!output)return 0;
    abi::SpreadFileReport report{abi::SpreadFileRejected,0,0,0,0};
    const auto finish=[&](std::uint32_t code) noexcept {
        report.code=code;*output=report;return 1;
    };
    if(reading_engine_skill||!fs)return finish(abi::SpreadFileRejected);
    methods::CallResult refused=methods::CallResult::InvalidObject;
    const auto open=methods::VerifiedMethod(fs,2,refused);
    const auto size=methods::VerifiedMethod(fs,7,refused);
    const auto read=methods::VerifiedMethod(fs,0,refused);
    const auto close=methods::VerifiedMethod(fs,3,refused);
    if(!open||!size||!read||!close)return finish(abi::SpreadFileRejected);
    using Open=void* (__thiscall *)(void*,const char*,const char*,const char*);
    using Size=unsigned (__thiscall *)(void*,void*);
    using Read=int (__thiscall *)(void*,void*,int,void*);
    using Close=void (__thiscall *)(void*,void*);
    EngineReadScope scope;
    void* handle=nullptr;
    try {handle=reinterpret_cast<Open>(open)(fs,"scripts/rules_combat.txt","rb","GAME");}
    catch(...){return finish(abi::SpreadFileOpenUncertain);}
    if(!handle)return finish(abi::SpreadFileMissing);
    std::uint32_t code=abi::SpreadFileReadFailed;
    std::string bytes;
    try {
        report.expected_bytes=reinterpret_cast<Size>(size)(fs,handle);
        if(!report.expected_bytes||report.expected_bytes>sin_independent::attack_spread::max_bytes){
            code=abi::SpreadFileInvalidLength;
        }else{
            bytes.resize(report.expected_bytes);
            while(report.read_bytes<report.expected_bytes){
                const auto remaining=report.expected_bytes-report.read_bytes;
                const int n=reinterpret_cast<Read>(read)(fs,&bytes[report.read_bytes],
                    static_cast<int>(remaining),handle);
                if(n<=0||static_cast<unsigned>(n)>remaining)break;
                report.read_bytes+=static_cast<unsigned>(n);
            }
            if(report.read_bytes==report.expected_bytes)code=abi::SpreadFileLoaded;
        }
    }catch(...){code=abi::SpreadFileReadFailed;}
    report.close_attempted=1;
    try {reinterpret_cast<Close>(close)(fs,handle);report.close_returned=1;}
    catch(...){return finish(abi::SpreadFileCloseUncertain);}
    if(code!=abi::SpreadFileLoaded)return finish(code);
    try {
        if(!spread_rules.load(bytes))return finish(abi::SpreadFileParseFailed);
    }catch(...){return finish(abi::SpreadFileParseFailed);}
    return finish(abi::SpreadFileLoaded);
}

// Original server rule methods use VEngineRandom001 +0x4 with
// RandomFloat(0.f,1.f). Resolve the slot only when the rule needs RNG.
namespace {
int SIN_FIXED_CALL read_native_npc_random(void* engine,float lo,float hi,float* out) {
    methods::CallResult refused=methods::CallResult::InvalidObject;
    const auto method=methods::VerifiedMethod(engine,1,refused);
    if(!method)return 0;
    using RandomFloat=float (__thiscall *)(void*,float,float);
    *out=reinterpret_cast<RandomFloat>(method)(engine,lo,hi);
    return 1;
}
}
#pragma comment(linker,"/EXPORT:SiN_DecideNpcWithEngineRandom=_SiN_DecideNpcWithEngineRandom")
extern "C" int SIN_FIXED_CALL SiN_DecideNpcWithEngineRandom(void* engine,
    std::uint32_t kind,float factor,abi::NpcDecisionOutput* out) noexcept {
    return npc_decide(kind,factor,&read_native_npc_random,engine,out);
}

// Phase675: original engine manager and event slots authenticated against
// the owner engine.dll RTTI-backed CGameEventManager/CGameEvent vtables.
// All pointers are borrowed on the game thread. No addresses from the
// original executable are called by absolute VA.
#pragma comment(linker,"/EXPORT:SiN_PublishCampaignTriggerEvent=_SiN_PublishCampaignTriggerEvent")
extern "C" int SIN_FIXED_CALL SiN_PublishCampaignTriggerEvent(void* manager,
    const abi::NativeTriggerEvent* input) noexcept {
    if(native_event_active||(reading_engine_skill&&!trigger_publish_active)||
       !manager||!input||!bounded_string(input->name))return abi::EventRejected;
    const auto event=*input;
    const bool dynamic=event.kind==abi::TriggerDynamicEvent;
    if(!dynamic&&event.kind!=abi::TriggerGameEvent)return abi::EventRejected;
    if(event.advisor!=-1||
       (dynamic&&(event.descriptor!=0||!std::isfinite(event.value)||!std::isfinite(event.x)||
                  !std::isfinite(event.y)||!std::isfinite(event.z)))||
       (!dynamic&&(event.owner!=-1||(event.descriptor!=11&&event.descriptor!=12))))
        return abi::EventRejected;
    std::string name;
    try {name=event.name;}catch(...){return abi::EventRejected;}
    NativeEventScope scope;
    methods::CallResult refused=methods::CallResult::InvalidObject;
    const auto create=methods::VerifiedMethod(manager,6,refused);
    const auto fire=methods::VerifiedMethod(manager,7,refused);
    const auto release=methods::VerifiedMethod(manager,10,refused);
    if(!create||!fire||!release)return abi::EventRejected;
    using Create=void* (__thiscall *)(void*,const char*,bool);
    using Fire=bool (__thiscall *)(void*,void*,bool);
    using Release=void (__thiscall *)(void*,void*);
    using SetInt=void (__thiscall *)(void*,const char*,int);
    using SetFloat=void (__thiscall *)(void*,const char*,float);
    using SetString=void (__thiscall *)(void*,const char*,const char*);
    void* object=nullptr;
    try {object=reinterpret_cast<Create>(create)(manager,dynamic?"dynamic_difficulty":"game",false);}
    catch(...){return abi::EventCreateUncertain;}
    if(!object)return abi::EventUnavailable;
    // FreeEvent itself dispatches the event's slot 0 deleting destructor.
    // If that is unverified, ownership is uncertain; do not call into it.
    if(!methods::VerifiedMethod(object,0,refused))return abi::EventObjectUnverified;
    const auto integer=methods::VerifiedMethod(object,10,refused);
    const auto real=dynamic?methods::VerifiedMethod(object,11,refused):nullptr;
    const auto string=methods::VerifiedMethod(object,12,refused);
    const auto discard=[&]() noexcept {
        try {reinterpret_cast<Release>(release)(manager,object);return abi::EventDiscarded;}
        catch(...){return abi::EventCleanupUncertain;}
    };
    if(!integer||!string||(dynamic&&!real))return discard();
    const auto set_int=reinterpret_cast<SetInt>(integer);
    const auto set_string=reinterpret_cast<SetString>(string);
    const auto set_float=reinterpret_cast<SetFloat>(real);
    try {
        set_int(object,"descriptor",event.descriptor);
        set_int(object,"owner",event.owner);
        set_int(object,"subject",event.advisor);
        set_string(object,"name",name.c_str());
        if(dynamic){
            set_int(object,"decision",event.decision);
            set_float(object,"value",event.value);
            set_float(object,"x",event.x);
            set_float(object,"y",event.y);
            set_float(object,"z",event.z);
        }
    }catch(...){return discard();}
    // Original engine FireEvent frees on both ordinary true/false paths.
    // No cleanup or retry after entering FireEvent, including exceptions.
    try {return reinterpret_cast<Fire>(fire)(manager,object,false)?
        abi::EventFired:abi::EventFireReturnedFalse;}
    catch(...){return abi::EventFireUncertain;}
}

#pragma comment(linker,"/EXPORT:SiN_ApplyEngineSkill=_SiN_ApplyEngineSkill")
extern "C" int SIN_FIXED_CALL SiN_ApplyEngineSkill(void* engine,std::uint32_t op) noexcept {
    if(reading_engine_skill || !engine ||
       (op!=abi::BeginFromEngine && op!=abi::SyncFromEngine) ||
       (op==abi::BeginFromEngine && campaign) ||
       (op==abi::SyncFromEngine && !campaign))return 0;
    int skill=0;
    {
        EngineReadScope scope;
        if(!read_engine_skill(engine,skill))return 0;
    }
    return op==abi::BeginFromEngine?begin(skill):select(skill);
}

// Original ServerGameDLL003 slot 1 (Phase88) queues the literal via
// VEngineServer021 +0x90, then executes via +0x94. The preceding
// global reset now clears the independent CGlobalState owner. The caller supplies
// the correctly initialized interfaces at an explicit campaign boundary.
#pragma comment(linker,"/EXPORT:SiN_StartConfiguredCampaign=_SiN_StartConfiguredCampaign")
extern "C" int SIN_FIXED_CALL SiN_StartConfiguredCampaign(void* server,void* cvar) noexcept {
    if(reading_engine_skill||campaign||!server||!cvar)return abi::ConfigRejected;
    int skill=0;
    {
        EngineReadScope scope;
        methods::CallResult refused=methods::CallResult::InvalidObject;
        // Check all three method slots before any command-buffer effect.
        const auto queue=methods::VerifiedMethod(server,36,refused);
        const auto execute=methods::VerifiedMethod(server,37,refused);
        const auto find=methods::VerifiedMethod(cvar,3,refused);
        if(!queue||!execute||!find)return abi::ConfigRejected;
        // Original GameInit's first operation is CGlobalState::Clear.
        // Clear this module's owned equivalent before exec game.cfg.
        // Refusal before preflight leaves it untouched; after this
        // boundary external effects are not rolled back.
        clear_global_storage();
        using Queue=void (__thiscall *)(void*,const char*);
        using Execute=void (__thiscall *)(void*);
        try {reinterpret_cast<Queue>(queue)(server,"exec game.cfg\n");}
        catch(...) {return abi::ConfigQueueUncertain;}
        try {reinterpret_cast<Execute>(execute)(server);}
        catch(...) {return abi::ConfigExecuteUncertain;}
        // This lookup must follow execution: game.cfg may change skill.
        if(!read_engine_skill(cvar,skill))return abi::ConfigRanSkillUnavailable;
    }
    return begin(skill)?abi::ConfigStarted:abi::ConfigRanSkillUnavailable;
}
#endif
