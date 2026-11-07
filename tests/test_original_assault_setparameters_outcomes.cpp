#include "sin_compat/original_assault_setparameters_outcomes.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <string_view>
using namespace sin_compat;

static void test_three_original_branches() {
    OriginalAssaultCueGate cue{2, 4};
    assert(!apply_original_setparameters_cue_effects(
        cue, 1, OriginalAssaultParameterOutcome::NoRallyCandidate));
    assert(cue.configured_mode == 2 && cue.received_cue == 4);
    assert(!apply_original_setparameters_cue_effects(
        cue, 3, OriginalAssaultParameterOutcome::UnresolvedRally));
    assert(cue.configured_mode == 0 && cue.received_cue == 4);
    assert(apply_original_setparameters_cue_effects(
        cue, 1, OriginalAssaultParameterOutcome::ResolvedRally));
    assert(cue.configured_mode == 1 && cue.received_cue == 0);
    assert(!cue.check(false));
}

static void test_original_direct_npc_mode_three() {
    OriginalAssaultCueGate cue{1, 1};
    int host_calls = 0;
    const auto result = configure_original_assault_parameters(
        cue, 117, "warehouse_rally_1", 3,
        [&](std::uint64_t token, std::string_view rally, std::int32_t mode) {
            ++host_calls;
            assert(token == 117 && rally == "warehouse_rally_1" && mode == 3);
            return OriginalAssaultParameterOutcome::ResolvedRally;
        });
    assert(result == OriginalAssaultParameterOutcome::ResolvedRally);
    assert(host_calls == 1 && cue.configured_mode == 3 && cue.received_cue == 0);
    assert(cue.check(false) && cue.received_cue == 3);
}

static void test_missing_vs_unresolved_are_distinct() {
    OriginalAssaultCueGate cue{3, 1};
    auto result = configure_original_assault_parameters(
        cue, 118, "missing", 2,
        [](std::uint64_t, std::string_view, std::int32_t) {
            return OriginalAssaultParameterOutcome::NoRallyCandidate;
        });
    assert(result == OriginalAssaultParameterOutcome::NoRallyCandidate);
    assert(cue.configured_mode == 3 && cue.received_cue == 1);
    result = configure_original_assault_parameters(
        cue, 118, "unresolved", 2,
        [](std::uint64_t, std::string_view, std::int32_t) {
            return OriginalAssaultParameterOutcome::UnresolvedRally;
        });
    assert(result == OriginalAssaultParameterOutcome::UnresolvedRally);
    assert(cue.configured_mode == 0 && cue.received_cue == 1);
    assert(!cue.check(false));
}

static void test_invalid_port_outcome_fails_closed() {
    OriginalAssaultCueGate cue{2, 1};
    assert(!apply_original_setparameters_cue_effects(
        cue, 1, static_cast<OriginalAssaultParameterOutcome>(99)));
    assert(cue.configured_mode == 2 && cue.received_cue == 1);
}

int main() {
    test_three_original_branches();
    test_original_direct_npc_mode_three();
    test_missing_vs_unresolved_are_distinct();
    test_invalid_port_outcome_fails_closed();
}
