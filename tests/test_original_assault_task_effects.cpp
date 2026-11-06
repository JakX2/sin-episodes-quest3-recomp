#include "sin_compat/original_assault_task_effects.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <limits>
#include <optional>
#include <vector>

int main() {
    using namespace sin_compat;
    OriginalAssaultTaskEffectState state;
    std::vector<OriginalAssaultArrivalOutput> outputs;
    auto fire_output = [&](OriginalAssaultArrivalOutput output) {
        outputs.push_back(output);
    };
    assert(!state.hit_rally_point);
    assert(!state.hit_assault_point);
    assert(!state.defer_schedule_selection_until);

    // Missing game entity must not accidentally fire an OnArrival output.
    assert(!state.mark_rally_arrived_if_resolved(false, fire_output));
    assert(!state.mark_assault_arrived_if_resolved(false, fire_output));
    assert(outputs.empty());
    assert(!state.hit_rally_point && !state.hit_assault_point);

    // Original native x86: set flag then emit the specific point's OnArrival.
    assert(state.mark_rally_arrived_if_resolved(true, fire_output));
    assert(state.hit_rally_point && !state.hit_assault_point);
    assert(outputs.size() == 1);
    assert(outputs.back() == OriginalAssaultArrivalOutput::RallyOnArrival);
    assert(state.mark_assault_arrived_if_resolved(true, fire_output));
    assert(state.hit_assault_point);
    assert(outputs.size() == 2);
    assert(outputs.back() == OriginalAssaultArrivalOutput::AssaultOnArrival);

    // Do not invent a native one-shot suppression rule not in the evidence.
    assert(state.mark_rally_arrived_if_resolved(true, fire_output));
    assert(outputs.size() == 3);

    // Configured delay and point timeout are separate source fields.
    auto delay = state.rally_delay_for_host(true, 0.75f);
    auto timeout = state.assault_timeout_for_host(true, 3.5f);
    assert(delay && *delay == 0.75f);
    assert(timeout && *timeout == 3.5f);
    assert(!state.rally_delay_for_host(false, 0.75f));
    assert(!state.assault_timeout_for_host(false, 3.5f));

    // Config values are forwarded, not overwritten with a generic 4.0
    // diversion threshold or a 3.0 FGD default.
    assert(state.rally_delay_for_host(true, 0.0f) == 0.0f);
    assert(state.assault_timeout_for_host(true, -1.0f) == -1.0f);

    // No engine timer is secretly started by returning a configured float.
    assert(!state.defer_schedule_selection_until);
    assert(state.defer_schedule_selection(10.0f, 1.5f));
    assert(state.defer_schedule_selection_until == 11.5f);
    assert(state.defer_schedule_selection(11.5f, 0.0f));
    assert(state.defer_schedule_selection_until == 11.5f);

    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    assert(!state.rally_delay_for_host(true, nan));
    assert(!state.assault_timeout_for_host(true, inf));
    assert(!state.defer_schedule_selection(nan, 1.0f));
    assert(!state.defer_schedule_selection(1.0f, inf));
    assert(state.defer_schedule_selection_until == 11.5f);
    return 0;
}
