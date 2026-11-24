#include "server_fixed_difficulty_service.h"
#include "original_ammo_definitions.h"
#include "phase45_zoom_transition.hpp"
#include "phase47_ads_activity.hpp"
#include "sin_compat/original_server_scattergun_ammo.hpp"
#include "sin_compat/original_ar_grenade_runtime.hpp"
#include "sin_compat/original_ar_grenade_touch.hpp"
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
enum class EntityKind { EnvGlobal, InfoChallenge, DynamicTrigger, SecretTrigger, Magnum, AssaultRifle, Scattergun, ARGrenade };
struct MapEntity {
    std::uint32_t generation=0;
    EntityKind kind=EntityKind::EnvGlobal;
    bool alive=true,spawned=false,remove_requested=false;
    std::optional<std::string> global_name;
    std::uint32_t flags=0;
    std::int32_t initial_state=0;
    float snapshot=0.f;
    sin_phase45::WeaponState weapon_zoom{};
    bool weapon_resync=false;
    sin_compat::original_server_scattergun_ammo::ObservedAmmoState scattergun{};
    bool scattergun_resync=false;
    sin_compat::ar_grenade::Runtime grenade{};
    bool grenade_contact=false,grenade_uncertain=false;
    bool rifle_launch_uncertain=false;
    bool magnum_uncertain=false;
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

// Original CAI_BaseNPC 0x22031120. The base spread is already scaled
// by metric 12. Apply only the transient focus penalty afterwards.
bool make_focus_base(const char* npc,const char* weapon,const char* target,
        const abi::NpcFocusMemory* memory,abi::NpcAttackSpreadResult& result) noexcept {
    if(!memory||memory->found>1)return false;
    if(!query_spread_rules(npc,weapon,target,&result.rules))return false;
    result.result={result.rules.x,result.rules.y,result.rules.z};
    result.focus_multiplier=1.f;
    return true;
}
bool focus_needs_multiplier(const abi::NpcFocusMemory& memory,float focus,
        abi::NpcAttackSpreadResult& result,bool& needed) noexcept {
    needed=false;
    if(!std::isfinite(focus))return false;
    if(focus<=0.f)return true;
    if(!std::isfinite(memory.curtime)||!std::isfinite(memory.time_valid_enemy))return false;
    // Native stores elapsed as float32 and clamps negative differences.
    const float elapsed=memory.curtime-memory.time_valid_enemy;
    if(!std::isfinite(elapsed))return false;
    result.elapsed=elapsed<0.f?0.f:elapsed;
    needed=result.elapsed<focus;
    return true;
}
bool apply_focus_penalty(float focus,float cone,abi::NpcAttackSpreadResult& result) noexcept {
    if(!std::isfinite(cone))return false;
    if(cone<=1.f)return true;
    const double multiplier=1.0+(static_cast<double>(cone)-1.0)*
        (1.0-static_cast<double>(result.elapsed)/focus);
    const float x=static_cast<float>(result.result.x*multiplier);
    const float y=static_cast<float>(result.result.y*multiplier);
    const float z=static_cast<float>(result.result.z*multiplier);
    const float stored=static_cast<float>(multiplier);
    if(!std::isfinite(stored)||!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z))return false;
    result.result={x,y,z};
    result.focus_multiplier=stored;
    result.defocused=1;
    return true;
}
int SIN_FIXED_CALL query_npc_attack_spread(const char* npc,const char* weapon,const char* target,
        const abi::NpcFocusMemory* memory,const abi::NpcFocusTuning* tuning,
        abi::NpcAttackSpreadResult* out) noexcept {
    abi::NpcAttackSpreadResult result{};
    if(!out||!make_focus_base(npc,weapon,target,memory,result))return 0;
    if(memory->found){
        bool needed=false;
        if(!tuning||!focus_needs_multiplier(*memory,tuning->focus_time,result,needed))return 0;
        if(needed&&!apply_focus_penalty(tuning->focus_time,tuning->defocused_multiplier,result))return 0;
    }
    *out=result;return 1;
}
const abi::NpcAttackSpreadApi npc_attack_spread_api{abi::npc_attack_spread_version,
    sizeof(abi::NpcAttackSpreadApi),&query_npc_attack_spread};

// Original 222d7ee0 packs three folded pointer identities into a
// 48-bit key. Collision aliasing is intentional original behavior.
std::map<std::uint64_t,abi::AttackSpreadResult> original_spread_cache;
constexpr std::size_t max_spread_cache=65536;
bool valid_spread_identity(const abi::SpreadClassIdentity* id) noexcept {
    return id&&id->present<=1&&
        (id->present?(id->identity!=0&&(!id->name||bounded_string(id->name))):(!id->identity&&!id->name));
}
std::uint32_t fold_spread_identity(const abi::SpreadClassIdentity& id) noexcept {
    return id.present?((id.identity^(id.identity>>16))&0xffffu):0u;
}
const char* spread_identity_name(const abi::SpreadClassIdentity& id) noexcept {
    return !id.present?"*":id.name?id.name:"";
}
int SIN_FIXED_CALL clear_cached_spread() noexcept {
    if(reading_engine_skill)return 0;
    original_spread_cache.clear();return 1;
}
std::uint32_t SIN_FIXED_CALL cached_spread_count() noexcept {
    return static_cast<std::uint32_t>(original_spread_cache.size());
}
int SIN_FIXED_CALL query_cached_spread(const abi::SpreadClassIdentity* attacker,
        const abi::SpreadClassIdentity* weapon,const abi::SpreadClassIdentity* target,
        abi::CachedSpreadResult* out) noexcept {
    if(reading_engine_skill||!campaign||!out||!valid_spread_identity(attacker)||
       !valid_spread_identity(weapon)||!valid_spread_identity(target))return 0;
    const auto low=fold_spread_identity(*attacker)|(fold_spread_identity(*weapon)<<16);
    const auto high=fold_spread_identity(*target);
    const auto key=(static_cast<std::uint64_t>(high)<<32)|low;
    const auto existing=original_spread_cache.find(key);
    if(existing!=original_spread_cache.end()){
        *out={existing->second,1,low,high};return 1;
    }
    if(original_spread_cache.size()>=max_spread_cache)return 0;
    abi::AttackSpreadResult result{};
    if(!query_spread_rules(spread_identity_name(*attacker),spread_identity_name(*weapon),
            spread_identity_name(*target),&result))return 0;
    try {original_spread_cache.emplace(key,result);}
    catch(...){return 0;}
    *out={result,0,low,high};return 1;
}
int SIN_FIXED_CALL query_cached_focus(const abi::SpreadClassIdentity* attacker,
        const abi::SpreadClassIdentity* weapon,const abi::SpreadClassIdentity* target,
        const abi::NpcFocusMemory* memory,const abi::NpcFocusTuning* tuning,
        abi::NpcAttackSpreadResult* out) noexcept {
    if(!out||!memory||memory->found>1)return 0;
    abi::CachedSpreadResult base{};
    if(!query_cached_spread(attacker,weapon,target,&base))return 0;
    abi::NpcAttackSpreadResult result{};
    result.rules=base.rules;result.result={base.rules.x,base.rules.y,base.rules.z};result.focus_multiplier=1.f;
    if(memory->found){
        bool needed=false;
        if(!tuning||!focus_needs_multiplier(*memory,tuning->focus_time,result,needed))return 0;
        if(needed&&!apply_focus_penalty(tuning->focus_time,tuning->defocused_multiplier,result))return 0;
    }
    *out=result;return 1;
}
const abi::CachedAttackSpreadApi cached_spread_api{abi::cached_spread_version,
    sizeof(abi::CachedAttackSpreadApi),&clear_cached_spread,&query_cached_spread,&query_cached_focus,&cached_spread_count};

// Original CAI_BaseNPC slot244 returns the shot bias. Its second
// elapsed-time branch deliberately DOES NOT clamp negative reacquisition.
bool npc_bias(const abi::NpcAimMemory& memory,float shot_bias,float focus,
        abi::NpcBiasResult& result) noexcept {
    if(memory.focus.found>1||!std::isfinite(shot_bias))return false;
    result={shot_bias,shot_bias,1.f,abi::NpcBiasUnfocused};
    if(!memory.focus.found)return true;
    if(!std::isfinite(focus))return false;
    if(focus<=0.f)return true;
    if(!std::isfinite(memory.focus.curtime)||!std::isfinite(memory.focus.time_valid_enemy)||
       !std::isfinite(memory.time_last_reacquired))return false;
    // x87 keeps valid-enemy elapsed in the FPU; reacquired elapsed is
    // explicitly stored as f32 in the original before the branch.
    const double elapsed=static_cast<double>(memory.focus.curtime)-memory.focus.time_valid_enemy;
    const double valid_elapsed=elapsed<0.0?0.0:elapsed;
    const float reacquired=memory.focus.curtime-memory.time_last_reacquired;
    if(!std::isfinite(reacquired))return false;
    double ratio=1.0;
    if(valid_elapsed<focus){
        ratio=valid_elapsed/focus;result.path=abi::NpcBiasNewEnemy;
    }else if(reacquired<focus){
        ratio=static_cast<double>(reacquired)/focus;result.path=abi::NpcBiasReacquired;
    }
    const float scale=static_cast<float>(ratio);
    const float bias=static_cast<float>(static_cast<double>(shot_bias)*ratio);
    if(!std::isfinite(scale)||!std::isfinite(bias))return false;
    result.scale=scale;result.bias=bias;
    return true;
}
int SIN_FIXED_CALL query_npc_aim(const char* npc,const char* weapon,const char* target,
        const abi::NpcAimMemory* memory,const abi::NpcAimTuning* tuning,abi::NpcAimResult* out) noexcept {
    if(reading_engine_skill||!campaign||!memory||!tuning||!out)return 0;
    abi::NpcAimResult result{};
    // Original firing sites call bias slot244 before spread slot243.
    if(!npc_bias(*memory,tuning->shot_bias,tuning->pattern_focus_time,result.bias)||
       !query_npc_attack_spread(npc,weapon,target,&memory->focus,&tuning->spread,&result.spread))return 0;
    *out=result;return 1;
}
const abi::NpcAimApi npc_aim_api{abi::npc_aim_version,sizeof(abi::NpcAimApi),&query_npc_aim};

