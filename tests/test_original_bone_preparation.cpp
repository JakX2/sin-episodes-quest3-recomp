#include "sin_compat/original_bone_preparation.hpp"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
using namespace sin_compat::jessica;
using Update=OriginalBoneUpdateCode;
using Prep=OriginalBonePrepareCode;
void put16(std::vector<std::uint8_t>& v,std::size_t at,std::uint16_t x){
    v.at(at)=std::uint8_t(x);v.at(at+1)=std::uint8_t(x>>8);
}
void put32(std::vector<std::uint8_t>& v,std::size_t at,std::uint32_t x){
    for(int i=0;i<4;++i)v.at(at+i)=std::uint8_t(x>>(8*i));
}
void putfloat(std::vector<std::uint8_t>& v,std::size_t at,float f){
    std::uint32_t bits;std::memcpy(&bits,&f,4);put32(v,at,bits);
}
std::vector<std::uint8_t> fixture(float stamp=5.0f,std::uint32_t tag=0x300){
    std::vector<std::uint8_t> v(0x14+0x30+2*48,0);
    putfloat(v,0,stamp);put32(v,4,tag);
    put16(v,0x0c,2); // compact matrix count
    put16(v,0x0e,0x30); // compact matrix offset
    put16(v,0x10,0x10); // reverse bone mapping offset
    put16(v,0x14,1); // forward bone 0 -> compact index 1
    put16(v,0x16,0xffff); // bone 1 isn't cached
    put16(v,0x18,0); // bone 2 -> compact 0
    put16(v,0x24,2); // reverse compact index 0 -> source bone 2
    put16(v,0x26,0); // reverse compact index 1 -> source bone 0
    return v;
}
WorldMatrix3x4 pose(float x){
    WorldMatrix3x4 m{};m.row_major={1,0,0,x,0,1,0,x+1,0,0,1,x+2};
    return m;
}
void eq(float a,float b){assert(std::abs(a-b)<0.0001f);}
void test_freshness(){
    assert(original_bone_cache_is_fresh(5.0f,5.0f));
    assert(original_bone_cache_is_fresh(5.05f,5.0f));
    assert(!original_bone_cache_is_fresh(5.5f,5.0f));
    assert(original_bone_cache_is_fresh(4.0f,5.0f)); // future cached stamp
    const float nan=std::numeric_limits<float>::quiet_NaN();
    assert(!original_bone_cache_is_fresh(nan,5));
    assert(!original_bone_cache_is_fresh(5,nan));
    assert(!original_bone_cache_is_fresh(std::numeric_limits<float>::infinity(),5));
}
void test_update(){
    auto original=fixture();
    auto out=update_original_compact_bone_cache(original,
        {pose(11),pose(22),pose(33)},6.0f);
    assert(out.successful());
    assert(original!=out.bytes);
    auto b0=read_original_bone_cache_matrix(out.bytes,0);
    auto b2=read_original_bone_cache_matrix(out.bytes,2);
    eq(b0.matrix.row_major[3],11); // forward bone 0 -> cached matrix 1
    eq(b2.matrix.row_major[3],33); // forward bone 2 -> cached matrix 0
    assert(read_original_bone_cache_matrix(out.bytes,1).code==
           OriginalBoneCacheReadCode::NotCached);
    // source order comes from reverse table, not the forward remap
    assert(out.bytes[0x24]==2 && out.bytes[0x26]==0);
    assert(original_bone_cache_is_fresh(6.0f,6.0f));
    original=fixture();
    put16(original,0x24,0xffff);
    assert(update_original_compact_bone_cache(original,
        {pose(11),pose(22),pose(33)},6).code==Update::InvalidSourceIndex);
    put16(original,0x24,5);
    assert(update_original_compact_bone_cache(original,
        {pose(11),pose(22),pose(33)},6).code==Update::InvalidSourceIndex);
    original=fixture();
    put16(original,0x10,0xffff);
    assert(update_original_compact_bone_cache(original,
        {pose(11),pose(22),pose(33)},6).code==Update::InvalidBuffer);
    original=fixture();put16(original,0x0e,0xffff);
    assert(update_original_compact_bone_cache(original,
        {pose(11),pose(22),pose(33)},6).code==Update::InvalidBuffer);
    original=fixture();original.resize(5);
    assert(update_original_compact_bone_cache(original,
        {pose(11),pose(22),pose(33)},6).code==Update::InvalidBuffer);
    original=fixture();auto bad=pose(33);
    bad.row_major[4]=std::numeric_limits<float>::quiet_NaN();
    assert(update_original_compact_bone_cache(original,
        {pose(11),pose(22),bad},6).code==Update::InvalidMatrix);
    assert(update_original_compact_bone_cache(original,
        {pose(11),pose(22),pose(33)},std::numeric_limits<float>::infinity()).code
        ==Update::InvalidTime);
    original=fixture();put16(original,0x0c,0);
    auto no_bones=update_original_compact_bone_cache(original,{},7);
    assert(no_bones.successful());
    assert(no_bones.bytes.size()==original.size());
}
struct Fake final:IOriginalBonePreparationHost {
    OriginalBonePreparationSnapshot current{6.0f,3,OriginalBoneCacheToken{42}};
    std::map<OriginalBoneCacheToken,std::vector<std::uint8_t>> caches;
    std::vector<WorldMatrix3x4> poses{pose(11),pose(22),pose(33)};
    std::vector<std::string> calls;
    bool refuse_calculate=false,refuse_store=false,refuse_allocate=false;
    bool throw_state=false;
    Fake(){caches[42]=fixture();}
    std::optional<OriginalBonePreparationSnapshot> state(WorldEntityId) override {
        calls.emplace_back("state");
        if(throw_state)throw std::runtime_error("state");
        return current;
    }
    std::optional<std::vector<std::uint8_t>> resolve_cache(OriginalBoneCacheToken key) override {
        calls.emplace_back("resolve");
        auto it=caches.find(key);if(it==caches.end())return std::nullopt;
        return it->second;
    }
    bool release_cache(OriginalBoneCacheToken key) override {
        calls.emplace_back("release");caches.erase(key);return true;
    }
    bool set_entity_cache_handle(WorldEntityId,
        std::optional<OriginalBoneCacheToken> key) override {
        calls.emplace_back(key?"set_handle":"clear_handle");
        current.stored_handle=key;return true;
    }
    std::optional<std::vector<WorldMatrix3x4>> calculate_bones(
        WorldEntityId,std::uint32_t mask) override {
        assert(mask==0x300);calls.emplace_back("calculate");
        if(refuse_calculate)return std::nullopt;
        return poses;
    }
    bool store_cache_bytes(OriginalBoneCacheToken key,
                           const std::vector<std::uint8_t>& data) override {
        calls.emplace_back("store");
        if(refuse_store)return false;
        caches[key]=data;return true;
    }
    std::optional<OriginalBoneCacheToken> allocate_cache(
        WorldEntityId,std::uint32_t count,
        const std::vector<WorldMatrix3x4>& matrices,float now,
        std::uint32_t mask) override {
        calls.emplace_back("allocate");assert(mask==0x300&&count==3);
        if(refuse_allocate)return std::nullopt;
        auto bytes=fixture(now,mask);
        auto r=update_original_compact_bone_cache(bytes,matrices,now);
        assert(r.successful());caches[99]=r.bytes;
        return OriginalBoneCacheToken{99};
    }
};
void test_prepare(){
    Fake f;
    auto r=prepare_original_bone_cache(f,1);
    assert(r.code==Prep::Updated&&r.handle==OriginalBoneCacheToken{42});
    eq(read_original_bone_cache_matrix(r.bytes,2).matrix.row_major[3],33);
    assert((f.calls==std::vector<std::string>{"state","resolve","calculate","store"}));
    f.calls.clear();f.current.current_time=6.01f;
    r=prepare_original_bone_cache(f,1);
    assert(r.code==Prep::Fresh);
    assert((f.calls==std::vector<std::string>{"state","resolve"}));
    // Timestamp freshness check comes BEFORE type check in the original.
    f.calls.clear();put32(f.caches[42],4,0x123);
    r=prepare_original_bone_cache(f,1);
    assert(r.code==Prep::Fresh);
    assert((f.calls==std::vector<std::string>{"state","resolve"}));
    f.calls.clear();f.current.current_time=7;
    r=prepare_original_bone_cache(f,1);
    assert(r.code==Prep::Allocated&&r.handle==OriginalBoneCacheToken{99});
    assert((f.calls==std::vector<std::string>{"state","resolve","release",
        "clear_handle","calculate","allocate","set_handle","resolve"}));
    assert(f.current.stored_handle==OriginalBoneCacheToken{99});
    assert(f.caches.count(42)==0);
    eq(read_original_bone_cache_matrix(r.bytes,0).matrix.row_major[3],11);
    f.calls.clear();f.current.current_time=8;f.current.stored_handle=std::nullopt;
    r=prepare_original_bone_cache(f,1);
    assert(r.code==Prep::Allocated);
    assert((f.calls==std::vector<std::string>{"state","calculate","allocate","set_handle","resolve"}));
    f.calls.clear();f.current.stored_handle=123;
    r=prepare_original_bone_cache(f,1);
    assert(r.code==Prep::Allocated);
    assert((f.calls==std::vector<std::string>{"state","resolve","calculate","allocate","set_handle","resolve"}));
}
void test_failures(){
    Fake f;f.current.studio_bone_count.reset();
    auto r=prepare_original_bone_cache(f,1);
    assert(r.code==Prep::MissingStudio);
    f.current.studio_bone_count=3;
    f.current.current_time=std::numeric_limits<float>::quiet_NaN();
    assert(prepare_original_bone_cache(f,1).code==Prep::InvalidTime);
    f.current.current_time=6;f.caches[42].resize(4);
    assert(prepare_original_bone_cache(f,1).code==Prep::InvalidCache);
    f.caches[42]=fixture();putfloat(f.caches[42],0,
        std::numeric_limits<float>::quiet_NaN());
    assert(prepare_original_bone_cache(f,1).code==Prep::InvalidTime);
    f.caches[42]=fixture();f.refuse_calculate=true;
    assert(prepare_original_bone_cache(f,1).code==Prep::InvalidPose);
    f.refuse_calculate=false;f.refuse_store=true;
    assert(prepare_original_bone_cache(f,1).code==Prep::HostRejected);
    f.refuse_store=false;f.current.stored_handle.reset();f.refuse_allocate=true;
    assert(prepare_original_bone_cache(f,1).code==Prep::HostRejected);
    f.throw_state=true;
    assert(prepare_original_bone_cache(f,1).code==Prep::HostRejected);
}
int main(){test_freshness();test_update();test_prepare();test_failures();
    std::cout<<"SiN bone cache preparation and reverse mapping tests PASS\n";}
