#include "sin_compat/original_studio_attachment.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
using namespace sin_compat::jessica;
void eq(float a,float b){assert(std::abs(a-b)<0.02f);}
WorldMatrix3x4 pose(float yaw,TossVec3 pos){
    return *make_local_angle_matrix({0,yaw,0},pos);
}
struct World final:IOriginalWorldTransformHost {
    std::map<WorldEntityId,WorldTransformSnapshot> entities;
    std::optional<WorldTransformSnapshot> snapshot(WorldEntityId id) override {
        const auto it=entities.find(id);
        if(it==entities.end())return std::nullopt;
        return it->second;
    }
    std::optional<WorldMatrix3x4> attachment_parent_matrix(
        WorldEntityId,WorldEntityId,std::uint8_t) override {return std::nullopt;}
    bool commit_world_transform(WorldEntityId id,const WorldTransformSnapshot& s) override {
        entities.at(id)=s;return true;
    }
    bool on_world_transform_rebuilt(WorldEntityId) override{return true;}
};
struct Studio final:IOriginalStudioAttachmentHost {
    std::uint16_t flags=0;
    bool has_attachment=true;
    WorldMatrix3x4 bone=pose(90,{100,200,0});
    std::optional<std::uint32_t> attachment_count(WorldEntityId) override {
        return has_attachment?std::optional<std::uint32_t>(1):std::nullopt;
    }
    std::optional<OriginalStudioAttachmentRecord> attachment_record(
        WorldEntityId,std::uint32_t index) override {
        if(index!=0)return std::nullopt;
        return OriginalStudioAttachmentRecord{flags,2,pose(0,{10,0,0})};
    }
    std::optional<WorldMatrix3x4> bone_world_transform(
        WorldEntityId,std::uint32_t) override {return bone;}
};
int main(){
    World world;Studio studio;
    WorldTransformSnapshot parent{};
    parent.world_matrix=pose(0,{1,2,3});
    parent.world_angles={0,0,0};parent.world_origin={1,2,3};
    world.entities[10]=parent;
    WorldTransformSnapshot child{};
    child.parent=10;child.parent_attachment=1;
    child.local_origin={1,0,0};child.local_angles={0,0,0};child.dirty=true;
    world.entities[11]=child;
    OriginalStudioNativeAttachmentAdapter animations(studio);
    OriginalAttachmentWorldTransformAdapter bridge(world,animations);
    auto result=refresh_original_world_transform(bridge,11);
    assert(result.successful());
    eq(result.world_origin.x,100);eq(result.world_origin.y,211);
    eq(result.world_angles.y,90);
    studio.flags=0x100;
    world.entities[11].dirty=true;
    result=refresh_original_world_transform(bridge,11);
    assert(result.successful());
    eq(result.world_origin.x,101);eq(result.world_origin.y,210);
    eq(result.world_angles.y,0);
    studio.has_attachment=false;
    world.entities[11].dirty=true;
    result=refresh_original_world_transform(bridge,11);
    assert(result.successful());
    eq(result.world_origin.x,2);eq(result.world_origin.y,2);
    world.entities[11].dirty=true;world.entities[11].parent=999;
    result=refresh_original_world_transform(bridge,11);
    assert(result.successful());
    eq(result.world_origin.x,1);eq(result.world_origin.y,0);
    std::cout<<"SiN studio bone attachment -> parent world transform PASS\n";
}