bool finite_vector(const abi::NpcSpread& v) noexcept {
    return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);
}
int SIN_FIXED_CALL npc_sample_shot(const abi::NpcShotBasis* basis,const abi::NpcAimResult* aim,
        const abi::NpcShotTuning* tuning,abi::NpcRandomFloat random,void* context,
        abi::NpcShotReport* out) noexcept {
    if(!out)return 0;
    abi::NpcShotReport result{};
    const auto finish=[&](std::uint32_t code) noexcept {
        result.code=code;*out=result;return 1;
    };
    if(reading_engine_skill||!campaign||!basis||!aim||!tuning||!random)
        return finish(abi::NpcShotRejected);
    // Copy before foreign callbacks; no borrowed pointer is read afterwards.
    const auto frame=*basis;
    const auto spread=aim->spread.result;
    const auto range=*tuning;
    const float bias=aim->bias.bias;
    if(!finite_vector(frame.forward)||!finite_vector(frame.right)||!finite_vector(frame.up)||
       !finite_vector(spread)||!std::isfinite(bias)||!std::isfinite(range.bias_min)||
       !std::isfinite(range.bias_max))return finish(abi::NpcShotRejected);
    const float bounded=bias<0.f?0.f:(bias>1.f?1.f:bias);
    result.effective_bias=static_cast<float>(static_cast<double>(range.bias_min)+
        (static_cast<double>(range.bias_max)-range.bias_min)*bounded);
    result.flatness=std::fabs(result.effective_bias)*0.5f;
    const float complement=1.f-result.flatness;
    if(!std::isfinite(result.effective_bias)||!std::isfinite(result.flatness)||!std::isfinite(complement))
        return finish(abi::NpcShotOverflow);
    NpcRandomScope scope;
    const auto draw=[&](float& value) noexcept -> std::uint32_t {
        ++result.random_calls;
        try {if(!random(context,-1.f,1.f,&value))return abi::NpcShotRandomRefused;}
        catch(...){return abi::NpcShotRandomRefused;}
        return std::isfinite(value)&&value>=-1.f&&value<=1.f?
            abi::NpcShotAccepted:abi::NpcShotInvalidSample;
    };
    for(unsigned attempt=0;attempt<abi::npc_shot_attempt_limit;++attempt){
        ++result.attempts;
        float v[4]{};
        for(auto& value:v){
            const auto code=draw(value);
            if(code!=abi::NpcShotAccepted)return finish(code);
        }
        // Original x87 stores each first weighted sample as f32. X is
        // stored before inversion; Y remains in the FPU until consumed.
        float x=static_cast<float>(static_cast<double>(static_cast<float>(
            static_cast<double>(v[0])*result.flatness))+static_cast<double>(v[1])*complement);
        double y=static_cast<double>(static_cast<float>(static_cast<double>(v[2])*result.flatness))+
            static_cast<double>(v[3])*complement;
        if(result.effective_bias<0.f){
            x=static_cast<float>(x>=0.f?1.0-x:-1.0-x);
            y=y>=0.0?1.0-y:-1.0-y;
        }
        if(!std::isfinite(x)||!std::isfinite(y))return finish(abi::NpcShotOverflow);
        if(static_cast<double>(x)*x+y*y>1.0)continue;
        const double dx=static_cast<double>(x)*spread.x,dy=y*spread.y;
        const auto component=[&](float forward,float right,float up) noexcept {
            const float horizontal=static_cast<float>(dx*right);
            const float vertical=static_cast<float>(dy*up);
            const float intermediate=static_cast<float>(static_cast<double>(forward)+horizontal);
            return static_cast<float>(static_cast<double>(intermediate)+vertical);
        };
        const abi::NpcSpread direction{component(frame.forward.x,frame.right.x,frame.up.x),
            component(frame.forward.y,frame.right.y,frame.up.y),
            component(frame.forward.z,frame.right.z,frame.up.z)};
        if(!finite_vector(direction))return finish(abi::NpcShotOverflow);
        result.x=x;result.y=static_cast<float>(y);result.direction=direction;
        return finish(abi::NpcShotAccepted);
    }
    // Retail loops indefinitely. Bounded retry is explicitly port-only.
    return finish(abi::NpcShotAttemptLimit);
}
const abi::NpcShotApi npc_shot_api{abi::npc_shot_version,sizeof(abi::NpcShotApi),&npc_sample_shot};

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
#pragma comment(linker,"/EXPORT:SiN_GetCachedAttackSpreadApi=_SiN_GetCachedAttackSpreadApi")
#pragma comment(linker,"/EXPORT:SiN_GetNpcAttackSpreadApi=_SiN_GetNpcAttackSpreadApi")
#pragma comment(linker,"/EXPORT:SiN_GetNpcAimApi=_SiN_GetNpcAimApi")
#pragma comment(linker,"/EXPORT:SiN_GetNpcShotApi=_SiN_GetNpcShotApi")
#pragma comment(linker,"/EXPORT:SiN_GetWeaponBehaviorApi=_SiN_GetWeaponBehaviorApi")
#pragma comment(linker,"/EXPORT:SiN_GetScattergunPumpApi=_SiN_GetScattergunPumpApi")
#pragma comment(linker,"/EXPORT:SiN_GetScattergunSecondaryApi=_SiN_GetScattergunSecondaryApi")
#pragma comment(linker,"/EXPORT:SiN_GetARGrenadeApi=_SiN_GetARGrenadeApi")
#pragma comment(linker,"/EXPORT:SiN_GetRifleGrenadeApi=_SiN_GetRifleGrenadeApi")
#pragma comment(linker,"/EXPORT:SiN_GetMagnumApi=_SiN_GetMagnumApi")
#pragma comment(linker,"/EXPORT:SiN_GetMagnumSecondaryApi=_SiN_GetMagnumSecondaryApi")
#pragma comment(linker,"/EXPORT:SiN_GetCombatAmmoApi=_SiN_GetCombatAmmoApi")
#pragma comment(linker,"/EXPORT:SiN_GetWeaponSnapshotApi=_SiN_GetWeaponSnapshotApi")
#pragma comment(linker,"/EXPORT:SiN_GetInventorySnapshotApi=_SiN_GetInventorySnapshotApi")
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

extern "C" SIN_FIXED_EXPORT const abi::NpcShotApi* SIN_FIXED_CALL
SiN_GetNpcShotApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==abi::npc_shot_version&&bytes==sizeof(abi::NpcShotApi)?&npc_shot_api:nullptr;
}

extern "C" SIN_FIXED_EXPORT const abi::NpcAimApi* SIN_FIXED_CALL
SiN_GetNpcAimApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==abi::npc_aim_version&&bytes==sizeof(abi::NpcAimApi)?&npc_aim_api:nullptr;
}

extern "C" SIN_FIXED_EXPORT const abi::NpcAttackSpreadApi* SIN_FIXED_CALL
SiN_GetNpcAttackSpreadApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==abi::npc_attack_spread_version&&bytes==sizeof(abi::NpcAttackSpreadApi)?
        &npc_attack_spread_api:nullptr;
}

extern "C" SIN_FIXED_EXPORT const abi::CachedAttackSpreadApi* SIN_FIXED_CALL
SiN_GetCachedAttackSpreadApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==abi::cached_spread_version&&bytes==sizeof(abi::CachedAttackSpreadApi)?&cached_spread_api:nullptr;
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

