#include "sin_compat/original_parent_bone_merge.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
using namespace sin_compat::jessica;
using Code=OriginalParentBoneMergeCode;
void eq(float a,float b){assert(std::abs(a-b)<0.025f);}
void put16(std::vector<std::uint8_t>& b,std::size_t p,std::uint16_t v){
    b.at(p)=static_cast<std::uint8_t>(v);b.at(p+1)=static_cast<std::uint8_t>(v>>8);
}
void putf(std::vector<std::uint8_t>& b,std::size_t p,float f){
    std::uint32_t u=0;std::memcpy(&u,&f,4);
    for(int i=0;i<4;i++)b.at(p+i)=static_cast<std::uint8_t>(u>>(8*i));
}
std::vector<std::uint8_t> bone_cache(){
    std::vector<std::uint8_t> b(0x14+0x40+2*48);
    put16(b,0x0e,0x40); // two matrices from +0x54
    put16(b,0x14,1);    // Pelvis -> matrix 1
    put16(b,0x16,0xffff); // Arm -> absent/uncached
    put16(b,0x18,0);    // Other -> matrix 0
    auto a=*make_local_angle_matrix({0,0,0},{50,0,0});
    auto c=*make_local_angle_matrix({0,0,0},{100,10,0});
    for(int i=0;i<12;i++){
        putf(b,0x54+i*4,a.row_major[i]);
        putf(b,0x54+48+i*4,c.row_major[i]);
    }
    return b;
}
OriginalParentBoneMergeInput fixture(){
    OriginalParentBoneMergeInput in;
    in.parent_studio.bones={{"PELVis",-1},{"Arm",0},{"Other",0}};
    in.parent_studio.sorted_name_indices={1,2,0};
    in.child_studio.bones={{"pelvis",-1},{"ARM",0},{"hand",1}};
    in.child_local_poses={
        {{0,0,5},{0,0,0,1}},
        {{1,0,0},{0,0,0.70710678f,0.70710678f}},
        {{2,0,0},{0,0,0,1}}};
    in.parent_bone_cache_bytes=bone_cache();
    in.child_world_origin={1,2,3};
    return in;
}
void test_merging(){
    auto in=fixture();
    const auto r=merge_original_parent_bones(in);
    assert(r.code==Code::Merged && r.matrices.size()==3);
    assert(r.reused_parent_bones==1 && r.locally_composed_bones==2);
    // Exact copied parent world matrix, child root pose ignored.
    eq(r.matrices[0].row_major[3],100);
    eq(r.matrices[0].row_major[7],10);
    eq(r.matrices[0].row_major[11],0);
    // Parent cache 'Arm' is deliberately uncached, so compose its local
    // quaternion and position using the *merged* child root world matrix.
    eq(r.matrices[1].row_major[3],101);
    eq(r.matrices[1].row_major[7],10);
    eq(r.matrices[1].row_major[0],0);
    eq(r.matrices[1].row_major[4],1);
    // Hand is not in parent studio, so inherit rotation of child arm.
    eq(r.matrices[2].row_major[3],101);
    eq(r.matrices[2].row_major[7],12);
    assert(find_original_bone_by_name(in.parent_studio,"arm")==1);
    assert(find_original_bone_by_name(in.parent_studio,"PELvis")==0);
    assert(!find_original_bone_by_name(in.parent_studio,"HAND"));
}
void test_root_fallback(){
    auto in=fixture();
    in.parent_studio.bones={{"unrelated",-1}};
    in.parent_studio.sorted_name_indices={0};
    in.child_world_angles={0,90,0};in.child_world_origin={10,20,30};
    auto r=merge_original_parent_bones(in);
    assert(r.code==Code::Merged && r.reused_parent_bones==0);
    eq(r.matrices[0].row_major[3],10);
    eq(r.matrices[0].row_major[7],20);
    eq(r.matrices[0].row_major[11],35);
    // child local arm offset x=1 rotates to world +Y when root yaw=90.
    eq(r.matrices[1].row_major[3],10);
    eq(r.matrices[1].row_major[7],21);
}
void test_robustness(){
    auto in=fixture();
    in.child_studio.bones[1].parent_index=99;
    assert(merge_original_parent_bones(in).code==Code::InvalidModel);
    in=fixture();in.child_studio.bones[1].parent_index=1;
    assert(merge_original_parent_bones(in).code==Code::InvalidModel);
    in=fixture();in.child_local_poses.pop_back();
    assert(merge_original_parent_bones(in).code==Code::InvalidModel);
    in=fixture();in.parent_studio.sorted_name_indices={0,1,2};
    assert(merge_original_parent_bones(in).code==Code::InvalidModel);
    in=fixture();in.parent_studio.sorted_name_indices={1,1,0};
    assert(merge_original_parent_bones(in).code==Code::InvalidModel);
    in=fixture();in.parent_bone_cache_bytes.clear();
    assert(merge_original_parent_bones(in).code==Code::InvalidParentCache);
    in=fixture();in.child_local_poses[2].orientation.z=
         std::numeric_limits<float>::quiet_NaN();
    assert(merge_original_parent_bones(in).code==Code::InvalidPose);
    in=fixture();in.child_world_origin.z=std::numeric_limits<float>::infinity();
    assert(merge_original_parent_bones(in).code==Code::InvalidModel);
    in=fixture();in.child_local_poses[2].orientation.x=
         std::numeric_limits<float>::max();
    assert(merge_original_parent_bones(in).code==Code::InvalidPose);
    in=fixture();put16(in.parent_bone_cache_bytes,0x14,0x7fff);
    assert(merge_original_parent_bones(in).code==Code::InvalidParentCache);
}
int main(){test_merging();test_root_fallback();test_robustness();
    std::cout<<"Original 2006 SiN parent-bone name/cached matrix merge PASS\n";}
