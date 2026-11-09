#include "sin_compat/original_grunt_help_up_task.hpp"

namespace sin_compat::ai::grunt {
namespace {

void execute_help_up_path(IOriginalGruntTaskHost& host) noexcept {
    const HostWoundedObjectToken target = host.wounded_object_token();

    // Both original case bodies test m_pWounded before calling 0x2235A0F0.
    if (target) {
        host.invoke_wounded_object_helper(target);
    }

    // The fixed instruction anchors at 0x2235C747 and 0x2235CF5D place this
    // clear on the shared null/non-null path, so it is unconditional even
    // when m_pWounded was already null.
    host.clear_wounded_object();

    // Both original case paths reach 0x2203D2D0 with a zero argument.
    host.complete_task_with_argument(0);
}

} // namespace

GruntTaskDispatchDisposition start_original_grunt_task(
    OriginalGruntTaskId task,
    IOriginalGruntTaskHost& host) noexcept {
    if (task != task_grunt_help_up) {
        host.delegate_start_task_to_humanoid(task);
        return GruntTaskDispatchDisposition::DelegateToHumanoid;
    }

    execute_help_up_path(host);
    return GruntTaskDispatchDisposition::Handled;
}

GruntTaskDispatchDisposition run_original_grunt_task(
    OriginalGruntTaskId task,
    IOriginalGruntTaskHost& host) noexcept {
    if (task != task_grunt_help_up) {
        host.delegate_run_task_to_humanoid(task);
        return GruntTaskDispatchDisposition::DelegateToHumanoid;
    }

    execute_help_up_path(host);
    return GruntTaskDispatchDisposition::Handled;
}

} // namespace sin_compat::ai::grunt
