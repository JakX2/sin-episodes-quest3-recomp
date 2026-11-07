#pragma once

// Original 2006 SiN goal actor setup is recovered at server.dll 0x220CFF00:
// goal m_RallyPoint (+0x3C8) and m_AssaultCue (+0x3CC) are forwarded for
// each resolved actor to CAI_AssaultBehavior::SetParameters (0x220CFC00).
// Direct CNPC_MobGrunt::InputAssault (0x22357E50) instead passes mode 3.
//
// This is a NEW host-side map I/O boundary, not a recovered C++ method name,
// a SourceVR entity factory, or an original Windows x86 EHANDLE implementation.
// A real engine must snapshot actor handles, perform name/class filtering
// when collecting assault_rallypoint candidates, and supply live callbacks.

#include "sin_compat/assault_npc_c_api.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace sin_compat {

// Engine-supplied entity snapshot. The host must include only rallypoint
// entities matching this goal's exact/pattern rallypoint name in the given
// engine version; no map-global implicit selection occurs in this helper.
struct AssaultGoalActorBindingSnapshot {
    std::array<float, 3> npc_origin{};
    std::vector<sin_assault_candidate_v1> matching_rallypoints{};
};

struct AssaultGoalActorBindResult {
    sin_assault_ref_v1 actor{};
    int32_t status = SIN_ASSAULT_INVALID_ARGUMENT;
    std::uint32_t rally_id = 0; // nonzero only for a confirmed successful bind
};

struct AssaultGoalConfigureReport {
    std::size_t visited = 0;
    bool invalid_configuration = false;
    std::size_t invalid_handles = 0;
    std::size_t duplicate_handles = 0;
    std::size_t missing_sessions = 0;
    std::size_t unavailable_snapshots = 0;
    std::size_t bound = 0;
    std::size_t no_candidates = 0;
    std::size_t lock_rejected = 0;
    std::size_t unresolved_rallies = 0;
    std::size_t expired_actors = 0;
    std::size_t already_bound = 0;
    std::size_t reentrant = 0;
    std::size_t host_failures = 0;
    std::vector<AssaultGoalActorBindResult> actor_results{};
};

// The two host callables are:
//   resolve_session(actor_ref) -> sin_assault_session_v1* or nullptr
//   collect_matching_rallies(actor_ref, rallypoint_name)
//      -> std::optional<AssaultGoalActorBindingSnapshot>
// The collector must be a synchronous, generation-safe read of an engine
// level snapshot and supply matching rallypoints only. nullopt indicates
// unavailable host entity data, NOT original "no rallypoint" behavior.
//
// Original goal setup calls SetParameters per actor. Unlike that x86 routine,
// duplicate suppression, invalid mode/name rejection, callback isolation,
// explicit statuses and exclusive-lock rollback are NEW port policies.
//
// The underlying C ABI checks NPC liveness, candidate validation and locking
// and applies the three original cue-state outcomes. This helper NEVER sends
// BeginAssault: goal configuration is distinct from the later map input.
template <typename ResolveSession, typename CollectMatchingRallies>
AssaultGoalConfigureReport configure_original_goal_assault_fanout(
    const std::vector<sin_assault_ref_v1>& actor_snapshot,
    std::string_view rallypoint_name, std::int32_t cue_mode,
    ResolveSession&& resolve_session,
    CollectMatchingRallies&& collect_matching_rallies) {
    AssaultGoalConfigureReport report{};
    report.visited = actor_snapshot.size();
    if (rallypoint_name.empty() || cue_mode < 1 || cue_mode > 3) {
        report.invalid_configuration = true;
        return report;
    }
    std::unordered_set<std::uint64_t> seen;
    seen.reserve(actor_snapshot.size());
    report.actor_results.reserve(actor_snapshot.size());
    for (const auto& actor : actor_snapshot) {
        if (!actor.index || !actor.generation) {
            ++report.invalid_handles;
            continue;
        }
        const auto key = (static_cast<std::uint64_t>(actor.generation) << 32) |
                         static_cast<std::uint64_t>(actor.index);
        if (!seen.insert(key).second) {
            ++report.duplicate_handles;
            continue;
        }
        AssaultGoalActorBindResult result{};
        result.actor = actor;
        try {
            auto* session = resolve_session(actor);
            if (!session) {
                ++report.missing_sessions;
                report.actor_results.push_back(result);
                continue;
            }
            auto data = collect_matching_rallies(actor, rallypoint_name);
            if (!data) {
                ++report.unavailable_snapshots;
                result.status = SIN_ASSAULT_HOST_FAILED;
            } else if (data->matching_rallypoints.size() >
                       SIN_ASSAULT_MAX_CANDIDATES) {
                result.status = SIN_ASSAULT_INVALID_ARGUMENT;
                ++report.host_failures;
            } else {
                const auto& candidates = data->matching_rallypoints;
                result.status = sin_assault_bind_v1(
                    session, data->npc_origin.data(),
                    candidates.empty() ? nullptr : candidates.data(),
                    static_cast<std::uint32_t>(candidates.size()),
                    cue_mode, &result.rally_id);
                switch (result.status) {
                  case SIN_ASSAULT_OK: ++report.bound; break;
                  case SIN_ASSAULT_NO_CANDIDATE: ++report.no_candidates; break;
                  case SIN_ASSAULT_LOCK_REJECTED: ++report.lock_rejected; break;
                  case SIN_ASSAULT_UNRESOLVED_RALLY:
                    ++report.unresolved_rallies; break;
                  case SIN_ASSAULT_ACTOR_EXPIRED: ++report.expired_actors; break;
                  case SIN_ASSAULT_ALREADY_BOUND: ++report.already_bound; break;
                  case SIN_ASSAULT_REENTRANT: ++report.reentrant; break;
                  default: ++report.host_failures; break;
                }
            }
        } catch (...) {
            result.status = SIN_ASSAULT_HOST_FAILED;
            result.rally_id = 0;
            ++report.host_failures;
        }
        report.actor_results.push_back(result);
    }
    return report;
}

// The *separate* original CNPC_MobGrunt::InputAssault(string) always passes
// mode 3 into SetParameters at server.dll 0x22357E8B. It does NOT send the
// goal's BeginAssault input, so do not silently mark Provoked here.
template <typename ResolveSession, typename CollectMatchingRallies>
AssaultGoalConfigureReport configure_original_mobgrunt_assault_input(
    sin_assault_ref_v1 actor,
    std::string_view rallypoint_name,
    ResolveSession&& resolve_session,
    CollectMatchingRallies&& collect_matching_rallies) {
    return configure_original_goal_assault_fanout(
        std::vector<sin_assault_ref_v1>{actor}, rallypoint_name, 3,
        std::forward<ResolveSession>(resolve_session),
        std::forward<CollectMatchingRallies>(collect_matching_rallies));
}

} // namespace sin_compat
