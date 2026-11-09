#include "sin_compat/original_float_short_term_stat.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>

using namespace sin_compat::difficulty;
namespace {
void test_original_descending_ring() {
    OriginalFloatShortTermStat source(4U);
    assert(source.window_size()==4U && source.descending_cursor()==3U);
    assert(source.read_slot2()==0.0f && !source.dirty_slot11());
    for (int i=1;i<=4;++i) assert(source.append_slot9(float(i)));
    assert(source.descending_cursor()==3U);
    assert((source.physical_ring()==std::vector<float>{4.0f,3.0f,2.0f,1.0f}));
    assert(source.read_slot2()==2.5f);
    assert(source.append_slot9(5.0f));
    assert((source.physical_ring()==std::vector<float>{4.0f,3.0f,2.0f,5.0f}));
    assert(source.read_slot2()==3.5f);
    assert(source.dirty_slot11());
    assert(source.consume_dirty_slot12() && !source.consume_dirty_slot12());
}
void test_original_fill_and_resize() {
    OriginalFloatShortTermStat source(4U);
    assert(source.append_slot9(8.0f));
    assert(source.read_slot2()==2.0f);
    assert(source.consume_dirty_slot12());
    const auto saved_cursor=source.descending_cursor();
    source.write_slot3(3.0f);
    assert(source.descending_cursor()==saved_cursor);
    assert((source.physical_ring()==std::vector<float>{3.0f,3.0f,3.0f,3.0f}));
    assert(source.read_slot2()==3.0f && !source.dirty_slot11());
    source.append_slot9(5.0f);
    assert(source.read_slot2()==3.5f);
    source.resize_slot4(3U);
    assert(source.window_size()==3U && source.descending_cursor()==2U);
    assert((source.physical_ring()==std::vector<float>{3.5f,3.5f,3.5f}));
    assert(source.dirty_slot11()); // original resize does not clear flag
    source.consume_dirty_slot12();
    source.resize_slot4(3U);
    assert(!source.dirty_slot11());
    source.append_slot9(6.5f);
    assert((source.physical_ring()==std::vector<float>{3.5f,3.5f,6.5f}));
    assert(source.read_slot2()==4.5f);
}
void test_empty_history_is_safe_in_portable_model() {
    OriginalFloatShortTermStat source(0U);
    assert(source.window_size()==0U);
    assert(std::isnan(source.read_slot2()));
    assert(!source.append_slot9(9.0f));
    source.write_slot3(4.0f);
    assert(!source.dirty_slot11());
    source.resize_slot4(2U);
    assert(source.window_size()==2U);
    assert(std::isnan(source.read_slot2()));
    source.write_slot3(0.0f);
    assert(source.read_slot2()==0.0f);
}
void test_integrated_short_term_gate_and_advisor() {
    OriginalFloatShortTermStat a(4U), b(4U);
    OriginalShortTermStatGate ordinary(a,10.0f,1.0f);
    OriginalShortTermStatGateConjugate inverse(b,10.0f,1.0f);
    assert(ordinary.read_current_value()==0.0f);
    assert(inverse.read_current_value()==10.0f);
    for (int i=0; i<4; ++i) {
        assert(a.append_slot9(0.2f));
        assert(b.append_slot9(0.6f));
    }
    OriginalStatAdvisorFields advisor{};
    advisor.values={{2.0f,4.0f,6.0f,8.0f}};
    std::vector<IOriginalAdvisorListener*> listeners{&ordinary,&inverse};
    auto result=submit_original_advisor_sample(advisor,true,7.0f,listeners);
    assert(result.accepted);
    assert(std::fabs(ordinary.read_current_value()-2.0f)<0.000001f);
    assert(std::fabs(inverse.read_current_value()-4.0f)<0.000001f);
    assert(std::fabs(advisor.score-3.0f)<0.000001f);
    // Gate slot2 forwarding fills the entire source history, not append.
    ordinary.forward_slot2(5.0f);
    assert(a.read_slot2()==0.5f);
    assert((a.physical_ring()==std::vector<float>{0.5f,0.5f,0.5f,0.5f}));
    assert(ordinary.read_current_value()==5.0f);
    inverse.forward_slot2(7.0f);
    assert(b.read_slot2()==0.7f);
    result=submit_original_advisor_sample(advisor,true,8.0f,listeners);
    assert(result.accepted);
    assert(std::fabs(ordinary.read_current_value()-5.0f)<0.000001f);
    assert(std::fabs(inverse.read_current_value()-3.0f)<0.000002f);
    assert(std::fabs(advisor.score-4.0f)<0.000002f);
}
}
int main() {
    test_original_descending_ring();
    test_original_fill_and_resize();
    test_empty_history_is_safe_in_portable_model();
    test_integrated_short_term_gate_and_advisor();
}
