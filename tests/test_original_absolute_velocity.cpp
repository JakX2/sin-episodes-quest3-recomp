#include "sin_compat/original_absolute_velocity.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
using namespace sin_compat::jessica;
using Code=AbsoluteVelocityCode;
AbsoluteMatrix3x4 identity(){return {{{1,0,0,999,0,1,0,888,0,0,1,777}}};}
AbsoluteMatrix3x4 yaw90(){return {{{0,-1,0,100,1,0,0,200,0,0,1,300}}};}
void match(TossVec3 a,TossVec3 b){
    assert(std::abs(a.x-b.x)<0.001f&&std::abs(a.y-b.y)<0.001f&&
           std::abs(a.z-b.z)<0.001f);
}
AbsoluteVelocitySnapshot node(TossVec3 local,TossVec3 abs,
                             std::optional<AbsoluteEntityId> parent={}){
    AbsoluteVelocitySnapshot s{};s.local=local;s.absolute=abs;
    s.parent=parent;s.entity_to_world=identity();return s;
}
struct Host final:IOriginalAbsoluteVelocityHost {
    std::map<AbsoluteEntityId,AbsoluteVelocitySnapshot> nodes;
    std::vector<std::string> writes;
    bool reject_invalidation=false,fail_transform=false,throw_snapshot=false;
    std::optional<AbsoluteVelocitySnapshot> snapshot(AbsoluteEntityId id) override {
        if(throw_snapshot)throw std::runtime_error("host failure");
        auto it=nodes.find(id);if(it==nodes.end())return std::nullopt;
        return it->second;
    }
    bool invalidate_velocity_dependents(AbsoluteEntityId id,std::uint32_t mask) override {
        assert(nodes.count(id)&&mask==4);writes.emplace_back("invalidate");
        return !reject_invalidation;
    }
    bool clear_absolute_dirty(AbsoluteEntityId id) override {
        writes.emplace_back("clear");nodes.at(id).absolute_dirty=false;return true;
    }
    bool store_absolute_velocity(AbsoluteEntityId id,const TossVec3& v) override {
        writes.emplace_back("absolute");nodes.at(id).absolute=v;return true;
    }
    bool store_local_velocity(AbsoluteEntityId id,const TossVec3& v) override {
        writes.emplace_back("local");nodes.at(id).local=v;return true;
    }
    bool refresh_world_transform(AbsoluteEntityId id) override {
        writes.emplace_back("transform");
        if(fail_transform)return false;
        nodes.at(id).transform_dirty=false;return true;
    }
};
void test_matrix(){
    match(*rotate_local_velocity_to_world(yaw90(),{1,2,3}),{-2,1,3});
    match(*inverse_rotate_world_velocity(yaw90(),{-2,1,3}),{1,2,3});
    match(*rotate_local_velocity_to_world(identity(),{1,2,3}),{1,2,3});
    auto m=identity();m.row_major[0]=2;
    // Original helper uses transpose, not generic matrix inverse.
    match(*inverse_rotate_world_velocity(m,{2,3,4}),{4,3,4});
    m.row_major[0]=std::numeric_limits<float>::quiet_NaN();
    assert(!rotate_local_velocity_to_world(m,{2,3,4}));
}
void test_unparented(){
    Host h;h.nodes[1]=node({0,0,0},{5,6,7});
    assert(set_original_absolute_velocity(h,1,{5,6,7}).code==Code::Unchanged);
    assert(h.writes.empty());
    assert(set_original_absolute_velocity(h,1,{10,20,30}).code==Code::Set);
    match(h.nodes[1].absolute,{10,20,30});
    match(h.nodes[1].local,{10,20,30});
    assert((h.writes==std::vector<std::string>{"invalidate","clear","absolute","local"}));
    h.writes.clear();h.nodes[1].local={7,8,9};h.nodes[1].absolute_dirty=true;
    auto result=refresh_original_absolute_velocity(h,1);
    assert(result.code==Code::Refreshed);
    match(result.value,{7,8,9});
    assert(!h.nodes[1].absolute_dirty);
    assert((h.writes==std::vector<std::string>{"absolute","clear"}));
    h.writes.clear();
    assert(refresh_original_absolute_velocity(h,1).code==Code::Unchanged);
    assert(h.writes.empty());
}
void test_parent(){
    Host h;
    h.nodes[7]=node({10,20,0},{10,20,0});
    h.nodes[7].entity_to_world=yaw90();
    h.nodes[8]=node({0,0,0},{0,0,0},7);
    assert(set_original_absolute_velocity(h,8,{110,20,5}).code==Code::Set);
    match(h.nodes[8].local,{0,-100,5});
    match(h.nodes[8].absolute,{110,20,5});
    h.nodes[8].absolute_dirty=true;h.nodes[8].absolute={0,0,0};
    h.writes.clear();
    auto r=refresh_original_absolute_velocity(h,8);
    assert(r.code==Code::Refreshed);
    match(r.value,{110,20,5});
    h.nodes[7].absolute_dirty=true;
    h.nodes[7].local={3,4,5};h.nodes[7].absolute={-99,-99,-99};
    h.nodes[8].absolute_dirty=true;h.writes.clear();
    r=refresh_original_absolute_velocity(h,8);
    assert(r.code==Code::Refreshed);
    match(r.value,{103,4,10});
    assert((h.writes==std::vector<std::string>{"absolute","clear","absolute","clear"}));
    h.nodes[7].transform_dirty=true;h.writes.clear();
    r=set_original_absolute_velocity(h,8,{203,4,10});
    assert(r.code==Code::Set);
    match(h.nodes[8].local,{0,-200,5});
    assert(h.writes.front()=="transform");
}
void test_nested(){
    Host h;h.nodes[1]=node({1,0,0},{1,0,0});
    h.nodes[1].entity_to_world=yaw90();
    h.nodes[2]=node({1,2,0},{0,0,0},1);
    h.nodes[2].absolute_dirty=true;
    h.nodes[3]=node({3,4,5},{0,0,0},2);
    h.nodes[3].absolute_dirty=true;
    auto r=refresh_original_absolute_velocity(h,3);
    assert(r.code==Code::Refreshed);
    match(h.nodes[2].absolute,{-1,1,0});
    match(r.value,{2,5,5});
}
void test_invalid(){
    Host h;h.nodes[1]=node({1,2,3},{4,5,6},2);
    auto r=set_original_absolute_velocity(h,1,{7,8,9});
    assert(r.code==Code::MissingParent&&h.writes.empty());
    h.nodes[2]=node({0,0,0},{0,0,0},1);
    h.nodes[1].absolute_dirty=true;h.nodes[2].absolute_dirty=true;
    r=refresh_original_absolute_velocity(h,1);
    assert(r.code==Code::ParentCycle&&h.writes.empty());
    h.nodes[1].parent=1;h.nodes[1].absolute_dirty=false;
    r=set_original_absolute_velocity(h,1,{9,10,11});
    assert(r.code==Code::ParentCycle&&h.writes.empty());
    h.nodes[1].parent.reset();h.nodes[1].absolute={0,0,0};
    float nan=std::numeric_limits<float>::quiet_NaN();
    assert(set_original_absolute_velocity(h,1,{nan,0,0}).code==Code::InvalidInput);
    h.throw_snapshot=true;
    assert(set_original_absolute_velocity(h,1,{1,2,3}).code==Code::HostRejected);
    h.throw_snapshot=false;h.reject_invalidation=true;
    assert(set_original_absolute_velocity(h,1,{1,2,3}).code==Code::HostRejected);
    Host deep;
    for(int i=1;i<=70;++i) {
        deep.nodes[i]=node({1,0,0},{0,0,0},
            i<70?std::optional<AbsoluteEntityId>(i+1):std::nullopt);
        deep.nodes[i].absolute_dirty=true;
    }
    assert(refresh_original_absolute_velocity(deep,1).code==Code::DepthLimit);
    assert(deep.writes.empty());
}
int main(){
    test_matrix();test_unparented();test_parent();test_nested();test_invalid();
    std::cout<<"Original SiN parent-aware absolute velocity tests PASS\n";
}
