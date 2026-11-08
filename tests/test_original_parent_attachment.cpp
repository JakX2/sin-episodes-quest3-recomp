#include "sin_compat/original_parent_attachment.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
using namespace sin_compat::jessica;
using Route=OriginalParentMatrixRoute;
void near(float a,float b){assert(std::fabs(a-b)<0.003f);}
WorldMatrix3x4 matrix(float x,float y,float z) {
    WorldMatrix3x4 m{};m.row_major={1,0,0,x,0,1,0,y,0,0,1,z};return m;
}
struct Host final:IOriginalParentMatrixHost {
    std::map<WorldEntityId,OriginalParentBinding> bindings;
    std::map<WorldEntityId,OriginalParentWorldState> parents;
    std::optional<OriginalAttachmentPose> pose;
    std::vector<std::string> log;
    bool animating=false,fail_refresh=false,throw_pose=false;
    std::optional<OriginalParentBinding> binding(WorldEntityId id) override {
        log.emplace_back("binding");auto i=bindings.find(id);
        return i==bindings.end()?std::nullopt:
            std::optional<OriginalParentBinding>(i->second);
    }
    bool parent_is_live(WorldEntityId id) override {
        log.emplace_back("live");return parents.count(id)!=0;
    }
    bool parent_has_animating_interface(WorldEntityId) override {
        log.emplace_back("animating");return animating;
    }
    std::optional<OriginalAttachmentPose> attachment_pose(
        WorldEntityId,std::uint8_t slot) override {
        assert(slot==7);log.emplace_back("attachment");
        if(throw_pose)throw std::runtime_error("host error");
        return pose;
    }
    std::optional<OriginalParentWorldState> parent_world_state(
        WorldEntityId id) override {
        log.emplace_back("matrix");auto i=parents.find(id);
        return i==parents.end()?std::nullopt:
            std::optional<OriginalParentWorldState>(i->second);
    }
    bool refresh_parent_world(WorldEntityId id) override {
        log.emplace_back("refresh");
        if(fail_refresh)return false;
        parents.at(id).dirty=false;return true;
    }
};
int main(){
    Host h;
    assert(resolve_original_parent_matrix(h,11).route==Route::MissingChild);
    h.log.clear();h.bindings[11]={std::nullopt,0};
    auto r=resolve_original_parent_matrix(h,11);
    assert(r.route==Route::Identity);
    near(r.matrix.row_major[0],1);near(r.matrix.row_major[5],1);
    near(r.matrix.row_major[10],1);near(r.matrix.row_major[3],0);
    assert((h.log==std::vector<std::string>{"binding"}));
    h.bindings[11]={42,7};h.log.clear();
    assert(resolve_original_parent_matrix(h,11).route==Route::Identity);
    assert((h.log==std::vector<std::string>{"binding","live"}));
    h.parents[42]={matrix(10,20,30),false};h.log.clear();
    r=resolve_original_parent_matrix(h,11);
    assert(r.route==Route::ParentMatrix);
    near(r.matrix.row_major[3],10);
    assert((h.log==std::vector<std::string>{"binding","live","animating","matrix"}));
    h.animating=true;h.log.clear();
    r=resolve_original_parent_matrix(h,11);
    assert(r.route==Route::ParentMatrix);
    // A false attachment lookup must discard its fallback angles/origin.
    assert((h.log==std::vector<std::string>{
        "binding","live","animating","attachment","matrix"}));
    h.pose=OriginalAttachmentPose{{80,90,100},{0,90,0}};h.log.clear();
    r=resolve_original_parent_matrix(h,11);
    assert(r.route==Route::AttachmentMatrix);
    near(r.matrix.row_major[3],80);
    near(r.matrix.row_major[7],90);
    near(r.matrix.row_major[0],0);
    near(r.matrix.row_major[1],-1);
    assert((h.log==std::vector<std::string>{
        "binding","live","animating","attachment"}));
    h.bindings[11].attachment=0;h.log.clear();
    assert(resolve_original_parent_matrix(h,11).route==Route::ParentMatrix);
    assert((h.log==std::vector<std::string>{"binding","live","matrix"}));
    h.parents[42].dirty=true;h.log.clear();
    assert(resolve_original_parent_matrix(h,11).route==Route::ParentMatrix);
    assert((h.log==std::vector<std::string>{
        "binding","live","matrix","refresh","matrix"}));
    h.parents[42].dirty=true;h.fail_refresh=true;
    assert(resolve_original_parent_matrix(h,11).route==Route::HostRejected);
    h.parents[42].dirty=false;h.fail_refresh=false;
    h.bindings[11].attachment=7;
    h.pose=OriginalAttachmentPose{{0,0,0},{
        0,std::numeric_limits<float>::quiet_NaN(),0}};
    assert(resolve_original_parent_matrix(h,11).route==Route::InvalidTransform);
    h.throw_pose=true;
    assert(resolve_original_parent_matrix(h,11).route==Route::HostRejected);
    h.throw_pose=false;h.pose.reset();
    h.parents[42].matrix.row_major[0]=std::numeric_limits<float>::infinity();
    assert(resolve_original_parent_matrix(h,11).route==Route::InvalidTransform);
    std::cout<<"Original SiN attachment-parent matrix selection PASS\n";
}
