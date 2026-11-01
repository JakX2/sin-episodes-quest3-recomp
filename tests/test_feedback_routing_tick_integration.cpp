#include "sin_compat/difficulty_feedback_routing.hpp"
#include "sin_compat/difficulty_feedback_tick.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <vector>
using namespace sin_compat;
struct Scene : IDifficultyFeedbackTickHost {
    DifficultyFeedbackRouting route;
    std::vector<FeedbackCategorySelection> sampled;
    std::vector<std::size_t> follows;
    std::size_t resets=0;
    DifficultyFeedbackTick* tick=nullptr;
    void sample_category(std::size_t index) override {
        FeedbackCategorySelection record;
        assert(route.select(index,record));
        sampled.push_back(record);
        if (index==5) tick->request_publication(); // synthetic only
    }
    void follow_up(std::size_t stage) override { follows.push_back(stage); }
    void reset_feedback_counters() override { ++resets; }
};
int main() {
    Scene scene;
    assert(scene.route.configure(3,{{4,3,2,1,4,3}}));
    DifficultyFeedbackTick tick(scene);
    scene.tick=&tick;
    assert(tick.configure(2.0f,4.0f,1));
    assert(tick.advance_to(1.0f)==FeedbackTickResult::Deferred);
    assert(scene.sampled.empty());
    assert(tick.advance_to(2.0f)==FeedbackTickResult::Published);
    assert(tick.next_due()==4.0f);
    assert(scene.sampled.size()==6);
    const int expected[]={4,2,2,1,4,3};
    for (std::size_t i=0;i<6;++i) {
        assert(static_cast<std::size_t>(scene.sampled[i].category)==i);
        assert(scene.sampled[i].effective_level==expected[i]);
    }
    assert(scene.resets==1 && scene.follows.size()==6);
    assert(scene.follows[0]==0 && scene.follows[5]==5);
    // Changing host levels for a later tick does not recompute older events.
    assert(scene.route.configure(0,{{4,3,2,1,4,3}}));
    assert(tick.advance_to(4.0f)==FeedbackTickResult::Published);
    assert(tick.next_due()==8.0f);
    assert(scene.sampled.size()==12 && scene.resets==2);
    assert(scene.sampled[6].effective_level==2);
    assert(scene.sampled[10].effective_level==4); // Death bypass
    assert(scene.sampled[11].effective_level==3); // Time bypass
}
