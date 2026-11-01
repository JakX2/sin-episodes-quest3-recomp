#include "sin_compat/difficulty_feedback_routing.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <vector>
using namespace sin_compat;
struct Sink : IFeedbackCategoryRoutingSink {
    std::vector<FeedbackCategorySelection> calls;
    void dispatch(const FeedbackCategorySelection& selection) override {
        calls.push_back(selection);
    }
};
int main() {
    DifficultyFeedbackRouting router;
    FeedbackCategorySelection output;
    assert(!router.select(0,output));
    Sink sink;
    assert(!router.dispatch_all(sink));
    assert(!router.configure(-1,{{0,0,0,0,0,0}}));
    assert(!router.configure(5,{{0,0,0,0,0,0}}));
    assert(!router.configure(2,{{0,0,0,0,0,5}}));
    assert(!router.configure(2,{{0,-1,0,0,0,0}}));
    assert(!router.configured());
    const int expected[5][5] = {
        {0,1,2,2,2}, {0,1,2,2,2},
        {0,1,2,3,4}, {0,1,2,2,4}, {0,1,2,2,2}
    };
    // Exhaust all 25 real-level combinations in each of 6 slots.
    for (int global=0;global<5;++global) {
        for(int local=0;local<5;++local) {
            assert(router.configure(global,{{local,local,local,local,local,local}}));
            assert(router.global_level()==global);
            assert(router.configured());
            for(std::size_t slot=0;slot<6;++slot) {
                assert(router.select(slot,output));
                assert(static_cast<std::size_t>(output.category)==slot);
                assert(output.raw_level==local);
                assert(output.bypasses_global==(slot>=4));
                assert(output.effective_level==
                    (slot<4?expected[global][local]:local));
            }
            assert(!router.select(6,output));
        }
    }
    // Unrelated categories preserve their own configured local level.
    assert(router.configure(0,{{4,3,2,1,4,3}}));
    assert(router.dispatch_all(sink));
    assert(sink.calls.size()==6);
    const int expected_mixed[]={2,2,2,1,4,3};
    for(std::size_t i=0;i<6;++i) {
        assert(static_cast<std::size_t>(sink.calls[i].category)==i);
        assert(sink.calls[i].effective_level==expected_mixed[i]);
    }
    // A rejected update invalidates the prior routing config (fail closed).
    assert(!router.configure(0,{{4,3,2,1,4,-1}}));
    assert(!router.dispatch_all(sink));
    assert(sink.calls.size()==6);
}
