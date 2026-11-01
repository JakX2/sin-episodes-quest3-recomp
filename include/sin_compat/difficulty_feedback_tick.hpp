#pragma once
// Independently authored state adapter for original SiN 2006 DLL's timed
// update gate (0x222BA420) and its caller (0x222BC0E0). No engine ABI.
#include <cstddef>
namespace sin_compat {
enum class FeedbackTickResult {
    Deferred, SampledNoPublication, Published, InvalidTime, InvalidSchedule
};
class IDifficultyFeedbackTickHost {
public:
    virtual ~IDifficultyFeedbackTickHost() = default;
    // Original 0x222B9E80 iterates SIX category slots. Host owns calculations.
    virtual void sample_category(std::size_t index) = 0;
    // Caller invokes six downstream functions on a true tick.
    virtual void follow_up(std::size_t stage) = 0;
    // Original caller writes zero to counters at +0x94 and +0x98.
    virtual void reset_feedback_counters() = 0;
};
class DifficultyFeedbackTick final {
public:
    explicit DifficultyFeedbackTick(IDifficultyFeedbackTickHost& host) : host_(host) {}
    // Host-provided state, not a recovered engine save/restore format.
    bool configure(float next_due, float interval, int half_interval_ticks,
                   bool pending_publication = false) noexcept;
    // Can be raised during the category collector callback.
    void request_publication() noexcept { pending_publication_ = true; }
    FeedbackTickResult advance_to(float game_time);
    float next_due() const noexcept { return next_due_; }
    float interval() const noexcept { return interval_; }
    int remaining_half_intervals() const noexcept { return remaining_half_intervals_; }
    bool pending_publication() const noexcept { return pending_publication_; }
    bool configured() const noexcept { return configured_; }
private:
    IDifficultyFeedbackTickHost& host_;
    float next_due_ = 0.0f;
    float interval_ = 0.0f;
    float last_time_ = 0.0f;
    int remaining_half_intervals_ = 0;
    bool pending_publication_ = false;
    bool configured_ = false;
};
} // namespace sin_compat
