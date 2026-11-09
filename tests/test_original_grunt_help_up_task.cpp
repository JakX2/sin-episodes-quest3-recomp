#include "sin_compat/original_grunt_help_up_task.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
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

void test_other_start_task_delegates_without_side_effects() {
    FakeHost host;
    host.target = HostWoundedObjectToken{0xCAFEU};
    const OriginalGruntTaskId other{0x8BU};

    const auto result = start_original_grunt_task(other, host);

    assert(result == GruntTaskDispatchDisposition::DelegateToHumanoid);
    assert((host.events == std::vector<Event>{Event::ParentStart}));
    assert(host.delegated_task.value == other.value);
    assert(host.target.value == 0xCAFEU);
}

void test_other_run_task_delegates_even_outside_update_range() {
    FakeHost host;
    host.target = HostWoundedObjectToken{0xCAFEU};
    const OriginalGruntTaskId outside_update_range{0x9BU};

    const auto result = run_original_grunt_task(outside_update_range, host);

    assert(result == GruntTaskDispatchDisposition::DelegateToHumanoid);
    assert((host.events == std::vector<Event>{Event::ParentRun}));
    assert(host.delegated_task.value == outside_update_range.value);
    assert(host.target.value == 0xCAFEU);
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
    test_other_start_task_delegates_without_side_effects();
    test_other_run_task_delegates_even_outside_update_range();
    test_start_then_run_does_not_repeat_target_helper_after_clear();
    return 0;
}
