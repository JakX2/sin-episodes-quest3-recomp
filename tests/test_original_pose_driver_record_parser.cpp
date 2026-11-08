#include "sin_compat/original_pose_driver_first_pass.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <vector>
using namespace sin_compat::jessica;
void put32(std::vector<std::uint8_t>& b,std::size_t at,std::uint32_t x){
    for(int i=0;i<4;++i)b.at(at+i)=static_cast<std::uint8_t>(x>>(8*i));
}
void putf(std::vector<std::uint8_t>& b,std::size_t at,float v){
    std::uint32_t bits;std::memcpy(&bits,&v,4);put32(b,at,bits);
}
void eq(float a,float b){assert(std::abs(a-b)<0.00001f);}
int main(){
    std::vector<std::uint8_t> raw(0xf8,0);
    assert(!parse_original_pose_command(raw,0xf7));
    assert(!parse_original_pose_command(raw,raw.size()));
    assert(!parse_original_pose_command(raw,0xf8-0x7c+1));
    const std::size_t s=0x7c;
    put32(raw,s+0x04,5);put32(raw,s+0x08,0x49);
    put32(raw,s+0x10,2);put32(raw,s+0x60,0xCAFEBABE);
    putf(raw,s+0x14,8);putf(raw,s+0x18,9);putf(raw,s+0x1c,10);
    putf(raw,s+0x20,11);putf(raw,s+0x24,12);putf(raw,s+0x28,13);
    putf(raw,s+0x2c,0.1f);putf(raw,s+0x30,0.2f);
    putf(raw,s+0x34,0.3f);putf(raw,s+0x38,0.4f);
    putf(raw,s+0x50,0.7f);putf(raw,s+0x54,0.8f);
    putf(raw,s+0x58,0.9f);putf(raw,s+0x5c,1.0f);
    auto c=parse_original_pose_command(raw,s);
    assert(c);assert(c->type==5&&c->selector==0x49);
    assert(c->track_index==2&&c->optional_payload==0xCAFEBABE);
    eq(c->values[2],10);eq(c->origin.z,13);
    eq(c->quaternion[3],0.4f);eq(c->secondary,0.7f);
    eq(c->influence,0.8f);eq(c->weight,0.9f);eq(c->limit,1);
    auto r=apply_original_pose_driver_first_pass(3,{{*c}},3);
    assert(r.successful());assert(r.tracks[2].type==5);
    assert(r.tracks[2].optional_payload==0xCAFEBABE);
    assert(!parse_original_pose_command(std::vector<std::uint8_t>(0x7b)));
    put32(raw,s+0x10,0xffffffff);
    c=parse_original_pose_command(raw,s);assert(c&&c->track_index==-1);
    r=apply_original_pose_driver_first_pass(3,{{*c}},3);
    assert(r.code==OriginalPoseFirstPassCode::InvalidTrack);
    putf(raw,s+0x20,std::numeric_limits<float>::infinity());
    c=parse_original_pose_command(raw,s);assert(c);
    r=apply_original_pose_driver_first_pass(3,{{*c}},3);
    assert(r.code==OriginalPoseFirstPassCode::InvalidTrack);
    std::cout<<"SiN original 124-byte pose command record parser PASS\n";
}
