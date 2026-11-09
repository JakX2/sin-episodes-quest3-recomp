#pragma once
// Independent reconstruction of original server.dll CStatAdvisor sample
// pipeline: 0x222C4DC0 -> 0x222C6750 -> vtable slot1 0x222C6830
// -> 0x222C4C80 -> slots 9/10/11. The real x86 ABI is not used.
#include "sin_compat/original_stat_advisor_evaluators.hpp"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace sin_compat::difficulty {
enum class OriginalAdvisorRoute : std::uint8_t { Slot9, Slot10, Slot11 };

struct OriginalAdvisorDispatch {
    OriginalAdvisorRoute route = OriginalAdvisorRoute::Slot11;
    float original_argument = 0.0f; // x87-calculated float passed to virtual
    bool virtual_return = false;
};

// The original 0x222C4C80 checks this+0x3C and branches on the
// relative ordering of score, channel1 (+0x28), channel2 (+0x2C).
// The comparison shape below is equivalent to the original x87 PF/ZF
// branches for finite floats, equal values, infinities and quiet NaNs.
inline OriginalAdvisorDispatch classify_original_advisor(
    OriginalStatAdvisorFields& advisor, bool use_threshold_routes) noexcept {
    const float score = advisor.score;
    const float low_or_high_a = advisor.values[1];
    const float low_or_high_b = advisor.values[2];
    if (!use_threshold_routes) {
        return {OriginalAdvisorRoute::Slot11, score,
                evaluate_stat_advisor_slot11(advisor)};
    }
    bool call_slot10 = false;
    bool call_slot9 = false;
    if (low_or_high_a > low_or_high_b) {
        call_slot10 = score > low_or_high_a;
        call_slot9 = score < low_or_high_b;
    } else {
        call_slot10 = score < low_or_high_a;
        call_slot9 = score > low_or_high_b;
    }
    if (call_slot10) {
        const float arg = std::fabs(score - low_or_high_a);
        return {OriginalAdvisorRoute::Slot10, arg,
                evaluate_stat_advisor_slot10(advisor)};
    }
    if (call_slot9) {
        const float arg = std::fabs(low_or_high_b - score);
        return {OriginalAdvisorRoute::Slot9, arg,
                evaluate_stat_advisor_slot9(advisor)};
    }
    return {OriginalAdvisorRoute::Slot11, score,
            evaluate_stat_advisor_slot11(advisor)};
}

// Original listener array at this+0x08, signed count at this+0x14.
// 0x222C6750 invokes listener vtable +0x0C with the raw input sample.
// Slot1 0x222C6830 invokes listener vtable +0x04 to read one float.
// Interface here is independent, not a guessed binary-compatible vtable.
struct IOriginalAdvisorListener {
    virtual ~IOriginalAdvisorListener() = default;
    virtual void receive_sample(float raw_sample) = 0;
    virtual float read_current_value() = 0;
};

struct OriginalAdvisorSampleResult {
    bool accepted = false; // 0x222C4DC0 low byte, not classifier return
    OriginalAdvisorDispatch dispatch{};
};

// Real method 0x222C4DC0: when gate>input returns false immediately.
// Otherwise updates gate, notifies ALL listeners, averages their values,
// stores score at +0x1C and routes through 0x222C4C80.
// The listener pointers must be live and nonnull (native host responsibility).
inline OriginalAdvisorSampleResult submit_original_advisor_sample(
    OriginalStatAdvisorFields& advisor, bool use_threshold_routes,
    float sample, const std::vector<IOriginalAdvisorListener*>& listeners) {
    if (advisor.gate_value > sample) return {};
    advisor.gate_value = sample + advisor.gate_step;
    for (IOriginalAdvisorListener* listener : listeners) {
        listener->receive_sample(sample);
    }
    float sum = 0.0f;
    for (IOriginalAdvisorListener* listener : listeners) {
        sum += listener->read_current_value();
    }
    // 0x222C6830 sums into a 32-bit float and divides by signed count.
    // Under the original masked x87 exception mode, zero/zero -> NaN.
    advisor.score = listeners.empty()
        ? std::numeric_limits<float>::quiet_NaN()
        : sum / static_cast<float>(listeners.size());
    auto dispatch = classify_original_advisor(advisor, use_threshold_routes);
    return {true, dispatch};
}
} // namespace sin_compat::difficulty
