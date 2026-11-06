#include "sin_compat/original_goal_assault_input.hpp"
#include "sin_compat/assault_schedule_runner.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

using namespace sin_compat;

static void test_goal_setup_configures_per_actor_only_on_host_success() {
    OriginalAssaultCueGate first{2, 4};
    OriginalGoalAssaultActor actor{101, &first, true};
    const OriginalGoalAssaultSetup settings{"roof_rally_*", 1};
    int host_calls = 0;
    auto rejecting = [&](std::uint64_t id, std::string_view rally, std::int32_t cue) {
        ++host_calls;
        assert(id == 101 && rally == "roof_rally_*" && cue == 1);
        return false; // Source engine failed rally-point selection
    };
    assert(!configure_goal_assault_actor(actor, settings, rejecting));
    assert(first.configured_mode == 2 && first.received_cue == 4);
    assert(host_calls == 1);

    auto accepting = [&](std::uint64_t id, std::string_view rally, std::int32_t cue) {
        ++host_calls;
        return id == 101 && rally == "roof_rally_*" && cue == 1;
    };
    assert(configure_goal_assault_actor(actor, settings, accepting));
    assert(host_calls == 2);
    assert(first.configured_mode == 1 && first.received_cue == 0);
    assert(!first.check(false)); // initial entity-input cue not yet received

    OriginalGoalAssaultActor missing{102, nullptr, true};
    OriginalGoalAssaultActor no_owner{103, &first, false};
    assert(!configure_goal_assault_actor(missing, settings, accepting));
    assert(!configure_goal_assault_actor(no_owner, settings, accepting));
    assert(host_calls == 2);
}

static void test_begin_assault_fanout_order_and_resolution_gates() {
    OriginalAssaultCueGate a{1, 0}, b{1, 0}, c{2, 0}, d{3, 0};
    const std::vector<OriginalGoalAssaultActor> actors{
        {11, &a, true}, {12, &b, false}, {13, nullptr, true},
        {14, &c, true}, {15, &d, false}
    };
    std::vector<std::pair<std::uint64_t, int>> events;
    auto reset = [&](std::uint64_t actor) {
        events.emplace_back(actor, 0);
    };
    auto condition = [&](std::uint64_t actor, int numeric_id) {
        events.emplace_back(actor, numeric_id);
    };

    assert(begin_goal_assault(actors, reset, condition) == 2);
    assert((events == std::vector<std::pair<std::uint64_t, int>>{
        {11, 0}, {11, 0x18}, {14, 0}, {14, 0x18}
    }));
    assert(a.received_cue == 1);
    assert(c.received_cue == 1);
    assert(b.received_cue == 0 && d.received_cue == 0);

    // Original handler always sends 1. Mode 2 requires its own NPC
    // gunfire condition 0x33; an input cue of 1 does not replace that check.
    assert(a.check(false));
    assert(!c.check(false));
    assert(c.check(true));
    assert(c.received_cue == 1); // gunfire check has no write

    events.clear();
    assert(begin_goal_assault(actors, reset, condition) == 2);
    assert(events.size() == 4); // repeats are not silently dropped
}

static void test_begin_assault_mode_one_enables_original_hold_rally_task() {
    OriginalAssaultCueGate target{1, 0};
    const std::vector<OriginalGoalAssaultActor> actors{{9, &target, true}};
    AssaultScheduleRunner runner(AssaultSchedule::HoldRallyPoint);
    assert(runner.start(AssaultSchedule::HoldRallyPoint));

    auto host = [&](const AssaultTaskWitness& task) {
        if (task.original_task_symbol == "TASK_AWAIT_CUE")
            return target.check(false) ? AssaultHostTaskResult::Completed
                                       : AssaultHostTaskResult::Pending;
        assert(task.original_task_symbol == "TASK_WAIT_ASSAULT_DELAY");
        return AssaultHostTaskResult::Completed;
    };
    auto r = runner.tick({}, host);
    assert(r.result == AssaultTickResult::TaskPending);
    int field_reset_count = 0;
    int condition_count = 0;
    assert(begin_goal_assault(actors,
        [&](std::uint64_t id) { assert(id == 9); ++field_reset_count; },
        [&](std::uint64_t id, int condition_id) {
            assert(id == 9 && condition_id == 0x18);
            ++condition_count;
        }) == 1);
    assert(field_reset_count == 1 && condition_count == 1);

    r = runner.tick({}, host);
    assert(r.result == AssaultTickResult::TaskCompleted);
    assert(runner.next_task_position() == 1);
    r = runner.tick({}, host);
    assert(r.result == AssaultTickResult::TaskCompleted);
    r = runner.tick({}, host);
    assert(r.result == AssaultTickResult::ScheduleFinished);
}

static void test_direct_mobgrunt_assault_uses_different_cue_route() {
    // Independently verified: original MobGrunt::InputAssault passes mode 3
    // to CAI_AssaultBehavior::SetParameters. No ai_goal_assault
    // BeginAssault input is needed for this direct-NPC path.
    OriginalAssaultCueGate direct{3, 0};
    assert(direct.check(false));
    assert(direct.received_cue == 3);
}

int main() {
    test_goal_setup_configures_per_actor_only_on_host_success();
    test_begin_assault_fanout_order_and_resolution_gates();
    test_begin_assault_mode_one_enables_original_hold_rally_task();
    test_direct_mobgrunt_assault_uses_different_cue_route();
    return 0;
}
