#include "sin_compat/original_pose_second_pass_state.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace sin_compat::jessica;
using Code=OriginalPoseSecondPassCode;
OriginalPoseDriverTrack track(float influence,float cap){
    OriginalPoseDriverTrack t{};t.influence=influence;t.cap=cap;return t;
}
struct Geometry final:IOriginalPoseSecondPassGeometry {
    std::vector<std::size_t> called;
    int reject=-1;bool throwing=false;
    bool evaluate_active_track(std::size_t i,const OriginalPoseDriverTrack&) override {
        called.push_back(i);
        if(throwing)throw std::runtime_error("simulated native callback failure");
        return static_cast<int>(i)!=reject;
    }
};
void test_gate_and_history(){
    Geometry g;
    std::vector<OriginalPoseSecondPassState> prior(5);
    for(auto& state:prior){
        state.history_active=7;state.history_accumulator_bits=0x1234;
        state.last_sequence=9;state.full_cap_mode=true;
    }
    std::vector<OriginalPoseDriverTrack> tracks={
        track(0.f,1.f),track(-1.f,1.f),track(.5f,1.f),
        track(2.f,.8f),track(std::numeric_limits<float>::quiet_NaN(),1.f)};
    auto r=apply_original_pose_second_pass_state(tracks,10,prior,&g);
    assert(r.successful());assert(r.skipped==3 && r.evaluated==2);
    assert(r.histories_cleared==0);
    assert((g.called==std::vector<std::size_t>{2,3}));
    assert(r.states[0].last_sequence==9 && r.states[0].history_active==7);
    assert(r.states[4].last_sequence==9 && r.states[4].history_accumulator_bits==0x1234);
    assert(r.states[2].last_sequence==10 && r.states[2].full_cap_mode);
    assert(!r.states[3].full_cap_mode && r.states[3].history_active==7);
    r=apply_original_pose_second_pass_state(tracks,12,prior,&g);
    assert(r.successful() && r.histories_cleared==2);
    assert(r.states[2].history_active==0 && r.states[2].history_accumulator_bits==0);
    assert(r.states[0].history_active==7);
}
void test_window_and_numeric_checks(){
    std::vector<OriginalPoseSecondPassState> prior(1);
    prior[0]={true,1,0xdeadbeef,5};
    auto r=apply_original_pose_second_pass_state({track(1.f,1.f)},5,prior);
    assert(r.successful() && r.histories_cleared==0);
    assert(r.states[0].history_accumulator_bits==0xdeadbeef);
    r=apply_original_pose_second_pass_state({track(1.f,1.f)},6,prior);
    assert(r.successful() && r.histories_cleared==0);
    r=apply_original_pose_second_pass_state({track(1.f,1.f)},4,prior);
    assert(r.successful() && r.histories_cleared==1);
    r=apply_original_pose_second_pass_state({track(1.f,1.f)},-1,prior);
    assert(r.successful() && r.histories_cleared==1);
    assert(r.states[0].last_sequence==-1);
    r=apply_original_pose_second_pass_state({track(1.f,1.f)},0,
        {{false,1,1,-1}});
    assert(r.successful() && r.histories_cleared==0);
    r=apply_original_pose_second_pass_state({track(1.f,1.f)},0,
        {{false,1,1,0}});
    assert(r.successful() && r.histories_cleared==0);
    r=apply_original_pose_second_pass_state({track(1.f,1.f)},0,
        {{false,1,1,1}});
    assert(r.successful() && r.histories_cleared==1);
    r=apply_original_pose_second_pass_state(
        {track(std::numeric_limits<float>::infinity(),1.f)},0);
    assert(r.code==Code::InvalidFloat && r.states.empty());
    r=apply_original_pose_second_pass_state(
        {track(1.f,std::numeric_limits<float>::quiet_NaN())},0);
    assert(r.code==Code::InvalidFloat && r.states.empty());
    r=apply_original_pose_second_pass_state({track(1.f,1.f)},0,
        std::vector<OriginalPoseSecondPassState>(2));
    assert(r.code==Code::InvalidTrackCount);
    r=apply_original_pose_second_pass_state(
        std::vector<OriginalPoseDriverTrack>(5000),0);
    assert(r.code==Code::InvalidTrackCount);
}
void test_geometry_failure(){
    Geometry g;g.reject=1;
    auto r=apply_original_pose_second_pass_state(
        {track(1,1),track(.5f,1)},0,{},&g);
    assert(r.code==Code::GeometryRejected && r.states.empty());
    assert((g.called==std::vector<std::size_t>{0,1}));
    g.reject=-1;g.throwing=true;
    r=apply_original_pose_second_pass_state({track(1,1)},0,{},&g);
    assert(r.code==Code::HostRejected && r.states.empty());
}
int main(){test_gate_and_history();test_window_and_numeric_checks();
    test_geometry_failure();
    std::cout<<"Original SiN pose second pass gate and sequence continuity PASS\n";
}
