#include "sin_compat/original_grunt_squad_kick_deferral.hpp"

namespace sin_compat::ai::grunt {

GruntTaskDispatchDisposition start_original_grunt_squad_kick_deferral_task(
    OriginalGruntTaskId task,
    IOriginalGruntSquadKickDeferralHost& host) {
    if (task != task_grunt_defer_squad_grenade_kick) {
        return start_original_grunt_task(
            task, static_cast<IOriginalGruntTaskHost&>(host));
    }

    // Phase 10 identifies the ordered squad traversal and the per-resolved-
    // member +0xCB8 invalidation. The host owns list resolution and identity.
    const auto members = host.resolved_squad_members_in_original_order();
    for (const OriginalGruntEntityToken member : members) {
        host.clear_member_current_grenade_handle(member);
    }

    // The original start case then calls the shared task-completion helper
    // with argument zero.
    host.complete_task_with_argument(0);
    return GruntTaskDispatchDisposition::Handled;
}

} // namespace sin_compat::ai::grunt
