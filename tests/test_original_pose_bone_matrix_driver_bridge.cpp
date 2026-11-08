#include "sin_compat/original_pose_bone_matrix.hpp"
#include "sin_compat/original_pose_driver_first_pass.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace sin_compat::jessica;
void eq(float a,float b){assert(std::abs(a-b)<0.015f);}
int main(){
    OriginalPoseDriverCommand command{};
    command.type=5;command.track_index=1;command.selector=7;
    command.origin={2,3,4};command.quaternion={0,0,0,1};
    command.influence=1;command.weight=1;command.limit=1;
    const auto first=apply_original_pose_driver_first_pass(2,{{command}},0);
    assert(first.successful() && first.commands_applied==1);
    OriginalPoseBoneMatrixInput tree{};
    tree.parent_bones={-1,0};
    tree.local_poses={{{1,0,0},{0,0,0,1}},
                      {first.tracks[1].origin,
                       {first.tracks[1].quaternion[0],
                        first.tracks[1].quaternion[1],
                        first.tracks[1].quaternion[2],
                        first.tracks[1].quaternion[3]}}};
    tree.root_world=*make_local_angle_matrix({0,0,0},{10,20,30});
    tree.requested_bone=1;
    auto result=resolve_original_pose_bone_matrix(tree);
    assert(result.successful() && result.newly_built==2);
    eq(result.matrices[1]->row_major[3],13);
    eq(result.matrices[1]->row_major[7],23);
    eq(result.matrices[1]->row_major[11],34);
    // Revisit the same sampled pose with the bitset retained, no rebuild.
    tree.computed_words=result.computed_words;
    tree.previous_matrices=result.matrices;
    result=resolve_original_pose_bone_matrix(tree);
    assert(result.code==OriginalPoseBoneMatrixCode::AlreadyComputed);
    assert(result.newly_built==0);
    std::cout<<"SiN first-pass pose -> recursive world-bone matrix PASS\n";
}
