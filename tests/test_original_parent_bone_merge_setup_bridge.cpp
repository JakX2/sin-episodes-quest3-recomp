#include "sin_compat/original_parent_bone_merge.hpp"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>
using namespace sin_compat::jessica;
void eq(float a,float b){assert(std::abs(a-b)<0.01f);}
struct FakeSetup final:IOriginalSkeletalSetupHost {
    OriginalSkeletalSetupSnapshot state{};
    bool parent_prepared=true,locked=false;
    std::vector<std::string> calls;
    FakeSetup(){state.valid_studio=true;state.parent=42;state.world_angles={};}
    bool lock_model_cache(WorldEntityId)override{assert(!locked);locked=true;calls.push_back("lock");return true;}
    bool unlock_model_cache(WorldEntityId)override{assert(locked);locked=false;calls.push_back("unlock");return true;}
    std::optional<OriginalSkeletalSetupSnapshot> snapshot(WorldEntityId)override{return state;}
    bool refresh_world_transform(WorldEntityId)override{return true;}
    bool increment_pose_driver_counter(WorldEntityId)override{return true;}
    bool seed_pose_driver(WorldEntityId,const TossVec3&,const TossVec3&,std::uint32_t)override{return true;}
    bool evaluate_virtual_base_pose(WorldEntityId,std::uint32_t)override{calls.push_back("eval");return true;}
    bool blend_pose_driver(WorldEntityId,std::uint32_t)override{return true;}
    bool invoke_virtual_pose_hook(WorldEntityId,std::uint32_t)override{return true;}
    bool finalize_pose_driver(WorldEntityId,std::uint32_t)override{return true;}
    bool parent_is_animating(WorldEntityId,WorldEntityId id)override{return id==42;}
    bool prepare_parent_bone_cache(WorldEntityId id)override{return id==42&&parent_prepared;}
    bool compose_parented_bones(WorldEntityId,WorldEntityId,std::uint32_t)override{assert(false);return false;}
    bool compose_unparented_bones(WorldEntityId,std::uint32_t)override{calls.push_back("unparented");return true;}
    std::optional<std::vector<WorldMatrix3x4>> resulting_bone_matrices(WorldEntityId)override{
        return std::vector<WorldMatrix3x4>{*make_local_angle_matrix({0,0,0},{1,2,3})};
    }
};
struct FakeSource final:IOriginalParentBoneMergeSource {
    std::optional<OriginalParentBoneMergeInput> input;
    int hits=0;
    std::optional<OriginalParentBoneMergeInput> merge_inputs(
        WorldEntityId child,WorldEntityId parent,std::uint32_t mask)override {
        assert(child==11&&parent==42&&mask==0x300);
        ++hits;return input;
    }
};
int main(){
    FakeSetup setup;FakeSource source;
    OriginalParentBoneMergeInput data;
    data.parent_studio.bones={{"Spine",-1}};
    data.parent_studio.sorted_name_indices={0};
    data.child_studio.bones={{"spine",-1},{"Hand",0}};
    data.child_local_poses={{{1,0,0},{0,0,0,1}},{{2,0,0},{0,0,0,1}}};
    data.parent_bone_cache_bytes.assign(0x14+16+48,0);
    data.parent_bone_cache_bytes[0x0e]=16;
    const auto matrix=*make_local_angle_matrix({0,0,0},{100,0,0});
    for(std::size_t i=0;i<12;i++){
        std::uint32_t raw=0;std::memcpy(&raw,&matrix.row_major[i],4);
        for(int j=0;j<4;j++)data.parent_bone_cache_bytes[0x24+i*4+j]=
            static_cast<std::uint8_t>(raw>>(8*j));
    }
    source.input=data;
    OriginalParentBoneMergeSkeletalAdapter bridge(setup,source);
    const auto result=run_original_skeletal_setup(bridge,11,0x300);
    assert(result.code==OriginalSkeletalSetupCode::ParentMerged);
    assert(source.hits==1&&result.matrices.size()==2&&!setup.locked);
    eq(result.matrices[0].row_major[3],100);
    eq(result.matrices[1].row_major[3],102);
    assert((setup.calls==std::vector<std::string>{"lock","eval","unlock"}));
    setup.parent_prepared=false;setup.calls.clear();
    const auto fallback=run_original_skeletal_setup(bridge,11,0x300);
    assert(fallback.code==OriginalSkeletalSetupCode::Unparented);
    assert(fallback.matrices.size()==1);
    eq(fallback.matrices[0].row_major[3],1);
    assert((setup.calls==std::vector<std::string>{"lock","eval","unparented","unlock"}));
    setup.parent_prepared=true;source.input.reset();
    const auto rejected=run_original_skeletal_setup(bridge,11,0x300);
    assert(rejected.code==OriginalSkeletalSetupCode::SetupRejected);
    assert(!setup.locked);
    std::cout<<"Original SiN merge -> Phase113 skeletal dispatch PASS\n";
}
