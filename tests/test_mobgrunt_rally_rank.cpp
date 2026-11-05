#include "sin_compat/mobgrunt_rally_rank.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>

int main() {
    using sin_compat::RallyPointCandidate;
    using sin_compat::choose_mobgrunt_rally_point;
    const std::array<float, 3> npc{{0.f, 0.f, 0.f}};

    // Original source: higher integer priority wins even if further away.
    const std::vector<RallyPointCandidate> priority{{1, {{1.f, 0.f, 0.f}}, true},
                                                     {2, {{10.f, 0.f, 0.f}}, true}};
    auto chosen = choose_mobgrunt_rally_point(npc, priority);
    assert(chosen && chosen->index == 1);

    // Equal priority: smaller squared three-dimensional distance wins.
    const std::vector<RallyPointCandidate> distance{{1, {{9.f, 0.f, 0.f}}, true},
                                                     {1, {{0.f, 2.f, 3.f}}, true},
                                                     {1, {{0.f, 0.f, 3.f}}, true}};
    chosen = choose_mobgrunt_rally_point(npc, distance);
    assert(chosen && chosen->index == 2 && chosen->distance_squared == 9.0);

    // Equal priority and distance preserve the first candidate.
    const std::vector<RallyPointCandidate> same{{1, {{3.f, 0.f, 0.f}}, true},
                                                 {1, {{0.f, 3.f, 0.f}}, true}};
    chosen = choose_mobgrunt_rally_point(npc, same);
    assert(chosen && chosen->index == 0);

    // Original lock/availability handling unresolved; delegate to host.
    const std::vector<RallyPointCandidate> locked{{100, {{1.f, 0.f, 0.f}}, false},
                                                   {1, {{2.f, 0.f, 0.f}}, true}};
    chosen = choose_mobgrunt_rally_point(npc, locked);
    assert(chosen && chosen->index == 1);
    assert(!choose_mobgrunt_rally_point(npc, {}).has_value());

    // Explicitly avoid undefined original x87 NaN and negative priority cases.
    const float nan = std::numeric_limits<float>::quiet_NaN();
    assert(!choose_mobgrunt_rally_point({{nan, 0.f, 0.f}}, priority).has_value());
    const std::vector<RallyPointCandidate> invalid{{-1, {{0.f, 0.f, 0.f}}, true},
                                                    {5, {{nan, 0.f, 0.f}}, true}};
    assert(!choose_mobgrunt_rally_point(npc, invalid).has_value());
}
