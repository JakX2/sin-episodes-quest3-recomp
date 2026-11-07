#pragma once

// Source-grounded, engine-neutral bridge for original 2006 SiN
// ai_goal_assault configuration and BeginAssault(void) input.
// Original .data typedescription_t at 0x224E53AC and native handler
// at 0x220CFF30, independently verified from the original server.dll.
//
// A successful original BeginAssault actor iteration:
//  1. resolves actor and CAI_AssaultBehavior,
//  2. resets original owner NPC fields +0x864/+0x85C (if owner pointer exists),
//  3. writes m_ReceivedAssaultCue=1 in behavior,
//  4. invokes NPC condition setter with COND_PROVOKED (original ID 0x18).
// Engine handles, Source goal-entity iteration and NPC ownership are all host-
// supplied; this header does not use any original x86 object offset as ABI.

#include "sin_compat/original_assault_cue_wait.hpp"
#include "sin_compat/original_assault_condition_ids.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace sin_compat {

struct OriginalGoalAssaultActor {
    // Opaque portable handle, not a native 2006 EHANDLE.
    std::uint64_t actor_token = 0;
    OriginalAssaultCueGate* behavior_cue = nullptr;
    bool owning_npc_resolved = false;
};

struct OriginalGoalAssaultSetup {
    std::string_view rallypoint_name;
    std::int32_t assault_cue_mode = 1;
};

// Original goal setup helper at 0x220CFF00 passes goal m_RallyPoint
// (+0x3C8) and m_AssaultCue (+0x3CC) to per-NPC SetParameters.
// Native SetParameters can fail to find/lock a rally point; the engine host
// MUST report whether it actually accepted these parameters. Only then do
// we update our test-harness cue fields to mirror initialization state.
template <typename HostSetParameters>
bool configure_goal_assault_actor(
    const OriginalGoalAssaultActor& actor,
    const OriginalGoalAssaultSetup& setup,
    HostSetParameters&& set_parameters) {
    if (!actor.behavior_cue || !actor.owning_npc_resolved) return false;
    if (!set_parameters(actor.actor_token, setup.rallypoint_name,
                        setup.assault_cue_mode))
        return false;
    actor.behavior_cue->configured_mode = setup.assault_cue_mode;
    actor.behavior_cue->received_cue = 0;
    return true;
}

// Source InputBeginAssault uses this per-actor *ordering*. No cue-mode test
// occurs in the original handler: it writes received=1 even if the chosen
// mode is 2 or 3, leaving the cue gate to interpret its own mode.
// Host callbacks are used for the two unknown NPC fields and original COND_PROVOKED (ID 0x18); its name and numeric
// registration are now independently verified from the original DLL.
// Semantic variant for new ports: never expose historical x86 numeric
// conditions to the native runtime. The original registration still proves
// the historical numeric ID for COND_PROVOKED, but the host decides its own ID.
template <typename HostResetNpcFields, typename HostSetSemanticCondition>
std::size_t begin_goal_assault_semantic(
    const std::vector<OriginalGoalAssaultActor>& actors,
    HostResetNpcFields&& reset_npc_fields,
    HostSetSemanticCondition&& set_npc_condition) {
    std::size_t signaled = 0;
    for (const auto& actor : actors) {
        // Original invalid actor / missing assault behavior is skipped.
        // A missing owner is skipped as a NEW host-side safety choice.
        if (!actor.behavior_cue || !actor.owning_npc_resolved)
            continue;
        reset_npc_fields(actor.actor_token);
        actor.behavior_cue->received_cue = 1;
        set_npc_condition(actor.actor_token, OriginalAssaultCondition::Provoked);
        ++signaled;
    }
    return signaled;
}

// Preserve the original number-based reference API for evidence/tests.
// Historical IDs must never be passed directly to an ARM64 engine API.
template <typename HostResetNpcFields, typename HostSetNpcCondition>
std::size_t begin_goal_assault(
    const std::vector<OriginalGoalAssaultActor>& actors,
    HostResetNpcFields&& reset_npc_fields,
    HostSetNpcCondition&& set_npc_condition) {
    return begin_goal_assault_semantic(
        actors, std::forward<HostResetNpcFields>(reset_npc_fields),
        [&](std::uint64_t token, OriginalAssaultCondition semantic) {
            set_npc_condition(token, original_assault_condition_id(semantic).value);
        });
}

} // namespace sin_compat
