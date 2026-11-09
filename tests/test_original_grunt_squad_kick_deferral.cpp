#include "sin_compat/original_grunt_squad_kick_deferral.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>

using namespace sin_compat::ai::grunt;

namespace {
enum class Event { ClearMemberGrenade, CompleteTask, ParentStart, ParentRun };

class FakeHost final : public IOriginalGruntSquadKickDeferralHost {
public:
    std::vector<OriginalGruntEntityToken> members;
    std::vector<OriginalGruntEntityToken> cleared_members;
    std::vector<Event> events;
    OriginalGruntTaskId delegated_task{};
    std::int32_t completion_argument = -1;

    HostWoundedObjectToken wounded_object_token() const noexcept override {
        return {};
    }
    void invoke_wounded_object_helper(HostWoundedObjectToken) noexcept override {}
    void clear_wounded_object() noexcept override {}
    void complete_task_with_argument(std::int32_t argument) noexcept override {
        completion_argument = argument;
        events.push_back(Event::CompleteTask);
    }
    void delegate_start_task_to_humanoid(OriginalGruntTaskId task) noexcept override {
        delegated_task = task;
        events.push_back(Event::ParentStart);
    }
    void delegate_run_task_to_humanoid(OriginalGruntTaskId task) noexcept override {
        delegated_task = task;
        events.push_back(Event::ParentRun);
    }
    std::vector<OriginalGruntEntityToken>
    resolved_squad_members_in_original_order() const override {
        return members;
    }
    void clear_member_current_grenade_handle(
        OriginalGruntEntityToken member) noexcept override {
        cleared_members.push_back(member);
        events.push_back(Event::ClearMemberGrenade);
    }
};

void test_registered_task_id() {
    assert(task_grunt_defer_squad_grenade_kick.value == 0x9BU);
}

void test_clears_each_resolved_member_in_host_order_then_completes() {
    FakeHost host;
    host.members = {0x101U, 0x303U, 0x202U};

    const auto result = start_original_grunt_squad_kick_deferral_task(
        task_grunt_defer_squad_grenade_kick, host);

    assert(result == GruntTaskDispatchDisposition::Handled);
    assert((host.cleared_members ==
            std::vector<OriginalGruntEntityToken>{0x101U, 0x303U, 0x202U}));
    assert((host.events == std::vector<Event>{
        Event::ClearMemberGrenade,
        Event::ClearMemberGrenade,
        Event::ClearMemberGrenade,
        Event::CompleteTask}));
    assert(host.completion_argument == 0);
}

void test_empty_resolved_squad_still_completes_task() {
    FakeHost host;

    const auto result = start_original_grunt_squad_kick_deferral_task(
        task_grunt_defer_squad_grenade_kick, host);

    assert(result == GruntTaskDispatchDisposition::Handled);
    assert(host.cleared_members.empty());
    assert((host.events == std::vector<Event>{Event::CompleteTask}));
    assert(host.completion_argument == 0);
}

void test_other_registered_task_remains_unresolved() {
    FakeHost host;
    host.members = {0x111U};

    const auto result = start_original_grunt_squad_kick_deferral_task(
        OriginalGruntTaskId{0x8BU}, host);

    assert(result == GruntTaskDispatchDisposition::UnresolvedGruntTask);
    assert(host.events.empty());
    assert(host.cleared_members.empty());
}

void test_unregistered_task_uses_humanoid_start_fallback() {
    FakeHost host;
    const OriginalGruntTaskId task{0x91U};

    const auto result = start_original_grunt_squad_kick_deferral_task(task, host);

    assert(result == GruntTaskDispatchDisposition::DelegateToHumanoid);
    assert((host.events == std::vector<Event>{Event::ParentStart}));
    assert(host.delegated_task.value == task.value);
}
} // namespace

int main() {
    test_registered_task_id();
    test_clears_each_resolved_member_in_host_order_then_completes();
    test_empty_resolved_squad_still_completes_task();
    test_other_registered_task_remains_unresolved();
    test_unregistered_task_uses_humanoid_start_fallback();
    std::puts("PASS: original SiN grunt squad grenade-kick deferral start path (host-supplied squad resolution)");
    return 0;
}
