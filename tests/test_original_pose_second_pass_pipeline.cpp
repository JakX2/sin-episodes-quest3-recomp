#include "sin_compat/original_pose_second_pass_state.hpp"
#include "sin_compat/original_pose_bone_matrix.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace sin_compat::jessica;
void eq(float a,float b){assert(std::abs(a-b)<0.02f);}
struct Geometry final:IOriginalPoseSecondPassGeometry {
    int samples=0;
    std::vector<WorldMatrix3x4> outputs;
    bool evaluate_active_track(std::size_t i,
                               const OriginalPoseDriverTrack& track) override {
        ++samples;
        assert(i==1);
        OriginalPoseBoneMatrixInput input{};
        input.parent_bones={-1,0};
        input.local_poses={{{1,0,0},{0,0,0,1}},
                           {track.origin,
                           {track.quaternion[0],track.quaternion[1],
                            track.quaternion[2],track.quaternion[3]}}};
        input.root_world=*make_local_angle_matrix({0,0,0},{10,20,30});
        input.requested_bone=1;
        const auto r=resolve_original_pose_bone_matrix(input);
        if(!r.successful()||!r.matrices[1])return false;
        outputs.push_back(*r.matrices[1]);
        return true;
    }
};
int main(){
    OriginalPoseDriverCommand c{};
    c.type=5;c.selector=7;c.track_index=1;
    c.origin={2,3,4};c.quaternion={0,0,0,1};
    c.influence=1.f;c.weight=1.f;c.limit=1.f;
    const auto first=apply_original_pose_driver_first_pass(2,{{c}},0);
    assert(first.successful() && first.commands_applied==1);
    Geometry host;
    auto second=apply_original_pose_second_pass_state(first.tracks,9,{},&host);
    assert(second.successful() && second.evaluated==1 && second.skipped==1);
    assert(second.histories_cleared==1 && host.samples==1);
    assert(second.states[1].full_cap_mode && second.states[1].last_sequence==9);
    eq(host.outputs[0].row_major[3],13);
    eq(host.outputs[0].row_major[7],23);
    eq(host.outputs[0].row_major[11],34);
    const auto previous=second.states;
    second=apply_original_pose_second_pass_state(first.tracks,10,previous,&host);
    assert(second.successful() && second.histories_cleared==0);
    assert(host.samples==2 && second.states[1].last_sequence==10);
    second=apply_original_pose_second_pass_state(first.tracks,12,second.states,&host);
    assert(second.successful() && second.histories_cleared==1);
    // Deactivated tracks are not sampled or overwritten.
    c.influence=0;
    auto reset=apply_original_pose_driver_first_pass(2,{{c}},0);
    assert(reset.successful());
    const auto before=second.states;
    second=apply_original_pose_second_pass_state(reset.tracks,30,before,&host);
    assert(second.successful() && second.evaluated==0 && second.skipped==2);
    assert(host.samples==3 && second.states[1].last_sequence==12);
    std::cout<<"SiN first-pass -> recursive bones -> second-pass state PASS\n";
}
