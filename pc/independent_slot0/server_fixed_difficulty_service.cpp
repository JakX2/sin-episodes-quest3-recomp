#include "server_fixed_difficulty_service.h"
#include "sin_compat/fixed_difficulty_phase663.hpp"
#include "sin_compat/info_challenge_phase665.hpp"
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
// Map-local behavior objects, not actual CBaseEntity allocations.
enum class EntityKind { EnvGlobal, InfoChallenge };
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
#pragma comment(linker,"/EXPORT:SiN_GetGlobalStateApi=_SiN_GetGlobalStateApi")
#pragma comment(linker,"/EXPORT:SiN_EnvGlobalDispatch=_SiN_EnvGlobalDispatch")
#pragma comment(linker,"/EXPORT:SiN_GetMapEntityApi=_SiN_GetMapEntityApi")
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
