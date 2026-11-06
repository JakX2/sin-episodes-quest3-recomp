#include "sin_compat/assault_task_runner.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <string>
#include <vector>
int main() {
    using namespace sin_compat;
    AssaultTaskRunner runner;
    int calls = 0;
    assert(runner.tick([&](auto) { ++calls; return AssaultTaskResult::Complete; }) == AssaultRunnerState::Idle);
    runner.start(AssaultSchedule::MoveToRallyPoint);
    assert(!runner.interrupt("COND_NEW_ENEMY"));
    assert(!runner.interrupt("COND_NOT_A_REAL_CONDITION"));
    std::vector<std::string> observed;
    auto callback = [&](AssaultTaskInvocation task) {
        ++calls;
        assert(task.position == observed.size());
        observed.emplace_back(task.symbol);
        return AssaultTaskResult::Complete;
    };
    assert(runner.tick(callback) == AssaultRunnerState::Running);
    assert(observed[0] == "TASK_SET_FAIL_SCHEDULE");
    assert(runner.position() == 1);
    assert(runner.interrupt("COND_HEAR_DANGER"));
    assert(runner.tick(callback) == AssaultRunnerState::Interrupted);
    assert(calls == 1);
    runner.start(AssaultSchedule::MoveToRallyPoint);
    observed.clear();
    for (int i = 0; i < 8; ++i) {
        const auto expected = i == 7 ? AssaultRunnerState::Completed : AssaultRunnerState::Running;
        assert(runner.tick(callback) == expected);
    }
    assert(observed.back() == "TASK_SET_SCHEDULE");
    assert(runner.position() == 8);
    assert(runner.tick(callback) == AssaultRunnerState::Completed);
    runner.start(AssaultSchedule::WaitAndClear);
    assert(runner.tick([](auto task) {
        assert(task.symbol == "TASK_FACE_ASSAULT_POINT");
        return AssaultTaskResult::Running;
    }) == AssaultRunnerState::Running);
    assert(runner.position() == 0);
    assert(runner.tick([](auto) { return AssaultTaskResult::Failed; }) == AssaultRunnerState::Failed);
    assert(runner.tick(callback) == AssaultRunnerState::Failed);
    runner.start(AssaultSchedule::ClearAssaultPoint);
    assert(runner.tick([](auto task) {
        assert(task.symbol == "TASK_ANNOUNCE_CLEAR");
        return AssaultTaskResult::Complete;
    }) == AssaultRunnerState::Completed);
    return 0;
}
