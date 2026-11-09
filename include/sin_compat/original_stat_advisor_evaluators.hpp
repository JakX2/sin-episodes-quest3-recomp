#pragma once
// Original SiN Episodes (2006) CStatAdvisor virtual methods reconstructed
// from SHA-pinned server.dll. Slot IDs are proven; source method names aren't.
// CStatAdvisor vtable 0x2245CBD4 slots 9/10/11 -> 0x222C4D00/40/80.

#include <array>
#include <cstdint>

namespace sin_compat::difficulty {

// Portable state, NOT the 2006 x86 class layout or engine-owned entity.
struct OriginalStatAdvisorFields {
    float score = 0.0f;                   // original this+0x1C
    std::int32_t code = 0;               // original this+0x20
    std::array<float, 4> values{};       // original +0x24/+0x28/+0x2C/+0x30
    float gate_value = 0.0f;             // original this+0x34
    float gate_step = 0.0f;              // original this+0x38
};

// 0x222C4D00: original x87 FCOMP/FNSTSW/JP/TEST branches.
// NaN/unordered comparison of score also selects code 3.
inline bool evaluate_stat_advisor_slot9(OriginalStatAdvisorFields& a) noexcept {
    const bool middle_descending = a.values[2] < a.values[1];
    a.code = middle_descending
        ? (a.score < a.values[3] ? 4 : 3)
        : (a.score > a.values[3] ? 4 : 3);
    return true;
}

// 0x222C4D40: emits code 0/1 and low-byte true. NaN score selects 1.
inline bool evaluate_stat_advisor_slot10(OriginalStatAdvisorFields& a) noexcept {
    const bool middle_descending = a.values[1] > a.values[2];
    a.code = middle_descending
        ? (a.score > a.values[0] ? 0 : 1)
        : (a.score < a.values[0] ? 0 : 1);
    return true;
}

// 0x222C4D80: unconditional code 2, low-byte false.
inline bool evaluate_stat_advisor_slot11(OriginalStatAdvisorFields& a) noexcept {
    a.code = 2;
    return false;
}

// Independent original 0x222C4D90: returns false when the stored gate
// is strictly above the argument. Otherwise sets gate = argument+step.
// NaN follows the update branch. The engine/time semantics are UNKNOWN.
inline bool advance_stat_advisor_gate(OriginalStatAdvisorFields& a,
                                      float caller_value) noexcept {
    if (a.gate_value > caller_value) return false;
    a.gate_value = caller_value + a.gate_step;
    return true;
}

} // namespace sin_compat::difficulty
