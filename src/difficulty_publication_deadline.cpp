#include "sin_compat/difficulty_publication_deadline.hpp"
#include <cmath>
#include <limits>
namespace sin_compat {
bool DifficultyPublicationDeadline::configure(bool active,float due,float period) noexcept {
    configured_=false;
    if (!std::isfinite(due) || due<0.0f ||
        !std::isfinite(period) || period<=0.0f) return false;
    active_=active;
    deadline_=due;
    period_=period;
    previous_time_=0.0f;
    configured_=true;
    return true;
}
PublicationDeadlineResult DifficultyPublicationDeadline::advance_to(float now) {
    if (!configured_) return {PublicationDeadlineStatus::InvalidSchedule,false,false,{}};
    if (!std::isfinite(now) || now<previous_time_ || now<0.0f)
        return {PublicationDeadlineStatus::InvalidClock,false,false,{}};
    previous_time_=now;
    if (!active_) return {PublicationDeadlineStatus::Inactive,false,false,{}};
    bool raised=false;
    if (deadline_<=now) {
        const double future=static_cast<double>(now)+static_cast<double>(period_);
        if (!std::isfinite(future) || future>std::numeric_limits<float>::max())
            return {PublicationDeadlineStatus::InvalidSchedule,false,false,{}};
        deadline_=static_cast<float>(future);
        // Original 0x222BC475 writes this+0x05 = 1 BEFORE checking
        // the engine setting at global-pointer 0x225923A8.
        tick_.request_publication();
        raised=true;
    }
    if (!host_.feedback_updates_enabled())
        return {raised ? PublicationDeadlineStatus::DeadlineRaised
                       : PublicationDeadlineStatus::NotYetDue,
                raised,false,{}};
    // Original at 0x222BC48E invokes 0x222BC0E0 whether or not the
    // report deadline was reached, when active and setting enabled.
    const auto tick_result=tick_.advance_to(now);
    if (tick_result==FeedbackTickResult::InvalidSchedule ||
        tick_result==FeedbackTickResult::InvalidTime)
        return {PublicationDeadlineStatus::DownstreamRejected,raised,true,tick_result};
    return {raised ? PublicationDeadlineStatus::DeadlineRaisedAndTicked
                   : PublicationDeadlineStatus::TickAttempted,
            raised,true,tick_result};
}
} // namespace sin_compat
