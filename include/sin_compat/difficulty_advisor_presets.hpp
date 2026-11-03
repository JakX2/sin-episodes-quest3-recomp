#pragma once
// SiN Episodes (2006) x86 evidence-derived advisor configuration reference.
// Pure finite-input C++17 math; not original engine ABI or a running VR port.
#include <array>
#include <cstddef>
#include <string_view>

namespace sin_compat::difficulty {
inline constexpr std::size_t challenge_advisor_count = 6;
inline constexpr std::size_t challenge_preset_float_count = 5;
inline constexpr std::string_view challenge_cvar_name = "dd_challenge";
inline constexpr std::string_view challenge_cvar_default_text = "0.2f";
inline constexpr std::string_view forgiveness_cvar_name = "dd_forgiveness";
inline constexpr std::string_view forgiveness_cvar_default_text = "0.5f";

struct AdvisorPreset {
    float first;
    float second;
    float spread;
    float first_fraction;
    float second_fraction;
};
inline constexpr std::array<AdvisorPreset, challenge_advisor_count> original_advisor_presets{{
    {80.f, 20.f, 20.f, 0.3f, 0.3333f},
    {20.f, 60.f, 10.f, 0.3333f, 0.16666f},
    {70.f, 20.f, 20.f, 0.5f, 0.5f},
    {30.f, 5.f, 15.f, 0.1f, 0.5f},
    {10.f, 70.f, 30.f, 0.5f, 0.5f},
    {0.f, 0.f, 0.f, 0.f, 0.f},
}};

// Values in positional order of the original four output pointers to
// 0x222C0AE0 (callee args at ESP+8/+C/+10/+14). The caller forwards all
// four to manager-advisor construction. Semantic names are *unverified*.
struct AdvisorBlendOutputs {
    float output_0;
    float output_1;
    float output_2;
    float output_3;
};

// Equivalent finite-input arithmetic for original x87 branching based on
// first < second. No bounds checking on t occurs inside 0x222C0AE0.
// On ARM64 the final float bits may differ due to x87 excess precision.
inline AdvisorBlendOutputs blend_advisor_preset(const AdvisorPreset& p,
                                                 float t) noexcept {
    if (p.first < p.second) {
        const float center = (1.f - t) * (p.first + p.spread) + t * p.second;
        const float offset_center = center - p.spread;
        return {offset_center - p.first_fraction * offset_center,
                offset_center, center,
                center + p.second_fraction * (100.f - center)};
    }
    const float center = (1.f - t) * p.first + t * (p.second + p.spread);
    const float offset_center = center - p.spread;
    return {center + p.first_fraction * (100.f - center),
            center, offset_center,
            offset_center - p.second_fraction * offset_center};
}

inline std::array<AdvisorBlendOutputs, challenge_advisor_count>
blend_all_advisor_presets(float t) noexcept {
    std::array<AdvisorBlendOutputs, challenge_advisor_count> result{};
    for (std::size_t i = 0; i < result.size(); ++i)
        result[i] = blend_advisor_preset(original_advisor_presets[i], t);
    return result;
}

// Original initialization also computes manager +0x0C as (1 -
// dd_forgiveness). The two console variables have different roles.
inline float initial_forgiveness_complement(float forgiveness) noexcept {
    return 1.f - forgiveness;
}
} // namespace sin_compat::difficulty
