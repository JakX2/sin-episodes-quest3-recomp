// A deterministic *synthetic* SiN encounter. Joins three separately verified
// original gameplay references (rally rank, live-owner lease, schedule tasks)
// without inventing SourceVR native movement, weapons or AI conditions.

#include "sin_compat/assault_schedule_runner.hpp"
#include "sin_compat/mobgrunt_rally_rank.hpp"
#include "sin_compat/rally_point_lease.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstdint>
#include <optional>
#include <set>
#include <string_view>
#include <vector>

namespace {

using namespace sin_compat;
using Actor = RallyPointLease::OwnerToken;
constexpr Actor grunt_a = 0x110001;
constexpr Actor grunt_b = 0x220002;

struct FakeWorld {
    std::set<Actor> alive{grunt_a, grunt_b};
    std::array<RallyPointLease, 2> leases{};

    bool is_alive(Actor actor) const { return alive.count(actor) != 0; }

    // Original nearest-priority selection: host resolves live owner handles.
    // Coordinates and priority here are synthetic, NOT from the user's BSP.
    std::optional<std::size_t> reserve(Actor actor) {
        std::vector<RallyPointCandidate> choices{
            {7, {{10.f, 0.f, 0.f}}, !locked(leases[0])},
            {3, {{2.f, 0.f, 0.f}}, !locked(leases[1])},
        };
        const auto picked = choose_mobgrunt_rally_point(
            {{0.f, 0.f, 0.f}}, choices);
        if (!picked) return std::nullopt;
        const auto owner_live = [this](Actor existing) { return is_alive(existing); };
        if (!leases[picked->index].acquire(actor, owner_live))
            return std::nullopt;
        return picked->index;
    }

    bool locked(const RallyPointLease& lease) const {
        return lease.current_owner() && is_alive(*lease.current_owner());
    }

    bool free_by(std::size_t point, Actor owner) {
        return leases[point].release(
            owner, [this](Actor existing) { return is_alive(existing); });
    }
};

struct SyntheticHost {
    int movement_retries = 1;
    bool reached_rally = false;
    bool assault_cue = false;
    bool delay_finished = false;
    int called = 0;

    AssaultHostTaskResult run(const AssaultTaskWitness& task) {
        ++called;
        if (task.original_task_symbol == "TASK_WAIT_FOR_MOVEMENT") {
            if (movement_retries-- > 0) return AssaultHostTaskResult::Pending;
        }
        if (task.original_task_symbol == "TASK_HIT_RALLY_POINT") {
            reached_rally = true; // explicit fake event, NOT Source I/O
        }
        if (task.original_task_symbol == "TASK_AWAIT_CUE" && !assault_cue)
            return AssaultHostTaskResult::Pending;
        if (task.original_task_symbol == "TASK_WAIT_ASSAULT_DELAY" &&
            !delay_finished)
            return AssaultHostTaskResult::Pending;
        return AssaultHostTaskResult::Completed;
    }
};

void test_two_npcs_have_exclusive_rally_targets() {
    FakeWorld world;
    const auto a = world.reserve(grunt_a);
    const auto b = world.reserve(grunt_b);
    assert(a == std::size_t(0));
    assert(b == std::size_t(1));
    assert(world.locked(world.leases[0]));
    assert(world.locked(world.leases[1]));
    assert(!world.reserve(grunt_a));
    assert(!world.reserve(grunt_b));

    // The original owner-only lease rule forbids another live actor's unlock.
    assert(!world.free_by(*a, grunt_b));
    assert(world.leases[*a].current_owner() == grunt_a);

    // As with the original handle, an expired owner releases eligibility.
    world.alive.erase(grunt_a);
    assert(world.free_by(*a, grunt_b));
    const auto reused = world.reserve(grunt_b);
    assert(reused && *reused == *a);
    assert(world.leases[*a].current_owner() == grunt_b);
}

void test_rally_navigation_cue_and_delay_come_from_host() {
    FakeWorld world;
    assert(world.reserve(grunt_a) == std::size_t(0));
    assert(world.reserve(grunt_b) == std::size_t(1));

    AssaultScheduleRunner runner(AssaultSchedule::MoveToRallyPoint);
    assert(runner.start(AssaultSchedule::MoveToRallyPoint));
    SyntheticHost host;

    auto dispatch = [&host](const AssaultTaskWitness& task) {
        return host.run(task);
    };

    // If a Source-like engine reports danger on an interruptible schedule,
    // the reference returns an interruption and *never* invents a new AI plan.
    auto result = runner.tick({"COND_HEAR_DANGER"}, dispatch);
    assert(result.result == AssaultTickResult::Interrupted);
    assert(host.called == 0);
    assert(runner.next_task_position() == 0);

    bool saw_pending_movement = false;
    bool saw_explicit_hold = false;
    for (int i = 0; i < 24; ++i) {
        result = runner.tick({}, dispatch);
        if (result.result == AssaultTickResult::TaskPending)
            saw_pending_movement = true;
        if (result.result == AssaultTickResult::ExplicitScheduleChange) {
            assert(result.new_schedule == AssaultSchedule::HoldRallyPoint);
            saw_explicit_hold = true;
            break;
        }
    }
    assert(saw_pending_movement);
    assert(saw_explicit_hold);
    assert(host.reached_rally);
    assert(runner.current_schedule() == AssaultSchedule::HoldRallyPoint);

    // Original declarative hold requires a cue AND an assault delay. Their
    // actual engine-side meaning remains an unimplemented host obligation.
    result = runner.tick({}, dispatch);
    assert(result.result == AssaultTickResult::TaskPending);
    assert(result.task_symbol == "TASK_AWAIT_CUE");

    host.assault_cue = true;
    result = runner.tick({}, dispatch);
    assert(result.result == AssaultTickResult::TaskCompleted);

    result = runner.tick({}, dispatch);
    assert(result.result == AssaultTickResult::TaskPending);
    assert(result.task_symbol == "TASK_WAIT_ASSAULT_DELAY");

    host.delay_finished = true;
    result = runner.tick({}, dispatch);
    assert(result.result == AssaultTickResult::TaskCompleted);
    result = runner.tick({}, dispatch);
    assert(result.result == AssaultTickResult::ScheduleFinished);
    assert(!runner.active());

    // Finishing a hold schedule does not authorize the runner to unlock a
    // rally point or pick the NPC's next Source schedule automatically.
    assert(world.leases[0].current_owner() == grunt_a);
    assert(world.free_by(0, grunt_a)); // native host determines when
}

void test_invalid_live_host_does_not_bypass_original_lease() {
    FakeWorld world;
    const auto selected = world.reserve(grunt_a);
    assert(selected == std::size_t(0));
    assert(!world.free_by(*selected, grunt_b));

    // A second actor cannot force a lock through a stale initial selection.
    assert(!world.leases[*selected].acquire(
        grunt_b, [&world](Actor existing) { return world.is_alive(existing); }));
    assert(world.leases[*selected].current_owner() == grunt_a);
}

} // namespace

int main() {
    test_two_npcs_have_exclusive_rally_targets();
    test_rally_navigation_cue_and_delay_come_from_host();
    test_invalid_live_host_does_not_bypass_original_lease();
}
