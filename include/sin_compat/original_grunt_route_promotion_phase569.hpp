#pragma once
// Independent safe host model. Original PE32 layouts and original engine
// allocation/virtual callbacks are not encoded as C++ engine ABI classes.
#include "sin_compat/original_grunt_route_mutation_phase568.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace sin_compat::phase569 {
using phase568::Waypoint;
using phase568::Route;
using phase568::Result;
using phase568::Vec3;

struct CandidateHolder { Waypoint* first=nullptr; };
struct Navigator {
    Route active{};                 // native navigation object +0x24 -> separate route
    CandidateHolder candidate{};   // native navigation object +0x28 -> 4-byte pointer holder
    float candidate_stamp=-1.0f;   // native navigation +0x2C
    std::uint32_t extra_0x30=0;    // optional active route +0x08
    std::uint32_t extra_0x34=0;    // optional active route +0x24
    std::uint32_t active_raw_0x08=0;
    std::uint32_t active_raw_0x24=0;
    std::uint32_t active_raw_0x28=0;
    std::size_t holder_clear_count=0;
};
struct EntitySlot { void* entity=nullptr; std::uint32_t generation=0; };
inline void* resolve_nav_handle(std::uint32_t encoded,
                                const std::vector<EntitySlot>& slots) {
    // Original sentinel -1 and low 12-bit slot / high serial comparison.
    if(encoded==0xffffffffu)return nullptr;
    const std::uint32_t index=encoded&0xFFFu;
    if(index>=slots.size())return nullptr; // host guard; not in native code
    const EntitySlot& s=slots[index];
    return s.generation==(encoded>>12)?s.entity:nullptr;
}
inline void clear_holder(Navigator& n,Waypoint*& first) {
    // Original 0x220B2CF0 calls native 0x220B2C80 to dispose all held nodes
    // and zeroes the holder. Host tests have externally owned stack nodes;
    // count the callback but do not call the unverified original allocator.
    first=nullptr;
    ++n.holder_clear_count;
}
inline void mark_terminal_from_last(Route& r) {
    // Original 0x2209D000 copies LAST waypoint point (no route-origin add).
    Waypoint* last=phase568::last_with_guard(r.first);
    if(!last)return;
    r.endpoint=last->position;
    r.endpoint_valid_0x30=true;
    last->flags|=8u;
}
inline void endpoint_as_origin_plus(Route& r,Vec3 extra) {
    // Original 0x2209D040 writes endpoint and byte+0x30 only.
    r.endpoint=phase568::plus(r.origin,extra);
    r.endpoint_valid_0x30=true;
}
inline void store_optional_route_metadata(Navigator& n) {
    if(n.extra_0x30)n.active_raw_0x08=n.extra_0x30;
    if(n.extra_0x34) {
        n.active_raw_0x24=n.extra_0x34;
        n.active_raw_0x28=0xffffffffu; // original 0x2209CEF0
    }
}
using RefreshVirtual = std::function<bool(Navigator&)>;
inline bool refresh_candidate(Navigator& n,bool original_global_debug,
                              float original_engine_time,const RefreshVirtual& refresh_virtual) {
    // 0x22075480: candidate holder always cleared, stamp set invalid first.
    n.candidate_stamp=-1.0f;
    clear_holder(n,n.candidate.first);
    Waypoint* first=n.active.first;
    if(!first)return false;
    // Only node kind 1 or 3 skips original global debug check; labels opaque.
    const std::uint32_t raw_kind=first->dimensionality;
    if(raw_kind!=1u && raw_kind!=3u && !original_global_debug)return false;
    if(!refresh_virtual || !refresh_virtual(n))return false;
    n.candidate_stamp=original_engine_time;
    return true;
}
enum class TransferResult { Installed, NoCandidate, UnsupportedGraph };
inline TransferResult promote_candidate(Navigator& n,float original_engine_time,
                                         bool original_global_debug=false,
                                         const RefreshVirtual& refresh_virtual={}) {
    // 0x220754E0 first conditionally attempts candidate refresh.
    if(!n.candidate.first)
        refresh_candidate(n,original_global_debug,original_engine_time,refresh_virtual);
    if(!n.candidate.first)return TransferResult::NoCandidate;
    // Host graph guard. Original native code assumes valid candidate chain.
    if(!phase568::last_with_guard(n.candidate.first))return TransferResult::UnsupportedGraph;
    if(n.active.first) {
        clear_holder(n,n.active.first);
        n.active.raw_0x2c=0xffffffffu;
    }
    Waypoint* first=n.candidate.first;
    if(phase568::set_first(n.active,first,true)!=Result::Ok)
        return TransferResult::UnsupportedGraph;
    n.candidate.first=nullptr; // Original separate candidate-holder setter
    // These are TWO native calls: old route state matters.
    phase568::set_route_state(n.active,0u,original_engine_time);
    phase568::set_route_state(n.active,4u,original_engine_time);
    store_optional_route_metadata(n);
    return TransferResult::Installed;
}
} // namespace sin_compat::phase569