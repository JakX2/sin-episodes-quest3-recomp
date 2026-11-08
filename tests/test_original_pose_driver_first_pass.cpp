#include "sin_compat/original_pose_driver_first_pass.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace sin_compat::jessica;
using Code=OriginalPoseFirstPassCode;
void eq(float x,float y){assert(std::abs(x-y)<0.0002f);}
struct Averager final:IOriginalPoseQuaternionMixer {
    int count=0;bool reject=false;bool explode=false;
    std::optional<std::array<float,4>> mix(const std::array<float,4>& a,
        const std::array<float,4>& b,float alpha) override {
        ++count;if(explode)throw std::runtime_error("host callback");
        if(reject)return std::nullopt;
        std::array<float,4> r{};
        for(int i=0;i<4;++i)r[i]=a[i]+alpha*(b[i]-a[i]);
        return r;
    }
};
OriginalPoseDriverCommand data(std::uint32_t type,int track=0){
    OriginalPoseDriverCommand c{};
    c.type=type;c.track_index=track;c.selector=73;
    c.origin={10,20,30};c.quaternion={0,0,0,1};
    c.values={11,12,13};c.secondary=0.7f;c.influence=0.8f;
    c.weight=1.f;c.limit=0.9f;c.optional_payload=0xFABA12u;
    return c;
}
void test_reset_and_simple_commands(){
    OriginalPoseDriverTrack prior{};
    prior.selector=31;prior.type=9;prior.optional_payload=123;
    prior.origin={8,9,10};prior.quaternion={2,2,2,2};
    prior.cap=4;prior.secondary=17;prior.values={2,2,2};
    prior.influence=22;prior.type3_height=444;prior.type3_reset_marker=999;
    auto result=apply_original_pose_driver_first_pass(1,{},20,nullptr,{prior});
    assert(result.successful());
    const auto& t=result.tracks[0];
    assert(t.selector==31&&t.type==9&&t.optional_payload==123);
    eq(t.origin.x,0);eq(t.quaternion[3],0);eq(t.cap,1);
    eq(t.secondary,1);eq(t.values[0],0);eq(t.influence,0);
    eq(t.type3_height,444);eq(t.type3_reset_marker,999);
    auto c=data(3);
    result=apply_original_pose_driver_first_pass(2,{{c}},20);
    assert(result.successful());
    assert(result.commands_seen==1&&result.commands_applied==1);
    assert(result.tracks[0].selector==73&&result.tracks[0].type==3);
    assert(result.tracks[0].optional_payload==0);
    eq(result.tracks[0].origin.x,10);eq(result.tracks[0].quaternion[3],1);
    eq(result.tracks[0].values[0],11);
    eq(result.tracks[0].values[1],13);eq(result.tracks[0].values[2],12);
    eq(result.tracks[0].cap,0.9f);eq(result.tracks[0].secondary,0.7f);
    eq(result.tracks[0].influence,0.8f);
    eq(result.tracks[0].type3_height,33);eq(result.tracks[0].type3_reset_marker,0);
    assert(result.tracks[1].type==0);
    c=data(5,1);c.weight=0.5f;c.optional_payload=0xDEADBEEFu;
    result=apply_original_pose_driver_first_pass(2,{{c}},0);
    assert(result.successful());
    assert(result.tracks[1].type==5&&result.tracks[1].optional_payload==0xDEADBEEFu);
    eq(result.tracks[1].cap,0.45f);eq(result.tracks[1].influence,0.4f);
}
void test_modifiers_and_groups(){
    auto c=data(3);c.limit=0.8f;c.influence=0.8f;
    auto m4=data(4);m4.influence=0.3f;m4.weight=0.5f;m4.limit=0;
    auto m6=data(6);m6.influence=0.6f;m6.limit=0;
    auto ignored=data(2,123456);
    auto r=apply_original_pose_driver_first_pass(1,{{c,ignored},{m4,m6}},5);
    assert(r.successful());assert(r.commands_applied==3);
    assert(r.commands_ignored==1&&r.commands_seen==4);
    eq(r.tracks[0].cap,0.4f);
    eq(r.tracks[0].influence,0.8f*(1.f-0.5f*0.3f));
    m4.limit=1;
    r=apply_original_pose_driver_first_pass(1,{{c,m4}},5);
    assert(r.successful());eq(r.tracks[0].cap,0);
    m6.limit=1;
    r=apply_original_pose_driver_first_pass(1,{{c,m6}},5);
    assert(r.successful());eq(r.tracks[0].cap,0);
    m4=data(4);m4.influence=0.1f;m4.limit=0;
    r=apply_original_pose_driver_first_pass(1,{{m4}},0);
    assert(r.successful());eq(r.tracks[0].cap,0.9f);
    eq(r.tracks[0].influence,0);
}
void test_blending_and_failures(){
    Averager host;
    auto first=data(3);first.quaternion={1,0,0,0};
    auto second=data(5);second.origin={30,40,50};
    second.quaternion={0,0,0,1};second.weight=0.5f;
    second.limit=0.3f;second.influence=0.2f;second.secondary=0.1f;
    second.values={21,22,23};
    auto r=apply_original_pose_driver_first_pass(1,{{first,second}},1,&host);
    assert(r.successful());assert(host.count==1);
    eq(r.tracks[0].origin.x,20);eq(r.tracks[0].origin.y,30);
    eq(r.tracks[0].quaternion[0],0.5f);eq(r.tracks[0].quaternion[3],0.5f);
    eq(r.tracks[0].values[0],16);eq(r.tracks[0].values[1],18);
    eq(r.tracks[0].values[2],17);
    eq(r.tracks[0].cap,0.3f);eq(r.tracks[0].secondary,0.4f);
    eq(r.tracks[0].influence,0.5f);
    assert(r.tracks[0].type==5);
    assert(apply_original_pose_driver_first_pass(1,{{first,second}},1).code==
           Code::MissingQuaternionMixer);
    host.reject=true;
    assert(apply_original_pose_driver_first_pass(1,{{first,second}},1,&host).code==
           Code::QuaternionMixRejected);
    host.reject=false;host.explode=true;
    assert(apply_original_pose_driver_first_pass(1,{{first,second}},1,&host).code==
           Code::HostRejected);
    auto bad=data(3,-1);
    assert(apply_original_pose_driver_first_pass(1,{{bad}},1).code==Code::InvalidTrack);
    bad=data(3,2);
    assert(apply_original_pose_driver_first_pass(1,{{bad}},1).code==Code::InvalidTrack);
    bad=data(3);bad.limit=std::numeric_limits<float>::quiet_NaN();
    assert(apply_original_pose_driver_first_pass(1,{{bad}},1).code==Code::InvalidFloat);
    assert(apply_original_pose_driver_first_pass(1,{},
        std::numeric_limits<float>::infinity()).code==Code::InvalidFloat);
    assert(apply_original_pose_driver_first_pass(1,{},1,nullptr,
        std::vector<OriginalPoseDriverTrack>(2)).code==Code::InvalidTrack);
    assert(apply_original_pose_driver_first_pass(5000,{},1).code==Code::InvalidTrack);
}
int main(){test_reset_and_simple_commands();test_modifiers_and_groups();
    test_blending_and_failures();
    std::cout<<"SiN initial pose-driver command loop PASS\n";}
