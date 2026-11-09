#include "sin_compat/original_magnum_impact_effects.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

using namespace sin_compat::combat;

namespace {

class Host final : public IOriginalMagnumImpactHost {
public:
    std::vector<std::string> calls;
    std::vector<OriginalMagnumImpactDecal> decals;
    const void* last_payload = nullptr;
    std::uint32_t last_damage_type = 0;
    std::uint8_t last_trace_flags_42 = 0;

    void observe(const OriginalMagnumImpactInfoView& impact) {
        last_payload = impact.host_payload;
        last_damage_type = impact.damage_type;
        last_trace_flags_42 = impact.trace_flags_42;
    }

    void invoke_base_impact_effect(
        const OriginalMagnumImpactInfoView& impact) override {
        calls.emplace_back("base");
        observe(impact);
    }

    void dispatch_duag_decal(
        OriginalMagnumImpactDecal decal,
        const OriginalMagnumImpactInfoView& impact) override {
        calls.emplace_back(decal == OriginalMagnumImpactDecal::DuagDecal1
                               ? "DUAGDecal1" : "DUAGDecal2");
        decals.push_back(decal);
        observe(impact);
    }
};

void test_base_impact_runs_for_non_duag_ammunition() {
    Host host;
    int payload = 11;
    const auto result = dispatch_original_magnum_impact_effect(
        host, {0x00000002U, 0x00U, &payload});

    assert(result.base_impact_called);
    assert(!result.duag_special_bit_set);
    assert(!result.trace_gate_blocked);
    assert(!result.duag_decal1_dispatched && !result.duag_decal2_dispatched);
    assert(host.calls == std::vector<std::string>{"base"});
    assert(host.last_payload == &payload);
    assert(host.last_damage_type == 0x00000002U);
    assert(host.last_trace_flags_42 == 0x00U);
}

void test_duag_dispatches_both_original_decals_after_base() {
    Host host;
    int payload = 22;
    const auto result = dispatch_original_magnum_impact_effect(
        host, {0x00400002U, 0x00U, &payload});

    assert(result.base_impact_called && result.duag_special_bit_set);
    assert(!result.trace_gate_blocked);
    assert(result.duag_decal1_dispatched && result.duag_decal2_dispatched);
    assert(host.calls == (std::vector<std::string>{
        "base", "DUAGDecal1", "DUAGDecal2"}));
    assert(host.decals.size() == 2);
    assert(host.decals[0] == OriginalMagnumImpactDecal::DuagDecal1);
    assert(host.decals[1] == OriginalMagnumImpactDecal::DuagDecal2);
    assert(host.last_payload == &payload);
}

void test_original_trace_byte_bit_four_suppresses_extra_decals() {
    for (const std::uint8_t flags : {0x04U, 0x05U, 0x84U}) {
        Host host;
        const auto result = dispatch_original_magnum_impact_effect(
            host, {0x00400002U, flags, nullptr});
        assert(result.base_impact_called && result.duag_special_bit_set);
        assert(result.trace_gate_blocked);
        assert(!result.duag_decal1_dispatched && !result.duag_decal2_dispatched);
        assert(host.calls == std::vector<std::string>{"base"});
    }
}

void test_damage_gate_is_a_bit_test_not_full_value_equality() {
    Host host;
    const auto result = dispatch_original_magnum_impact_effect(
        host, {0x80400082U, 0x08U, nullptr});
    assert(result.base_impact_called && result.duag_special_bit_set);
    assert(!result.trace_gate_blocked);
    assert(result.duag_decal1_dispatched && result.duag_decal2_dispatched);
    assert(host.calls.size() == 3);
}

void test_unrelated_trace_bits_do_not_suppress_duag_decals() {
    Host host;
    const auto result = dispatch_original_magnum_impact_effect(
        host, {0x00400002U, 0x08U, nullptr});
    assert(result.duag_special_bit_set);
    assert(!result.trace_gate_blocked);
    assert(result.duag_decal1_dispatched && result.duag_decal2_dispatched);
}

void test_exact_original_decal_resource_names() {
    assert(std::string(original_magnum_impact_decal_name(
               OriginalMagnumImpactDecal::DuagDecal1)) == "DUAGDecal1");
    assert(std::string(original_magnum_impact_decal_name(
               OriginalMagnumImpactDecal::DuagDecal2)) == "DUAGDecal2");
}

} // namespace

int main() {
    test_base_impact_runs_for_non_duag_ammunition();
    test_duag_dispatches_both_original_decals_after_base();
    test_original_trace_byte_bit_four_suppresses_extra_decals();
    test_damage_gate_is_a_bit_test_not_full_value_equality();
    test_unrelated_trace_bits_do_not_suppress_duag_decals();
    test_exact_original_decal_resource_names();
    std::puts("PASS: original SiN Magnum DUAG impact-effect gates and decal order");
}
