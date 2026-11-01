#pragma once
// Independent host-neutral subset of the 2006 SiN difficulty update routine
// near 0x222BC3E3..0x222BC493. No commercial Source engine ABI.
#include "sin_compat/difficulty_feedback_tick.hpp"
namespace sin_compat {
class IFeedbackPublicationGateHost {
public:
    virtual ~IFeedbackPublicationGateHost() = default;
    // Original global engine setting via 0x225923A8 / object +0x2C.
    virtual bool feedback_updates_enabled() const = 0;
};
enum class PublicationDeadlineStatus {
    Inactive, NotYetDue, DeadlineRaised, DeadlineRaisedAndTicked,
    TickAttempted, InvalidClock, InvalidSchedule, DownstreamRejected
};
struct PublicationDeadlineResult {
    PublicationDeadlineStatus status = PublicationDeadlineStatus::InvalidSchedule;
    bool flag_raised = false;
    bool tick_attempted = false;
    FeedbackTickResult tick_result = FeedbackTickResult::InvalidSchedule;
};
class DifficultyPublicationDeadline final {
public:
    DifficultyPublicationDeadline(DifficultyFeedbackTick& tick,
                                  const IFeedbackPublicationGateHost& host)
        : tick_(tick), host_(host) {}
    bool configure(bool active, float deadline, float period) noexcept;
    void set_active(bool active) noexcept { active_=active; }
    PublicationDeadlineResult advance_to(float game_time);
    float next_deadline() const noexcept { return deadline_; }
    bool active() const noexcept {return active_;}
private:
    DifficultyFeedbackTick& tick_;
    const IFeedbackPublicationGateHost& host_;
    float deadline_ = 0.0f;
    float period_ = 0.0f;
    float previous_time_ = 0.0f;
    bool active_ = false;
    bool configured_ = false;
};
} // namespace sin_compat
