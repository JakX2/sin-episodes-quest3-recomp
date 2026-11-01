#include "sin_compat/difficulty_feedback_tick.hpp"
#include <cmath>
#include <limits>
namespace sin_compat {
bool DifficultyFeedbackTick::configure(float next_due, float interval,
                                        int accelerated, bool pending) noexcept {
    configured_=false;
    if (!std::isfinite(next_due) || next_due<0.0f ||
        !std::isfinite(interval) || interval<=0.0f || accelerated<0)
        return false;
    next_due_=next_due;
    interval_=interval;
    last_time_=0.0f;
    remaining_half_intervals_=accelerated;
    pending_publication_=pending;
    configured_=true;
    return true;
}
FeedbackTickResult DifficultyFeedbackTick::advance_to(float now) {
    if (!configured_) return FeedbackTickResult::InvalidSchedule;
    if (!std::isfinite(now) || now < 0.0f || now < last_time_)
        return FeedbackTickResult::InvalidTime;
    last_time_=now;
    // Original x87 fcomp+test 0x41+jp ignores next_due > now;
    // equality triggers the category collector.
    if (next_due_ > now) return FeedbackTickResult::Deferred;
    const float factor = remaining_half_intervals_ > 0 ? 0.5f : 1.0f;
    const double next = static_cast<double>(now) +
                        static_cast<double>(interval_) * factor;
    if (!std::isfinite(next) || next > std::numeric_limits<float>::max())
        return FeedbackTickResult::InvalidSchedule;
    // Decrement before scheduling. These operations are modeled separately
    // from the original engine's object and callback implementations.
    if (remaining_half_intervals_ > 0) --remaining_half_intervals_;
    next_due_=static_cast<float>(next);
    for (std::size_t i=0;i<6;++i) host_.sample_category(i);
    // The +0x05 one-shot flag is tested AFTER sampling.
    if (!pending_publication_) return FeedbackTickResult::SampledNoPublication;
    pending_publication_=false;
    host_.follow_up(0);
    host_.reset_feedback_counters();
    for (std::size_t i=1;i<6;++i) host_.follow_up(i);
    return FeedbackTickResult::Published;
}
} // namespace sin_compat
