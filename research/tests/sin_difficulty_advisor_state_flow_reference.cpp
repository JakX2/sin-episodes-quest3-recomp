#include "sin_compat/difficulty_advisor_state_flow.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>
using namespace sin_compat::difficulty;
static bool near(float a,float b){return std::fabs(a-b)<0.00002f;}
int main(){
    static_assert(challenge_advisor_count==6 && advisor_count==6 && channel_count==4);
    auto state=derive_all_advisor_fields(0.2f);
    const std::array<float,6> expected_scores{{62.f,31.f,54.f,20.5f,31.f,0.f}};
    for(std::size_t i=0;i<6;++i){
        assert(near(state[i].score_value,expected_scores[i]));
        const auto expected=blend_advisor_preset(original_advisor_presets[i],0.2f);
        assert(near(state[i].channels[0],expected.output_0));
        assert(near(state[i].channels[1],expected.output_1));
        assert(near(state[i].channels[2],expected.output_2));
        assert(near(state[i].channels[3],expected.output_3));
        assert(near(state[i].score_value,
               std::fabs(state[i].channels[1]+state[i].channels[2])*0.5f));
    }
    assert(near(state[0].channels[0],80.4f));
    assert(near(state[0].channels[1],72.f));
    assert(near(state[0].channels[2],52.f));
    assert(near(state[0].score_value,62.f));
    const auto negative=derive_advisor_fields({30.f,-70.f,-10.f,4.f});
    assert(near(negative.score_value,40.f));
    assert(near(negative.channels[3],4.f));
    std::vector<int> ids{7,8,10};
    std::vector<int> notified_ids;
    std::vector<float> delivered;
    store_and_forward_advisor_score(state[0],12.5f,ids.begin(),ids.end(),
        [&](int id,float value){notified_ids.push_back(id);delivered.push_back(value);});
    assert(near(state[0].score_value,12.5f));
    assert((notified_ids==std::vector<int>{7,8,10}));
    assert(delivered.size()==3);
    for(float value:delivered)assert(near(value,12.5f));
    notified_ids.clear();delivered.clear();
    store_and_forward_advisor_score(state[0],16.f,ids.begin(),ids.begin(),
        [&](int id,float value){notified_ids.push_back(id);delivered.push_back(value);});
    assert(near(state[0].score_value,16.f) && notified_ids.empty());
    std::array<float,6> aux{};
    broadcast_auxiliary_scalar(aux,0.625f);
    for(float value:aux)assert(near(value,0.625f));
    std::array<std::int16_t,6> codes{{2,1,3,4,0,-1}};
    const auto scores=advisor_scores_packet(state,codes);
    const auto channels=advisor_channels_packet(state);
    assert(near(scores.entries[0].value,16.f));
    ClientAdvisorPresentation client{};
    apply_advisor_scores(client,scores);
    apply_advisor_channels(client,channels);
    for(std::size_t i=0;i<6;++i){
        assert(near(client.scores[i],state[i].score_value));
        assert(client.codes[i]==codes[i]);
        for(std::size_t c=0;c<4;++c)
            assert(near(client.channels[c][i],state[i].channels[c]));
    }
    const auto start=derive_all_advisor_fields(0.f);
    const auto end=derive_all_advisor_fields(1.f);
    assert(!near(start[0].score_value,end[0].score_value));
}
