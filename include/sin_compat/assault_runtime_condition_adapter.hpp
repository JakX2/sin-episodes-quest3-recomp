#pragma once

// Engine-neutral condition-registration boundary for reconstructed SiN assault
// behavior. The original 2006 numeric condition IDs are research witnesses,
// NOT the IDs that an ARM64 Source-compatible engine must use.
//
// Original semantics and condition names come from the SHA-pinned 2006
// server.dll audit in docs/ORIGINAL_ASSAULT_CONDITION_IDS_NOV07.md.
// Registration policy, duplicate checks and host callbacks below are NEW
// defensive port mechanisms, not recovered Ritual source or SourceVR ABI.

#include "sin_compat/original_assault_condition_ids.hpp"
#include "sin_compat/original_goal_assault_input.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace sin_compat {

// Deliberately not implicitly convertible from LegacyAssaultConditionId,
// task IDs, schedule IDs or an untyped integer.
struct RuntimeAssaultConditionId {
    explicit constexpr RuntimeAssaultConditionId(std::uint32_t id) : value(id) {}
    std::uint32_t value;
};

class AssaultRuntimeConditionBindings {
public:
    // Each known semantic gets exactly one runtime ID; one runtime ID cannot
    // alias a different known condition. Repeating an identical bind is safe.
    // UINT32_MAX is reserved as an invalid/unregistered host sentinel.
    bool bind(OriginalAssaultCondition semantic, RuntimeAssaultConditionId native) {
        if (native.value == std::numeric_limits<std::uint32_t>::max())
            return false;

        for (std::size_t i = 0; i < original_assault_condition_ids.size(); ++i) {
            if (original_assault_condition_ids[i].semantic != semantic)
                continue;
            if (registered_[i])
                return registered_[i]->value == native.value;
            for (std::size_t j = 0; j < registered_.size(); ++j) {
                if (registered_[j] && registered_[j]->value == native.value)
                    return false;
            }
            registered_[i] = native;
            return true;
        }
        return false;
    }

    std::optional<RuntimeAssaultConditionId> resolve(
        OriginalAssaultCondition semantic) const {
        for (std::size_t i = 0; i < original_assault_condition_ids.size(); ++i) {
            if (original_assault_condition_ids[i].semantic == semantic)
                return registered_[i];
        }
        return std::nullopt;
    }

private:
    std::array<std::optional<RuntimeAssaultConditionId>,
               original_assault_condition_ids.size()> registered_{};
};

// Fail closed before changing NPC fields or received cues when the engine
// has not registered COND_PROVOKED. The strongly typed sink can never be
// accidentally passed the original 2006 value 0x18 by this interface.
template <typename HostResetNpcFields, typename HostSetNativeCondition>
std::optional<std::size_t> begin_goal_assault_with_runtime_conditions(
    const std::vector<OriginalGoalAssaultActor>& actors,
    const AssaultRuntimeConditionBindings& bindings,
    HostResetNpcFields&& reset_npc_fields,
    HostSetNativeCondition&& set_native_condition) {
    const auto provoked = bindings.resolve(OriginalAssaultCondition::Provoked);
    if (!provoked) return std::nullopt;

    return begin_goal_assault_semantic(
        actors, std::forward<HostResetNpcFields>(reset_npc_fields),
        [&](std::uint64_t actor, OriginalAssaultCondition semantic) {
            // Original InputBeginAssault signals only COND_PROVOKED.
            // If the source-derived helper ever changes, tests should flag
            // that before an engine adapter attempts different condition IDs.
            assert(semantic == OriginalAssaultCondition::Provoked);
            (void)semantic;
            set_native_condition(actor, *provoked);
        });
}

// Original cue mode 2 queries COND_HEAR_COMBAT (2006 ID 0x33).
// Other modes, including received_cue==4 override, never need a host query.
// A missing runtime registration in mode 2 returns nullopt instead of
// accidentally testing the 2006 numeric ID against the native condition map.
template <typename HostQueryNativeCondition>
std::optional<bool> check_assault_cue_with_runtime_conditions(
    OriginalAssaultCueGate& gate,
    std::uint64_t actor,
    const AssaultRuntimeConditionBindings& bindings,
    HostQueryNativeCondition&& query_native_condition) {
    if (gate.received_cue == 4 || gate.configured_mode != 2)
        return gate.check(false);

    const auto hear_combat = bindings.resolve(OriginalAssaultCondition::HearCombat);
    if (!hear_combat) return std::nullopt;
    return gate.check(static_cast<bool>(query_native_condition(actor, *hear_combat)));
}

} // namespace sin_compat
