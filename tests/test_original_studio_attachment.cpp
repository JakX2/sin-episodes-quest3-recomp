#include "sin_compat/original_studio_attachment.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace sin_compat::jessica;
using Code=OriginalStudioAttachmentCode;
void eq(float a,float b){assert(std::abs(a-b)<0.002f);}
WorldMatrix3x4 pose(float yaw,TossVec3 pos){
    return *make_local_angle_matrix({0,yaw,0},pos);
}
struct Fake final:IOriginalStudioAttachmentHost {
    std::optional<std::uint32_t> count=2;
    std::vector<OriginalStudioAttachmentRecord> records;
    WorldMatrix3x4 bone=pose(90,{100,200,300});
    bool missing_bone=false,throws=false;
    std::vector<std::string> calls;
    std::optional<std::uint32_t> attachment_count(WorldEntityId) override {
        calls.emplace_back("count");
        if(throws)throw std::runtime_error("host");
        return count;
    }
    std::optional<OriginalStudioAttachmentRecord> attachment_record(
        WorldEntityId,std::uint32_t slot) override {
        calls.emplace_back("record:"+std::to_string(slot));
        if(slot>=records.size())return std::nullopt;
        return records[slot];
    }
    std::optional<WorldMatrix3x4> bone_world_transform(
        WorldEntityId,std::uint32_t bone_index) override {
        calls.emplace_back("bone:"+std::to_string(bone_index));
        if(missing_bone)return std::nullopt;
        return bone;
    }
};
void test_routes(){
    Fake h;
    h.records={{0,12,pose(90,{10,0,4})},
               {0x100,13,pose(90,{10,0,4})}};
    auto r=resolve_original_studio_attachment(h,7,1);
    assert(r.code==Code::Composed);
    eq(r.matrix.row_major[3],100);eq(r.matrix.row_major[7],210);
    eq(r.matrix.row_major[11],304);
    eq(r.matrix.row_major[0],-1);eq(r.matrix.row_major[4],0);
    assert((h.calls==std::vector<std::string>{"count","record:0","bone:12"}));
    h.calls.clear();r=resolve_original_studio_attachment(h,7,2);
    assert(r.code==Code::WorldAligned);
    eq(r.matrix.row_major[3],100);eq(r.matrix.row_major[7],210);
    eq(r.matrix.row_major[11],304);
    eq(r.matrix.row_major[0],1);eq(r.matrix.row_major[5],1);
    eq(r.matrix.row_major[10],1);eq(r.matrix.row_major[1],0);
    assert((h.calls==std::vector<std::string>{"count","record:1","bone:13"}));
    OriginalStudioNativeAttachmentAdapter adapter(h);
    auto p=adapter.get_attachment_pose(7,1);
    assert(p);eq(p->origin.y,210);
    assert(std::abs(std::abs(p->angles.y)-180)<0.1f);
    p=adapter.get_attachment_pose(7,2);
    assert(p);eq(p->angles.y,0);eq(p->origin.x,100);
}
void test_failures(){
    Fake h;h.records={{0,9,pose(0,{0,0,0})}};
    assert(resolve_original_studio_attachment(h,1,0).code==Code::InvalidIndex);
    assert(resolve_original_studio_attachment(h,1,3).code==Code::InvalidIndex);
    assert(resolve_original_studio_attachment(h,1,2).code==Code::MissingRecord);
    h.missing_bone=true;
    assert(resolve_original_studio_attachment(h,1,1).code==Code::MissingBone);
    h.missing_bone=false;h.count.reset();
    assert(resolve_original_studio_attachment(h,1,1).code==Code::MissingStudio);
    OriginalStudioNativeAttachmentAdapter adapter(h);
    assert(!adapter.has_animating_interface(1));
    h.count=1;h.records[0].bone_local_matrix.row_major[0]=
        std::numeric_limits<float>::quiet_NaN();
    assert(resolve_original_studio_attachment(h,1,1).code==Code::InvalidMatrix);
    assert(!adapter.get_attachment_pose(1,1));
    h.records[0].bone_local_matrix=pose(0,{0,0,0});
    h.throws=true;
    assert(resolve_original_studio_attachment(h,1,1).code==Code::HostRejected);
    h.throws=false;
    h.bone.row_major[0]=std::numeric_limits<float>::infinity();
    assert(resolve_original_studio_attachment(h,1,1).code==Code::InvalidMatrix);
    h.bone=pose(0,{0,0,0});
    h.bone.row_major[3]=std::numeric_limits<float>::max();
    h.records[0].bone_local_matrix=pose(0,{std::numeric_limits<float>::max(),0,0});
    assert(resolve_original_studio_attachment(h,1,1).code==Code::InvalidMatrix);
}
int main(){test_routes();test_failures();
    std::cout<<"Original SiN CBaseAnimating studio attachment tests PASS\n";}
