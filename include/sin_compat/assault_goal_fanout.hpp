#pragma once
// Host-side iteration of original 2006 ai_goal_assault::InputBeginAssault's
// per-actor fanout, now using the independent portable NPC shared C ABI.
// Native source behavior is verified in original server.dll at 0x220CFF30.
// This new layer deliberately ignores x86 EHANDLE encoding and requires a
// stable generation-aware level-owned snapshot and trusted session resolver.
// It is NOT an entity-factory registration or a SourceVR game input handler.

#include "sin_compat/assault_npc_c_api.h"

#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <utility>
#include <vector>

namespace sin_compat {

struct AssaultGoalBeginReport {
    std::size_t visited = 0;
    std::size_t signaled = 0;
    std::size_t invalid_handles = 0;
    std::size_t duplicate_handles = 0;
    std::size_t missing_sessions = 0;
    std::size_t not_bound = 0;
    std::size_t expired_actors = 0;
    std::size_t host_failures = 0;
    std::size_t reentrant_sessions = 0;
};

// The host must snapshot active actors on the game thread and resolve each
// exact (index,generation) into the matching currently-live NPC session.
// The original x86 loop skips invalid/unresolved actors and continues.
// Duplicate suppression, typed statuses, unbound refusal and exception
// isolation are NEW port guardrails, not reconstructed original control flow.
//
// Resolver may be invoked only for valid, previously unseen handles.
// Callback/session references must be owned by the host for the entire call.
template <typename ResolveSession>
AssaultGoalBeginReport begin_original_goal_assault_fanout(
    const std::vector<sin_assault_ref_v1>& actor_snapshot,
    ResolveSession&& resolve_session) {
    AssaultGoalBeginReport report{};
    report.visited = actor_snapshot.size();
    std::unordered_set<std::uint64_t> seen;
    seen.reserve(actor_snapshot.size());
    for (const auto& actor : actor_snapshot) {
        if (!actor.index || !actor.generation) {
            ++report.invalid_handles;
            continue;
        }
        const std::uint64_t key =
            (static_cast<std::uint64_t>(actor.generation) << 32) | actor.index;
        if (!seen.insert(key).second) {
            ++report.duplicate_handles;
            continue;
        }
        try {
            auto* session = resolve_session(actor);
            if (!session) {
                ++report.missing_sessions;
                continue;
            }
            const auto result = sin_assault_begin_v1(session);
            switch (result) {
                case SIN_ASSAULT_OK: ++report.signaled; break;
                case SIN_ASSAULT_NOT_BOUND: ++report.not_bound; break;
                case SIN_ASSAULT_ACTOR_EXPIRED: ++report.expired_actors; break;
                case SIN_ASSAULT_REENTRANT: ++report.reentrant_sessions; break;
                default: ++report.host_failures; break;
            }
        } catch (...) {
            // A host-side resolver error is isolated to the current actor.
            // It does not abort signaling of the remaining valid NPCs.
            ++report.host_failures;
        }
    }
    return report;
}

} // namespace sin_compat
