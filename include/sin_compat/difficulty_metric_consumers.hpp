#pragma once

// 2006 SiN Episodes x86 code-derived, finite float gameplay-metric reference.
// No original engine ABI, no runtime manager scheduling, no VR FOV modification.
#include "difficulty_metric_math.hpp"
#include <array>
#include <cstdint>

namespace sin_compat::difficulty {
using RuntimeMetrics = std::array<float, original_metric_count>;
inline constexpr std::uint32_t original_damage_scale_bypass_mask = 0x80024021u;

// Original server 0x222D3490 receives t from a caller. In the observed
// initializer 0x222BA399..0x222BA3BD it comes from [0x22592168]+0x28;
// it is also cached in manager member +0x08. The name/identity of that
// pointer and the schedule of subsequent updates are NOT proven.
inline RuntimeMetrics metrics_for_blend_input(float t) noexcept {
    return blended_metrics(t);
}
inline RuntimeMetrics original_initial_metric_values() noexcept {
    RuntimeMetrics values{};
    for (std::size_t i=0;i<values.size();++i)
        values[i]=original_metrics[i].fields[0];
    return values;
}

struct OriginalDamageFields {
    float damage=0.0f;             // original CTakeDamageInfo-like field +0x2C
    std::uint32_t damage_type=0;   // original field +0x38
};

// Original rules virtual at 0x222D5420: no scale if any bypass-mask bit is
// present; otherwise multiply mutable damage member by metric index 0.
inline void apply_damage_taken_metric(OriginalDamageFields& info,
                                      const RuntimeMetrics& current) noexcept {
    if ((info.damage_type & original_damage_scale_bypass_mask) == 0)
        info.damage *= current[0];
}

// Original rules virtual at 0x222D47F0: metric index 1 times argument.
inline float scaled_damage_done(float incoming,
                                 const RuntimeMetrics& current) noexcept {
    return current[1]*incoming;
}

// Original rules virtual at 0x222D4830: directly returns metric index 2.
inline float ammo_quantity_multiplier(const RuntimeMetrics& current) noexcept {
    return current[2];
}

// Original rules virtual at 0x222D4CF0: metric index 11 + argument.
inline float npc_random_wait_before_shoot(float base_delay,
                                           const RuntimeMetrics& current) noexcept {
    return current[11]+base_delay;
}
} // namespace sin_compat::difficulty
