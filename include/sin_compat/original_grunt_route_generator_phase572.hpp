#pragma once
// Source-bounded entry/exit *model* for original 2006 nav virtual +0x54.
// Does NOT implement the native waypoint sampling loop / engine trace ABI.
#include <cmath>
#include <cstdint>

namespace sin_compat::phase572 {
struct NativeGateInputs {
    std::uint32_t initial_waypoint_kind = 0;
    float owner_measurement = 0.f; // owner member+0x14 virtual+0x58 result
    float computed_special_distance = 0.f; // abstracted native x87 calls
};
enum class Result { NoActiveWaypoint, RejectedMetric, NoCandidateProduced, Installed };
inline bool finite_eligible_gate(const NativeGateInputs& g) {
    // Native x87 requires separate status handling for NaN, and potentially
    // exceptions. This host model deliberately accepts finite inputs only.
    if (!std::isfinite(g.owner_measurement)) return false;
    const bool special = (g.initial_waypoint_kind == 1u ||
                          g.initial_waypoint_kind == 3u);
    if (!special) return g.owner_measurement > 0.1f;
    if (!std::isfinite(g.computed_special_distance)) return false;
    const float selected = (g.computed_special_distance > g.owner_measurement)
                               ? g.computed_special_distance
                               : g.owner_measurement;
    return selected > 0.01f;
}

// Separate abstract callback for *unreconstructed* original waypoint-building
// and visibility paths, which really call 0x22071C50 and 0x2228B220.
// This prevents the reference from falsely inventing native nav decisions.
template<class Host>
Result generate_candidate_prefix(Host& h) {
    h.clear_candidate_holder(); // original first E8 0x220B2CF0, always called
    if (!h.has_active_first()) return Result::NoActiveWaypoint;
    const NativeGateInputs gate=h.gate_inputs();
    if (!finite_eligible_gate(gate)) return Result::RejectedMetric;
    if (!h.build_using_native_opaque_callbacks()) return Result::NoCandidateProduced;
    if (!h.has_built_candidate_first()) return Result::NoCandidateProduced;
    h.commit_first_pointer(); // original 0x220B2A10, after non-null check
    return Result::Installed;
}
} // namespace sin_compat::phase572