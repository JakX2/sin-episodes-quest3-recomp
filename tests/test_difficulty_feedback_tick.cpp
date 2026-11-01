#include "sin_compat/difficulty_feedback_tick.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <limits>
#include <string>
#include <vector>
using namespace sin_compat;
struct Host final : IDifficultyFeedbackTickHost {
    DifficultyFeedbackTick* runtime = nullptr;
    std::vector<std::string> events;
    bool raise = false;
    void sample_category(std::size_t i) override {
        events.push_back("sample"+std::to_string(i));
        if (raise && i==3) runtime->request_publication();
    }
    void follow_up(std::size_t i) override {
        events.push_back("follow"+std::to_string(i));
    }
    void reset_feedback_counters() override {events.push_back("reset");}
    void clear() {events.clear();}
};
int main() {
    Host host;
    DifficultyFeedbackTick t(host); host.runtime=&t;
    assert(t.advance_to(0)==FeedbackTickResult::InvalidSchedule);
    assert(!t.configure(NAN,10,0));
    assert(!t.configure(0,0,0));
    assert(!t.configure(0,10,-1));
    assert(t.configure(5,8,2));
    assert(t.advance_to(4.999f)==FeedbackTickResult::Deferred);
    assert(host.events.empty());
    assert(t.advance_to(5)==FeedbackTickResult::SampledNoPublication);
    assert(t.next_due()==9 && t.remaining_half_intervals()==1);
    assert(host.events.size()==6);
    for(int i=0;i<6;++i)assert(host.events[i]=="sample"+std::to_string(i));
    host.clear();
    assert(t.advance_to(8.999f)==FeedbackTickResult::Deferred);
    host.raise=true;
    assert(t.advance_to(9)==FeedbackTickResult::Published);
    assert(t.next_due()==13 && t.remaining_half_intervals()==0);
    assert(!t.pending_publication());
    assert(host.events.size()==13);
    assert(host.events[6]=="follow0" && host.events[7]=="reset");
    for(int i=1;i<6;++i)assert(host.events[7+i]=="follow"+std::to_string(i));
    host.clear();host.raise=false;
    assert(t.advance_to(13)==FeedbackTickResult::SampledNoPublication);
    assert(t.next_due()==21 && host.events.size()==6);
    assert(t.advance_to(13)==FeedbackTickResult::Deferred);
    assert(t.advance_to(12)==FeedbackTickResult::InvalidTime);
    assert(t.advance_to(INFINITY)==FeedbackTickResult::InvalidTime);
    assert(t.next_due()==21);
    t.request_publication();
    assert(t.pending_publication());
    assert(t.advance_to(21)==FeedbackTickResult::Published);
    assert(!t.pending_publication() && t.next_due()==29);
    assert(t.configure(0,2,0,true));
    host.clear();
    assert(t.advance_to(0)==FeedbackTickResult::Published);
    assert(host.events.size()==13);
    assert(t.configure(1,std::numeric_limits<float>::max(),0));
    host.clear();
    assert(t.advance_to(std::numeric_limits<float>::max())==
           FeedbackTickResult::InvalidSchedule);
    assert(host.events.empty() && t.next_due()==1);
    assert(t.configure(1,2,0));
    assert(!t.configure(0,2,-5));
    assert(!t.configured());
}
