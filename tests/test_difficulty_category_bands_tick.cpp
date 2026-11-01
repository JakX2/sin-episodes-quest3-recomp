#include "sin_compat/difficulty_feedback_tick.hpp"
#include "sin_compat/difficulty_category_bands.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
using namespace sin_compat;
// Explicitly SYNTHETIC performance deltas: original magnitude/sign decision
// in 0x222BAE60 and 0x222C4BF0 is NOT established by this test.
struct Host final : IDifficultyFeedbackTickHost {
    DifficultyCategoryBands& factors;
    unsigned samples=0, followups=0;
    explicit Host(DifficultyCategoryBands& f):factors(f){}
    void sample_category(std::size_t i) override {
        const int band = i%2==0 ? 1 : 3;
        const float delta = i%2==0 ? 0.25f : -0.25f;
        assert(factors.add(i,band,delta)==DifficultyBandStatus::Applied);
        ++samples;
    }
    void follow_up(std::size_t) override {++followups;}
    void reset_feedback_counters() override {}
};
int main() {
    DifficultyCategoryBands values;
    std::array<DifficultyBandPair,6> initial{};
    assert(values.configure(initial));
    Host host(values);
    DifficultyFeedbackTick tick(host);
    assert(tick.configure(10.0f,10.0f,0,false));
    assert(tick.advance_to(9.0f)==FeedbackTickResult::Deferred);
    assert(host.samples==0);
    assert(tick.advance_to(10.0f)==FeedbackTickResult::SampledNoPublication);
    assert(host.samples==6 && host.followups==0);
    for(std::size_t i=0;i<6;++i) {
        const auto* row=values.category(i);
        assert(row);
        assert(row->low_current==(i%2==0?1.25f:1.0f));
        assert(row->high_current==(i%2==0?1.0f:0.75f));
        assert(row->low_saved==1.0f && row->high_saved==1.0f);
    }
    tick.request_publication();
    assert(tick.advance_to(20.0f)==FeedbackTickResult::Published);
    assert(host.samples==12 && host.followups==6);
    for(std::size_t i=0;i<6;++i) {
        const auto* row=values.category(i);
        assert(row);
        assert(row->low_current==(i%2==0?1.5f:1.0f));
        assert(row->high_current==(i%2==0?1.0f:0.5f));
    }
    assert(values.reset_from_saved()==DifficultyBandStatus::Applied);
    for(std::size_t i=0;i<6;++i) {
        const auto* row=values.category(i);
        assert(row && row->low_current==1.0f && row->high_current==1.0f);
    }
}
