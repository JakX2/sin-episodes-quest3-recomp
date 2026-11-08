#include "sin_compat/original_pose_bone_matrix.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace sin_compat::jessica;
using Code=OriginalPoseBoneMatrixCode;
void eq(float a,float b){assert(std::abs(a-b)<0.01f);}
OriginalPoseBoneMatrixInput sample(){
    OriginalPoseBoneMatrixInput i;
    i.parent_bones={-1,0,1,1};
    i.local_poses={{{1,0,0},{0,0,0,1}},
                   {{0,2,0},{0,0,0,1}},
                   {{0,0,3},{0,0,0,1}},
                   {{0,0,5},{0,0,0,1}}};
    i.root_world=*make_local_angle_matrix({0,0,0},{100,200,300});
    i.requested_bone=2;
    return i;
}
void test_leaf_recursion_and_bitset(){
    auto in=sample();auto r=resolve_original_pose_bone_matrix(in);
    assert(r.code==Code::Built && r.newly_built==3);
    assert(r.computed_words.size()==1 && r.computed_words[0]==0b111);
    eq(r.matrices[0]->row_major[3],101);
    eq(r.matrices[1]->row_major[7],202);
    eq(r.matrices[2]->row_major[11],303);
    assert(!r.matrices[3]);
    in.previous_matrices=r.matrices;in.computed_words=r.computed_words;
    auto reuse=resolve_original_pose_bone_matrix(in);
    assert(reuse.code==Code::AlreadyComputed && reuse.newly_built==0);
    in.requested_bone=3;
    auto sibling=resolve_original_pose_bone_matrix(in);
    assert(sibling.code==Code::Built && sibling.newly_built==1);
    assert(sibling.computed_words[0]==0b1111);
    eq(sibling.matrices[3]->row_major[11],305);
}
void test_rotation_and_forward_parent(){
    auto in=sample();in.parent_bones={2,0,-1,1};
    in.local_poses[2]={{1,0,0},{0,0,0.70710678f,0.70710678f}};
    in.local_poses[0]={{1,0,0},{0,0,0,1}};
    in.local_poses[1]={{1,0,0},{0,0,0,1}};
    in.requested_bone=3;
    auto r=resolve_original_pose_bone_matrix(in);
    assert(r.code==Code::Built && r.newly_built==4);
    eq(r.matrices[3]->row_major[3],101);
    eq(r.matrices[3]->row_major[7],202);
    eq(r.matrices[3]->row_major[11],305);
    eq(r.matrices[3]->row_major[0],0);
}
void test_errors(){
    auto in=sample();in.requested_bone=4;
    assert(resolve_original_pose_bone_matrix(in).code==Code::InvalidIndex);
    in=sample();in.parent_bones[0]=1;
    assert(resolve_original_pose_bone_matrix(in).code==Code::InvalidHierarchy);
    in=sample();in.parent_bones[1]=1;
    assert(resolve_original_pose_bone_matrix(in).code==Code::InvalidHierarchy);
    in=sample();in.local_poses.pop_back();
    assert(resolve_original_pose_bone_matrix(in).code==Code::InvalidModel);
    in=sample();in.computed_words={0x02};
    assert(resolve_original_pose_bone_matrix(in).code==Code::InvalidSnapshot);
    in=sample();in.computed_words={0x01};in.previous_matrices.resize(4);
    assert(resolve_original_pose_bone_matrix(in).code==Code::InvalidSnapshot);
    in=sample();in.computed_words={0x01};in.previous_matrices.resize(4);
    in.previous_matrices[0]=*make_local_angle_matrix({0,0,0},{4,5,6});
    auto reuse=resolve_original_pose_bone_matrix(in);
    assert(reuse.code==Code::Built && reuse.newly_built==2);
    eq(reuse.matrices[2]->row_major[3],4);
    in=sample();in.local_poses[1].orientation.w=std::numeric_limits<float>::quiet_NaN();
    assert(resolve_original_pose_bone_matrix(in).code==Code::InvalidPose);
    in=sample();in.root_world.row_major[0]=std::numeric_limits<float>::infinity();
    assert(resolve_original_pose_bone_matrix(in).code==Code::InvalidMatrix);
    in=sample();in.parent_bones[0]=-2;
    assert(resolve_original_pose_bone_matrix(in).code==Code::InvalidHierarchy);
}
int main(){test_leaf_recursion_and_bitset();test_rotation_and_forward_parent();test_errors();
 std::cout<<"Original SiN recursive pose bone matrix tests PASS\n";}
