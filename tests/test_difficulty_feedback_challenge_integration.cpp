#include "sin_compat/difficulty_feedback_tick.hpp"
#include "sin_compat/dynamic_difficulty_gpv.hpp"
#include "sin_compat/info_challenge_runtime.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <map>
#include <vector>
using namespace sin_compat;
// All inputs and event triggers here are SYNTHETIC; this is not the
// original feedback-weighting algorithm.
struct Scene final : IDifficultyFeedbackTickHost, IInfoChallengeHost {
    GameplayDifficultyVariables variables;
    DifficultyFeedbackTick* tick = nullptr;
    std::map<unsigned,unsigned> generations;
    std::vector<unsigned> sequence;
    unsigned collections=0, reports=0;
    bool is_live(EntityRef e) const override {
        auto i=generations.find(e.index);
        return i!=generations.end() && i->second==e.generation;
    }
    bool initial_challenge(float& out) const override {out=30.0f;return true;}
    bool current_challenge(float& out) const override {
        auto result=variables.current_challenge({"synthetic_damage","synthetic_ammo"});
        if(!result)return false;
        out=result.rating_percent;
        return true;
    }
    void sample_category(std::size_t i) override {
        sequence.push_back(static_cast<unsigned>(i));
        if(i==0) {
            ++collections;
            // Host-selected FAKE interpolation, not original performance->slider.
            assert(variables.interpolate(collections==1?0.75f:0.25f)==
                   GameplayDifficultyStatus::Ok);
        }
        if(i==5 && collections==1) tick->request_publication();
    }
    void follow_up(std::size_t i) override {
        ++reports;
        sequence.push_back(static_cast<unsigned>(10+i));
    }
    void reset_feedback_counters() override {sequence.push_back(99);}
};
int main() {
    Scene world;
    assert(world.variables.configure({
        {"synthetic_damage",0,0,1,0,1},
        {"synthetic_ammo",1,1,0,1,0},
    }));
    DifficultyFeedbackTick tick(world);
    world.tick=&tick;
    assert(tick.configure(10.0f,8.0f,1));
    InfoChallengeRuntime info(world);
    EntityRef e{7,1};world.generations[e.index]=e.generation;
    assert(info.register_entity(e,25.0f));
    assert(tick.advance_to(9)==FeedbackTickResult::Deferred);
    assert(info.handle_input(e,"GetCurrentChallenge").value==0.0f);
    assert(tick.advance_to(10)==FeedbackTickResult::Published);
    assert(tick.next_due()==14.0f);
    assert(info.handle_input(e,"GetCurrentChallenge").value==75.0f);
    assert(info.handle_input(e,"GetChallengeDifference").value==5000.0f);
    assert(world.sequence.size()==13 && world.sequence[7]==99);
    assert(tick.advance_to(14)==FeedbackTickResult::SampledNoPublication);
    assert(tick.next_due()==22);
    assert(info.handle_input(e,"GetCurrentChallenge").value==25.0f);
    assert(world.reports==6 && world.collections==2);
    world.generations[e.index]=2;
    assert(info.handle_input(e,"GetCurrentChallenge").status==
           InfoChallengeQueryStatus::InvalidEntity);
}
