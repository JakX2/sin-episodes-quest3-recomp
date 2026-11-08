#include "sin_compat/original_ordinary_bone.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace sin_compat::jessica;
using Code=OriginalOrdinaryBoneCode;
void eq(float a,float b){assert(std::abs(a-b)<0.025f);}
OriginalOrdinaryBoneInput model(){
    OriginalOrdinaryBoneInput in;
    in.studio.bones={{"root",-1},{"spine",0},{"hand",1},{"other",0}};
    in.bone_flags={0x300,0x100,0x200,0x0};
    in.local_poses={{{1,0,0},{0,0,0,1}},{{2,0,0},{0,0,0,1}},
                    {{3,0,0},{0,0,0,1}},{{4,0,0},{0,0,0,1}}};
    in.world_origin={10,20,30};in.world_angles={0,0,0};
    in.bone_mask=0x300;
    return in;
}
void test_full_mask_and_preservation(){
    auto in=model();
    auto r=build_original_ordinary_bones(in);
    assert(r.successful()&&r.built==3&&r.masked_out==1&&r.matrices.size()==4);
    eq(r.matrices[0]->row_major[3],11);
    eq(r.matrices[1]->row_major[3],13);
    eq(r.matrices[2]->row_major[3],16);
    assert(!r.matrices[3]); // masked-out matrix MUST NOT be overwritten
    in.prior_world_matrices.resize(4);
    in.prior_world_matrices[3]=*make_local_angle_matrix({0,0,0},{40,0,0});
    r=build_original_ordinary_bones(in);
    assert(r.successful()&&r.built==3&&r.masked_out==1);
    eq(r.matrices[3]->row_major[3],40);
    in.bone_mask=0x100;
    r=build_original_ordinary_bones(in);
    assert(r.successful()&&r.built==2&&r.masked_out==2);
    assert(!r.matrices[2]);
}
void test_single_chain_and_invalid_indices(){
    auto in=model();in.requested_bone=2;
    auto r=build_original_ordinary_bones(in);
    assert(r.successful()&&r.built==3&&r.masked_out==0);
    assert(!r.matrices[3]);eq(r.matrices[2]->row_major[3],16);
    in.requested_bone=3;
    r=build_original_ordinary_bones(in);
    assert(r.successful()&&r.built==1&&r.masked_out==1);
    assert(!r.matrices[1]);assert(!r.matrices[2]);
    in.requested_bone=900; // original clamps invalid index to 0
    r=build_original_ordinary_bones(in);
    assert(r.successful()&&r.built==1&&r.masked_out==0);
    assert(!r.matrices[1]);
    in.requested_bone=-100;
    r=build_original_ordinary_bones(in);
    assert(r.successful()&&r.built==1&&r.masked_out==0);
}
void test_mask_parent_dependency(){
    auto in=model();in.requested_bone=2;in.bone_mask=0x200;
    auto r=build_original_ordinary_bones(in);
    assert(r.code==Code::MissingParentMatrix); // parent spine masked out
    in.prior_world_matrices.resize(4);
    in.prior_world_matrices[1]=*make_local_angle_matrix({0,90,0},{50,60,70});
    r=build_original_ordinary_bones(in);
    assert(r.successful()&&r.built==2&&r.masked_out==1);
    eq(r.matrices[2]->row_major[3],50);
    eq(r.matrices[2]->row_major[7],63);
    eq(r.matrices[1]->row_major[3],50); // preserved masked parent
    // Missing/invalid poses for masked bones should not be examined.
    in.local_poses[1].position.x=std::numeric_limits<float>::quiet_NaN();
    r=build_original_ordinary_bones(in);
    assert(r.successful());
}
void test_hierarchy_invalids(){
    auto in=model();in.studio.bones[1].parent_index=1;
    assert(build_original_ordinary_bones(in).code==Code::InvalidHierarchy);
    in=model();in.studio.bones[2].parent_index=3;
    assert(build_original_ordinary_bones(in).code==Code::InvalidHierarchy);
    in=model();in.studio.bones[0].parent_index=-2;
    assert(build_original_ordinary_bones(in).code==Code::InvalidHierarchy);
    in=model();in.bone_flags.pop_back();
    assert(build_original_ordinary_bones(in).code==Code::InvalidStudio);
    in=model();in.local_poses.pop_back();
    assert(build_original_ordinary_bones(in).code==Code::InvalidStudio);
    in=model();in.local_poses[2].position.x=std::numeric_limits<float>::quiet_NaN();
    assert(build_original_ordinary_bones(in).code==Code::InvalidPose);
    in=model();in.prior_world_matrices.resize(4);
    in.prior_world_matrices[3]=*make_local_angle_matrix({0,0,0},{0,0,0});
    in.prior_world_matrices[3]->row_major[0]=std::numeric_limits<float>::infinity();
    assert(build_original_ordinary_bones(in).code==Code::InvalidMatrix);
    in=model();in.world_origin.x=std::numeric_limits<float>::infinity();
    assert(build_original_ordinary_bones(in).code==Code::InvalidPose);
}
void test_rotation_and_zero_mask(){
    auto in=model();in.studio.bones.resize(1);in.bone_flags.resize(1);in.local_poses.resize(1);
    in.local_poses[0]={{0,1,0},{0,0,std::sqrt(0.5f),std::sqrt(0.5f)}};
    in.world_angles={0,90,0};in.world_origin={10,10,0};
    auto r=build_original_ordinary_bones(in);
    assert(r.successful()&&r.built==1);
    eq(r.matrices[0]->row_major[3],9);
    eq(r.matrices[0]->row_major[7],10);
    eq(r.matrices[0]->row_major[0],-1);
    in.bone_mask=0;
    r=build_original_ordinary_bones(in);
    assert(r.successful()&&r.built==0&&r.masked_out==1&&!r.matrices[0]);
    in.studio.bones.clear();in.local_poses.clear();in.bone_flags.clear();
    r=build_original_ordinary_bones(in);
    assert(r.successful()&&r.matrices.empty()&&r.built==0);
}
int main(){test_full_mask_and_preservation();test_single_chain_and_invalid_indices();
    test_mask_parent_dependency();test_hierarchy_invalids();test_rotation_and_zero_mask();
    std::cout<<"Original SiN unparented skeletal builder tests PASS\n";}
