#include "sin_compat/original_bone_cache.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace sin_compat::jessica;
using Read=OriginalBoneCacheReadCode;
using Code=OriginalBoneTransformCode;
void put_u16(std::vector<std::uint8_t>& b,std::size_t at,std::uint16_t v){
    b.at(at)=std::uint8_t(v);b.at(at+1)=std::uint8_t(v>>8);
}
void put_float(std::vector<std::uint8_t>& b,std::size_t at,float v){
    std::uint32_t raw;std::memcpy(&raw,&v,sizeof(v));
    for(int i=0;i<4;++i)b.at(at+i)=std::uint8_t(raw>>(8*i));
}
std::vector<std::uint8_t> make_cache(){
    std::vector<std::uint8_t> b(0x14+0x20+2*48);
    put_u16(b,0x0e,0x20); // matrix block from cache+0x14
    put_u16(b,0x14,1); // bone 0 maps to cached matrix 1
    put_u16(b,0x16,0xffff); // bone 1 uncached
    put_u16(b,0x18,0); // bone 2 maps to cached matrix 0
    for(int n=0;n<2;++n){
        const std::size_t start=0x34+n*48;
        const float m[]={1,0,0,float(100+n),0,1,0,2,0,0,1,3};
        for(int i=0;i<12;++i)put_float(b,start+i*4,m[i]);
    }
    return b;
}
void eq(float a,float b){assert(std::abs(a-b)<0.01f);}
struct Fake final:IOriginalBoneCacheHost {
    std::optional<std::uint32_t> count=3;
    std::optional<std::vector<std::uint8_t>> cache=make_cache();
    std::optional<WorldMatrix3x4> world;
    bool throws=false;
    std::vector<std::string> calls;
    Fake(){WorldMatrix3x4 w{};w.row_major={1,0,0,9,0,1,0,8,0,0,1,7};world=w;}
    std::optional<std::uint32_t> studio_bone_count(WorldEntityId) override {
        calls.emplace_back("count");if(throws)throw std::runtime_error("host");return count;
    }
    std::optional<std::vector<std::uint8_t>> prepared_bone_cache(WorldEntityId) override {
        calls.emplace_back("cache");return cache;
    }
    std::optional<WorldMatrix3x4> entity_world_matrix(WorldEntityId) override {
        calls.emplace_back("world");return world;
    }
};
void test_parsing(){
    auto cache=make_cache();
    auto a=read_original_bone_cache_matrix(cache,0);
    assert(a.code==Read::Found);eq(a.matrix.row_major[3],101);
    auto b=read_original_bone_cache_matrix(cache,2);
    assert(b.code==Read::Found);eq(b.matrix.row_major[3],100);
    assert(read_original_bone_cache_matrix(cache,1).code==Read::NotCached);
    // Low-level helper intentionally has no studiohdr bone count gate.
    assert(read_original_bone_cache_matrix(cache,3).code==Read::Found);
    assert(read_original_bone_cache_matrix(cache,100).code==Read::InvalidBone);
    std::vector<std::uint8_t> tiny(1);
    assert(read_original_bone_cache_matrix(tiny,0).code==Read::InvalidBuffer);
    put_u16(cache,0x0e,0xfffe);
    assert(read_original_bone_cache_matrix(cache,0).code==Read::InvalidBuffer);
    cache=make_cache();put_float(cache,0x34,float(std::numeric_limits<float>::quiet_NaN()));
    assert(read_original_bone_cache_matrix(cache,2).code==Read::InvalidMatrix);
    cache=make_cache();put_u16(cache,0x14,0x7fff);
    assert(read_original_bone_cache_matrix(cache,0).code==Read::InvalidBuffer);
}
void test_dispatch(){
    Fake host;
    auto r=resolve_original_bone_transform(host,4,0);
    assert(r.code==Code::Cached);eq(r.matrix.row_major[3],101);
    assert((host.calls==std::vector<std::string>{"count","cache"}));
    host.calls.clear();r=resolve_original_bone_transform(host,4,1);
    assert(r.code==Code::EntityWorldFallback);eq(r.matrix.row_major[3],9);
    assert((host.calls==std::vector<std::string>{"count","cache","world"}));
    host.calls.clear();
    assert(resolve_original_bone_transform(host,4,3).code==Code::InvalidBone);
    assert((host.calls==std::vector<std::string>{"count"}));
    host.count.reset();
    assert(resolve_original_bone_transform(host,4,0).code==Code::MissingStudio);
    host.count=3;host.cache.reset();
    assert(resolve_original_bone_transform(host,4,0).code==Code::InvalidCache);
    host.cache=make_cache();host.world.reset();
    assert(resolve_original_bone_transform(host,4,1).code==Code::MissingWorldMatrix);
    host.world=WorldMatrix3x4{};host.world->row_major[0]=
        std::numeric_limits<float>::infinity();
    assert(resolve_original_bone_transform(host,4,1).code==Code::InvalidMatrix);
    host.throws=true;
    assert(resolve_original_bone_transform(host,4,0).code==Code::HostRejected);
}
int main(){test_parsing();test_dispatch();
    std::cout<<"SiN original bone remap and fallback tests PASS\n";}
