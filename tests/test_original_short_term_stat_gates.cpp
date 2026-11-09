#include "sin_compat/original_short_term_stat_gates.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

using namespace sin_compat::difficulty;
namespace {
struct Stat final : IOriginalShortTermStatSource {
    float value = 0.0f;
    int read_count = 0;
    std::vector<float> forwarded;
    std::vector<std::string>* trace = nullptr;
    explicit Stat(float initial, std::vector<std::string>* events = nullptr)
        : value(initial), trace(events) {}
    float read_slot2() override {
        ++read_count;
        if (trace) trace->push_back("source.read");
        return value;
    }
    void write_slot3(float normalized) override {
        forwarded.push_back(normalized);
        if (trace) trace->push_back("source.write");
    }
};
void test_ctor_clamp_gain_and_conjugate() {
    Stat a(0.8f);
    OriginalShortTermStatGate gate(a,10.0f,0.5f);
    assert(a.read_count == 1 && gate.read_current_value() == 4.0f);
    a.value = 3.0f;
    gate.receive_sample(-777.0f); // original slot3 ignores argument
    assert(gate.read_current_value() == 10.0f);
    a.value = -3.0f;
    gate.receive_sample(999.0f);
    assert(gate.read_current_value() == 0.0f);

    Stat b(0.8f);
    OriginalShortTermStatGateConjugate conjugate(b,10.0f,0.5f);
    assert(b.read_count == 2); // original derived ctor refreshes twice
    assert(conjugate.read_current_value() == 6.0f);
    b.value = 3.0f;
    conjugate.receive_sample(1.0f);
    assert(conjugate.read_current_value() == 0.0f);
    b.value = -3.0f;
    conjugate.receive_sample(1.0f);
    assert(conjugate.read_current_value() == 10.0f);
}
void test_infinities_and_unordered_inputs() {
    Stat a(std::numeric_limits<float>::infinity());
    OriginalShortTermStatGate gate(a, 7.0f, 1.0f);
    assert(gate.read_current_value() == 7.0f);
    a.value = -std::numeric_limits<float>::infinity();
    gate.receive_sample(0.0f);
    assert(gate.read_current_value() == 0.0f);
    a.value = std::numeric_limits<float>::quiet_NaN();
    gate.receive_sample(0.0f);
    assert(std::isnan(gate.read_current_value()));

    Stat b(std::numeric_limits<float>::quiet_NaN());
    OriginalShortTermStatGateConjugate conjugate(b, 7.0f, 1.0f);
    assert(std::isnan(conjugate.read_current_value()));
    b.value = std::numeric_limits<float>::infinity();
    conjugate.receive_sample(0.0f);
    assert(conjugate.read_current_value() == 0.0f);
}
void test_shared_slot2_forwards_normalized_without_clamp() {
    Stat stat(0.8f);
    OriginalShortTermStatGate normal(stat,10.0f,0.5f);
    normal.forward_slot2(15.0f);
    assert(normal.read_current_value() == 15.0f); // raw sample cached
    assert(stat.forwarded.size()==1 && stat.forwarded.back() == 3.0f);
    stat.value = 0.3f;
    normal.receive_sample(15.0f);
    assert(normal.read_current_value() == 1.5f);

    Stat other(0.8f);
    OriginalShortTermStatGateConjugate inverse(other,10.0f,0.5f);
    inverse.forward_slot2(15.0f);
    assert(inverse.read_current_value() == 15.0f);
    assert(other.forwarded.size() == 1 && other.forwarded.back()==3.0f);
    other.value = 0.3f;
    inverse.receive_sample(15.0f);
    assert(inverse.read_current_value() == 8.5f);
}
void test_real_phase149_advisor_listener_integration() {
    std::vector<std::string> events;
    Stat source_a(0.4f,&events), source_b(0.6f,&events);
    OriginalShortTermStatGate normal(source_a,10.0f,1.0f);
    OriginalShortTermStatGateConjugate inverse(source_b,10.0f,1.0f);
    // Constructors already queried sources. Reset trace before sample.
    events.clear();
    OriginalStatAdvisorFields advisor{};
    advisor.gate_step = 1.0f;
    advisor.values={{2.0f,4.0f,6.0f,8.0f}};
    std::vector<IOriginalAdvisorListener*> listeners{&normal,&inverse};
    auto result=submit_original_advisor_sample(advisor,true,5.0f,listeners);
    assert(result.accepted);
    assert(events.size()==2 && events[0]=="source.read" && events[1]=="source.read");
    assert(normal.read_current_value()==4.0f);
    assert(inverse.read_current_value()==4.0f);
    assert(advisor.score==4.0f);
    assert(advisor.gate_value==6.0f);
    events.clear();
    result=submit_original_advisor_sample(advisor,true,5.0f,listeners);
    assert(!result.accepted && events.empty());
}
void test_signed_values_scale_and_zero_gain() {
    Stat stat(0.1f);
    OriginalShortTermStatGate gate(stat,-2.0f,2.0f);
    assert(gate.read_current_value()==-0.4f);
    gate.forward_slot2(1.0f);
    assert(stat.forwarded.back()==-0.25f);
    // The original x87 code has no division-by-zero guard.
    Stat b(1.0f);
    OriginalShortTermStatGate zero(b,0.0f,0.0f);
    zero.forward_slot2(1.0f);
    assert(std::isinf(b.forwarded.back()));
}
}
int main() {
    test_ctor_clamp_gain_and_conjugate();
    test_infinities_and_unordered_inputs();
    test_shared_slot2_forwards_normalized_without_clamp();
    test_real_phase149_advisor_listener_integration();
    test_signed_values_scale_and_zero_gain();
    return 0;
}
