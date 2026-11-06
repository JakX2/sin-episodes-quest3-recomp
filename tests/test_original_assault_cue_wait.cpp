#include "sin_compat/original_assault_cue_wait.hpp"
#include "sin_compat/assault_schedule_runner.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>

using namespace sin_compat;

static void test_original_cue_decision_table() {
    OriginalAssaultCueGate cue;
    cue.configured_mode = 0;
    cue.received_cue = 0;
    assert(!cue.check(false));
    assert(!cue.check(true));

    cue.configured_mode = 1; // original FGD: Entity System Input
    assert(!cue.check(false));
    cue.received_cue = 2;
    assert(!cue.check(true));
    cue.received_cue = 1;
    assert(cue.check(false)); // no gunfire condition needed

    cue.configured_mode = 2; // original FGD: Gunfire
    cue.received_cue = 0;
    assert(!cue.check(false));
    assert(cue.check(true));
    assert(cue.received_cue == 0); // gunfire path does NOT set received cue

    cue.configured_mode = 3; // original FGD: Don't wait for a cue
    assert(cue.check(false));
    assert(cue.received_cue == 3);

    cue.configured_mode = 4; // original code's extra unnamed reserved case
    cue.received_cue = 0;
    assert(!cue.check(true));
    cue.configured_mode = 999;
    assert(!cue.check(true));
    cue.configured_mode = -1;
    assert(!cue.check(true));

    // Original helper: a received cue value 4 bypasses all configured modes.
    cue.received_cue = 4;
    assert(cue.check(false));
    assert(cue.received_cue == 4);
}

static void test_original_wait_quantization_and_completion() {
    auto now = original_fixed_npc_wait_deadline(10.0f, 0.0f);
    assert(now && *now == 10.0f);
    auto whole = original_fixed_npc_wait_deadline(10.0f, 3.0f);
    assert(whole && std::fabs(*whole - 13.0f) < 0.0001f);

    // Original x87 rounds UP to next 1/10 second, not down.
    auto ceiling = original_fixed_npc_wait_deadline(10.0f, 3.21f);
    assert(ceiling && std::fabs(*ceiling - 13.3f) < 0.0001f);
    auto small = original_fixed_npc_wait_deadline(10.0f, 0.001f);
    assert(small && std::fabs(*small - 10.1f) < 0.0001f);
    auto past = original_fixed_npc_wait_deadline(10.0f, -1.0f);
    assert(past && std::fabs(*past - 9.0f) < 0.0001f);

    auto before = original_fixed_npc_wait_finished(13.29f, *ceiling);
    auto equal = original_fixed_npc_wait_finished(*ceiling, *ceiling);
    auto after = original_fixed_npc_wait_finished(13.31f, *ceiling);
    assert(before && !*before);
    assert(equal && *equal);
    assert(after && *after);

    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();
    assert(!original_fixed_npc_wait_deadline(nan, 0.5f));
    assert(!original_fixed_npc_wait_deadline(10.0f, infinity));
    assert(!original_fixed_npc_wait_deadline(10.0f,
                                             std::numeric_limits<float>::max()));
    assert(!original_fixed_npc_wait_finished(10.0f, nan));
    assert(!original_fixed_npc_wait_finished(infinity, 11.0f));
}

static void test_original_cue_and_wait_inside_host_runner() {
    AssaultScheduleRunner runner(AssaultSchedule::HoldRallyPoint);
    assert(runner.start(AssaultSchedule::HoldRallyPoint));
    OriginalAssaultCueGate cue;
    cue.configured_mode = 1;
    float curtime = 20.0f;
    const float rally_assault_delay = 0.21f;
    std::optional<float> deadline;
    int cue_calls = 0;
    int wait_calls = 0;

    auto fake_host = [&](const AssaultTaskWitness& task) {
        if (task.original_task_symbol == "TASK_AWAIT_CUE") {
            ++cue_calls;
            return cue.check(false) ? AssaultHostTaskResult::Completed
                                    : AssaultHostTaskResult::Pending;
        }
        assert(task.original_task_symbol == "TASK_WAIT_ASSAULT_DELAY");
        ++wait_calls;
        if (!deadline) {
            deadline = original_fixed_npc_wait_deadline(curtime, rally_assault_delay);
            assert(deadline);
        }
        const auto done = original_fixed_npc_wait_finished(curtime, *deadline);
        assert(done.has_value());
        return *done ? AssaultHostTaskResult::Completed
                     : AssaultHostTaskResult::Pending;
    };

    auto r = runner.tick({}, fake_host);
    assert(r.result == AssaultTickResult::TaskPending);
    assert(runner.next_task_position() == 0);
    cue.received_cue = 1; // synthetic owner-directed cue arrives
    r = runner.tick({}, fake_host);
    assert(r.result == AssaultTickResult::TaskCompleted);
    assert(runner.next_task_position() == 1);

    r = runner.tick({}, fake_host);
    assert(r.result == AssaultTickResult::TaskPending);
    assert(deadline && std::fabs(*deadline - 20.3f) < 0.0001f);
    curtime = 20.2f;
    r = runner.tick({}, fake_host);
    assert(r.result == AssaultTickResult::TaskPending);
    curtime = 20.4f;
    r = runner.tick({}, fake_host);
    assert(r.result == AssaultTickResult::TaskCompleted);
    assert(runner.next_task_position() == 2);
    r = runner.tick({}, fake_host);
    assert(r.result == AssaultTickResult::ScheduleFinished);
    assert(!runner.active());
    assert(cue_calls == 2 && wait_calls == 3);
}

int main() {
    test_original_cue_decision_table();
    test_original_wait_quantization_and_completion();
    test_original_cue_and_wait_inside_host_runner();
    return 0;
}
