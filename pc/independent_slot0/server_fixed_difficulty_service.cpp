#include "server_fixed_difficulty_service.h"
#include "sin_compat/fixed_difficulty_phase663.hpp"
#include "sin_compat/info_challenge_phase665.hpp"
#include <cmath>
#include <optional>

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
int SIN_FIXED_CALL begin(int skill) noexcept {
    if(campaign || !fixed::valid_skill(skill))return 0;
    campaign.emplace(skill);
    return 1;
}
int SIN_FIXED_CALL end() noexcept {
    if(!campaign)return 0;
    campaign.reset();
    return 1;
}
int SIN_FIXED_CALL select(int skill) noexcept {
    if(!campaign)return 0;
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
    if(!campaign||!std::isfinite(time))return 0;
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