bool is_weapon(EntityKind kind) noexcept {
    return kind==EntityKind::Magnum||kind==EntityKind::AssaultRifle||kind==EntityKind::Scattergun;
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
    else if(std::strcmp(classname,"weapon_magnum")==0)kind=EntityKind::Magnum;
    else if(std::strcmp(classname,"weapon_assault_rifle")==0)kind=EntityKind::AssaultRifle;
    else if(std::strcmp(classname,"weapon_scattergun")==0)kind=EntityKind::Scattergun;
    else if(std::strcmp(classname,"grenade_ar")==0)kind=EntityKind::ARGrenade;
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
    if(is_trigger(entity->kind)||entity->kind==EntityKind::ARGrenade)return abi::MapRejected;
    if(entity->kind==EntityKind::EnvGlobal){
        const int result=SiN_EnvGlobalDispatch(abi::EnvSpawn,
            entity->global_name?entity->global_name->c_str():nullptr,
            level->name.c_str(),entity->flags,entity->initial_state);
        if(result==abi::EnvRejected)return abi::MapRejected;
        if(result==abi::EnvRemoveSelf){entity->remove_requested=true;return abi::MapRemoveSelf;}
    }else if(entity->kind==EntityKind::InfoChallenge&&!capture(&entity->snapshot))return abi::MapRejected;
    entity->spawned=true;
    return abi::MapApplied;
}
struct OutputScope {
    bool prior=reading_engine_skill;
    OutputScope() noexcept {reading_engine_skill=true;}
    ~OutputScope(){reading_engine_skill=prior;}
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
    if(is_trigger(entity->kind)||is_weapon(entity->kind)||entity->kind==EntityKind::ARGrenade)return abi::MapUnhandledInput;
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
MapEntity* find_weapon(abi::MapEntityRef ref) noexcept {
    auto* entity=map_find(ref);
    return entity&&is_weapon(entity->kind)?entity:nullptr;
}
abi::NativeWeaponState weapon_state_value(const MapEntity& entity) noexcept {
    return {entity.weapon_zoom.zoom_active_0x578?1u:0u,
        entity.weapon_zoom.restore_pending_0x579?1u:0u,entity.weapon_resync?1u:0u};
}
int SIN_FIXED_CALL weapon_profile(abi::MapEntityRef ref,abi::NativeWeaponProfile* out) noexcept {
    const auto* entity=find_weapon(ref);
    if(!entity||!out)return 0;
    // Authenticated original scripts, not guesses from generic HL2 weapons.
    switch(entity->kind){
    case EntityKind::Magnum:*out={abi::WeaponMagnum,20,-1,20,0,50};break;
    case EntityKind::AssaultRifle:*out={abi::WeaponAssaultRifle,45,-1,45,0,40};break;
    case EntityKind::Scattergun:*out={abi::WeaponScattergun,6,-1,6,0,60};break;
    default:return 0;
    }
    return 1;
}
int SIN_FIXED_CALL weapon_state(abi::MapEntityRef ref,abi::NativeWeaponState* out) noexcept {
    const auto* entity=find_weapon(ref);
    if(!entity||!entity->spawned||!out)return 0;
    *out=weapon_state_value(*entity);return 1;
}
int SIN_FIXED_CALL weapon_reconcile(abi::MapEntityRef ref,std::uint32_t zoom,std::uint32_t pending) noexcept {
    auto* entity=find_weapon(ref);
    if(reading_engine_skill||!entity||!entity->spawned||zoom>1||pending>1)return 0;
    entity->weapon_zoom={zoom!=0,pending!=0};
    entity->weapon_resync=false;return 1;
}
int SIN_FIXED_CALL weapon_frame(abi::MapEntityRef ref,const abi::NativeWeaponFrame* observed,
        const abi::NativeWeaponEffects* effects,void* context,abi::NativeWeaponReport* out) noexcept {
    if(!out)return 0;
    abi::NativeWeaponReport report{};
    auto* entity=find_weapon(ref);
    if(entity)report.state=weapon_state_value(*entity);
    const auto finish=[&](std::uint32_t code) noexcept {
        report.code=code;
        if(entity)report.state=weapon_state_value(*entity);
        *out=report;return 1;
    };
    if(reading_engine_skill||!entity||!entity->spawned||entity->weapon_resync||!observed)
        return finish(abi::WeaponRejected);
    const auto in=*observed;
    if(in.has_player>1||in.player_ready>1||in.reload_active>1||in.external_block>1||
       in.fov_allowed>1||in.vr_mode>1||(in.has_player&&in.owner.epoch!=ref.epoch))
        return finish(abi::WeaponRejected);
    const sin_phase45::Inputs input{in.has_player!=0,in.player_ready!=0,
        in.reload_active!=0,in.external_block!=0,in.pressed_buttons};
    auto next=entity->weapon_zoom;
    const auto decision=sin_phase45::step(next,input);
    if(decision.transition==sin_phase45::Transition::None)return finish(abi::WeaponUnchanged);
    const bool entering=decision.transition==sin_phase45::Transition::Enter;
    const auto requests=entering?sin_phase45::enter_effects():
        sin_phase45::exit_effects(in.current_activity==sin_phase45::kZoomInActivityArgument);
    const bool fov=in.fov_allowed&&!in.vr_mode;
    const bool animation=requests.activity_argument>=0;
    if((fov||animation)&&(!effects||effects->version!=abi::weapon_version||
       effects->size!=sizeof(*effects)||(fov&&!effects->fov)||(animation&&!effects->activity)))
        return finish(abi::WeaponRejected);
    if(fov&&(entering&&in.zoom_fov<=0))return finish(abi::WeaponRejected);
    report.interrupted=decision.interrupted?1u:0u;
    report.automatic_restore=decision.automatic_restore?1u:0u;
    // Original FOV helper request precedes activity and final zoom byte
    // store. A failed foreign effect cannot safely be rolled back.
    {
        OutputScope scope;
        try {
            if(fov){
                report.fov_attempted=1;
                if(!effects->fov(context,ref,in.owner,entering?in.zoom_fov:0,requests.fov_seconds)){
                    entity->weapon_resync=true;return finish(abi::WeaponEffectsUncertain);
                }
                report.fov_delivered=1;
            }
            if(animation){
                report.activity_attempted=1;
                if(!effects->activity(context,ref,requests.activity_argument)){
                    entity->weapon_resync=true;return finish(abi::WeaponEffectsUncertain);
                }
                report.activity_delivered=1;
            }
        }catch(...){entity->weapon_resync=true;return finish(abi::WeaponEffectsUncertain);}
    }
    entity->weapon_zoom=next;
    return finish(entering?abi::WeaponEntered:abi::WeaponExited);
}
int SIN_FIXED_CALL weapon_attack_activity(abi::MapEntityRef ref,std::uint32_t secondary,
        std::int32_t recoil,std::int32_t* out) noexcept {
    const auto* entity=find_weapon(ref);
    if(reading_engine_skill||!entity||!entity->spawned||entity->weapon_resync||secondary>1||!out)return 0;
    using Kind=sin_re_phase47::WeaponKind;
    const auto kind=entity->kind==EntityKind::AssaultRifle?Kind::AssaultRifle:
        entity->kind==EntityKind::Scattergun?Kind::Scattergun:Kind::Magnum;
    const bool zoom=entity->weapon_zoom.zoom_active_0x578;
    *out=static_cast<std::int32_t>(secondary?sin_re_phase47::selectSecondaryActivity(zoom):
        sin_re_phase47::selectPrimaryActivity(kind,zoom,recoil));
    return 1;
}
const abi::WeaponBehaviorApi weapon_api{abi::weapon_version,sizeof(abi::WeaponBehaviorApi),
    &weapon_profile,&weapon_state,&weapon_reconcile,&weapon_frame,&weapon_attack_activity};

MapEntity* find_scattergun(abi::MapEntityRef ref) noexcept {
    auto* entity=find_weapon(ref);
    return entity&&entity->spawned&&entity->kind==EntityKind::Scattergun?entity:nullptr;
}
abi::ScattergunSnapshot scattergun_snapshot(const MapEntity& entity) noexcept {
    return {entity.scattergun.prior_clip_57c,entity.scattergun.prior_secondary_580,
        entity.scattergun.change_pending_589?1u:0u,entity.scattergun_resync?1u:0u};
}
int SIN_FIXED_CALL scattergun_state(abi::MapEntityRef ref,abi::ScattergunSnapshot* out) noexcept {
    const auto* entity=find_scattergun(ref);
    if(!entity||!out)return 0;
    *out=scattergun_snapshot(*entity);return 1;
}
int SIN_FIXED_CALL scattergun_reconcile(abi::MapEntityRef ref,const abi::ScattergunSnapshot* observed) noexcept {
    auto* entity=find_scattergun(ref);
    if(reading_engine_skill||!entity||!observed||observed->pending>1||observed->needs_resync)return 0;
    entity->scattergun.prior_clip_57c=observed->prior_primary;
    entity->scattergun.prior_secondary_580=observed->prior_secondary;
    entity->scattergun.change_pending_589=static_cast<std::uint8_t>(observed->pending);
    entity->scattergun_resync=false;
    return 1;
}
int SIN_FIXED_CALL scattergun_before_deploy(abi::MapEntityRef ref,std::int32_t clip) noexcept {
    auto* entity=find_scattergun(ref);
    if(reading_engine_skill||!entity||entity->scattergun_resync)return 0;
    entity->scattergun.prior_clip_57c=static_cast<std::uint32_t>(clip);
    return 1;
}
int SIN_FIXED_CALL scattergun_after_base(abi::MapEntityRef ref,const abi::ScattergunPumpObservation* input,
        const abi::ScattergunPumpHost* host,void* context,abi::ScattergunPumpReport* out) noexcept {
    if(!out)return 0;
    abi::ScattergunPumpReport report{};
    report.activity=-1;
    auto* entity=find_scattergun(ref);
    const auto finish=[&](std::uint32_t code) noexcept {
        report.code=code;
        if(entity)report.state=scattergun_snapshot(*entity);
        *out=report;return 1;
    };
    if(reading_engine_skill||!entity||entity->weapon_resync||entity->scattergun_resync||!input)
        return finish(abi::ScattergunRejected);
    const auto in=*input;
    if(in.reload_active>1||in.owner_present>1||
       (in.owner_present&&in.owner.epoch!=ref.epoch))return finish(abi::ScattergunRejected);
    auto& s=entity->scattergun;
    const auto prior=s.prior_clip_57c,secondary=s.prior_secondary_580;
    s.clip_4ec=static_cast<std::uint32_t>(in.primary_clip);
    s.secondary_tracking_578=entity->weapon_zoom.zoom_active_0x578?1u:0u;
    s.secondary_584=in.secondary_state;
    bool pending=false;
    // Phase165 verified original snapshot logic. The caller has
    // already completed the actual base callback 0x222cd050.
    sin_compat::original_server_scattergun_ammo::original_scattergun_update_ammo(
        s,[](){},[&](){pending=true;});
    report.primary_changed=s.prior_clip_57c!=prior?1u:0u;
    report.secondary_changed=s.prior_secondary_580!=secondary?1u:0u;
    if(!pending)return finish(abi::ScattergunNoPump);
    if(!std::isfinite(in.primary_deadline)||!std::isfinite(in.curtime))
        return finish(abi::ScattergunRejected);
    // Original waits only while primary deadline > curtime; equality
    // is due. Signed primary clip must be positive, and not reloading.
    if(in.primary_deadline>in.curtime||in.primary_clip<=0||in.reload_active||!in.owner_present)
        return finish(abi::ScattergunWaiting);
    const bool zoomed=entity->weapon_zoom.zoom_active_0x578;
    if(!host||host->version!=abi::scattergun_version||host->size!=sizeof(*host)||
       !host->sound||(zoomed&&!host->lookup_activity)||!host->activity||
       !host->sequence_duration||!host->clock||!host->write_deadline||!host->read_owner_deadline)
        return finish(abi::ScattergunRejected);
    // Original clears +0x589 before sound/animation callbacks.
    s.change_pending_589=0;
    OutputScope scope;
    const auto uncertain=[&]() noexcept {
        entity->scattergun_resync=true;return finish(abi::ScattergunUncertain);
    };
    const auto call=[&](auto fn,auto... args)->bool {
        ++report.callbacks_attempted;
        if(!fn(context,args...))return false;
        ++report.callbacks_delivered;return true;
    };
    try {
        if(!call(host->sound,ref,13,0.f))return uncertain();
        report.activity=0x126;
        if(zoomed&&!call(host->lookup_activity,ref,"ACT_VM_ZOOMED_PUMP",&report.activity))return uncertain();
        // Original forwards the returned activity even if lookup gives -1.
        if(!call(host->activity,ref,report.activity))return uncertain();
        float duration=0.f,now=0.f;
        if(!call(host->sequence_duration,ref,&duration)||!std::isfinite(duration)||duration<0.f||
           !call(host->clock,&now)||!std::isfinite(now))return uncertain();
        report.requested_deadline=static_cast<float>(static_cast<double>(now)+duration);
        if(!std::isfinite(report.requested_deadline)||
           !call(host->write_deadline,in.owner,abi::ScattergunOwnerDeadline,report.requested_deadline))return uncertain();
        // Original reads the actual owner field after its setter and
        // again after the first weapon timer setter, not a cached guess.
        float observed=0.f;
        if(!call(host->read_owner_deadline,in.owner,&observed)||!std::isfinite(observed)||
           !call(host->write_deadline,ref,abi::ScattergunPrimaryDeadline,observed)||
           !call(host->read_owner_deadline,in.owner,&observed)||!std::isfinite(observed)||
           !call(host->write_deadline,ref,abi::ScattergunSecondaryDeadline,observed))return uncertain();
    }catch(...){return uncertain();}
    return finish(abi::ScattergunPumped);
}
const abi::ScattergunPumpApi scattergun_api{abi::scattergun_version,sizeof(abi::ScattergunPumpApi),
    &scattergun_state,&scattergun_reconcile,&scattergun_before_deploy,&scattergun_after_base};

// Restricted nested ammo service access during the specific original
// owner query or debit callback. All other callbacks remain guarded.
struct AmmoAccess {abi::MapEntityRef owner{};std::uint32_t operation=0xffffffffu;};
AmmoAccess ammo_access{};
bool ammo_active=false;
struct AmmoWindow {
    AmmoAccess prior=ammo_access;
    AmmoWindow(abi::MapEntityRef owner,std::uint32_t op) noexcept {ammo_access={owner,op};}
    ~AmmoWindow(){ammo_access=prior;}
};
struct AmmoScope {
    AmmoScope() noexcept {ammo_active=true;}
    ~AmmoScope(){ammo_active=false;}
};
bool same_ammo_owner(abi::MapEntityRef a,abi::MapEntityRef b) noexcept {
    return a.epoch==b.epoch&&a.index==b.index&&a.generation==b.generation;
}
int SIN_FIXED_CALL combat_ammo(abi::MapEntityRef owner,const abi::AmmoRequest* input,
        const abi::AmmoHost* host,void* context,abi::AmmoReport* out) noexcept {
    if(!out)return 0;
    abi::AmmoReport report{};
    const auto finish=[&](std::uint32_t code) noexcept {report.code=code;*out=report;return 1;};
    if(!input||ammo_active||!active_level(owner.epoch)||
       owner.index==(std::numeric_limits<std::uint32_t>::max)()||
       owner.generation==(std::numeric_limits<std::uint32_t>::max)()||
       (reading_engine_skill&&(!same_ammo_owner(owner,ammo_access.owner)||
         input->operation!=ammo_access.operation)))return finish(abi::AmmoRejected);
    const auto in=*input;
    if(in.operation>abi::AmmoClear)return finish(abi::AmmoRejected);
    if(in.operation==abi::AmmoGetCount&&in.index==-1)return finish(abi::AmmoRead);
    if(in.operation==abi::AmmoRemove&&in.amount<=0)return finish(abi::AmmoUnchanged);
    if(in.operation!=abi::AmmoClear&&(in.index<0||in.index>=static_cast<std::int32_t>(abi::ammo_slots)))
        return finish(abi::AmmoRejected);
    if(!host||host->version!=abi::ammo_version||host->size!=sizeof(*host)||!host->live||
       !host->read||
       (in.operation!=abi::AmmoGetCount&&(!host->notify||!host->store)))return finish(abi::AmmoRejected);
    const auto cb=*host;
    OutputScope scope;
    AmmoScope ammo_scope;
    const auto call=[&](auto fn,auto... args){
        ++report.attempted;
        if(!fn(context,args...))throw 1;
        ++report.delivered;
    };
    const auto read=[&](std::int32_t index){
        std::int32_t v=0;call(cb.read,owner,index,&v);return v;
    };
    const auto set=[&](std::int32_t index,std::int32_t old,std::int32_t value){
        report.value=value;
        if(old==value)return;
        call(cb.notify,owner);
        call(cb.store,owner,index,value);
        ++report.changed_slots;
    };
    try {
        ++report.attempted;
        if(!cb.live(context,owner))return finish(abi::AmmoRejected);
        ++report.delivered;
        if(in.operation==abi::AmmoGetCount||in.operation==abi::AmmoRemove){
            std::int32_t max=sin_port::original_ammo::original_max_carry(in.index);
            if(cb.max_count)call(cb.max_count,in.index,&max);
            if(max==-2){
                report.infinite=1;
                if(in.operation==abi::AmmoGetCount){report.value=999;return finish(abi::AmmoRead);}
                return finish(abi::AmmoUnchanged);
            }
            const auto old=read(in.index);
            if(in.operation==abi::AmmoGetCount){report.value=old;return finish(abi::AmmoRead);}
            // Native subtract wraps as a DWORD, then clamps using signed
            // JLE. Explicit unsigned arithmetic avoids C++ signed UB.
            const auto raw=static_cast<std::uint32_t>(old)-static_cast<std::uint32_t>(in.amount);
            const auto value=(raw&0x80000000u)?0:static_cast<std::int32_t>(raw);
            set(in.index,old,value);
        }else if(in.operation==abi::AmmoSet){
            set(in.index,read(in.index),in.amount);
        }else{
            for(std::int32_t i=0;i<static_cast<std::int32_t>(abi::ammo_slots);++i)set(i,read(i),0);
        }
    }catch(...){return finish(abi::AmmoUncertain);}
    return finish(report.changed_slots?abi::AmmoChanged:abi::AmmoUnchanged);
}
const abi::CombatAmmoApi ammo_api{abi::ammo_version,sizeof(abi::CombatAmmoApi),&combat_ammo};

// Phase167 original secondary-fire ordering, with authoritative host
// writes at the actual call boundaries instead of copying live ammo/timers.
int SIN_FIXED_CALL scattergun_secondary_fire(abi::MapEntityRef ref,
        const abi::ScattergunFireObservation* observed,const abi::ScattergunFireHost* host,
        void* context,abi::ScattergunFireReport* out) noexcept {
    if(!out)return 0;
    abi::ScattergunFireReport report{};
    auto* entity=find_scattergun(ref);
    const auto finish=[&](std::uint32_t code) noexcept {
        report.code=code;
        report.needs_resync=entity&&entity->scattergun_resync?1u:0u;
        *out=report;return 1;
    };
    if(reading_engine_skill||!entity||entity->weapon_resync||entity->scattergun_resync||
       !observed||observed->owner_present>1)return finish(abi::FireRejected);
    const auto in=*observed;
    if(!in.owner_present)return finish(abi::FireMissingOwner);
    if(in.owner.epoch!=ref.epoch||in.owner.index==(std::numeric_limits<std::uint32_t>::max)()||
       in.owner.generation==(std::numeric_limits<std::uint32_t>::max)()||
       !host||host->version!=abi::scattergun_fire_version||host->size!=sizeof(*host)||
       !host->query||!host->effect)return finish(abi::FireRejected);
    const auto callbacks=*host;
    OutputScope scope;
    const auto query=[&](std::uint32_t op,std::uint32_t arg=0u){
        abi::ScattergunFireValue value{};
        ++report.queries_attempted;
        AmmoWindow window(in.owner,op==abi::FireOwnerAmmoCount?abi::AmmoGetCount:0xffffffffu);
        if(!callbacks.query(context,ref,in.owner,op,arg,&value))throw 1;
        ++report.queries_delivered;
        return value;
    };
    const auto boolean=[&](std::uint32_t op){
        const auto value=query(op).word0;
        if(value>1)throw 1;
        return value!=0;
    };
    const auto effect=[&](std::uint32_t op,std::uint32_t a=0u,std::uint32_t b=0u,
                           float x=0.f,float y=0.f){
        const abi::ScattergunFireValue value{a,b,x,y};
        ++report.effects_attempted;
        AmmoWindow window(in.owner,op==abi::FireConsumeReserve?abi::AmmoRemove:0xffffffffu);
        if(!callbacks.effect(context,ref,in.owner,op,&value))throw 1;
        ++report.effects_delivered;
    };
    const auto real=[&](std::uint32_t op,std::uint32_t arg=0u){
        const float v=query(op,arg).real0;
        if(!std::isfinite(v))throw 1;
        return v;
    };
    const auto deadline=[&](float interval){
        const float now=real(abi::FireClock);
        const float value=static_cast<float>(static_cast<double>(interval)+now);
        if(!std::isfinite(value))throw 1;
        return value;
    };
    try {
        const bool animate=boolean(abi::FireOwnerCapable);
        if(query(abi::FireOwnerMode).word0==3u){
            effect(abi::FireStateEvent,0,0);
            const float when=static_cast<float>(static_cast<double>(real(abi::FireClock))+0.2);
            if(!std::isfinite(when))throw 1;
            effect(abi::FireSecondaryTimer,0,0,when);
            return finish(abi::FireModeCooldown);
        }
        if(!boolean(abi::FireAcquireVectors))return finish(abi::FireNoVectors);
        if(boolean(abi::FireUsesClip)){
            const auto clip=query(abi::FireReadClip).word0;
            if(clip==0u||(clip&0x80000000u)){
                effect(abi::FireEmpty);return finish(abi::FireEmptyClip);
            }
        }
        effect(abi::FireStateEvent,3,0);
        effect(abi::FireActivity,query(abi::FireSelectActivity).word0);
        const float interval=real(abi::FireInterval,in.timing_arg);
        effect(abi::FirePrimaryTimer,0,0,deadline(interval));
        effect(abi::FireSecondaryTimer,0,0,deadline(interval));
        effect(abi::FireOwnerCallback);
        if(animate){
            effect(abi::FireOwnerAnimation,5);
            // The second clip-mode test is an independent original call.
            if(boolean(abi::FireUsesClip))effect(abi::FireConsumeClip);
            else effect(abi::FireConsumeReserve,1,in.alternate_ammo_type);
            const auto type=query(abi::FireAmmoType).word0;
            const auto count=query(abi::FireOwnerAmmoCount,type).word0;
            effect(abi::FireStoreSecondary,count);
            effect(abi::FirePost);
            effect(abi::FireFeedback,6);
        }
        effect(abi::FirePreparePellets);
        for(unsigned i=0;i<8;++i){
            effect(abi::FirePellet,i,0,1.f,1500.f);
            ++report.pellets_delivered;
        }
    }catch(...){
        // Calls already delivered cannot be undone. Even a failed query
        // may have advanced engine state; require observed reconciliation.
        entity->scattergun_resync=true;
        return finish(abi::FireUncertain);
    }
    return finish(abi::FireCompleted);
}
const abi::ScattergunSecondaryApi scattergun_fire_api{abi::scattergun_fire_version,
    sizeof(abi::ScattergunSecondaryApi),&scattergun_secondary_fire};

namespace grenade=sin_compat::ar_grenade;
namespace grenade_touch=sin_compat::ar_grenade_touch;
MapEntity* find_grenade(abi::MapEntityRef ref) noexcept {
    auto* entity=map_find(ref);
    return entity&&entity->kind==EntityKind::ARGrenade?entity:nullptr;
}
grenade::Vec3 grenade_vector(const abi::NpcSpread& v) noexcept {return {v.x,v.y,v.z};}
abi::NpcSpread grenade_vector(const grenade::Vec3& v) noexcept {return {v.x,v.y,v.z};}
abi::GrenadeSnapshot grenade_snapshot(const MapEntity& e) noexcept {
    return {e.grenade.armed()?1u:0u,e.grenade_contact?1u:0u,e.grenade_uncertain?1u:0u,
        e.remove_requested?1u:0u,e.grenade.danger_radius(),e.grenade.next_think_seconds(),
        e.grenade.damage(),e.grenade.blast_radius(),grenade_vector(e.grenade.origin()),
        grenade_vector(e.grenade.velocity())};
}
bool grenade_ready(const MapEntity* e) noexcept {return e&&e->spawned&&!e->grenade_uncertain;}
int SIN_FIXED_CALL grenade_spawn(abi::MapEntityRef ref,const abi::GrenadeSpawn* input) noexcept {
    auto* e=find_grenade(ref);
    if(reading_engine_skill||!e||e->spawned||!input)return 0;
    if(!e->grenade.spawn(grenade_vector(input->origin),grenade_vector(input->velocity),input->time))return 0;
    e->spawned=true;
    return 1;
}
int SIN_FIXED_CALL grenade_state(abi::MapEntityRef ref,abi::GrenadeSnapshot* out) noexcept {
    const auto* e=find_grenade(ref);
    if(!e||!e->spawned||!out)return 0;
    *out=grenade_snapshot(*e);return 1;
}
int SIN_FIXED_CALL grenade_kinematics(abi::MapEntityRef ref,const abi::NpcSpread* origin,
        const abi::NpcSpread* velocity) noexcept {
    auto* e=find_grenade(ref);
    return !reading_engine_skill&&grenade_ready(e)&&origin&&velocity&&
        e->grenade.update_kinematics(grenade_vector(*origin),grenade_vector(*velocity))?1:0;
}
int SIN_FIXED_CALL grenade_observe_contact(abi::MapEntityRef ref,std::uint32_t value) noexcept {
    auto* e=find_grenade(ref);
    if(reading_engine_skill||!grenade_ready(e)||!e->grenade.armed()||value>1)return 0;
    e->grenade_contact=value!=0;return 1;
}
bool valid_grenade_host(const abi::GrenadeHost* host) noexcept {
    return host&&host->version==abi::grenade_version&&host->size==sizeof(*host)&&host->emit;
}
class GrenadeBridge final:public grenade::IHost {
public:
    GrenadeBridge(MapEntity& entity,abi::MapEntityRef ref,const abi::GrenadeHost* host,
        void* context,abi::GrenadeReport& report):entity_(entity),ref_(ref),
        emit_(host?host->emit:nullptr),context_(context),report_(report){}
    void emit(abi::GrenadeEffect effect){
        ++report_.effects_attempted;
        if(!emit_||!emit_(context_,ref_,&effect))throw 1;
        ++report_.effects_delivered;
    }
    void on_flight_ai_sound(const grenade::AiSoundEvent& e) override {sound(abi::GrenadeFlightSound,e);}
    void on_original_trace_effect(grenade::Decal decal) override {
        abi::GrenadeEffect e{};e.kind=abi::GrenadeTraceEffect;
        e.decal=static_cast<std::uint32_t>(decal);e.origin=grenade_vector(entity_.grenade.origin());emit(e);
    }
    void on_screen_shake_feedback(const grenade::ScreenShakeEvent& v) override {
        abi::GrenadeEffect e{};e.kind=abi::GrenadeShake;e.origin=grenade_vector(v.origin);
        e.radius=v.radius;e.duration=v.duration_seconds;e.requested_amplitude=v.requested_amplitude;
        e.clamped_amplitude=v.engine_clamped_amplitude;e.frequency=v.frequency;emit(e);
    }
    void on_radius_damage(const grenade::BlastDamageEvent& v) override {
        abi::GrenadeEffect e{};e.kind=abi::GrenadeRadiusDamage;e.origin=grenade_vector(v.origin);
        e.type=v.damage_type;e.damage=v.damage;e.radius=v.radius;emit(e);
    }
    void on_explosion_ai_sound(const grenade::AiSoundEvent& e) override {sound(abi::GrenadeExplosionSound,e);}
    void on_remove_projectile() override {
        abi::GrenadeEffect e{};e.kind=abi::GrenadeQueueRemove;emit(e);entity_.remove_requested=true;
    }
    void flag(bool set){
        abi::GrenadeEffect e{};e.kind=set?abi::GrenadeSetContact:abi::GrenadeClearContact;
        emit(e);entity_.grenade_contact=set;
    }
private:
    void sound(std::uint32_t kind,const grenade::AiSoundEvent& v){
        abi::GrenadeEffect e{};e.kind=kind;e.type=v.type;e.channel=static_cast<std::uint32_t>(v.channel);
        e.origin=grenade_vector(v.origin);e.radius=v.radius;e.duration=v.duration_seconds;emit(e);
    }
    MapEntity& entity_;
    abi::MapEntityRef ref_;
    decltype(abi::GrenadeHost::emit) emit_;
    void* context_;
    abi::GrenadeReport& report_;
};
std::uint32_t grenade_result(grenade::StepResult code) noexcept {
    switch(code){
    case grenade::StepResult::Applied:return abi::GrenadeApplied;
    case grenade::StepResult::NotDue:return abi::GrenadeNotDue;
    case grenade::StepResult::Disarmed:return abi::GrenadeDisarmed;
    case grenade::StepResult::TriggerRejected:return abi::GrenadeBlocked;
    default:return abi::GrenadeRejected;
    }
}
int grenade_finish(const MapEntity* e,abi::GrenadeReport& report,
        std::uint32_t code,abi::GrenadeReport* out) noexcept {
    report.code=code;
    if(e&&e->spawned)report.state=grenade_snapshot(*e);
    *out=report;return 1;
}
int SIN_FIXED_CALL grenade_think(abi::MapEntityRef ref,float time,std::uint32_t ai,std::uint32_t increment,
        const abi::GrenadeHost* host,void* context,abi::GrenadeReport* out) noexcept {
    if(!out)return 0;
    abi::GrenadeReport report{};report.touch_gate=0xffffffffu;
    auto* e=find_grenade(ref);
    if(reading_engine_skill||!grenade_ready(e)||ai>1||increment>1||
       (ai&&!valid_grenade_host(host))||(host&&!valid_grenade_host(host)))
        return grenade_finish(e,report,abi::GrenadeRejected,out);
    OutputScope scope;
    GrenadeBridge bridge(*e,ref,host,context,report);
    try {
        return grenade_finish(e,report,grenade_result(e->grenade.think(time,ai!=0,increment!=0,bridge)),out);
    }catch(...){e->grenade_uncertain=true;return grenade_finish(e,report,abi::GrenadeUncertain,out);}
}
int grenade_detonate_impl(MapEntity& e,abi::MapEntityRef ref,std::uint32_t decal,bool set,
        const abi::GrenadeHost* host,void* context,abi::GrenadeReport& report,abi::GrenadeReport* out) noexcept {
    if(!e.grenade.armed())return grenade_finish(&e,report,abi::GrenadeDisarmed,out);
    if(!set&&!e.grenade_contact)return grenade_finish(&e,report,abi::GrenadeBlocked,out);
    if(decal>static_cast<std::uint32_t>(grenade::Decal::SmallScorch)||!valid_grenade_host(host))
        return grenade_finish(&e,report,abi::GrenadeRejected,out);
    OutputScope scope;
    GrenadeBridge bridge(e,ref,host,context,report);
    try {
        if(set)bridge.flag(true);
        // Original detonation clears +0x795 before its effect path.
        bridge.flag(false);
        return grenade_finish(&e,report,grenade_result(e.grenade.detonate(true,
            static_cast<grenade::Decal>(decal),bridge)),out);
    }catch(...){e.grenade_uncertain=true;return grenade_finish(&e,report,abi::GrenadeUncertain,out);}
}
int SIN_FIXED_CALL grenade_detonate(abi::MapEntityRef ref,std::uint32_t decal,const abi::GrenadeHost* host,
        void* context,abi::GrenadeReport* out) noexcept {
    if(!out)return 0;
    abi::GrenadeReport report{};report.touch_gate=0xffffffffu;
    auto* e=find_grenade(ref);
    if(reading_engine_skill||!grenade_ready(e))return grenade_finish(e,report,abi::GrenadeRejected,out);
    return grenade_detonate_impl(*e,ref,decal,false,host,context,report,out);
}
int SIN_FIXED_CALL grenade_touch_dispatch(abi::MapEntityRef ref,const abi::GrenadeTouch* input,
        const abi::GrenadeHost* host,void* context,abi::GrenadeReport* out) noexcept {
    if(!out)return 0;
    abi::GrenadeReport report{};report.touch_gate=0xffffffffu;
    auto* e=find_grenade(ref);
    if(reading_engine_skill||!grenade_ready(e)||!input||input->other_present>1||
       input->other_flags_120>255||input->virtual_fc_nonnull>1||input->virtual_fc_matches>1)
        return grenade_finish(e,report,abi::GrenadeRejected,out);
    if(!e->grenade.armed())return grenade_finish(e,report,abi::GrenadeDisarmed,out);
    const grenade_touch::OriginalTouchObservation observation{input->other_present!=0,
        input->other_field_11c,static_cast<std::uint8_t>(input->other_flags_120),
        input->virtual_fc_nonnull!=0,input->virtual_fc_matches!=0,e->grenade_contact};
    const auto decision=grenade_touch::evaluate(observation);
    report.touch_gate=static_cast<std::uint32_t>(decision.gate);
    if(!decision.should_dispatch_detonation())return grenade_finish(e,report,abi::GrenadeBlocked,out);
    return grenade_detonate_impl(*e,ref,input->decal,decision.original_flag_795_set_requested,
        host,context,report,out);
}
const abi::ARGrenadeApi grenade_api{abi::grenade_version,sizeof(abi::ARGrenadeApi),
    &grenade_spawn,&grenade_state,&grenade_kinematics,&grenade_observe_contact,
    &grenade_think,&grenade_touch_dispatch,&grenade_detonate};

MapEntity* find_rifle(abi::MapEntityRef ref) noexcept {
    auto* e=find_weapon(ref);
    return e&&e->spawned&&e->kind==EntityKind::AssaultRifle?e:nullptr;
}
int SIN_FIXED_CALL rifle_launch_status(abi::MapEntityRef ref) noexcept {
    const auto* e=find_rifle(ref);
    return e?(e->rifle_launch_uncertain?1:0):-1;
}
bool valid_launch_ref(abi::MapEntityRef e,std::uint32_t epoch) noexcept {
    return e.epoch==epoch&&e.index!=(std::numeric_limits<std::uint32_t>::max)()&&
        e.generation!=(std::numeric_limits<std::uint32_t>::max)();
}
int SIN_FIXED_CALL rifle_launch(abi::MapEntityRef ref,const abi::RifleLaunchInput* input,
        const abi::RifleLaunchHost* host,void* context,abi::RifleLaunchReport* out) noexcept {
    if(!out)return 0;
    abi::RifleLaunchReport report{};
    auto* rifle=find_rifle(ref);
    const auto finish=[&](std::uint32_t code) noexcept {
        report.code=code;report.uncertain=rifle&&rifle->rifle_launch_uncertain?1u:0u;
        *out=report;return 1;
    };
    if(reading_engine_skill||!rifle||rifle->weapon_resync||rifle->rifle_launch_uncertain||
       !input||!valid_launch_ref(input->owner,ref.epoch)||input->owner.index==ref.index||
       !host||host->version!=abi::rifle_launch_version||host->size!=sizeof(*host)||
       !host->authorize||!host->prepare||!host->commit||!host->abort)
        return finish(abi::RifleRejected);
    const auto in=*input;
    const auto cb=*host;
    OutputScope scope;
    const auto uncertain=[&](MapEntity* projectile=nullptr) noexcept {
        rifle->rifle_launch_uncertain=true;
        if(projectile){projectile->alive=true;projectile->grenade_uncertain=true;}
        return finish(abi::RifleLaunchUncertain);
    };
    abi::RifleLaunchObservation auth{};
    try {
        AmmoWindow window(in.owner,abi::AmmoGetCount);
        if(!cb.authorize(context,ref,in.owner,&auth))return finish(abi::RifleUnauthorized);
    }catch(...){return uncertain();}
    if(auth.live_owner_and_weapon!=1||auth.correct_owner!=1||auth.selected>1||
       auth.reload_active>1||auth.holstered>1||auth.grunt_authorized>1)
        return finish(abi::RifleUnauthorized);
    if(auth.owner_kind==abi::RifleOwnerPlayer){
        if(!auth.selected||auth.reload_active||auth.holstered)return finish(abi::RifleUnauthorized);
        if(auth.ammo_kind!=abi::RifleAmmoFinite)return finish(abi::RifleAmmoUnverified);
        if(auth.ammo_remaining<1)return finish(abi::RifleNoAmmo);
        report.multiplier=2000.f;report.planned_debit=1;
    }else if(auth.owner_kind==abi::RifleOwnerGrunt&&auth.grunt_authorized){
        report.multiplier=1000.f;
    }else return finish(abi::RifleUnauthorized);
    const abi::GrenadeSpawn spawn{in.origin,
        {in.direction.x*report.multiplier,in.direction.y*report.multiplier,in.direction.z*report.multiplier},in.time};
    MapEntity candidate;
    candidate.kind=EntityKind::ARGrenade;
    candidate.alive=false;candidate.spawned=true;
    if(!candidate.grenade.spawn(grenade_vector(spawn.origin),grenade_vector(spawn.velocity),spawn.time))
        return finish(abi::RifleInvalidInput);
    // Refuse before allocating a shell if even a fresh slot is
    // impossible. Existing retired indices remain reusable.
    const auto limit=level->entities.size()>=max_map_entities;
    if(limit){
        bool retired=false;
        for(const auto& item:level->entities)
            if(!item.second.alive){retired=true;break;}
        if(!retired)return finish(abi::RifleCapacityRefused);
    }
    try {
        if(!cb.prepare(context,in.owner,grenade::entity_classname,grenade::original_model,
            &spawn,&report.projectile))return finish(abi::RiflePrepareRefused);
    }catch(...){return uncertain();}
    report.prepared=1;
    const auto projectile=report.projectile;
    if(!valid_launch_ref(projectile,ref.epoch)||projectile.index==ref.index||
       projectile.index==in.owner.index)return uncertain();
    auto it=level->entities.find(projectile.index);
    // Never abort a collision with a live or stale identity: the returned
    // handle might designate a previously existing engine entity.
    if(it!=level->entities.end()&&(it->second.alive||projectile.generation<=it->second.generation))
        return uncertain();
    MapEntity* staged=nullptr;
    const auto abort=[&](std::uint32_t code) noexcept {
        report.abort_attempted=1;
        try {
            if(!cb.abort(context,projectile))return uncertain(staged);
        }catch(...){return uncertain(staged);}
        report.aborted=1;
        return finish(code);
    };
    if(it==level->entities.end()&&limit)return abort(abi::RifleCapacityRefused);
    candidate.generation=projectile.generation;
    try {
        if(it==level->entities.end())
            it=level->entities.emplace(projectile.index,std::move(candidate)).first;
        else it->second=std::move(candidate);
        staged=&it->second;
    }catch(...){return abort(abi::RifleCapacityRefused);}
    report.attached=1;
    // The staged registry entry is not live yet, including during
    // commit. Engine callbacks cannot discover or mutate it.
    report.commit_attempted=1;
    try {
        if(!cb.commit(context,projectile,in.owner,report.planned_debit))
            return abort(abi::RifleCommitRefused);
    }catch(...){return uncertain(staged);}
    report.committed=1;
    staged->alive=true;
    return finish(abi::RifleLaunched);
}
const abi::RifleGrenadeApi rifle_launch_api{abi::rifle_launch_version,sizeof(abi::RifleGrenadeApi),
    &rifle_launch,&rifle_launch_status};

MapEntity* find_magnum(abi::MapEntityRef ref) noexcept {
    auto* e=find_weapon(ref);
    return e&&e->spawned&&e->kind==EntityKind::Magnum?e:nullptr;
}
// Narrow synchronous nesting: an actual game firing callback can
// call this exact weapon's impact handler. No other mutation or
// recursive impact is enabled, and the outer guard is restored.
MapEntity* magnum_impact_target=nullptr;
bool magnum_impact_active=false;
struct MagnumImpactWindow {
    MapEntity* prior=magnum_impact_target;
    explicit MagnumImpactWindow(MapEntity* e) noexcept {magnum_impact_target=e;}
    ~MagnumImpactWindow(){magnum_impact_target=prior;}
};
struct MagnumImpactScope {
    MagnumImpactScope() noexcept {magnum_impact_active=true;}
    ~MagnumImpactScope(){magnum_impact_active=false;}
};
int SIN_FIXED_CALL magnum_acknowledge(abi::MapEntityRef ref) noexcept {
    auto* e=find_magnum(ref);
    if(reading_engine_skill||!e)return 0;
    e->magnum_uncertain=false;return 1;
}
// Original slot216 0x22316b30. This is the before-base deferred
// primary path, not a reconstruction of primary attack itself.
int SIN_FIXED_CALL magnum_frame(abi::MapEntityRef ref,const abi::MagnumFrameObservation* input,
        const abi::MagnumFrameHost* host,void* context,abi::MagnumFrameReport* out) noexcept {
    if(!out)return 0;
    abi::MagnumFrameReport report{};
    auto* e=find_magnum(ref);
    const auto finish=[&](std::uint32_t code) noexcept {
        report.code=code;report.needs_resync=e&&e->magnum_uncertain?1u:0u;
        *out=report;return 1;
    };
    if(reading_engine_skill||!e||e->weapon_resync||e->magnum_uncertain||!input||
       !host||host->version!=abi::magnum_version||host->size!=sizeof(*host)||!host->base_update)
        return finish(abi::MagnumFrameRejected);
    const auto in=*input;
    const auto cb=*host;
    report.pending_after=in.pending_count;
    if(in.pending_count>0&&(!cb.clock||!cb.read_primary_deadline||!cb.write_primary_deadline||
       !cb.write_pending_count||!cb.primary_attack||!cb.read_pending_count||!cb.write_pending_deadline))
        return finish(abi::MagnumFrameRejected);
    OutputScope scope;
    const auto call=[&](auto fn,auto... args){
        ++report.attempted;
        if(!fn(context,args...))throw 1;
        ++report.delivered;
    };
    const auto clock=[&](){
        float now=0.f;call(cb.clock,&now);
        if(!std::isfinite(now))throw 1;
        return now;
    };
    try {
        bool fired=false;
        if(in.pending_count>0){
            const float now=clock();
            // Original x87 unordered comparison does not enter the due
            // branch. Keep NaN in the skip path rather than firing.
            if(in.pending_deadline<=now){
                float saved=0.f;
                call(cb.read_primary_deadline,ref,&saved);
                if(!std::isfinite(saved))throw 1;
                // The original setter reads gpGlobals+0xc again.
                call(cb.write_primary_deadline,ref,clock());
                call(cb.write_pending_count,ref,in.pending_count-1);
                report.pending_after=in.pending_count-1;
                {
                    MagnumImpactWindow window(e);
                    call(cb.primary_attack,ref);
                    report.primary_delivered=1;
                    if(e->magnum_uncertain)throw 1;
                }
                call(cb.write_primary_deadline,ref,saved);
                report.restored_deadline=saved;
                call(cb.read_pending_count,ref,&report.pending_after);
                if(report.pending_after>0){
                    const float next=static_cast<float>(static_cast<double>(clock())+static_cast<double>(0.05f));
                    if(!std::isfinite(next))throw 1;
                    call(cb.write_pending_deadline,ref,next);
                    report.next_pending_deadline=next;
                }
                fired=true;
            }
        }
        call(cb.base_update,ref);report.base_delivered=1;
        return finish(fired?abi::MagnumDeferredPrimary:abi::MagnumBaseOnly);
    }catch(...){e->magnum_uncertain=true;return finish(abi::MagnumFrameUncertain);}
}
// Original slot102 0x22316910: base first, DUAG bit, trace gate,
// CPVSFilter + CTESmoke, then two original decal virtual calls.
int SIN_FIXED_CALL magnum_impact(abi::MapEntityRef ref,std::uint32_t damage_type,
        const abi::MagnumImpactHost* host,void* context,abi::MagnumImpactReport* out) noexcept {
    if(!out)return 0;
    abi::MagnumImpactReport report{};
    auto* e=find_magnum(ref);
    const auto finish=[&](std::uint32_t code) noexcept {
        report.code=code;report.needs_resync=e&&e->magnum_uncertain?1u:0u;
        *out=report;return 1;
    };
    const bool special=(damage_type&0x00400000u)!=0;
    if(!e||magnum_impact_active||(reading_engine_skill&&magnum_impact_target!=e)||
       e->magnum_uncertain||!host||host->version!=abi::magnum_version||
       host->size!=sizeof(*host)||!host->base||(special&&(!host->trace||!host->smoke||!host->decal)))
        return finish(abi::MagnumImpactRejected);
    const auto cb=*host;
    OutputScope scope;
    MagnumImpactScope impact_scope;
    const auto call=[&](auto fn,auto... args){
        ++report.attempted;
        if(!fn(context,args...))throw 1;
        ++report.delivered;
    };
    try {
        call(cb.base,ref,damage_type);report.base_delivered=1;
        if(!special)return finish(abi::MagnumImpactBaseOnly);
        abi::MagnumTraceObservation trace{};
        call(cb.trace,ref,&trace);
        if(trace.flags_42>255u)throw 1;
        if(trace.flags_42&4u)return finish(abi::MagnumImpactTraceBlocked);
        if(!finite_vector(trace.start)||!finite_vector(trace.end))throw 1;
        // Original subtracts three components with f32 stores, then
        // calls its normalize dispatch. Reproduce the authenticated
        // default 0x22268100 epsilon path for normal finite inputs.
        const abi::NpcSpread delta{trace.end.x-trace.start.x,trace.end.y-trace.start.y,trace.end.z-trace.start.z};
        if(!finite_vector(delta))throw 1;
        const double length=std::sqrt(static_cast<double>(delta.x)*delta.x+
            static_cast<double>(delta.y)*delta.y+static_cast<double>(delta.z)*delta.z);
        const double inv=1.0/(length+static_cast<double>(std::numeric_limits<float>::epsilon()));
        const abi::NpcSpread pos{static_cast<float>(delta.x*inv),static_cast<float>(delta.y*inv),
            static_cast<float>(delta.z*inv)};
        // The original builds the recipient filter at trace.end,
        // but passes the normalized difference to the temp entity.
        const abi::MagnumSmoke smoke{trace.end,pos,0.25f,2.5f,10,"sprites/blueglow1.vmt"};
        call(cb.smoke,ref,&smoke);report.smoke_delivered=1;
        call(cb.decal,ref,"DUAGDecal1");++report.decals_delivered;
        call(cb.decal,ref,"DUAGDecal2");++report.decals_delivered;
        return finish(abi::MagnumImpactDUAG);
    }catch(...){e->magnum_uncertain=true;return finish(abi::MagnumImpactUncertain);}
}
const abi::MagnumApi magnum_api{abi::magnum_version,sizeof(abi::MagnumApi),
    &magnum_frame,&magnum_impact,&magnum_acknowledge};

// Original Magnum slot227 (0x22316cf0). Keep the original early
// exact-zero clip check, live repeat queries and post-fire ordering.
int SIN_FIXED_CALL magnum_secondary(abi::MapEntityRef ref,const abi::MagnumSecondaryObservation* input,
        const abi::MagnumSecondaryHost* host,void* context,abi::MagnumSecondaryReport* out) noexcept {
    if(!out)return 0;
    abi::MagnumSecondaryReport report{};
    auto* e=find_magnum(ref);
    const auto finish=[&](std::uint32_t code) noexcept {
        report.code=code;report.needs_resync=e&&e->magnum_uncertain?1u:0u;
        *out=report;return 1;
    };
    if(reading_engine_skill||!e||e->weapon_resync||e->magnum_uncertain||!input||
       input->owner_present>1||!host||host->version!=abi::magnum_secondary_version||
       host->size!=sizeof(*host)||!host->query||!host->effect||!host->fire)
        return finish(abi::MagSecondaryRejected);
    const auto in=*input;
    if(in.owner_present&&(!valid_launch_ref(in.owner,ref.epoch)||in.owner.index==ref.index))
        return finish(abi::MagSecondaryRejected);
    const auto cb=*host;
    OutputScope scope;
    const auto q=[&](std::uint32_t op,std::uint32_t arg=0u){
        abi::MagnumSecondaryValue v{};
        ++report.queries_attempted;
        AmmoWindow window(in.owner,op==abi::MagSecondaryReserveCount?abi::AmmoGetCount:0xffffffffu);
        if(!cb.query(context,ref,in.owner,op,arg,&v))throw 1;
        ++report.queries_delivered;return v;
    };
    const auto boolean=[&](std::uint32_t op){
        const auto v=q(op).integer;
        if(v!=0&&v!=1)throw 1;
        return v!=0;
    };
    const auto real=[&](std::uint32_t op,std::uint32_t arg=0u){
        const float v=q(op,arg).real;
        if(!std::isfinite(v))throw 1;
        return v;
    };
    const auto vector=[&](std::uint32_t op,std::uint32_t arg=0u){
        const auto v=q(op,arg).vector;
        if(!finite_vector(v))throw 1;
        return v;
    };
    const auto effect=[&](std::uint32_t op,std::int32_t word=0,float f=0.f,
                          abi::NpcSpread v={},std::int32_t extra=0){
        const abi::MagnumSecondaryValue value{word,f,v};
        ++report.effects_attempted;
        AmmoWindow window(in.owner,op==abi::MagSecondaryDebitReserve?abi::AmmoRemove:0xffffffffu);
        if(!cb.effect(context,ref,in.owner,op,&value,extra))throw 1;
        ++report.effects_delivered;
    };
    const auto deadline=[&](float interval){
        const float value=static_cast<float>(static_cast<double>(interval)+real(abi::MagSecondaryClock));
        if(!std::isfinite(value))throw 1;
        return value;
    };
    try {
        if(boolean(abi::MagSecondaryUsesClip)&&q(abi::MagSecondaryReadClip).integer==0){
            effect(abi::MagSecondaryEmpty);return finish(abi::MagSecondaryEmptyClip);
        }
        // Retail dereferences the owner later even on the fallback
        // branch. Refuse a missing live owner rather than emulate a crash.
        if(!in.owner_present)return finish(abi::MagSecondaryRejected);
        const bool direct=boolean(abi::MagSecondaryOwnerCapable);
        effect(abi::MagSecondaryEvent,3,0.f,{},0);
        effect(abi::MagSecondaryActivity,q(abi::MagSecondarySelection).integer);
        const float interval=real(abi::MagSecondaryInterval,in.timing_arg);
        effect(abi::MagSecondaryPrimaryTimer,0,deadline(interval));
        effect(abi::MagSecondarySecondaryTimer,0,deadline(interval));
        if(direct){
            effect(abi::MagSecondaryOwnerFire);
            effect(abi::MagSecondaryOwnerAnimation,5);
        }
        auto& record=report.record;
        record.shots=1;record.weapon=ref;record.tracer_frequency=1;
        record.distance=56755.83984375f;record.force_scale=1.f;
        record.origin=vector(abi::MagSecondaryOrigin);
        record.direction=direct?vector(abi::MagSecondaryDirectVector,0x3db27eb6u):
            vector(abi::MagSecondaryFallbackVector);
        if(direct){
            if(boolean(abi::MagSecondaryUsesClip)){
                const auto clip=q(abi::MagSecondaryReadClip).integer;
                // Original signed minimum, even if the host reports
                // negative values. The original subtract helper owns
                // wrap/dirty notification, not this record.
                record.shots=clip<1?clip:1;
                effect(abi::MagSecondaryDebitClip,record.shots);
            }else{
                auto type=q(abi::MagSecondaryAmmoType).integer;
                const auto count=q(abi::MagSecondaryReserveCount,static_cast<std::uint32_t>(type)).integer;
                if(record.shots>=count){
                    type=q(abi::MagSecondaryAmmoType).integer;
                    record.shots=q(abi::MagSecondaryReserveCount,static_cast<std::uint32_t>(type)).integer;
                }
                type=q(abi::MagSecondaryAmmoType).integer;
                effect(abi::MagSecondaryDebitReserve,record.shots,0.f,{},type);
            }
        }
        record.ammo_type=q(abi::MagSecondaryAmmoType).integer;
        record.spread=vector(abi::MagSecondarySpread);
        if(direct&&!boolean(abi::MagSecondaryZoomed)){
            if(static_cast<std::uint32_t>(q(abi::MagSecondaryOwnerFlags).integer)&0x800u)
                effect(abi::MagSecondaryOwnerFlagHelper);
            const auto v=vector(abi::MagSecondaryAuxVector);
            // Exact f32 stores of the game's auxiliary point. This
            // field is not assigned a guessed source-level name.
            record.aux_position={
                static_cast<float>(record.origin.x+static_cast<float>(v.x*4.f)),
                static_cast<float>(record.origin.y+static_cast<float>(v.y*4.f)),
                static_cast<float>(record.origin.z+static_cast<float>(v.z*4.f))};
            if(!finite_vector(record.aux_position))throw 1;
            record.aux_enabled=1;
        }
        {
            MagnumImpactWindow window(e);
            report.fire_attempted=1;
            if(!cb.fire(context,in.owner,&record))throw 1;
            report.fire_delivered=1;
            if(e->magnum_uncertain)throw 1;
        }
        if(direct){
            effect(abi::MagSecondaryFeedback,6);
            auto angles=vector(abi::MagSecondaryOwnerAngles);
            const auto r1=q(abi::MagSecondaryRandomInt).integer;
            const auto r2=q(abi::MagSecondaryRandomInt).integer;
            if(r1< -1||r1>1||r2< -1||r2>1)throw 1;
            angles.x+=static_cast<float>(r1);angles.y+=static_cast<float>(r2);angles.z=0.f;
            if(!finite_vector(angles))throw 1;
            effect(abi::MagSecondaryAngleHelper,0,0.f,angles);
            const auto punch=real(abi::MagSecondaryRandomFloat);
            if(punch< -2.f||punch>2.f)throw 1;
            effect(abi::MagSecondaryPunch,0,0.f,{-8.f,punch,0.f});
        }
    }catch(...){e->magnum_uncertain=true;return finish(abi::MagSecondaryUncertain);}
    return finish(abi::MagSecondaryCompleted);
}
const abi::MagnumSecondaryApi magnum_secondary_api{abi::magnum_secondary_version,
    sizeof(abi::MagnumSecondaryApi),&magnum_secondary};

int SIN_FIXED_CALL weapon_snapshot_capture(abi::MapEntityRef ref,std::uint32_t viewer,
        const abi::WeaponSnapshotHost* host,void* context,sin_port::weapon_net::Payload* out) noexcept {
    namespace net=sin_port::weapon_net;
    const auto* e=find_weapon(ref);
    if(reading_engine_skill||!e||!e->spawned||e->remove_requested||!out||
       viewer<1||viewer>256||!host||host->version!=net::version||
       host->size!=sizeof(*host)||!host->owner||!host->read)return 0;
    const auto cb=*host;
    OutputScope scope;
    net::OwnerObservation owner{};
    net::RecipientMask recipients{};
    net::Values values{};
    const auto& contract=net::owned_api();
    try {
        if(!cb.owner(context,ref,&owner)||!contract.recipients(&owner,&recipients))return 0;
        values.words[net::Owner]=owner.native_handle;
        const auto slot=viewer-1;
        const bool local=(recipients.words[slot/32]&(1u<<(slot%32)))!=0;
        const auto mask=local?net::all_mask:net::public_mask;
        for(unsigned i=0;i<net::Owner;++i)
            if((mask&(1u<<i))&&!cb.read(context,ref,i,&values.words[i]))return 0;
    }catch(...){return 0;}
    return contract.encode(&values,&owner,viewer,out);
}
const abi::WeaponSnapshotApi weapon_snapshot_api{sin_port::weapon_net::version,
    sizeof(abi::WeaponSnapshotApi),&weapon_snapshot_capture};

int SIN_FIXED_CALL inventory_snapshot_capture(abi::MapEntityRef ref,std::uint32_t viewer,
        const abi::InventorySnapshotHost* host,void* context,sin_port::weapon_net::InventoryPayload* out) noexcept {
    namespace net=sin_port::weapon_net;
    if(reading_engine_skill||!valid_launch_ref(ref,ref.epoch)||!active_level(ref.epoch)||
       !out||viewer<1||viewer>256||!host||host->version!=net::version||
       host->size!=sizeof(*host)||!host->recipients||!host->active_weapon)return 0;
    const auto cb=*host;
    OutputScope scope;
    net::InventoryRecipients recipients{};
    net::InventoryValues values{};
    net::InventoryHandleObservations handles{};
    const auto& contract=net::owned_inventory_api();
    try {
        // Host callback authenticates the full live character identity.
        if(!cb.recipients(context,ref,&recipients))return 0;
        const auto groups=contract.groups(&recipients,viewer);
        if(!groups)return 0;
        if(groups&net::InventoryAmmo){
            if(!cb.raw_ammo)return 0;
            for(unsigned i=0;i<net::inventory_ammo_count;++i)
                if(!cb.raw_ammo(context,ref,i,&values.ammo[i]))return 0;
        }
        if(groups&net::InventoryCombat){
            if(!cb.next_attack||!cb.weapon||!cb.next_attack(context,ref,&values.next_attack))return 0;
            for(unsigned i=0;i<net::inventory_weapon_count;++i)
                if(!cb.weapon(context,ref,i,&values.weapons[i],&handles.weapons[i]))return 0;
        }
        if(!cb.active_weapon(context,ref,&values.active_weapon,&handles.active_weapon))return 0;
    }catch(...){return 0;}
    return contract.encode(&values,&handles,&recipients,viewer,out);
}
const abi::InventorySnapshotApi inventory_snapshot_api{sin_port::weapon_net::version,
    sizeof(abi::InventorySnapshotApi),&inventory_snapshot_capture};

const abi::MapEntityApi map_api{abi::map_entity_version,sizeof(abi::MapEntityApi),
    &map_begin,&map_end,&map_create,&map_keyvalue,&map_spawn,&map_input,&map_remove,&map_count};
}
extern "C" SIN_FIXED_EXPORT const abi::InventorySnapshotApi* SIN_FIXED_CALL
SiN_GetInventorySnapshotApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==sin_port::weapon_net::version&&bytes==sizeof(abi::InventorySnapshotApi)?
        &inventory_snapshot_api:nullptr;
}

