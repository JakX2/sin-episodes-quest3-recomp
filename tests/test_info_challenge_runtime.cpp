#include "sin_compat/info_challenge_runtime.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstring>
#include <map>
#include <limits>
using namespace sin_compat;
struct Host final : IInfoChallengeHost {
    std::map<unsigned,unsigned> gen;
    float initial=0.25f, current=0.5f;
    bool initial_ok=true,current_ok=true;
    EntityRef add(unsigned index, unsigned generation=1) {gen[index]=generation;return {index,generation};}
    bool is_live(EntityRef e) const override {
        auto it=gen.find(e.index);return it!=gen.end()&&it->second==e.generation;
    }
    bool initial_challenge(float& out) const override {out=initial;return initial_ok;}
    bool current_challenge(float& out) const override {out=current;return current_ok;}
};
int main() {
    Host host; auto entity=host.add(1);
    InfoChallengeRuntime info(host);
    assert(!info.handle_input(entity,"GetCurrentChallenge"));
    assert(info.register_entity(entity,0.25f));
    assert(!info.register_entity(entity,0.3f));
    auto initial=info.handle_input(entity,"GetInitialChallenge");
    assert(initial && std::strcmp(initial.output_name,"OnInitialChallenge")==0 && initial.value==0.25f);
    auto current=info.handle_input(entity,"GetCurrentChallenge");
    assert(current && std::strcmp(current.output_name,"OnCurrentChallenge")==0 && current.value==0.5f);
    auto diff=info.handle_input(entity,"GetChallengeDifference");
    assert(diff && std::strcmp(diff.output_name,"OnChallengeDifference")==0 && diff.value==25.0f);
    host.current=0.125f;
    assert(info.handle_input(entity,"GetChallengeDifference").value==-12.5f);
    host.current=0.0f;
    assert(info.handle_input(entity,"GetChallengeDifference").value==-25.0f);
    host.initial=0.875f;
    assert(info.handle_input(entity,"GetInitialChallenge").value==0.875f);
    // Original GetInitial and cached difference baseline use distinct sources.
    assert(info.handle_input(entity,"GetChallengeDifference").value==-25.0f);
    assert(info.handle_input(entity,"getinitialchallenge").status==InfoChallengeQueryStatus::UnknownInput);
    host.current_ok=false;
    assert(info.handle_input(entity,"GetCurrentChallenge").status==InfoChallengeQueryStatus::ProviderUnavailable);
    host.current_ok=true;
    host.current=std::numeric_limits<float>::infinity();
    assert(info.handle_input(entity,"GetCurrentChallenge").status==InfoChallengeQueryStatus::InvalidValue);
    host.current=0.5f;host.initial_ok=false;
    assert(info.handle_input(entity,"GetInitialChallenge").status==InfoChallengeQueryStatus::ProviderUnavailable);
    host.initial_ok=true;
    host.gen[1]=2;
    assert(info.handle_input(entity,"GetInitialChallenge").status==InfoChallengeQueryStatus::InvalidEntity);
    auto replacement=EntityRef{1,2};
    assert(info.register_entity(replacement,0.0f));
    info.remove_entity(entity);
    assert(info.entity_count()==1);
    assert(info.handle_input(replacement,"GetChallengeDifference").value==50.0f);
    auto bad=host.add(3);
    assert(!info.register_entity(bad,std::numeric_limits<float>::quiet_NaN()));
    info.remove_entity(replacement);
    assert(info.entity_count()==0);
}
