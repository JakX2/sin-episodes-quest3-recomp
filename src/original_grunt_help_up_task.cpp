#include "sin_compat/original_grunt_help_up_task.hpp"

namespace sin_compat::ai::grunt {
namespace {

constexpr std::uint32_t registered_grunt_task_ids[] = {
    0x8BU, 0x8CU, 0x8DU, 0x8EU, 0x8FU, 0x90U, 0x92U, 0x93U, 0x94U,
    0x95U, 0x96U, 0x97U, 0x98U, 0x99U, 0x9AU, 0x9BU, 0x9CU, 0x9DU,
    0x9EU,
};

constexpr std::uint32_t explicit_grunt_update_task_ids[] = {
    0x8BU, 0x8DU, 0x8FU, 0x92U, 0x93U, 0x94U, 0x95U, 0x9AU,
};

template <std::size_t N>
bool contains_task_id(const std::uint32_t (&ids)[N],
                      OriginalGruntTaskId task) noexcept {
    for (const std::uint32_t id : ids) {
        if (id == task.value) {
            return true;
        }
    }
    return false;
}

void execute_help_up_path(IOriginalGruntTaskHost& host) noexcept {
    const HostWoundedObjectToken target = host.wounded_object_token();

    // Both original case bodies test m_pWounded before calling 0x2235A0F0.
    if (target) {
        host.invoke_wounded_object_helper(target);
    }

    // The saved Phase 10 disassembly report places the field clear on the
    // common completion path after the null check.
    host.clear_wounded_object();

    // Both original case paths reach 0x2203D2D0 with a zero argument.
    host.complete_task_with_argument(0);
}

} // namespace

GruntTaskDispatchDisposition start_original_grunt_task(
    OriginalGruntTaskId task,
    IOriginalGruntTaskHost& host) noexcept {
    if (task == task_grunt_help_up) {
        execute_help_up_path(host);
        return GruntTaskDispatchDisposition::Handled;
    }

    if (contains_task_id(registered_grunt_task_ids, task)) {
        return GruntTaskDispatchDisposition::UnresolvedGruntTask;
    }

    host.delegate_start_task_to_humanoid(task);
    return GruntTaskDispatchDisposition::DelegateToHumanoid;
}

GruntTaskDispatchDisposition run_original_grunt_task(
    OriginalGruntTaskId task,
    IOriginalGruntTaskHost& host) noexcept {
    if (task == task_grunt_help_up) {
        execute_help_up_path(host);
        return GruntTaskDispatchDisposition::Handled;
    }

    if (contains_task_id(explicit_grunt_update_task_ids, task)) {
        return GruntTaskDispatchDisposition::UnresolvedGruntTask;
    }

    host.delegate_run_task_to_humanoid(task);
    return GruntTaskDispatchDisposition::DelegateToHumanoid;
}

} // namespace sin_compat::ai::grunt
