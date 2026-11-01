#include "sin_compat/dynamic_difficulty_activation.hpp"
#include "sin_compat/info_challenge_runtime.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <map>
using namespace sin_compat;
struct Scene : IGameplayDifficultySettingSource, IInfoChallengeHost {
    GameplayDifficultyVariables vars;
    float convar=0.25f;
    std::map<unsigned,unsigned> generations;
    bool read_setting(float& out) const override {out=convar;return true;}
    bool is_live(EntityRef e) const override {
        auto it=generations.find(e.index);
        return it!=generations.end()&&it->second==e.generation;
    }
    bool initial_challenge(float& out) const override {out=42;return true;}
    bool current_challenge(float& out) const override {
        const auto rating=vars.current_challenge({"damage","armor"});
        if(!rating)return false;
        out=rating.rating_percent;return true;
    }
};
int main() {
    Scene scene;
    assert(scene.vars.configure({{"damage",0,0,10,0,10},
                                  {"armor",10,10,0,10,0}}));
    GameplayDifficultyActivation active(scene.vars,scene);
    InfoChallengeRuntime challenge(scene);
    EntityRef e{2,1};
    scene.generations[e.index]=e.generation;
    assert(challenge.register_entity(e,30.0f));
    assert(active.initialize_from_setting()==GameplayActivationStatus::Applied);
    assert(challenge.handle_input(e,"GetCurrentChallenge").value==25.0f);
    assert(challenge.handle_input(e,"GetChallengeDifference").value==-500.0f);
    scene.convar=0.75f;
    assert(active.restore_if_uninitialized()==GameplayActivationStatus::AlreadyInitialized);
    assert(challenge.handle_input(e,"GetCurrentChallenge").value==25.0f);
    assert(active.initialize_from_setting()==GameplayActivationStatus::Applied);
    assert(challenge.handle_input(e,"GetCurrentChallenge").value==75.0f);
    assert(challenge.handle_input(e,"GetChallengeDifference").value==4500.0f);
    assert(challenge.handle_input(e,"GetInitialChallenge").value==42.0f);
    assert(active.seed_cached_state(0.5f,false));
    assert(active.restore_if_uninitialized()==GameplayActivationStatus::Applied);
    assert(challenge.handle_input(e,"GetCurrentChallenge").value==50.0f);
    assert(active.cached_setting()==0.5f);
    scene.generations[e.index]=2;
    assert(challenge.handle_input(e,"GetCurrentChallenge").status==InfoChallengeQueryStatus::InvalidEntity);
}
