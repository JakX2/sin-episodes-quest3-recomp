#include "sin_compat/difficulty_publication_deadline.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <limits>
#include <string>
#include <vector>
using namespace sin_compat;
struct Host final : IDifficultyFeedbackTickHost, IFeedbackPublicationGateHost {
    bool enabled=true;
    bool raise_during_sample=false;
    std::vector<std::string> events;
    DifficultyFeedbackTick* tick=nullptr;
    bool feedback_updates_enabled() const override {return enabled;}
    void sample_category(std::size_t index) override {
        events.push_back("s"+std::to_string(index));
        if (index==5 && raise_during_sample) tick->request_publication();
    }
    void follow_up(std::size_t index) override {events.push_back("f"+std::to_string(index));}
    void reset_feedback_counters() override {events.push_back("reset");}
};
int main(){
    Host host;
    DifficultyFeedbackTick feedback(host);
    host.tick=&feedback;
    DifficultyPublicationDeadline report(feedback,host);
    assert(report.advance_to(1).status==PublicationDeadlineStatus::InvalidSchedule);
    assert(!report.configure(true,NAN,4));
    assert(!report.configure(true,3,-1));
    assert(!report.configure(true,3,0));
    assert(feedback.configure(5.0f,4.0f,0));
    assert(report.configure(true,3.0f,8.0f));
    auto event=report.advance_to(2.0f);
    assert(event.status==PublicationDeadlineStatus::TickAttempted);
    assert(event.tick_attempted && !event.flag_raised);
    assert(event.tick_result==FeedbackTickResult::Deferred);
    assert(report.next_deadline()==3.0f && !feedback.pending_publication());
    event=report.advance_to(3.0f);
    assert(event.status==PublicationDeadlineStatus::DeadlineRaisedAndTicked);
    assert(event.flag_raised && event.tick_attempted);
    assert(event.tick_result==FeedbackTickResult::Deferred);
    assert(report.next_deadline()==11.0f && feedback.pending_publication());
    assert(host.events.empty());
    event=report.advance_to(5.0f);
    assert(event.status==PublicationDeadlineStatus::TickAttempted);
    assert(!event.flag_raised && event.tick_result==FeedbackTickResult::Published);
    assert(!feedback.pending_publication());
    assert(host.events.size()==13);
    assert(host.events[0]=="s0" && host.events[6]=="f0");
    assert(host.events[7]=="reset" && host.events[12]=="f5");
    host.events.clear();
    event=report.advance_to(11.0f);
    assert(event.flag_raised && event.tick_result==FeedbackTickResult::Published);
    assert(report.next_deadline()==19.0f);
    assert(host.events.size()==13);
    assert(!feedback.pending_publication());
    host.events.clear();
    host.enabled=false;
    event=report.advance_to(19.0f);
    assert(event.status==PublicationDeadlineStatus::DeadlineRaised);
    assert(event.flag_raised && !event.tick_attempted);
    assert(report.next_deadline()==27.0f && feedback.pending_publication());
    assert(host.events.empty());
    host.enabled=true;
    event=report.advance_to(20.0f);
    assert(event.status==PublicationDeadlineStatus::TickAttempted);
    assert(!event.flag_raised && event.tick_result==FeedbackTickResult::Published);
    assert(!feedback.pending_publication());
    assert(host.events.size()==13);
    report.set_active(false);
    event=report.advance_to(21.0f);
    assert(event.status==PublicationDeadlineStatus::Inactive && !event.tick_attempted);
    assert(event.status!=PublicationDeadlineStatus::DeadlineRaised);
    assert(report.next_deadline()==27.0f);
    assert(report.advance_to(20.0f).status==PublicationDeadlineStatus::InvalidClock);
    assert(report.advance_to(INFINITY).status==PublicationDeadlineStatus::InvalidClock);
    assert(report.configure(true,0,std::numeric_limits<float>::max()));
    assert(report.advance_to(std::numeric_limits<float>::max()).status==
           PublicationDeadlineStatus::InvalidSchedule);
    assert(report.next_deadline()==0.0f);
    // An unconfigured downstream cannot silently claim publication success.
    DifficultyFeedbackTick broken_feedback(host);
    DifficultyPublicationDeadline broken(broken_feedback,host);
    assert(broken.configure(true,1,1));
    event=broken.advance_to(1);
    assert(event.status==PublicationDeadlineStatus::DownstreamRejected);
    assert(event.flag_raised && event.tick_attempted);
    assert(event.tick_result==FeedbackTickResult::InvalidSchedule);
}
