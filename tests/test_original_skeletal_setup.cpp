#include "sin_compat/original_skeletal_setup.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using namespace sin_compat::jessica;
using Code=OriginalSkeletalSetupCode;
WorldMatrix3x4 identity(float x=0) {
    WorldMatrix3x4 m{};m.row_major={1,0,0,x,0,1,0,0,0,0,1,0};return m;
}
struct Host final:IOriginalSkeletalSetupHost {
    OriginalSkeletalSetupSnapshot s{};
    bool lock_ok=true,unlock_ok=true,refresh_ok=true,pose_ok=true;
    bool parent_animating=false,parent_cache=true,compose_ok=true;
    bool output_ok=true,throw_after_lock=false;
    std::vector<std::string> log;
    TossVec3 seeded{};
    std::uint32_t seen_mask=0;
    std::optional<WorldEntityId> new_parent{};
    Host(){s.valid_studio=true;s.world_origin={1,2,3};s.world_angles={0,45,0};s.vertical_offset=7;}
    bool lock_model_cache(WorldEntityId) override {
        log.emplace_back("lock");return lock_ok;
    }
    bool unlock_model_cache(WorldEntityId) override {
        log.emplace_back("unlock");return unlock_ok;
    }
    std::optional<OriginalSkeletalSetupSnapshot> snapshot(WorldEntityId) override {
        log.emplace_back("state");
        if(throw_after_lock)throw std::runtime_error("host snapshot");
        return s;
    }
    bool refresh_world_transform(WorldEntityId) override {
        log.emplace_back("world-refresh");s.world_transform_dirty=false;return refresh_ok;
    }
    bool increment_pose_driver_counter(WorldEntityId) override {
        log.emplace_back("counter");return true;
    }
    bool seed_pose_driver(WorldEntityId,const TossVec3& origin,
                           const TossVec3&,std::uint32_t mask) override {
        log.emplace_back("seed");seeded=origin;seen_mask=mask;return pose_ok;
    }
    bool evaluate_virtual_base_pose(WorldEntityId,std::uint32_t mask) override {
        log.emplace_back("base+2ac");seen_mask=mask;
        if(new_parent)s.parent=new_parent;
        return pose_ok;
    }
    bool blend_pose_driver(WorldEntityId,std::uint32_t) override {
        log.emplace_back("blend");return pose_ok;
    }
    bool invoke_virtual_pose_hook(WorldEntityId,std::uint32_t) override {
        log.emplace_back("hook+2b8");return pose_ok;
    }
    bool finalize_pose_driver(WorldEntityId,std::uint32_t) override {
        log.emplace_back("finish-driver");return pose_ok;
    }
    bool parent_is_animating(WorldEntityId,WorldEntityId p) override {
        assert(p==2);log.emplace_back("cast-parent");return parent_animating;
    }
    bool prepare_parent_bone_cache(WorldEntityId p) override {
        assert(p==2);log.emplace_back("prepare-parent");return parent_cache;
    }
    bool compose_parented_bones(WorldEntityId,WorldEntityId p,
                                 std::uint32_t mask) override {
        assert(p==2&&mask==0x300);
        log.emplace_back("merge-parent");return compose_ok;
    }
    bool compose_unparented_bones(WorldEntityId,std::uint32_t mask) override {
        assert(mask==0x300);log.emplace_back("compose-base");return compose_ok;
    }
    std::optional<std::vector<WorldMatrix3x4>> resulting_bone_matrices(
        WorldEntityId) override {
        log.emplace_back("result");
        if(!output_ok)return std::nullopt;
        return std::vector<WorldMatrix3x4>{identity(25)};
    }
};
void same(const std::vector<std::string>& a,
          std::initializer_list<const char*> expected) {
    std::vector<std::string> e(expected.begin(),expected.end());
    if(a!=e){
        for(const auto& s:a)std::cerr<<s<<" ";
        std::cerr<<"\n";
        assert(false);
    }
}
void test_base(){
    Host h;auto r=run_original_skeletal_setup(h,1,0x300);
    assert(r.code==Code::Unparented && r.matrices.size()==1);
    same(h.log,{"lock","state","base+2ac","state",
                "compose-base","result","unlock"});
}
void test_driver(){
    Host h;h.s.has_pose_driver=true;h.s.world_transform_dirty=true;
    auto r=run_original_skeletal_setup(h,1,0x300);
    assert(r.code==Code::Unparented);
    assert(h.seen_mask==0x300);
    assert(h.seeded.x==1&&h.seeded.y==2&&h.seeded.z==10);
    same(h.log,{"lock","state","world-refresh","state","counter",
                "state","seed","base+2ac","blend","hook+2b8",
                "finish-driver","state","compose-base","result","unlock"});
}
void test_parent(){
    Host h;h.s.parent=2;h.parent_animating=true;
    auto r=run_original_skeletal_setup(h,1,0x300);
    assert(r.code==Code::ParentMerged);
    same(h.log,{"lock","state","base+2ac","state",
                "cast-parent","prepare-parent","state","merge-parent",
                "result","unlock"});
    h.log.clear();h.parent_cache=false;
    r=run_original_skeletal_setup(h,1,0x300);
    assert(r.code==Code::Unparented);
    same(h.log,{"lock","state","base+2ac","state",
                "cast-parent","prepare-parent","compose-base","result","unlock"});
    h.log.clear();h.parent_animating=false;
    r=run_original_skeletal_setup(h,1,0x300);
    assert(r.code==Code::Unparented);
    same(h.log,{"lock","state","base+2ac","state",
                "cast-parent","compose-base","result","unlock"});
}
void test_updated_parent(){
    Host h;h.parent_animating=true;h.new_parent=2;
    auto r=run_original_skeletal_setup(h,1,0x300);
    assert(r.code==Code::ParentMerged);
}
void test_errors(){
    Host h;h.lock_ok=false;
    assert(run_original_skeletal_setup(h,1,0x300).code==Code::ModelCacheRejected);
    same(h.log,{"lock"});
    h=Host{};h.s.valid_studio=false;
    assert(run_original_skeletal_setup(h,1,0x300).code==Code::MissingStudio);
    assert(h.log.back()=="unlock");
    h=Host{};h.throw_after_lock=true;
    assert(run_original_skeletal_setup(h,1,0x300).code==Code::HostRejected);
    assert(h.log.back()=="unlock");
    h=Host{};h.s.vertical_offset=std::numeric_limits<float>::infinity();
    assert(run_original_skeletal_setup(h,1,0x300).code==Code::InvalidState);
    h=Host{};h.s.vertical_offset=std::numeric_limits<float>::max();
    h.s.world_origin.z=std::numeric_limits<float>::max();
    assert(run_original_skeletal_setup(h,1,0x300).code==Code::InvalidState);
    h=Host{};h.pose_ok=false;
    assert(run_original_skeletal_setup(h,1,0x300).code==Code::SetupRejected);
    assert(h.log.back()=="unlock");
    h=Host{};h.compose_ok=false;
    assert(run_original_skeletal_setup(h,1,0x300).code==Code::SetupRejected);
    h=Host{};h.output_ok=false;
    assert(run_original_skeletal_setup(h,1,0x300).code==Code::MissingMatrices);
    h=Host{};h.unlock_ok=false;
    assert(run_original_skeletal_setup(h,1,0x300).code==Code::HostRejected);
}
int main(){test_base();test_driver();test_parent();test_updated_parent();test_errors();
    std::cout<<"Original SiN skeletal setup dispatch PASS\n";}
