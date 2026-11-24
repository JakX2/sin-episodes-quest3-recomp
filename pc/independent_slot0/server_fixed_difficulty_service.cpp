#include "server_fixed_difficulty_service.h"
#include "sin_compat/fixed_difficulty_phase663.hpp"
#include "sin_compat/info_challenge_phase665.hpp"
#include <cmath>
#include <optional>
#include <vector>
#include <string>
#include <cstring>
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
    if(reading_engine_skill || !campaign)return 0;
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
