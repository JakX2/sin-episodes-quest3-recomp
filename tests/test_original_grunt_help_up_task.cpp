#include "sin_compat/original_grunt_help_up_task.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>

using namespace sin_compat::ai::grunt;

enum class Event {
    InvokeWoundedHelper,
    ClearWounded,
    CompleteTask,
    ParentStart,
    ParentRun,
};

class FakeHost final : public IOriginalGruntTaskHost {
public:
    HostWoundedObjectToken target{};
    HostWoundedObjectToken helper_target{};
    OriginalGruntTaskId delegated_task{};
    std::int32_t completion_argument = -1;
    std::vector<Event> events;

    HostWoundedObjectToken wounded_object_token() const noexcept override {
        return target;
    }

    void invoke_wounded_object_helper(
        HostWoundedObjectToken value) noexcept override {
        helper_target = value;
        events.push_back(Event::InvokeWoundedHelper);
    }

    void clear_wounded_object() noexcept override {
        target = {};
        events.push_back(Event::ClearWounded);
    }

    void complete_task_with_argument(std::int32_t argument) noexcept override {
        completion_argument = argument;
        events.push_back(Event::CompleteTask);
    }

    void delegate_start_task_to_humanoid(
        OriginalGruntTaskId task) noexcept override {
        delegated_task = task;
        events.push_back(Event::ParentStart);
    }

    void delegate_run_task_to_humanoid(
        OriginalGruntTaskId task) noexcept override {
        delegated_task = task;
        events.push_back(Event::ParentRun);
    }
};

void test_start_with_wounded_target_preserves_original_order() {
    FakeHost host;
    host.target = HostWoundedObjectToken{0x1234U};

    const auto result = start_original_grunt_task(task_grunt_help_up, host);

    assert(result == GruntTaskDispatchDisposition::Handled);
    assert((host.events == std::vector<Event>{
        Event::InvokeWoundedHelper,
        Event::ClearWounded,
        Event::CompleteTask}));
    assert(host.helper_target.value == 0x1234U);
    assert(host.target.value == 0U);
    assert(host.completion_argument == 0);
}

void test_start_with_null_target_still_clears_and_completes() {
    FakeHost host;

    const auto result = start_original_grunt_task(task_grunt_help_up, host);

    assert(result == GruntTaskDispatchDisposition::Handled);
    assert((host.events == std::vector<Event>{
        Event::ClearWounded,
        Event::CompleteTask}));
    assert(host.target.value == 0U);
    assert(host.completion_argument == 0);
}

void test_run_with_wounded_target_preserves_original_order() {
    FakeHost host;
    host.target = HostWoundedObjectToken{0xBEEFU};

    const auto result = run_original_grunt_task(task_grunt_help_up, host);

    assert(result == GruntTaskDispatchDisposition::Handled);
    assert((host.events == std::vector<Event>{
        Event::InvokeWoundedHelper,
        Event::ClearWounded,
        Event::CompleteTask}));
    assert(host.helper_target.value == 0xBEEFU);
    assert(host.target.value == 0U);
    assert(host.completion_argument == 0);
}

void test_run_with_null_target_still_clears_and_completes() {
    FakeHost host;

    const auto result = run_original_grunt_task(task_grunt_help_up, host);

    assert(result == GruntTaskDispatchDisposition::Handled);
    assert((host.events == std::vector<Event>{
        Event::ClearWounded,
        Event::CompleteTask}));
    assert(host.completion_argument == 0);
}

void test_other_registered_start_task_is_not_misrouted_to_parent() {
    FakeHost host;
    host.target = HostWoundedObjectToken{0xCAFEU};
    const OriginalGruntTaskId other_registered{0x8BU};

    const auto result = start_original_grunt_task(other_registered, host);

    assert(result == GruntTaskDispatchDisposition::UnresolvedGruntTask);
    assert(host.events.empty());
    assert(host.target.value == 0xCAFEU);
}

void test_unregistered_start_task_uses_humanoid_fallback() {
    FakeHost host;
    const OriginalGruntTaskId unregistered{0x91U};

    const auto result = start_original_grunt_task(unregistered, host);

    assert(result == GruntTaskDispatchDisposition::DelegateToHumanoid);
    assert((host.events == std::vector<Event>{Event::ParentStart}));
    assert(host.delegated_task.value == unregistered.value);
}

void test_other_explicit_update_task_is_not_misrouted_to_parent() {
    FakeHost host;
    host.target = HostWoundedObjectToken{0xCAFEU};
    const OriginalGruntTaskId other_custom_update{0x8BU};

    const auto result = run_original_grunt_task(other_custom_update, host);

    assert(result == GruntTaskDispatchDisposition::UnresolvedGruntTask);
    assert(host.events.empty());
    assert(host.target.value == 0xCAFEU);
}

void test_inherited_update_task_uses_humanoid_fallback() {
    FakeHost host;
    const OriginalGruntTaskId inherited_update{0x9BU};

    const auto result = run_original_grunt_task(inherited_update, host);

    assert(result == GruntTaskDispatchDisposition::DelegateToHumanoid);
    assert((host.events == std::vector<Event>{Event::ParentRun}));
    assert(host.delegated_task.value == inherited_update.value);
}

void test_start_then_run_does_not_repeat_target_helper_after_clear() {
    FakeHost host;
    host.target = HostWoundedObjectToken{0x777U};

    assert(start_original_grunt_task(task_grunt_help_up, host) ==
           GruntTaskDispatchDisposition::Handled);
    assert(run_original_grunt_task(task_grunt_help_up, host) ==
           GruntTaskDispatchDisposition::Handled);

    assert((host.events == std::vector<Event>{
        Event::InvokeWoundedHelper,
        Event::ClearWounded,
        Event::CompleteTask,
        Event::ClearWounded,
        Event::CompleteTask}));
}

int main() {
    test_start_with_wounded_target_preserves_original_order();
    test_start_with_null_target_still_clears_and_completes();
    test_run_with_wounded_target_preserves_original_order();
    test_run_with_null_target_still_clears_and_completes();
    test_other_registered_start_task_is_not_misrouted_to_parent();
    test_unregistered_start_task_uses_humanoid_fallback();
    test_other_explicit_update_task_is_not_misrouted_to_parent();
    test_inherited_update_task_uses_humanoid_fallback();
    test_start_then_run_does_not_repeat_target_helper_after_clear();
    std::puts("PASS: original SiN grunt help-up start/update paths and inherited fallback");
    return 0;
}
