#include "sin_compat/assault_rally_binding.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <set>
#include <vector>

using namespace sin_compat;
using Token = RallyPointLease::OwnerToken;

int main() {
    const std::array<float, 3> origin{{0.f, 0.f, 0.f}};
    std::set<Token> live{11, 22, 33};
    const auto is_live = [&live](Token x) { return live.count(x) != 0; };

    {
        OriginalAssaultCueGate cue{2, 4};
        std::vector<AssaultRallyBindingCandidate> points{};
        bool called = false;
        const auto r = bind_original_assault_rally_point(cue, origin, points,
            11, 3, is_live, [&](std::size_t) { called = true; return true; });
        assert(r.status == AssaultRallyBindStatus::NoCandidate && !r.selected_index);
        assert(cue.configured_mode == 2 && cue.received_cue == 4 && !called);
    }

    {
        OriginalAssaultCueGate cue{1, 1};
        std::vector<AssaultRallyBindingCandidate> points{
            {RallyPointCandidate{9, {{1.f, 0.f, 0.f}}, true}, {}},
            {RallyPointCandidate{3, {{10.f, 0.f, 0.f}}, true}, {}},
            {RallyPointCandidate{3, {{2.f, 0.f, 0.f}}, true}, {}},
        };
        assert(points[0].ownership.acquire(22, is_live));
        const auto r = bind_original_assault_rally_point(cue, origin, points,
            11, 2, is_live, [&](std::size_t n) { return n == 2; });
        assert(r.status == AssaultRallyBindStatus::Bound && r.selected_index == 2);
        assert(points[0].ownership.current_owner() == 22);
        assert(!points[1].ownership.current_owner());
        assert(points[2].ownership.current_owner() == 11);
        assert(cue.configured_mode == 2 && cue.received_cue == 0);

        // A same-owner retry is also unavailable until that lease is released.
        // The remaining candidate is chosen, but resolution now fails.
        const auto r2 = bind_original_assault_rally_point(cue, origin, points,
            11, 3, is_live, [&](std::size_t n) { assert(n == 1); return false; });
        assert(r2.status == AssaultRallyBindStatus::UnresolvedRally && r2.selected_index == 1);
        assert(!points[1].ownership.current_owner()); // port-side rollback
        assert(cue.configured_mode == 0 && cue.received_cue == 0);
        assert(points[2].ownership.release(11, is_live));
    }

    {
        OriginalAssaultCueGate cue{1, 4};
        std::vector<AssaultRallyBindingCandidate> points{
            {RallyPointCandidate{1, {{0.f, 0.f, 0.f}}, true}, {}},
        };
        assert(points[0].ownership.acquire(22, is_live));
        // Generation-stale lease is immediately eligible for a different actor.
        live.erase(22);
        const auto r = bind_original_assault_rally_point(cue, origin, points,
            33, 3, is_live, [](std::size_t) { return true; });
        assert(r.status == AssaultRallyBindStatus::Bound);
        assert(points[0].ownership.current_owner() == 33);
        assert(cue.configured_mode == 3 && cue.received_cue == 0);
        assert(cue.check(false) && cue.received_cue == 3);
    }

    {
        OriginalAssaultCueGate cue{2, 1};
        std::vector<AssaultRallyBindingCandidate> points{
            {RallyPointCandidate{5, {{0.f, 0.f, 0.f}}, true}, {}},
        };
        assert(points[0].ownership.acquire(22, [](Token) { return false; }));
        int checks = 0;
        const auto flickering_liveness = [&](Token) { return ++checks > 1; };
        const auto r = bind_original_assault_rally_point(cue, origin, points,
            11, 3, flickering_liveness, [](std::size_t) { assert(false); return true; });
        assert(r.status == AssaultRallyBindStatus::LockRejected && r.selected_index == 0);
        assert(points[0].ownership.current_owner() == 22);
        assert(cue.configured_mode == 2 && cue.received_cue == 1);
    }

    {
        OriginalAssaultCueGate cue{2, 4};
        std::vector<AssaultRallyBindingCandidate> points{
            {RallyPointCandidate{5, {{0.f, 0.f, 0.f}}, true}, {}},
        };
        const auto r = bind_original_assault_rally_point(cue, origin, points,
            11, 2, is_live, [](std::size_t) { return false; });
        assert(r.status == AssaultRallyBindStatus::UnresolvedRally);
        assert(!points[0].ownership.current_owner());
        assert(cue.configured_mode == 0 && cue.received_cue == 4);
        assert(cue.check(false)); // original received-cue override still wins
    }
}
