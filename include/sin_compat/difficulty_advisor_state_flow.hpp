#pragma once
// SiN Episodes 2006, original CStatAdvisor binary-derived reference.
// Does not implement the Source ABI, message envelopes or game tick scheduling.
#include "difficulty_advisor_presets.hpp"
#include "difficulty_advisors_contract.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace sin_compat::difficulty {
struct AdvisorStateFields {
    std::array<float,4> channels{}; // original advisor +0x24,+0x28,+0x2C,+0x30
    float score_value = 0.0f;       // original advisor +0x1C
};

// The six presets produce four positionally defined outputs. Original
// 0x222C0B90 passes them to 0x222B9450, which forwards to 0x222C4AC0.
// The store order is reversed in x86 but results in ascending offsets.
// 0x222B9475: f32 score <- FABS(output1+output2)*0.5f.
inline AdvisorStateFields derive_advisor_fields(const AdvisorBlendOutputs& x) noexcept {
    return {{{x.output_0,x.output_1,x.output_2,x.output_3}},
            std::fabs(x.output_1+x.output_2)*0.5f};
}
inline std::array<AdvisorStateFields,challenge_advisor_count>
derive_all_advisor_fields(float t) noexcept {
    const auto presets=blend_all_advisor_presets(t);
    std::array<AdvisorStateFields,challenge_advisor_count> result{};
    for(std::size_t i=0;i<result.size();++i)
        result[i]=derive_advisor_fields(presets[i]);
    return result;
}

// Original CStatAdvisor vtable+8 -> 0x222C4AB0 stores +0x1C and
// tail-jumps to 0x222C6720. The latter visits the listener pointer
// array at +0x08 (count +0x14), calling each listener's virtual +8.
// The host supplies valid listeners and handles their lifetime.
template<typename It,typename Notify>
inline void store_and_forward_advisor_score(AdvisorStateFields& state,float score,
                                            It first,It last,Notify&& notify_slot_8) {
    state.score_value=score;
    for(;first!=last;++first)notify_slot_8(*first,score);
}

// Separate 0x222B9420 manager routine broadcasts one scalar to all
// six advisors, invoking 0x222C4AA0 (plain store at +0x34).
// Meaning and triggering conditions remain unproven.
inline void broadcast_auxiliary_scalar(
    std::array<float,challenge_advisor_count>& values,float value) noexcept {
    values.fill(value);
}

// The two paired message contracts from Phase 76 use the exact state
// members written by the preceding original code. Advisor code +0x20
// is independent; caller provides it rather than inferring it.
inline AdvisorScores advisor_scores_packet(
    const std::array<AdvisorStateFields,challenge_advisor_count>& advisors,
    const std::array<std::int16_t,challenge_advisor_count>& codes) noexcept {
    AdvisorScores result{};
    for(std::size_t i=0;i<advisor_count;++i)
        result.entries[i]={advisors[i].score_value,codes[i]};
    return result;
}
inline AdvisorChannels advisor_channels_packet(
    const std::array<AdvisorStateFields,challenge_advisor_count>& advisors) noexcept {
    AdvisorChannels result{};
    for(std::size_t i=0;i<advisor_count;++i)
        result.entries[i]=advisors[i].channels;
    return result;
}
} // namespace sin_compat::difficulty
