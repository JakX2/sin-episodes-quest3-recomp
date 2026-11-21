#pragma once
// Independent host interpretation of 2006 original 0x220B2B10 matched-ID branch.
// Uses the host-only Phase570 waypoint type; not an original x86 C++ layout.
#include "sin_compat/original_grunt_waypoint_pool_phase570.hpp"
#include <functional>
#include <unordered_set>

namespace sin_compat::phase571 {
using Point = phase570::Point;
enum class Status { Ok, UnsupportedGraph, UnsupportedAliasing, UnsupportedNullNativePath };
struct Outcome { Status status; bool released_incoming; Point* final_node; };

inline Outcome splice_identity(Point* introduced_prefix, Point* incoming,
                               const std::function<void(Point*)>& pool_release = {}) {
    // Unlike the original routine, preflight unsafe inputs before ANY write.
    // This also prevents malformed/cyclic/overlapping chains from turning
    // an owner-side regression test into host use-after-free.
    if (!introduced_prefix || !phase570::valid_forward_chain(introduced_prefix) ||
        (incoming && !phase570::valid_forward_chain(incoming)))
        return {Status::UnsupportedGraph, false, nullptr};
    std::unordered_set<Point*> prefix_nodes;
    Point* tail = nullptr;
    for (Point* p = introduced_prefix; p; p = p->forward) {
        prefix_nodes.insert(p);
        tail = p;
    }
    if (!tail) return {Status::UnsupportedGraph, false, nullptr};
    if (incoming) {
        if (incoming->backward && incoming->backward->forward != incoming)
            return {Status::UnsupportedGraph, false, nullptr};
        if (incoming->backward && prefix_nodes.count(incoming->backward))
            return {Status::UnsupportedAliasing, false, nullptr};
        for (Point* p = incoming; p; p = p->forward)
            if (prefix_nodes.count(p))
                return {Status::UnsupportedAliasing, false, nullptr};
    }
    // Original code reads incoming +0x10 without null guard when the
    // introduced prefix tail ID is not -1. Do not reproduce the crash.
    if (!incoming && tail->identity_0x10 != -1)
        return {Status::UnsupportedNullNativePath, false, nullptr};

    const bool match = incoming && tail->identity_0x10 != -1 &&
                       tail->identity_0x10 == incoming->identity_0x10;
    tail->flags_0x20 &= ~std::uint32_t{8};
    Point* destination = incoming;
    if (match) {
        Point* successor = incoming->forward;
        Point* former_predecessor = incoming->backward;
        // Reproduce original 0x220B2B40..0x220B2B64 ordering:
        // detach incoming, return it to the native pool, THEN splice.
        if (successor) successor->backward = former_predecessor;
        if (former_predecessor) former_predecessor->forward = successor;
        if (pool_release) pool_release(incoming);
        destination = successor;
        // After release, the original reconnects destination to the prefix
        // and cuts any former predecessor from the new chain.
    }
    tail->forward = destination;
    if (destination) {
        Point* former_predecessor = destination->backward;
        if (former_predecessor) former_predecessor->forward = nullptr;
        destination->backward = tail;
    }
    Point* final_node = tail;
    for (Point* p = destination; p; p = p->forward) final_node = p;
    final_node->flags_0x20 |= 8;
    return {Status::Ok, match, final_node};
}

inline Outcome prepend_identity(phase570::Holder& holder, Point* prefix,
                                 const std::function<void(Point*)>& pool_release = {}) {
    Outcome result = splice_identity(prefix, holder.first, pool_release);
    if (result.status == Status::Ok) holder.first = prefix;
    return result;
}

inline void default_node_220B2A60(Point& point, phase570::Vec3 default_position) {
    // Complete original constructor first clears all 0x30 bytes, then
    // writes vector default, ID=-1, length=-1. At +0x18/+0x1C the initial
    // -1 writes are overwritten by the native REP STOSD: final values 0.
    point = {};
    point.raw_0x18 = 0;
    point.raw_0x1C = 0;
    point.position = default_position;
    point.identity_0x10 = -1;
    point.cached_distance_0x14 = -1.f;
}
inline void explicit_node_220B2AB0(Point& point, phase570::Vec3 pos,
                                    std::uint32_t raw_0x0C, std::uint32_t kind,
                                    std::uint32_t flags, std::int32_t identity) {
    // Five arguments, native ret 0x14. x86 object uses 48 bytes.
    point = {};
    point.raw_0x18 = 0;
    point.raw_0x1C = 0;
    point.position = pos;
    point.raw_0x0C = raw_0x0C;
    point.kind_0x24 = kind;
    point.flags_0x20 = flags;
    point.identity_0x10 = identity;
    point.cached_distance_0x14 = -1.f;
}
} // namespace sin_compat::phase571