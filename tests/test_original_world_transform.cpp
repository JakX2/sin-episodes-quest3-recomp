#include "sin_compat/original_world_transform.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
using namespace sin_compat::jessica;
using Code=WorldRefreshCode;
void near(float a,float b) {assert(std::abs(a-b)<0.002f);}
void vec(TossVec3 a,TossVec3 b){near(a.x,b.x);near(a.y,b.y);near(a.z,b.z);}
WorldTransformSnapshot node(TossVec3 origin,TossVec3 angle,
                            std::optional<WorldEntityId> parent={}) {
    WorldTransformSnapshot v{};
    v.local_origin=origin;v.local_angles=angle;v.parent=parent;v.dirty=true;
    return v;
}
struct Host final:IOriginalWorldTransformHost {
    std::map<WorldEntityId,WorldTransformSnapshot> nodes;
    std::map<WorldEntityId,WorldMatrix3x4> attachments;
    std::vector<std::string> events;
    int attachments_queried=0;
    bool reject_commit=false,reject_notify=false,throw_snapshot=false;
    std::optional<WorldTransformSnapshot> snapshot(WorldEntityId id) override {
        if(throw_snapshot)throw std::runtime_error("host error");
        auto i=nodes.find(id);
        if(i==nodes.end())return std::nullopt;
        return i->second;
    }
    std::optional<WorldMatrix3x4> attachment_parent_matrix(
        WorldEntityId child,WorldEntityId parent,std::uint8_t idx) override {
        assert(nodes.count(child)&&nodes.count(parent)&&idx>0);
        ++attachments_queried;
        auto i=attachments.find(child);
        if(i==attachments.end())return std::nullopt;
        return i->second;
    }
    bool commit_world_transform(WorldEntityId id,
                                const WorldTransformSnapshot& value) override {
        events.emplace_back("commit");
        if(reject_commit)return false;
        nodes.at(id)=value;return true;
    }
    bool on_world_transform_rebuilt(WorldEntityId) override {
        events.emplace_back("notify");return !reject_notify;
    }
};
void test_math() {
    auto m=make_local_angle_matrix({0,90,0},{10,20,30});
    assert(m);
    near(m->row_major[0],0);near(m->row_major[1],-1);
    near(m->row_major[4],1);near(m->row_major[5],0);
    vec({m->row_major[3],m->row_major[7],m->row_major[11]},
        {10,20,30});
    auto local=make_local_angle_matrix({0,45,0},{1,2,3});
    assert(local);
    auto combined=compose_world_matrices(*m,*local);
    assert(combined);
    vec({combined->row_major[3],combined->row_major[7],
         combined->row_major[11]},{8,21,33});
    near(angles_from_world_matrix(*combined)->y,135);
    for(TossVec3 angles: {TossVec3{20,45,30},TossVec3{-30,-90,12},
                          TossVec3{89,45,0},TossVec3{90,30,5}}) {
        auto original=make_local_angle_matrix(angles,{0,0,0});assert(original);
        auto extracted=angles_from_world_matrix(*original);assert(extracted);
        auto rebuilt=make_local_angle_matrix(*extracted,{0,0,0});assert(rebuilt);
        for(int i=0;i<12;++i)near(original->row_major[i],rebuilt->row_major[i]);
    }
    auto malformed=*m;
    malformed.row_major[5]=std::numeric_limits<float>::quiet_NaN();
    assert(!compose_world_matrices(malformed,*local));
    assert(!angles_from_world_matrix(malformed));
}
void test_unparented_and_noop(){
    Host h;h.nodes[1]=node({10,20,30},{10,45,-20});
    auto result=refresh_original_world_transform(h,1);
    assert(result.code==Code::Refreshed);
    vec(result.world_origin,{10,20,30});
    vec(result.world_angles,{10,45,-20});
    assert(!h.nodes[1].dirty);
    assert((h.events==std::vector<std::string>{"commit","notify"}));
    h.events.clear();
    result=refresh_original_world_transform(h,1);
    assert(result.code==Code::Unchanged && h.events.empty());
}
void test_parent_and_zero_angle(){
    Host h;h.nodes[1]=node({10,20,30},{0,90,0});
    h.nodes[2]=node({2,1,3},{0,0,0},1);
    auto r=refresh_original_world_transform(h,2);
    assert(r.code==Code::Refreshed);
    vec(r.world_origin,{9,22,33});
    vec(r.world_angles,{0,90,0});
    assert(h.attachments_queried==0);
    assert((h.events==std::vector<std::string>{"commit","notify",
                                                "commit","notify"}));
    h.nodes[3]=node({1,0,0},{0,45,0},2);
    r=refresh_original_world_transform(h,3);
    assert(r.code==Code::Refreshed);
    vec(r.world_origin,{9,23,33});
    near(r.world_angles.y,135);
}
void test_attachment_and_fallback(){
    Host h;h.nodes[1]=node({10,20,30},{0,0,0});
    h.nodes[2]=node({2,0,0},{0,45,0},1);
    h.nodes[2].parent_attachment=3;
    auto attachment=make_local_angle_matrix({0,90,0},{30,40,50});
    assert(attachment);h.attachments[2]=*attachment;
    auto r=refresh_original_world_transform(h,2);
    assert(r.code==Code::Refreshed);
    vec(r.world_origin,{30,42,50});
    near(r.world_angles.y,135);
    assert(h.attachments_queried==1);
    h.nodes[2].dirty=true;
    h.attachments.clear();
    r=refresh_original_world_transform(h,2);
    assert(r.code==Code::Refreshed);
    vec(r.world_origin,{12,20,30});
    near(r.world_angles.y,45);
    assert(h.attachments_queried==2);
}
void test_invalid(){
    Host h;
    assert(refresh_original_world_transform(h,1).code==Code::MissingEntity);
    h.nodes[1]=node({0,0,0},{0,0,0},2);
    // Original invalid EHANDLE serial falls through to no-parent copy.
    auto stale=refresh_original_world_transform(h,1);
    assert(stale.code==Code::Refreshed);
    vec(stale.world_origin,{0,0,0});
    h.events.clear();h.nodes[1].dirty=true;
    h.nodes[2]=node({1,0,0},{0,0,0},1);
    assert(refresh_original_world_transform(h,1).code==Code::ParentCycle);
    assert(h.events.empty());
    h.nodes[1].parent=1;
    assert(refresh_original_world_transform(h,1).code==Code::ParentCycle);
    h.nodes[1].parent.reset();
    h.nodes[1].local_origin.x=std::numeric_limits<float>::quiet_NaN();
    assert(refresh_original_world_transform(h,1).code==Code::InvalidTransform);
    h.nodes[1].local_origin={0,0,0};h.reject_commit=true;
    assert(refresh_original_world_transform(h,1).code==Code::HostRejected);
    h.reject_commit=false;h.throw_snapshot=true;
    assert(refresh_original_world_transform(h,1).code==Code::HostRejected);
    Host longchain;
    for(int i=1;i<=70;++i)
        longchain.nodes[i]=node({1,0,0},{0,0,0},
            i<70?std::optional<WorldEntityId>(i+1):std::nullopt);
    assert(refresh_original_world_transform(longchain,1).code==Code::DepthLimit);
    assert(longchain.events.empty());
}
int main(){
    test_math();test_unparented_and_noop();test_parent_and_zero_angle();
    test_attachment_and_fallback();test_invalid();
    std::cout<<"Original SiN world-transform cache reconstruction PASS\n";
}