extern "C" SIN_FIXED_EXPORT const abi::WeaponSnapshotApi* SIN_FIXED_CALL
SiN_GetWeaponSnapshotApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==sin_port::weapon_net::version&&bytes==sizeof(abi::WeaponSnapshotApi)?
        &weapon_snapshot_api:nullptr;
}

extern "C" SIN_FIXED_EXPORT const abi::CombatAmmoApi* SIN_FIXED_CALL
SiN_GetCombatAmmoApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==abi::ammo_version&&bytes==sizeof(abi::CombatAmmoApi)?&ammo_api:nullptr;
}

extern "C" SIN_FIXED_EXPORT const abi::MagnumSecondaryApi* SIN_FIXED_CALL
SiN_GetMagnumSecondaryApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==abi::magnum_secondary_version&&bytes==sizeof(abi::MagnumSecondaryApi)?
        &magnum_secondary_api:nullptr;
}

extern "C" SIN_FIXED_EXPORT const abi::MagnumApi* SIN_FIXED_CALL
SiN_GetMagnumApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==abi::magnum_version&&bytes==sizeof(abi::MagnumApi)?&magnum_api:nullptr;
}

extern "C" SIN_FIXED_EXPORT const abi::RifleGrenadeApi* SIN_FIXED_CALL
SiN_GetRifleGrenadeApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==abi::rifle_launch_version&&bytes==sizeof(abi::RifleGrenadeApi)?&rifle_launch_api:nullptr;
}

