#pragma once
// Engine-neutral reference of the original 2006 Source assault behavior's
// verified rally-point ranking, NOT a recreated CAI_AssaultBehavior or entity.
// Original server.dll 0x220CFC00 prefers highest priority (candidate+0x3A0),
// then smaller squared 3D origin distance (+0x2BC) on priority ties.
// Original point locking uses m_hLockedBy (+0x3BC); the live handle-resolution
// check remains host-supplied as candidate.eligible.

#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <vector>

namespace sin_compat {

struct RallyPointCandidate {
    int priority = 1;
    std::array<float, 3> origin{{0.0f, 0.0f, 0.0f}};
    bool eligible = true;
};

struct RallyPointChoice {
    std::size_t index;
    double distance_squared;
};

// Preserve first-in-list order when priority and distance tie.
// Reject negative priority and nonfinite coordinates rather than applying
// undefined/ambiguous Source x87 NaN corner-case behavior.
inline std::optional<RallyPointChoice> choose_mobgrunt_rally_point(
    const std::array<float, 3>& npc_origin,
    const std::vector<RallyPointCandidate>& candidates) {
    for (const float axis : npc_origin) {
        if (!std::isfinite(axis)) return std::nullopt;
    }
    std::optional<RallyPointChoice> best;
    int best_priority = -1;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const auto& candidate = candidates[i];
        if (!candidate.eligible || candidate.priority < 0) continue;
        double squared_distance = 0.0;
        bool valid = true;
        for (std::size_t axis = 0; axis < 3; ++axis) {
            if (!std::isfinite(candidate.origin[axis])) {
                valid = false;
                break;
            }
            const double delta = static_cast<double>(candidate.origin[axis]) - npc_origin[axis];
            squared_distance += delta * delta;
        }
        if (!valid || !std::isfinite(squared_distance)) continue;
        if (!best || candidate.priority > best_priority ||
            (candidate.priority == best_priority && squared_distance < best->distance_squared)) {
            best = RallyPointChoice{i, squared_distance};
            best_priority = candidate.priority;
        }
    }
    return best;
}

} // namespace sin_compat
