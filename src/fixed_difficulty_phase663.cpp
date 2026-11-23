#include "sin_compat/fixed_difficulty_phase663.hpp"
#include <limits>

namespace sin_compat::difficulty::fixed_phase663 {
namespace {
// These changes are a deliberately limited NEW balancing policy.
//
// Keep every unverified spawn-variation and helmet-shot-off metric at the
// original current value until its actual game-rule consumer is audited.
// Damage dealt, ammo scaling, forced health and cover remain unchanged.
// Original metric indices:
// 0 DAMAGE_TAKEN_SCALE, 4 PERCENT_HEALTH_KITS_DROP,
// 5 PERCENT_NPC_2ND_FIRE, 8 PERCENT_NPC_WEARS_HELMET,
// 11 RANDOM_NPC_WAIT_BEFORE_SHOOT, 12 PERCENT_NPC_ACCURATE,
// 13 PERCENT_NPC_MOBILE.
// Values are drawn from the original endpoints but rearranged into logical
// Easy/Normal/Hard, since global dd_challenge t is NOT a monotonic skill.
RuntimeMetrics modified_original(const Level level) noexcept {
    RuntimeMetrics m=original_initial_metric_values();
    if(level==Level::Easy) {
        m[0]=0.75f;
        m[4]=0.5f;
        m[5]=0.f;
        m[8]=0.1f;
        m[11]=0.1f;
        m[12]=0.5f;
        m[13]=0.3f;
    } else if(level==Level::Hard) {
        m[0]=1.4f;
        m[4]=0.3f;
        m[5]=0.5f;
        m[8]=0.6f;
        m[11]=-0.1f;
        m[12]=1.f;
        m[13]=0.7f;
    }
    return m;
}
} // namespace

bool valid_skill(int raw) noexcept {
    return raw>=kEasySkill && raw<=kHardSkill;
}
Level level_for_skill(int raw) noexcept {
    switch(raw) {
        case kEasySkill:return Level::Easy;
        case kHardSkill:return Level::Hard;
        default:return Level::Normal;
    }
}
const char* profile_name(Level level) noexcept {
    switch(level) {
        case Level::Easy:return "Easy";
        case Level::Hard:return "Hard";
        default:return "Normal";
    }
}
Profile make_profile(Level level) noexcept {
    if(level!=Level::Easy&&level!=Level::Hard)level=Level::Normal;
    return {level,profile_name(level),modified_original(level)};
}

FixedDifficultyRuntime::FixedDifficultyRuntime() noexcept
    :level_(Level::Normal),metrics_(modified_original(Level::Normal)){}

Selection FixedDifficultyRuntime::set_skill(int next) noexcept {
    if(!valid_skill(next))return Selection::InvalidSkill;
    const Level target=level_for_skill(next);
    if(target==level_)return Selection::Unchanged;
    level_=target;
    metrics_=modified_original(level_);
    ++revision_;
    return Selection::Applied;
}
float FixedDifficultyRuntime::metric_at(std::size_t idx) const noexcept {
    return idx<original_metric_count
        ?metrics_[idx]:std::numeric_limits<float>::quiet_NaN();
}
bool FixedDifficultyRuntime::lookup(std::string_view name,float& out) const noexcept {
    const auto index=metric_index(name);
    if(index<0)return false;
    out=metrics_[static_cast<std::size_t>(index)];
    return true;
}
PublishResult FixedDifficultyRuntime::publish_to_host(
        IOriginalMetricSink& host) const noexcept {
    PublishResult result{};
    try {
        for(std::size_t i=0u;i<original_metric_count;++i) {
            ++result.attempted;
            if(!host.write_original_metric(i,metrics_[i]))
                return result;
            ++result.committed;
        }
        result.complete=true;
    }catch(...) {
        // External host may have committed an earlier prefix of writes.
        // The caller must treat incomplete publication as non-bootable
        // rather than claiming an atomic update or proceeding with a
        // partially mixed difficulty profile.
    }
    return result;
}

void FixedDifficultyRuntime::observe_dynamic_trigger(
        const TriggerObservation& trigger) noexcept {
    ++trigger_count_;
    // The source-authenticated original trigger uses BYTE(this+0x478),
    // not a DWORD check; preserve the low byte for level completion.
    if((static_cast<std::uint32_t>(trigger.end_of_game)&0xffu)!=0u)
        completed_=true;
    // Do NOT apply EstimatedDifficulty/EstimatedTime to the selected profile.
}
Selection FixedDifficultyRuntime::reapply_after_archive(int selected) noexcept {
    if(!valid_skill(selected))return Selection::InvalidSkill;
    const Level target=level_for_skill(selected);
    // Always rebuild to remove adaptive metrics a host may have loaded from
    // an original save, even when the user's skill hasn't changed.
    const Selection result=target==level_?
            Selection::Unchanged:Selection::Applied;
    level_=target;
    metrics_=modified_original(target);
    ++revision_;
    return result;
}
} // namespace sin_compat::difficulty::fixed_phase663
