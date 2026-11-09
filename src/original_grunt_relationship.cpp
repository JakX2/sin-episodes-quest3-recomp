#include "sin_compat/original_grunt_relationship.hpp"
namespace sin_compat::ai::grunt {

std::int32_t original_grunt_relation_type(
    const IOriginalGruntRelationshipHost& host,
    bool hostility_mode_enabled,
    OriginalGruntEntityToken target) noexcept {
    const std::int32_t parent_relation = host.parent_relation_type(target);
    if (!hostility_mode_enabled) return parent_relation;

    // Original slot 240 checks IsNPC first and short-circuits before IsPlayer.
    if (host.target_is_npc(target) || host.target_is_player(target))
        return disposition_hate;
    return disposition_neutral;
}

OriginalGruntConditionGatherResult gather_original_grunt_conditions(
    IOriginalGruntRelationshipHost& host,
    bool hostility_mode_enabled) {
    OriginalGruntConditionGatherResult result{};
    if (!hostility_mode_enabled) {
        host.delegate_parent_condition_gather();
        return result;
    }

    result.disposition =
        OriginalGruntConditionGatherDisposition::PartialHostilityMode;
    const auto targets = host.relationship_targets_in_original_order();
    for (const OriginalGruntEntityToken target : targets) {
        ++result.targets_visited;

        // Slot 316 sets SEE_PLAYER before invoking the relationship override.
        if (host.target_is_player(target))
            host.set_ai_condition(OriginalSharedAiCondition::SeePlayer);

        if (original_grunt_relation_type(host, true, target) ==
            disposition_neutral) {
            ++result.neutral_targets_skipped;
            continue;
        }

        if (target == host.current_enemy())
            host.set_ai_condition(OriginalSharedAiCondition::SeeEnemy);

        const std::int32_t priority = host.relation_priority(target);
        if (priority < 0)
            host.set_ai_condition(OriginalSharedAiCondition::SeeDislike);
        else if (priority > 10)
            host.set_ai_condition(OriginalSharedAiCondition::SeeNemesis);
        else
            host.set_ai_condition(OriginalSharedAiCondition::SeeHate);

        host.defer_unrecovered_target_condition_tail(target);
        ++result.unresolved_tails_deferred;
    }
    return result;
}
} // namespace sin_compat::ai::grunt
