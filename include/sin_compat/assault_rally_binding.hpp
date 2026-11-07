#pragma once

// Engine-neutral assembly of three independently recovered 2006 SiN pieces:
// priority/distance rally ranking, CAssaultRallyPoint ownership, and
// CAI_AssaultBehavior::SetParameters cue-state updates.
// The host's lock recheck and failed-resolution rollback are NEW safeguards.
// Original server.dll calls the lock at 0x220CFE49 but does not test its
// Boolean result before looking up the selected point's entity handle.
// This does not create Source NPCs or use original x86 object layouts.

#include "sin_compat/mobgrunt_rally_rank.hpp"
#include "sin_compat/original_assault_setparameters_outcomes.hpp"
#include "sin_compat/rally_point_lease.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace sin_compat {

struct AssaultRallyBindingCandidate {
    RallyPointCandidate rank;
    RallyPointLease ownership;
};

enum class AssaultRallyBindStatus : std::uint8_t {
    NoCandidate,
    LockRejected,  // new port guard when lock ownership changes after ranking
    UnresolvedRally,
    Bound,
};

struct AssaultRallyBindReport {
    AssaultRallyBindStatus status;
    std::optional<std::size_t> selected_index;
};

// IsLive(owner_token) must resolve the host's generation-safe entity identity.
// ResolvePoint(index) must test the selected rally entity's actual validity.
// Neither callback encodes 2006 Windows EHANDLE bits into Quest host memory.
// The lease is rolled back on failed resolution (port policy, NOT original).
// A failed binding never resets an earlier received cue, and an occupied
// candidate is excluded even when occupied by this same live actor.
template <typename IsLive, typename ResolvePoint>
AssaultRallyBindReport bind_original_assault_rally_point(
    OriginalAssaultCueGate& cue,
    const std::array<float, 3>& npc_origin,
    std::vector<AssaultRallyBindingCandidate>& candidates,
    RallyPointLease::OwnerToken actor_token,
    std::int32_t requested_mode,
    IsLive&& is_live,
    ResolvePoint&& resolve_point) {
    std::vector<RallyPointCandidate> rankable;
    rankable.reserve(candidates.size());
    for (const auto& candidate : candidates) {
        auto rank = candidate.rank;
        const auto owner = candidate.ownership.current_owner();
        if (owner && is_live(*owner)) rank.eligible = false;
        rankable.push_back(rank);
    }
    const auto choice = choose_mobgrunt_rally_point(npc_origin, rankable);
    if (!choice) {
        apply_original_setparameters_cue_effects(
            cue, requested_mode, OriginalAssaultParameterOutcome::NoRallyCandidate);
        return {AssaultRallyBindStatus::NoCandidate, std::nullopt};
    }

    auto& point = candidates[choice->index];
    // Unlike the original SetParameters caller, check that the reservation
    // actually succeeded before treating the selected point as ours.
    if (!point.ownership.acquire(actor_token, is_live)) {
        return {AssaultRallyBindStatus::LockRejected, choice->index};
    }

    if (!resolve_point(choice->index)) {
        // The original x86 invalid-handle path leaves the configured mode 0;
        // proactively releasing the acquired lease is a new host safeguard.
        point.ownership.release(actor_token, is_live);
        apply_original_setparameters_cue_effects(
            cue, requested_mode, OriginalAssaultParameterOutcome::UnresolvedRally);
        return {AssaultRallyBindStatus::UnresolvedRally, choice->index};
    }
    apply_original_setparameters_cue_effects(
        cue, requested_mode, OriginalAssaultParameterOutcome::ResolvedRally);
    return {AssaultRallyBindStatus::Bound, choice->index};
}

} // namespace sin_compat