extern "C" SIN_FIXED_EXPORT const abi::ARGrenadeApi* SIN_FIXED_CALL
SiN_GetARGrenadeApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==abi::grenade_version&&bytes==sizeof(abi::ARGrenadeApi)?&grenade_api:nullptr;
}

extern "C" SIN_FIXED_EXPORT const abi::ScattergunSecondaryApi* SIN_FIXED_CALL
SiN_GetScattergunSecondaryApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==abi::scattergun_fire_version&&bytes==sizeof(abi::ScattergunSecondaryApi)?
        &scattergun_fire_api:nullptr;
}

extern "C" SIN_FIXED_EXPORT const abi::ScattergunPumpApi* SIN_FIXED_CALL
SiN_GetScattergunPumpApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==abi::scattergun_version&&bytes==sizeof(abi::ScattergunPumpApi)?&scattergun_api:nullptr;
}

extern "C" SIN_FIXED_EXPORT const abi::WeaponBehaviorApi* SIN_FIXED_CALL
SiN_GetWeaponBehaviorApi(std::uint32_t version,std::uint32_t bytes) noexcept {
    return version==abi::weapon_version&&bytes==sizeof(abi::WeaponBehaviorApi)?&weapon_api:nullptr;
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
constexpr std::size_t parent_offset=0x18, name_offset=0x0c, real_offset=0x28, int_offset=0x2c;
template<class T>
bool read_engine_cvar(void* engine,const char* expected,std::size_t offset,T& value) noexcept {
    methods::CallResult refused=methods::CallResult::InvalidObject;
    const auto method=methods::VerifiedMethod(engine,3,refused);
    if(!method)return false;
    using Find=sin_independent::console_abi::FindSlot;
    const void* variable=nullptr;
    try {variable=reinterpret_cast<Find>(method)(engine,expected);}
    catch(...) {return false;}
    if(!memory::Readable(variable,parent_offset+sizeof(void*)))return false;
    const auto* bytes=static_cast<const unsigned char*>(variable);
    const unsigned char* root=nullptr;
    std::memcpy(&root,bytes+parent_offset,sizeof(root));
    if(!memory::Readable(root,offset+sizeof(T)))return false;
    const unsigned char* root_parent=nullptr;
    std::memcpy(&root_parent,root+parent_offset,sizeof(root_parent));
    if(root_parent!=root)return false;
    const char* name=nullptr;
    std::memcpy(&name,root+name_offset,sizeof(name));
    const auto length=std::strlen(expected)+1;
    if(!memory::Readable(name,length)||std::memcmp(name,expected,length)!=0)return false;
    T observed{};
    std::memcpy(&observed,root+offset,sizeof(observed));
    value=observed;
    return true;
}
bool read_engine_skill(void* engine,int& value) noexcept {
    int observed=0;
    if(!read_engine_cvar(engine,"skill",int_offset,observed)||!fixed::valid_skill(observed))return false;
    value=observed;return true;
}
struct EngineReadScope {
    EngineReadScope() noexcept {reading_engine_skill=true;}
    ~EngineReadScope(){reading_engine_skill=false;}
};
}
// The original CAI_BaseNPC override queries enemy memory before reading
// either ConVar. Mirror that branch order with host-resolved memory.
#pragma comment(linker,"/EXPORT:SiN_QueryNpcAttackSpreadFromEngine=_SiN_QueryNpcAttackSpreadFromEngine")
extern "C" int SIN_FIXED_CALL SiN_QueryNpcAttackSpreadFromEngine(void* cvar,
        const char* npc,const char* weapon,const char* target,
        const abi::NpcFocusMemory* memory,abi::NpcAttackSpreadResult* out) noexcept {
    abi::NpcAttackSpreadResult result{};
    if(!out||!make_focus_base(npc,weapon,target,memory,result))return 0;
    if(memory->found){
        EngineReadScope scope;
        float focus=0.f,cone=0.f;
        bool needed=false;
        if(!read_engine_cvar(cvar,"ai_spread_cone_focus_time",real_offset,focus)||
           !focus_needs_multiplier(*memory,focus,result,needed))return 0;
        if(needed&&(!read_engine_cvar(cvar,"ai_spread_defocused_cone_multiplier",real_offset,cone)||
            !apply_focus_penalty(focus,cone,result)))return 0;
    }
    *out=result;return 1;
}

