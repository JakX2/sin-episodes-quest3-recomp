#include "sin_compat/original_stat_advisor_evaluators.hpp"
#include "sin_compat/difficulty_advisors_contract.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <limits>
#include <vector>

using namespace sin_compat::difficulty;
namespace {

void test_slot9() {
    for (int m1=-2;m1<=4;++m1)
        for (int m2=-2;m2<=4;++m2)
            for (int score=-2;score<=4;++score)
                for (int bound=-2;bound<=4;++bound) {
                    OriginalStatAdvisorFields a{};
                    a.score=static_cast<float>(score);
                    a.values={{0.0f,static_cast<float>(m1),
                               static_cast<float>(m2),static_cast<float>(bound)}};
                    const int expected = m2<m1
                        ? (score<bound ? 4:3) : (score>bound ? 4:3);
                    assert(evaluate_stat_advisor_slot9(a));
                    assert(a.code==expected);
                }
}

void test_slot10() {
    for (int m1=-2;m1<=4;++m1)
        for (int m2=-2;m2<=4;++m2)
            for (int score=-2;score<=4;++score)
                for (int bound=-2;bound<=4;++bound) {
                    OriginalStatAdvisorFields a{};
                    a.score=static_cast<float>(score);
                    a.values={{static_cast<float>(bound),static_cast<float>(m1),
                               static_cast<float>(m2),0.0f}};
                    const int expected = m1>m2
                        ? (score>bound ? 0:1) : (score<bound ? 0:1);
                    assert(evaluate_stat_advisor_slot10(a));
                    assert(a.code==expected);
                }
}

void test_unordered_and_equal() {
    const float nan=std::numeric_limits<float>::quiet_NaN();
    OriginalStatAdvisorFields a{};
    a.values={{1.0f,10.0f,2.0f,10.0f}};
    a.score=10.0f;
    assert(evaluate_stat_advisor_slot9(a) && a.code==3);
    a.score=nan;
    assert(evaluate_stat_advisor_slot9(a) && a.code==3);
    a.score=11.0f;
    a.values[2]=nan;
    assert(evaluate_stat_advisor_slot9(a) && a.code==4);
    a.values[2]=10.0f;
    a.score=10.0f;
    assert(evaluate_stat_advisor_slot10(a) && a.code==1);
    a.score=nan;
    assert(evaluate_stat_advisor_slot10(a) && a.code==1);
    a.values[1]=nan;
    a.score=0.0f;
    assert(evaluate_stat_advisor_slot10(a) && a.code==0);
    a.code=-9;
    assert(!evaluate_stat_advisor_slot11(a) && a.code==2);
}

void test_gate() {
    OriginalStatAdvisorFields a{};
    a.gate_value=10.0f;
    a.gate_step=0.75f;
    assert(!advance_stat_advisor_gate(a,9.0f));
    assert(a.gate_value==10.0f);
    assert(advance_stat_advisor_gate(a,10.0f));
    assert(a.gate_value==10.75f);
    assert(advance_stat_advisor_gate(a,11.0f));
    assert(a.gate_value==11.75f);
    assert(advance_stat_advisor_gate(a,std::numeric_limits<float>::quiet_NaN()));
}

struct Datum { char kind; float f; std::int16_t s; };
struct MockWire final : TypedWriter, TypedReader {
    std::vector<Datum> data;
    std::size_t pos=0;
    bool write_f32(float v) override {data.push_back({'f',v,0});return true;}
    bool write_i16(std::int16_t v) override {data.push_back({'s',0.0f,v});return true;}
    bool read_f32(float& v) override {
        if(pos>=data.size()||data[pos].kind!='f')return false;
        v=data[pos++].f;return true;
    }
    bool read_i16(std::int16_t& v) override {
        if(pos>=data.size()||data[pos].kind!='s')return false;
        v=data[pos++].s;return true;
    }
};
void test_phase76_client_wire_integration() {
    OriginalStatAdvisorFields a{};
    a.score=4.0f;
    a.values={{5.0f,1.0f,2.0f,3.0f}};
    assert(evaluate_stat_advisor_slot10(a) && a.code==0);
    AdvisorScores packet{};
    packet.entries[0]={a.score,static_cast<std::int16_t>(a.code)};
    for(std::size_t i=1;i<advisor_count;++i) packet.entries[i]={0.0f,2};
    MockWire wire;
    assert(write_advisor_scores(wire,packet));
    AdvisorScores copy{};
    assert(read_advisor_scores(wire,copy));
    ClientAdvisorPresentation client{};
    apply_advisor_scores(client,copy);
    assert(client.codes[0]==0 && client.scores[0]==4.0f);
    assert(wire.pos==12U);
}
}

int main(){
    test_slot9();
    test_slot10();
    test_unordered_and_equal();
    test_gate();
    test_phase76_client_wire_integration();
    return 0;
}
