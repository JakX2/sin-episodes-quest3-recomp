#pragma once
// Original-free bounded host model of SiN 2006 server 0x22071C50 dispatch.
// Native geometric callbacks, filters, math and 56-byte output ABI are opaque.
#include <cstdint>
#include <cmath>

namespace sin_compat::phase573 {
struct ResultRecord {
    std::int32_t raw_status=0; // native output +0x00, nonnegative => allow
    float initial_distance=0.f; // original computes at output +0x20
    float other_distance=0.f; // output +0x24, native default 0
};
enum class Path { Kind0, Kind1, Kind2, Kind3, OutsideRange };
struct Outcome { bool allowed; Path path; };

template<class Host>
Outcome check_spatial_mode(Host& engine, std::uint32_t waypoint_kind,
                           ResultRecord& rec) {
    // Original resets the report and measures a distance BEFORE its switch.
    rec = {};
    rec.initial_distance = engine.initial_distance(waypoint_kind);
    if (waypoint_kind > 3u) {
        rec.raw_status = -4; // original unconditionally writes -4 for >3
        rec.other_distance = engine.fallback_distance(waypoint_kind);
        return {false, Path::OutsideRange};
    }
    const Path path = static_cast<Path>(waypoint_kind);
    // These are *distinct* original E8 callees, not a generic hull test.
    // Mode0 contains sub-branches including the original negative -2 code;
    // its complete engine filters are still delegated to the caller.
    rec.raw_status = engine.mode_specific_result(path, rec);
    return {rec.raw_status >= 0, path};
}
} // namespace sin_compat::phase573