// The original firing sites call bias first, then spread. Read the
// exact original ConVar names in that order, with no write/registration.
#pragma comment(linker,"/EXPORT:SiN_QueryNpcAimFromEngine=_SiN_QueryNpcAimFromEngine")
extern "C" int SIN_FIXED_CALL SiN_QueryNpcAimFromEngine(void* cvar,
        const char* npc,const char* weapon,const char* target,
        const abi::NpcAimMemory* memory,abi::NpcAimResult* out) noexcept {
    if(reading_engine_skill||!campaign||!memory||memory->focus.found>1||!out)return 0;
    abi::NpcAimResult result{};
    {
        EngineReadScope scope;
        float shot_bias=0.f,focus=0.f;
        if(!read_engine_cvar(cvar,"ai_shot_bias",real_offset,shot_bias)||
           (memory->focus.found&&!read_engine_cvar(cvar,"ai_spread_pattern_focus_time",real_offset,focus))||
           !npc_bias(*memory,shot_bias,focus,result.bias))return 0;
    }
    if(!SiN_QueryNpcAttackSpreadFromEngine(cvar,npc,weapon,target,&memory->focus,&result.spread))return 0;
    *out=result;return 1;
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
// Original firing parameter calls, then CShotManipulator's min/max
// reads, followed by the original RNG bounds and sample order.
#pragma comment(linker,"/EXPORT:SiN_QueryNpcShotFromEngine=_SiN_QueryNpcShotFromEngine")
extern "C" int SIN_FIXED_CALL SiN_QueryNpcShotFromEngine(void* cvar,void* random,
        const char* npc,const char* weapon,const char* target,const abi::NpcAimMemory* memory,
        const abi::NpcShotBasis* basis,abi::NpcShotFromEngineResult* out) noexcept {
    if(!out)return 0;
    abi::NpcShotFromEngineResult result{};
    if(!basis||!finite_vector(basis->forward)||!finite_vector(basis->right)||!finite_vector(basis->up)||
       !random||!SiN_QueryNpcAimFromEngine(cvar,npc,weapon,target,memory,&result.aim)){
        *out=result;return 1;
    }
    abi::NpcShotTuning tuning{};
    {
        EngineReadScope scope;
        if(!read_engine_cvar(cvar,"ai_shot_bias_min",real_offset,tuning.bias_min)||
           !read_engine_cvar(cvar,"ai_shot_bias_max",real_offset,tuning.bias_max)){
            *out=result;return 1;
        }
    }
    (void)npc_sample_shot(basis,&result.aim,&tuning,&read_native_npc_random,random,&result.shot);
    *out=result;return 1;
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
