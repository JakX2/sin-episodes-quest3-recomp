#include "sin_compat/dynamic_difficulty_gpv.hpp"
#include "sin_compat/info_challenge_runtime.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <map>
using namespace sin_compat;
struct Host final : IInfoChallengeHost {
    GameplayDifficultyVariables& vars;
    std::map<unsigned,unsigned> generations;
    float initial=50.0f;
    explicit Host(GameplayDifficultyVariables& v):vars(v){}
    bool is_live(EntityRef e) const override {
        auto it=generations.find(e.index);
        return it!=generations.end() && it->second==e.generation;
    }
    bool initial_challenge(float& value) const override {value=initial;return true;}
    bool current_challenge(float& value) const override {
        // Synthetic host selects two names. Real runtime's active GPV list
        // and cached baseline are NOT established by this phase.
        auto r=vars.current_challenge({"first","second"});
        if(!r)return false;
        value=r.rating_percent;return true;
    }
};
int main() {
    GameplayDifficultyVariables vars;
    assert(vars.configure({{"first",0.5f,0,1,0,1},
                           {"second",0.5f,1,0,1,0}}));
    Host host(vars);
    host.generations[7]=1;
    InfoChallengeRuntime challenge(host);
    EntityRef info{7,1};
    assert(challenge.register_entity(info,40.0f));
    assert(challenge.handle_input(info,"GetCurrentChallenge").value==50.0f);
    assert(challenge.handle_input(info,"GetChallengeDifference").value==1000.0f);
    assert(challenge.handle_input(info,"GetInitialChallenge").value==50.0f);
    assert(vars.interpolate(1.0f)==GameplayDifficultyStatus::Ok);
    assert(challenge.handle_input(info,"GetCurrentChallenge").value==100.0f);
    assert(challenge.handle_input(info,"GetChallengeDifference").value==6000.0f);
    host.generations[7]=2;
    assert(challenge.handle_input(info,"GetCurrentChallenge").status==
           InfoChallengeQueryStatus::InvalidEntity);
}
