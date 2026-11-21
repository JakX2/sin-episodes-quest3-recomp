#pragma once
// Copyright: independent original-free C++17 reference, not retail engine code.
// Original authenticated addresses: 0x2209D370, 0x2209D410, 0x220B2A20,
// 0x2206D9F0, 0x222680E0. Structs here ARE NOT original ABI layouts.
#include <cmath>
#include <cstdint>
#include <cstring>
#include <unordered_set>

namespace sin_compat::phase566 {
struct Vec3 { float x=0.f, y=0.f, z=0.f; };
struct Waypoint {
    Vec3 position{};
    // Corresponds to original node +0x24. Zero uses XY, nonzero uses XYZ.
    std::uint32_t original_distance_mode=0;
    // Corresponds to original node +0x14. Bitwise -1.0f requests recalc.
    float cached_cumulative=-1.f;
    // Original node +0x28, +0x2C links.
    Waypoint* forward=nullptr;
    Waypoint* backward=nullptr;
};
struct Route { Waypoint* first=nullptr; };
enum class Outcome { Ok, UnsupportedGraph, UnsupportedFloat };
struct Length { Outcome outcome; float value; };

inline bool original_cache_invalid(float value) {
    std::uint32_t bits=0;
    static_assert(sizeof(bits)==sizeof(value));
    std::memcpy(&bits,&value,sizeof(bits));
    return bits==0xBF800000u;
}

inline Length get_length(Route route) {
    // An absent first node returns original .rdata +0.0f.
    if(!route.first) return {Outcome::Ok,0.0f};
    std::unordered_set<const Waypoint*> seen;
    Waypoint* last=route.first;
    while(last->forward) {
        if(!seen.insert(last).second) return {Outcome::UnsupportedGraph,0.0f};
        last=last->forward;
    }
    if(!seen.insert(last).second) return {Outcome::UnsupportedGraph,0.0f};

    // Original 0x2209D410 checks only terminal node's raw +0x14 against
    // exact 0xBF800000. If not invalidated, it returns FIRST node's cached
    // value without recalculation, even if different from the tail's value.
    if(original_cache_invalid(last->cached_cumulative)) {
        // Check malformed graphs and finite coordinates before modifying
        // host state; native code had no cycle or NaN safety guarantees.
        std::unordered_set<const Waypoint*> reverse_seen;
        for(const Waypoint* cur=last;cur;cur=cur->backward) {
            if(!reverse_seen.insert(cur).second) return {Outcome::UnsupportedGraph,0.0f};
            if(!std::isfinite(cur->position.x)||!std::isfinite(cur->position.y)||
               !std::isfinite(cur->position.z)) return {Outcome::UnsupportedFloat,0.0f};
            if(cur->backward && cur->backward->forward!=cur)
                return {Outcome::UnsupportedGraph,0.0f};
        }
        if(!reverse_seen.count(route.first)) return {Outcome::UnsupportedGraph,0.0f};
        last->cached_cumulative=0.0f;
        for(Waypoint* cur=last;cur && cur->backward;cur=cur->backward) {
            Waypoint* prev=cur->backward;
            float dx=cur->position.x-prev->position.x;
            float dy=cur->position.y-prev->position.y;
            float dz=cur->position.z-prev->position.z;
            float sum=dx*dx+dy*dy;
            if(cur->original_distance_mode!=0) sum+=dz*dz;
            if(!std::isfinite(sum)) return {Outcome::UnsupportedFloat,0.0f};
            prev->cached_cumulative=cur->cached_cumulative+std::sqrt(sum);
            if(!std::isfinite(prev->cached_cumulative)) return {Outcome::UnsupportedFloat,0.0f};
        }
    }
    const float value=route.first->cached_cumulative;
    if(!std::isfinite(value)) return {Outcome::UnsupportedFloat,0.0f};
    return {Outcome::Ok,value};
}

inline bool flank_bark_if_original_metric_eligible(const Length& length) {
    return length.outcome==Outcome::Ok && length.value>240.0f;
}
} // namespace sin_compat::phase566