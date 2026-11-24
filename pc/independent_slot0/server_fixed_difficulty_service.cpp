#include "server_fixed_difficulty_service.h"
#include "sin_compat/fixed_difficulty_phase663.hpp"
#include "sin_compat/info_challenge_phase665.hpp"
#include <cmath>
#include <optional>
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
#endif
