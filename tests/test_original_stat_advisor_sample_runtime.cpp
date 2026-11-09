#include "sin_compat/original_stat_advisor_sample_runtime.hpp"
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
struct Listener : IOriginalAdvisorListener {
    std::vector<std::string>& log;
    const char* id;
    float value;
    Listener(std::vector<std::string>& log, const char* id, float value)
      : log(log), id(id), value(value) {}
    void receive_sample(float raw) override {
        log.push_back(std::string("sample:")+id);
        assert(raw == 5.0f);
    }
    float read_current_value() override {
        log.push_back(std::string("read:")+id);
        return value;
    }
};
OriginalStatAdvisorFields advisor(float score,float a,float b) {
    OriginalStatAdvisorFields s{};
    s.score=score;
    s.values={{3.0f,a,b,6.0f}};
    return s;
}
void test_route_matrix() {
    const float nan=std::numeric_limits<float>::quiet_NaN();
    const float inf=std::numeric_limits<float>::infinity();
    const float samples[] = {-inf,-5.0f,-1.0f,0.0f,1.0f,5.0f,inf,nan};
    for (float score:samples) for (float a:samples) for(float b:samples) {
        auto s=advisor(score,a,b);
        OriginalAdvisorRoute expected=OriginalAdvisorRoute::Slot11;
        if(a>b) {
            if(score>a) expected=OriginalAdvisorRoute::Slot10;
            else if(score<b) expected=OriginalAdvisorRoute::Slot9;
        } else {
            if(score<a) expected=OriginalAdvisorRoute::Slot10;
            else if(score>b) expected=OriginalAdvisorRoute::Slot9;
        }
        const auto r=classify_original_advisor(s,true);
        assert(r.route==expected);
        assert(s.code >= 0 && s.code<=4);
        assert(r.virtual_return==(expected!=OriginalAdvisorRoute::Slot11));
        if(expected==OriginalAdvisorRoute::Slot11) assert(s.code==2);
        auto disabled=advisor(score,a,b);
        const auto rd=classify_original_advisor(disabled,false);
        assert(rd.route==OriginalAdvisorRoute::Slot11 && disabled.code==2 && !rd.virtual_return);
    }
    auto descending=advisor(9.0f,8.0f,2.0f);
    auto right=classify_original_advisor(descending,true);
    assert(right.route==OriginalAdvisorRoute::Slot10 && right.original_argument==1.0f);
    auto ascending=advisor(9.0f,2.0f,8.0f);
    right=classify_original_advisor(ascending,true);
    assert(right.route==OriginalAdvisorRoute::Slot9 && right.original_argument==1.0f);
    auto neutral=advisor(5.0f,2.0f,8.0f);
    right=classify_original_advisor(neutral,true);
    assert(right.route==OriginalAdvisorRoute::Slot11 && right.original_argument==5.0f);
}
void test_sample_pipeline_order_average_and_gating() {
    std::vector<std::string> log;
    Listener a(log,"a",4.0f), b(log,"b",6.0f), c(log,"c",8.0f);
    std::vector<IOriginalAdvisorListener*> ls{&a,&b,&c};
    auto s=advisor(0.0f,2.0f,5.0f);
    s.gate_value=4.0f;
    s.gate_step=0.25f;
    auto r=submit_original_advisor_sample(s,true,5.0f,ls);
    assert(r.accepted && r.dispatch.route==OriginalAdvisorRoute::Slot9);
    assert(r.dispatch.virtual_return);
    assert(s.score==6.0f && s.gate_value==5.25f);
    assert((log==std::vector<std::string>{"sample:a","sample:b","sample:c","read:a","read:b","read:c"}));
    log.clear();
    r=submit_original_advisor_sample(s,true,5.0f,ls);
    assert(!r.accepted && log.empty() && s.gate_value==5.25f && s.score==6.0f);
}
void test_no_listeners_and_unordered() {
    auto s=advisor(1.0f,2.0f,4.0f);
    s.gate_step=1.0f;
    const auto r=submit_original_advisor_sample(s,true,2.0f,{});
    assert(r.accepted && !r.dispatch.virtual_return);
    assert(r.dispatch.route==OriginalAdvisorRoute::Slot11 && s.code==2);
    assert(std::isnan(s.score) && s.gate_value==3.0f);
    s.gate_value=std::numeric_limits<float>::quiet_NaN();
    const auto again=submit_original_advisor_sample(s,true,2.0f,{});
    assert(again.accepted && s.gate_value==3.0f);
}
}
int main() {
    test_route_matrix();
    test_sample_pipeline_order_average_and_gating();
    test_no_listeners_and_unordered();
}
