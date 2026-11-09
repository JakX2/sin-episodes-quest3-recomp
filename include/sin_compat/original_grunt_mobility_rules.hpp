#pragma once

// Independently authored portable reconstruction of the original SiN
// CSE1Rules slot-104 mobility decision and its four preserved grunt call-site
// tuning factors. Not Ritual source, an engine vtable, or a complete AI.

#include <cmath>
#include <cstdint>

namespace sin_compat::rules {

enum class GruntMobilityCallSite : std::uint8_t {
    GruntVtable292,
    GruntVtable530InPlayerSights,
    GruntVtable530Alternate,
    GruntVtable271ScheduleTranslation
};

// Phase 6's receiver-aware call trace recorded these factors. Phase 7's
// binary registration extraction names grunt-local condition 0x4D as
// COND_GRUNT_IN_PLAYER_SIGHTS.
constexpr float grunt_mobility_factor(GruntMobilityCallSite site) noexcept {
    switch (site) {
    case GruntMobilityCallSite::GruntVtable292:
        return 1.0f; // original call site 0x223587A3
    case GruntMobilityCallSite::GruntVtable530InPlayerSights:
        return 4.0f; // original call site 0x223596A9
    case GruntMobilityCallSite::GruntVtable530Alternate:
        return 1.0f; // original call site 0x223596B8
    case GruntMobilityCallSite::GruntVtable271ScheduleTranslation:
        return 20.0f; // original call site 0x2235D889
    }
    return 0.0f;
}

// CSE1Rules slot 104, original server.dll VA 0x222D4B70.
// The original requests a random float in [0,1], computes
// modifier13 * factor * 0.2f, rejects an exactly-zero threshold, and compares
// random <= abs(threshold). It does not clamp thresholds above one.
inline bool should_npc_be_mobile(float modifier13,
                                 float factor,
                                 float random_sample) noexcept {
    const float threshold = modifier13 * factor * 0.2f;
    if (threshold == 0.0f) return false;
    return random_sample <= std::fabs(threshold);
}

inline bool grunt_mobility_decision(float modifier13,
                                    GruntMobilityCallSite site,
                                    float random_sample) noexcept {
    return should_npc_be_mobile(
        modifier13, grunt_mobility_factor(site), random_sample);
}

} // namespace sin_compat::rules
