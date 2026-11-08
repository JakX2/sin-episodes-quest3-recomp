#include "sin_compat/original_ordinary_bone.hpp"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>
using namespace sin_compat::jessica;
void eq(float a,float b){assert(std::abs(a-b)<0.025f);}
struct Setup final:IOriginalSkeletalSetupHost {
    OriginalSkeletalSetupSnapshot state{};
    bool locked=false,parent_ready=true;
    int native_unparented_calls=0;
    std::vector<std::string> calls;
    Setup(){state.valid_studio=true;state.parent.reset();}
    bool lock_model_cache(WorldEntityId) override {assert(!locked);locked=true;calls.push_back("lock");return true;}
    bool unlock_model_cache(WorldEntityId) override {assert(locked);locked=false;calls.push_back("unlock");return true;}
    std::optional<OriginalSkeletalSetupSnapshot> snapshot(WorldEntityId) override {return state;}
    bool refresh_world_transform(WorldEntityId) override{return true;}
    bool increment_pose_driver_counter(WorldEntityId) override{return true;}
    bool seed_pose_driver(WorldEntityId,const TossVec3&,const TossVec3&,std::uint32_t) override{return true;}
    bool evaluate_virtual_base_pose(WorldEntityId,std::uint32_t) override {calls.push_back("evaluate");return true;}
    bool blend_pose_driver(WorldEntityId,std::uint32_t) override{return true;}
    bool invoke_virtual_pose_hook(WorldEntityId,std::uint32_t) override{return true;}
    bool finalize_pose_driver(WorldEntityId,std::uint32_t) override{return true;}
    bool parent_is_animating(WorldEntityId,WorldEntityId p) override{return p==42;}
    bool prepare_parent_bone_cache(WorldEntityId p) override{return parent_ready&&p==42;}
    bool compose_parented_bones(WorldEntityId,WorldEntityId,std::uint32_t) override {
        assert(false);return false;
    }
    bool compose_unparented_bones(WorldEntityId,std::uint32_t) override {
        ++native_unparented_calls;assert(false);return false;
    }
    std::optional<std::vector<WorldMatrix3x4>> resulting_bone_matrices(WorldEntityId) override {
        return std::nullopt;
    }
};
struct Input final:IOriginalOrdinaryBoneSource {
    std::optional<OriginalOrdinaryBoneInput> data;
    int calls=0;
    std::optional<OriginalOrdinaryBoneInput> ordinary_inputs(
        WorldEntityId child,std::uint32_t mask) override {
        assert(child==7&&mask==0x300);
        ++calls;return data;
    }
};
struct ParentSource final:IOriginalParentBoneMergeSource {
    std::optional<OriginalParentBoneMergeInput> data;
    std::optional<OriginalParentBoneMergeInput> merge_inputs(
        WorldEntityId child,WorldEntityId parent,std::uint32_t mask) override {
        assert(child==7&&parent==42&&mask==0x300);
        return data;
    }
};
int main(){
    Setup native;Input args;ParentSource parent;
    OriginalOrdinaryBoneInput data;
    data.studio.bones={{"root",-1},{"hand",0},{"unselected",0}};
    data.bone_flags={0x300,0x100,0x0};
    data.local_poses={{{1,0,0},{0,0,0,1}},{{2,0,0},{0,0,0,1}},
                      {{4,0,0},{0,0,0,1}}};
    data.prior_world_matrices.resize(3);
    data.prior_world_matrices[2]=*make_local_angle_matrix({0,0,0},{90,0,0});
    data.world_origin={10,20,30};
    data.bone_mask=0; // stale host mask must NOT override caller mask
    args.data=data;
    OriginalOrdinaryBoneSkeletalAdapter ordinary(native,args);
    OriginalParentBoneMergeSkeletalAdapter both(ordinary,parent);
    auto r=run_original_skeletal_setup(both,7,0x300);
    assert(r.code==OriginalSkeletalSetupCode::Unparented&&!native.locked);
    assert(args.calls==1&&native.native_unparented_calls==0);
    assert(r.matrices.size()==3);
    eq(r.matrices[0].row_major[3],11);
    eq(r.matrices[1].row_major[3],13);
    eq(r.matrices[2].row_major[3],90);
    assert((native.calls==std::vector<std::string>{"lock","evaluate","unlock"}));
    native.state.parent=42;
    native.parent_ready=false;native.calls.clear();
    r=run_original_skeletal_setup(both,7,0x300);
    assert(r.code==OriginalSkeletalSetupCode::Unparented&&!native.locked);
    assert(args.calls==2);
    native.parent_ready=true;native.calls.clear();
    OriginalParentBoneMergeInput merged;
    merged.parent_studio.bones={{"root",-1}};
    merged.parent_studio.sorted_name_indices={0};
    merged.child_studio.bones={{"root",-1},{"hand",0}};
    merged.child_local_poses={{{1,0,0},{0,0,0,1}},{{2,0,0},{0,0,0,1}}};
    merged.child_world_origin={10,0,0};
    merged.parent_bone_cache_bytes.assign(0x14+16+48,0);
    merged.parent_bone_cache_bytes[0x0e]=16;
    auto matrix=*make_local_angle_matrix({0,0,0},{100,0,0});
    for(std::size_t i=0;i<12;++i){
        std::uint32_t bits=0;std::memcpy(&bits,&matrix.row_major[i],4);
        for(int j=0;j<4;++j)merged.parent_bone_cache_bytes[0x24+i*4+j]=
            static_cast<std::uint8_t>(bits>>(8*j));
    }
    parent.data=merged;
    r=run_original_skeletal_setup(both,7,0x300);
    assert(r.code==OriginalSkeletalSetupCode::ParentMerged&&!native.locked);
    eq(r.matrices[0].row_major[3],100);
    eq(r.matrices[1].row_major[3],102);
    assert(args.calls==2); // ordinary callback not invoked for parent merge
    args.data->prior_world_matrices[2].reset();
    native.state.parent.reset();
    r=run_original_skeletal_setup(both,7,0x300);
    assert(r.code==OriginalSkeletalSetupCode::SetupRejected&&!native.locked);
    std::cout<<"SiN masked ordinary bones -> parent merged setup bridge PASS\n";
}
